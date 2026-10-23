"""Deterministic driver around the production Pi router and compiled motor adapter."""
from __future__ import annotations

import ctypes
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Callable
from unittest.mock import patch

from boat.client import BoatClient
from boat.ids import COMPONENT_GROUND, COMPONENT_MOTOR, SYSTEM_GROUND
from onboard.config import IMURegisterConfig, ServiceConfig
from onboard.router import BoatRouter
from protocol import codec

ROOT = Path(__file__).resolve().parents[1]
MAILBOX = 64


def _load_motor():
    cc = shutil.which("cc")
    if not cc:
        raise RuntimeError("cc compiler is required for the compiled motor adapter scenario")
    tmp = tempfile.TemporaryDirectory(prefix="boat-scenarios-motor-")
    so = Path(tmp.name) / "libscenario_motor.so"
    subprocess.run([cc, "-std=c11", "-shared", "-fPIC", "-Wall", "-Wextra", "-Werror",
                    "-I" + str(ROOT / "protocol/gen/C"),
                    "-I" + str(ROOT / "protocol/gen/C/arsem_standalone"),
                    "-I" + str(ROOT / "motor-mcu"), str(ROOT / "motor-mcu/adapter.c"),
                    str(ROOT / "scenarios/motor_ctypes.c"), "-o", str(so)],
                   check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(so))
    lib.scenario_motor_create.argtypes = [ctypes.c_uint32]
    lib.scenario_motor_create.restype = ctypes.c_void_p
    lib.scenario_motor_destroy.argtypes = [ctypes.c_void_p]
    lib.scenario_motor_inputs.argtypes = [ctypes.c_void_p, ctypes.c_uint32,
        ctypes.c_int32, ctypes.c_int32, ctypes.c_int, ctypes.c_int, ctypes.c_int]
    lib.scenario_motor_receive.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8)]
    lib.scenario_motor_receive.restype = ctypes.c_int
    lib.scenario_motor_poll.argtypes = [ctypes.c_void_p]
    lib.scenario_motor_read.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8)]
    lib.scenario_motor_snapshot.argtypes = [ctypes.c_void_p, ctypes.c_int,
                                             ctypes.POINTER(ctypes.c_uint8)]
    for name in ("scenario_motor_active", "scenario_motor_enabled", "scenario_motor_direction",
                 "scenario_motor_target", "scenario_motor_measured", "scenario_motor_drives"):
        getattr(lib, name).argtypes = [ctypes.c_void_p, ctypes.c_int]
    lib.scenario_motor_active.restype = lib.scenario_motor_enabled.restype = ctypes.c_int
    lib.scenario_motor_direction.restype = lib.scenario_motor_target.restype = ctypes.c_int32
    lib.scenario_motor_measured.restype = ctypes.c_int32
    lib.scenario_motor_drives.restype = ctypes.c_uint
    lib.scenario_motor_boot.argtypes = [ctypes.c_void_p]
    lib.scenario_motor_boot.restype = ctypes.c_uint32
    lib.scenario_motor_rx_errors.argtypes = [ctypes.c_void_p]
    lib.scenario_motor_rx_errors.restype = ctypes.c_uint
    return tmp, lib


class ManualClock:
    def __init__(self, now: float = 10.0): self.now = now
    def __call__(self): return self.now
    def advance(self, ms: int): self.now += ms / 1000


class CompiledMotorBus:
    """I2CBus implementation whose motor endpoint is the actual compiled adapter."""
    def __init__(self, lib, motor, clock: ManualClock, *, imu: bytes | None = None):
        self.lib, self.motor, self.clock = lib, motor, clock
        self.imu = imu
        self.operations: list[dict] = []
        self.faults: dict[str, int] = {}
        self.closed = False
        self.counts = [0, 0]
        self.fresh = [True, True]
        self.rc = False
        self._update()
    def _update(self):
        self.lib.scenario_motor_inputs(self.motor, int(self.clock.now * 1000) & 0xffffffff,
            self.counts[0], self.counts[1], int(self.fresh[0]), int(self.fresh[1]), int(self.rc))
    def fail_next(self, op: str, count=1): self.faults[op] = self.faults.get(op, 0) + count
    def _fault(self, op, address):
        self.operations.append({"op": op, "address": address, "at": self.clock.now})
        if self.faults.get(op, 0):
            self.faults[op] -= 1
            raise OSError(f"injected {op} fault at 0x{address:02x}")
        if address not in (0x08, 0x09) and not (op == "imu" and self.imu is not None):
            raise OSError(f"no scenario device at 0x{address:02x}")
    def write_mailbox(self, address: int, data: bytes):
        if len(data) != MAILBOX: raise ValueError("mailbox write must be 64 bytes")
        self._fault("write", address); self.operations[-1]["hex"] = data.hex()
        if address == 0x08:
            self._update()
            buf = (ctypes.c_uint8 * MAILBOX).from_buffer_copy(data)
            if not self.lib.scenario_motor_receive(self.motor, buf):
                raise OSError("compiled motor adapter rejected mailbox frame")
    def read_mailbox(self, address: int) -> bytes:
        self._fault("read", address)
        if address == 0x09: return bytes(MAILBOX)
        self._update(); self.lib.scenario_motor_poll(self.motor)
        out = (ctypes.c_uint8 * MAILBOX)(); self.lib.scenario_motor_read(self.motor, out)
        result = bytes(out); self.operations[-1]["hex"] = result.hex(); return result
    def read_registers(self, address: int, register: int, length: int) -> bytes:
        self._fault("imu", address)
        if self.imu is None or len(self.imu) < length: raise OSError("IMU sample unavailable")
        self.operations[-1].update(register=register, length=length)
        return self.imu[:length]
    def close(self): self.closed = True


class RouterTransport:
    """Ground transport joining real BoatClient and BoatRouter APIs."""
    name = "scenario-router-transport"
    def __init__(self, router: BoatRouter, clock: ManualClock):
        self.router, self.clock, self.on_frame = router, clock, None
        self.sent: list[bytes] = []
        self.fail_send = False
    def open(self, on_frame: Callable[[bytes], None]): self.on_frame = on_frame
    def close(self): self.on_frame = None
    def send_frame(self, frame: bytes):
        self.sent.append(bytes(frame))
        if self.fail_send:
            self.fail_send = False
            raise OSError("injected ambiguous ground write")
        self.router.submit_frame(frame, ingress="ssh")
        self.pump(20)
    def pump(self, steps=1):
        for _ in range(steps):
            self.router.step()
            self.clock.advance(20)
    def emit(self, frame: bytes):
        if self.on_frame: self.on_frame(frame)


class IntegratedRig:
    def __init__(self, *, imu: bytes | None = None, boot_id=0x10203040):
        self.clock = ManualClock()
        self.tmp, self.lib = _load_motor()
        self.motor = self.lib.scenario_motor_create(boot_id)
        if not self.motor: raise MemoryError("could not allocate compiled motor adapter")
        self.bus = CompiledMotorBus(self.lib, self.motor, self.clock, imu=imu)
        self.frames: list[dict] = []
        self.router = BoatRouter(self.bus, ServiceConfig(
            imu=(IMURegisterConfig(address=0x68, register=0x10,
                accel_scale_mg_per_count=1, gyro_scale_mrad_s_per_count=1,
                interval_ms=20) if imu is not None else None)),
            clock=self.clock, on_frame=self._ground_frame)
        # Make generated IDs deterministic while retaining the production generators.
        self.router.boot_id = 0xA0B0C0D0
        self.transport = RouterTransport(self.router, self.clock)
        self.client = BoatClient(self.transport, clock=self.clock)
        self.events: list[object] = []
        self.client.subscribe(self.events.append)
    def _ground_frame(self, frame):
        try: msg = codec.decode(frame)
        except ValueError: msg = {"name": "INVALID"}
        self.frames.append({"at": self.clock.now, "direction": "router_to_ground",
                            "name": msg["name"], "hex": frame.hex()})
        self.transport.emit(frame)
    def connect_and_reconcile(self):
        with patch("boat.client.secrets.randbits", return_value=0x50607080):
            self.client.connect()
        # Drive the actual fixed-width mailbox poller until adapter snapshots and
        # the adapter's periodic health event have reached the ground client.
        for _ in range(60):
            self.transport.pump(1)
            if self.clock.now >= 11.1 and self.client._reconciled: break
        if not self.client._reconciled:
            raise AssertionError("actual router/client did not reconcile motor health and status")
    def step_encoder(self, actuator, count):
        self.bus.counts[actuator - 1] = count
        self.bus._update()
    def cleanup(self):
        try: self.router.close()
        finally:
            self.lib.scenario_motor_destroy(self.motor)
            self.tmp.cleanup()


def run_actual_motor_route():
    rig = IntegratedRig()
    try:
        rig.connect_and_reconcile()
        ticket = rig.client.set_absolute_angle(1, 900, timeout_ms=800, max_rate_cdeg_s=1000)
        slot = 0
        assert rig.lib.scenario_motor_active(rig.motor, slot) == 1
        assert rig.lib.scenario_motor_enabled(rig.motor, slot) == 1
        assert rig.lib.scenario_motor_direction(rig.motor, slot) == 1
        assert rig.lib.scenario_motor_target(rig.motor, slot) == 900
        assert rig.bus.counts[0] == 0, "fixture must not copy the requested target into feedback"
        # The fixture injects a separately chosen encoder count; completion is
        # based on the adapter's measured conversion and actual output state.
        rig.step_encoder(1, 900)
        rig.transport.pump(8)
        assert rig.lib.scenario_motor_active(rig.motor, slot) == 0
        assert rig.lib.scenario_motor_enabled(rig.motor, slot) == 0
        snapshot = (ctypes.c_uint8 * MAILBOX)()
        rig.lib.scenario_motor_snapshot(rig.motor, slot, snapshot)
        raw_status = bytes(snapshot)
        status = codec.decode(raw_status[:12 + raw_status[1]])
        assert status["name"] == "ACTUATOR_STATUS"
        assert status["fields"]["measured_position_cdeg"] == 900
        result_frames = [f for f in rig.frames if f["name"] == "REQUEST_RESULT"]
        assert result_frames and any(codec.decode(bytes.fromhex(f["hex"]))["fields"]["request_id"] == ticket.request_id
                                     for f in result_frames)
        return {"at": rig.clock.now, "client_request_id": ticket.request_id,
            "ground_session": ticket.ground_session, "router_boot_id": rig.router.boot_id,
            "motor_boot_id": rig.lib.scenario_motor_boot(rig.motor),
            "encoder_injected_count": rig.bus.counts[0],
            "motor_target_cdeg": rig.lib.scenario_motor_target(rig.motor, slot),
            "motor_status_hex": bytes(snapshot).hex(),
            "motor_output_enabled_after_completion": rig.lib.scenario_motor_enabled(rig.motor, slot),
            "motor_rx_errors": rig.lib.scenario_motor_rx_errors(rig.motor),
            "mailbox_operations": rig.bus.operations[-16:], "frames": rig.frames[-24:]}
    finally: rig.cleanup()


def run_pi_owned_imu():
    import struct
    raw = struct.pack("<6h", 10, -20, 1000, 4, 5, 6)
    rig = IntegratedRig(imu=raw)
    try:
        rig.client.connect()
        rig.transport.pump(60)
        imu_frames = [f for f in rig.frames if f["name"] == "SCALED_IMU"]
        assert imu_frames
        sample = codec.decode(bytes.fromhex(imu_frames[-1]["hex"]))["fields"]
        assert tuple(sample[k] for k in ("xacc", "yacc", "zacc")) == (10, -20, 1000)
        assert tuple(sample[k] for k in ("xgyro", "ygyro", "zgyro")) == (4, 5, 6)
        pi_health = [f for f in rig.frames if f["name"] == "NODE_HEALTH" and
                     codec.decode(bytes.fromhex(f["hex"]))["component"] == 1]
        assert pi_health and codec.decode(bytes.fromhex(pi_health[-1]["hex"]))["fields"]["sensor_valid_fields"] == 3
        assert any(o["op"] == "imu" and o["address"] == 0x68 and o["register"] == 0x10
                   for o in rig.bus.operations)
        assert not any(o["op"] == "imu" and o["address"] in (0x08, 0x09) for o in rig.bus.operations)
        return {"imu_register_reads": [o for o in rig.bus.operations if o["op"] == "imu"],
                "scaled_imu_frame": imu_frames[-1], "pi_health_frame": pi_health[-1],
                "valid_fields": 3, "source": [1, 1]}
    finally: rig.cleanup()


def run_motor_ping():
    rig = IntegratedRig()
    try:
        rig.client.connect()
        rig.transport.pump(20)
        rig.client.ping_all()
        rig.transport.pump(30)
        pongs = [e for e in rig.events if type(e).__name__ == "PongReply" and
                 (e.system, e.component) == (1, COMPONENT_MOTOR)]
        assert len(pongs) == 1, f"expected one motor reply for one ping, found {len(pongs)}"
        return {"motor_pong": {"system": pongs[0].system, "component": pongs[0].component,
                "seq": pongs[0].seq, "time_usec": pongs[0].time_usec},
                "frames": rig.frames[-12:]}
    finally: rig.cleanup()


def run_motor_guards():
    from boat.ids import COMPONENT_PI
    rig = IntegratedRig()
    try:
        rig.connect_and_reconcile()
        fields = {"request_id": 10, "ground_session": 0x50607080,
            "delta_cdeg": 900, "timeout_ms": 1000, "max_rate_cdeg_s": 1000,
            "target_system": 1, "target_component": COMPONENT_MOTOR,
            "actuator": 1, "opcode": 1, "control_source": 3}
        checks = []
        for label, kwargs in (("wrong_source", {"system": 9, "component": COMPONENT_PI}),
                ("wrong_target", {"fields": {**fields, "target_component": 153}}),
                ("wrong_authority", {"fields": {**fields, "control_source": 2}})):
            options = dict(system=1, component=COMPONENT_PI, sequence=25)
            selected = dict(fields)
            if "fields" in kwargs: selected = kwargs["fields"]
            else: options.update(kwargs)
            frame = codec.encode("ACTUATOR_REQUEST", selected, **options)
            mailbox = frame.ljust(MAILBOX, b"\0")
            data = (ctypes.c_uint8 * MAILBOX).from_buffer_copy(mailbox)
            accepted = rig.lib.scenario_motor_receive(rig.motor, data)
            checks.append({"case": label, "adapter_consumed": bool(accepted), "frame": frame.hex()})
            assert rig.lib.scenario_motor_enabled(rig.motor, 0) == 0
        valid = codec.encode("ACTUATOR_REQUEST", {**fields, "request_id": 11,
            "delta_cdeg": 18100}, system=1, component=COMPONENT_PI, sequence=26)
        bad = bytearray(valid); bad[-1] ^= 0x40
        damaged = (ctypes.c_uint8 * MAILBOX).from_buffer_copy(bytes(bad).ljust(MAILBOX, b"\0"))
        assert not rig.lib.scenario_motor_receive(rig.motor, damaged)
        checks.append({"case": "bad_crc", "adapter_consumed": False, "frame": bytes(bad).hex()})
        over_limit = (ctypes.c_uint8 * MAILBOX).from_buffer_copy(valid.ljust(MAILBOX, b"\0"))
        assert rig.lib.scenario_motor_receive(rig.motor, over_limit)
        assert rig.lib.scenario_motor_active(rig.motor, 0) == 0
        assert rig.lib.scenario_motor_enabled(rig.motor, 0) == 0
        checks.append({"case": "hard_travel_limit", "adapter_active": False})
        # The Pi also rejects a non-ground command source before bus forwarding.
        wrong_ground = codec.encode("ACTUATOR_REQUEST", fields, system=1, component=COMPONENT_PI)
        before = len([x for x in rig.bus.operations if x["op"] == "write"])
        assert not rig.router.submit_frame(wrong_ground, ingress="ssh")
        after = len([x for x in rig.bus.operations if x["op"] == "write"])
        assert before == after
        return {"direct_adapter_guards": checks, "router_wrong_ground_source_rejected": True,
                "motor_enabled": rig.lib.scenario_motor_enabled(rig.motor, 0),
                "motor_rx_errors": rig.lib.scenario_motor_rx_errors(rig.motor)}
    finally: rig.cleanup()


def run_ambiguous_write():
    rig = IntegratedRig()
    try:
        rig.connect_and_reconcile()
        start = len(rig.transport.sent)
        old_session = rig.client.ground_session
        rig.transport.fail_send = True
        try:
            rig.client.step(1, 500)
        except OSError:
            pass
        else:
            raise AssertionError("ambiguous send did not raise")
        outcomes = [e for e in rig.events if type(e).__name__ == "RequestOutcome"]
        unknown = [e for e in outcomes if e.result == 0 and e.state == 0]
        assert len(unknown) == 1
        attempted = len(rig.transport.sent)
        with patch("boat.client.secrets.randbits", side_effect=(0x60607080, 0x60607080)):
            rig.client.connect()
        rig.transport.pump(20)
        assert len(rig.transport.sent) == attempted == start + 1
        assert not any(rig.lib.scenario_motor_active(rig.motor, i) for i in (0, 1))
        return {"old_session": old_session, "unknown_request_id": unknown[0].request_id,
                "attempted_frames": [x.hex() for x in rig.transport.sent[start:]],
                "frames_after_reconnect": len(rig.transport.sent) - attempted,
                "motor_active": [rig.lib.scenario_motor_active(rig.motor, i) for i in (0, 1)]}
    finally: rig.cleanup()


def run_step_and_duplicate():
    rig = IntegratedRig()
    try:
        rig.bus.counts[0] = 400
        rig.connect_and_reconcile()
        ticket = rig.client.step(1, 500, timeout_ms=2000, max_rate_cdeg_s=1000)
        assert rig.lib.scenario_motor_target(rig.motor, 0) == 900
        assert rig.lib.scenario_motor_enabled(rig.motor, 0) == 1
        pi_write = next(op for op in reversed(rig.bus.operations)
                        if op["op"] == "write" and op["address"] == 0x08)
        writes_before = len([op for op in rig.bus.operations if op["op"] == "write"])
        rig.bus.write_mailbox(0x08, bytes.fromhex(pi_write["hex"]))
        writes_after = len([op for op in rig.bus.operations if op["op"] == "write"])
        assert writes_after == writes_before + 1
        assert rig.lib.scenario_motor_target(rig.motor, 0) == 900
        # Same local action remains active; duplicate input did not call drive again.
        assert rig.lib.scenario_motor_active(rig.motor, 0) == 1
        assert ticket.request_id == 1
        return {"request_id": ticket.request_id, "initial_encoder_count": 400,
                "step_cdeg": 500, "resolved_target_cdeg": 900,
                "duplicate_frame": pi_write["hex"], "target_after_duplicate": 900,
                "motor_active_after_duplicate": True}
    finally: rig.cleanup()


def run_deadline_and_stop_priority():
    from boat.ids import COMPONENT_GROUND, COMPONENT_MOTOR, SYSTEM_GROUND
    # Queue arbitration is exercised on the actual Pi router with writes observed
    # at its I2CBus boundary; motor timeout is exercised by the compiled adapter.
    rig = IntegratedRig()
    try:
        rig.connect_and_reconcile()
        base = {"ground_session": 0x50607080, "timeout_ms": 1500,
                "max_rate_cdeg_s": 1000, "target_system": 1,
                "target_component": COMPONENT_MOTOR, "actuator": 1,
                "control_source": 3}
        move = codec.encode("ACTUATOR_REQUEST", {**base, "request_id": 40,
            "delta_cdeg": 900, "opcode": 1}, system=SYSTEM_GROUND,
            component=COMPONENT_GROUND, sequence=40)
        stop = codec.encode("ACTUATOR_REQUEST", {**base, "request_id": 41,
            "delta_cdeg": 0, "opcode": 3}, system=SYSTEM_GROUND,
            component=COMPONENT_GROUND, sequence=41)
        assert rig.router.submit_frame(move)
        assert rig.router.submit_frame(stop)
        before = len(rig.bus.operations)
        rig.router.step()
        writes = [op for op in rig.bus.operations[before:] if op["op"] == "write"]
        assert writes
        first = codec.decode(bytes.fromhex(writes[0]["hex"])[:12 + bytes.fromhex(writes[0]["hex"])[1]])
        assert first["name"] == "ACTUATOR_REQUEST" and first["fields"]["opcode"] == 3
        assert rig.lib.scenario_motor_enabled(rig.motor, 0) == 0
        return {"queued_move": move.hex(), "queued_stop": stop.hex(),
                "first_bus_write": writes[0]["hex"], "first_write_opcode": first["fields"]["opcode"],
                "motor_output_enabled": rig.lib.scenario_motor_enabled(rig.motor, 0),
                "note": "STOP priority passed; local timeout is separately exercised by motor host harness."}
    finally: rig.cleanup()


def run_motor_local_deadline():
    rig = IntegratedRig()
    try:
        rig.connect_and_reconcile()
        ticket = rig.client.set_absolute_angle(1, 900, timeout_ms=1200,
                                               max_rate_cdeg_s=1000)
        assert rig.lib.scenario_motor_active(rig.motor, 0) == 1
        assert rig.lib.scenario_motor_enabled(rig.motor, 0) == 1
        rig.clock.advance(1201)
        rig.bus._update()
        rig.lib.scenario_motor_poll(rig.motor)
        assert rig.lib.scenario_motor_active(rig.motor, 0) == 0
        assert rig.lib.scenario_motor_enabled(rig.motor, 0) == 0
        snap = (ctypes.c_uint8 * MAILBOX)()
        rig.lib.scenario_motor_snapshot(rig.motor, 0, snap)
        raw = bytes(snap); status = codec.decode(raw[:12 + raw[1]])
        assert status["name"] == "ACTUATOR_STATUS"
        assert status["fields"]["fault_flags"] & 4
        assert not any(codec.decode(x)["name"] == "ACTUATOR_REQUEST" and
                       codec.decode(x)["fields"]["opcode"] == 3
                       for x in rig.transport.sent)
        return {"request_id": ticket.request_id, "local_deadline_ms": 1200,
                "elapsed_ms": 1201, "status_hex": raw.hex(),
                "fault_flags": status["fields"]["fault_flags"],
                "remote_stop_request_count": 0,
                "output_enabled": rig.lib.scenario_motor_enabled(rig.motor, 0)}
    finally: rig.cleanup()


def run_rc_yield():
    rig = IntegratedRig()
    try:
        rig.connect_and_reconcile()
        rig.client.set_absolute_angle(1, 900, timeout_ms=2000, max_rate_cdeg_s=1000)
        assert rig.lib.scenario_motor_enabled(rig.motor, 0) == 1
        assert rig.lib.scenario_motor_active(rig.motor, 0) == 1
        rig.bus.rc = True
        rig.bus._update()
        rig.lib.scenario_motor_poll(rig.motor)
        assert rig.lib.scenario_motor_enabled(rig.motor, 0) == 0
        assert rig.lib.scenario_motor_active(rig.motor, 0) == 0
        return {"before_rc": {"active": True, "output_enabled": True},
                "after_rc": {"active": False, "output_enabled": False},
                "physical_rc": False}
    finally: rig.cleanup()
