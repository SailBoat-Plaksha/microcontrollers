import base64
import pathlib
import sys
import types
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "protocol"))
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

fake_ubinascii = types.ModuleType("ubinascii")
fake_ubinascii.b2a_base64 = lambda value: base64.b64encode(value) + b"\n"
fake_ubinascii.a2b_base64 = base64.b64decode
sys.modules.setdefault("ubinascii", fake_ubinascii)

from protocol import codec
from gateway import Gateway
from mavlink_stream import FrameStream, validate_frame


class FakeUART:
    def __init__(self):
        self.writes = []

    def write(self, value):
        self.writes.append(bytes(value))
        return len(value)


class FakeUSB:
    def __init__(self):
        self.writes = []

    def write(self, value):
        self.writes.append(bytes(value))
        return len(value)


def ping(seq=5):
    return codec.encode("PING", {"time_usec": 123456, "seq": seq,
        "target_system": 0, "target_component": 0}, system=255, component=190)


def actuator(opcode):
    return codec.encode("ACTUATOR_REQUEST", {"request_id": 1, "ground_session": 9,
        "delta_cdeg": 0 if opcode == 3 else 500, "timeout_ms": 1000,
        "max_rate_cdeg_s": 200, "target_system": 1, "target_component": 154,
        "actuator": 1, "opcode": opcode, "control_source": 3},
        system=255, component=190)


class GatewayTests(unittest.TestCase):
    def setUp(self):
        self.uart = FakeUART()
        self.usb = FakeUSB()
        self.gateway = Gateway(self.uart, self.usb)

    def test_usb_fragmentation_and_base64_radio_envelope(self):
        frame = ping()
        self.assertEqual(self.gateway.feed_usb(frame[:3]), 0)
        self.assertEqual(self.gateway.feed_usb(frame[3:]), 1)
        self.assertEqual(self.uart.writes[0], b"AT+ADDRESS=2\r\n")
        for _ in range(4):
            self.gateway.feed_modem(b"+OK\r\n")
        self.assertFalse(self.gateway.waiting_config)
        self.gateway.tick()
        command = self.uart.writes[-1]
        self.assertTrue(command.startswith(b"AT+SEND=1,"))
        self.assertNotIn(frame, command)
        encoded = command.split(b",", 2)[2].strip()
        self.assertEqual(base64.b64decode(encoded), frame)

    def test_modem_fragmentation_forwards_raw_bytes_to_usb(self):
        frame = codec.encode("NODE_HEALTH", {"boot_id": 44, "time_boot_ms": 77,
            "fault_flags": 0, "sensor_valid_fields": 0, "queue_drops": 0,
            "reset_reason": 0, "feedback_valid": 0}, system=1, component=158)
        payload = base64.b64encode(frame)
        line = b"+RCV=1," + str(len(payload)).encode() + b"," + payload + b",-70,9.0\r\n"
        self.gateway.feed_modem(line[:8])
        self.gateway.feed_modem(line[8:])
        self.gateway.tick()
        self.assertEqual(self.usb.writes, [frame])

    def test_stop_moves_ahead_of_queued_movement(self):
        self.gateway.feed_usb(actuator(1) + actuator(3))
        for _ in range(4):
            self.gateway.feed_modem(b"+OK\r\n")
        self.gateway.tick()
        command = self.uart.writes[-1]
        encoded = command.split(b",", 2)[2].strip()
        self.assertEqual(codec.decode(base64.b64decode(encoded))["fields"]["opcode"], 3)

    def test_stop_evicts_queued_movement_when_bounded_queue_is_full(self):
        self.gateway.feed_usb(actuator(1) * 8)
        self.assertEqual(len(self.gateway.radio_queue), 8)
        self.gateway.feed_usb(actuator(3))
        self.assertEqual(len(self.gateway.radio_queue), 8)
        command = self.gateway.radio_queue[0][1]
        encoded = command.split(b",", 2)[2].strip()
        self.assertEqual(codec.decode(base64.b64decode(encoded))["fields"]["opcode"], 3)

    def test_multiple_modem_frames_are_queued_for_usb(self):
        frame_a, frame_b = ping(5), ping(6)
        payload_a, payload_b = base64.b64encode(frame_a), base64.b64encode(frame_b)
        line_a = b"+RCV=1," + str(len(payload_a)).encode() + b"," + payload_a + b",-70,9\r\n"
        line_b = b"+RCV=1," + str(len(payload_b)).encode() + b"," + payload_b + b",-71,8\r\n"
        self.gateway.feed_modem(line_a + line_b)
        self.gateway.tick()
        self.gateway.tick()
        self.assertEqual(self.usb.writes, [frame_a, frame_b])

    def test_crc_signed_oversize_and_unknown_messages_are_rejected(self):
        good = bytearray(ping())
        self.assertTrue(validate_frame(bytes(good)))
        bad_crc = bytearray(good)
        bad_crc[-1] ^= 1
        signed = bytearray(good)
        signed[2] = 1
        unknown = bytearray(good)
        unknown[7:10] = (9000).to_bytes(3, "little")
        for candidate in (bad_crc, signed, unknown):
            self.assertFalse(validate_frame(bytes(candidate)))
        stream = FrameStream()
        self.assertEqual(stream.feed(bytes(bad_crc) + bytes(good)), [bytes(good)])

    def test_usb_rejects_frames_with_non_ground_source_ids(self):
        wrong_source = codec.encode("PING", {"time_usec": 10, "seq": 1,
            "target_system": 0, "target_component": 0}, system=1, component=1)
        self.assertEqual(self.gateway.feed_usb(wrong_source), 0)
        self.assertEqual(self.gateway.radio_queue, [])

    def test_malformed_radio_line_does_not_reach_usb(self):
        self.gateway.feed_modem(b"+RCV=1,5,A,B,C\r\n")
        self.gateway.tick()
        self.assertEqual(self.usb.writes, [])


if __name__ == "__main__":
    unittest.main()
