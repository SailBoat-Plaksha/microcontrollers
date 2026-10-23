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
        self._poll_addresses = [self.config.motor_address, self.config.comms_address]
        if self.config.imu:
            self._poll_addresses.append(self.config.imu.address)
        self._poll_index = 0
        self._consecutive_writes = 0
        self._sessions: dict[tuple[int, int], int] = {}
        self._highest_request: dict[tuple[int, int, int], int] = {}
        self._requests: dict[tuple[int, int, int, int], dict[str, int | str]] = {}
        self._request_routes: dict[tuple[int, int, int, int], str] = {}
        self.snapshots: dict[tuple[int, int], dict[str, bytes]] = {}
        self.node_last_seen: dict[tuple[int, int], float] = {}
        self.node_present: dict[tuple[int, int], bool] = {}
        self.node_boot_ids: dict[tuple[int, int], int] = {}
        self.node_health: dict[tuple[int, int], tuple[float, int, int]] = {}
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
            target = (fields["target_system"], fields["target_component"])
            if target == (ids.SYSTEM_BOAT, ids.COMPONENT_PI):
                self._queue_ping_reply(message, ingress)
            elif target == (ids.SYSTEM_BOAT, self.config.motor_component):
                self._queue(frame, self.config.motor_address, 1)
            elif target == (ids.SYSTEM_BOAT, self.config.comms_component) and ingress == "ssh":
                self._queue(frame, self.config.comms_address, 1)
            else:
                self._event("rejected", "PING target is not a configured node", frame)
                return False
            return True
        self._event("rejected", "ground message is not a command", frame)
        return False

    def step(self) -> bool:
        """Perform at most one bus operation; safe to drive with an injected clock."""
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
            if health is None or self.clock() - health[0] > 5.0:
                self._reject_request(fields, ingress, result=4, state=0,
                                     reason="fresh motor health is required before movement")
                return False
            if fields["opcode"] == 2 and not health[2]:
                self._reject_request(fields, ingress, result=3, state=6,
                                     reason="motor feedback is not marked valid; STEP not forwarded")
                return False
        if request_key in self._requests:
            self._event("duplicate", "duplicate request suppressed", original)
            self._send_saved_result(request_key, ingress)
            return True
        current_session = self._sessions.get(key[:2])
        if current_session is not None and current_session != fields["ground_session"]:
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
                        self._make_result(old_key, 0, 0, 4, 6,
                                          "session changed before bus write; request marked stale")
                    else:
                        self._make_result(old_key, 0, 0, 0, 0,
                                          "session changed after bus write; outcome unknown")
            self._event("session", "ground session changed; old requests marked unknown")
        self._sessions[key[:2]] = fields["ground_session"]
        high_key = (*key[:2], fields["ground_session"])
        previous = self._highest_request.get(high_key, 0)
        if fields["request_id"] <= previous:
            self._reject_request(fields, ingress, result=4, state=6, reason="reordered or stale request ID")
            return False
        if self.queued_writes >= self.config.queue_limit:
            self.queue_drops += 1
            self._reject_request(fields, ingress, result=0, state=0, reason="bounded write queue is full")
            return False
        self._highest_request[high_key] = fields["request_id"]
        self._requests[request_key] = dict(fields)
        self._request_routes[request_key] = ingress
        # Rewrite only the MAVLink source header. The STEP delta and correlation payload
        # reach the motor unchanged, where STEP is resolved exactly once.
        local = bytearray(original)
        local[5], local[6] = ids.SYSTEM_BOAT, ids.COMPONENT_PI
        self._refresh_crc(local)
        priority = 0 if fields["opcode"] == 3 else 2
        self._queue(bytes(local), self.config.motor_address, priority, request_key)
        motor_boot = self.node_boot_ids.get((ids.SYSTEM_BOAT, self.config.motor_component), 0)
        self._make_result(request_key, motor_boot, 0, 0, 1, "request received; motor processing is pending")
        self._event("routed", "ground request queued for motor", bytes(local), (ids.SYSTEM_BOAT, ids.COMPONENT_PI))
        return True

    def _reject_request(self, fields: dict[str, int], route: str, *, result: int,
                        state: int, reason: str) -> None:
        key = (ids.SYSTEM_GROUND, ids.COMPONENT_GROUND, fields["ground_session"], fields["request_id"])
        self._requests[key] = dict(fields)
        self._request_routes[key] = route
        self._make_result(key, 0, 0, result, state, reason)

    def _make_result(self, key: tuple[int, int, int, int], boot: int, command: int,
                     result: int, state: int, reason: str) -> bytes:
        frame = codec.encode("REQUEST_RESULT", {
            "ground_session": key[2], "request_id": key[3], "motor_boot_id": boot,
            "local_command_id": command, "result": result, "state": state,
            "target_system": key[0], "target_component": key[1]},
            system=ids.SYSTEM_BOAT, component=ids.COMPONENT_PI, sequence=self._next_sequence())
        record = self._requests.setdefault(key, {})
        record.update(result=result, state=state, motor_boot_id=boot,
                      local_command_id=command, result_frame=frame)
        self._route_result(key, frame)
        self._event("request_result", reason, frame, (ids.SYSTEM_BOAT, ids.COMPONENT_PI))
        return frame

    def _route_result(self, key: tuple[int, int, int, int], frame: bytes) -> None:
        if self._request_routes.get(key) == "comms":
            self._queue(frame, self.config.comms_address, 1)
        else:
            self.on_frame(frame)

    def _send_saved_result(self, key: tuple[int, int, int, int], ingress: str) -> None:
        record = self._requests[key]
        frame = record.get("result_frame")
        if isinstance(frame, bytes):
            if ingress == "comms":
                self._queue(frame, self.config.comms_address, 1)
            else:
                self.on_frame(frame)

    def _queue(self, frame: bytes, address: int, priority: int,
               key: tuple[int, int, int, int] | None = None) -> None:
        try:
            codec.decode(frame)
        except ValueError as exc:
            self._event("rejected", f"internal invalid frame: {exc}", frame)
            return
        if len(frame) > ids.MAILBOX_FRAME:
            self._event("rejected", "outgoing frame exceeds 64-byte mailbox", frame)
            return
        entry = _Write(address, bytes(frame), priority, key)
        if priority == 0:
            if len(self._stops) >= self.config.queue_limit:
                replaced = self._stops.popleft()
                if replaced.key is not None:
                    self._make_result(replaced.key, 0, 0, 5, 6,
                                      "queued STOP superseded by a newer STOP")
            while len(self._normal) + len(self._stops) >= self.config.queue_limit and self._normal:
                dropped = self._normal.popleft()
                if dropped.key is not None:
                    self._make_result(dropped.key, 0, 0, 4, 6,
                                      "queued movement removed to prioritize STOP")
                else:
                    self.queue_drops += 1
                    self._event("queue_drop", "routine frame removed to prioritize STOP", dropped.frame)
            self._stops.append(entry)
        else:
            if frame[0] == codec.MAGIC:
                try:
                    message_name = codec.decode(frame)["name"]
                except ValueError:
                    message_name = ""
                if message_name in ("HEARTBEAT", "NODE_HEALTH", "SCALED_IMU"):
                    for index, pending in enumerate(self._normal):
                        if pending.address == address:
                            try:
                                if codec.decode(pending.frame)["name"] == message_name:
                                    self._normal[index] = entry
                                    return
                            except ValueError:
                                pass
            if len(self._normal) + len(self._stops) >= self.config.queue_limit:
                self.queue_drops += 1
                self._event("queue_drop", "bounded write queue full", frame)
                return
            self._normal.append(entry)

    def _write(self, write: _Write) -> bool:
        mailbox = write.frame.ljust(ids.MAILBOX_FRAME, b"\0")
        try:
            self.bus.write_mailbox(write.address, mailbox)
        except OSError as exc:
            self.bus_errors += 1
            self.node_present[self._node_for_address(write.address)] = False
            self._event("bus_error", f"ambiguous write at 0x{write.address:02x}: {exc}; not retried", write.frame)
            if write.key is not None:
                self._make_result(write.key, 0, 0, 0, 0, "ambiguous motor write; outcome unknown, not retried")
            self._consecutive_writes += 1
            return False
        self._consecutive_writes += 1
        if write.key is not None:
            record = self._requests.get(write.key)
            if record is not None:
                record["write_complete"] = 1
        self._event("bus_write", f"wrote {len(write.frame)} frame bytes to 0x{write.address:02x}", write.frame)
        return True

    def _poll_node(self, address: int) -> bool:
        node = self._node_for_address(address)
        try:
            mailbox = self.bus.read_mailbox(address)
            if len(mailbox) != ids.MAILBOX_FRAME:
                raise OSError(f"mailbox read length {len(mailbox)} != 64")
        except OSError as exc:
            self.bus_errors += 1
            self.node_present[node] = False
            self._event("bus_error", f"read failed at 0x{address:02x}: {exc}")
            self._consecutive_writes = 0
            return False
        self.node_present[node] = True
        self.node_last_seen[node] = self.clock()
        self._consecutive_writes = 0
        if not any(mailbox):
            self._event("poll", f"empty mailbox from {node[0]}/{node[1]}")
            return True
        try:
            frame = self._decode_mailbox(mailbox)
            message = codec.decode(frame)
        except ValueError as exc:
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
                self._unknown_pending_for_boot(source, "motor boot changed; prior request outcome unknown")
            self.node_boot_ids[source] = boot
            self.node_health[source] = (now, boot, fields["feedback_valid"])
        elif name == "REQUEST_RESULT":
            request_key = (fields["target_system"], fields["target_component"],
                           fields["ground_session"], fields["request_id"])
            if request_key in self._requests:
                previous_boot = self.node_boot_ids.get(source)
                if previous_boot is not None and fields["motor_boot_id"] != previous_boot:
                    self._make_result(request_key, 0, 0, 0, 0, "motor boot changed; request outcome unknown")
                else:
                    record = self._requests[request_key]
                    record.update(result=fields["result"], state=fields["state"],
                                  motor_boot_id=fields["motor_boot_id"],
                                  local_command_id=fields["local_command_id"], result_frame=frame)
                    self._route_result(request_key, frame)
            elif (fields["target_system"], fields["target_component"]) == (ids.SYSTEM_GROUND, ids.COMPONENT_GROUND):
                # A Pi restart may have lost its in-memory ledger; return the motor's
                # correlated observation so the ground client can match its session.
                self._queue(frame, self.config.comms_address, 1)
        self.snapshots.setdefault(source, {})[name] = bytes(frame)
        self.on_frame(frame)
        if source[1] == self.config.motor_component and name != "REQUEST_RESULT":
            self._queue(frame, self.config.comms_address, 1)
        self._event("frame", f"forwarded {name} from {source[0]}/{source[1]}", frame, source)

    def _unknown_pending_for_boot(self, source: tuple[int, int], reason: str) -> None:
        if source[1] != ids.COMPONENT_MOTOR:
            return
        for key, record in list(self._requests.items()):
            if int(record.get("state", 0)) not in TERMINAL_STATES:
                self._make_result(key, 0, int(record.get("local_command_id", 0)), 0, 0, reason)

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
        except (OSError, ValueError) as exc:
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
