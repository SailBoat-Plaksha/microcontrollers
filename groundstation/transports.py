"""Raw MAVLink transports used by the NiceGUI app and CLI."""
from __future__ import annotations

import queue
import re
import secrets
import subprocess
import threading
import time
from typing import Callable

from protocol import codec


class SerialTransport:
    """USB raw MAVLink transport for the ground Pico."""
    name = "LoRa · ground Pico USB"

    def __init__(self, port: str, baudrate: int = 115200, *, serial_factory=None):
        self.port, self.baudrate = port, baudrate
        self._serial_factory = serial_factory
        self._serial = None
        self._stop = threading.Event()
        self._thread = None
        self._write_lock = threading.Lock()

    @property
    def is_connected(self):
        return self._serial is not None and not self._stop.is_set()

    def open(self, on_frame: Callable[[bytes], None]) -> None:
        if self._serial is not None:
            return
        if self._serial_factory is None:
            import serial
            self._serial_factory = serial.Serial
        serial_obj = self._serial_factory(self.port, self.baudrate, timeout=0.1, write_timeout=1.0)
        self._serial = serial_obj
        self._stop.clear()
        self._thread = threading.Thread(target=self._read_loop, args=(on_frame,), daemon=True,
                                        name="groundstation-serial")
        self._thread.start()

    def _read_loop(self, on_frame):
        stream = codec.FrameStream()
        while not self._stop.is_set():
            try:
                chunk = self._serial.read(256)
                for frame, _ in stream.feed(chunk):
                    on_frame(frame)
            except Exception:
                if not self._stop.is_set():
                    self._stop.set()
                break

    def send_frame(self, frame: bytes) -> None:
        if self._serial is None or self._stop.is_set():
            raise ConnectionError("USB serial transport is disconnected")
        codec.decode(frame)
        with self._write_lock:
            written = self._serial.write(frame)
            if written != len(frame):
                raise OSError(f"short serial write ({written}/{len(frame)} bytes)")
            self._serial.flush()

    def close(self) -> None:
        serial_obj, self._serial = self._serial, None
        self._stop.set()
        if serial_obj is not None:
            try:
                serial_obj.close()
            finally:
                if self._thread is not None and self._thread is not threading.current_thread():
                    self._thread.join(timeout=1.0)
        self._thread = None


class SSHTransport:
    """Persistent strict-host-key SSH process carrying raw frames on stdio."""
    name = "SSH · Pi bridge"
    _ALIAS = re.compile(r"^[A-Za-z0-9_.@-]{1,128}$")

    def __init__(self, host_alias: str, *, config_path="/etc/arsem/onboard.toml",
                 popen_factory=subprocess.Popen):
        if not self._ALIAS.fullmatch(host_alias) or host_alias.startswith("-"):
            raise ValueError("use a safe SSH config host alias (letters, digits, . _ @ -)")
        self.host_alias = host_alias
        if not config_path.startswith("/") or any(c in config_path for c in "\x00\n\r"):
            raise ValueError("bridge config path must be an absolute path")
        self.config_path = config_path
        self._popen_factory = popen_factory
        self._proc = None
        self._stop = threading.Event()
        self._stdout_thread = self._stderr_thread = None
        self._on_frame = None
        self._write_lock = threading.Lock()
        self.stderr_lines: queue.Queue[str] = queue.Queue(maxsize=100)

    @property
    def is_connected(self):
        return (self._proc is not None and not self._stop.is_set() and
                self._proc.poll() is None)

    def open(self, on_frame: Callable[[bytes], None]) -> None:
        if self._proc is not None:
            return
        argv = ["ssh", "-T", "-o", "BatchMode=yes", "-o", "StrictHostKeyChecking=yes",
                "-o", "ClearAllForwardings=yes", self.host_alias,
                "python3", "-m", "onboard", "bridge", "--config", self.config_path]
        proc = self._popen_factory(argv, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, bufsize=0)
        if proc.stdin is None or proc.stdout is None or proc.stderr is None:
            proc.kill()
            raise OSError("SSH process did not provide all required pipes")
        self._proc, self._on_frame = proc, on_frame
        self._stop.clear()
        self._stdout_thread = threading.Thread(target=self._read_stdout, daemon=True,
                                               name="groundstation-ssh-frames")
        self._stderr_thread = threading.Thread(target=self._read_stderr, daemon=True,
                                               name="groundstation-ssh-logs")
        self._stdout_thread.start()
        self._stderr_thread.start()

    def _read_stdout(self):
        stream = codec.FrameStream()
        try:
            while not self._stop.is_set() and self._proc is not None:
                chunk = self._proc.stdout.read(256)
                if not chunk:
                    break
                for frame, _ in stream.feed(chunk):
                    self._on_frame(frame)
        except Exception as exc:
            self._put_stderr(f"stdout reader: {type(exc).__name__}: {exc}")
        finally:
            if not self._stop.is_set():
                self._stop.set()

    def _read_stderr(self):
        try:
            for line in iter(self._proc.stderr.readline, b""):
                self._put_stderr(line.decode("utf-8", errors="replace").rstrip())
        except Exception as exc:
            self._put_stderr(f"stderr reader: {type(exc).__name__}: {exc}")

    def _put_stderr(self, line):
        try:
            self.stderr_lines.put_nowait(line)
        except queue.Full:
            try:
                self.stderr_lines.get_nowait()
            except queue.Empty:
                pass
            try:
                self.stderr_lines.put_nowait(line)
            except queue.Full:
                pass

    def send_frame(self, frame: bytes) -> None:
        if self._proc is None or self._stop.is_set() or self._proc.poll() is not None:
            raise ConnectionError("SSH bridge is disconnected")
        codec.decode(frame)
        with self._write_lock:
            written = self._proc.stdin.write(frame)
            if written is not None and written != len(frame):
                raise OSError(f"short SSH stdin write ({written}/{len(frame)} bytes); outcome uncertain")
            self._proc.stdin.flush()

    def close(self) -> None:
        proc, self._proc = self._proc, None
        self._stop.set()
        if proc is None:
            return
        try:
            if proc.stdin:
                proc.stdin.close()
        except OSError:
            pass
        try:
            proc.wait(timeout=1.0)
        except subprocess.TimeoutExpired:
            proc.terminate()
            try:
                proc.wait(timeout=1.0)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=1.0)
        for thread in (self._stdout_thread, self._stderr_thread):
            if thread and thread is not threading.current_thread():
                thread.join(timeout=0.5)
        self._stdout_thread = self._stderr_thread = None


class SimulationTransport:
    """Codec-backed in-process peer. Every value is explicitly simulated."""
    name = "SIMULATION · generated values"

    def __init__(self, *, response_delay: float = 0.005, telemetry_interval: float = 1.0,
                 fail_open: bool = False, fail_send: bool = False):
        self.response_delay = response_delay
        self.telemetry_interval = telemetry_interval
        self.fail_open, self.fail_send = fail_open, fail_send
        self.on_frame = None
        self.connected = False
        self._boot_id = secrets.randbits(32) or 1
        self._angles = {1: 0, 2: 0}
        self._enabled = {1: False, 2: False}
        self._counter = 0
        self._sample_seq = 0
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._worker = None
        self._generation = 0

    @property
    def is_connected(self):
        return self.connected

    def open(self, on_frame):
        if self.fail_open:
            raise OSError("simulated open failure")
        self._generation += 1
        self._boot_id = secrets.randbits(32) or 1
        self.on_frame, self.connected = on_frame, True
        self._stop.clear()
        self._emit_telemetry()
        self._worker = threading.Thread(target=self._telemetry_loop, daemon=True,
                                        name="groundstation-simulation-telemetry")
        self._worker.start()

    def close(self):
        self._generation += 1
        self.connected, self.on_frame = False, None
        self._stop.set()
        if self._worker is not None and self._worker is not threading.current_thread():
            self._worker.join(timeout=max(0.1, min(self.telemetry_interval + .1, 1.5)))
        self._worker = None

    def _telemetry_loop(self):
        while not self._stop.wait(max(0.05, self.telemetry_interval)):
            self._emit_telemetry()

    def send_frame(self, frame: bytes):
        if not self.connected:
            raise ConnectionError("simulation is disconnected")
        if self.fail_send:
            raise OSError("simulated ambiguous write failure")
        msg = codec.decode(frame)
        with self._lock:
            self._counter = (self._counter + 1) & 0xffffffff
            n = self._counter
            if msg["name"] == "ACTUATOR_REQUEST" and msg["fields"]["opcode"] != 3:
                self._enabled[msg["fields"]["actuator"]] = True
        generation = self._generation
        threading.Thread(target=self._respond, args=(msg, n, generation), daemon=True,
                         name="groundstation-simulation").start()

    def _emit(self, name, fields, *, system=1, component=1):
        if self.connected and self.on_frame is not None:
            frame = codec.encode(name, fields, system=system, component=component,
                                 sequence=self._counter & 0xff)
            self.on_frame(frame)

    def _respond(self, msg, counter, generation):
        time.sleep(self.response_delay)
        if not self.connected or generation != self._generation:
            return
        name, f = msg["name"], msg["fields"]
        if name == "PING" and f["target_system"] == 0 and f["target_component"] == 0:
            for component in (1, 154, 158):
                self._emit("PING", {"time_usec": f["time_usec"], "seq": f["seq"],
                                     "target_system": msg["system"],
                                     "target_component": msg["component"]}, component=component)
            self._emit_telemetry()
        elif name == "ACTUATOR_REQUEST":
            op = f["opcode"]
            actuator = f["actuator"]
            if op == 1:
                self._angles[actuator] = f["delta_cdeg"]
            elif op == 2:
                self._angles[actuator] += f["delta_cdeg"]
            elif op == 3:
                self._enabled[actuator] = False
            session, req = f["ground_session"], f["request_id"]
            self._emit("REQUEST_RESULT", {"ground_session": session, "request_id": req,
                "motor_boot_id": self._boot_id, "local_command_id": counter,
                "result": 1 if op != 3 else 0, "state": 2 if op != 3 else 5,
                "target_system": msg["system"], "target_component": msg["component"]})
            if op != 3:
                self._emit("REQUEST_RESULT", {"ground_session": session, "request_id": req,
                    "motor_boot_id": self._boot_id, "local_command_id": counter,
                    "result": 1, "state": 3, "target_system": msg["system"],
                    "target_component": msg["component"]})
            threading.Thread(target=self._complete_request,
                args=(actuator, op, counter, session, req, msg["system"],
                      msg["component"], generation),
                daemon=True, name="groundstation-simulation-command").start()
        elif name == "IMU_REQUEST":
            f = msg["fields"]
            with self._lock:
                self._sample_seq = (self._sample_seq + 1) & 0xffffffff
                sample_seq = self._sample_seq
            board_path = f["path"] == 1
            self._emit("IMU_SAMPLE", {"ground_session": f["ground_session"],
                "request_id": f["request_id"],
                "time_boot_ms": int(time.monotonic() * 1000) & 0xffffffff,
                "sample_seq": sample_seq, "acquisition_boot_id": 1,
                "source_boot_id": 159 if board_path else 1,
                "sample_age_ms": 0, "valid_fields": 7,
                "xacc": 12, "yacc": -8, "zacc": 1000,
                "xgyro": 2, "ygyro": 0, "zgyro": -1,
                "xmag": 31, "ymag": -24, "zmag": 8,
                "status": 1, "path": f["path"], "acquisition_component": 1,
                "target_system": f["target_system"],
                "target_component": f["target_component"]},
                component=159 if board_path else 1)

    def _complete_request(self, actuator, op, counter, session, req, system, component, generation):
        time.sleep(max(0.02, self.response_delay))
        if not self.connected or generation != self._generation:
            return
        self._enabled[actuator] = False
        if op != 3:
            self._emit("REQUEST_RESULT", {"ground_session": session, "request_id": req,
                "motor_boot_id": self._boot_id, "local_command_id": counter,
                "result": 1, "state": 4, "target_system": system,
                "target_component": component})
        self._emit_status(actuator, counter)

    def _emit_telemetry(self):
        # Fresh health, IMU, and independent actuator measurements for UI work.
        for component in (1, 154, 158):
            self._emit("HEARTBEAT", {"type": 6, "autopilot": 8, "base_mode": 0,
                "custom_mode": 0, "system_status": 4, "mavlink_version": 3}, component=component)
            self._emit("NODE_HEALTH", {"boot_id": self._boot_id if component == 154 else component,
                "time_boot_ms": int(time.monotonic()*1000) & 0xffffffff,
                "fault_flags": 0, "sensor_valid_fields": 3 if component == 1 else 0,
                "queue_drops": 0, "reset_reason": 0,
                "feedback_valid": int(component == 154)}, component=component)
        self._emit("SCALED_IMU", {"time_boot_ms": int(time.monotonic()*1000) & 0xffffffff,
            "xacc": 12, "yacc": -8, "zacc": 1000, "xgyro": 2, "ygyro": 0,
            "zgyro": -1, "xmag": 0, "ymag": 0, "zmag": 0, "temperature": 0}, component=1)
        self._emit_status(1)
        self._emit_status(2)

    def _emit_status(self, actuator, command_id=0):
        self._emit("ACTUATOR_STATUS", {"time_boot_ms": int(time.monotonic()*1000) & 0xffffffff,
            "command_id": command_id, "requested_position_cdeg": self._angles[actuator],
            "measured_position_cdeg": self._angles[actuator], "velocity_cdeg_s": 0,
            "fault_flags": 0, "actuator": actuator, "control_source": 3,
            "result": 0, "enabled": int(self._enabled[actuator])}, component=154)
