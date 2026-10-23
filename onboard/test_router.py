"""Host-only routing and deterministic fault tests for the Pi service."""

from __future__ import annotations

import struct
import tempfile
import unittest
from pathlib import Path
import queue
import io
from contextlib import redirect_stderr

from boat.ids import COMPONENT_GROUND, COMPONENT_MOTOR, SYSTEM_BOAT, SYSTEM_GROUND
from protocol import codec

from onboard import BoatRouter, IMURegisterConfig, ServiceConfig, SimulatedBus
from onboard.__main__ import _enqueue_stdout


class Clock:
    def __init__(self):
        self.value = 10.0

    def __call__(self):
        return self.value

    def advance(self, seconds=0.02):
        self.value += seconds


def ground_request(request_id=1, opcode=2, delta=500, session=123, actuator=1):
    return codec.encode("ACTUATOR_REQUEST", {
        "request_id": request_id, "ground_session": session, "delta_cdeg": delta,
        "timeout_ms": 2000, "max_rate_cdeg_s": 500,
        "target_system": SYSTEM_BOAT, "target_component": COMPONENT_MOTOR,
        "actuator": actuator, "opcode": opcode, "control_source": 3,
    }, system=SYSTEM_GROUND, component=COMPONENT_GROUND, sequence=request_id)


def health(boot=55, feedback=1, fault_flags=0):
    return codec.encode("NODE_HEALTH", {
        "boot_id": boot, "time_boot_ms": 100, "fault_flags": fault_flags,
        "sensor_valid_fields": 0, "queue_drops": 0, "reset_reason": 0,
        "feedback_valid": feedback,
    }, system=SYSTEM_BOAT, component=COMPONENT_MOTOR)


def status(boot_ms=100, actuator=1, result=0, control_source=3, fault_flags=0, enabled=0):
    return codec.encode("ACTUATOR_STATUS", {
        "time_boot_ms": boot_ms, "command_id": 0, "requested_position_cdeg": 0,
        "measured_position_cdeg": 0, "velocity_cdeg_s": 0, "fault_flags": fault_flags,
        "actuator": actuator, "control_source": control_source, "result": result, "enabled": enabled,
    }, system=SYSTEM_BOAT, component=COMPONENT_MOTOR)


class RouterTests(unittest.TestCase):
    def setUp(self):
        self.clock = Clock()
        self.bus = SimulatedBus()
        self.output = []
        self.events = []
        self.router = BoatRouter(self.bus, clock=self.clock, on_frame=self.output.append,
                                 on_event=self.events.append)
        self.router._observe_node_message(codec.decode(health()), health())
        self.router._observe_node_message(codec.decode(status()), status())
        self.router._normal.clear()
        self.router._poll_index = 1
        self.bus.operations.clear()

    def test_step_routes_delta_unchanged_with_pi_source_and_ground_correlation(self):
        original = ground_request()
        self.assertTrue(self.router.submit_frame(original))
        self.router.step()
        self.router.step()
        write = next(op for op in self.bus.operations if op.kind == "write" and op.address == 0x08)
        decoded = codec.decode(self.router._decode_mailbox(write.data))
        self.assertEqual((decoded["system"], decoded["component"]), (1, 1))
        self.assertEqual(decoded["fields"], codec.decode(original)["fields"])
        self.assertEqual(decoded["fields"]["delta_cdeg"], 500)

    def test_allowlist_source_and_crc_validation(self):
        unsupported = codec.encode("HEARTBEAT", {"type": 18, "autopilot": 0,
            "base_mode": 0, "custom_mode": 0, "system_status": 4}, system=255, component=190)
        self.assertFalse(self.router.submit_frame(unsupported))
        damaged = bytearray(ground_request())
        damaged[-1] ^= 0x80
        self.assertFalse(self.router.submit_frame(bytes(damaged)))
        self.assertTrue(any(e.kind == "rejected" for e in self.events))

    def test_step_denied_when_feedback_invalid(self):
        self.bus.inject_frame(0x08, health(feedback=0))
        self.router._poll_node(0x08)
        self.assertFalse(self.router.submit_frame(ground_request()))
        self.assertFalse(any(op.kind == "write" and op.address == 0x08 for op in self.bus.operations))

    def test_stop_preempts_queued_step_and_is_distinct(self):
        self.router.submit_frame(ground_request(request_id=1, opcode=1, delta=100))
        self.router.submit_frame(ground_request(request_id=2, opcode=3, delta=0))
        self.router.step()
        self.router.step()
        writes = [op for op in self.bus.operations if op.kind == "write"]
        self.assertEqual(len(writes), 1)
        write = writes[0]
        fields = codec.decode(self.router._decode_mailbox(write.data))["fields"]
        self.assertEqual((fields["request_id"], fields["opcode"], fields["delta_cdeg"]), (2, 3, 0))

    def test_fair_polling_services_both_peers_and_configured_imu(self):
        cfg = ServiceConfig(imu=IMURegisterConfig(
            address=0x68, register=0x3B, accel_scale_mg_per_count=1,
            gyro_scale_mrad_s_per_count=1))
        bus = SimulatedBus(imu_samples={0x68: struct.pack("<6h", 0, 1, 1000, 2, 3, 4)})
        r = BoatRouter(bus, cfg, clock=self.clock, on_frame=self.output.append)
        for _ in range(3):
            r.step()
            self.clock.advance(0.2)
        polls = [op.address for op in bus.operations if op.kind in ("read", "imu")]
        self.assertEqual(polls[:3], [0x08, 0x09, 0x68])
        sample = next(codec.decode(frame) for frame in reversed(self.output)
                      if codec.decode(frame)["name"] == "SCALED_IMU")
        self.assertEqual(sample["name"], "SCALED_IMU")
        self.assertEqual(sample["fields"]["zacc"], 1000)

    def test_ambiguous_write_is_unknown_and_never_replayed(self):
        self.router.submit_frame(ground_request(opcode=3, delta=0))
        self.bus.inject_fault("write")
        self.router.step()
        writes = [op for op in self.bus.operations if op.kind == "write" and op.address == 0x08]
        self.assertEqual(len(writes), 1)
        self.assertTrue(any(e.kind == "bus_error" and "not retried" in e.detail for e in self.events))
        self.assertTrue(any(codec.decode(f)["name"] == "REQUEST_RESULT" for f in self.output))
        self.router.step()
        self.assertEqual(len([op for op in self.bus.operations if op.kind == "write" and op.address == 0x08]), 1)

    def test_mailbox_rejects_concatenated_or_nonzero_padded_frames(self):
        frame = codec.encode("HEARTBEAT", {"type": 18, "autopilot": 0,
            "base_mode": 0, "custom_mode": 0, "system_status": 4})
        with self.assertRaises(ValueError):
            self.router._decode_mailbox(frame.ljust(64, b"\0")[:-1] + b"\x01")
        with self.assertRaises(ValueError):
            self.router._decode_mailbox((frame + frame).ljust(64, b"\0"))

    def test_session_change_cannot_replace_pending_owner_motion(self):
        self.assertTrue(self.router.submit_frame(ground_request(request_id=1, session=123)))
        queued = self.router._normal.pop()
        self.router._write(queued)
        current_queue = [(item.key, item.frame) for item in self.router._normal]
        self.assertFalse(self.router.submit_frame(ground_request(request_id=1, session=456)))
        self.assertEqual(self.router._sessions[(SYSTEM_GROUND, COMPONENT_GROUND)], 123)
        self.assertEqual([(item.key, item.frame) for item in self.router._normal], current_queue)

    def test_explicit_stop_from_new_session_supersedes_unsent_motion(self):
        self.assertTrue(self.router.submit_frame(ground_request(request_id=1, session=123)))
        self.assertTrue(self.router.submit_frame(ground_request(request_id=1, opcode=3, delta=0, session=456)))
        self.router.step()
        write = next(op for op in self.bus.operations if op.kind == "write" and op.address == 0x08)
        moved = codec.decode(self.router._decode_mailbox(write.data))["fields"]
        self.assertEqual((moved["request_id"], moved["ground_session"], moved["opcode"]), (1, 456, 3))
        old_results = [codec.decode(frame)["fields"] for frame in self.output
                       if codec.decode(frame)["name"] == "REQUEST_RESULT"]
        self.assertTrue(any(result["ground_session"] == 123 and result["result"] == 4
                            for result in old_results))

    def test_full_queue_admits_stop_and_evicts_routine_traffic(self):
        router = BoatRouter(self.bus, ServiceConfig(queue_limit=4), clock=self.clock,
                            on_frame=self.output.append, on_event=self.events.append)
        router.node_health[(SYSTEM_BOAT, COMPONENT_MOTOR)] = (self.clock(), 55, 1, 0)
        router.node_boot_ids[(SYSTEM_BOAT, COMPONENT_MOTOR)] = 55
        router.motor_status_at[1] = self.clock()
        router.motor_status[1] = codec.decode(status())["fields"]
        router.motor_reconcile_required = False
        for frame in (status(), health(), codec.encode("LINK_STATUS", {
                "time_boot_ms": 1, "rx_packets": 0, "tx_packets": 0,
                "rx_errors": 0, "tx_errors": 0, "address": 0,
                "rssi_dbm": 0, "snr_cdb": 0, "link_type": 1,
                "connected": 1}, system=SYSTEM_BOAT, component=COMPONENT_MOTOR),
                codec.encode("SCALED_IMU", {
                    "time_boot_ms": 1, "xacc": 0, "yacc": 0, "zacc": 0,
                    "xgyro": 0, "ygyro": 0, "zgyro": 0, "xmag": 0,
                    "ymag": 0, "zmag": 0, "temperature": 0},
                    system=SYSTEM_BOAT, component=COMPONENT_MOTOR)):
            router._observe_node_message(codec.decode(frame), frame)
        self.assertEqual(router.queued_writes, 4)
        self.assertFalse(router.submit_frame(ground_request(request_id=2, opcode=1, delta=100)))
        self.assertFalse(any(item.key and item.key[3] == 2 for item in router._normal))
        self.assertFalse(any("request received; motor processing is pending" in event.detail
                             and event.frame and codec.decode(event.frame)["fields"]["request_id"] == 2
                             for event in self.events))
        self.assertTrue(router.submit_frame(ground_request(request_id=1, opcode=3, delta=0)))
        self.assertEqual(router.queued_writes, 5)
        self.assertEqual(len(router._stops), 1)
        router.step()
        written = next(op for op in self.bus.operations if op.kind == "write")
        self.assertEqual(codec.decode(router._decode_mailbox(written.data))["fields"]["opcode"], 3)

    def test_stop_reserve_admits_when_bounded_request_ledger_is_full(self):
        router = BoatRouter(self.bus, ServiceConfig(queue_limit=4), clock=self.clock,
                            on_frame=self.output.append, on_event=self.events.append)
        router._observe_node_message(codec.decode(health()), health())
        router._observe_node_message(codec.decode(status()), status())
        for request_id in range(100, 116):
            key = (SYSTEM_GROUND, COMPONENT_GROUND, 123, request_id)
            fields = codec.decode(ground_request(request_id=request_id))["fields"]
            router._remember_request(key, fields, "ssh")
            router._make_result(key, 55, 0, 0, 1, "pending")
        self.assertEqual(len(router._requests), 16)
        self.assertTrue(router.submit_frame(ground_request(request_id=1, opcode=3, delta=0)))
        self.assertEqual(len(router._stops), 1)
        self.assertEqual(len(router._requests), 17)

    def test_boot_change_purges_queued_motion_until_health_and_status_reconcile(self):
        self.router._normal.clear()
        self.assertTrue(self.router.submit_frame(ground_request(request_id=1, opcode=1, delta=100)))
        changed = codec.decode(health(boot=56))
        self.router._observe_node_message(changed, health(boot=56))
        self.assertFalse(any(item.key and item.key[3] == 1 for item in self.router._normal))
        self.assertTrue(self.router.motor_reconcile_required)
        self.assertFalse(self.router.submit_frame(ground_request(request_id=2, opcode=1, delta=100)))
        self.router._observe_node_message(codec.decode(status(200)), status(200))
        self.assertFalse(self.router.motor_reconcile_required)

    def test_delayed_retired_session_cannot_replace_current_session(self):
        self.assertTrue(self.router.submit_frame(ground_request(request_id=1, session=123)))
        self.router._write(self.router._normal.pop())
        terminal = codec.encode("REQUEST_RESULT", {
            "ground_session": 123, "request_id": 1, "motor_boot_id": 55,
            "local_command_id": 1, "result": 1, "state": 4,
            "target_system": SYSTEM_GROUND, "target_component": COMPONENT_GROUND,
        }, system=SYSTEM_BOAT, component=COMPONENT_MOTOR)
        self.router._observe_node_message(codec.decode(terminal), terminal)
        self.assertTrue(self.router.submit_frame(ground_request(request_id=1, session=456)))
        current_queue = [(item.key, item.frame) for item in self.router._normal if item.key]
        self.assertFalse(self.router.submit_frame(ground_request(request_id=2, session=123)))
        self.assertEqual(self.router._sessions[(SYSTEM_GROUND, COMPONENT_GROUND)], 456)
        self.assertEqual([(item.key, item.frame) for item in self.router._normal if item.key], current_queue)

    def test_motor_boot_change_retires_session_and_preserves_old_boot_unknown(self):
        self.assertTrue(self.router.submit_frame(ground_request(request_id=1, opcode=3, delta=0)))
        changed = health(boot=56)
        self.router._observe_node_message(codec.decode(changed), changed)
        self.assertIn((SYSTEM_GROUND, COMPONENT_GROUND, 123), self.router._retired_sessions)
        self.assertFalse(self.router.submit_frame(ground_request(request_id=2, session=123)))
        self.assertTrue(self.router.submit_frame(ground_request(request_id=3, opcode=3, delta=0, session=123)))
        unknown = [codec.decode(frame) for frame in self.output if codec.decode(frame)["name"] == "REQUEST_RESULT"]
        self.assertTrue(any(msg["fields"]["request_id"] == 1 and
                            msg["fields"]["motor_boot_id"] == 55 and msg["fields"]["result"] == 0
                            for msg in unknown))

    def test_actuator_readiness_uses_selected_fault_free_status_not_enabled_bit(self):
        self.assertTrue(self.router.submit_frame(ground_request(actuator=1)))
        self.assertFalse(self.router.submit_frame(ground_request(request_id=2, actuator=2)))
        frame = status(actuator=2, enabled=0)
        self.router._observe_node_message(codec.decode(frame), frame)
        self.assertTrue(self.router.submit_frame(ground_request(request_id=3, actuator=2)))
        for rid, status_frame in (
            (4, status(actuator=2, result=1)),
            (5, status(actuator=2, result=0, control_source=1)),
            (6, status(actuator=2, result=0, fault_flags=1)),
        ):
            self.router._observe_node_message(codec.decode(status_frame), status_frame)
            self.assertFalse(self.router.submit_frame(ground_request(request_id=rid, actuator=2)))

    def test_health_faults_deny_movement_even_with_valid_feedback(self):
        frame = health(fault_flags=1)
        self.router._observe_node_message(codec.decode(frame), frame)
        self.assertFalse(self.router.submit_frame(ground_request()))

    def test_observed_protected_result_coalesces_by_request_at_full_queue(self):
        router = BoatRouter(self.bus, ServiceConfig(queue_limit=4), clock=self.clock,
                            on_frame=self.output.append, on_event=self.events.append)
        key = (SYSTEM_GROUND, COMPONENT_GROUND, 123, 1)
        router._remember_request(key, codec.decode(ground_request())["fields"], "comms")
        router._observe_node_message(codec.decode(status()), status())
        router._observe_node_message(codec.decode(health()), health())
        link = codec.encode("LINK_STATUS", {
            "time_boot_ms": 1, "rx_packets": 0, "tx_packets": 0,
            "rx_errors": 0, "tx_errors": 0, "address": 0,
            "rssi_dbm": 0, "snr_cdb": 0, "link_type": 1,
            "connected": 1}, system=SYSTEM_BOAT, component=COMPONENT_MOTOR)
        router._observe_node_message(codec.decode(link), link)
        for state, result in ((1, 0), (4, 1)):
            frame = codec.encode("REQUEST_RESULT", {
                "ground_session": 123, "request_id": 1, "motor_boot_id": 55,
                "local_command_id": 1, "result": result, "state": state,
                "target_system": SYSTEM_GROUND, "target_component": COMPONENT_GROUND,
            }, system=SYSTEM_BOAT, component=COMPONENT_MOTOR)
            router._observe_node_message(codec.decode(frame), frame)
        self.assertEqual(len(router._normal), 4)
        result_entry = next(item for item in router._normal if item.key == key)
        self.assertEqual(codec.decode(result_entry.frame)["fields"]["state"], 4)

    def test_mismatched_motor_result_is_not_forwarded(self):
        result = codec.encode("REQUEST_RESULT", {
            "ground_session": 123, "request_id": 999, "motor_boot_id": 55,
            "local_command_id": 1, "result": 1, "state": 4,
            "target_system": 255, "target_component": 191,
        }, system=SYSTEM_BOAT, component=COMPONENT_MOTOR)
        before = list(self.output)
        self.router._observe_node_message(codec.decode(result), result)
        self.assertEqual(self.output, before)

    def test_addressed_ping_reply_never_triggers_pi_ping_response(self):
        reply = codec.encode("PING", {"time_usec": 123, "seq": 7,
            "target_system": SYSTEM_GROUND, "target_component": COMPONENT_GROUND},
            system=SYSTEM_BOAT, component=COMPONENT_MOTOR)
        self.router._observe_node_message(codec.decode(reply), reply)
        pi_responses = [frame for frame in self.output if codec.decode(frame)["name"] == "PING"
                        and codec.decode(frame)["system"] == 1 and codec.decode(frame)["component"] == 1]
        self.assertEqual(pi_responses, [])

    def test_queued_movement_is_rejected_after_health_age_timeout(self):
        self.router._normal.clear()
        self.assertTrue(self.router.submit_frame(ground_request(request_id=1, opcode=1, delta=100)))
        write = self.router._normal.pop()
        self.clock.advance(5.1)
        self.router._write(write)
        motor_writes = [op for op in self.bus.operations if op.kind == "write" and op.address == 0x08]
        self.assertEqual(motor_writes, [])
        self.assertTrue(any(event.kind == "stale" for event in self.events))

    def test_bridge_stdout_queue_is_bounded_and_prioritizes_terminal_results(self):
        out = queue.Queue(maxsize=2)
        telemetry = codec.encode("SCALED_IMU", {
            "time_boot_ms": 1, "xacc": 0, "yacc": 0, "zacc": 0,
            "xgyro": 0, "ygyro": 0, "zgyro": 0, "xmag": 0,
            "ymag": 0, "zmag": 0, "temperature": 0}, system=1, component=1)
        result = codec.encode("REQUEST_RESULT", {
            "ground_session": 123, "request_id": 5, "motor_boot_id": 55,
            "local_command_id": 1, "result": 1, "state": 4,
            "target_system": 255, "target_component": 190,
        }, system=1, component=1)
        _enqueue_stdout(out, telemetry)
        _enqueue_stdout(out, telemetry)
        _enqueue_stdout(out, result)
        self.assertEqual(out.qsize(), 2)
        self.assertEqual(codec.decode(out.get_nowait())["name"], "REQUEST_RESULT")

    def test_bridge_stdout_overflow_reports_unknown_result_delivery(self):
        out = queue.Queue(maxsize=1)
        def terminal(request_id):
            return codec.encode("REQUEST_RESULT", {
                "ground_session": 123, "request_id": request_id, "motor_boot_id": 55,
                "local_command_id": request_id, "result": 1, "state": 4,
                "target_system": 255, "target_component": 190,
            }, system=1, component=1)
        self.assertTrue(_enqueue_stdout(out, terminal(1)))
        stderr = io.StringIO()
        with redirect_stderr(stderr):
            accepted = _enqueue_stdout(out, terminal(2))
        self.assertFalse(accepted)
        self.assertIn("delivery unknown", stderr.getvalue())
        self.assertIn("request_id=2", stderr.getvalue())

    def test_config_rejects_noncontract_components_and_malformed_imu_values(self):
        with self.assertRaises(ValueError):
            ServiceConfig(motor_component=155)
        with self.assertRaises(ValueError):
            IMURegisterConfig(address=0x68, register=0x3B,
                              accel_scale_mg_per_count=float("nan"),
                              gyro_scale_mrad_s_per_count=1)
        with tempfile.TemporaryDirectory() as temp_dir:
            config = Path(temp_dir) / "bad.toml"
            config.write_text("[bus]\nnumber = 'one'\n", encoding="utf-8")
            with self.assertRaises(ValueError):
                ServiceConfig.from_toml(config)

    def test_motor_result_is_returned_through_comms_with_correlation(self):
        self.router.submit_frame(ground_request(opcode=3, delta=0), ingress="comms")
        # Poll past the write and an empty comms read, then provide the motor result.
        self.router.step()
        result = codec.encode("REQUEST_RESULT", {
            "ground_session": 123, "request_id": 1, "motor_boot_id": 55,
            "local_command_id": 77, "result": 1, "state": 4,
            "target_system": 255, "target_component": 190,
        }, system=SYSTEM_BOAT, component=COMPONENT_MOTOR)
        self.bus.inject_frame(0x08, result)
        self.router.step()
        self.router.step()
        self.router.step()
        returned = [op for op in self.bus.operations if op.kind == "write" and op.address == 0x09]
        self.assertTrue(returned)
        forwarded = codec.decode(self.router._decode_mailbox(returned[-1].data))
        self.assertEqual(forwarded["name"], "REQUEST_RESULT")
        self.assertEqual((forwarded["fields"]["request_id"],
                          forwarded["fields"]["ground_session"]), (1, 123))

    def test_mailbox_read_fault_marks_node_absent(self):
        self.bus.inject_fault("read")
        self.router.step()  # next round-robin node is comms
        self.assertFalse(self.router.node_present[(SYSTEM_BOAT, 158)])

    def test_empty_mailbox_ack_does_not_refresh_node_frame_age(self):
        self.router.node_last_seen.pop((SYSTEM_BOAT, 158), None)
        self.router.node_present[(SYSTEM_BOAT, 158)] = False
        self.router._poll_node(0x09)
        self.assertIn((SYSTEM_BOAT, 158), self.router.node_ack_at)
        self.assertNotIn((SYSTEM_BOAT, 158), self.router.node_last_seen)
        self.assertFalse(self.router.node_present[(SYSTEM_BOAT, 158)])

    def test_imu_failure_emits_fresh_invalid_health(self):
        cfg = ServiceConfig(imu=IMURegisterConfig(
            address=0x68, register=0x3B, accel_scale_mg_per_count=1,
            gyro_scale_mrad_s_per_count=1))
        bus = SimulatedBus(imu_samples={0x68: struct.pack("<6h", 0, 0, 1, 0, 0, 0)})
        bus.inject_fault("imu")
        output = []
        router = BoatRouter(bus, cfg, clock=self.clock, on_frame=output.append)
        for _ in range(3):
            router.step()
        reports = [codec.decode(frame) for frame in output
                   if codec.decode(frame)["name"] == "NODE_HEALTH"]
        self.assertTrue(reports)
        self.assertEqual(reports[-1]["fields"]["sensor_valid_fields"], 0)
        self.assertIsNotNone(router.imu_error)


if __name__ == "__main__":
    unittest.main()
