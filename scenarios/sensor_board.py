"""Production Pi/BoatClient scenarios with the compiled sensor Pico endpoint."""

from __future__ import annotations

import ctypes
import json
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path
from unittest.mock import patch

from boat.client import BoatClient, ImuReading
from boat.ids import COMPONENT_GROUND, COMPONENT_SENSOR, SYSTEM_GROUND
from onboard.config import ServiceConfig
from onboard.router import BoatRouter
from protocol import codec

ROOT = Path(__file__).resolve().parents[1]
MAILBOX = 64


class ManualClock:
    def __init__(self, now: float = 100.0): self.now = now
    def __call__(self): return self.now
    def advance(self, ms: int): self.now += ms / 1000


def _load_sensor():
    cc = shutil.which("cc")
    if not cc: raise RuntimeError("cc compiler required for compiled sensor scenario")
    tmp = tempfile.TemporaryDirectory(prefix="boat-scenarios-sensor-")
    so = Path(tmp.name) / "libscenario_sensor.so"
    subprocess.run([cc, "-std=c11", "-shared", "-fPIC", "-Wall", "-Wextra", "-Werror",
        "-I" + str(ROOT / "sensor-mcu/host/fakes"),
        "-I" + str(ROOT / "sensor-mcu/include"),
        "-I" + str(ROOT / "protocol"),
        "-I" + str(ROOT / "protocol/gen/C"),
        "-I" + str(ROOT / "protocol/gen/C/arsem_standalone"),
        str(ROOT / "sensor-mcu/sensor_node.c"),
        str(ROOT / "sensor-mcu/mailbox_transport.c"),
        str(ROOT / "sensor-mcu/host/fakes/i2c.c"),
        str(ROOT / "scenarios/sensor_node_ctypes.c"), "-o", str(so)],
        check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(so))
    lib.scenario_sensor_create.argtypes = [ctypes.c_uint32]
    lib.scenario_sensor_create.restype = ctypes.c_void_p
    lib.scenario_sensor_destroy.argtypes = [ctypes.c_void_p]
    lib.scenario_sensor_set_time.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
    lib.scenario_sensor_write.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8)]
    lib.scenario_sensor_write.restype = ctypes.c_int
    lib.scenario_sensor_read.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8)]
    lib.scenario_sensor_read.restype = ctypes.c_int
    lib.scenario_sensor_request_local.argtypes = [ctypes.c_void_p, ctypes.c_uint32,
                                                   ctypes.c_uint16, ctypes.c_uint16]
    lib.scenario_sensor_request_local.restype = ctypes.c_int
    return tmp, lib


class RegisterMap:
    """Faithful bounded register map for the LSM9DS1 driver contract."""
    def __init__(self):
        self.operations: list[dict] = []
        self.ag = {0x0F: 0x68, 0x17: 0x03, 0x22: 0x00}
        self.mag = {0x0F: 0x3D, 0x27: 0x08}
        self._put_axes(self.ag, 0x28, (-1000, 500, 2000))
        self._put_axes(self.ag, 0x18, (100, -200, 300))
        self._put_axes(self.mag, 0x28, (-1000, 2000, -3000))

    @staticmethod
    def _put_axes(bank, register, values):
        raw = struct.pack("<3h", *values)
        bank.update({register + i: value for i, value in enumerate(raw)})

    def write_registers(self, address: int, register: int, data: bytes):
        bank = self.ag if address == 0x6B else self.mag if address == 0x1E else None
        if bank is None: raise OSError(f"no register device at 0x{address:02x}")
        data = bytes(data)
        self.operations.append({"op": "write_registers", "address": address,
                                "register": register, "hex": data.hex()})
        for offset, value in enumerate(data): bank[register + offset] = value

    def read_registers(self, address: int, register: int, length: int) -> bytes:
        bank = self.ag if address == 0x6B else self.mag if address == 0x1E else None
        if bank is None: raise OSError(f"no register device at 0x{address:02x}")
        burst = register == 0xA8  # mag SUB[7] is the only mag burst increment bit
        start = register & 0x7F if address == 0x1E else register
        data = bytes(bank.get(start + (offset if burst or address == 0x6B else 0), 0)
                     for offset in range(length))
        self.operations.append({"op": "read_registers", "address": address,
            "register": register, "length": length, "burst": burst, "hex": data.hex()})
        return data


class PiBus:
    """Pi controller boundary; the sensor side calls the compiled mailbox callbacks."""
    def __init__(self, clock: ManualClock, sensor_lib=None, sensor=None):
        self.clock, self.start_at, self.lib, self.sensor = clock, clock.now, sensor_lib, sensor
        self.registers = RegisterMap()
        self.operations: list[dict] = []
        self.closed = False

    @staticmethod
    def _buffer(frame: bytes): return (ctypes.c_uint8 * MAILBOX).from_buffer_copy(frame)

    def write_mailbox(self, address: int, data: bytes):
        data = bytes(data)
        if len(data) != MAILBOX: raise ValueError("mailbox write must be 64 bytes")
        item = {"op": "write_mailbox", "address": address, "hex": data.hex(), "at": self.clock.now}
        self.operations.append(item)
        if address == 0x0A:
            self.lib.scenario_sensor_set_time(self.sensor, round((self.clock.now - self.start_at) * 1000))
            if self.lib.scenario_sensor_write(self.sensor, self._buffer(data)) != 1:
                raise OSError("compiled sensor callback did not stage the complete write")
        elif address != 0x08:
            raise OSError(f"no mailbox device at 0x{address:02x}")

    def read_mailbox(self, address: int) -> bytes:
        item = {"op": "read_mailbox", "address": address, "at": self.clock.now}
        self.operations.append(item)
        if address == 0x0A:
            self.lib.scenario_sensor_set_time(self.sensor, round((self.clock.now - self.start_at) * 1000))
            output = (ctypes.c_uint8 * MAILBOX)()
            if self.lib.scenario_sensor_read(self.sensor, output) != 1:
                raise OSError("compiled sensor callback missed output frame")
            result = bytes(output)
            item["hex"] = result.hex()
            return result
        if address == 0x08:
            item["hex"] = bytes(MAILBOX).hex()
            return bytes(MAILBOX)
        raise OSError(f"no mailbox device at 0x{address:02x}")

    def read_registers(self, address: int, register: int, length: int) -> bytes:
        return self.registers.read_registers(address, register, length)

    def write_registers(self, address: int, register: int, data: bytes) -> None:
        self.registers.write_registers(address, register, data)

    def close(self): self.closed = True


class RouterTransport:
    name = "compiled-sensor-router"
    def __init__(self, router: BoatRouter, clock: ManualClock):
        self.router, self.clock, self.on_frame = router, clock, None
        self.sent: list[bytes] = []
        self.frames: list[dict] = []
    def open(self, callback): self.on_frame = callback
    def close(self): self.on_frame = None
    def send_frame(self, frame: bytes):
        self.sent.append(bytes(frame))
        self.router.submit_frame(frame, ingress="radio")
    def receive(self, frame: bytes):
        message = codec.decode(frame)
        self.frames.append({"name": message["name"], "system": message["system"],
                            "component": message["component"], "hex": frame.hex(),
                            "fields": message["fields"]})
        if self.on_frame: self.on_frame(frame)
    def pump(self, steps=1):
        for _ in range(steps):
            self.router.step()
            self.clock.advance(20)


class SensorRig:
    def __init__(self, *, board=False, sensor_valid=True):
        self.clock = ManualClock()
        self.tmp, self.lib = _load_sensor()
        self.sensor = self.lib.scenario_sensor_create(0x159159)
        config = ServiceConfig(comms_enabled=False, sensor_enabled=board,
                               lsm9ds1_enabled=True)
        self.bus = PiBus(self.clock, self.lib, self.sensor)
        if not sensor_valid: self.bus.registers.ag[0x0F] = 0
        self.events = []
        self.transport = None
        self.router = BoatRouter(self.bus, config, clock=self.clock,
            on_frame=self._on_router_frame, on_event=self.events.append)
        self.router.boot_id = 0xA0B0C0D0
        self.transport = RouterTransport(self.router, self.clock)
        self.client = BoatClient(self.transport, clock=self.clock)
        self.client_events = []
        self.client.subscribe(self.client_events.append)
    def _on_router_frame(self, frame):
        if self.transport: self.transport.receive(frame)
    def connect(self):
        with patch("boat.client.secrets.randbits", return_value=0x50607080):
            self.client.connect()
        self.transport.pump(8)
    def pump_until(self, predicate, limit=120):
        for _ in range(limit):
            if predicate(): return
            self.transport.pump()
        raise AssertionError("production Pi/router/client did not complete before bounded ticks")
    def cleanup(self):
        try: self.router.close()
        finally:
            self.lib.scenario_sensor_destroy(self.sensor)
            self.tmp.cleanup()


def _imu_readings(rig): return [event for event in rig.client_events if isinstance(event, ImuReading)]
def _register_reads(rig): return [op for op in rig.bus.registers.operations if op["op"] == "read_registers"]


def run_sensor_direct_and_cache():
    """Direct path runs without a Pico; use only the production driver/router/client."""
    rig = SensorRig(board=False)
    try:
        rig.connect()
        ticket = rig.client.request_imu("direct", timeout_ms=800, max_age_ms=0)
        rig.pump_until(lambda: any(r.request_id == ticket.request_id for r in _imu_readings(rig)))
        reading = next(r for r in _imu_readings(rig) if r.request_id == ticket.request_id)
        assert (reading.status, reading.path, reading.valid_fields) == (1, 0, 7)
        assert reading.accel_mg == (-61, 31, 122)
        assert reading.gyro_mrad_s == (15, -31, 46)
        assert reading.mag_mgauss == (-140, 280, -420)
        assert (reading.acquisition_component, reading.source) == (1, (1, 1))
        assert not any(op["address"] == 0x0A for op in rig.bus.operations)
        assert any(op["register"] == 0xA8 and op["burst"] for op in _register_reads(rig))

        # Replay the exact request to prove one acquisition and an age-updated result.
        original = rig.transport.sent[-1]
        before = len(_register_reads(rig))
        rig.clock.advance(137)
        rig.router.submit_frame(original, ingress="radio")
        cached = []
        rig.client.subscribe(cached.append)
        rig.transport.pump(3)
        assert len(_register_reads(rig)) == before
        replay = [item for item in rig.transport.frames if item["name"] == "IMU_SAMPLE"][-1]
        assert replay["fields"]["sample_seq"] == reading.sample_seq
        assert replay["fields"]["sample_age_ms"] >= 137
        return {"direct": {"request_id": ticket.request_id, "ground_session": ticket.ground_session,
            "reading": reading.__dict__, "register_reads": _register_reads(rig),
            "mailbox_sensor_transactions": [op for op in rig.bus.operations if op["address"] == 0x0A],
            "duplicate_replay": replay}}
    finally: rig.cleanup()


def run_sensor_board_route():
    """Ground -> Pi -> compiled sensor callback -> Pi LSM -> board -> ground."""
    rig = SensorRig(board=True)
    try:
        rig.connect()
        ticket = rig.client.request_imu("board", timeout_ms=1200, max_age_ms=0)
        def dependency_ready():
            for op in rig.bus.operations:
                if op["op"] == "read_mailbox" and op["address"] == 0x0A and "hex" in op:
                    raw = bytes.fromhex(op["hex"])
                    if raw[0] == codec.MAGIC:
                        message = codec.decode(raw[:12 + raw[1]])
                        if message["name"] == "IMU_REQUEST": return True
            return False
        rig.pump_until(dependency_ready, limit=80)
        dependency_ops = [op for op in rig.bus.operations if op["op"] == "read_mailbox" and
                          op["address"] == 0x0A and "hex" in op]
        dependency = next((codec.decode(bytes.fromhex(op["hex"])[:12 + bytes.fromhex(op["hex"])[1]])
                           for op in dependency_ops
                           if bytes.fromhex(op["hex"])[0] == codec.MAGIC), None)
        assert dependency and dependency["name"] == "IMU_REQUEST", "compiled Pico did not emit dependency"
        assert dependency["fields"]["ground_session"] == ticket.ground_session
        assert dependency["fields"]["ground_session"] != 0
        assert dependency["fields"]["request_id"] == ticket.request_id
        assert dependency["fields"]["path"] == 0
        # Current parent Pi R1 rejects this real C dependency because it requires
        # session zero and guesses the sole parent. Pi R2 is being integrated separately.
        rig.transport.pump(8)
        rejects = [e.detail for e in rig.events if e.kind == "rejected" and
                   "sensor Pico dependency" in e.detail or "sensor dependency" in e.detail]
        assert not rejects, "known Pi R1 bug: _accept_imu_request rejects the C dependency's preserved ground_session"
        rig.pump_until(lambda: any(r.request_id == ticket.request_id for r in _imu_readings(rig)), limit=100)
        reading = next(r for r in _imu_readings(rig) if r.request_id == ticket.request_id)
        assert reading.path == 1 and reading.status == 1 and reading.valid_fields == 7
        assert reading.source == (1, COMPONENT_SENSOR)
        assert reading.mag_mgauss == (-140, 280, -420)
        before = len(_register_reads(rig))
        request_frame = rig.transport.sent[-1]
        rig.clock.advance(159)
        rig.router.submit_frame(request_frame, ingress="radio")
        rig.transport.pump(3)
        assert len(_register_reads(rig)) == before, "exact duplicate triggered another Pi acquisition"
        replay = [frame for frame in rig.transport.frames if frame["name"] == "IMU_SAMPLE"][-1]
        assert replay["fields"]["sample_seq"] == reading.sample_seq
        assert replay["fields"]["sample_age_ms"] >= 159
        return {"board": {"request_id": ticket.request_id, "ground_session": ticket.ground_session,
            "dependency": dependency, "reading": reading.__dict__,
            "duplicate_replay": replay, "register_reads": _register_reads(rig),
            "mailbox": rig.bus.operations}}
    finally: rig.cleanup()


def run_sensor_local_and_wrong_correlation():
    """Board-local session zero returns to Pi only and rejects a wrong request ID."""
    rig = SensorRig(board=True)
    try:
        assert rig.lib.scenario_sensor_request_local(rig.sensor, 700, 500, 0) == 1
        dependency_frame = rig.bus.read_mailbox(0x0A)
        dependency = codec.decode(dependency_frame[:12 + dependency_frame[1]])
        fields = dependency["fields"]
        assert dependency["name"] == "IMU_REQUEST"
        assert fields["ground_session"] == 0 and fields["request_id"] == 700
        assert (fields["target_system"], fields["target_component"]) == (1, 1)
        wrong = codec.encode("IMU_SAMPLE", {
            "ground_session": 0, "request_id": 701, "time_boot_ms": 42,
            "sample_seq": 1, "acquisition_boot_id": 991, "source_boot_id": 991,
            "sample_age_ms": 0, "valid_fields": 7,
            "xacc": 1, "yacc": 2, "zacc": 3, "xgyro": 4, "ygyro": 5,
            "zgyro": 6, "xmag": 7, "ymag": 8, "zmag": 9, "status": 1,
            "path": 0, "acquisition_component": 1, "target_system": 1,
            "target_component": COMPONENT_SENSOR,
        }, system=1, component=1)
        assert rig.bus.write_mailbox(0x0A, wrong + bytes(MAILBOX - len(wrong))) is None
        assert rig.bus.read_mailbox(0x0A) == bytes(MAILBOX)
        valid = dict(codec.decode(wrong)["fields"]); valid["request_id"] = 700
        response = codec.encode("IMU_SAMPLE", valid, system=1, component=1)
        rig.bus.write_mailbox(0x0A, response + bytes(MAILBOX - len(response)))
        reply = rig.bus.read_mailbox(0x0A)
        message = codec.decode(reply[:12 + reply[1]])
        assert message["name"] == "IMU_SAMPLE"
        assert message["fields"]["ground_session"] == 0
        assert message["fields"]["path"] == 1
        assert (message["fields"]["target_system"], message["fields"]["target_component"]) == (1, 1)
        assert not _imu_readings(rig), "board-local result must not fabricate a ground client event"
        return {"local_dependency": dependency, "wrong_correlation_reply": bytes(MAILBOX).hex(),
                "pi_target_reply": message}
    finally: rig.cleanup()


def run_sensor_error_and_partial():
    """Real direct driver errors and partial valid masks stay correlated and zero invalid axes."""
    partial = SensorRig(board=False)
    try:
        partial.bus.registers.ag[0x17] = 0x01  # acceleration ready, gyro not ready
        partial.bus.registers.mag[0x27] = 0
        partial.connect()
        ticket = partial.client.request_imu("direct", timeout_ms=800)
        partial.pump_until(lambda: any(r.request_id == ticket.request_id for r in _imu_readings(partial)))
        reading = next(r for r in _imu_readings(partial) if r.request_id == ticket.request_id)
        assert reading.status == 1 and reading.valid_fields == 1
        assert reading.gyro_mrad_s == (0, 0, 0) and reading.mag_mgauss == (0, 0, 0)
        partial_reading = reading.__dict__
    finally: partial.cleanup()

    broken = SensorRig(board=False, sensor_valid=False)
    try:
        broken.connect()
        ticket = broken.client.request_imu("direct", timeout_ms=1200)
        broken.pump_until(lambda: any(frame["name"] == "IMU_SAMPLE" and
            frame["fields"]["request_id"] == ticket.request_id for frame in broken.transport.frames), limit=100)
        error_frame = next(frame for frame in broken.transport.frames if frame["name"] == "IMU_SAMPLE" and
                           frame["fields"]["request_id"] == ticket.request_id)
        fields = error_frame["fields"]
        assert fields["status"] == 3 and fields["valid_fields"] == 0
        assert not any(fields[name] for name in ("xacc", "yacc", "zacc", "xgyro", "ygyro",
                                                  "zgyro", "xmag", "ymag", "zmag"))
        return {"partial": partial_reading, "identity_error_wire_frame": error_frame}
    finally: broken.cleanup()


def run_sensor_stop_priority():
    """One complete STOP write reaches the motor before queued sensor work."""
    rig = SensorRig(board=False)
    try:
        rig.connect()
        rig.client.request_imu("direct", timeout_ms=800)
        stop = codec.encode("ACTUATOR_REQUEST", {
            "request_id": 9, "ground_session": rig.client.ground_session,
            "delta_cdeg": 0, "timeout_ms": 2000, "max_rate_cdeg_s": 500,
            "target_system": 1, "target_component": 154,
            "actuator": 1, "opcode": 3, "control_source": 3,
        }, system=SYSTEM_GROUND, component=COMPONENT_GROUND, sequence=9)
        assert rig.router.submit_frame(stop, ingress="radio")
        rig.router.step()
        writes = [op for op in rig.bus.operations if op["op"] == "write_mailbox"]
        assert len(writes) == 1 and writes[0]["address"] == 0x08
        message = codec.decode(bytes.fromhex(writes[0]["hex"])[:12 + bytes.fromhex(writes[0]["hex"])[1]])
        assert message["name"] == "ACTUATOR_REQUEST" and message["fields"]["opcode"] == 3
        assert not _register_reads(rig), "sensor acquisition ran ahead of the queued STOP"
        return {"motor_stop_write": writes[0], "decoded": message,
                "sensor_register_reads": _register_reads(rig)}
    finally: rig.cleanup()


def run_sensor_radio_route():
    """Use both production modem drivers and Pi service around the C mailbox."""
    from scenarios.direct_radio import DirectRadioRig
    fixture = SensorRig(board=True)
    radio = DirectRadioRig(rig=fixture)
    try:
        radio.start(reconcile_motor=False)
        evidence = []
        for path, source in (("direct", (1, 1)), ("board", (1, 159))):
            ticket = radio.client.request_imu(path, timeout_ms=2000)
            result = None
            for _ in range(80):
                radio.pump()
                result = next((event for event in radio.events
                    if isinstance(event, ImuReading) and event.request_id == ticket.request_id), None)
                if result is not None:
                    break
            assert result is not None, f"{path}: no correlated IMU radio response"
            assert (result.status, result.valid_fields, result.source) == (1, 7, source), result
            assert result.mag_mgauss == (-140, 280, -420), result
            evidence.append({"path": path, "request_id": result.request_id,
                "source": list(result.source), "valid_fields": result.valid_fields,
                "mag_mgauss": list(result.mag_mgauss)})
        decoded = [codec.decode(bytes.fromhex(item["frame"])) for item in radio.air.rf_deliveries]
        assert sum(item["name"] == "IMU_REQUEST" for item in decoded) == 2
        assert sum(item["name"] == "IMU_SAMPLE" for item in decoded) == 2
        assert not any(item["address"] == 0x09 for item in fixture.bus.operations)
        return {"readings": evidence, "rf_frames": len(decoded),
                "sensor_mailbox_operations": len([item for item in fixture.bus.operations
                    if item["address"] == 0x0A])}
    finally:
        radio.close()


def run_sensor_scenarios():
    """Run sensor scenarios individually so the known integration defect stays visible."""
    results = {}
    for name, function in (("direct", run_sensor_direct_and_cache),
                           ("board", run_sensor_board_route),
                           ("local", run_sensor_local_and_wrong_correlation),
                           ("errors", run_sensor_error_and_partial),
                           ("stop_priority", run_sensor_stop_priority),
                           ("radio", run_sensor_radio_route)):
        try: results[name] = {"status": "PASS", "evidence": function()}
        except Exception as exc: results[name] = {"status": "FAIL", "detail": str(exc)}
    return results


if __name__ == "__main__":
    report = run_sensor_scenarios()
    print(json.dumps(report, indent=2, sort_keys=True, default=str))
    raise SystemExit(1 if any(item["status"] != "PASS" for item in report.values()) else 0)
