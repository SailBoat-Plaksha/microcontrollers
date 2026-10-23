"""Transport-independent ground client for the frozen boat contract."""

from __future__ import annotations

import secrets
import struct
import time
from dataclasses import dataclass
from typing import Callable, Protocol

from protocol import codec
from .ids import (COMPONENT_GROUND, COMPONENT_MOTOR, COMPONENT_PI, SYSTEM_BOAT,
                  SYSTEM_GROUND)


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
    """Ground protocol client with bounded correlation and session state."""

    _MAX_PENDING = 256
    _MAX_PINGS = 128
    _MAX_SEEN = 512
    _TRACK_TTL = 300.0
    _VALID_ACTUATORS = frozenset((1, 2))
    _OUTCOME_TERMINAL_STATES = frozenset((0, 4, 5, 6))
    _RESULT_UNKNOWN = 0
    _STATE_UNKNOWN = 0
    _ALLOWED_RESULT_SOURCES = frozenset(((SYSTEM_BOAT, COMPONENT_PI),
                                         (SYSTEM_BOAT, COMPONENT_MOTOR)))

    def __init__(self, transport: Transport, *, system: int = SYSTEM_GROUND,
                 component: int = COMPONENT_GROUND,
                 clock: Callable[[], float] = time.monotonic) -> None:
        self.transport = transport
        self.system = self._uint(system, 8, "system")
        self.component = self._uint(component, 8, "component")
        self._clock = clock
        self._session: int | None = None
        self._next_request_id = 1
        self._frame_sequence = 0
        self._next_ping_seq = 0
        self._subscribers: list[Callable[[object], None]] = []
        self._last_seen: dict[tuple[int, int], float] = {}
        self._sensor_valid_fields: dict[tuple[int, int], int] = {}
        self._boot_ids: dict[tuple[int, int], int] = {}
        self._ping_sent: dict[int, tuple[int, float]] = {}
        self._seen_pongs: dict[tuple[int, int, int, int], float] = {}
        self._pending_tickets: dict[tuple[int, int], RequestTicket] = {}
        self._pending_at: dict[tuple[int, int], float] = {}
        self._seen_outcomes: dict[tuple[int, int, int, int, int, int, int], float] = {}
        self._reconcile_health = False
        self._reconcile_status = False
        self._reconciled = False

    @property
    def ground_session(self) -> int:
        if self._session is None:
            raise RuntimeError("client is disconnected")
        return self._session

    def connect(self) -> None:
        if self._session is not None:
            self.disconnect()
        self._clear_connection_state(emit_unknown=True)
        new_session = secrets.randbits(32) or 1
        try:
            self.transport.open(self._receive)
        except Exception as exc:
            # A partially opened transport must not leave a usable session behind.
            self._safe_close()
            self._publish(TransportEvent("error", f"open failed: {exc}"))
            raise
        self._session = new_session
        self._next_request_id = 1
        self._next_ping_seq = 0
        self._publish(TransportEvent("connected", self.transport.name))
        self._publish(TransportEvent("reconciliation_required",
                                     "awaiting motor NODE_HEALTH and ACTUATOR_STATUS"))

    def disconnect(self) -> None:
        if self._session is None:
            return
        session = self._session
        self._session = None
        self._clear_connection_state(emit_unknown=True, session=session)
        close_error = self._safe_close()
        if close_error is not None:
            self._publish(TransportEvent("error", f"close failed: {close_error}"))
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
        now = self._clock()
        self._prune(now)
        if self._next_ping_seq > 0xFFFFFFFF:
            raise OverflowError("ping sequence space exhausted for this session")
        seq = self._next_ping_seq
        self._next_ping_seq += 1
        timestamp = max(0, int(now * 1_000_000))
        self._ping_sent[seq] = (timestamp, now)
        while len(self._ping_sent) > self._MAX_PINGS:
            del self._ping_sent[next(iter(self._ping_sent))]
        self._send("PING", {"time_usec": timestamp, "seq": seq,
                             "target_system": 0, "target_component": 0})

    def subscribe(self, callback: Callable[[object], None]) -> Callable[[], None]:
        if not callable(callback):
            raise TypeError("callback must be callable")
        self._subscribers.append(callback)

        def unsubscribe() -> None:
            if callback in self._subscribers:
                self._subscribers.remove(callback)
        return unsubscribe

    def _movement(self, actuator: int, opcode: int, delta: int,
                  timeout_ms: int, max_rate_cdeg_s: int) -> RequestTicket:
        self._require_connected()
        if opcode != 3 and not (self._reconcile_health and self._reconcile_status):
            raise RuntimeError("fresh motor health and status are required before movement")
        actuator = self._uint(actuator, 8, "actuator")
        if actuator not in self._VALID_ACTUATORS:
            raise ValueError("actuator must be sail (1) or rudder (2)")
        self._sint(delta, 16, "position_cdeg" if opcode == 1 else "delta_cdeg")
        self._uint(timeout_ms, 16, "timeout_ms")
        self._uint(max_rate_cdeg_s, 16, "max_rate_cdeg_s")
        if opcode == 3 and delta != 0:
            raise ValueError("STOP requires a zero delta")
        now = self._clock()
        self._prune(now)
        if len(self._pending_tickets) >= self._MAX_PENDING:
            raise RuntimeError("too many unresolved movement requests")
        if self._next_request_id > 0xFFFFFFFF:
            raise OverflowError("request_id space exhausted for this session")
        request_id = self._next_request_id
        ticket = RequestTicket(request_id, self.ground_session)
        key = (ticket.ground_session, ticket.request_id)
        fields = {
            "request_id": request_id, "ground_session": ticket.ground_session,
            "delta_cdeg": delta, "timeout_ms": timeout_ms,
            "max_rate_cdeg_s": max_rate_cdeg_s, "target_system": SYSTEM_BOAT,
            "target_component": COMPONENT_MOTOR, "actuator": actuator,
            "opcode": opcode, "control_source": 3,
        }
        # Reserve correlation before sending; on failure it is finalized UNKNOWN.
        self._next_request_id += 1
        self._pending_tickets[key] = ticket
        self._pending_at[key] = now
        self._send("ACTUATOR_REQUEST", fields, affected=(ticket,))
        return ticket

    def _require_connected(self) -> None:
        if self._session is None:
            raise RuntimeError("client is disconnected")

    @staticmethod
    def _uint(value: int, bits: int, name: str) -> int:
        if isinstance(value, bool) or not isinstance(value, int) or not 0 <= value < (1 << bits):
            raise ValueError(f"{name} must be an unsigned {bits}-bit integer")
        return value

    @staticmethod
    def _sint(value: int, bits: int, name: str) -> int:
        if isinstance(value, bool) or not isinstance(value, int) or not -(1 << (bits - 1)) <= value < (1 << (bits - 1)):
            raise ValueError(f"{name} must be a signed {bits}-bit integer")
        return value

    def _send(self, name: str, fields: dict[str, int], *,
              affected: tuple[RequestTicket, ...] = ()) -> None:
        try:
            frame = codec.encode(name, fields, system=self.system, component=self.component,
                                 sequence=self._frame_sequence)
        except (KeyError, TypeError, ValueError, struct.error):
            # User input was checked before ticket allocation; don't leave phantom work.
            for ticket in affected:
                self._discard_ticket(ticket)
            raise
        try:
            self.transport.send_frame(frame)
        except Exception as exc:
            self._fail_transport(exc, affected=affected)
            raise
        self._frame_sequence = (self._frame_sequence + 1) & 0xFF
        self._publish(RawFrameEvent(frame, self.transport.name, "out"))

    def _receive(self, frame: bytes) -> None:
        # Ignore callbacks retained by a transport after disconnect/open failure.
        if self._session is None:
            return
        raw = bytes(frame)
        self._publish(RawFrameEvent(raw, self.transport.name, "in"))
        try:
            message = codec.decode(raw)
        except (ValueError, KeyError, struct.error, TypeError):
            return
        now = self._clock()
        self._prune(now)
        source = (message["system"], message["component"])
        self._last_seen[source] = now
        name, fields = message["name"], message["fields"]
        if name == "HEARTBEAT":
            self._publish(Heartbeat(*source, 0.0))
        elif name == "PING":
            if (fields["target_system"] != self.system or
                    fields["target_component"] != self.component):
                return
            tracked = self._ping_sent.get(fields["seq"])
            if tracked is None or tracked[0] != fields["time_usec"]:
                return
            pong_key = (*source, fields["seq"], fields["time_usec"])
            if pong_key in self._seen_pongs:
                return
            self._seen_pongs[pong_key] = now
            self._publish(PongReply(*source, fields["seq"], fields["time_usec"],
                                    max(0.0, (now - tracked[1]) * 1000.0)))
        elif name == "ACTUATOR_STATUS":
            if source == (SYSTEM_BOAT, COMPONENT_MOTOR):
                self._reconcile_status = True
                self._publish_reconciled_if_ready()
            self._publish(MotorReport(*(fields[key] for key in (
                "time_boot_ms", "command_id", "requested_position_cdeg",
                "measured_position_cdeg", "velocity_cdeg_s", "fault_flags",
                "actuator", "control_source", "result", "enabled")), now, source))
        elif name == "REQUEST_RESULT":
            self._handle_result(fields, source, now)
        elif name == "NODE_HEALTH":
            self._handle_health(fields, source, now)
        elif name == "SCALED_IMU":
            self._publish(ImuSample(fields["time_boot_ms"],
                                    (fields["xacc"], fields["yacc"], fields["zacc"]),
                                    (fields["xgyro"], fields["ygyro"], fields["zgyro"]),
                                    self._sensor_valid_fields.get(source, 0), source))

    def _handle_result(self, fields: dict[str, int], source: tuple[int, int], now: float) -> None:
        ticket_key = (fields["ground_session"], fields["request_id"])
        if (source not in self._ALLOWED_RESULT_SOURCES or
                ticket_key not in self._pending_tickets or
                fields["ground_session"] != self._session or
                fields["target_system"] != self.system or
                fields["target_component"] != self.component):
            return
        motor_boot_id = fields["motor_boot_id"]
        known_motor_boot = self._boot_ids.get((SYSTEM_BOAT, COMPONENT_MOTOR))
        changed_boot = known_motor_boot is not None and motor_boot_id != known_motor_boot
        if changed_boot:
            motor = (SYSTEM_BOAT, COMPONENT_MOTOR)
            self._sensor_valid_fields.pop(motor, None)
            self._reconcile_health = False
            self._reconcile_status = False
            self._reconciled = False
            self._publish(TransportEvent("reconciliation_required",
                                         "REQUEST_RESULT reports a changed motor boot"))
            self._invalidate_node_requests(motor, known_motor_boot, now)
            return
        values = dict(fields)
        key = (values["ground_session"], values["request_id"], values["motor_boot_id"],
               values["local_command_id"], values["result"], values["state"],
               values["target_system"] << 8 | values["target_component"])
        if key not in self._seen_outcomes:
            self._seen_outcomes[key] = now
            self._publish(RequestOutcome(*(values[field] for field in (
                "ground_session", "request_id", "motor_boot_id", "local_command_id",
                "result", "state", "target_system", "target_component")), source))
        if values["state"] in self._OUTCOME_TERMINAL_STATES:
            self._discard_ticket(self._pending_tickets[ticket_key])

    def _handle_health(self, fields: dict[str, int], source: tuple[int, int], now: float) -> None:
        boot_id = fields["boot_id"]
        old_boot = self._boot_ids.get(source)
        if old_boot is not None and old_boot != boot_id:
            self._sensor_valid_fields.pop(source, None)
            if source in ((SYSTEM_BOAT, COMPONENT_PI), (SYSTEM_BOAT, COMPONENT_MOTOR)):
                self._reconcile_health = False
                self._reconcile_status = False
                self._reconciled = False
                self._publish(TransportEvent("reconciliation_required",
                                             f"boot changed for {source[0]}/{source[1]}"))
            self._invalidate_node_requests(source, old_boot, now)
        self._boot_ids[source] = boot_id
        self._sensor_valid_fields[source] = fields["sensor_valid_fields"]
        if source == (SYSTEM_BOAT, COMPONENT_MOTOR):
            self._reconcile_health = True
            self._publish_reconciled_if_ready()
        self._publish(NodeHealthReport(*(fields[key] for key in (
            "boot_id", "time_boot_ms", "fault_flags", "sensor_valid_fields",
            "queue_drops", "reset_reason", "feedback_valid")), source))

    def _invalidate_node_requests(self, source: tuple[int, int], old_boot: int,
                                  now: float) -> None:
        if source not in ((SYSTEM_BOAT, COMPONENT_PI), (SYSTEM_BOAT, COMPONENT_MOTOR)):
            return
        for ticket in tuple(self._pending_tickets.values()):
            self._publish_unknown(ticket, old_boot, source)
            self._discard_ticket(ticket)
        self._seen_outcomes.clear()

    def _publish_unknown(self, ticket: RequestTicket, boot_id: int = 0,
                         source: tuple[int, int] | None = None) -> None:
        self._publish(RequestOutcome(ticket.ground_session, ticket.request_id,
                                     boot_id, 0, self._RESULT_UNKNOWN,
                                     self._STATE_UNKNOWN, self.system, self.component,
                                     source or (SYSTEM_BOAT, COMPONENT_PI)))

    def _publish_reconciled_if_ready(self) -> None:
        if (self._reconcile_health and self._reconcile_status and
                not self._reconciled):
            self._reconciled = True
            self._publish(TransportEvent("reconciled", "motor health and status received"))

    def _discard_ticket(self, ticket: RequestTicket) -> None:
        key = (ticket.ground_session, ticket.request_id)
        self._pending_tickets.pop(key, None)
        self._pending_at.pop(key, None)

    def _clear_connection_state(self, *, emit_unknown: bool,
                                session: int | None = None) -> None:
        tickets = tuple(self._pending_tickets.values())
        self._last_seen.clear()
        self._sensor_valid_fields.clear()
        self._boot_ids.clear()
        self._reconcile_health = False
        self._reconcile_status = False
        self._reconciled = False
        self._ping_sent.clear()
        self._seen_pongs.clear()
        self._seen_outcomes.clear()
        self._pending_tickets.clear()
        self._pending_at.clear()
        if emit_unknown:
            for ticket in tickets:
                if session is None or ticket.ground_session == session:
                    self._publish_unknown(ticket)

    def _fail_transport(self, exc: Exception, *,
                        affected: tuple[RequestTicket, ...] = ()) -> None:
        session = self._session
        self._session = None
        tickets = { (ticket.ground_session, ticket.request_id): ticket
                    for ticket in self._pending_tickets.values() }
        tickets.update({(ticket.ground_session, ticket.request_id): ticket
                        for ticket in affected})
        self._clear_connection_state(emit_unknown=False)
        for ticket in tickets.values():
            self._publish_unknown(ticket)
        self._safe_close()
        self._publish(TransportEvent("error", f"send failed; movement outcome unknown: {exc}"))
        if session is not None:
            self._publish(TransportEvent("disconnected", self.transport.name))

    def _safe_close(self) -> Exception | None:
        try:
            self.transport.close()
        except Exception as exc:  # cleanup is best-effort; state is already invalidated
            return exc
        return None

    def _prune(self, now: float) -> None:
        cutoff = now - self._TRACK_TTL
        for key, sent in tuple(self._ping_sent.items()):
            if sent[1] < cutoff:
                self._ping_sent.pop(key, None)
        for mapping in (self._seen_pongs, self._seen_outcomes):
            for key, seen_at in tuple(mapping.items()):
                if seen_at < cutoff:
                    mapping.pop(key, None)
        for source, seen_at in tuple(self._last_seen.items()):
            if seen_at < cutoff:
                self._last_seen.pop(source, None)
        self._trim(self._last_seen, self._MAX_SEEN)
        self._trim(self._sensor_valid_fields, self._MAX_SEEN)
        # Keep the two safety-relevant boot identities while bounding incidental peers.
        self._trim(self._sensor_valid_fields, self._MAX_SEEN)
        self._trim(self._boot_ids, self._MAX_SEEN)
        for key, ticket in tuple(self._pending_tickets.items()):
            if self._pending_at.get(key, now) < cutoff:
                self._publish_unknown(ticket)
                self._discard_ticket(ticket)
        self._trim(self._seen_pongs, self._MAX_SEEN)
        self._trim(self._seen_outcomes, self._MAX_SEEN)

    @staticmethod
    def _trim(mapping: dict, limit: int) -> None:
        while len(mapping) > limit:
            del mapping[next(iter(mapping))]

    def _publish(self, event: object) -> None:
        for callback in tuple(self._subscribers):
            try:
                callback(event)
            except Exception:
                # Subscribers are observers; one broken observer cannot stop IO dispatch.
                continue
