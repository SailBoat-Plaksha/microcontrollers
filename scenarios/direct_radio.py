"""Direct Laptop/Pi RYLR998 route scenarios with paired fake serial modems.

The fixture models only the physical modem boundary: serial AT commands,
acknowledgements, and the matching +RCV line delivered to the paired endpoint.
BoatClient, LoRaModemTransport, the Pi radio service/router, and the compiled
motor adapter remain the production implementations.
"""
from __future__ import annotations

import base64
import json
import threading
import time
from types import SimpleNamespace

from boat.client import BoatClient, PongReply, RequestOutcome
from boat.radio import LoRaModemTransport
from onboard.__main__ import _make_radio_transport, _run_radio_service
from onboard.config import ServiceConfig
from protocol import codec
from scenarios.integrated import IntegratedRig

TIMEOUT = 2.0


def wait_for(predicate, message, timeout=TIMEOUT):
    deadline = threading.Event()
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if predicate():
            return
        deadline.wait(0.002)
    raise AssertionError(message)


class FakeRadioAir:
    """Two serial devices linked at RYLR998 +SEND/+RCV boundary."""
    def __init__(self):
        self.cv = threading.Condition()
        self.devices = {}
        self.tx = []
        self.ack_mode = {1: "ok", 2: "ok"}
        self.air_delay = 0.0
        self.closed = set()

    def factory(self, port, baudrate, **kwargs):
        address = 1 if port == "pi-fake" else 2
        device = FakeSerial(self, address)
        self.devices[address] = device
        return device

    def deliver(self, source, command):
        if not command.startswith(b"AT+SEND="):
            if command.startswith((b"AT+ADDRESS=", b"AT+NETWORKID=", b"AT+BAND=",
                                  b"AT+PARAMETER=")):
                self.devices[source].inject(b"+OK\r\n")
            return
        fields = command.strip().split(b",", 2)
        if len(fields) != 3:
            raise AssertionError(f"malformed production AT+SEND: {command!r}")
        address = int(fields[0].split(b"=", 1)[1])
        text = fields[2]
        if int(fields[1]) != len(text):
            raise AssertionError("AT+SEND length did not match payload")
        payload = base64.b64decode(text, validate=True)
        codec.decode(payload)
        self.tx.append({"source": source, "target": address, "command": command.decode("ascii").strip(),
                        "frame": payload.hex()})
        if self.air_delay:
            # Deterministic air delay is a bounded condition wait, not a blind sleep.
            gate = threading.Event()
            if not gate.wait(self.air_delay):
                pass
        mode = self.ack_mode[source]
        if mode == "ok":
            self.devices[source].inject(b"+OK\r\n")
        elif mode == "err":
            self.devices[source].inject(b"+ERR=busy\r\n")
        peer = self.devices.get(address)
        if peer is not None:
            encoded = base64.b64encode(payload)
            peer.inject(b"+RCV=" + str(source).encode() + b"," + str(len(encoded)).encode() +
                        b"," + encoded + b",-61,9.5\r\n")


class FakeSerial:
    def __init__(self, air, address):
        self.air, self.address = air, address
        self.cv = threading.Condition()
        self.rx = bytearray()
        self.is_open = True
        self.short_write = False
        self.fail_read = False
        self.writes = []
        self.tx_buffer = bytearray()

    def inject(self, data):
        with self.cv:
            self.rx.extend(data)
            self.cv.notify_all()

    def write(self, data):
        if not self.is_open:
            raise OSError("fake serial disconnected")
        raw = bytes(data)
        if self.short_write:
            self.short_write = False
            written = max(1, len(raw) // 2)
        else:
            written = len(raw)
        part = raw[:written]
        self.writes.append(part)
        self.tx_buffer.extend(part)
        while b"\n" in self.tx_buffer:
            stop = self.tx_buffer.index(b"\n") + 1
            command = bytes(self.tx_buffer[:stop])
            del self.tx_buffer[:stop]
            self.air.deliver(self.address, command)
        return written

    def flush(self):
        if not self.is_open:
            raise OSError("fake serial disconnected")

    def read(self, size):
        with self.cv:
            if not self.rx and self.is_open:
                self.cv.wait(0.03)
            if self.fail_read:
                raise OSError("injected fake serial disconnect")
            data = bytes(self.rx[:size])
            del self.rx[:size]
            return data

    def close(self):
        self.is_open = False
        with self.cv:
            self.cv.notify_all()
        self.air.closed.add(self.address)


class Pulse:
    """Lets a test advance one real Pi service iteration at a time."""
    def __init__(self, clock):
        self.clock = clock
        self.request = threading.Event()
        self.reached = threading.Event()
        self.waiting = threading.Event()
        self.stop = threading.Event()
        self.count = 0

    def sleep(self, seconds):
        if self.stop.is_set():
            return
        self.reached.clear()
        self.waiting.set()
        if not self.request.wait(TIMEOUT):
            self.stop.set()
            raise AssertionError("bounded Pi service pulse timed out")
        self.request.clear()
        self.waiting.clear()
        self.clock.advance(round(seconds * 1000))
        self.count += 1
        self.reached.set()

    def tick(self):
        self.reached.clear()
        self.waiting.clear()
        self.request.set()
        if not self.reached.wait(TIMEOUT):
            raise AssertionError("Pi service did not finish bounded tick")
        if not self.waiting.wait(TIMEOUT):
            raise AssertionError("Pi service did not return to its pulse wait")


class DirectRadioRig:
    def __init__(self):
        self.air = FakeRadioAir()
        self.rig = IntegratedRig(boot_id=0x12345678,
            service_config=ServiceConfig(comms_enabled=False, poll_interval_ms=20))
        self.clock = self.rig.clock
        args = SimpleNamespace(port="pi-fake", baud=115200, local_address=1,
            peer_address=2, network_id=18, band_hz=915000000, parameter="10,7,1,12")
        self.pi = _make_radio_transport(args, transport_factory=lambda *a, **kw:
            LoRaModemTransport(*a, **kw, serial_factory=self.air.factory,
                command_timeout_s=0.3, queue_limit=4))
        self.ground = LoRaModemTransport("laptop-fake", local_address=2,
            peer_address=1, network_id=18, band_hz=915000000,
            parameter="10,7,1,12", serial_factory=self.air.factory,
            command_timeout_s=0.3, queue_limit=4)
        self.pulse = Pulse(self.clock)
        self.service_error = []
        self.service = threading.Thread(target=self._service, daemon=True)
        self.client = None
        self.events = []

    def __getattr__(self, name):
        # Keep assertions readable while delegating motor/router evidence to the
        # existing compiled-motor harness used by the original scenarios.
        return getattr(self.rig, name)

    def _service(self):
        try:
            _run_radio_service(self.rig.router, self.pi, lambda: self.pulse.stop.is_set(),
                               sleep=self.pulse.sleep)
        except BaseException as exc:
            self.service_error.append(exc)

    def start(self):
        self.service.start()
        wait_for(lambda: self.pi.is_connected, "Pi modem did not finish AT configuration")
        self.client = BoatClient(self.ground, clock=self.clock)
        self.client.subscribe(self.events.append)
        self.client.connect()
        self.pump(80)
        wait_for(lambda: self.client._reconciled,
                 "actual modem/router/motor path did not reconcile motor health/status")

    def pump(self, count=1):
        for _ in range(count):
            if self.service_error:
                raise self.service_error[0]
            self.pulse.tick()

    def settle_ground_rx(self):
        """Wait for queued paired-modem lines and callbacks to drain."""
        serial = self.air.devices[2]
        wait_for(lambda: not serial.rx, "ground fake modem RX did not drain")
        if not self.ground._callback_lock.acquire(timeout=TIMEOUT):
            raise AssertionError("ground transport callback did not finish")
        self.ground._callback_lock.release()

    def close(self):
        self.pulse.stop.set()
        self.pulse.request.set()
        if self.service.is_alive():
            self.service.join(TIMEOUT)
        if self.service.is_alive():
            self.pi.close()
            self.service.join(TIMEOUT)
        self.ground.close()
        self.rig.cleanup()


def _eventually(rig, pred, message, ticks=80):
    for _ in range(ticks):
        if pred():
            return
        rig.pump(1)
    raise AssertionError(message)


def run_direct_route():
    rig = DirectRadioRig()
    try:
        rig.start()
        assert rig.pi.local_address == 1 and rig.pi.peer_address == 2
        assert rig.ground.local_address == 2 and rig.ground.peer_address == 1
        assert rig.pi.stats["config_commands"] == rig.ground.stats["config_commands"] == 4
        setup_lines = {1: rig.air.devices[1].writes, 2: rig.air.devices[2].writes}
        for addr, expected in ((1, b"AT+ADDRESS=1\r\n"), (2, b"AT+ADDRESS=2\r\n")):
            assert expected in setup_lines[addr]
            assert b"AT+NETWORKID=18\r\n" in setup_lines[addr]
            assert b"AT+BAND=915000000\r\n" in setup_lines[addr]
            assert b"AT+PARAMETER=10,7,1,12\r\n" in setup_lines[addr]

        rig.settle_ground_rx()
        absolute = rig.client.set_absolute_angle(1, 1200, timeout_ms=1000, max_rate_cdeg_s=1000)
        _eventually(rig, lambda: rig.lib.scenario_motor_enabled(rig.motor, 0) == 1,
                    "absolute request never enabled compiled motor output")
        assert rig.lib.scenario_motor_target(rig.motor, 0) == 1200
        rig.step_encoder(1, 1200)
        _eventually(rig, lambda: any(isinstance(e, RequestOutcome) and
            e.request_id == absolute.request_id and e.state in (4, 5, 6) for e in rig.events),
            "actual terminal REQUEST_RESULT did not return to BoatClient")
        _eventually(rig, lambda: rig.lib.scenario_motor_enabled(rig.motor, 0) == 0,
                    "compiled motor output did not stop at measured target")
        outcome = next(e for e in rig.events if isinstance(e, RequestOutcome) and
                       e.request_id == absolute.request_id and e.state in (4, 5, 6))
        assert outcome.ground_session == absolute.ground_session
        snapshot = (__import__("ctypes").c_uint8 * 64)()
        rig.lib.scenario_motor_snapshot(rig.motor, 0, snapshot)
        raw = bytes(snapshot)
        motor_status = codec.decode(raw[:12 + raw[1]])
        assert motor_status["name"] == "ACTUATOR_STATUS"
        assert motor_status["fields"]["measured_position_cdeg"] == 1200, motor_status
        assert motor_status["fields"]["enabled"] == 0

        rig.settle_ground_rx()
        step = rig.client.step(1, 300, timeout_ms=1000, max_rate_cdeg_s=900)
        _eventually(rig, lambda: rig.lib.scenario_motor_target(rig.motor, 0) == 1500,
                    "STEP did not resolve from independently measured position")
        rig.step_encoder(1, 1500)
        _eventually(rig, lambda: any(isinstance(e, RequestOutcome) and
            e.request_id == step.request_id and e.state in (4, 5, 6) for e in rig.events),
            "STEP terminal outcome was not correlated")
        rig.lib.scenario_motor_snapshot(rig.motor, 0, snapshot)
        raw = bytes(snapshot)
        motor_status = codec.decode(raw[:12 + raw[1]])
        assert motor_status["fields"]["measured_position_cdeg"] == 1500, motor_status
        assert motor_status["fields"]["enabled"] == 0

        before_writes = sum(op["op"] == "write" for op in rig.bus.operations)
        invalid_fields = {"request_id": 77, "ground_session": rig.client.ground_session,
            "delta_cdeg": 200, "timeout_ms": 1000, "max_rate_cdeg_s": 500,
            "target_system": 1, "target_component": 154, "actuator": 1,
            "opcode": 2, "control_source": 3}
        rig.ground.send_frame(codec.encode("ACTUATOR_REQUEST", invalid_fields,
                                           system=42, component=190))
        bad_target = dict(invalid_fields, request_id=78, target_system=99)
        rig.ground.send_frame(codec.encode("ACTUATOR_REQUEST", bad_target,
                                           system=255, component=190))
        for _ in range(3):
            rig.pump(1)
        after_writes = sum(op["op"] == "write" for op in rig.bus.operations)
        assert after_writes == before_writes, "wrong source/target reached motor I2C write"

        rig.settle_ground_rx()
        rig.client.ping_all()
        _eventually(rig, lambda: any(isinstance(e, PongReply) and e.system == 1 and
            e.component == 1 for e in rig.events), "Pi ping reply missing")
        _eventually(rig, lambda: any(isinstance(e, PongReply) and e.system == 1 and
            e.component == 154 for e in rig.events), "motor ping reply missing")
        pongs = [e for e in rig.events if isinstance(e, PongReply)]
        assert all(e.rtt_ms is not None for e in pongs)
        assert all(e.component != 158 for e in pongs)
        assert not any(op["address"] == 0x09 for op in rig.bus.operations)
        assert {1, 2} == {entry["source"] for entry in rig.air.tx}
        rig.clock.advance(2100)
        try:
            rig.client.step(1, 100, timeout_ms=500, max_rate_cdeg_s=500)
        except RuntimeError as exc:
            assert "fresh motor health and status" in str(exc)
        else:
            raise AssertionError("stale motor query evidence did not block movement")
        return {"status": "PASS", "checks": ["actual modem AT setup both directions",
            "absolute motion and returned correlated result", "STEP from measured state",
            "wrong source/target blocked before motor I2C", "Pi and motor ping; no comms 0x09 poll",
            "stale motor health/status blocks movement"],
            "requests": [absolute.request_id, step.request_id],
            "outcome": {"request_id": outcome.request_id, "state": outcome.state,
                        "result": outcome.result, "source": list(outcome.source)},
            "measured_cdeg": 1500,
            "pongs": sorted([[e.system, e.component] for e in pongs]),
            "bus_addresses": sorted({o["address"] for o in rig.bus.operations}),
            "at_commands": {str(k): [v.decode("ascii").strip() for v in vals
                            if v.startswith((b"AT+ADDRESS=", b"AT+NETWORKID=", b"AT+BAND=",
                                             b"AT+PARAMETER="))]
                            for k, vals in setup_lines.items()}}
    finally:
        rig.close()


def run_validation_rejection():
    # The actual transport parser rejects wrong peer, corrupt CRC and malformed
    # +RCV envelopes before they reach the actual Pi router callback.
    air = FakeRadioAir()
    transport = LoRaModemTransport("laptop-fake", local_address=2, peer_address=1,
        serial_factory=air.factory, command_timeout_s=0.2)
    received = []
    try:
        transport.open(received.append)
        frame = codec.encode("PING", {"time_usec": 4, "seq": 1,
            "target_system": 0, "target_component": 0}, system=255, component=190)
        encoded = base64.b64encode(frame)
        serial = air.devices[2]
        serial.inject(b"+RCV=9," + str(len(encoded)).encode() + b"," + encoded + b",-60,9\r\n")
        damaged = bytearray(frame); damaged[-1] ^= 1
        enc_bad = base64.b64encode(damaged)
        serial.inject(b"+RCV=1," + str(len(enc_bad)).encode() + b"," + enc_bad + b",-60,9\r\n")
        serial.inject(b"+RCV=1,3,***,-60,9\r\n")
        wait_for(lambda: transport.stats["rx_rejected"] >= 3,
                 "wrong source/CRC/malformed RCV were not rejected")
        assert received == [] and transport.stats["rx_frames"] == 0
        return {"status": "PASS", "rejected": transport.stats["rx_rejected"],
                "delivered": len(received)}
    finally:
        transport.close()


def _ping(seq):
    return codec.encode("PING", {"time_usec": seq + 100, "seq": seq,
        "target_system": 0, "target_component": 0}, system=255, component=190)


def run_transport_reliability():
    air = FakeRadioAir()
    ground = LoRaModemTransport("laptop-fake", local_address=2, peer_address=1,
        serial_factory=air.factory, command_timeout_s=0.12, queue_limit=2)
    pi = LoRaModemTransport("pi-fake", local_address=1, peer_address=2,
        serial_factory=air.factory, command_timeout_s=0.12, queue_limit=2)
    received = []
    errors = []
    try:
        pi.open(received.append); ground.open(received.append)
        ground_serial = air.devices[2]
        ground_serial.short_write = True
        ground.send_frame(_ping(1))
        assert sum(tx["frame"] == _ping(1).hex() for tx in air.tx) == 1

        # Hold one real transport command at the fake modem ACK boundary and
        # ensure reconnect never retries its uncertain payload.
        air.ack_mode[2] = "hold"
        ambiguous = _ping(2)
        thread = threading.Thread(target=lambda: _capture_send(ground, ambiguous, errors), daemon=True)
        thread.start()
        wait_for(lambda: any(tx["frame"] == ambiguous.hex() for tx in air.tx),
                 "ambiguous frame never reached fake modem")
        thread.join(1.0)
        assert errors and isinstance(errors[-1], TimeoutError), errors
        ground.close()
        air.ack_mode[2] = "ok"
        ground.open(received.append)
        ground.send_frame(_ping(3))
        assert sum(tx["frame"] == ambiguous.hex() for tx in air.tx) == 1
        assert sum(tx["frame"] == _ping(3).hex() for tx in air.tx) == 1

        air.ack_mode[2] = "err"
        rejected = _ping(6)
        _capture_send(ground, rejected, errors)
        assert isinstance(errors[-1], OSError), errors
        assert sum(tx["frame"] == rejected.hex() for tx in air.tx) == 1
        ground.close()
        air.ack_mode[2] = "ok"
        ground.open(received.append)

        # Reader failure during a waiting send closes the generation and also
        # leaves the delivered frame unreplayed after reconnect.
        air.ack_mode[2] = "hold"
        disconnected = _ping(4)
        thread = threading.Thread(target=lambda: _capture_send(ground, disconnected, errors), daemon=True)
        thread.start()
        wait_for(lambda: any(tx["frame"] == disconnected.hex() for tx in air.tx),
                 "disconnect fixture frame did not transmit")
        air.devices[2].fail_read = True
        air.devices[2].inject(b"")
        wait_for(lambda: not ground.is_connected, "reader disconnect did not close transport")
        thread.join(1.0)
        ground.close()
        air.devices[2].fail_read = False
        air.ack_mode[2] = "ok"
        ground.open(received.append)
        ground.send_frame(_ping(5))
        assert sum(tx["frame"] == disconnected.hex() for tx in air.tx) == 1
        assert sum(tx["frame"] == _ping(5).hex() for tx in air.tx) == 1
        return {"status": "PASS", "short_write_commands": 1,
                "ambiguous_timeout_transmissions": 1, "disconnect_transmissions": 1,
                "reconnect_new_transmission": True,
                "errors": [type(err).__name__ for err in errors]}
    finally:
        ground.close(); pi.close()


def _capture_send(transport, frame, errors):
    try:
        transport.send_frame(frame)
    except BaseException as exc:
        errors.append(exc)


def _request_frame(request_id, opcode):
    return codec.encode("ACTUATOR_REQUEST", {
        "request_id": request_id, "ground_session": 0x1234, "delta_cdeg": 100,
        "timeout_ms": 1000, "max_rate_cdeg_s": 500, "target_system": 1,
        "target_component": 154, "actuator": 1, "opcode": opcode,
        "control_source": 3}, system=255, component=190)


def run_stop_queue_priority():
    air = FakeRadioAir()
    ground = LoRaModemTransport("laptop-fake", local_address=2, peer_address=1,
        serial_factory=air.factory, command_timeout_s=1.0, queue_limit=2)
    pi = LoRaModemTransport("pi-fake", local_address=1, peer_address=2,
        serial_factory=air.factory, command_timeout_s=1.0, queue_limit=2)
    errors = {"inflight": [], "move1": [], "move2": [], "stop": []}
    frames = {"inflight": _ping(10), "move1": _request_frame(11, 2),
              "move2": _request_frame(12, 1), "stop": _request_frame(13, 3)}
    threads = {}
    try:
        pi.open(lambda _frame: None); ground.open(lambda _frame: None)
        air.ack_mode[2] = "hold"
        threads["inflight"] = threading.Thread(target=lambda: _capture_send(
            ground, frames["inflight"], errors["inflight"]), daemon=True)
        threads["inflight"].start()
        wait_for(lambda: ground._pending is not None, "first send did not block at ACK")
        for name in ("move1", "move2"):
            threads[name] = threading.Thread(target=lambda n=name: _capture_send(
                ground, frames[n], errors[n]), daemon=True)
            threads[name].start()
            wait_for(lambda: len(ground._queue) >= (1 if name == "move1" else 2),
                     f"{name} did not queue behind blocked modem ACK")
        threads["stop"] = threading.Thread(target=lambda: _capture_send(
            ground, frames["stop"], errors["stop"]), daemon=True)
        threads["stop"].start()
        stop_command = codec.radio_command(frames["stop"], address=1)
        wait_for(lambda: any(isinstance(e, BufferError) for e in errors["move2"])
                 or any(item.command == stop_command for item in ground._queue),
                 "STOP did not enter bounded send queue")
        assert len(ground._queue) <= 2
        wait_for(lambda: bool(errors["move2"]), "displaced movement caller was not released")
        assert len(errors["move2"]) == 1 and isinstance(errors["move2"][0], BufferError), (
            f"move2 outcome={errors['move2']!r}, queued={[item.command for item in ground._queue]!r}")
        air.ack_mode[2] = "ok"
        air.devices[2].inject(b"+OK\r\n")
        for thread in threads.values():
            thread.join(2.0)
            assert not thread.is_alive(), "queued transport send did not finish"
        sent = [bytes.fromhex(tx["frame"]) for tx in air.tx]
        queued_requests = [codec.decode(frame)["fields"]["request_id"]
                           for frame in sent if codec.decode(frame)["name"] == "ACTUATOR_REQUEST"]
        assert queued_requests == [13, 11], queued_requests
        assert errors["inflight"] == errors["move1"] == errors["stop"] == []
        return {"status": "PASS", "queue_limit": 2, "high_water": 2,
                "queued_request_order": queued_requests, "displaced_request": 12,
                "stop_before_queued_movement": queued_requests.index(13) < queued_requests.index(11)}
    finally:
        ground.close(); pi.close()


def main():
    results = {"direct_route": run_direct_route(),
               "validation_rejection": run_validation_rejection(),
               "transport_reliability": run_transport_reliability(),
               "stop_queue_priority": run_stop_queue_priority()}
    print(json.dumps(results, sort_keys=True, separators=(",", ":")))


if __name__ == "__main__":
    main()
