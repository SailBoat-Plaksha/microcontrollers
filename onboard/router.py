"""Validated routing and a single serialized scheduler for the shared Pi bus."""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
import secrets
import struct
import time
from typing import Callable

from boat import ids
from protocol import codec

from .bus import I2CBus
from .config import ServiceConfig

ALLOWED_MESSAGES = {
    "HEARTBEAT", "PING", "SCALED_IMU", "NODE_INFO", "ACTUATOR_STATUS",
    "CONTROL_AUTHORITY", "LINK_STATUS", "ACTUATOR_REQUEST", "NODE_HEALTH",
    "REQUEST_RESULT", "ENVIRONMENTAL_SAMPLE", "POWER_STATE",
}
TERMINAL_STATES = {4, 5, 6}


@dataclass(frozen=True)
class RouterEvent:
    kind: str
    at: float
    detail: str
    frame: bytes | None = None
    source: tuple[int, int] | None = None


@dataclass
class _Write:
    address: int
    frame: bytes
    priority: int
    key: tuple[int, int, int, int] | None = None
    queued_at: float = 0.0
    motion: bool = False


class BoatRouter:
    """Injectable service API. Call ``step`` from exactly one service process."""

    def __init__(self, bus: I2CBus, config: ServiceConfig | None = None, *,
                 clock: Callable[[], float] = time.monotonic,
                 on_frame: Callable[[bytes], None] | None = None,
                 on_event: Callable[[RouterEvent], None] | None = None):
        self.bus = bus
        self.config = config or ServiceConfig()
        self.clock = clock
        self.on_frame = on_frame or (lambda _frame: None)
        self.on_event = on_event or (lambda _event: None)
        self.boot_id = secrets.randbits(32) or 1
        self._boot_at = clock()
        self._sequence = 0
        self._normal: deque[_Write] = deque()
        self._stops: deque[_Write] = deque()
        self._stop_limit = max(1, self.config.queue_limit)
        self._poll_addresses = [self.config.motor_address, self.config.comms_address]
        if self.config.imu:
            self._poll_addresses.append(self.config.imu.address)
        self._poll_index = 0
        self._consecutive_writes = 0
        self._sessions: dict[tuple[int, int], int] = {}
        self._retired_sessions: deque[tuple[int, int, int]] = deque(maxlen=64)
        self._highest_request: dict[tuple[int, int, int], int] = {}
        self._requests: dict[tuple[int, int, int, int], dict[str, int | str]] = {}
        self._request_routes: dict[tuple[int, int, int, int], str] = {}
        self.snapshots: dict[tuple[int, int], dict[str, bytes]] = {}
        self.node_last_seen: dict[tuple[int, int], float] = {}
        self.node_ack_at: dict[tuple[int, int], float] = {}
        self.node_present: dict[tuple[int, int], bool] = {}
        self.node_boot_ids: dict[tuple[int, int], int] = {}
        self.node_health: dict[tuple[int, int], tuple[float, int, int, int]] = {}
        self.motor_status_at: dict[int, float] = {}
        self.motor_status: dict[int, dict[str, int]] = {}
        self.motor_reconcile_required = True
        self._motor_reference_seen = False
        self.imu_valid_fields = 0
        self.imu_last_sample: tuple[int, int, int, int, int, int] | None = None
        self.imu_last_seen: float | None = None
        self.imu_error: str | None = None
        self.bus_errors = 0
        self.queue_drops = 0
        self._last_imu_poll = float("-inf")
        self._last_heartbeat = self._boot_at
        self._last_health = self._boot_at

    @property
    def queued_writes(self) -> int:
        return len(self._stops) + len(self._normal)

    def submit_frame(self, frame: bytes, *, ingress: str = "ssh") -> bool:
        """Accept one complete raw frame from SSH or the comms mailbox."""
        try:
            message = codec.decode(frame)
        except (ValueError, KeyError, struct.error) as exc:
            self._event("rejected", f"invalid frame: {exc}", frame)
            return False
        if len(frame) > ids.MAILBOX_FRAME:
            self._event("rejected", "frame exceeds 64-byte mailbox", frame)
            return False
        if message["name"] not in ALLOWED_MESSAGES:
            self._event("rejected", "message is not allowlisted", frame)
            return False
        if ingress not in ("ssh", "comms"):
            self._event("rejected", "unknown ingress", frame)
            return False
        if (message["system"], message["component"]) != (ids.SYSTEM_GROUND, ids.COMPONENT_GROUND):
            self._event("rejected", "command source must be ground 255/190", frame)
            return False
        if ingress == "comms" and message["name"] not in ("PING", "ACTUATOR_REQUEST"):
            self._event("rejected", "comms ingress accepts only PING and ACTUATOR_REQUEST", frame)
            return False
        name, fields = message["name"], message["fields"]
        if name == "ACTUATOR_REQUEST":
            return self._accept_request(frame, fields, ingress)
        if name == "PING":
            if (fields["target_system"], fields["target_component"]) == (0, 0):
                self._queue_ping_reply(message, ingress)
                if ingress != "comms":
                    self._queue(frame, self.config.comms_address, 1)
                self._queue(frame, self.config.motor_address, 1)
                return True
            self._event("rejected", "only broadcast PING requests are accepted", frame)
            return False
        self._event("rejected", "ground message is not a command", frame)
        return False

    def step(self) -> bool:
        """Perform at most one bus operation; safe to drive with an injected clock."""
        now = self.clock()
        for address in (self.config.motor_address, self.config.comms_address):
            identity = self._node_for_address(address)
            seen = self.node_last_seen.get(identity)
            self.node_present[identity] = seen is not None and now - seen <= 5.0
        self._emit_periodic()
        if self._stops and self._consecutive_writes < 3:
            write = self._stops.popleft()
            return self._write(write)
        if self._normal and self._consecutive_writes < 3:
            write = self._normal.popleft()
            return self._write(write)
        self._consecutive_writes = 0
        address = self._poll_addresses[self._poll_index]
        self._poll_index = (self._poll_index + 1) % len(self._poll_addresses)
        if self.config.imu and address == self.config.imu.address:
            now = self.clock()
            interval = self.config.imu.interval_ms / 1000.0
            if now - self._last_imu_poll < interval:
                return False
            return self._poll_imu()
        return self._poll_node(address)

    def run_forever(self, *, stop: Callable[[], bool] | None = None,
                    sleep: Callable[[float], None] = time.sleep) -> None:
        should_stop = stop or (lambda: False)
        delay = self.config.poll_interval_ms / 1000.0
        while not should_stop():
            self.step()
            sleep(delay)

    def close(self) -> None:
        self.bus.close()

    def _accept_request(self, original: bytes, fields: dict[str, int], ingress: str) -> bool:
        key = (ids.SYSTEM_GROUND, ids.COMPONENT_GROUND, fields["ground_session"])
        request_key = (*key, fields["request_id"])
        if fields["ground_session"] == 0 or fields["request_id"] == 0:
            self._reject_request(fields, ingress, result=3, state=6,
                                 reason="ground_session and request_id must be nonzero")
            return False
        is_stop = fields["opcode"] == 3
        if (*key[:2], key[2]) in self._retired_sessions and not is_stop:
            self._event("session", "delayed request from a retired ground session rejected", original)
            return False
        if (fields["target_system"], fields["target_component"]) != (ids.SYSTEM_BOAT, self.config.motor_component):
            self._reject_request(fields, ingress, result=3, state=6, reason="wrong movement target")
            return False
        if fields["control_source"] != 3:
            self._reject_request(fields, ingress, result=2, state=6, reason="authority must be REMOTE_OPERATOR (3)")
            return False
        if fields["actuator"] not in (1, 2) or fields["opcode"] not in (1, 2, 3):
            self._reject_request(fields, ingress, result=3, state=6, reason="invalid actuator or opcode")
            return False
        if fields["opcode"] == 3 and fields["delta_cdeg"] != 0:
            self._reject_request(fields, ingress, result=3, state=6, reason="STOP delta must be zero")
            return False
        if fields["opcode"] != 3:
            health = self.node_health.get((ids.SYSTEM_BOAT, self.config.motor_component))
            if health is None or self.clock() - health[0] > 5.0 or health[3] != 0:
                self._reject_request(fields, ingress, result=4, state=0,
                                     reason="fresh fault-free motor health is required before movement")
                return False
            actuator_status = self.motor_status.get(fields["actuator"])
            status_at = self.motor_status_at.get(fields["actuator"])
            if (actuator_status is None or status_at is None or self.clock() - status_at > 5.0 or
                    actuator_status["fault_flags"] != 0):
                self._reject_request(fields, ingress, result=4, state=0,
                                     reason="fresh fault-free status for selected actuator is required")
                return False
            if actuator_status["result"] == 1:
                self._reject_request(fields, ingress, result=4, state=0,
                                     reason="selected actuator is busy")
                return False
            if actuator_status["control_source"] != 3:
                self._reject_request(fields, ingress, result=2, state=6,
                                     reason="selected actuator is owned by another control source")
                return False
            if fields["opcode"] == 2 and not health[2]:
                self._reject_request(fields, ingress, result=3, state=6,
                                     reason="motor feedback is not marked valid; STEP not forwarded")
                return False
        if request_key in self._requests:
            self._event("duplicate", "duplicate request suppressed", original)
            self._send_saved_result(request_key, ingress)
            return True
        if not is_stop and len(self._normal) >= self.config.queue_limit:
            self.queue_drops += 1
            self._reject_request(fields, ingress, result=0, state=0, reason="bounded write queue is full")
            return False
        current_session = self._sessions.get(key[:2])
        if current_session is not None and current_session != fields["ground_session"]:
            if not is_stop:
                if self._has_active_motion(key[:2], current_session):
                    self._reject_request(fields, ingress, result=4, state=6,
                                         reason="another ground session owns pending or in-flight motion")
                    return False
                status_at = self.motor_status_at.get(fields["actuator"])
                if status_at is None or self.clock() - status_at > 5.0:
                    self._reject_request(fields, ingress, result=4, state=0,
                                         reason="fresh selected actuator status is required to change session")
                    return False
            if len(self._retired_sessions) == self._retired_sessions.maxlen:
                self._reject_request(fields, ingress, result=4, state=6,
                                     reason="session rotation limit reached; restart router to clear replay guard")
                return False
            self._retired_sessions.append((*key[:2], current_session))
            kept_normal, kept_stops, queued_keys = deque(), deque(), set()
            for queue, kept in ((self._normal, kept_normal), (self._stops, kept_stops)):
                while queue:
                    item = queue.popleft()
                    if item.key and item.key[:3] == (*key[:2], current_session):
                        queued_keys.add(item.key)
                    else:
                        kept.append(item)
            self._normal, self._stops = kept_normal, kept_stops
            for old_key, old_record in list(self._requests.items()):
                if (old_key[:2] == key[:2] and old_key[2] == current_session and
                        int(old_record.get("state", 0)) not in TERMINAL_STATES):
                    if old_key in queued_keys:
                        self._make_result(old_key, self.node_boot_ids.get((ids.SYSTEM_BOAT, self.config.motor_component), 0), 0, 4, 6,
                                          "session changed before bus write; request marked stale")
                    else:
                        self._make_result(old_key, self.node_boot_ids.get((ids.SYSTEM_BOAT, self.config.motor_component), 0), 0, 0, 0,
                                          "session changed after bus write; outcome unknown")
            self._event("session", "ground session changed; old requests marked unknown")
            self._highest_request.pop((*key[:2], current_session), None)
        self._sessions[key[:2]] = fields["ground_session"]
        high_key = (*key[:2], fields["ground_session"])
        previous = self._highest_request.get(high_key, 0)
        if fields["request_id"] <= previous:
            self._reject_request(fields, ingress, result=4, state=6, reason="reordered or stale request ID")
            return False
        self._prune_ledgers()
        ledger_limit = max(16, self.config.queue_limit * 4)
        if len(self._requests) >= ledger_limit and (not is_stop or len(self._requests) >= ledger_limit + self._stop_limit):
            self._reject_request(fields, ingress, result=0, state=0,
                                 reason="bounded request ledger is full")
            return False
        self._highest_request[high_key] = fields["request_id"]
        self._remember_request(request_key, fields, ingress)
        # Rewrite only the MAVLink source header. The STEP delta and correlation payload
        # reach the motor unchanged, where STEP is resolved exactly once.
        local = bytearray(original)
        local[5], local[6] = ids.SYSTEM_BOAT, ids.COMPONENT_PI
        self._refresh_crc(local)
        priority = 0 if is_stop else 2
        queued = self._queue(bytes(local), self.config.motor_address, priority, request_key,
                             motion=True)
        if not queued:
            self._requests.pop(request_key, None)
            self._request_routes.pop(request_key, None)
            self._highest_request.pop(high_key, None)
            return False
        motor_boot = self.node_boot_ids.get((ids.SYSTEM_BOAT, self.config.motor_component), 0)
        self._make_result(request_key, motor_boot, 0, 0, 1, "request received; motor processing is pending")
        self._event("routed", "ground request queued for motor", bytes(local), (ids.SYSTEM_BOAT, ids.COMPONENT_PI))
        return True

    def _has_active_motion(self, identity: tuple[int, int], session: int) -> bool:
        prefix = (*identity, session)
        if any(item.key and item.key[:3] == prefix and item.motion
               for item in (*self._normal, *self._stops)):
            return True
        return any(key[:3] == prefix and record.get("opcode") in (1, 2, 3) and
                   int(record.get("state", 0)) not in TERMINAL_STATES
                   for key, record in self._requests.items())

    def _reject_request(self, fields: dict[str, int], route: str, *, result: int,
                        state: int, reason: str) -> None:
        key = (ids.SYSTEM_GROUND, ids.COMPONENT_GROUND, fields["ground_session"], fields["request_id"])
        if key in self._requests:
            self._event("duplicate", "rejected frame reused a correlated request ID; saved result retained")
            self._send_saved_result(key, route)
            return
        self._prune_ledgers()
        if len(self._requests) < max(16, self.config.queue_limit * 4):
            self._remember_request(key, fields, route)
        boot = self.node_boot_ids.get((ids.SYSTEM_BOAT, self.config.motor_component), 0)
        self._make_result(key, boot, 0, result, state, reason, route=route)

    def _remember_request(self, key, fields, route):
        self._requests[key] = dict(fields)
        self._request_routes[key] = route

    def _prune_ledgers(self) -> None:
        limit = max(16, self.config.queue_limit * 4)
        if len(self._requests) < limit:
            return
        queued = {item.key for item in (*self._normal, *self._stops) if item.key is not None}
        for key, record in list(self._requests.items()):
            if int(record.get("state", 0)) in TERMINAL_STATES and key not in queued:
                self._requests.pop(key, None)
                self._request_routes.pop(key, None)
                if len(self._requests) < limit:
                    return

    def _make_result(self, key: tuple[int, int, int, int], boot: int, command: int,
                     result: int, state: int, reason: str, *, enqueue: bool = True,
                     route: str | None = None) -> bytes:
        frame = codec.encode("REQUEST_RESULT", {
            "ground_session": key[2], "request_id": key[3], "motor_boot_id": boot,
            "local_command_id": command, "result": result, "state": state,
            "target_system": key[0], "target_component": key[1]},
            system=ids.SYSTEM_BOAT, component=ids.COMPONENT_PI, sequence=self._next_sequence())
        record = self._requests.get(key)
        if record is not None:
            record.update(result=result, state=state, motor_boot_id=boot,
                          local_command_id=command, result_frame=frame)
        if enqueue:
            self._route_result(key, frame, route=route)
        elif route == "ssh" or (route is None and self._request_routes.get(key) == "ssh"):
            self.on_frame(frame)
        self._event("request_result", reason, frame, (ids.SYSTEM_BOAT, ids.COMPONENT_PI))
        return frame

    def _route_result(self, key: tuple[int, int, int, int], frame: bytes,
                      *, route: str | None = None) -> None:
        if route == "comms" or (route is None and self._request_routes.get(key) == "comms"):
            self._queue(frame, self.config.comms_address, 1, key)
        else:
            self.on_frame(frame)

    def _send_saved_result(self, key: tuple[int, int, int, int], ingress: str) -> None:
        record = self._requests[key]
        frame = record.get("result_frame")
        if isinstance(frame, bytes):
            if ingress == "comms":
                self._queue(frame, self.config.comms_address, 1, key)
            else:
                self.on_frame(frame)

    def _queue(self, frame: bytes, address: int, priority: int,
               key: tuple[int, int, int, int] | None = None, *,
               motion: bool = False) -> bool:
        try:
            codec.decode(frame)
        except ValueError as exc:
            self._event("rejected", f"internal invalid frame: {exc}", frame)
            return False
        if len(frame) > ids.MAILBOX_FRAME:
            self._event("rejected", "outgoing frame exceeds 64-byte mailbox", frame)
            return False
        entry = _Write(address, bytes(frame), priority, key, self.clock(), motion)
        if priority == 0:
            stop_message = codec.decode(frame)
            stop_actuator = stop_message["fields"].get("actuator")
            for index in range(len(self._normal) - 1, -1, -1):
                pending = self._normal[index]
                pending_record = self._requests.get(pending.key, {}) if pending.key else {}
                if (pending.address == address and pending.motion and
                        pending_record.get("opcode") in (1, 2) and
                        pending_record.get("actuator") == stop_actuator):
                    del self._normal[index]
                    self._discard_queued(pending, "unsent movement cancelled by STOP")
            if len(self._stops) >= self._stop_limit:
                replaced = self._stops.popleft()
                if replaced.key is not None:
                    self._make_result(replaced.key,
                                      self.node_boot_ids.get((ids.SYSTEM_BOAT, self.config.motor_component), 0),
                                      0, 5, 6, "queued STOP superseded by a newer STOP", enqueue=False)
            if len(self._stops) >= self._stop_limit:
                self.queue_drops += 1
                self._event("queue_drop", "STOP queue is full of protected traffic", frame)
                return False
            self._stops.append(entry)
            return True
        else:
            if frame[0] == codec.MAGIC:
                try:
                    message_name = codec.decode(frame)["name"]
                except ValueError:
                    message_name = ""
                if message_name in ("HEARTBEAT", "NODE_HEALTH", "SCALED_IMU",
                                    "ACTUATOR_STATUS", "LINK_STATUS"):
                    for index, pending in enumerate(self._normal):
                        if pending.address == address:
                            try:
                                if self._queue_identity(pending.frame) == self._queue_identity(frame):
                                    self._normal[index] = entry
                                    return True
                            except ValueError:
                                pass
            if len(self._normal) >= self.config.queue_limit:
                if priority < 2:
                    index = next((i for i in range(len(self._normal) - 1, -1, -1)
                                  if self._normal[i].priority >= 2), None)
                    if index is not None:
                        dropped = self._normal[index]
                        del self._normal[index]
                        self._discard_queued(dropped,
                                             "routine frame removed for protected status or response")
                    else:
                        # Replace an older snapshot for the same message, or
                        # the previous state of the same correlated request.
                        replace = next((i for i, pending in enumerate(self._normal)
                                        if pending.address == address and
                                        (pending.key == key if key is not None else
                                         self._queue_identity(pending.frame) == self._queue_identity(frame))), None)
                        if replace is not None:
                            del self._normal[replace]
                        else:
                            self.queue_drops += 1
                            self._event("queue_drop", "protected queue full; no matching snapshot to coalesce", frame)
                            return False
                else:
                    self.queue_drops += 1
                    self._event("queue_drop", "routine frame evicted or rejected because bounded write queue is full", frame)
                    return False
            index = next((i for i, pending in enumerate(self._normal)
                          if pending.priority > priority), len(self._normal))
            self._normal.insert(index, entry)
        return True

    @staticmethod
    def _queue_identity(frame: bytes) -> tuple:
        message = codec.decode(frame)
        name = message["name"]
        fields = message["fields"]
        suffix = ()
        if name == "ACTUATOR_STATUS":
            suffix = (fields["actuator"],)
        elif name == "LINK_STATUS":
            suffix = (fields["link_type"],)
        elif name == "REQUEST_RESULT":
            suffix = (fields["ground_session"], fields["request_id"])
        return name, message["system"], message["component"], *suffix

    def _discard_queued(self, dropped: _Write, reason: str) -> None:
        if dropped.key is not None and dropped.motion:
            self._make_result(dropped.key,
                              self.node_boot_ids.get((ids.SYSTEM_BOAT, self.config.motor_component), 0),
                              0, 4, 6, reason, enqueue=False)
        else:
            self.queue_drops += 1
            self._event("queue_drop", reason, dropped.frame)

    def _write(self, write: _Write) -> bool:
        if write.key is not None and write.motion:
            record = self._requests.get(write.key, {})
            if record.get("opcode") != 3:
                now = self.clock()
                health = self.node_health.get((ids.SYSTEM_BOAT, self.config.motor_component))
                actuator_status = self.motor_status.get(int(record.get("actuator", 0)))
                status_at = self.motor_status_at.get(int(record.get("actuator", 0)))
                if (now - write.queued_at > 5.0 or health is None or now - health[0] > 5.0 or
                        health[3] != 0 or actuator_status is None or status_at is None or
                        now - status_at > 5.0 or actuator_status["fault_flags"] != 0 or
                        actuator_status["result"] == 1 or actuator_status["control_source"] != 3 or
                        (record.get("opcode") == 2 and not health[2])):
                    boot = self.node_boot_ids.get((ids.SYSTEM_BOAT, self.config.motor_component), 0)
                    self._make_result(write.key, boot, 0, 4, 0,
                                      "movement readiness or queue freshness expired before write",
                                      enqueue=False)
                    self._event("stale", "queued movement removed before bus write", write.frame)
                    self._consecutive_writes += 1
                    return False
        mailbox = write.frame.ljust(ids.MAILBOX_FRAME, b"\0")
        try:
            self.bus.write_mailbox(write.address, mailbox)
        except OSError as exc:
            self.bus_errors += 1
            self.node_present[self._node_for_address(write.address)] = False
            self._event("bus_error", f"ambiguous write at 0x{write.address:02x}: {exc}; not retried", write.frame)
            if write.key is not None and write.motion:
                boot = self.node_boot_ids.get((ids.SYSTEM_BOAT, self.config.motor_component), 0)
                self._make_result(write.key, boot, 0, 0, 0,
                                  "ambiguous motor write; outcome unknown, inspect status before motion")
                if write.address == self.config.motor_address:
                    self.motor_reconcile_required = True
            self._consecutive_writes += 1
            return False
        self._consecutive_writes += 1
        if write.key is not None and write.motion:
            record = self._requests.get(write.key)
            if record is not None:
                record["write_complete"] = 1
        self._event("bus_write", f"wrote {len(write.frame)} frame bytes to 0x{write.address:02x}", write.frame)
        return True

    def _poll_node(self, address: int) -> bool:
        node = self._node_for_address(address)
        try:
            mailbox = self.bus.read_mailbox(address)
            if not isinstance(mailbox, (bytes, bytearray)):
                raise OSError("mailbox read did not return bytes")
            if len(mailbox) != ids.MAILBOX_FRAME:
                raise OSError(f"mailbox read length {len(mailbox)} != 64")
        except (OSError, TypeError, ValueError) as exc:
            self.bus_errors += 1
            self.node_present[node] = False
            self._event("bus_error", f"read failed at 0x{address:02x}: {exc}")
            self._consecutive_writes = 0
            return False
        self.node_ack_at[node] = self.clock()
        self._consecutive_writes = 0
        if not any(mailbox):
            self._event("poll", f"empty mailbox from {node[0]}/{node[1]}")
            return True
        try:
            frame = self._decode_mailbox(mailbox)
            message = codec.decode(frame)
        except (ValueError, TypeError, struct.error) as exc:
            self.bus_errors += 1
            self._event("rejected", f"invalid mailbox from {node[0]}/{node[1]}: {exc}")
            return False
        expected = (ids.SYSTEM_BOAT, node[1])
        ground_request = node[1] == ids.COMPONENT_COMMS_SENSOR and (message["system"], message["component"]) == (ids.SYSTEM_GROUND, ids.COMPONENT_GROUND)
        if not ground_request and (message["system"], message["component"]) != expected:
            self._event("rejected", f"source does not match mailbox peer {node[0]}/{node[1]}", frame)
            return False
        if ground_request:
            self.submit_frame(frame, ingress="comms")
            return True
        self.node_last_seen[node] = self.clock()
        self.node_present[node] = True
        self._observe_node_message(message, frame)
        return True

    def _observe_node_message(self, message: dict, frame: bytes) -> None:
        source = (message["system"], message["component"])
        if message["name"] not in ALLOWED_MESSAGES:
            self._event("rejected", "peer message is not allowlisted", frame, source)
            return
        now = self.clock()
        self.node_last_seen[source] = now
        name, fields = message["name"], message["fields"]
        if name == "NODE_HEALTH":
            prior = self.node_boot_ids.get(source)
            boot = fields["boot_id"]
            if prior is not None and prior != boot:
                self._unknown_pending_for_boot(source, "motor boot changed; prior request outcome unknown", prior)
                self.motor_reconcile_required = True
                self._motor_reference_seen = False
                self.motor_status.clear()
                self.motor_status_at.clear()
                active = self._sessions.pop((ids.SYSTEM_GROUND, ids.COMPONENT_GROUND), None)
                if active is not None:
                    self._retired_sessions.append((ids.SYSTEM_GROUND, ids.COMPONENT_GROUND, active))
                    self._event("session", "motor boot changed; ground session retired and must reconnect")
            self.node_boot_ids[source] = boot
            self.node_health[source] = (now, boot, fields["feedback_valid"], fields["fault_flags"])
        elif name == "ACTUATOR_STATUS" and source[1] == self.config.motor_component:
            actuator = fields["actuator"]
            self.motor_status_at[actuator] = now
            self.motor_status[actuator] = dict(fields)
            self._motor_reference_seen = True
            health = self.node_health.get(source)
            if health is not None and now - health[0] <= 5.0:
                self.motor_reconcile_required = False
        elif name == "REQUEST_RESULT":
            if source != (ids.SYSTEM_BOAT, self.config.motor_component):
                self._event("rejected", "REQUEST_RESULT source is not the configured motor", frame, source)
                return
            request_key = (fields["target_system"], fields["target_component"],
                           fields["ground_session"], fields["request_id"])
            previous_boot = self.node_boot_ids.get(source)
            if ((fields["target_system"], fields["target_component"]) !=
                    (ids.SYSTEM_GROUND, ids.COMPONENT_GROUND) or
                    (previous_boot is not None and fields["motor_boot_id"] != previous_boot)):
                self._event("rejected", "motor result target or boot does not match current request context", frame, source)
                return
            if request_key in self._requests:
                record = self._requests[request_key]
                record.update(result=fields["result"], state=fields["state"],
                              motor_boot_id=fields["motor_boot_id"],
                              local_command_id=fields["local_command_id"], result_frame=frame)
                self._route_result(request_key, frame)
            else:
                # A Pi restart may have lost its in-memory ledger; return the motor's
                # correlated observation so the ground client can match its session.
                if self._request_routes.get(request_key) == "comms":
                    self._queue(frame, self.config.comms_address, 1)
                else:
                    self.on_frame(frame)
            self.snapshots.setdefault(source, {})[name] = bytes(frame)
            self._event("frame", "forwarded REQUEST_RESULT from motor", frame, source)
            return
        self.snapshots.setdefault(source, {})[name] = bytes(frame)
        self.on_frame(frame)
        if source[1] == self.config.motor_component and name != "REQUEST_RESULT":
            self._queue(frame, self.config.comms_address, 1)
        self._event("frame", f"forwarded {name} from {source[0]}/{source[1]}", frame, source)

    def _unknown_pending_for_boot(self, source: tuple[int, int], reason: str, old_boot: int) -> None:
        if source[1] != ids.COMPONENT_MOTOR:
            return
        kept = deque()
        for item in self._normal:
            if item.address == self.config.motor_address and item.key is not None:
                record = self._requests.get(item.key, {})
                if record.get("opcode") in (1, 2, 3):
                    continue
            kept.append(item)
        self._normal = kept
        self._stops = deque(item for item in self._stops
                            if item.address != self.config.motor_address or
                            (item.key is not None and self._requests.get(item.key, {}).get("opcode") == 3))
        for key, record in list(self._requests.items()):
            if int(record.get("state", 0)) not in TERMINAL_STATES:
                self._make_result(key, old_boot,
                                  int(record.get("local_command_id", 0)), 0, 0, reason)

    def _poll_imu(self) -> bool:
        cfg = self.config.imu
        assert cfg is not None
        self._last_imu_poll = self.clock()
        try:
            raw = self.bus.read_registers(cfg.address, cfg.register, cfg.sample_size)
            if len(raw) != cfg.sample_size:
                raise OSError("short IMU register read")
            values = [int.from_bytes(raw[index:index + 2], cfg.byte_order, signed=cfg.signed)
                      for index in range(0, cfg.sample_size, 2)]
            accel = tuple(round(value * cfg.accel_scale_mg_per_count) for value in values[:3])
            gyro = tuple(round(value * cfg.gyro_scale_mrad_s_per_count) for value in values[3:])
            if any(not -32768 <= value <= 32767 for value in (*accel, *gyro)):
                raise ValueError("scaled IMU value exceeds SCALED_IMU int16 range")
            self.imu_last_sample = (*accel, *gyro)
            self.imu_last_seen = self.clock()
            self.imu_valid_fields = 3
            self.imu_error = None
            frame = codec.encode("SCALED_IMU", {
                "time_boot_ms": int((self.clock() - self._boot_at) * 1000) & 0xFFFFFFFF,
                "xacc": accel[0], "yacc": accel[1], "zacc": accel[2],
                "xgyro": gyro[0], "ygyro": gyro[1], "zgyro": gyro[2],
                "xmag": 0, "ymag": 0, "zmag": 0, "temperature": 0},
                system=ids.SYSTEM_BOAT, component=ids.COMPONENT_PI, sequence=self._next_sequence())
            self.on_frame(frame)
            self._queue(frame, self.config.comms_address, 1)
            self._event("imu_sample", "simulated IMU measurement" if getattr(self.bus, "simulated", False) else "IMU register sample", frame, (ids.SYSTEM_BOAT, ids.COMPONENT_PI))
            self._emit_health()
            return True
        except (OSError, ValueError, TypeError, OverflowError, struct.error) as exc:
            self.bus_errors += 1
            self.imu_last_sample = None
            self.imu_last_seen = None
            self.imu_valid_fields = 0
            self.imu_error = str(exc)
            self._event("imu_error", f"IMU invalid: {exc}")
            self._emit_health(force=True)
            return False

    def _emit_periodic(self) -> None:
        now = self.clock()
        if now - self._last_heartbeat >= 1.0:
            self._last_heartbeat = now
            frame = codec.encode("HEARTBEAT", {
                "type": 18, "autopilot": 0, "base_mode": 0,
                "custom_mode": 0, "system_status": 4},
                system=ids.SYSTEM_BOAT, component=ids.COMPONENT_PI, sequence=self._next_sequence())
            self.node_present[(ids.SYSTEM_BOAT, ids.COMPONENT_PI)] = True
            self.node_last_seen[(ids.SYSTEM_BOAT, ids.COMPONENT_PI)] = now
            self.snapshots.setdefault((ids.SYSTEM_BOAT, ids.COMPONENT_PI), {})["HEARTBEAT"] = frame
            self.on_frame(frame)
            self._queue(frame, self.config.comms_address, 1)
        if now - self._last_health >= 1.0:
            self._emit_health()

    def _emit_health(self, force: bool = False) -> None:
        now = self.clock()
        if not force and now - self._last_health < 1.0:
            return
        self._last_health = now
        frame = codec.encode("NODE_HEALTH", {
            "boot_id": self.boot_id,
            "time_boot_ms": int((now - self._boot_at) * 1000) & 0xFFFFFFFF,
            "fault_flags": 2 if self.imu_error else 0,
            "sensor_valid_fields": self.imu_valid_fields,
            "queue_drops": min(self.queue_drops, 0xFFFF), "reset_reason": 0,
            "feedback_valid": 0}, system=ids.SYSTEM_BOAT, component=ids.COMPONENT_PI,
            sequence=self._next_sequence())
        self.node_present[(ids.SYSTEM_BOAT, ids.COMPONENT_PI)] = True
        self.node_last_seen[(ids.SYSTEM_BOAT, ids.COMPONENT_PI)] = now
        self.snapshots.setdefault((ids.SYSTEM_BOAT, ids.COMPONENT_PI), {})["NODE_HEALTH"] = frame
        self.on_frame(frame)
        self._queue(frame, self.config.comms_address, 1)
        self._event("health", "Pi IMU health", frame, (ids.SYSTEM_BOAT, ids.COMPONENT_PI))

    def _queue_ping_reply(self, message: dict, ingress: str) -> None:
        fields = message["fields"]
        reply = codec.encode("PING", {"time_usec": fields["time_usec"], "seq": fields["seq"],
                                       "target_system": message["system"],
                                       "target_component": message["component"]},
                             system=ids.SYSTEM_BOAT, component=ids.COMPONENT_PI,
                             sequence=self._next_sequence())
        self._route_upstream(reply, ingress)

    def _route_upstream(self, frame: bytes, ingress: str) -> None:
        if ingress == "comms":
            self._queue(frame, self.config.comms_address, 1)
        else:
            self.on_frame(frame)

    def _node_for_address(self, address: int) -> tuple[int, int]:
        component = self.config.motor_component if address == self.config.motor_address else self.config.comms_component
        return ids.SYSTEM_BOAT, component

    @staticmethod
    def _decode_mailbox(mailbox: bytes) -> bytes:
        if len(mailbox) != ids.MAILBOX_FRAME:
            raise ValueError("mailbox must be 64 bytes")
        if mailbox[0] != codec.MAGIC:
            raise ValueError("mailbox does not begin with MAVLink 2")
        if mailbox[2] & 1:
            raise ValueError("signed MAVLink frames are not accepted")
        size = 12 + mailbox[1]
        if size > ids.MAILBOX_FRAME:
            raise ValueError("frame exceeds mailbox")
        frame = mailbox[:size]
        if any(mailbox[size:]):
            raise ValueError("nonzero padding or concatenated frame")
        codec.decode(frame)
        return frame

    def _next_sequence(self) -> int:
        value = self._sequence & 0xFF
        self._sequence += 1
        return value

    @staticmethod
    def _refresh_crc(frame: bytearray) -> None:
        definition = codec.MESSAGES[int.from_bytes(frame[7:10], "little")]
        checksum = codec.crc(frame[1:10 + frame[1]] + bytes((definition["crc_extra"],)))
        frame[10 + frame[1]:12 + frame[1]] = struct.pack("<H", checksum)

    def _event(self, kind: str, detail: str, frame: bytes | None = None,
               source: tuple[int, int] | None = None) -> None:
        self.on_event(RouterEvent(kind, self.clock(), detail, frame, source))
