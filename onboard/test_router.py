"""Host-only routing and deterministic fault tests for the Pi service."""

from __future__ import annotations

import struct
import unittest

from boat.ids import COMPONENT_GROUND, COMPONENT_MOTOR, SYSTEM_BOAT, SYSTEM_GROUND
from protocol import codec

from onboard import BoatRouter, IMURegisterConfig, ServiceConfig, SimulatedBus


class Clock:
    def __init__(self):
        self.value = 10.0

    def __call__(self):
        return self.value

    def advance(self, seconds=0.02):
        self.value += seconds


def ground_request(request_id=1, opcode=2, delta=500, session=123):
    return codec.encode("ACTUATOR_REQUEST", {
        "request_id": request_id, "ground_session": session, "delta_cdeg": delta,
        "timeout_ms": 2000, "max_rate_cdeg_s": 500,
        "target_system": SYSTEM_BOAT, "target_component": COMPONENT_MOTOR,
        "actuator": 1, "opcode": opcode, "control_source": 3,
    }, system=SYSTEM_GROUND, component=COMPONENT_GROUND, sequence=request_id)


def health(boot=55, feedback=1):
    return codec.encode("NODE_HEALTH", {
        "boot_id": boot, "time_boot_ms": 100, "fault_flags": 0,
        "sensor_valid_fields": 0, "queue_drops": 0, "reset_reason": 0,
        "feedback_valid": feedback,
    }, system=SYSTEM_BOAT, component=COMPONENT_MOTOR)


class RouterTests(unittest.TestCase):
    def setUp(self):
        self.clock = Clock()
        self.bus = SimulatedBus()
        self.output = []
        self.events = []
        self.router = BoatRouter(self.bus, clock=self.clock, on_frame=self.output.append,
                                 on_event=self.events.append)
        self.bus.inject_frame(0x08, health(), snapshot=True)
        self.router.step()  # establish fresh motor boot and feedback state
        self.router.step()  # drain the forwarded snapshot into the comms mailbox
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
        self.router.step()
        self.router.step()
        self.router.step()
        self.assertFalse(self.router.submit_frame(ground_request()))
        self.assertFalse(any(op.kind == "write" and op.address == 0x08 for op in self.bus.operations))

    def test_stop_preempts_queued_step_and_is_distinct(self):
        self.router.submit_frame(ground_request(request_id=1, opcode=1, delta=100))
        self.router.submit_frame(ground_request(request_id=2, opcode=3, delta=0))
        self.router.step()
        write = next(op for op in self.bus.operations if op.kind == "write")
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

    def test_session_change_marks_previous_pending_outcome_unknown(self):
        self.router.submit_frame(ground_request(request_id=1, opcode=3, delta=0, session=123))
        self.router.submit_frame(ground_request(request_id=1, opcode=3, delta=0, session=456))
        self.assertTrue(any("session changed" in e.detail for e in self.events))

    def test_session_change_removes_unsent_old_movement(self):
        self.assertTrue(self.router.submit_frame(ground_request(request_id=1, session=123)))
        self.assertTrue(self.router.submit_frame(ground_request(request_id=1, session=456)))
        self.router.step()
        write = next(op for op in self.bus.operations if op.kind == "write" and op.address == 0x08)
        moved = codec.decode(self.router._decode_mailbox(write.data))["fields"]
        self.assertEqual((moved["request_id"], moved["ground_session"]), (1, 456))
        old_results = [codec.decode(frame)["fields"] for frame in self.output
                       if codec.decode(frame)["name"] == "REQUEST_RESULT"]
        self.assertTrue(any(result["ground_session"] == 123 and result["result"] == 4
                            for result in old_results))

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
