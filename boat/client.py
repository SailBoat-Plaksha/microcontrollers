"""Transport-independent ground client for the frozen boat contract."""

from __future__ import annotations

import secrets
import struct
import time
from dataclasses import dataclass
from typing import Callable, Protocol

from protocol import codec
from .ids import (COMPONENT_GROUND, COMPONENT_MOTOR, SYSTEM_GROUND,
                  SYSTEM_BOAT)


class Transport(Protocol):
    name: str

    def open(self, on_frame: Callable[[bytes], None]) -> None: ...
    def close(self) -> None: ...
    def send_frame(self, frame: bytes) -> None: ...


@dataclass(frozen=True)
class RequestTicket:
    request_id: int
    ground_session: int


@dataclass(frozen=True)
class Heartbeat:
    system: int
    component: int
    age: float


@dataclass(frozen=True)
class PongReply:
    system: int
    component: int
    seq: int
    time_usec: int
    rtt_ms: float | None


@dataclass(frozen=True)
class MotorReport:
    time_boot_ms: int
    command_id: int
    requested_position_cdeg: int
    measured_position_cdeg: int
    velocity_cdeg_s: int
    fault_flags: int
    actuator: int
    control_source: int
    result: int
    enabled: int
    decoded_at: float
    source: tuple[int, int]


@dataclass(frozen=True)
class RequestOutcome:
    ground_session: int
    request_id: int
    motor_boot_id: int
    local_command_id: int
    result: int
    state: int
    target_system: int
    target_component: int
    source: tuple[int, int]


@dataclass(frozen=True)
class NodeHealthReport:
    boot_id: int
    time_boot_ms: int
    fault_flags: int
    sensor_valid_fields: int
    queue_drops: int
    reset_reason: int
    feedback_valid: int
    source: tuple[int, int]


@dataclass(frozen=True)
class ImuSample:
    time_boot_ms: int
    accel_mg: tuple[int, int, int]
    gyro_mrad_s: tuple[int, int, int]
    valid_fields: int
    source: tuple[int, int]


@dataclass(frozen=True)
class RawFrameEvent:
    bytes: bytes
    transport: str
    direction: str


@dataclass(frozen=True)
class TransportEvent:
    state: str
    detail: str


class BoatClient:
    def __init__(self, transport: Transport, *, system: int = SYSTEM_GROUND,
                 component: int = COMPONENT_GROUND,
                 clock: Callable[[], float] = time.monotonic) -> None:
        self.transport = transport
        self.system = system
        self.component = component
        self._clock = clock
        self._session: int | None = None
        self._next_request_id = 1
        self._sequence = 0
        self._subscribers: list[Callable[[object], None]] = []
        self._last_seen: dict[tuple[int, int], float] = {}
        self._sensor_valid_fields: dict[tuple[int, int], int] = {}
        self._ping_sent: dict[int, float] = {}
        self._pending_tickets: dict[tuple[int, int], RequestTicket] = {}
        self._seen_outcomes: set[tuple[int, int, int, int, int, int, int]] = set()

    @property
    def ground_session(self) -> int:
        if self._session is None:
            raise RuntimeError("client is disconnected")
        return self._session

    def connect(self) -> None:
        if self._session is not None:
            self.disconnect()
        self._session = secrets.randbits(32) or 1
        self._next_request_id = 1
        self._seen_outcomes.clear()
        self._pending_tickets.clear()
        self.transport.open(self._receive)
        self._publish(TransportEvent("connected", self.transport.name))

    def disconnect(self) -> None:
        if self._session is None:
            return
        self.transport.close()
        self._session = None
        self._ping_sent.clear()
        self._pending_tickets.clear()
        self._publish(TransportEvent("disconnected", self.transport.name))

    def set_absolute_angle(self, actuator: int, position_cdeg: int, *,
                           timeout_ms: int = 0, max_rate_cdeg_s: int = 0) -> RequestTicket:
        return self._movement(actuator, 1, position_cdeg, timeout_ms, max_rate_cdeg_s)

    def step(self, actuator: int, delta_cdeg: int, *,
             timeout_ms: int = 0, max_rate_cdeg_s: int = 0) -> RequestTicket:
        return self._movement(actuator, 2, delta_cdeg, timeout_ms, max_rate_cdeg_s)

    def stop(self, actuator: int) -> RequestTicket:
        return self._movement(actuator, 3, 0, 0, 0)

    def ping_all(self) -> None:
        self._require_connected()
        seq = self._sequence & 0xFFFFFFFF
        self._sequence += 1
        timestamp = int(self._clock() * 1_000_000)
        self._ping_sent[seq] = self._clock()
        self._send("PING", {"time_usec": timestamp, "seq": seq,
                             "target_system": 0, "target_component": 0})

    def subscribe(self, callback: Callable[[object], None]) -> Callable[[], None]:
        self._subscribers.append(callback)
        def unsubscribe() -> None:
            if callback in self._subscribers:
                self._subscribers.remove(callback)
        return unsubscribe

    def _movement(self, actuator: int, opcode: int, delta: int,
                  timeout_ms: int, max_rate_cdeg_s: int) -> RequestTicket:
        self._require_connected()
        request_id = self._next_request_id
        if request_id > 0xFFFFFFFF:
            raise OverflowError("request_id space exhausted for this session")
        self._next_request_id += 1
        ticket = RequestTicket(request_id, self.ground_session)
        self._pending_tickets[(ticket.ground_session, ticket.request_id)] = ticket
        self._send("ACTUATOR_REQUEST", {
            "request_id": request_id, "ground_session": ticket.ground_session,
            "delta_cdeg": delta, "timeout_ms": timeout_ms,
            "max_rate_cdeg_s": max_rate_cdeg_s, "target_system": SYSTEM_BOAT,
            "target_component": COMPONENT_MOTOR, "actuator": actuator,
            "opcode": opcode, "control_source": 3,
        })
        return ticket

    def _require_connected(self) -> None:
        if self._session is None:
            raise RuntimeError("client is disconnected")

    def _send(self, name: str, fields: dict[str, int]) -> None:
        frame = codec.encode(name, fields, system=self.system, component=self.component,
                             sequence=self._sequence & 0xFF)
        self._sequence = (self._sequence + 1) & 0xFFFFFFFF
        self.transport.send_frame(frame)
        self._publish(RawFrameEvent(frame, self.transport.name, "out"))

    def _receive(self, frame: bytes) -> None:
        self._publish(RawFrameEvent(bytes(frame), self.transport.name, "in"))
        try:
            message = codec.decode(frame)
        except (ValueError, KeyError, struct.error):
            return
        now = self._clock()
        source = (message["system"], message["component"])
        self._last_seen[source] = now
        name, fields = message["name"], message["fields"]
        if name == "HEARTBEAT":
            self._publish(Heartbeat(*source, 0.0))
        elif name == "PING":
            if fields["target_system"] == self.system and fields["target_component"] == self.component:
                sent = self._ping_sent.pop(fields["seq"], None)
                self._publish(PongReply(*source, fields["seq"], fields["time_usec"],
                                        None if sent is None else (now - sent) * 1000.0))
        elif name == "ACTUATOR_STATUS":
            self._publish(MotorReport(*(fields[key] for key in (
                "time_boot_ms", "command_id", "requested_position_cdeg",
                "measured_position_cdeg", "velocity_cdeg_s", "fault_flags",
                "actuator", "control_source", "result", "enabled")), now, source))
        elif name == "REQUEST_RESULT":
            ticket_key = (fields["ground_session"], fields["request_id"])
            if (ticket_key not in self._pending_tickets or
                    fields["target_system"] != self.system or
                    fields["target_component"] != self.component):
                return
            key = (fields["ground_session"], fields["request_id"], fields["motor_boot_id"],
                   fields["local_command_id"], fields["result"], fields["state"],
                   fields["target_system"] << 8 | fields["target_component"])
            if key not in self._seen_outcomes:
                self._seen_outcomes.add(key)
                self._publish(RequestOutcome(*(fields[key] for key in (
                    "ground_session", "request_id", "motor_boot_id", "local_command_id",
                    "result", "state", "target_system", "target_component")), source))
            if fields["state"] in (4, 5, 6):
                self._pending_tickets.pop(ticket_key, None)
        elif name == "NODE_HEALTH":
            self._sensor_valid_fields[source] = fields["sensor_valid_fields"]
            self._publish(NodeHealthReport(*(fields[key] for key in (
                "boot_id", "time_boot_ms", "fault_flags", "sensor_valid_fields",
                "queue_drops", "reset_reason", "feedback_valid")), source))
        elif name == "SCALED_IMU":
            self._publish(ImuSample(fields["time_boot_ms"],
                                    (fields["xacc"], fields["yacc"], fields["zacc"]),
                                    (fields["xgyro"], fields["ygyro"], fields["zgyro"]),
                                    self._sensor_valid_fields.get(source, 0), source))

    def _publish(self, event: object) -> None:
        for callback in tuple(self._subscribers):
            callback(event)
