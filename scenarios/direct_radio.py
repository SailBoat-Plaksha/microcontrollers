"""Direct Laptop/Pi RYLR998 route scenarios with paired fake serial modems.

The fixture models only the physical modem boundary: serial AT commands,
acknowledgements, and the matching +RCV line delivered to the paired endpoint.
BoatClient, LoRaModemTransport, the Pi radio service/router, and the compiled
motor adapter remain the production implementations.
"""
from __future__ import annotations

import base64
from collections import deque
import argparse
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
        self.rf_deliveries = []
        self.injections = {1: [], 2: []}
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
        mode = self.ack_mode[source]
        if mode == "ok":
            self.devices[source].inject(b"+OK\r\n")
        elif mode == "err":
            self.devices[source].inject(b"+ERR=busy\r\n")
            return
        peer = self.devices.get(address)
        if peer is not None:
            encoded = base64.b64encode(payload)
            self.rf_deliveries.append({"source": source, "target": address,
                                       "frame": payload.hex()})
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
        self.air.injections[self.address].append(bytes(data))
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
    def __init__(self, *, rig=None):
        self.air = FakeRadioAir()
        self.rig = rig if rig is not None else IntegratedRig(boot_id=0x12345678,
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
        self.ground_delivery_cv = threading.Condition()
        self.ground_completed_deliveries = 0
        self.ground_rf_baseline = 0
        self.ground_callback_baseline = 0
        original_open = self.ground.open

        def open_with_completion(callback):
            def completed_callback(frame):
                try:
                    callback(frame)
                finally:
                    with self.ground_delivery_cv:
                        self.ground_completed_deliveries += 1
                        self.ground_delivery_cv.notify_all()
            return original_open(completed_callback)

        # Fixture-owned completion barrier, independent of transport internals.
        self.ground.open = open_with_completion
        self.pulse = Pulse(self.clock)
        self.service_error = []
        self.radio_tx_cv = threading.Condition()
        self.radio_tx_pending = 0
        self.service = threading.Thread(target=self._service, daemon=True)
        self.client = None
        self.events = []

    def __getattr__(self, name):
        # Keep assertions readable while delegating motor/router evidence to the
        # existing compiled-motor harness used by the original scenarios.
        return getattr(self.rig, name)

    def _service(self):
        import onboard.__main__ as onboard_main

        enqueue_radio = onboard_main._enqueue_radio
        send_frame = self.pi.send_frame
        fixture = self

        class TrackedQueueItems(deque):
            # Count actual insertions, including eviction, rather than enqueue
            # calls: coalescing replaces a frame without adding writer work.
            def insert(self, index, value):
                super().insert(index, value)
                fixture.radio_tx_pending += 1

            def __delitem__(self, index):
                super().__delitem__(index)
                fixture.radio_tx_pending -= 1

        def tracked_enqueue(outgoing, frame):
            with self.radio_tx_cv:
                with outgoing.mutex:
                    if not isinstance(outgoing.queue, TrackedQueueItems):
                        self.radio_tx_pending += len(outgoing.queue)
                        outgoing.queue = TrackedQueueItems(outgoing.queue)
                accepted = enqueue_radio(outgoing, frame)
                self.radio_tx_cv.notify_all()
                return accepted

        def tracked_send(frame):
            try:
                return send_frame(frame)
            finally:
                with self.radio_tx_cv:
                    self.radio_tx_pending -= 1
                    self.radio_tx_cv.notify_all()

        onboard_main._enqueue_radio = tracked_enqueue
        self.pi.send_frame = tracked_send
        try:
            _run_radio_service(self.rig.router, self.pi, lambda: self.pulse.stop.is_set(),
                               sleep=self.pulse.sleep)
        except BaseException as exc:
            self.service_error.append(exc)
        finally:
            onboard_main._enqueue_radio = enqueue_radio
            self.pi.send_frame = send_frame

    def start(self, *, reconcile_motor=True):
        self.service.start()
        wait_for(lambda: self.pi.is_connected, "Pi modem did not finish AT configuration")
        self.client = BoatClient(self.ground, clock=self.clock)
        self.client.subscribe(self.events.append)
        self.client.connect()
        self.ground_rf_baseline = len(self.air.rf_deliveries)
        self.ground_callback_baseline = self.ground_completed_deliveries
        if not reconcile_motor:
            return
        _eventually(self, lambda: self.client._reconciled,
                    f"actual modem/router/motor path did not reconcile motor health/status; "
                    f"clock={self.clock.now}, events={self.events!r}, "
                    f"tx={self.air.tx!r}, pi_stats={self.pi.stats!r}, "
                    f"ground_stats={self.ground.stats!r}, service_error={self.service_error!r}")

    def pump(self, count=1):
        for _ in range(count):
            if self.service_error:
                raise self.service_error[0]
            self.pulse.tick()
            end = time.monotonic() + TIMEOUT
            with self.radio_tx_cv:
                while self.radio_tx_pending:
                    remaining = end - time.monotonic()
                    if remaining <= 0:
                        raise AssertionError(
                            f"Pi radio writer did not drain after scheduler tick: {self.radio_tx_pending} pending")
                    self.radio_tx_cv.wait(remaining)
            self.settle_ground_rx()

    def settle_ground_rx(self):
        """Wait until each injected RF frame completed BoatClient dispatch."""
        expected = (self.ground_callback_baseline +
                    sum(item["target"] == 2 for item in
                        self.air.rf_deliveries[self.ground_rf_baseline:]))
        end = time.monotonic() + TIMEOUT
        with self.ground_delivery_cv:
            while self.ground_completed_deliveries < expected:
                remaining = end - time.monotonic()
                if remaining <= 0:
                    raise AssertionError(
                        f"ground callback barrier timed out: {self.ground_completed_deliveries}/"
                        f"expected={expected}")
                self.ground_delivery_cv.wait(remaining)

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
            e.request_id == absolute.request_id and e.state == 4 and e.result == 1 for e in rig.events),
            "actual terminal REQUEST_RESULT did not return to BoatClient")
        _eventually(rig, lambda: rig.lib.scenario_motor_enabled(rig.motor, 0) == 0,
                    "compiled motor output did not stop at measured target")
        outcome = next(e for e in rig.events if isinstance(e, RequestOutcome) and
                       e.request_id == absolute.request_id and e.state == 4 and e.result == 1)
        assert outcome.state == 4 and outcome.result == 1, outcome
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
            e.request_id == step.request_id and e.state == 4 and e.result == 1 for e in rig.events),
            "STEP terminal outcome was not correlated")
        step_outcome = next(e for e in rig.events if isinstance(e, RequestOutcome) and
                            e.request_id == step.request_id and e.state == 4 and e.result == 1)
        assert step_outcome.state == 4 and step_outcome.result == 1, step_outcome
        rig.lib.scenario_motor_snapshot(rig.motor, 0, snapshot)
        raw = bytes(snapshot)
        motor_status = codec.decode(raw[:12 + raw[1]])
        assert motor_status["fields"]["measured_position_cdeg"] == 1500, motor_status
        assert motor_status["fields"]["enabled"] == 0

        step_frames = [bytes.fromhex(entry["frame"]) for entry in rig.air.rf_deliveries
                       if entry["source"] == 2 and
                       codec.decode(bytes.fromhex(entry["frame"]))["name"] == "ACTUATOR_REQUEST" and
                       codec.decode(bytes.fromhex(entry["frame"]))["fields"]["request_id"] == step.request_id]
        assert len(step_frames) == 1, len(step_frames)
        duplicate = step_frames[0]
        before_duplicate_writes = sum(op["op"] == "write" for op in rig.bus.operations)
        before_duplicate_target = rig.lib.scenario_motor_target(rig.motor, 0)
        rig.ground.send_frame(duplicate)
        wait_for(lambda: sum(entry["source"] == 2 and entry["frame"] == duplicate.hex()
                             for entry in rig.air.rf_deliveries) == 2,
                 "duplicate STEP was not delivered through paired modems")
        for _ in range(4):
            rig.pump(1)
        after_duplicate_writes = sum(op["op"] == "write" for op in rig.bus.operations)
        assert after_duplicate_writes == before_duplicate_writes, "duplicate STEP caused another motor I2C write"
        assert rig.lib.scenario_motor_target(rig.motor, 0) == before_duplicate_target

        active = rig.client.set_absolute_angle(1, 2000, timeout_ms=1000, max_rate_cdeg_s=500)
        _eventually(rig, lambda: rig.lib.scenario_motor_enabled(rig.motor, 0) == 1 and
                    rig.lib.scenario_motor_target(rig.motor, 0) == 2000,
                    "active request did not establish compiled motor output before STOP")
        stop = rig.client.stop(1)
        _eventually(rig, lambda: any(isinstance(e, RequestOutcome) and
            e.request_id == active.request_id and e.state == 5 for e in rig.events),
            f"active request did not report STOPPED after actual STOP path: "
            f"{[e for e in rig.events if isinstance(e, RequestOutcome) and e.request_id in (active.request_id, stop.request_id)]!r}; "
            f"clock={rig.clock.now}, air_tx={rig.air.tx!r}, service_error={rig.service_error!r}, "
            f"bus_tail={rig.bus.operations[-12:]!r}")
        _eventually(rig, lambda: any(isinstance(e, RequestOutcome) and
            e.request_id == stop.request_id and e.state == 5 for e in rig.events),
            "STOP request did not report STOPPED through actual Pi/motor path")
        _eventually(rig, lambda: rig.lib.scenario_motor_enabled(rig.motor, 0) == 0,
                    "STOP did not turn off compiled motor output")
        active_outcome = next(e for e in rig.events if isinstance(e, RequestOutcome) and
                              e.request_id == active.request_id and e.state == 5)
        stop_outcome = next(e for e in rig.events if isinstance(e, RequestOutcome) and
                            e.request_id == stop.request_id and e.state == 5)
        assert active_outcome.state == 5 and stop_outcome.state == 5

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
            "absolute motion and completed correlated result", "STEP from measured state and duplicate suppression",
            "active-output STOP through BoatClient, paired modems, Pi router and compiled motor",
            "wrong source/target blocked before motor I2C", "Pi and motor ping; no comms 0x09 poll",
            "stale motor health/status blocks movement"],
            "requests": [absolute.request_id, step.request_id, active.request_id, stop.request_id],
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
        assert not any(item["frame"] == rejected.hex() for item in air.rf_deliveries), (
            "known local +ERR was incorrectly modeled as RF delivery")
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


def run_receive_before_ack():
    """Exercise real reader and callback threads with peer RCV before local ACK."""
    air = FakeRadioAir()
    ground = LoRaModemTransport("laptop-fake", local_address=2, peer_address=1,
        serial_factory=air.factory, command_timeout_s=0.5, queue_limit=4)
    pi = LoRaModemTransport("pi-fake", local_address=1, peer_address=2,
        serial_factory=air.factory, command_timeout_s=0.5, queue_limit=4)
    callback_done = threading.Event()
    callback_errors = []
    send_errors = []
    ground_received = []

    def respond(_frame):
        try:
            pi.send_frame(_ping(91))
        except BaseException as exc:
            callback_errors.append(exc)
        finally:
            callback_done.set()

    def receive_ground(frame):
        ground_received.append(bytes(frame))

    try:
        pi.open(respond)
        ground.open(receive_ground)
        air.injections[2].clear()
        air.ack_mode[2] = "hold"
        outgoing = _ping(90)
        sender = threading.Thread(target=lambda: _capture_send(ground, outgoing, send_errors),
                                  daemon=True)
        sender.start()
        wait_for(callback_done.is_set,
                 "paired Pi callback did not return a real modem response before ground ACK")
        wait_for(lambda: bool(ground_received),
                 "ground reader/callback did not dispatch peer +RCV before local +OK")
        observed = air.injections[2]
        assert observed and observed[0].startswith(b"+RCV="), observed
        assert not any(line.startswith(b"+OK") for line in observed)
        assert sender.is_alive(), "held local modem ACK did not keep send pending"
        air.devices[2].inject(b"+OK\r\n")
        sender.join(TIMEOUT)
        assert not sender.is_alive(), "ground send did not complete after delayed +OK"
        assert send_errors == [] and callback_errors == [], (send_errors, callback_errors)
        assert ground_received == [_ping(91)], ground_received
        assert sum(item["frame"] == outgoing.hex() for item in air.rf_deliveries) == 1
        assert sum(item["frame"] == _ping(91).hex() for item in air.rf_deliveries) == 1
        return {"status": "PASS", "ground_injection_order": [
                    "peer +RCV", "local +OK"], "request_transmissions": 1,
                "response_transmissions": 1, "send_errors": [], "callback_errors": []}
    finally:
        ground.close(); pi.close()


def _request_frame(request_id, opcode):
    return codec.encode("ACTUATOR_REQUEST", {
        "request_id": request_id, "ground_session": 0x1234, "delta_cdeg": 0 if opcode == 3 else 100,
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
    parser = argparse.ArgumentParser(description="Direct paired-modem host scenarios")
    parser.add_argument("--json", metavar="PATH", help="also write JSON report to PATH")
    args = parser.parse_args()
    results = {"direct_route": run_direct_route(),
               "validation_rejection": run_validation_rejection(),
               "receive_before_ack": run_receive_before_ack(),
               "transport_reliability": run_transport_reliability(),
               "stop_queue_priority": run_stop_queue_priority()}
    report = json.dumps(results, sort_keys=True, separators=(",", ":"))
    if args.json:
        with open(args.json, "w", encoding="utf-8") as output:
            output.write(report + "\n")
    print(report)


if __name__ == "__main__":
    main()
