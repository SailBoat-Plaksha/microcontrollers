import threading
import unittest
from unittest.mock import patch

from boat.client import (BoatClient, Heartbeat, ImuSample, MotorReport,
                         NodeHealthReport, PongReply, RequestOutcome,
                         TransportEvent)
from protocol import codec


class FakeTransport:
    name = "memory"

    def __init__(self):
        self.frames = []
        self.callback = None
        self.opened = False
        self.open_error = None
        self.open_error_after_frames = False
        self.open_frames = []
        self.send_error = None
        self.on_send = None

    def open(self, on_frame):
        if self.open_error and not self.open_error_after_frames:
            raise self.open_error
        self.callback = on_frame
        self.opened = True
        for frame in self.open_frames:
            on_frame(frame)
        if self.open_error:
            raise self.open_error

    def close(self):
        self.opened = False

    def send_frame(self, frame):
        if self.send_error:
            raise self.send_error
        self.frames.append(frame)
        if self.on_send:
            self.on_send(frame)

    def receive(self, name, fields, *, system=1, component=154):
        self.callback(codec.encode(name, fields, system=system, component=component))

    def reconcile_motor(self, client, *, boot_id=7):
        self.receive("NODE_HEALTH", dict(boot_id=boot_id, time_boot_ms=1,
            fault_flags=0, sensor_valid_fields=0, queue_drops=0, reset_reason=0,
            feedback_valid=1), system=1, component=154)
        self.receive("ACTUATOR_STATUS", dict(time_boot_ms=1, command_id=0,
            requested_position_cdeg=0, measured_position_cdeg=0, velocity_cdeg_s=0,
            fault_flags=0, actuator=1, control_source=0, result=0, enabled=1),
            system=1, component=154)


class BoatClientTests(unittest.TestCase):
    def setUp(self):
        self.now = 10.0
        self.transport = FakeTransport()
        self.client = BoatClient(self.transport, clock=lambda: self.now)
        self.events = []
        self.client.subscribe(self.events.append)
        self.client.connect()

    def test_movement_frames_and_monotonic_request_ids(self):
        self.transport.reconcile_motor(self.client)
        first = self.client.set_absolute_angle(1, 9000, timeout_ms=500, max_rate_cdeg_s=1200)
        second = self.client.step(2, -500)
        stop = self.client.stop(1)
        self.assertEqual((first.request_id, second.request_id, stop.request_id), (1, 2, 3))
        self.assertEqual(first.ground_session, self.client.ground_session)
        decoded = [codec.decode(frame) for frame in self.transport.frames]
        self.assertEqual([m["name"] for m in decoded], ["ACTUATOR_REQUEST"] * 3)
        self.assertEqual([m["fields"]["opcode"] for m in decoded], [1, 2, 3])
        self.assertEqual([m["fields"]["delta_cdeg"] for m in decoded], [9000, -500, 0])
        self.assertEqual([m["fields"]["request_id"] for m in decoded], [1, 2, 3])
        self.assertEqual(decoded[0]["fields"]["timeout_ms"], 500)

    def test_session_changes_on_reconnect_and_requires_connected_state(self):
        with patch("boat.client.secrets.randbits", side_effect=[0x1234, 0x5678]):
            self.client.disconnect()
            self.client.connect()
            self.assertEqual(self.client.ground_session, 0x1234)
            self.transport.reconcile_motor(self.client)
            self.client.connect()
            self.assertEqual(self.client.ground_session, 0x5678)
            self.transport.reconcile_motor(self.client)
        self.assertEqual(self.client.set_absolute_angle(1, 0).request_id, 1)
        self.client.disconnect()
        with self.assertRaisesRegex(RuntimeError, "disconnected"):
            self.client.stop(1)

    def test_typed_dispatch_and_duplicate_outcome_suppression(self):
        self.transport.receive("HEARTBEAT", dict(type=0, autopilot=0, base_mode=0,
            custom_mode=0, system_status=4, mavlink_version=3), system=1, component=154)
        self.transport.receive("ACTUATOR_STATUS", dict(time_boot_ms=10, command_id=4,
            requested_position_cdeg=100, measured_position_cdeg=95, velocity_cdeg_s=5,
            fault_flags=0, actuator=1, control_source=2, result=1, enabled=1))
        self.transport.receive("NODE_HEALTH", dict(boot_id=99, time_boot_ms=10,
            fault_flags=0, sensor_valid_fields=0, queue_drops=0, reset_reason=0,
            feedback_valid=1), system=1, component=154)
        with self.assertRaisesRegex(RuntimeError, "motor is busy"):
            self.client.step(1, 100)
        self.transport.receive("ACTUATOR_STATUS", dict(time_boot_ms=11, command_id=4,
            requested_position_cdeg=100, measured_position_cdeg=95, velocity_cdeg_s=0,
            fault_flags=0, actuator=1, control_source=0, result=0, enabled=1))
        self.client.step(1, 100)
        result = dict(ground_session=self.client.ground_session, request_id=1,
            motor_boot_id=99, local_command_id=4, result=0, state=2,
            target_system=255, target_component=190)
        self.transport.receive("REQUEST_RESULT", {**result, "request_id": 2}, system=1, component=1)
        self.transport.receive("REQUEST_RESULT", {**result, "target_component": 1}, system=1, component=1)
        self.transport.receive("REQUEST_RESULT", result, system=1, component=1)
        self.transport.receive("REQUEST_RESULT", result, system=1, component=1)
        result = {**result, "state": 3}
        self.transport.receive("REQUEST_RESULT", result, system=1, component=1)
        self.assertEqual(len([e for e in self.events if isinstance(e, Heartbeat)]), 1)
        self.assertEqual(len([e for e in self.events if isinstance(e, MotorReport)]), 2)
        outcomes = [e for e in self.events if isinstance(e, RequestOutcome)]
        self.assertEqual([e.state for e in outcomes], [2, 3])
        self.assertTrue(any(isinstance(e, TransportEvent) and e.state == "connected" for e in self.events))

    def test_ping_uses_injected_clock_and_tracks_each_responder(self):
        self.client.ping_all()
        sent = codec.decode(self.transport.frames[-1])
        self.assertEqual(sent["name"], "PING")
        self.assertEqual((sent["fields"]["target_system"], sent["fields"]["target_component"]), (0, 0))
        self.now = 10.125
        self.transport.receive("PING", dict(time_usec=10_000_000, seq=0,
            target_system=255, target_component=190), system=1, component=154)
        pong = next(event for event in self.events if isinstance(event, PongReply))
        self.assertEqual((pong.system, pong.component, pong.seq), (1, 154, 0))
        self.assertAlmostEqual(pong.rtt_ms, 125.0)

    def test_ping_keeps_per_responder_state_and_checks_echo_and_duplicates(self):
        self.client.ping_all()
        ping = codec.decode(self.transport.frames[-1])["fields"]
        self.now = 10.1
        reply = dict(time_usec=ping["time_usec"], seq=ping["seq"],
                     target_system=255, target_component=190)
        self.transport.receive("PING", {**reply, "time_usec": ping["time_usec"] + 1},
                               system=1, component=154)
        self.transport.receive("PING", reply, system=1, component=154)
        self.transport.receive("PING", reply, system=1, component=154)
        self.transport.receive("PING", reply, system=1, component=158)
        pongs = [event for event in self.events if isinstance(event, PongReply)]
        self.assertEqual([(event.system, event.component) for event in pongs],
                         [(1, 154), (1, 158)])
        self.assertTrue(all(abs(event.rtt_ms - 100.0) < 1e-9 for event in pongs))

    def test_open_failure_does_not_commit_session_or_leave_transport_open(self):
        self.client.disconnect()
        self.transport.open_error = OSError("open failed")
        with self.assertRaisesRegex(OSError, "open failed"):
            self.client.connect()
        self.assertFalse(self.transport.opened)
        with self.assertRaisesRegex(RuntimeError, "disconnected"):
            _ = self.client.ground_session
        self.assertTrue(any(isinstance(event, TransportEvent) and event.state == "error"
                            for event in self.events))

    def test_synchronous_open_frames_are_processed_only_after_open_succeeds(self):
        self.client.disconnect()
        self.transport.open_frames = [
            codec.encode("NODE_HEALTH", dict(boot_id=7, time_boot_ms=1,
                fault_flags=0, sensor_valid_fields=0, queue_drops=0,
                reset_reason=0, feedback_valid=1), system=1, component=154),
            codec.encode("ACTUATOR_STATUS", dict(time_boot_ms=1, command_id=0,
                requested_position_cdeg=0, measured_position_cdeg=0,
                velocity_cdeg_s=0, fault_flags=0, actuator=1,
                control_source=0, result=0, enabled=1), system=1, component=154),
        ]
        self.client.connect()
        self.assertEqual(len([event for event in self.events
                              if isinstance(event, MotorReport)]), 1)
        self.assertEqual(len([event for event in self.events
                              if isinstance(event, NodeHealthReport)]), 1)
        self.assertEqual(self.client.step(1, 10).request_id, 1)

        self.client.disconnect()
        reports_before_failed_open = len([event for event in self.events
                                          if isinstance(event, NodeHealthReport)])
        self.transport.open_error = OSError("late open failure")
        self.transport.open_error_after_frames = True
        with self.assertRaisesRegex(OSError, "late open failure"):
            self.client.connect()
        self.assertEqual(len([event for event in self.events
                              if isinstance(event, NodeHealthReport)]),
                         reports_before_failed_open)
        with self.assertRaisesRegex(RuntimeError, "disconnected"):
            _ = self.client.ground_session

    def test_old_transport_callback_is_ignored_after_reconnect(self):
        old_callback = self.transport.callback
        self.client.disconnect()
        self.client.connect()
        old_callback(codec.encode("NODE_HEALTH", dict(boot_id=88, time_boot_ms=1,
            fault_flags=0, sensor_valid_fields=0, queue_drops=0,
            reset_reason=0, feedback_valid=1), system=1, component=154))
        old_callback(codec.encode("ACTUATOR_STATUS", dict(time_boot_ms=1,
            command_id=0, requested_position_cdeg=0, measured_position_cdeg=0,
            velocity_cdeg_s=0, fault_flags=0, actuator=1, control_source=0,
            result=0, enabled=1), system=1, component=154))
        self.assertNotIn((1, 154), self.client._boot_ids)
        with self.assertRaisesRegex(RuntimeError, "fresh motor health and status"):
            self.client.step(1, 10)

    def test_motor_readiness_expires_and_allows_stopped_output(self):
        self.transport.reconcile_motor(self.client)
        self.transport.receive("NODE_HEALTH", dict(boot_id=7, time_boot_ms=2,
            fault_flags=0, sensor_valid_fields=0, queue_drops=0,
            reset_reason=0, feedback_valid=0), system=1, component=154)
        with self.assertRaisesRegex(RuntimeError, "valid motor feedback"):
            self.client.step(1, 10)
        self.transport.receive("NODE_HEALTH", dict(boot_id=7, time_boot_ms=3,
            fault_flags=0, sensor_valid_fields=0, queue_drops=0,
            reset_reason=0, feedback_valid=1), system=1, component=154)
        self.transport.receive("ACTUATOR_STATUS", dict(time_boot_ms=3,
            command_id=0, requested_position_cdeg=0, measured_position_cdeg=0,
            velocity_cdeg_s=0, fault_flags=0, actuator=1, control_source=0,
            result=0, enabled=0), system=1, component=154)
        self.assertEqual(self.client.step(1, 10).request_id, 1)
        self.transport.receive("ACTUATOR_STATUS", dict(time_boot_ms=4,
            command_id=0, requested_position_cdeg=0, measured_position_cdeg=0,
            velocity_cdeg_s=0, fault_flags=0, actuator=1, control_source=0,
            result=0, enabled=1), system=1, component=154)
        self.assertEqual(self.client.step(1, 10).request_id, 2)
        self.now += self.client._motor_freshness_s + 0.01
        with self.assertRaisesRegex(RuntimeError, "fresh motor health and status"):
            self.client.step(1, 10)
        self.assertGreaterEqual(self.client.stop(1).request_id, 2)

    def test_expired_unknown_motion_requires_new_status(self):
        self.transport.reconcile_motor(self.client)
        self.client.step(1, 10)
        self.now += self.client._TRACK_TTL + 1
        self.client.ping_all()  # prunes the expired ticket and invalidates status readiness
        self.assertTrue(any(isinstance(event, RequestOutcome) and event.state == 0
                            for event in self.events))
        self.transport.receive("NODE_HEALTH", dict(boot_id=7, time_boot_ms=2,
            fault_flags=0, sensor_valid_fields=0, queue_drops=0,
            reset_reason=0, feedback_valid=1), system=1, component=154)
        with self.assertRaisesRegex(RuntimeError, "fresh motor health and status"):
            self.client.step(1, 10)
        self.transport.receive("ACTUATOR_STATUS", dict(time_boot_ms=2,
            command_id=0, requested_position_cdeg=0, measured_position_cdeg=0,
            velocity_cdeg_s=0, fault_flags=0, actuator=1, control_source=0,
            result=0, enabled=1), system=1, component=154)
        self.assertEqual(self.client.step(1, 10).request_id, 2)

    def test_result_subscriber_can_disconnect_during_dispatch(self):
        self.transport.reconcile_motor(self.client)
        ticket = self.client.step(1, 10)
        self.client.subscribe(lambda event: self.client.disconnect()
                              if isinstance(event, RequestOutcome) else None)
        self.transport.receive("REQUEST_RESULT", dict(
            ground_session=ticket.ground_session, request_id=ticket.request_id,
            motor_boot_id=7, local_command_id=1, result=1, state=4,
            target_system=255, target_component=190), system=1, component=1)
        self.assertIsNone(self.client._session)
        self.assertEqual(self.client._pending_tickets, {})

    def test_synchronous_result_during_send_uses_reserved_correlation(self):
        self.transport.reconcile_motor(self.client)

        def reply_during_send(frame):
            message = codec.decode(frame)
            if message["name"] != "ACTUATOR_REQUEST":
                return
            fields = message["fields"]
            self.transport.receive("REQUEST_RESULT", dict(
                ground_session=fields["ground_session"],
                request_id=fields["request_id"], motor_boot_id=7,
                local_command_id=3, result=1, state=4,
                target_system=255, target_component=190), system=1, component=1)

        self.transport.on_send = reply_during_send
        ticket = self.client.step(1, 10)
        self.assertEqual(ticket.request_id, 1)
        self.assertNotIn((ticket.ground_session, ticket.request_id),
                         self.client._pending_tickets)
        outcomes = [event for event in self.events if isinstance(event, RequestOutcome)]
        self.assertEqual([(event.request_id, event.state) for event in outcomes], [(1, 4)])

    def test_incidental_health_sources_do_not_evict_safety_boot_ids(self):
        self.transport.receive("NODE_HEALTH", dict(boot_id=11, time_boot_ms=1,
            fault_flags=0, sensor_valid_fields=0, queue_drops=0,
            reset_reason=0, feedback_valid=0), system=1, component=1)
        self.transport.receive("NODE_HEALTH", dict(boot_id=22, time_boot_ms=1,
            fault_flags=0, sensor_valid_fields=0, queue_drops=0,
            reset_reason=0, feedback_valid=1), system=1, component=154)
        for index in range(self.client._MAX_SEEN + 20):
            system, component = divmod(index, 256)
            system += 2
            self.transport.receive("NODE_HEALTH", dict(boot_id=index + 100,
                time_boot_ms=1, fault_flags=0, sensor_valid_fields=0,
                queue_drops=0, reset_reason=0, feedback_valid=0),
                system=system, component=component)
        self.assertEqual(self.client._boot_ids[(1, 1)], 11)
        self.assertEqual(self.client._boot_ids[(1, 154)], 22)
        self.assertLessEqual(len(self.client._boot_ids), self.client._MAX_SEEN)
        self.assertLessEqual(len(self.client._health_at), self.client._MAX_SEEN)

    def test_malformed_protocol_frame_emits_error_event(self):
        self.transport.callback(b"not a MAVLink frame")
        self.assertTrue(any(isinstance(event, TransportEvent) and
                            event.state == "protocol_error" for event in self.events))

    def test_reader_thread_can_close_transport_during_callback(self):
        self.transport.reconcile_motor(self.client)
        self.client.step(1, 10)
        closed_on = []
        original_close = self.transport.close

        def close():
            closed_on.append(threading.current_thread().name)
            original_close()

        self.transport.close = close
        subscriber = self.client.subscribe(lambda event: self.client.disconnect()
            if isinstance(event, Heartbeat) else None)
        failures = []

        def deliver():
            try:
                self.transport.receive("HEARTBEAT", dict(type=0, autopilot=0,
                    base_mode=0, custom_mode=0, system_status=4, mavlink_version=3))
            except Exception as exc:  # expose reader-thread failures to the test
                failures.append(exc)

        reader = threading.Thread(target=deliver, name="fake-reader")
        reader.start()
        reader.join(timeout=2)
        subscriber()
        self.assertFalse(reader.is_alive())
        self.assertEqual(failures, [])
        self.assertEqual(closed_on, ["fake-reader"])
        self.assertIsNone(self.client._session)
        self.assertEqual(self.client._pending_tickets, {})
        self.assertTrue(any(isinstance(event, RequestOutcome) and event.state == 0
                            for event in self.events))

    def test_send_failure_emits_unknown_and_marks_transport_disconnected(self):
        self.transport.reconcile_motor(self.client)
        self.transport.send_error = OSError("write lost")
        with self.assertRaisesRegex(OSError, "write lost"):
            self.client.step(1, 100)
        outcomes = [event for event in self.events if isinstance(event, RequestOutcome)]
        self.assertEqual(len(outcomes), 1)
        self.assertEqual((outcomes[0].request_id, outcomes[0].result, outcomes[0].state),
                         (1, 0, 0))
        self.assertEqual(outcomes[0].motor_boot_id, 7)
        self.assertTrue(any(isinstance(event, TransportEvent) and event.state == "error"
                            for event in self.events))
        self.assertFalse(self.transport.opened)
        with self.assertRaisesRegex(RuntimeError, "disconnected"):
            self.client.stop(1)

    def test_disconnect_and_motor_reboot_invalidate_pending_and_sensor_state(self):
        self.transport.reconcile_motor(self.client)
        self.client.step(1, 100)
        health = dict(boot_id=7, time_boot_ms=20, fault_flags=0,
                      sensor_valid_fields=3, queue_drops=0, reset_reason=0,
                      feedback_valid=1)
        self.transport.receive("NODE_HEALTH", health, system=1, component=154)
        self.transport.receive("NODE_HEALTH", {**health, "boot_id": 8},
                               system=1, component=154)
        outcomes = [event for event in self.events if isinstance(event, RequestOutcome)]
        self.assertEqual([(event.result, event.state, event.motor_boot_id) for event in outcomes],
                         [(0, 0, 7)])
        self.assertEqual(self.client._sensor_valid_fields[(1, 154)], 3)
        with self.assertRaisesRegex(RuntimeError, "fresh motor health and status"):
            self.client.step(1, 150)
        self.transport.receive("ACTUATOR_STATUS", dict(time_boot_ms=30, command_id=0,
            requested_position_cdeg=0, measured_position_cdeg=0, velocity_cdeg_s=0,
            fault_flags=0, actuator=1, control_source=0, result=0, enabled=1),
            system=1, component=154)
        self.client.step(1, 200)
        self.client.disconnect()
        outcomes = [event for event in self.events if isinstance(event, RequestOutcome)]
        self.assertEqual([(event.request_id, event.state) for event in outcomes], [(1, 0), (2, 0)])
        self.client.connect()
        with self.assertRaisesRegex(RuntimeError, "fresh motor health and status"):
            self.client.step(1, 300)
        self.assertEqual(self.client._sensor_valid_fields, {})

    def test_request_result_requires_router_identity_and_matching_motor_boot(self):
        self.transport.reconcile_motor(self.client, boot_id=7)
        self.client.step(1, 100)
        health = dict(boot_id=7, time_boot_ms=20, fault_flags=0,
                      sensor_valid_fields=0, queue_drops=0, reset_reason=0,
                      feedback_valid=1)
        self.transport.receive("NODE_HEALTH", health, system=1, component=154)
        result = dict(ground_session=self.client.ground_session, request_id=1,
                      motor_boot_id=7, local_command_id=12, result=1, state=4,
                      target_system=255, target_component=190)
        self.transport.receive("REQUEST_RESULT", result, system=9, component=1)
        self.transport.receive("REQUEST_RESULT", {**result, "target_component": 1},
                               system=1, component=1)
        self.transport.receive("REQUEST_RESULT", {**result, "motor_boot_id": 8},
                               system=1, component=1)
        outcomes = [event for event in self.events if isinstance(event, RequestOutcome)]
        self.assertEqual([(event.result, event.state, event.local_command_id)
                          for event in outcomes], [(0, 0, 0)])

    def test_bad_inputs_do_not_allocate_or_send_and_zero_absolute_is_not_stop(self):
        self.transport.reconcile_motor(self.client)
        before = len(self.transport.frames)
        for args in ((0, 10), (1, 40000), (1, -40000)):
            with self.assertRaises(ValueError):
                self.client.set_absolute_angle(*args)
        with self.assertRaises(ValueError):
            self.client.step(1, 10, timeout_ms=65536)
        self.assertEqual(len(self.transport.frames), before)
        absolute_zero = self.client.set_absolute_angle(1, 0)
        stopped = self.client.stop(1)
        frames = [codec.decode(frame) for frame in self.transport.frames[before:]]
        self.assertEqual((absolute_zero.request_id, stopped.request_id), (1, 2))
        self.assertEqual([message["fields"]["opcode"] for message in frames], [1, 3])
        self.assertEqual([message["fields"]["delta_cdeg"] for message in frames], [0, 0])

    def test_stop_remains_available_while_reconciliation_is_required(self):
        with self.assertRaisesRegex(RuntimeError, "fresh motor health and status"):
            self.client.step(1, 50)
        ticket = self.client.stop(1)
        sent = codec.decode(self.transport.frames[-1])
        self.assertEqual(ticket.request_id, 1)
        self.assertEqual((sent["fields"]["opcode"], sent["fields"]["delta_cdeg"]), (3, 0))

    def test_subscriber_exception_does_not_break_delivery_or_receive(self):
        self.transport.reconcile_motor(self.client)
        received = []
        self.client.subscribe(lambda _event: (_ for _ in ()).throw(RuntimeError("observer")))
        self.client.subscribe(received.append)
        self.transport.receive("HEARTBEAT", dict(type=0, autopilot=0, base_mode=0,
            custom_mode=0, system_status=4, mavlink_version=3), system=1, component=154)
        self.assertEqual(len([event for event in received if isinstance(event, Heartbeat)]), 1)
        ticket = self.client.step(1, 1)
        self.assertEqual(ticket.request_id, 1)

    def test_pending_and_ping_trackers_expire_with_injected_clock(self):
        self.client.ping_all()
        self.transport.reconcile_motor(self.client)
        self.client.step(1, 1)
        self.now += self.client._TRACK_TTL + 1
        self.client.ping_all()
        self.assertEqual(len(self.client._ping_sent), 1)
        self.assertEqual(self.client._pending_tickets, {})
        self.assertTrue(any(isinstance(event, RequestOutcome) and event.state == 0
                            for event in self.events))

    def test_imu_dispatch_contains_accel_and_gyro_units(self):
        self.transport.receive("NODE_HEALTH", dict(boot_id=9, time_boot_ms=20,
            fault_flags=0, sensor_valid_fields=3, queue_drops=0, reset_reason=0,
            feedback_valid=0), system=1, component=1)
        self.transport.receive("SCALED_IMU", dict(time_boot_ms=21, xacc=1, yacc=2, zacc=3,
            xgyro=4, ygyro=5, zgyro=6, xmag=0, ymag=0, zmag=0, temperature=0),
            system=1, component=1)
        sample = next(event for event in self.events if isinstance(event, ImuSample))
        self.assertEqual(sample.accel_mg, (1, 2, 3))
        self.assertEqual(sample.gyro_mrad_s, (4, 5, 6))
        self.assertEqual(sample.valid_fields, 3)
        self.assertEqual(sample.source, (1, 1))


if __name__ == "__main__":
    unittest.main(verbosity=2)
