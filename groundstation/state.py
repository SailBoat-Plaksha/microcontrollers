"""Thread-safe process-wide client and event state shared by all browser tabs."""
from __future__ import annotations

import dataclasses
import json
import queue
import threading
import time
from collections import deque
from dataclasses import dataclass

from boat.client import (BoatClient, Heartbeat, ImuReading, ImuSample, MotorReport, NodeHealthReport,
                         PongReply, RawFrameEvent, RequestOutcome, TransportEvent)
from protocol import codec


class GroundStationState:
    def __init__(self):
        self.lock = threading.RLock()
        self.events: queue.Queue[tuple[float, object, object | None]] = queue.Queue(maxsize=1000)
        self.terminal_events: deque[tuple[float, RequestOutcome, object | None]] = deque(maxlen=256)
        self.client: BoatClient | None = None
        self.unsubscribe = None
        self.connected = False
        self.mode = "DISCONNECTED"
        self.detail = "Choose a transport and connect."
        self.uncertain = False
        self.busy = False
        self.feedback_valid = False
        self.motor_report: MotorReport | None = None
        self.motor_reports: dict[int, MotorReport] = {}
        self.motor_report_at: dict[int, float] = {}
        self.motor_health: NodeHealthReport | None = None
        self.nodes: dict[tuple[int, int], dict] = {}
        self.pings: dict[tuple[int, int], PongReply] = {}
        self.imu: deque[tuple[float, ImuSample]] = deque(maxlen=100)
        self.imu_reading: tuple[float, ImuReading] | None = None
        self.trace: deque[dict] = deque(maxlen=1000)
        self.transport = None
        self.trace_drops = 0
        self.outcomes = deque(maxlen=256)

    def enqueue(self, event, owner=None):
        terminal = isinstance(event, RequestOutcome) and event.state in (0, 4, 5, 6)
        if terminal:
            with self.lock:
                self.terminal_events.append((time.monotonic(), event, owner))
            return
        try:
            self.events.put_nowait((time.monotonic(), event, owner))
        except queue.Full:
            with self.lock:
                self.trace_drops += 1

    def drain(self):
        changed = False
        while True:
            with self.lock:
                try:
                    at, event, owner = self.terminal_events.popleft()
                except IndexError:
                    break
            if owner is not None and owner is not self.client:
                continue
            changed = True
            with self.lock:
                self._apply(at, event)
        while True:
            try:
                at, event, owner = self.events.get_nowait()
            except queue.Empty:
                break
            if owner is not None and owner is not self.client:
                continue
            changed = True
            with self.lock:
                self._apply(at, event)
        return changed

    def _apply(self, at, event):
        name = type(event).__name__
        record = {"time_monotonic": at, "event": name}
        if dataclasses.is_dataclass(event):
            values = dataclasses.asdict(event)
            for key, value in tuple(values.items()):
                if isinstance(value, bytes):
                    values[key] = value.hex()
            record.update(values)
        elif isinstance(event, Exception):
            record.update(error=f"{type(event).__name__}: {event}")
        self.trace.append(record)
        if isinstance(event, TransportEvent):
            if event.state == "connected":
                self.connected = True
            elif event.state == "disconnected":
                self.connected = False
            self.mode = self.transport.name if self.connected and self.transport else event.state.upper()
            self.detail = event.detail
            if event.state == "disconnected":
                self.busy = False
                self.feedback_valid = False
                self.motor_report = None
                self.motor_reports.clear()
                self.motor_report_at.clear()
                self.motor_health = None
                self.imu_reading = None
                self.uncertain = True
        elif isinstance(event, TransportLog):
            self.detail = f"{event.transport} log: {event.message}"
        elif isinstance(event, Heartbeat):
            node = self.nodes.setdefault((event.system, event.component), {})
            node["seen"] = at
        elif isinstance(event, PongReply):
            self.pings[(event.system, event.component)] = event
            node = self.nodes.setdefault((event.system, event.component), {})
            node["seen"] = at
            node["rtt"] = event.rtt_ms
        elif isinstance(event, NodeHealthReport):
            node = self.nodes.setdefault(event.source, {})
            previous = node.get("health")
            if (event.source == (1, 154) and previous is not None and
                    previous.boot_id != event.boot_id):
                self.motor_report = None
                self.motor_reports.clear()
                self.motor_report_at.clear()
                self.busy = False
                self.uncertain = True
            node.update(seen=at, health=event)
            if event.source == (1, 154):
                self.motor_health = event
                self.feedback_valid = bool(event.feedback_valid)
                node["health_seen"] = at
                if self.motor_report is None:
                    self.uncertain = True
        elif isinstance(event, MotorReport):
            self.motor_reports[event.actuator] = event
            self.motor_report_at[event.actuator] = at
            if event.actuator == 1:
                self.motor_report = event
            node = self.nodes.setdefault(event.source, {})
            node.update(seen=at, feedback_seen=at, motor=event)
            self.uncertain = False
            self.busy = any(item.result == 1 or bool(item.enabled)
                            for item in self.motor_reports.values())
        elif isinstance(event, RequestOutcome):
            self.outcomes.append(event)
            self.uncertain = event.result == 0 or event.state in (0, 1)
            if event.state in (1, 2, 3):
                self.busy = event.state in (1, 2, 3)
            elif event.state in (4, 5, 6):
                self.busy = any(item.result == 1 or bool(item.enabled)
                                for item in self.motor_reports.values())
        elif isinstance(event, ImuSample):
            self.imu.append((at, event))
            node = self.nodes.setdefault(event.source, {})
            node["seen"] = at
        elif isinstance(event, ImuReading):
            self.imu_reading = (at, event)
            node = self.nodes.setdefault(event.source, {})
            node["seen"] = at
        elif isinstance(event, RawFrameEvent):
            try:
                message = codec.decode(event.bytes)
                fields = message["fields"]
                record.update(message=message["name"], source=[message["system"], message["component"]],
                              request_id=fields.get("request_id"),
                              ground_session=fields.get("ground_session"),
                              result=fields.get("result"), state=fields.get("state"),
                              target=([fields["target_system"], fields["target_component"]]
                                      if "target_system" in fields else None))
                if message["name"] == "LINK_STATUS" and event.direction == "in":
                    node = self.nodes.setdefault((message["system"], message["component"]), {})
                    node["link"] = fields
            except ValueError:
                record["decode_error"] = "invalid frame"
            record["frame_hex"] = event.bytes.hex()

    def can_move(self):
        return self.can_move_actuator(1)

    def can_move_actuator(self, actuator):
        if actuator not in (1, 2):
            return False
        with self.lock:
            transport_up = bool(getattr(self.transport, "is_connected", self.connected))
            if (not self.connected or not transport_up or self.busy or self.uncertain or
                    not self.feedback_valid):
                return False
            report = self.motor_reports.get(actuator)
            node = self.nodes.get((1, 154), {})
            now = time.monotonic()
            return (report is not None and report.result == 0 and not report.enabled and
                    now - self.motor_report_at.get(actuator, 0) <= 5.0 and
                    now - node.get("health_seen", 0) <= 5.0)

    def jsonl(self):
        with self.lock:
            return "".join(json.dumps(item, separators=(",", ":"), default=str) + "\n"
                           for item in self.trace)


STATE = GroundStationState()


@dataclass(frozen=True)
class TransportLog:
    transport: str
    message: str
