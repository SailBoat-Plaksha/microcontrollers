"""End-to-end host scenarios for the real LoRa and SSH software paths."""
from __future__ import annotations

import ctypes
import importlib
import io
import binascii
import struct
import os
import queue
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import types
from pathlib import Path
from unittest.mock import patch

from boat.ids import COMPONENT_GROUND, COMPONENT_MOTOR, SYSTEM_GROUND
from protocol import codec
from scenarios.integrated import IntegratedRig, MAILBOX, _load_motor

ROOT = Path(__file__).resolve().parents[1]


def _load_relay():
    cc = shutil.which("cc")
    if not cc:
        raise RuntimeError("cc compiler required for comms relay scenarios")
    tmp = tempfile.TemporaryDirectory(prefix="boat-scenarios-relay-")
    so = Path(tmp.name) / "libscenario_relay.so"
    subprocess.run([cc, "-std=c11", "-shared", "-fPIC", "-Wall", "-Wextra", "-Werror",
        "-I" + str(ROOT / "comms-mcu/include"), "-I" + str(ROOT / "protocol"),
        "-I" + str(ROOT / "protocol/gen/C"),
        "-I" + str(ROOT / "protocol/gen/C/arsem_standalone"),
        str(ROOT / "comms-mcu/relay.c"), str(ROOT / "comms-mcu/modem.c"),
        str(ROOT / "scenarios/relay_ctypes.c"), "-o", str(so)],
        check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(so))
    lib.scenario_relay_init.argtypes = [ctypes.c_uint32]
    lib.scenario_relay_radio.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
    lib.scenario_relay_radio.restype = ctypes.c_int
    lib.scenario_relay_modem_line.argtypes = [ctypes.c_char_p, ctypes.c_uint16]
    lib.scenario_relay_modem_line.restype = ctypes.c_int
    lib.scenario_modem_feed.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
    lib.scenario_modem_pop.argtypes = [ctypes.c_char_p]
    lib.scenario_modem_pop.restype = ctypes.c_int
    lib.scenario_relay_i2c_write.argtypes = [ctypes.POINTER(ctypes.c_uint8)]
    lib.scenario_relay_i2c_write.restype = ctypes.c_int
    lib.scenario_relay_i2c_read.argtypes = [ctypes.POINTER(ctypes.c_uint8)]
    lib.scenario_relay_i2c_read.restype = ctypes.c_int
    lib.scenario_relay_radio_next.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_size_t)]
    lib.scenario_relay_radio_complete.argtypes = [ctypes.c_int]
    lib.scenario_relay_radio_next.restype = ctypes.c_int
    lib.scenario_modem_build.argtypes = [ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                         ctypes.c_char_p, ctypes.c_size_t]
    lib.scenario_modem_build.restype = ctypes.c_int
    for name in ("scenario_relay_forwarded_from_radio", "scenario_relay_forwarded_from_pi",
                 "scenario_relay_malformed", "scenario_relay_drops"):
        getattr(lib, name).restype = ctypes.c_uint
    return tmp, lib


def _bytes_arg(value: bytes):
    return (ctypes.c_uint8 * len(value)).from_buffer_copy(value)


def _actual_gateway():
    # The deployed MicroPython gateway imports ubinascii. CPython's binascii has
    # compatible base64 APIs for this test-only hardware-free import.
    import binascii
    alias = types.ModuleType("ubinascii")
    alias.b2a_base64 = binascii.b2a_base64
    alias.a2b_base64 = binascii.a2b_base64
    sys.modules["ubinascii"] = alias
    path = str(ROOT / "ground-gateway")
    sys.path.insert(0, path)
    try:
        for name in ("config", "dialect", "mavlink_stream", "gateway"):
            sys.modules.pop(name, None)
        module = importlib.import_module("gateway")
        class UART:
            def __init__(self): self.writes = []
            def write(self, data): self.writes.append(bytes(data)); return len(data)
        class USBOut:
            def __init__(self): self.data = bytearray(); self.max_chunk = 7; self.chunks = []
            def write(self, data):
                n = min(len(data), self.max_chunk)
                chunk=bytes(data[:n]); self.data.extend(chunk); self.chunks.append(chunk); return n
        uart, usb = UART(), USBOut()
        return module.Gateway(uart, usb), uart, usb
    finally:
        sys.path.remove(path)


class LoRaRig:
    """Actual gateway -> compiled modem/relay -> actual router -> motor chain."""
    def __init__(self, *, imu=None):
        self.tmp, self.relay = _load_relay()
        self.relay.scenario_relay_init(0x15801580)
        self.gateway, self.uart, self.usb = _actual_gateway()
        # Drive the actual nonblocking config TX/wait states. ACK only after
        # each complete AT line has crossed the UART boundary. This clock is
        # deliberately fixture-owned and passed into every gateway tick.
        self.gateway_now_ms = 0
        self.gateway_commands = []
        for expected in self.gateway.configs:
            start = len(self.uart.writes)
            for _ in range(100):
                self.gateway.tick(self.gateway_now_ms)
                self.gateway_now_ms += 1
                if self.gateway.modem_state == "config_wait":
                    break
            else:
                raise AssertionError(f"gateway did not finish writing modem config {expected!r}")
            actual = b"".join(self.uart.writes[start:]).strip()
            if actual != expected.encode():
                raise AssertionError(f"gateway config UART line {actual!r} != {expected!r}")
            self.gateway_commands.append(actual)
            self.gateway.feed_modem(b"+OK\r\n", self.gateway_now_ms)
        if not self.gateway.modem_ready:
            raise AssertionError("gateway did not reach modem_ready after four config ACKs")
        self.rig = IntegratedRig(imu=imu, boot_id=0x15401540)
        self.rig.router.boot_id=0xA0B0C0D0
        original_write, original_read = self.rig.bus.write_mailbox, self.rig.bus.read_mailbox
        self.bus_ops = []
        def write(address, data):
            if address == 0x09:
                raw = bytes(data)
                self.bus_ops.append({"op":"write","address":address,"hex":raw.hex()})
                result = self.relay.scenario_relay_i2c_write(_bytes_arg(raw))
                if result != 0: raise OSError(f"actual relay rejected Pi mailbox write result={result}")
            else: original_write(address, data)
        def read(address):
            if address == 0x09:
                out = (ctypes.c_uint8 * 64)()
                self.relay.scenario_relay_i2c_read(out)
                raw = bytes(out)
                self.bus_ops.append({"op":"read","address":address,"hex":raw.hex()})
                return raw
            return original_read(address)
        self.rig.bus.write_mailbox, self.rig.bus.read_mailbox = write, read
        # Actual I2C comms reply is converted to RYLR998 +RCV only at the virtual
        # modem boundary; relay and gateway perform all byte parsing/validation.
        self.trace = []
        self.radio_delivered = []
        self.usb_stream=codec.FrameStream()
        self.ground_frames=[]
        self.rig.router.on_frame=self._capture_router_frame
        self.rig.transport.send_frame=self._send_client_frame

    def _capture_router_frame(self, frame):
        message=codec.decode(frame)
        self.trace.append({"at":self.rig.clock.now,"direction":"router_frame",
            "name":message["name"],"source":[message["system"],message["component"]],
            "hex":bytes(frame).hex()})

    def _send_client_frame(self, frame):
        raw=bytes(frame); self.rig.transport.sent.append(raw); self.ground_frames.append(raw)
        via_pi=self.gateway_usb_to_radio(raw)
        self.last_radio_request=via_pi
        accepted=self.rig.router.submit_frame(via_pi,ingress="comms")
        self.trace.append({"at":self.rig.clock.now,"direction":"radio_to_router",
            "accepted":accepted,"hex":via_pi.hex()})
        return accepted

    def connect_and_reconcile(self):
        with patch("boat.client.secrets.randbits", return_value=0x50607080):
            self.rig.client.connect()
        for _ in range(80):
            self.rig.transport.pump(1)
            self.radio_delivered.extend(self.pi_to_gateway_usb())
            if self.rig.clock.now >= 11.1 and self.rig.client._reconciled:
                return
        raise AssertionError("actual LoRa path did not reconcile motor status and health")

    def gateway_usb_to_radio(self, frame: bytes):
        accepted = self.gateway.feed_usb(frame)
        if accepted != 1: raise AssertionError(f"production gateway accepted {accepted} USB frames")
        start = len(self.uart.writes)
        for _ in range(100):
            self.gateway.tick(self.gateway_now_ms)
            self.gateway_now_ms += 1
            if self.gateway.modem_state == "send_wait":
                break
        else:
            raise AssertionError(f"gateway did not finish writing queued AT+SEND; state={self.gateway.modem_state}")
        command = b"".join(self.uart.writes[start:]).strip()
        if not command.startswith(b"AT+SEND=1,"):
            raise AssertionError(f"gateway emitted unexpected UART data {command!r}")
        parts = command.split(b",", 2)
        encoded = parts[2]
        self.trace.append({"at":self.rig.clock.now,"direction":"gateway_to_modem","hex":command.hex()})
        # LoRa air and UART modem framing terminate at this injected physical
        # boundary. C modem parser consumes the production +RCV line.
        # The virtual modem ACK arrives only after the complete send command.
        self.gateway.feed_modem(b"+OK\r\n", self.gateway_now_ms)
        line = b"+RCV=2," + parts[1] + b"," + encoded + b",-70,8.0\r\n"
        result = self.relay.scenario_relay_modem_line(line, 2)
        self.trace.append({"at":self.rig.clock.now,"direction":"modem_to_relay","hex":line.hex(),"result":result})
        if result != 0: raise AssertionError(f"production modem/relay rejected valid radio frame: {result}")
        box = (ctypes.c_uint8 * 64)()
        self.relay.scenario_relay_i2c_read(box)
        raw = bytes(box)
        msglen = raw[1] + 12
        actual = raw[:msglen]
        self.trace.append({"at":self.rig.clock.now,"direction":"relay_to_pi","hex":actual.hex()})
        return actual

    def pi_to_gateway_usb(self):
        # Pull actual C relay outgoing frames, serialize with actual C modem,
        # then pass the generated +RCV line through production gateway parser.
        delivered = []
        while True:
            out = (ctypes.c_uint8 * 64)(); length = ctypes.c_size_t()
            if not self.relay.scenario_relay_radio_next(out, ctypes.byref(length)): break
            raw = bytes(out[:length.value])
            cmd = ctypes.create_string_buffer(300)
            if not self.relay.scenario_modem_build(2, _bytes_arg(raw), len(raw), cmd, len(cmd)):
                raise AssertionError("production modem failed to serialize outgoing relay frame")
            command = cmd.value
            encoded = command.split(b",", 2)[2]
            line = b"+RCV=1," + str(len(encoded)).encode() + b"," + encoded + b",-69,7.5\r\n"
            self.gateway.feed_modem(line, self.gateway_now_ms)
            # A terminal REQUEST_RESULT remains reserved/in-flight in the real
            # relay until the modem reports local acceptance. Complete that
            # transaction at the same explicit virtual modem boundary.
            if codec.decode(raw)["name"] == "REQUEST_RESULT":
                self.relay.scenario_relay_radio_complete(1)
            before = len(self.usb.chunks)
            for _ in range(100):
                self.gateway.tick(self.gateway_now_ms)
                self.gateway_now_ms += 1
                if not self.gateway.usb_queue: break
            chunks=self.usb.chunks[before:]
            piece=b"".join(chunks)
            for chunk in chunks:
                for decoded,_ in self.usb_stream.feed(chunk):
                    self.rig.transport.emit(decoded)
            # Small writes intentionally preserve the real stream parser's
            # fragmentation behavior in the caller.
            delivered.append((raw, piece))
            self.trace.append({"at":self.rig.clock.now,"direction":"relay_to_gateway_usb","hex":piece.hex()})
        return delivered

    def cleanup(self):
        self.rig.cleanup(); self.tmp.cleanup()


def run_lora_full_route():
    rig = LoRaRig()
    try:
        rig.connect_and_reconcile()
        ticket = rig.rig.client.set_absolute_angle(1, 900, timeout_ms=2000, max_rate_cdeg_s=1000)
        assert rig.ground_frames and codec.decode(rig.ground_frames[-1])["name"]=="ACTUATOR_REQUEST"
        assert codec.decode(rig.last_radio_request)["name"]=="ACTUATOR_REQUEST"
        for _ in range(8): rig.rig.transport.pump(1)
        assert rig.rig.lib.scenario_motor_active(rig.rig.motor, 0) == 1
        assert rig.rig.lib.scenario_motor_target(rig.rig.motor, 0) == 900
        # Completion uses separately injected feedback, and resulting actual
        # REQUEST_RESULT and status are forwarded through relay + gateway.
        rig.rig.step_encoder(1, 900)
        radio_frames = []
        # Drain the actual relay after each bounded router step. The relay now
        # holds one terminal result in flight until the virtual modem accepts
        # it; batching all polls before draining would only observe a coalesced
        # intermediate snapshot and would not model that transaction boundary.
        for _ in range(30):
            rig.rig.transport.pump(1)
            radio_frames.extend(rig.pi_to_gateway_usb())
            if (any(codec.decode(raw)["name"] == "REQUEST_RESULT" and
                    codec.decode(raw)["fields"]["request_id"] == ticket.request_id and
                    codec.decode(raw)["fields"]["state"] == 4 for raw, _ in radio_frames) and
                    any(codec.decode(raw)["name"] == "ACTUATOR_STATUS" for raw, _ in radio_frames)):
                break
        names = [codec.decode(raw)["name"] for raw, _ in radio_frames]
        assert "REQUEST_RESULT" in names and "ACTUATOR_STATUS" in names, names
        assert all(codec.decode(raw)["system"] == 1 for raw, _ in radio_frames)
        terminal=[codec.decode(raw)["fields"] for raw,_ in radio_frames
                  if codec.decode(raw)["name"]=="REQUEST_RESULT" and
                  codec.decode(raw)["fields"]["request_id"]==ticket.request_id and
                  codec.decode(raw)["fields"]["ground_session"]==ticket.ground_session]
        assert any(item["state"]==4 for item in terminal), terminal
        client_terminal=[e for e in rig.rig.events if type(e).__name__=="RequestOutcome" and
                         e.request_id==ticket.request_id and e.ground_session==ticket.ground_session and e.state==4]
        assert client_terminal, "BoatClient did not receive completion through gateway USB output"
        stream=codec.FrameStream(); recovered=[]
        usb_bytes=b"".join(piece for _,piece in radio_frames)
        for offset in range(0,len(usb_bytes),3):
            recovered.extend(frame for frame,_ in stream.feed(usb_bytes[offset:offset+3]))
        assert [codec.decode(frame)["name"] for frame in recovered]==names
        return {"request_id":ticket.request_id,"session":ticket.ground_session,
                "motor_boot":rig.rig.lib.scenario_motor_boot(rig.rig.motor),
                "target_cdeg":rig.rig.lib.scenario_motor_target(rig.rig.motor,0),
                "terminal_state":4,"client_received_completion":True,"usb_fragment_bytes":3,
                "returned_names":names,"bus_ops":rig.bus_ops,"trace":rig.trace}
    finally: rig.cleanup()


def _radio_ingress(rig: LoRaRig, frame: bytes):
    encoded = binascii.b2a_base64(frame).strip()
    line = b"+RCV=2," + str(len(encoded)).encode() + b"," + encoded + b",-71,6.0"
    return rig.relay.scenario_relay_modem_line(line, 2)


def run_lora_authority_and_integrity_guards():
    rig = LoRaRig()
    try:
        rig.connect_and_reconcile()
        base = {"request_id":20,"ground_session":0x50607080,"delta_cdeg":900,
            "timeout_ms":1000,"max_rate_cdeg_s":1000,"target_system":1,
            "target_component":154,"actuator":1,"opcode":1,"control_source":3}
        rejected = []
        # These structurally valid frames cross the production gateway/relay and
        # are then checked by the real router from its mailbox ingress.
        cases = [
            ("wrong_target", codec.encode("ACTUATOR_REQUEST",{**base,"request_id":20,
                "target_component":153},system=255,component=190), "router"),
            ("wrong_authority", codec.encode("ACTUATOR_REQUEST",{**base,"request_id":21,
                "control_source":2},system=255,component=190), "router"),
            ("wrong_source", codec.encode("ACTUATOR_REQUEST",{**base,"request_id":22},
                system=1,component=1), "router"),
        ("unsupported", codec.encode("ACTUATOR_SETPOINT",{"command_id":1,
                "position_cdeg":900,"timeout_ms":1000,
                "max_rate_cdeg_s":1000,"target_system":1,"target_component":154,
                "actuator":1,"control_source":3},system=255,component=190), "router"),
        ]
        for label, frame, expected in cases:
            result = _radio_ingress(rig, frame)
            if expected == "relay":
                assert result != 0, f"relay accepted {label}"
                rejected.append({"case":label,"relay_result":result})
                continue
            assert result == 0, (label, result)
            mailbox = (ctypes.c_uint8 * 64)(); rig.relay.scenario_relay_i2c_read(mailbox)
            routed = bytes(mailbox); actual = routed[:routed[1] + 12]
            accepted = rig.rig.router.submit_frame(actual, ingress="comms")
            assert not accepted, f"router accepted {label}"
            rejected.append({"case":label,"frame":actual.hex(),"router_accepted":accepted})
        damaged = bytearray(codec.encode("ACTUATOR_REQUEST",{**base,"request_id":23},
            system=255,component=190)); damaged[-1] ^= 0x20
        signed = bytearray(codec.encode("ACTUATOR_REQUEST",{**base,"request_id":24},
            system=255,component=190)); signed[2] |= 1
        msgid = int.from_bytes(signed[7:10],"little")
        extra = codec.MESSAGES[msgid]["crc_extra"]
        checksum = codec.crc(signed[1:10+signed[1]] + bytes((extra,)))
        signed[10+signed[1]:12+signed[1]] = checksum.to_bytes(2,"little")
        for label, raw in (("damaged_crc",bytes(damaged)),("signed",bytes(signed))):
            result = _radio_ingress(rig, raw)
            assert result != 0, f"relay accepted {label}"
            rejected.append({"case":label,"relay_result":result,"frame":raw.hex()})
        assert not any(rig.rig.lib.scenario_motor_active(rig.rig.motor, i) for i in (0,1))
        assert all(rig.rig.lib.scenario_motor_enabled(rig.rig.motor, i) == 0 for i in (0,1))
        return {"rejections":rejected,"motor_active":[rig.rig.lib.scenario_motor_active(rig.rig.motor,i)
                for i in (0,1)],"motor_enabled":[rig.rig.lib.scenario_motor_enabled(rig.rig.motor,i)
                for i in (0,1)],"relay_malformed":rig.relay.scenario_relay_malformed()}
    finally: rig.cleanup()


def run_motor_replay_reference_busy_matrix():
    rig = IntegratedRig(boot_id=0x15401540)
    try:
        rig.connect_and_reconcile()
        def frame(request_id, session=0x50607080, angle=900):
            return codec.encode("ACTUATOR_REQUEST",{
                "request_id":request_id,"ground_session":session,"delta_cdeg":angle,
                "timeout_ms":3000,"max_rate_cdeg_s":1000,"target_system":1,
                "target_component":154,"actuator":1,"opcode":1,"control_source":3},
                system=1,component=1,sequence=request_id & 0xff).ljust(64,b"\0")
        adapter = rig.lib.scenario_motor_receive
        slot = 0; outcomes = []
        rig.lib.scenario_motor_set_reference(rig.motor,slot,0)
        assert adapter(rig.motor,_bytes_arg(frame(1)))
        assert rig.lib.scenario_motor_active(rig.motor,slot) == 0
        assert rig.lib.scenario_motor_enabled(rig.motor,slot) == 0
        outcomes.append({"case":"invalid_reference","active":0,"enabled":0})
        rig.lib.scenario_motor_set_reference(rig.motor,slot,1)
        rig.bus.fresh[0] = False; rig.bus._update()
        assert adapter(rig.motor,_bytes_arg(frame(2)))
        assert rig.lib.scenario_motor_active(rig.motor,slot) == 0
        outcomes.append({"case":"stale_feedback_rejected","active":0})
        rig.bus.fresh[0] = True; rig.bus._update()
        good = frame(3)
        assert adapter(rig.motor,_bytes_arg(good))
        rig.lib.scenario_motor_poll(rig.motor)
        drives = rig.lib.scenario_motor_drives(rig.motor,slot)
        target = rig.lib.scenario_motor_target(rig.motor,slot)
        assert target == 900 and rig.lib.scenario_motor_active(rig.motor,slot) == 1
        assert adapter(rig.motor,_bytes_arg(good))
        assert rig.lib.scenario_motor_drives(rig.motor,slot) == drives
        assert rig.lib.scenario_motor_target(rig.motor,slot) == target
        outcomes.append({"case":"duplicate_exactly_once","drive_calls":drives,"target":target})
        assert adapter(rig.motor,_bytes_arg(frame(2)))
        assert rig.lib.scenario_motor_drives(rig.motor,slot) == drives
        outcomes.append({"case":"reordered_id_suppressed","drive_calls":drives})
        assert adapter(rig.motor,_bytes_arg(frame(1,session=0x60607080,angle=1200)))
        assert rig.lib.scenario_motor_target(rig.motor,slot) == target
        outcomes.append({"case":"busy_second_session_rejected","target_unchanged":target})
        # Router independently refuses an active motion owned by another client.
        router_move = codec.encode("ACTUATOR_REQUEST",{
            "request_id":50,"ground_session":0x11110001,"delta_cdeg":800,
            "timeout_ms":1000,"max_rate_cdeg_s":1000,"target_system":1,
            "target_component":154,"actuator":2,"opcode":1,"control_source":3},
            system=255,component=190)
        # Update router ledger with the live client's action and then test a
        # second origin against the real busy/ownership gate.
        rig.rig_client_ticket = rig.client.set_absolute_angle(2,800,timeout_ms=1000,max_rate_cdeg_s=1000)
        competing = bytearray(router_move); competing[12:16] = (51).to_bytes(4,"little")
        msgid = int.from_bytes(competing[7:10],"little"); extra=codec.MESSAGES[msgid]["crc_extra"]
        checksum=codec.crc(competing[1:10+competing[1]]+bytes((extra,)))
        competing[10+competing[1]:12+competing[1]]=checksum.to_bytes(2,"little")
        accepted=rig.router.submit_frame(bytes(competing),ingress="ssh")
        assert not accepted
        outcomes.append({"case":"router_concurrent_client_rejected","accepted":accepted})
        # Complete current adapter move, then prove command ID exhaustion cannot wrap.
        rig.step_encoder(1,900); rig.bus._update(); rig.lib.scenario_motor_poll(rig.motor)
        rig.lib.scenario_motor_set_next_command(rig.motor,0xffffffff)
        assert adapter(rig.motor,_bytes_arg(frame(4,session=0x50607080,angle=1000)))
        assert rig.lib.scenario_motor_active(rig.motor,slot) == 0
        assert rig.lib.scenario_motor_enabled(rig.motor,slot) == 0
        outcomes.append({"case":"local_command_id_no_wrap","active":0,"enabled":0})
        rig.client._next_request_id=0xffffffff
        max_ticket=rig.client.stop(1)
        attempts=len(rig.transport.sent)
        try: rig.client.stop(1)
        except OverflowError: pass
        else: raise AssertionError("ground request ID wrapped within the same session")
        assert max_ticket.request_id==0xffffffff and len(rig.transport.sent)==attempts
        outcomes.append({"case":"ground_request_id_exhaustion_same_session",
                         "last_id":max_ticket.request_id,"next_send_count":0})
        return {"checks":outcomes,"final_motor_boot":rig.lib.scenario_motor_boot(rig.motor),
                "motor_enabled":rig.lib.scenario_motor_enabled(rig.motor,slot),
                "router_queued_writes":rig.router.queued_writes}
    finally: rig.cleanup()


def run_lora_ping_all_nodes():
    rig = LoRaRig()
    try:
        rig.connect_and_reconcile()
        # Drain startup snapshots from the C relay so replies are attributable
        # to this single broadcast sequence.
        while True:
            raw=(ctypes.c_uint8*64)(); size=ctypes.c_size_t()
            if not rig.relay.scenario_relay_radio_next(raw,ctypes.byref(size)): break
        ping=codec.encode("PING",{"time_usec":123456789,"seq":77,
            "target_system":0,"target_component":0},system=255,component=190,sequence=77)
        to_pi=rig.gateway_usb_to_radio(ping)
        assert codec.decode(to_pi)["name"] == "PING"
        rig.rig.router.submit_frame(to_pi,ingress="comms")
        for _ in range(20): rig.rig.transport.pump(1)
        returned=rig.pi_to_gateway_usb()
        replies=[codec.decode(raw) for raw,_ in returned if codec.decode(raw)["name"]=="PING"]
        identities={(m["system"],m["component"]) for m in replies
                    if m["fields"]["seq"]==77 and m["fields"]["time_usec"]==123456789}
        assert identities == {(1,1),(1,154),(1,158)}, identities
        assert all(m["fields"]["target_system"]==255 and
                   m["fields"]["target_component"]==190 for m in replies if m["fields"]["seq"]==77)
        # An addressed reply is injected once at the relay and must not be
        # reflected or answered again by the production C relay.
        addressed=codec.encode("PING",{"time_usec":123456789,"seq":77,
            "target_system":255,"target_component":190},system=1,component=154)
        assert _radio_ingress(rig,addressed)==0
        after=(ctypes.c_uint8*64)(); size=ctypes.c_size_t()
        leftovers=[]
        while rig.relay.scenario_relay_radio_next(after,ctypes.byref(size)):
            raw=bytes(after[:size.value])
            leftovers.append(codec.decode(raw))
        assert not any(m["name"]=="PING" and m["fields"]["seq"]==77 for m in leftovers), leftovers
        return {"broadcast_seq":77,"reply_sources":[list(x) for x in sorted(identities)],
                "reply_count":len([m for m in replies if m["fields"]["seq"]==77]),
                "relay_forwarded_from_radio":rig.relay.scenario_relay_forwarded_from_radio(),
                "router_mailbox_ops":rig.bus_ops,"trace":rig.trace}
    finally: rig.cleanup()


def run_modem_fragment_malformed_recovery():
    temp,relay=_load_relay()
    try:
        relay.scenario_relay_init(0x15801580)
        frame=codec.encode("ACTUATOR_REQUEST",{"request_id":31,"ground_session":0x50607080,
            "delta_cdeg":500,"timeout_ms":1000,"max_rate_cdeg_s":1000,"target_system":1,
            "target_component":154,"actuator":1,"opcode":1,"control_source":3},
            system=255,component=190)
        encoded=binascii.b2a_base64(frame).strip()
        line=b"+RCV=2,"+str(len(encoded)).encode()+b","+encoded+b",-68,7.0\r\n"
        chunks=[line[:4],line[4:11],line[11:len(line)-3],line[-3:]]
        for chunk in chunks:
            relay.scenario_modem_feed(_bytes_arg(chunk),len(chunk))
            if chunk is not chunks[-1]:
                pending=ctypes.create_string_buffer(300)
                assert not relay.scenario_modem_pop(pending), "fragmented modem line parsed before CR/LF"
        parsed=ctypes.create_string_buffer(300)
        assert relay.scenario_modem_pop(parsed)
        assert relay.scenario_relay_modem_line(parsed.value,2)==0
        mailbox=(ctypes.c_uint8*64)(); relay.scenario_relay_i2c_read(mailbox)
        assert bytes(mailbox[:len(frame)])==frame
        malformed=b"+RCV=2,8,AAAA,-68,7.0\r\n"
        relay.scenario_modem_feed(_bytes_arg(malformed),len(malformed))
        bad=ctypes.create_string_buffer(300); assert relay.scenario_modem_pop(bad)
        assert relay.scenario_relay_modem_line(bad.value,2)!=0
        # A later good packet recovers and is delivered once, with no replay of
        # either the malformed payload or the incomplete fragment.
        line2=b"+RCV=2,"+str(len(encoded)).encode()+b","+encoded+b",-68,7.0\r\n"
        relay.scenario_modem_feed(_bytes_arg(line2),len(line2))
        recovered=ctypes.create_string_buffer(300); assert relay.scenario_modem_pop(recovered)
        assert relay.scenario_relay_modem_line(recovered.value,2)==0
        second=(ctypes.c_uint8*64)(); relay.scenario_relay_i2c_read(second)
        assert bytes(second[:len(frame)])==frame
        assert relay.scenario_relay_malformed()>=1
        return {"fragment_sizes":[len(c) for c in chunks],"incomplete_before_terminator":True,
                "malformed_rejected":True,"recovered_frame_hex":frame.hex(),
                "malformed_count":relay.scenario_relay_malformed()}
    finally: temp.cleanup()


def run_lora_imu_units_error():
    import struct
    raw=struct.pack("<6h",12,-34,1000,7,-8,9)
    rig=LoRaRig(imu=raw)
    try:
        rig.connect_and_reconcile()
        before=len(rig.radio_delivered)
        for _ in range(120):
            rig.rig.transport.pump(1)
            rig.radio_delivered.extend(rig.pi_to_gateway_usb())
        returned=rig.radio_delivered[before:]
        messages=[codec.decode(frame) for frame,_ in returned]
        samples=[m for m in messages if m["name"]=="SCALED_IMU"]
        pi_health=[m for m in messages if m["name"]=="NODE_HEALTH" and m["component"]==1]
        assert samples, "Pi IMU samples did not traverse the relay and gateway"
        sample=samples[-1]["fields"]
        assert tuple(sample[k] for k in ("xacc","yacc","zacc"))==(12,-34,1000)
        assert tuple(sample[k] for k in ("xgyro","ygyro","zgyro"))==(7,-8,9)
        assert pi_health and pi_health[-1]["fields"]["sensor_valid_fields"]==3
        # Keep register reads unavailable until invalid health crosses the
        # bounded radio queues; a single failed read may recover before then.
        rig.rig.bus.faults["imu"]=100
        rig.rig.clock.advance(25)
        assert not rig.rig.router._poll_imu()
        fault_frames=[]
        for _ in range(20):
            rig.rig.transport.pump(1)
            fault_frames.extend(rig.pi_to_gateway_usb())
        fault_health=[codec.decode(frame) for frame,_ in fault_frames
                      if codec.decode(frame)["name"]=="NODE_HEALTH" and
                      codec.decode(frame)["component"]==1]
        assert fault_health and fault_health[-1]["fields"]["sensor_valid_fields"]==0
        assert fault_health[-1]["fields"]["fault_flags"] & 2
        rig.rig.bus.faults["imu"]=0
        assert rig.rig.router._poll_imu()
        recovered_frames=[]
        for _ in range(20):
            rig.rig.transport.pump(1)
            recovered_frames.extend(rig.pi_to_gateway_usb())
        recovered_health=[codec.decode(frame) for frame,_ in recovered_frames
                          if codec.decode(frame)["name"]=="NODE_HEALTH" and
                          codec.decode(frame)["component"]==1]
        assert recovered_health and recovered_health[-1]["fields"]["sensor_valid_fields"]==3
        assert recovered_health[-1]["fields"]["fault_flags"]==0
        return {"sample_fields":{k:sample[k] for k in
                    ("xacc","yacc","zacc","xgyro","ygyro","zgyro")},
                "valid_fields":3,"fault_valid_fields":fault_health[-1]["fields"]["sensor_valid_fields"],
                "fault_flags":fault_health[-1]["fields"]["fault_flags"],
                "recovered_valid_fields":recovered_health[-1]["fields"]["sensor_valid_fields"],
                "trace":rig.trace[-30:]}
    finally: rig.cleanup()


def run_imu_stale_expiry():
    """Assert sample freshness expires after the contract scenario's 5 s bound."""
    import struct
    rig=IntegratedRig(imu=struct.pack("<6h",1,2,3,4,5,6))
    try:
        rig.router._poll_imu()
        assert rig.router.imu_valid_fields==3
        sample_at=rig.router.imu_last_seen
        rig.clock.advance(5001)
        # Periodic health and a later scheduler tick must not continue to claim
        # old register data is valid. This is an explicit boundary assertion.
        rig.router.step()
        assert rig.router.imu_valid_fields==0, (
            f"stale sample remained valid: sampled_at={sample_at}, now={rig.clock.now}, "
            f"valid_fields={rig.router.imu_valid_fields}")
        return {"sample_at":sample_at,"now":rig.clock.now,"valid_fields":rig.router.imu_valid_fields}
    finally: rig.cleanup()


def run_router_bus_fault_recovery():
    rig=IntegratedRig()
    try:
        rig.connect_and_reconcile()
        rig.bus.faults["write"]=1
        writes_before=sum(op["op"]=="write" and op["address"]==0x08 for op in rig.bus.operations)
        try:
            rig.client.set_absolute_angle(1,850,timeout_ms=1500,max_rate_cdeg_s=1000)
        except OSError:
            pass
        outcomes=[e for e in rig.events if type(e).__name__=="RequestOutcome" and e.result==0 and e.state==0]
        assert outcomes, "ambiguous bus write did not become UNKNOWN"
        attempted=[op for op in rig.bus.operations if op["op"]=="write" and op["address"]==0x08]
        assert len(attempted)==writes_before+1
        for _ in range(30): rig.transport.pump(1)
        attempted_after=[op for op in rig.bus.operations if op["op"]=="write" and op["address"]==0x08]
        assert len(attempted_after)==writes_before+1, "ambiguous movement was replayed after bus recovery"
        assert not rig.lib.scenario_motor_active(rig.motor,0)
        assert not rig.router.motor_reconcile_required, "fresh status did not reconcile after ambiguous write"
        # Short mailbox reads become explicit bus errors; a following full read
        # recovers peer presence without hidden framing repair.
        real_read=rig.bus.read_mailbox
        first=[True]
        def truncated(address):
            if address==0x08 and first[0]: first[0]=False; return bytes(23)
            return real_read(address)
        rig.bus.read_mailbox=truncated
        before=rig.router.bus_errors
        assert not rig.router._poll_node(0x08)
        assert rig.router.bus_errors==before+1
        assert rig.router._poll_node(0x08)
        return {"attempted_motor_write_delta":len(attempted_after)-writes_before,
                "unknown_request_id":outcomes[-1].request_id,
                "motor_write_count_after_recovery":len(attempted_after),
                "truncated_read_error_count":rig.router.bus_errors,
                "motor_active":rig.lib.scenario_motor_active(rig.motor,0),
                "motor_reconciled_after_recovery":not rig.router.motor_reconcile_required,
                "bus_ops":rig.bus.operations[-20:]}
    finally: rig.cleanup()


def run_router_queue_fairness():
    from onboard.config import ServiceConfig
    rig=IntegratedRig()
    try:
        rig.connect_and_reconcile()
        rig.router.config=ServiceConfig(queue_limit=8)
        # Fill the actual bounded router queue with routine node telemetry to
        # both peers, then insert a ground STOP for the compiled motor.
        for i in range(8): rig.router._queue(codec.encode("HEARTBEAT",{"type":18,
                "autopilot":0,"base_mode":0,"custom_mode":i,"system_status":4},
                system=20+i,component=1,sequence=i),
                0x08 if i%2==0 else 0x09,3)
        stop=codec.encode("ACTUATOR_REQUEST",{"request_id":90,"ground_session":0x50607080,
            "delta_cdeg":0,"timeout_ms":0,"max_rate_cdeg_s":0,"target_system":1,
            "target_component":154,"actuator":1,"opcode":3,"control_source":3},
            system=255,component=190)
        assert rig.router.submit_frame(stop)
        status=codec.encode("ACTUATOR_STATUS",{"time_boot_ms":1,"command_id":0,
            "requested_position_cdeg":0,"measured_position_cdeg":0,"velocity_cdeg_s":0,
            "fault_flags":0,"actuator":1,"control_source":0,"result":0,"enabled":0},
            system=1,component=154,sequence=40)
        health=codec.encode("NODE_HEALTH",{"boot_id":0x33334444,"time_boot_ms":1,
            "fault_flags":0,"sensor_valid_fields":0,"queue_drops":0,"reset_reason":0,
            "feedback_valid":1},system=1,component=154,sequence=41)
        assert rig.router._queue(status,0x09,1)
        assert rig.router._queue(health,0x09,1)
        stop_writes=0; polls={0x08:0,0x09:0}; consecutive=0; max_consecutive=0
        write_names=[]
        for _ in range(24):
            before=len(rig.bus.operations); rig.router.step(); rig.clock.advance(20)
            ops=rig.bus.operations[before:]
            if ops and ops[-1]["op"]=="write":
                consecutive+=1; max_consecutive=max(max_consecutive,consecutive)
                box=bytes.fromhex(ops[-1]["hex"])
                try:
                    msg=codec.decode(box[:box[1]+12])
                    write_names.append(msg["name"])
                    if msg["name"]=="ACTUATOR_REQUEST" and msg["fields"]["opcode"]==3:
                        stop_writes+=1
                except ValueError: pass
            else:
                consecutive=0
                if ops and ops[-1]["op"]=="read": polls[ops[-1]["address"]]+=1
        assert stop_writes==1
        assert max_consecutive<=3, f"scheduler exceeded 3 consecutive writes: {max_consecutive}"
        assert all(count for count in polls.values()), polls
        assert rig.router.queued_writes<=9
        assert write_names[:3][0]=="ACTUATOR_REQUEST" and write_names[0:3].count("NODE_HEALTH")==1 and \
               write_names[0:3].count("ACTUATOR_STATUS")==1, write_names[:3]
        return {"stop_bus_writes":stop_writes,"max_consecutive_writes":max_consecutive,
                "first_three_writes":write_names[:3],
                "poll_counts":polls,"queue_depth_after_24_ticks":rig.router.queued_writes,
                "tick_interval_ms":20,"max_elapsed_ms":480}
    finally: rig.cleanup()


def run_motor_reboot_retirement():
    rig=IntegratedRig(boot_id=0x11112222)
    try:
        rig.connect_and_reconcile()
        ticket=rig.client.set_absolute_angle(1,1100,timeout_ms=3000,max_rate_cdeg_s=1000)
        assert rig.lib.scenario_motor_active(rig.motor,0)==1
        old_motor=rig.motor
        rig.lib.scenario_motor_destroy(old_motor)
        rig.motor=rig.lib.scenario_motor_create(0x33334444)
        assert rig.motor
        rig.bus.motor=rig.motor
        rig.clock.advance(1001)
        rig.bus._update(); rig.lib.scenario_motor_poll(rig.motor)
        # Poll actual boot health from the newly instantiated C adapter.
        for _ in range(12): rig.transport.pump(1)
        unknown=[e for e in rig.events if type(e).__name__=="RequestOutcome" and
                 e.request_id==ticket.request_id and e.result==0 and e.state==0]
        assert unknown, "router did not mark old-boot pending action UNKNOWN"
        assert ticket.ground_session in [session for _,_,session in rig.router._retired_sessions]
        writes_before=len([op for op in rig.bus.operations if op["op"]=="write" and op["address"]==8])
        rig.client.disconnect()
        with patch("boat.client.secrets.randbits",return_value=0x60607080): rig.client.connect()
        for _ in range(30): rig.transport.pump(1)
        writes_after=len([op for op in rig.bus.operations if op["op"]=="write" and op["address"]==8])
        assert writes_after==writes_before, "new session replayed the old movement"
        return {"old_boot":0x11112222,"new_boot":rig.lib.scenario_motor_boot(rig.motor),
                "old_request_id":ticket.request_id,"old_session_retired":ticket.ground_session,
                "unknown_outcomes":len(unknown),"post_reconnect_motor_writes":writes_after-writes_before,
                "trace":rig.frames[-24:]}
    finally: rig.cleanup()


def run_stale_queued_motion_on_motor_boot():
    rig=IntegratedRig(boot_id=0x11112222)
    try:
        rig.connect_and_reconcile()
        fields={"request_id":88,"ground_session":0x50607080,"delta_cdeg":1250,
            "timeout_ms":2000,"max_rate_cdeg_s":1000,"target_system":1,
            "target_component":154,"actuator":1,"opcode":1,"control_source":3}
        request=codec.encode("ACTUATOR_REQUEST",fields,system=255,component=190)
        assert rig.router.submit_frame(request,ingress="ssh")
        assert rig.router.queued_writes==1
        old_writes=sum(op["op"]=="write" and op["address"]==8 for op in rig.bus.operations)
        rig.lib.scenario_motor_destroy(rig.motor)
        rig.motor=rig.lib.scenario_motor_create(0x33334444); rig.bus.motor=rig.motor
        rig.clock.advance(1001); rig.bus._update(); rig.lib.scenario_motor_poll(rig.motor)
        mailbox=rig.bus.read_mailbox(0x08)
        frame=mailbox[:mailbox[1]+12]
        message=codec.decode(frame)
        assert message["name"]=="NODE_HEALTH" and message["fields"]["boot_id"]==0x33334444
        rig.router._observe_node_message(message,frame)
        key=(255,190,fields["ground_session"],fields["request_id"])
        assert not any(item.key==key for item in (*rig.router._normal,*rig.router._stops))
        stale=[e for e in rig.frames if e["name"]=="REQUEST_RESULT"]
        results=[codec.decode(bytes.fromhex(e["hex"]))["fields"] for e in stale]
        request_result=[f for f in results if f["request_id"]==88]
        assert request_result and request_result[-1]["state"]==0
        writes=sum(op["op"]=="write" and op["address"]==8 for op in rig.bus.operations)
        assert writes==old_writes, "stale queued request reached the new motor boot"
        return {"queued_request_id":88,"new_motor_boot":message["fields"]["boot_id"],
                "stale_state":request_result[-1]["state"],"queued_after_reset":rig.router.queued_writes,
                "post_reset_motor_writes":writes-old_writes,"trace":rig.frames[-12:]}
    finally: rig.cleanup()


def run_missing_comms_isolation():
    rig=IntegratedRig()
    try:
        rig.connect_and_reconcile()
        real=rig.bus.read_mailbox
        def missing(address):
            if address==0x09: raise OSError("scenario missing comms peer")
            return real(address)
        rig.bus.read_mailbox=missing
        before=sum(op["op"]=="read" and op["address"]==0x08 for op in rig.bus.operations)
        for _ in range(12):
            rig.router._poll_node(0x09)
            assert rig.router._poll_node(0x08)
        after=sum(op["op"]=="read" and op["address"]==0x08 for op in rig.bus.operations)
        assert after-before==12
        assert rig.router.node_present[(1,158)] is False
        assert rig.router.node_present[(1,154)] is True
        return {"missing_peer":[1,158],"healthy_peer":[1,154],
                "healthy_reads_continued":after-before,"bus_errors":rig.router.bus_errors,
                "healthy_motor_boot":rig.lib.scenario_motor_boot(rig.motor)}
    finally: rig.cleanup()


def run_router_comms_resets():
    from onboard.router import BoatRouter
    from onboard.config import ServiceConfig
    rig=LoRaRig()
    try:
        rig.connect_and_reconcile()
        old_router_boot=rig.rig.router.boot_id
        old_comms_boot=0x15801580
        # Recreate both production state machines with fresh test boot IDs.
        router=BoatRouter(rig.rig.bus,ServiceConfig(),clock=rig.rig.clock)
        router.boot_id=0xA1B2C3D4
        rig.rig.router.close(); rig.rig.router=router
        rig.rig.transport.router=router
        rig.relay.scenario_relay_init(0x15811581)
        frames=[]
        while True:
            out=(ctypes.c_uint8*64)(); size=ctypes.c_size_t()
            if not rig.relay.scenario_relay_radio_next(out,ctypes.byref(size)): break
            frames.append(codec.decode(bytes(out[:size.value])))
        health=[m for m in frames if m["name"]=="NODE_HEALTH" and m["component"]==158]
        assert health and health[0]["fields"]["boot_id"]==0x15811581
        assert router.boot_id!=old_router_boot and health[0]["fields"]["boot_id"]!=old_comms_boot
        # The new router owns empty queues and does not inherit previous motion.
        assert router.queued_writes==0 and not router._requests
        return {"router_boot_before":old_router_boot,"router_boot_after":router.boot_id,
                "comms_boot_before":old_comms_boot,"comms_boot_after":health[0]["fields"]["boot_id"],
                "router_queued_after_reset":router.queued_writes,"inherited_requests":len(router._requests)}
    finally: rig.cleanup()


def run_relay_queue_stop_priority():
    tmp,relay=_load_relay()
    try:
        relay.scenario_relay_init(0x15801580)
        for seq in range(8):
            telemetry=codec.encode("HEARTBEAT",{"type":18,"autopilot":0,"base_mode":0,
                "custom_mode":seq,"system_status":4},system=20+seq,component=1,sequence=seq)
            assert relay.scenario_relay_radio(_bytes_arg(telemetry),len(telemetry))==0
        stop=codec.encode("ACTUATOR_REQUEST",{"request_id":99,"ground_session":0x50607080,
            "delta_cdeg":0,"timeout_ms":0,"max_rate_cdeg_s":0,"target_system":1,
            "target_component":154,"actuator":1,"opcode":3,"control_source":3},
            system=255,component=190)
        assert relay.scenario_relay_radio(_bytes_arg(stop),len(stop))==0
        box=(ctypes.c_uint8*64)(); relay.scenario_relay_i2c_read(box)
        first=bytes(box[:box[1]+12]); decoded=codec.decode(first)
        assert decoded["name"]=="ACTUATOR_REQUEST" and decoded["fields"]["opcode"]==3
        assert relay.scenario_relay_drops()>0
        return {"telemetry_enqueued":8,"queue_drops":relay.scenario_relay_drops(),
                "first_mailbox_name":decoded["name"],"first_mailbox_opcode":decoded["fields"]["opcode"],
                "stop_frame_hex":first.hex()}
    finally: tmp.cleanup()


class _LocalBridgeProcess:
    """Pipe-backed process facade running production _run_bridge locally."""
    def __init__(self, argv, *, stdin, stdout, stderr, bufsize):
        assert argv == ["ssh", "-T", "-o", "BatchMode=yes", "-o",
            "StrictHostKeyChecking=yes", "-o", "ClearAllForwardings=yes", "boat-pi",
            "python3", "-m", "onboard", "bridge", "--config", "/etc/arsem/onboard.toml"]
        assert stdin == subprocess.PIPE and stdout == subprocess.PIPE and stderr == subprocess.PIPE and bufsize == 0
        self.argv = argv
        child_in, parent_in = os.pipe()
        parent_out, child_out = os.pipe()
        parent_err, child_err = os.pipe()
        self.stdin = os.fdopen(parent_in, "wb", buffering=0)
        self.stdout = os.fdopen(parent_out, "rb", buffering=0)
        self.stderr = os.fdopen(parent_err, "rb", buffering=0)
        self._done = threading.Event()
        self._stop = threading.Event()
        self._output_queue = queue.Queue()
        global _SSH_OUTPUT_QUEUE
        _SSH_OUTPUT_QUEUE = self._output_queue
        self._threads = []

        def run():
            import onboard.__main__ as entry
            old_in, old_err = sys.stdin, sys.stderr
            try:
                sys.stdin = io.TextIOWrapper(os.fdopen(child_in, "rb", buffering=0))
                sys.stderr = os.fdopen(child_err, "w", buffering=1)
                print("local bridge worker stderr marker", file=sys.stderr, flush=True)
                entry._run_bridge(_SSH_ACTIVE_RIG.router, self._stop.is_set)
            finally:
                sys.stdin.close(); sys.stderr.close()
                sys.stdin, sys.stderr = old_in, old_err
                self._output_queue.put(None)
                self._output_thread.join(timeout=1)
                os.close(child_out)
                self._done.set()

        def output_frames():
            # Short writes force SSHTransport's real MAVLink stream parser to
            # reassemble frames fragmented across stdout reads.
            while True:
                data = self._output_queue.get()
                if data is None: break
                for off in range(0, len(data), 5):
                    try: os.write(child_out, data[off:off + 5])
                    except BrokenPipeError: return

        self._output_thread = threading.Thread(target=output_frames, daemon=True)
        self._worker = threading.Thread(target=run, daemon=True)
        self._output_thread.start(); self._worker.start()

    def poll(self): return 0 if self._done.is_set() else None
    def wait(self, timeout=None):
        if not self._done.wait(timeout): raise subprocess.TimeoutExpired(self.argv, timeout)
        return 0
    def terminate(self): self._stop.set(); self.stdin.close()
    def kill(self): self._stop.set(); self.stdin.close()


_SSH_OUTPUT_QUEUE = __import__("queue").Queue()
_SSH_ACTIVE_RIG = None


def run_ssh_bridge_process():
    """Exercise actual SSHTransport stdio against actual onboard bridge loop."""
    from groundstation.transports import SSHTransport
    global _SSH_ACTIVE_RIG
    rig = IntegratedRig(boot_id=0x15401540, imu=struct.pack("<6h",12,-34,1000,7,-8,9))
    # Advance once to cross the adapter health cadence, then hold the test
    # clock fixed so frame timestamps and trace ordering stay deterministic.
    rig.clock.advance(1001)
    _SSH_ACTIVE_RIG = rig
    events, raw_frames = [], []
    transport = SSHTransport("boat-pi", popen_factory=_LocalBridgeProcess)
    client = rig.client
    client.transport = transport
    client.subscribe(events.append)
    # Production _run_bridge writes raw frames through the stdout pipe. Its
    # reader and stderr reader remain the threads owned by SSHTransport.
    trace = []
    def bridge_output(frame):
        message=codec.decode(frame)
        fields=message["fields"]
        record_result=(message["name"]=="REQUEST_RESULT" and fields.get("state") in (0,4,5,6))
        if message["name"] in ("NODE_HEALTH","SCALED_IMU") or record_result:
            trace.append({"at":rig.clock.now,"direction":"router_to_ssh_stdout",
                "name":message["name"],"source":[message["system"],message["component"]],
                "request_id":fields.get("request_id"),"ground_session":fields.get("ground_session"),
                "motor_boot_id":fields.get("motor_boot_id"),"hex":bytes(frame).hex()})
        _SSH_OUTPUT_QUEUE.put(bytes(frame))
    rig.router.on_frame = bridge_output
    old_submit = rig.router.submit_frame
    def observed_submit(frame, *, ingress="ssh"):
        raw_frames.append(bytes(frame))
        trace.append({"at":rig.clock.now,"direction":"stdio_to_router","ingress":ingress,"hex":bytes(frame).hex()})
        return old_submit(frame, ingress=ingress)
    rig.router.submit_frame = observed_submit
    try:
        with patch("boat.client.secrets.randbits", return_value=0x50607080): client.connect()
        limit = time.monotonic() + 3
        while not client._reconciled and time.monotonic() < limit: time.sleep(0.01)
        assert client._reconciled, "production SSH stdio did not deliver reconciled motor reports"
        limit=time.monotonic()+2
        while not any(type(e).__name__=="ImuSample" for e in events) and time.monotonic()<limit:
            time.sleep(0.01)
        imu_samples=[e for e in events if type(e).__name__=="ImuSample"]
        assert imu_samples, f"production SSH stdout did not carry the Pi-owned IMU sample; frames={[x['name'] for x in trace]}"
        # The Pi reports the new validity mask with the sample. Let its next
        # scheduled sample use that mask, then assert the typed client record.
        rig.clock.advance(1001)
        limit=time.monotonic()+2
        while not any(type(e).__name__=="ImuSample" and e.valid_fields==3 for e in events) and time.monotonic()<limit:
            time.sleep(0.01)
        imu_samples=[e for e in events if type(e).__name__=="ImuSample"]
        imu=imu_samples[-1]
        assert imu.accel_mg==(12,-34,1000) and imu.gyro_mrad_s==(7,-8,9)
        assert imu.source==(1,1) and imu.valid_fields==3, [e.valid_fields for e in imu_samples]
        ticket = client.set_absolute_angle(1, 700, timeout_ms=2000, max_rate_cdeg_s=1000)
        limit = time.monotonic() + 2
        while rig.lib.scenario_motor_active(rig.motor, 0) != 1 and time.monotonic() < limit: time.sleep(0.01)
        assert rig.lib.scenario_motor_active(rig.motor, 0) == 1
        rig.step_encoder(1, 700)
        # Clock loop and bridge scheduler are independent; wait for actual
        # terminal correlation to arrive from compiled adapter via stdout.
        limit = time.monotonic() + 3
        while not any(type(e).__name__ == "RequestOutcome" and e.request_id == ticket.request_id and e.state == 4
                      for e in events) and time.monotonic() < limit: time.sleep(0.01)
        completed = [e for e in events if type(e).__name__ == "RequestOutcome" and
                     e.request_id == ticket.request_id and e.state == 4]
        assert completed, "compiled motor completion did not return over SSH stdout"
        stderr = []
        limit = time.monotonic() + 1
        while time.monotonic() < limit:
            try: stderr.append(transport.stderr_lines.get_nowait())
            except __import__("queue").Empty: break
        assert "local bridge worker stderr marker" in stderr, f"stderr did not stay on log channel: {stderr}"
        assert raw_frames and all(codec.decode(f)["system"] == 255 for f in raw_frames)
        stable_trace=[]; seen=set()
        for item in trace:
            if item["direction"]=="stdio_to_router":
                message=codec.decode(bytes.fromhex(item["hex"]))
                key=("stdin",message["name"])
                if key in seen: continue
                seen.add(key)
                stable_trace.append({"at":item["at"],"direction":item["direction"],
                    "name":message["name"],"source":[message["system"],message["component"]],
                    "fields":message["fields"],"hex":item["hex"]})
                continue
            message=codec.decode(bytes.fromhex(item["hex"]))
            fields=message["fields"]
            if message["name"]=="NODE_HEALTH": key=("health",message["component"])
            elif message["name"]=="SCALED_IMU": key=("imu",)
            elif message["name"]=="REQUEST_RESULT" and fields["state"]==4:
                key=("terminal",fields["ground_session"],fields["request_id"])
            else: continue
            if key in seen: continue
            seen.add(key)
            stable_trace.append({"at":item["at"],"direction":item["direction"],
                "name":message["name"],"source":[message["system"],message["component"]],
                "fields":fields})
        return {"argv":transport._proc.argv,"request_id":ticket.request_id,
                "session":ticket.ground_session,"motor_boot":rig.lib.scenario_motor_boot(rig.motor),
                "completed_state":completed[0].state,"stderr_lines":stderr,
                "imu_accel_mg":imu.accel_mg,"imu_gyro_mrad_s":imu.gyro_mrad_s,
                "imu_valid_fields":imu.valid_fields,"stdin_frames":len(raw_frames),"trace":stable_trace}
    finally:
        transport.close(); rig.cleanup(); _SSH_ACTIVE_RIG = None
