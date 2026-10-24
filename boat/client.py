"""Transport-independent ground client for the frozen boat contract."""

from __future__ import annotations

import math
import secrets
import struct
import threading
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
class ImuReading:
    """Correlated nine-axis IMU reply, including explicit invalid/error state."""

    ground_session: int
    request_id: int
    time_boot_ms: int
    sample_seq: int
    acquisition_boot_id: int
    source_boot_id: int
    sample_age_ms: int
    valid_fields: int
    accel_mg: tuple[int, int, int]
    gyro_mrad_s: tuple[int, int, int]
    mag_mgauss: tuple[int, int, int]
    status: int
    path: int
    acquisition_component: int
    target_system: int
    target_component: int
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
    _STATUS_IN_PROGRESS = 1
    _DEFAULT_MOTOR_FRESHNESS_S = 2.0
    _ALLOWED_RESULT_SOURCES = frozenset(((SYSTEM_BOAT, COMPONENT_PI),
                                         (SYSTEM_BOAT, COMPONENT_MOTOR)))
    _IMU_STATUS_NAMES = {1: "OK", 2: "NOT_READY", 3: "UNAVAILABLE",
                         4: "TIMEOUT", 5: "REJECTED"}
    _IMU_PATHS = {"direct": 0, "board": 1}
    _IMU_ALLOWED_SOURCES = {0: (SYSTEM_BOAT, COMPONENT_PI),
                            1: (SYSTEM_BOAT, 159)}

    def __init__(self, transport: Transport, *, system: int = SYSTEM_GROUND,
                 component: int = COMPONENT_GROUND,
                 clock: Callable[[], float] = time.monotonic,
                 motor_freshness_s: float = _DEFAULT_MOTOR_FRESHNESS_S) -> None:
        self.transport = transport
        self.system = self._uint(system, 8, "system")
        self.component = self._uint(component, 8, "component")
        self._clock = clock
        if (isinstance(motor_freshness_s, bool) or
                not isinstance(motor_freshness_s, (int, float)) or
                not math.isfinite(motor_freshness_s) or motor_freshness_s <= 0):
            raise ValueError("motor_freshness_s must be a positive number")
        self._motor_freshness_s = float(motor_freshness_s)
        self._state_lock = threading.RLock()
        self._session: int | None = None
        self._connection_generation = 0
        self._opening_generation: int | None = None
        self._opening_frames: list[bytes] = []
        self._next_request_id = 1
        self._next_imu_request_id = 1
        self._frame_sequence = 0
        self._next_ping_seq = 0
        self._subscribers: list[Callable[[object], None]] = []
        self._last_seen: dict[tuple[int, int], float] = {}
        self._sensor_valid_fields: dict[tuple[int, int], int] = {}
        self._boot_ids: dict[tuple[int, int], int] = {}
        self._health_at: dict[tuple[int, int], float] = {}
        self._motor_health_at: float | None = None
        self._motor_feedback_valid = False
        self._motor_status_at: float | None = None
        self._motor_enabled = False
        self._motor_busy = False
        self._ping_sent: dict[int, tuple[int, float]] = {}
        self._seen_pongs: dict[tuple[int, int, int, int], float] = {}
        self._pending_tickets: dict[tuple[int, int], RequestTicket] = {}
        self._pending_at: dict[tuple[int, int], float] = {}
        self._pending_imu: dict[tuple[int, int], tuple[RequestTicket, int, float]] = {}
        self._seen_imu: dict[tuple[int, ...], float] = {}
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
        with self._state_lock:
            if self._session is not None:
                self.disconnect()
            self._clear_connection_state(emit_unknown=True)
            new_session = secrets.randbits(32) or 1
            self._connection_generation += 1
            generation = self._connection_generation
            self._opening_generation = generation
            self._opening_frames = []
            try:
                self.transport.open(lambda frame: self._transport_frame(generation, frame))
            except Exception as exc:
                # An open-time callback is not accepted unless open commits successfully.
                self._connection_generation += 1
                self._opening_generation = None
                self._opening_frames = []
                self._safe_close()
                self._publish(TransportEvent("error", f"open failed: {exc}"))
                raise
            queued_frames = self._opening_frames
            self._opening_frames = []
            self._opening_generation = None
            self._session = new_session
            self._next_request_id = 1
            self._next_imu_request_id = 1
            self._next_ping_seq = 0
            self._publish(TransportEvent("connected", self.transport.name))
            if self._session != new_session:
                return
            self._publish(TransportEvent("reconciliation_required",
                                         "awaiting motor NODE_HEALTH and ACTUATOR_STATUS"))
            for frame in queued_frames:
                self._receive(frame, generation)

    def disconnect(self) -> None:
        with self._state_lock:
            if self._session is None:
                return
            session = self._session
            self._session = None
            self._connection_generation += 1
            self._opening_generation = None
            self._opening_frames = []
            tickets = tuple(self._pending_tickets.values())
            motor_boot = self._boot_ids.get((SYSTEM_BOAT, COMPONENT_MOTOR), 0)
            self._clear_connection_state(emit_unknown=False, session=session)
            close_error = self._safe_close()
            for ticket in tickets:
                if ticket.ground_session == session:
                    self._publish_unknown(ticket, motor_boot)
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

    def request_imu(self, path: str = "direct", *, timeout_ms: int = 1000,
                    max_age_ms: int = 0) -> RequestTicket:
        """Request one IMU sample without requiring motor readiness."""
        with self._state_lock:
            self._require_connected()
            if path not in self._IMU_PATHS:
                raise ValueError("path must be 'direct' or 'board'")
            self._uint(timeout_ms, 16, "timeout_ms")
            if not 1 <= timeout_ms <= 5000:
                raise ValueError("timeout_ms must be between 1 and 5000")
            self._uint(max_age_ms, 16, "max_age_ms")
            now = self._clock()
            session = self.ground_session
            self._prune(now)
            if self._session != session:
                raise RuntimeError("client session changed while expiring IMU requests")
            if len(self._pending_imu) >= self._MAX_PENDING:
                raise RuntimeError("too many unresolved IMU requests")
            if self._next_imu_request_id > 0xFFFFFFFF:
                raise OverflowError("IMU request_id space exhausted for this session")
            request_id = self._next_imu_request_id
            self._next_imu_request_id += 1
            ticket = RequestTicket(request_id, session)
            key = (ticket.ground_session, request_id)
            path_id = self._IMU_PATHS[path]
            self._pending_imu[key] = (ticket, path_id, now + timeout_ms / 1000.0)
            self._send("IMU_REQUEST", {"ground_session": ticket.ground_session,
                "request_id": request_id, "timeout_ms": timeout_ms,
                "max_age_ms": max_age_ms, "path": path_id,
                "target_system": self.system, "target_component": self.component})
            return ticket

    def poll_imu_requests(self) -> None:
        """Expire IMU requests; call periodically from event-loop driven clients."""
        with self._state_lock:
            self._require_connected()
            self._prune(self._clock())

    def ping_all(self) -> None:
        with self._state_lock:
            self._ping_all()

    def _ping_all(self) -> None:
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
        with self._state_lock:
            self._subscribers.append(callback)

        def unsubscribe() -> None:
            with self._state_lock:
                if callback in self._subscribers:
                    self._subscribers.remove(callback)
        return unsubscribe

    def _movement(self, actuator: int, opcode: int, delta: int,
                  timeout_ms: int, max_rate_cdeg_s: int) -> RequestTicket:
        with self._state_lock:
            return self._movement_locked(actuator, opcode, delta, timeout_ms,
                                         max_rate_cdeg_s)

    def _movement_locked(self, actuator: int, opcode: int, delta: int,
                         timeout_ms: int, max_rate_cdeg_s: int) -> RequestTicket:
        self._require_connected()
        now = self._clock()
        self._prune(now)
        if opcode != 3:
            self._require_motor_ready(now)
        actuator = self._uint(actuator, 8, "actuator")
        if actuator not in self._VALID_ACTUATORS:
            raise ValueError("actuator must be sail (1) or rudder (2)")
        self._sint(delta, 16, "position_cdeg" if opcode == 1 else "delta_cdeg")
        self._uint(timeout_ms, 16, "timeout_ms")
        self._uint(max_rate_cdeg_s, 16, "max_rate_cdeg_s")
        if opcode == 3 and delta != 0:
            raise ValueError("STOP requires a zero delta")
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

    def _require_motor_ready(self, now: float) -> None:
        health_fresh = (self._motor_health_at is not None and
                        now - self._motor_health_at <= self._motor_freshness_s)
        status_fresh = (self._motor_status_at is not None and
                        now - self._motor_status_at <= self._motor_freshness_s)
        if not (self._reconcile_health and self._reconcile_status and
                health_fresh and status_fresh):
            raise RuntimeError("fresh motor health and status are required before movement")
        if not self._motor_feedback_valid:
            raise RuntimeError("fresh valid motor feedback is required before movement")
        if self._motor_busy:
            raise RuntimeError("motor is busy")

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

    def _transport_frame(self, generation: int, frame: bytes) -> None:
        with self._state_lock:
            if generation != self._connection_generation:
                return
            if self._session is None:
                if self._opening_generation == generation:
                    self._opening_frames.append(bytes(frame))
                return
            self._receive(frame, generation)

    def _receive(self, frame: bytes, generation: int | None = None) -> None:
        with self._state_lock:
            self._receive_locked(frame, generation)

    def _receive_locked(self, frame: bytes, generation: int | None) -> None:
        # Ignore callbacks retained by a transport after disconnect/open failure.
        if (self._session is None or
                (generation is not None and generation != self._connection_generation)):
            return
        raw = bytes(frame)
        self._publish(RawFrameEvent(raw, self.transport.name, "in"))
        try:
            message = codec.decode(raw)
        except (ValueError, KeyError, struct.error, TypeError) as exc:
            self._publish(TransportEvent("protocol_error", f"invalid frame: {exc}"))
            return
        now = self._clock()
        self._prune(now)
        if self._session is None:
            return
        source = (message["system"], message["component"])
        self._last_seen[source] = now
        self._trim(self._last_seen, self._MAX_SEEN)
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
            self._trim(self._seen_pongs, self._MAX_SEEN)
            self._publish(PongReply(*source, fields["seq"], fields["time_usec"],
                                    max(0.0, (now - tracked[1]) * 1000.0)))
        elif name == "ACTUATOR_STATUS":
            if source == (SYSTEM_BOAT, COMPONENT_MOTOR):
                self._reconcile_status = True
                self._motor_status_at = now
                self._motor_enabled = bool(fields["enabled"])
                self._motor_busy = fields["result"] == self._STATUS_IN_PROGRESS
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
        elif name == "IMU_SAMPLE":
            self._handle_imu_sample(fields, source, now)

    def _handle_imu_sample(self, fields: dict[str, int], source: tuple[int, int],
                           now: float) -> None:
        key = (fields["ground_session"], fields["request_id"])
        pending = self._pending_imu.get(key)
        if pending is None:
            return
        _ticket, expected_path, deadline = pending
        expected_source = self._IMU_ALLOWED_SOURCES[expected_path]
        if (now > deadline or source != expected_source or
                fields["path"] != expected_path or
                fields["target_system"] != self.system or
                fields["target_component"] != self.component or
                fields["acquisition_component"] != COMPONENT_PI or
                fields["status"] not in self._IMU_STATUS_NAMES or
                fields["valid_fields"] & ~7 or
                (fields["status"] != 1 and fields["valid_fields"] != 0)):
            return
        invalid_axes = (
            (fields["valid_fields"] & 1, (fields["xacc"], fields["yacc"], fields["zacc"])),
            (fields["valid_fields"] & 2, (fields["xgyro"], fields["ygyro"], fields["zgyro"])),
            (fields["valid_fields"] & 4, (fields["xmag"], fields["ymag"], fields["zmag"])),
        )
        if any(not bit and any(values) for bit, values in invalid_axes):
            return
        known_pi_boot = self._boot_ids.get((SYSTEM_BOAT, COMPONENT_PI))
        known_source_boot = self._boot_ids.get(source)
        if (fields["acquisition_boot_id"] == 0 or fields["source_boot_id"] == 0 or
                (expected_path == 0 and
                 fields["source_boot_id"] != fields["acquisition_boot_id"]) or
                (known_pi_boot is not None and fields["acquisition_boot_id"] != known_pi_boot) or
                (known_source_boot is not None and fields["source_boot_id"] != known_source_boot)):
            return
        dedupe = (key[0], key[1], fields["acquisition_boot_id"],
                  fields["source_boot_id"], fields["sample_seq"], expected_path,
                  fields["status"])
        if dedupe in self._seen_imu:
            return
        self._seen_imu[dedupe] = now
        self._trim(self._seen_imu, self._MAX_SEEN)
        self._pending_imu.pop(key, None)
        reading = ImuReading(fields["ground_session"], fields["request_id"],
            fields["time_boot_ms"], fields["sample_seq"], fields["acquisition_boot_id"],
            fields["source_boot_id"], fields["sample_age_ms"], fields["valid_fields"],
            (fields["xacc"], fields["yacc"], fields["zacc"]),
            (fields["xgyro"], fields["ygyro"], fields["zgyro"]),
            (fields["xmag"], fields["ymag"], fields["zmag"]), fields["status"],
            fields["path"], fields["acquisition_component"], fields["target_system"],
            fields["target_component"], source)
        self._publish(reading)

    def _publish_imu_failure(self, ticket: RequestTicket, path: int, status: int) -> None:
        self._publish(ImuReading(ticket.ground_session, ticket.request_id, 0, 0, 0, 0,
            0xFFFF, 0, (0, 0, 0), (0, 0, 0), (0, 0, 0), status, path,
            COMPONENT_PI, self.system, self.component, self._IMU_ALLOWED_SOURCES[path]))

    def _handle_result(self, fields: dict[str, int], source: tuple[int, int], now: float) -> None:
        ticket_key = (fields["ground_session"], fields["request_id"])
        if (source not in self._ALLOWED_RESULT_SOURCES or
                ticket_key not in self._pending_tickets or
                fields["ground_session"] != self._session or
                fields["target_system"] != self.system or
                fields["target_component"] != self.component):
            return
        ticket = self._pending_tickets[ticket_key]
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
            self._trim(self._seen_outcomes, self._MAX_SEEN)
            self._publish(RequestOutcome(*(values[field] for field in (
                "ground_session", "request_id", "motor_boot_id", "local_command_id",
                "result", "state", "target_system", "target_component")), source))
        if values["state"] in self._OUTCOME_TERMINAL_STATES:
            # A subscriber may disconnect during event delivery, clearing this ticket.
            if self._pending_tickets.get(ticket_key) == ticket:
                self._discard_ticket(ticket)

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
            self._invalidate_imu_requests(source)
            if self._session is None:
                return
        self._boot_ids[source] = boot_id
        self._trim_preserving(self._boot_ids, self._MAX_SEEN,
                              {(SYSTEM_BOAT, COMPONENT_PI),
                               (SYSTEM_BOAT, COMPONENT_MOTOR)})
        self._sensor_valid_fields[source] = fields["sensor_valid_fields"]
        self._trim_preserving(self._sensor_valid_fields, self._MAX_SEEN,
                              {(SYSTEM_BOAT, COMPONENT_MOTOR)})
        self._health_at[source] = now
        self._trim_health_tracker()
        if source == (SYSTEM_BOAT, COMPONENT_MOTOR):
            self._reconcile_health = True
            self._motor_health_at = now
            self._motor_feedback_valid = bool(fields["feedback_valid"])
            self._publish_reconciled_if_ready()
        self._publish(NodeHealthReport(*(fields[key] for key in (
            "boot_id", "time_boot_ms", "fault_flags", "sensor_valid_fields",
            "queue_drops", "reset_reason", "feedback_valid")), source))

    def _invalidate_node_requests(self, source: tuple[int, int], old_boot: int,
                                  now: float) -> None:
        if source not in ((SYSTEM_BOAT, COMPONENT_PI), (SYSTEM_BOAT, COMPONENT_MOTOR)):
            return
        for ticket in tuple(self._pending_tickets.values()):
            self._publish_unknown(ticket, source=source)
            self._discard_ticket(ticket)
        self._seen_outcomes.clear()

    def _invalidate_imu_requests(self, source: tuple[int, int]) -> None:
        if source == (SYSTEM_BOAT, COMPONENT_PI):
            matching = tuple(self._pending_imu.items())
        elif source == self._IMU_ALLOWED_SOURCES[1]:
            matching = tuple((key, value) for key, value in self._pending_imu.items()
                             if value[1] == 1)
        else:
            return
        for key, (ticket, path, _deadline) in matching:
            self._pending_imu.pop(key, None)
            self._publish_imu_failure(ticket, path, 3)

    def _publish_unknown(self, ticket: RequestTicket, boot_id: int | None = None,
                         source: tuple[int, int] | None = None) -> None:
        if boot_id is None:
            boot_id = self._boot_ids.get((SYSTEM_BOAT, COMPONENT_MOTOR), 0)
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
        imu_tickets = tuple(self._pending_imu.values())
        motor_boot = self._boot_ids.get((SYSTEM_BOAT, COMPONENT_MOTOR), 0)
        self._last_seen.clear()
        self._sensor_valid_fields.clear()
        self._boot_ids.clear()
        self._health_at.clear()
        self._motor_health_at = None
        self._motor_feedback_valid = False
        self._motor_status_at = None
        self._motor_enabled = False
        self._motor_busy = False
        self._reconcile_health = False
        self._reconcile_status = False
        self._reconciled = False
        self._ping_sent.clear()
        self._seen_pongs.clear()
        self._seen_outcomes.clear()
        self._pending_tickets.clear()
        self._pending_at.clear()
        self._pending_imu.clear()
        self._seen_imu.clear()
        if emit_unknown:
            for ticket in tickets:
                if session is None or ticket.ground_session == session:
                    self._publish_unknown(ticket, motor_boot)
        for ticket, path, _deadline in imu_tickets:
            self._publish_imu_failure(ticket, path, 3)

    def _fail_transport(self, exc: Exception, *,
                        affected: tuple[RequestTicket, ...] = ()) -> None:
        session = self._session
        self._session = None
        tickets = { (ticket.ground_session, ticket.request_id): ticket
                    for ticket in self._pending_tickets.values() }
        tickets.update({(ticket.ground_session, ticket.request_id): ticket
                        for ticket in affected})
        motor_boot = self._boot_ids.get((SYSTEM_BOAT, COMPONENT_MOTOR), 0)
        self._clear_connection_state(emit_unknown=False)
        for ticket in tickets.values():
            self._publish_unknown(ticket, motor_boot)
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
        for source, health_at in tuple(self._health_at.items()):
            if now - health_at > self._motor_freshness_s:
                self._health_at.pop(source, None)
                self._sensor_valid_fields.pop(source, None)
                if source == (SYSTEM_BOAT, COMPONENT_MOTOR):
                    self._motor_health_at = None
                    self._motor_feedback_valid = False
                    self._reconcile_health = False
                    self._reconciled = False
        if (self._motor_status_at is not None and
                now - self._motor_status_at > self._motor_freshness_s):
            self._motor_status_at = None
            self._motor_enabled = False
            self._motor_busy = False
            self._reconcile_status = False
            self._reconciled = False
        self._trim(self._last_seen, self._MAX_SEEN)
        for key, ticket in tuple(self._pending_tickets.items()):
            if self._pending_at.get(key, now) < cutoff:
                self._publish_unknown(ticket)
                self._discard_ticket(ticket)
                # An ambiguous expired movement must be reconciled with a newer snapshot.
                self._motor_status_at = None
                self._motor_enabled = False
                self._motor_busy = False
                self._reconcile_status = False
                self._reconciled = False
        for key, (ticket, path, deadline) in tuple(self._pending_imu.items()):
            if now >= deadline:
                self._pending_imu.pop(key, None)
                self._publish_imu_failure(ticket, path, 4)
        for key, seen_at in tuple(self._seen_imu.items()):
            if seen_at < cutoff:
                self._seen_imu.pop(key, None)
        # Keep the Pi and motor boot identities while bounding incidental peers.
        self._trim_preserving(self._boot_ids, self._MAX_SEEN,
                              {(SYSTEM_BOAT, COMPONENT_PI),
                               (SYSTEM_BOAT, COMPONENT_MOTOR)})
        self._trim_preserving(self._sensor_valid_fields, self._MAX_SEEN,
                              {(SYSTEM_BOAT, COMPONENT_MOTOR)})
        self._trim(self._seen_pongs, self._MAX_SEEN)
        self._trim(self._seen_outcomes, self._MAX_SEEN)

    @staticmethod
    def _trim(mapping: dict, limit: int) -> None:
        while len(mapping) > limit:
            del mapping[next(iter(mapping))]

    @staticmethod
    def _trim_preserving(mapping: dict, limit: int, preserved: set) -> None:
        while len(mapping) > limit:
            oldest = next((key for key in mapping if key not in preserved), None)
            if oldest is None:
                break
            del mapping[oldest]

    def _trim_health_tracker(self) -> None:
        preserved = {(SYSTEM_BOAT, COMPONENT_PI), (SYSTEM_BOAT, COMPONENT_MOTOR)}
        while len(self._health_at) > self._MAX_SEEN:
            oldest = next((key for key in self._health_at if key not in preserved), None)
            if oldest is None:
                break
            del self._health_at[oldest]
            self._sensor_valid_fields.pop(oldest, None)

    def _publish(self, event: object) -> None:
        for callback in tuple(self._subscribers):
            try:
                callback(event)
            except Exception:
                # Subscribers are observers; one broken observer cannot stop IO dispatch.
                continue
