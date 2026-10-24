"""Direct RYLR998 USB-UART transport for raw MAVLink frames."""
from __future__ import annotations

import heapq
import math
import queue
import threading
import time
from dataclasses import dataclass, field
from typing import Callable

from protocol import codec


@dataclass(order=True)
class _Send:
    priority: int
    order: int
    command: bytes = field(compare=False)
    done: threading.Event = field(compare=False)
    error: BaseException | None = field(default=None, compare=False)
    started: bool = field(default=False, compare=False)


class LoRaModemTransport:
    """Host-side RYLR998 modem transport implementing ``boat.client.Transport``.

    Serial reads, AT acknowledgements, and queued sends are managed off the
    caller thread. ``+OK`` only confirms modem acceptance; motor outcomes arrive
    later as MAVLink messages from the Pi/router. ``send_frame`` waits for the
    modem ACK and can block for ``command_timeout_s``; callers must invoke it
    from a worker, outside a GUI event loop and outside the Pi's sole I2C thread.
    """

    name = "LoRa · direct USB UART"
    MAX_LINE_BYTES = 1024
    MAX_RADIO_PAYLOAD = codec.MAX_RADIO_TEXT

    def __init__(self, port, baudrate=115200, *, local_address=2, peer_address=1,
                 network_id=18, band_hz=None, parameter="10,7,1,12",
                 serial_factory=None, command_timeout_s=3.0, send_gap_s=0.0,
                 queue_limit=32):
        for name, value, low, high in (("local_address", local_address, 0, 65535),
                                       ("peer_address", peer_address, 0, 65535),
                                       ("network_id", network_id, 3, 18)):
            if isinstance(value, bool) or not isinstance(value, int) or not low <= value <= high:
                label = {"local_address": "Local modem address", "peer_address": "Boat modem address",
                         "network_id": "Network ID"}[name]
                raise ValueError(f"{label} must be a whole number between {low} and {high}.")
        if network_id not in (*range(3, 16), 18):
            raise ValueError("network_id must be 3..15 or 18")
        if band_hz is not None and (isinstance(band_hz, bool) or not isinstance(band_hz, int)
                                    or band_hz <= 0):
            raise ValueError("Modem frequency must be a positive whole number in Hz, or left blank.")
        if not isinstance(port, str) or not port.strip() or any(c in port for c in "\x00\r\n"):
            raise ValueError("port must be a non-empty serial port name")
        if isinstance(baudrate, bool) or not isinstance(baudrate, int) or baudrate <= 0:
            raise ValueError("baudrate must be a positive integer")
        if not isinstance(parameter, str) or not parameter.isascii():
            raise ValueError("parameter must be four comma-separated integers")
        try:
            parts = parameter.split(",")
            if len(parts) != 4 or any(not value.strip().isdigit() for value in parts):
                raise ValueError
            sf, bandwidth, coding_rate, preamble = (int(value.strip()) for value in parts)
        except (TypeError, ValueError) as exc:
            raise ValueError("parameter must be four comma-separated integers") from exc
        if (not 5 <= sf <= 11 or not 7 <= bandwidth <= 9 or not 1 <= coding_rate <= 4
                or not 4 <= preamble <= 24 or (network_id != 18 and preamble != 12)):
            raise ValueError("parameter values are outside RYLR998 limits for this network")
        if (isinstance(command_timeout_s, bool) or not isinstance(command_timeout_s, (int, float))
                or not math.isfinite(command_timeout_s) or command_timeout_s <= 0
                or isinstance(send_gap_s, bool) or not isinstance(send_gap_s, (int, float))
                or not math.isfinite(send_gap_s) or send_gap_s < 0
                or isinstance(queue_limit, bool) or not isinstance(queue_limit, int)
                or queue_limit < 1):
            raise ValueError("timeouts, send gap, and queue limit must be positive")

        self.port, self.baudrate = port.strip(), baudrate
        self.local_address, self.peer_address = local_address, peer_address
        self.network_id, self.band_hz = network_id, band_hz
        self.parameter = f"{sf},{bandwidth},{coding_rate},{preamble}"
        self.command_timeout_s, self.send_gap_s, self.queue_limit = (
            float(command_timeout_s), float(send_gap_s), int(queue_limit))
        self._serial_factory = serial_factory
        self._serial = None
        self._on_frame = None
        self._stop = threading.Event()
        self._ready = False
        self._generation = 0
        self._reader = self._sender = self._dispatcher = None
        self._state_lock = threading.RLock()
        self._command_lock = threading.Lock()
        self._pending_lock = threading.Lock()
        self._pending = None
        self._queue_cond = threading.Condition()
        self._queue: list[_Send] = []
        self._queue_order = 0
        self._callback_queue = None
        self.stderr_lines: queue.Queue[str] = queue.Queue(maxsize=100)
        self.stats = {"rx_lines": 0, "rx_frames": 0, "rx_rejected": 0,
                      "tx_frames": 0, "tx_accepted": 0, "tx_errors": 0,
                      "queue_drops": 0, "config_commands": 0}

    @property
    def is_connected(self):
        with self._state_lock:
            return self._ready and self._serial is not None and not self._stop.is_set()

    def open(self, on_frame: Callable[[bytes], None]) -> None:
        with self._state_lock:
            if self._serial is not None:
                return
        # Retire a previous closed session before installing any new session
        # state. Joining happens without the state lock so a callback can finish.
        self._join_threads()
        with self._state_lock:
            if self._serial is not None:
                return
            if any(thread is not None and thread.is_alive()
                   for thread in (self._reader, self._sender, self._dispatcher)):
                raise RuntimeError("previous radio session is still stopping; call close and retry")
            self._generation += 1
            generation = self._generation
            self._stop.clear()
            self._ready = False
            factory = self._serial_factory
        try:
            if factory is None:
                import serial
                factory = serial.Serial
                self._serial_factory = factory
            serial_obj = factory(self.port, self.baudrate, timeout=0.1, write_timeout=1.0)
            with self._state_lock:
                if generation != self._generation:
                    serial_obj.close()
                    raise ConnectionError("radio open was cancelled")
                self._serial, self._on_frame = serial_obj, on_frame
                callback_queue = queue.Queue(maxsize=self.queue_limit)
                self._callback_queue = callback_queue
            self._dispatcher = threading.Thread(
                target=self._dispatch_loop, args=(serial_obj, generation, callback_queue),
                daemon=True, name="lora-modem-callbacks")
            self._dispatcher.start()
            self._reader = threading.Thread(target=self._read_loop, args=(serial_obj, generation),
                                            daemon=True, name="lora-modem-reader")
            self._reader.start()
            for command in self._configuration_commands():
                self._run_command(command, self.command_timeout_s, config=True)
            self._sender = threading.Thread(target=self._send_loop, args=(generation,),
                                            daemon=True, name="lora-modem-sender")
            self._sender.start()
            with self._state_lock:
                if generation != self._generation or self._stop.is_set():
                    raise ConnectionError("radio stopped before configuration was ready")
                self._ready = True
            self._status("RYLR998 configured and ready; waiting for boat frames")
        except BaseException:
            self._fail_and_close("radio configuration failed")
            self._join_threads()
            raise

    def _configuration_commands(self):
        commands = [f"AT+ADDRESS={self.local_address}\r\n"]
        commands.append(f"AT+NETWORKID={self.network_id}\r\n")
        if self.band_hz is not None:
            commands.append(f"AT+BAND={self.band_hz}\r\n")
        commands.append(f"AT+PARAMETER={self.parameter}\r\n")
        return [value.encode("ascii") for value in commands]

    def _status(self, line):
        try:
            self.stderr_lines.put_nowait(str(line)[:self.MAX_LINE_BYTES])
        except queue.Full:
            try:
                self.stderr_lines.get_nowait()
            except queue.Empty:
                pass
            try:
                self.stderr_lines.put_nowait(str(line)[:self.MAX_LINE_BYTES])
            except queue.Full:
                pass

    def _serial_write_once(self, serial_obj, command):
        # Continue a partial physical write at the unsent offset only. Never
        # retry a command or full frame after an ambiguous write.
        offset = 0
        while offset < len(command):
            written = serial_obj.write(command[offset:])
            if isinstance(written, bool) or not isinstance(written, int) or written <= 0:
                raise OSError(f"short serial write ({offset}/{len(command)} bytes); outcome uncertain")
            if written > len(command) - offset:
                raise OSError(f"invalid serial write count ({written}); outcome uncertain")
            offset += written
        serial_obj.flush()

    def _run_command(self, command, timeout, *, config=False):
        with self._command_lock:
            with self._state_lock:
                serial_obj, generation = self._serial, self._generation
            if serial_obj is None or self._stop.is_set():
                raise ConnectionError("LoRa modem is disconnected")
            done = threading.Event()
            pending = {"event": done, "result": None, "command": command,
                       "generation": generation}
            with self._pending_lock:
                self._pending = pending
            try:
                if not self._session_current(serial_obj, generation):
                    raise ConnectionError("LoRa modem session changed before write")
                self._serial_write_once(serial_obj, command)
            except BaseException:
                with self._pending_lock:
                    if self._pending is pending:
                        self._pending = None
                self.stats["tx_errors"] += 1
                raise
            if not done.wait(timeout):
                with self._pending_lock:
                    if self._pending is pending:
                        self._pending = None
                self.stats["tx_errors"] += 1
                raise TimeoutError(f"RYLR998 command timed out; outcome uncertain: {command[:32]!r}")
            result = pending["result"]
            if result != "ok":
                self.stats["tx_errors"] += 1
                raise OSError(f"RYLR998 rejected command: {result}")
            if config:
                self.stats["config_commands"] += 1
            else:
                self.stats["tx_accepted"] += 1

    def _read_loop(self, serial_obj, generation):
        line = bytearray()
        dropping = False
        try:
            while not self._stop.is_set():
                if not self._session_current(serial_obj, generation):
                    return
                data = serial_obj.read(256)
                if not data:
                    continue
                for byte in data:
                    if byte == 10:
                        if not dropping:
                            self._handle_line(bytes(line).strip(b"\r\n"), serial_obj, generation)
                        line.clear()
                        dropping = False
                    elif not dropping:
                        if len(line) >= self.MAX_LINE_BYTES:
                            line.clear()
                            dropping = True
                            self.stats["rx_rejected"] += 1
                            self._status("discarded oversized modem line")
                        else:
                            line.append(byte)
        except Exception as exc:
            if not self._stop.is_set():
                self._status(f"modem reader failed: {type(exc).__name__}: {exc}")
                self._fail_and_close("radio reader failed; reconnect required")

    def _session_current(self, serial_obj, generation):
        with self._state_lock:
            return generation == self._generation and serial_obj is self._serial

    def _handle_line(self, line, serial_obj, generation):
        if not self._session_current(serial_obj, generation):
            return
        if not line:
            return
        self.stats["rx_lines"] += 1
        if line in (b"+OK",) or line.startswith(b"+ERR="):
            with self._state_lock:
                current = generation == self._generation and serial_obj is self._serial
                if current:
                    with self._pending_lock:
                        pending = self._pending
                        if pending is not None and pending["generation"] == generation:
                            pending["result"] = "ok" if line == b"+OK" else line.decode("ascii", "replace")
                            self._pending = None
                            pending["event"].set()
            if line.startswith(b"+ERR="):
                self._status("modem " + line.decode("ascii", "replace"))
            return
        if not line.startswith(b"+RCV="):
            if line.startswith((b"+READY", b"+RESET")):
                self._status(line.decode("ascii", "replace"))
                with self._state_lock:
                    reset_after_ready = (generation == self._generation and serial_obj is self._serial
                                         and self._ready)
                if reset_after_ready:
                    self._fail_and_close("modem reset after configuration; reconnect required")
            return
        try:
            # REYAX examples show optional spaces after commas. Strip only the
            # five field boundaries; payload whitespace remains invalid.
            fields = line.split(b",")
            if len(fields) == 5:
                line = b",".join(value.strip() for value in fields)
            frame, _decoded, metadata = codec.radio_receive(line)
            if metadata["address"] != self.peer_address:
                raise ValueError(f"unexpected modem peer {metadata['address']}")
        except (ValueError, TypeError) as exc:
            self.stats["rx_rejected"] += 1
            self._status(f"discarded modem frame: {exc}")
            return
        self.stats["rx_frames"] += 1
        with self._state_lock:
            callback_queue = (self._callback_queue if generation == self._generation
                              and serial_obj is self._serial and self._ready
                              and not self._stop.is_set() else None)
        if callback_queue is not None:
            try:
                callback_queue.put_nowait((generation, frame))
            except queue.Full:
                self.stats["rx_rejected"] += 1
                self._fail_and_close("radio callback queue full; reconnect required")

    def _dispatch_loop(self, serial_obj, generation, callback_queue):
        while not self._stop.is_set():
            try:
                item_generation, frame = callback_queue.get(timeout=0.1)
            except queue.Empty:
                continue
            with self._state_lock:
                callback = (self._on_frame if item_generation == generation
                            and self._session_current(serial_obj, generation)
                            and self._ready and not self._stop.is_set() else None)
            if callback is not None:
                try:
                    callback(frame)
                except Exception as exc:
                    self._status(f"frame callback failed: {type(exc).__name__}: {exc}")

    def send_frame(self, raw_mavlink):
        codec.decode(raw_mavlink)
        if not self.is_connected:
            raise ConnectionError("LoRa modem transport is disconnected")
        encoded = codec.radio_command(raw_mavlink, address=self.peer_address)
        decoded = codec.decode(raw_mavlink)
        priority = 0 if decoded["name"] == "ACTUATOR_REQUEST" and decoded["fields"].get("opcode") == 3 else 1
        with self._queue_cond:
            if len(self._queue) >= self.queue_limit:
                if priority == 0:
                    candidates = [item for item in self._queue if item.priority > 0]
                    if candidates:
                        victim = max(candidates, key=lambda item: item.order)
                        self._queue.remove(victim)
                        heapq.heapify(self._queue)
                        victim.error = BufferError("queued movement displaced by STOP")
                        victim.done.set()
                        self.stats["queue_drops"] += 1
                    else:
                        raise BufferError("radio send queue is full of STOP commands")
                else:
                    self.stats["queue_drops"] += 1
                    raise BufferError("radio send queue is full")
            self._queue_order += 1
            item = _Send(priority, self._queue_order, encoded, threading.Event())
            heapq.heappush(self._queue, item)
            self._queue_cond.notify()
        if not item.done.wait(self.command_timeout_s + self.send_gap_s + 0.5):
            with self._queue_cond:
                if not item.started and item in self._queue:
                    self._queue.remove(item)
                    heapq.heapify(self._queue)
                    raise TimeoutError("radio send expired in queue; not transmitted")
            raise TimeoutError("radio send acknowledgement timed out; outcome uncertain")
        if item.error:
            raise item.error

    def _send_loop(self, generation):
        while not self._stop.is_set():
            with self._queue_cond:
                while not self._queue and not self._stop.is_set():
                    self._queue_cond.wait(0.1)
                if self._stop.is_set():
                    return
                item = heapq.heappop(self._queue)
                item.started = True
            try:
                self._run_command(item.command, self.command_timeout_s)
                self.stats["tx_frames"] += 1
            except BaseException as exc:
                item.error = exc
                self._status(f"radio send failed: {type(exc).__name__}: {exc}")
                item.done.set()
                self._fail_and_close("radio command failed")
                return
            item.done.set()
            if self.send_gap_s:
                self._stop.wait(self.send_gap_s)

    def _fail_and_close(self, reason):
        self._status(reason)
        with self._state_lock:
            serial_obj = self._serial
            self._ready = False
            self._generation += 1
            self._serial = None
            self._on_frame = None
            self._callback_queue = None
            self._stop.set()
        with self._pending_lock:
            pending, self._pending = self._pending, None
            if pending is not None:
                pending["result"] = "transport closed"
                pending["event"].set()
        with self._queue_cond:
            while self._queue:
                item = heapq.heappop(self._queue)
                item.error = ConnectionError("radio closed before command was sent")
                item.done.set()
            self._queue_cond.notify_all()
        if serial_obj is not None:
            try:
                serial_obj.close()
            except Exception:
                pass

    def close(self):
        self._fail_and_close("radio closed; reconnect required")
        self._join_threads()

    def _join_threads(self):
        for thread in (self._reader, self._sender, self._dispatcher):
            if thread and thread is not threading.current_thread():
                thread.join(timeout=1.0)
        if self._reader is not None and not self._reader.is_alive():
            self._reader = None
        if self._sender is not None and not self._sender.is_alive():
            self._sender = None
        if self._dispatcher is not None and not self._dispatcher.is_alive():
            self._dispatcher = None
