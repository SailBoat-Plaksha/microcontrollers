import unittest
from unittest.mock import patch

from boat.client import (BoatClient, Heartbeat, ImuSample, MotorReport, PongReply,
                         RequestOutcome, TransportEvent)
from protocol import codec


class FakeTransport:
    name = "memory"

    def __init__(self):
        self.frames = []
        self.callback = None
        self.opened = False

    def open(self, on_frame):
        self.callback = on_frame
        self.opened = True

    def close(self):
        self.opened = False

    def send_frame(self, frame):
        self.frames.append(frame)

    def receive(self, name, fields, *, system=1, component=154):
        self.callback(codec.encode(name, fields, system=system, component=component))


class BoatClientTests(unittest.TestCase):
    def setUp(self):
        self.now = 10.0
        self.transport = FakeTransport()
        self.client = BoatClient(self.transport, clock=lambda: self.now)
        self.events = []
        self.client.subscribe(self.events.append)
        self.client.connect()

    def test_movement_frames_and_monotonic_request_ids(self):
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
            self.client.connect()
            self.assertEqual(self.client.ground_session, 0x5678)
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
        self.assertEqual(len([e for e in self.events if isinstance(e, MotorReport)]), 1)
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
