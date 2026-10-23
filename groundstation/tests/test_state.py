import time
import unittest

from boat.client import (Heartbeat, MotorReport, NodeHealthReport, RawFrameEvent,
                         RequestOutcome, TransportEvent)
from groundstation.state import GroundStationState
from groundstation.transports import SimulationTransport
from protocol import codec


class StateTests(unittest.TestCase):
    def setUp(self):
        self.state = GroundStationState()
        self.state.transport = SimulationTransport()
        self.state.transport.connected = True
        self.state.connected = True

    def health(self, boot_id=1):
        return NodeHealthReport(boot_id, 1, 0, 0, 0, 0, 1, (1, 154))

    def motor(self, enabled=0):
        return MotorReport(1, 1, 0, 0, 0, 0, 1, 3, 0, enabled, 1.0, (1, 154))

    def test_move_requires_recent_status_and_health_not_only_heartbeat(self):
        old = time.monotonic() - 10
        self.state._apply(old, self.health())
        self.state._apply(old, self.motor())
        self.state._apply(time.monotonic(), Heartbeat(1, 154, 0))
        self.assertFalse(self.state.can_move())
        now = time.monotonic()
        self.state._apply(now, self.health())
        self.state._apply(now, self.motor())
        self.assertTrue(self.state.can_move())

    def test_new_motor_boot_invalidates_previous_measured_status(self):
        now = time.monotonic()
        self.state._apply(now, self.health(1))
        self.state._apply(now, self.motor())
        self.assertTrue(self.state.can_move())
        self.state._apply(now + 0.01, self.health(2))
        self.assertIsNone(self.state.motor_report)
        self.assertTrue(self.state.uncertain)
        self.assertFalse(self.state.can_move())

    def test_packet_trace_includes_route_and_correlation(self):
        frame = codec.encode("REQUEST_RESULT", {"ground_session": 10, "request_id": 11,
            "motor_boot_id": 12, "local_command_id": 13, "result": 3, "state": 6,
            "target_system": 255, "target_component": 190}, system=1, component=1)
        self.state._apply(time.monotonic(), RawFrameEvent(frame, "SIMULATION", "in"))
        text = self.state.jsonl()
        self.assertIn('"message":"REQUEST_RESULT"', text)
        self.assertIn('"request_id":11', text)
        self.assertIn('"target":[255,190]', text)
        self.assertIn(frame.hex(), text)

    def test_reconciliation_events_do_not_drop_live_connection(self):
        for event in (TransportEvent("connected", "sim"),
                      TransportEvent("reconciliation_required", "awaiting status"),
                      TransportEvent("reconciled", "fresh status received")):
            self.state._apply(time.monotonic(), event)
        self.assertTrue(self.state.connected)
        self.assertTrue(self.state.can_move() is False)
        self.state._apply(time.monotonic(), self.health())
        self.state._apply(time.monotonic(), self.motor())
        self.assertTrue(self.state.can_move_actuator(1))

    def test_selected_actuator_requires_its_own_status(self):
        now = time.monotonic()
        self.state._apply(now, self.health())
        self.state._apply(now, self.motor())
        self.assertTrue(self.state.can_move_actuator(1))
        self.assertFalse(self.state.can_move_actuator(2))
        rudder = MotorReport(1, 2, 0, 500, 0, 0, 2, 3, 0, 0, 1.0, (1, 154))
        self.state._apply(time.monotonic(), rudder)
        self.assertTrue(self.state.can_move_actuator(2))

    def test_terminal_outcome_is_preserved_when_event_queue_is_full(self):
        for i in range(self.state.events.maxsize):
            self.state.enqueue(Heartbeat(1, i % 255, 0))
        result = RequestOutcome(1, 2, 3, 4, 1, 4, 255, 190, (1, 154))
        self.state.enqueue(result)
        self.state.drain()
        self.assertIn(result, self.state.outcomes)


if __name__ == "__main__":
    unittest.main()
