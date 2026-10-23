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
        self.limit = None
        self.fail = None

    def write(self, value):
        if self.fail is not None:
            return self.fail
        count = len(value) if self.limit is None else min(len(value), self.limit)
        self.writes.append(bytes(value[:count]))
        return count


class FakeUSB:
    def __init__(self):
        self.writes = []
        self.value = "full"

    def write(self, value):
        if self.value == "full":
            count = len(value)
        elif self.value is None:
            return None
        elif self.value <= 0:
            return self.value
        else:
            count = min(self.value, len(value))
        self.writes.append(bytes(value[:count]))
        return count


def ping(seq=5):
    return codec.encode("PING", {"time_usec": 123456, "seq": seq,
        "target_system": 0, "target_component": 0}, system=255, component=190)


def actuator(opcode, request_id=1):
    return codec.encode("ACTUATOR_REQUEST", {"request_id": request_id, "ground_session": 9,
        "delta_cdeg": 0 if opcode == 3 else 500, "timeout_ms": 1000,
        "max_rate_cdeg_s": 200, "target_system": 1, "target_component": 154,
        "actuator": 1, "opcode": opcode, "control_source": 3},
        system=255, component=190)


class GatewayTests(unittest.TestCase):
    def setUp(self):
        self.uart = FakeUART()
        self.usb = FakeUSB()
        self.gateway = Gateway(self.uart, self.usb)

    def configure(self):
        for index in range(4):
            self.gateway.tick(index)
            self.assertEqual(self.uart.writes[-1], self.gateway.configs[index].encode() + b"\r\n")
            self.gateway.feed_modem(b"+OK\r\n", index)
        self.assertTrue(self.gateway.modem_ready)

    def test_usb_fragmentation_and_base64_radio_envelope(self):
        frame = ping()
        self.assertEqual(self.gateway.feed_usb(frame[:3]), 0)
        self.assertEqual(self.gateway.feed_usb(frame[3:]), 1)
        self.configure()
        self.gateway.tick(10)
        self.gateway.tick(10)
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
        self.configure()
        self.gateway.tick(10)
        self.gateway.tick(10)
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

    def test_partial_uart_write_waits_for_modem_ack(self):
        self.configure()
        self.uart.limit = 3
        self.gateway.feed_usb(ping(12))
        self.gateway.tick(10)
        self.gateway.tick(10)
        for _ in range(100):
            self.gateway.tick(11)
            if self.gateway.modem_state == "send_wait":
                break
        self.assertEqual(self.gateway.modem_state, "send_wait")
        writes = len(self.uart.writes)
        self.gateway.tick(12)
        self.assertEqual(len(self.uart.writes), writes)
        self.gateway.feed_modem(b"+OK\r\n", 12)
        self.assertEqual(self.gateway.local_modem_accepted, 1)
        self.assertTrue(self.gateway.modem_ready)

    def test_modem_error_is_not_malformed_radio_and_releases_reservation(self):
        self.configure()
        self.gateway.feed_usb(actuator(1))
        self.gateway.tick(10)
        self.gateway.tick(10)
        self.gateway.tick(10)
        self.assertEqual(self.gateway.modem_state, "send_wait")
        self.gateway.feed_modem(b"+ERR=10\r\n", 11)
        self.assertEqual(self.gateway.modem_state, "ready")
        self.assertEqual(self.gateway.modem_errors, 1)
        self.assertEqual(self.gateway.reservations, [])
        self.assertEqual(self.gateway.ambiguous_sends, 0)

    def test_usb_none_zero_negative_and_short_writes_preserve_frame(self):
        frame = codec.encode("NODE_HEALTH", {"boot_id": 3, "time_boot_ms": 5,
            "fault_flags": 0, "sensor_valid_fields": 0, "queue_drops": 0,
            "reset_reason": 0, "feedback_valid": 0}, system=1, component=158)
        payload = base64.b64encode(frame)
        self.gateway._radio_frame(b"+RCV=1," + str(len(payload)).encode() + b"," + payload + b",-70,9")
        self.usb.value = None
        self.gateway.tick(0)
        self.assertEqual(self.gateway.pending_usb_offset, 0)
        self.usb.value = 0
        self.gateway.tick(1)
        self.assertEqual(self.gateway.pending_usb_offset, 0)
        self.usb.value = -1
        self.gateway.tick(2)
        self.assertEqual(self.gateway.pending_usb_offset, 0)
        self.usb.value = 2
        self.gateway.tick(3)
        self.assertEqual(self.gateway.pending_usb_offset, 2)
        self.usb.value = len(frame)
        while self.gateway.usb_queue:
            self.gateway.tick(4)
        self.assertEqual(b"".join(self.usb.writes), frame)

    def test_ambiguous_send_timeout_is_not_replayed_on_reconnect(self):
        self.configure()
        self.gateway.feed_usb(actuator(2))
        self.gateway.tick(10)
        self.gateway.tick(10)
        self.gateway.tick(10)
        self.assertEqual(self.gateway.modem_state, "send_wait")
        self.gateway.tick(5010)
        self.assertEqual(self.gateway.modem_state, "send_uncertain")
        self.assertEqual(self.gateway.ambiguous_sends, 1)
        self.assertEqual(len(self.gateway.reservations), 1)
        self.gateway.reconnect()
        self.assertEqual(self.gateway.radio_queue, [])
        self.assertEqual(len(self.gateway.reservations), 1)
        for index in range(4):
            self.gateway.tick(6000 + index)
            self.gateway.feed_modem(b"+OK\r\n", 6000 + index)
        self.assertFalse(any(b"AT+SEND" in chunk for chunk in self.uart.writes[-4:]))

    def test_config_timeout_is_visible_and_requires_reconnect(self):
        self.gateway.tick(0)
        self.gateway.tick(3000)
        self.assertEqual(self.gateway.modem_state, "config_failed")
        self.assertEqual(self.gateway.status()["modem_errors"], 1)

    def test_response_queue_coalesces_by_correlation_and_counts_overflow(self):
        frame = codec.encode("REQUEST_RESULT", {"ground_session": 7, "request_id": 9,
            "motor_boot_id": 1, "local_command_id": 3, "result": 1, "state": 4,
            "target_system": 255, "target_component": 190}, system=1, component=1)
        changed = codec.encode("REQUEST_RESULT", {"ground_session": 7, "request_id": 9,
            "motor_boot_id": 1, "local_command_id": 4, "result": 1, "state": 5,
            "target_system": 255, "target_component": 190}, system=1, component=1)
        payload = base64.b64encode(frame)
        self.gateway._radio_frame(b"+RCV=1," + str(len(payload)).encode() + b"," + payload + b",-70,9")
        payload = base64.b64encode(changed)
        self.gateway._radio_frame(b"+RCV=1," + str(len(payload)).encode() + b"," + payload + b",-70,9")
        self.assertEqual(len(self.gateway.response_queue), 1)
        self.assertEqual(self.gateway.response_queue[0], changed)
        self.gateway.usb_queue = [ping(i) for i in range(16)]
        self.gateway._queue_usb(ping(17))
        self.assertEqual(self.gateway.usb_overflow, 1)

    def test_partial_terminal_output_is_not_replaced_mid_frame(self):
        first = codec.encode("REQUEST_RESULT", {"ground_session": 7, "request_id": 9,
            "motor_boot_id": 1, "local_command_id": 3, "result": 1, "state": 4,
            "target_system": 255, "target_component": 190}, system=1, component=1)
        latest = codec.encode("REQUEST_RESULT", {"ground_session": 7, "request_id": 9,
            "motor_boot_id": 1, "local_command_id": 4, "result": 1, "state": 5,
            "target_system": 255, "target_component": 190}, system=1, component=1)
        self.gateway._queue_usb(first)
        self.usb.value = 2
        self.gateway.tick(0)
        self.gateway._queue_usb(latest)
        while self.gateway.response_queue:
            self.usb.value = 64
            self.gateway.tick(1)
        self.assertEqual(b"".join(self.usb.writes), first + latest)

    def test_movement_admission_reserves_result_capacity_but_stop_still_admits(self):
        for request_id in range(1, 9):
            self.assertEqual(self.gateway.feed_usb(actuator(1, request_id)), 1)
        self.assertEqual(len(self.gateway.reservations), 8)
        self.gateway.radio_queue = []
        self.assertEqual(self.gateway.feed_usb(actuator(1, 9)), 0)
        self.assertEqual(self.gateway.feed_usb(actuator(3, 10)), 1)
        self.assertGreater(self.gateway.radio_overflow, 0)


if __name__ == "__main__":
    unittest.main()
