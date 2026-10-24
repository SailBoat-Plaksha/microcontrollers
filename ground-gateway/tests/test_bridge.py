import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

from bridge import ATBridge


class FakeUART:
    def __init__(self, limit=None, fail=None):
        self.writes = []
        self.limit = limit
        self.fail = fail

    def write(self, value):
        if self.fail is not None:
            return self.fail
        count = len(value) if self.limit is None else min(len(value), self.limit)
        self.writes.append(bytes(value[:count]))
        return count


class FakeUSB:
    def __init__(self, limit=None):
        self.writes = []
        self.limit = limit

    def write(self, value):
        if self.limit is None:
            count = len(value)
        elif self.limit <= 0:
            return self.limit
        else:
            count = min(self.limit, len(value))
        self.writes.append(bytes(value[:count]))
        return count


class HostStdinMachine:
    """Helper simulating host stdin and select.poll event loop with ATBridge."""

    def __init__(self, bridge, usb_sink=None):
        self.bridge = bridge
        self.stdin_buffer = bytearray()
        self.usb_sink = usb_sink

    def host_send(self, data):
        if isinstance(data, str):
            data = data.encode("latin-1")
        self.stdin_buffer.extend(data)

    def host_receive(self):
        if self.usb_sink is not None:
            out = b"".join(self.usb_sink.writes)
            self.usb_sink.writes.clear()
            return out
        return b""

    def step(self, chunk_size=1):
        if self.stdin_buffer:
            chunk = bytes(self.stdin_buffer[:chunk_size])
            del self.stdin_buffer[:chunk_size]
            self.bridge.feed_usb(chunk)
        self.bridge.tick()

    def step_all(self, chunk_size=1):
        while self.stdin_buffer:
            self.step(chunk_size)
        self.bridge.tick()


class ATBridgeTests(unittest.TestCase):
    def setUp(self):
        self.uart = FakeUART()
        self.usb = FakeUSB()
        self.bridge = ATBridge(self.uart, self.usb)

    def test_multi_fragment_lines(self):
        fragments = [b"A", b"T", b"+", b"SEND", b"=", b"1", b",", b"5", b",", b"H", b"E", b"L", b"L", b"O", b"\r", b"\n"]
        for frag in fragments:
            self.bridge.feed_usb(frag)
        self.bridge.tick()

        self.assertEqual(b"".join(self.uart.writes), b"AT+SEND=1,5,HELLO\r\n")
        self.assertEqual(self.usb.writes, [])

    def test_send_payload_unchanged(self):
        # Payload contains Base64 symbols and spaces; whitespace around command must be stripped
        # but payload bytes must NEVER be mutated!
        raw_cmd = b"  AT+SEND = 2 , 0 , SGVsbG8gV29ybGQ= \r\n"
        self.bridge.feed_usb(raw_cmd)
        self.bridge.tick()

        out = b"".join(self.uart.writes)
        # Expected: AT+SEND=2,<recalculated_length>,<preserved_payload>\r\n
        expected_payload = b" SGVsbG8gV29ybGQ= "
        expected_cmd = b"AT+SEND=2," + str(len(expected_payload)).encode("ascii") + b"," + expected_payload + b"\r\n"
        self.assertEqual(out, expected_cmd)

    def test_length_fix(self):
        cases = [
            (b"AT+SEND=1,0,DATA\r\n", b"AT+SEND=1,4,DATA\r\n"),
            (b"AT+SEND=1,999,DATA\r\n", b"AT+SEND=1,4,DATA\r\n"),
            (b"AT+SEND=1,invalid,DATA\r\n", b"AT+SEND=1,4,DATA\r\n"),
            (b"AT+SEND=1,4,DATA\r\n", b"AT+SEND=1,4,DATA\r\n"),
            (b"AT+SEND=1,0,\r\n", b"AT+SEND=1,0,\r\n"),
        ]
        for cmd_in, expected_out in cases:
            uart = FakeUART()
            usb = FakeUSB()
            bridge = ATBridge(uart, usb)
            bridge.feed_usb(cmd_in)
            bridge.tick()
            self.assertEqual(b"".join(uart.writes), expected_out)
            self.assertEqual(usb.writes, [])

    def test_invalid_commands_and_rejection(self):
        # 1. Non-AT command
        self.bridge.feed_usb(b"NOT_AN_AT_COMMAND\r\n")
        self.bridge.tick()
        self.assertIn(b"+ERR=BRIDGE_INVALID_COMMAND\r\n", b"".join(self.usb.writes))
        self.assertEqual(self.uart.writes, [])
        self.assertNotIn(b"+OK", b"".join(self.usb.writes))

        # 2. Control chars
        usb2 = FakeUSB()
        bridge2 = ATBridge(FakeUART(), usb2)
        bridge2.feed_usb(b"AT+TEST\x00CMD\r\n")
        bridge2.tick()
        self.assertIn(b"+ERR=BRIDGE_CONTROL_CHAR\r\n", b"".join(usb2.writes))
        self.assertNotIn(b"+OK", b"".join(usb2.writes))

        # 3. Non-ASCII
        usb3 = FakeUSB()
        bridge3 = ATBridge(FakeUART(), usb3)
        bridge3.feed_usb(b"AT+TEST\x80\r\n")
        bridge3.tick()
        self.assertIn(b"+ERR=BRIDGE_NON_ASCII\r\n", b"".join(usb3.writes))
        self.assertNotIn(b"+OK", b"".join(usb3.writes))

        # 4. Invalid address bounds (<0 or >65535 or non-digit)
        usb4 = FakeUSB()
        bridge4 = ATBridge(FakeUART(), usb4)
        bridge4.feed_usb(b"AT+SEND=65536,4,TEST\r\n")
        bridge4.tick()
        self.assertIn(b"+ERR=BRIDGE_INVALID_ADDRESS\r\n", b"".join(usb4.writes))

        usb4b = FakeUSB()
        bridge4b = ATBridge(FakeUART(), usb4b)
        bridge4b.feed_usb(b"AT+SEND=xyz,4,TEST\r\n")
        bridge4b.tick()
        self.assertIn(b"+ERR=BRIDGE_INVALID_ADDRESS\r\n", b"".join(usb4b.writes))

        # 5. AT+SEND parse errors (missing commas or =)
        usb5 = FakeUSB()
        bridge5 = ATBridge(FakeUART(), usb5)
        bridge5.feed_usb(b"AT+SEND=1,TEST\r\n")
        bridge5.tick()
        self.assertIn(b"+ERR=BRIDGE_PARSE_ERROR\r\n", b"".join(usb5.writes))

        usb5b = FakeUSB()
        bridge5b = ATBridge(FakeUART(), usb5b)
        bridge5b.feed_usb(b"AT+SEND\r\n")
        bridge5b.tick()
        self.assertIn(b"+ERR=BRIDGE_PARSE_ERROR\r\n", b"".join(usb5b.writes))

        # 6. Payload too large (>240)
        usb6 = FakeUSB()
        bridge6 = ATBridge(FakeUART(), usb6)
        oversize = b"A" * 241
        bridge6.feed_usb(b"AT+SEND=1,241," + oversize + b"\r\n")
        bridge6.tick()
        self.assertIn(b"+ERR=BRIDGE_PAYLOAD_TOO_LARGE\r\n", b"".join(usb6.writes))

    def test_newline_normalization(self):
        cases = [
            b"AT\r\n",
            b"AT\n",
            b"AT\r",
            b"  \r\n  AT\r\n  \r\n",
        ]
        for cmd in cases:
            uart = FakeUART()
            usb = FakeUSB()
            bridge = ATBridge(uart, usb)
            bridge.feed_usb(cmd)
            bridge.tick()
            self.assertEqual(b"".join(uart.writes), b"AT\r\n")

    def test_partial_uart_writes(self):
        # FakeUART only accepts 3 bytes per call
        uart = FakeUART(limit=3)
        usb = FakeUSB()
        bridge = ATBridge(uart, usb)

        bridge.feed_usb(b"AT+ADDRESS=2\r\n")  # 14 bytes
        self.assertEqual(len(uart.writes), 0)

        # Step ticks until completely sent
        while bridge.status()["uart_pending"] > 0:
            bridge.tick()

        self.assertEqual(b"".join(uart.writes), b"AT+ADDRESS=2\r\n")
        self.assertEqual(len(uart.writes), 5)  # 3+3+3+3+2 = 14 bytes

    def test_partial_usb_writes(self):
        # FakeUSB only accepts 4 bytes per call
        uart = FakeUART()
        usb = FakeUSB(limit=4)
        bridge = ATBridge(uart, usb)

        modem_line = b"+RCV=1,5,HELLO,-70,9.0\r\n"  # 24 bytes
        bridge.feed_modem(modem_line)

        while bridge.status()["usb_pending"] > 0:
            bridge.tick()

        self.assertEqual(b"".join(usb.writes), modem_line)
        self.assertEqual(len(usb.writes), 6)  # 4 * 6 = 24 bytes

    def test_overflow_and_recovery(self):
        # Test line length bounded input
        bridge = ATBridge(self.uart, self.usb, max_line=20)
        # Feed 25 bytes without newline
        bridge.feed_usb(b"A" * 25)
        self.bridge.tick()
        self.assertEqual(bridge.status()["rx_overflow"], 1)

        bridge.tick()
        self.assertIn(b"+ERR=BRIDGE_LINE_TOO_LONG\r\n", b"".join(self.usb.writes))

        # Continued bytes before newline are discarded
        bridge.feed_usb(b"DISCARDED_BYTES")
        self.assertEqual(bridge.status()["uart_pending"], 0)

        # Newline triggers recovery
        bridge.feed_usb(b"\r\n")

        # Now send a valid command
        bridge.feed_usb(b"AT\r\n")
        bridge.tick()
        self.assertEqual(b"".join(self.uart.writes), b"AT\r\n")

    def test_uart_backpressure_busy(self):
        # Small uart buffer to test backpressure
        bridge = ATBridge(self.uart, self.usb, max_uart_buffer=25)
        bridge.feed_usb(b"AT+ADDRESS=2\r\n")  # 14 bytes (fits)
        bridge.feed_usb(b"AT+NETWORKID=18\r\n")  # 17 bytes (14+17 = 31 > 25, rejected)
        bridge.tick()

        self.assertEqual(bridge.status()["uart_overflow"], 1)
        self.assertIn(b"+ERR=BRIDGE_BUSY\r\n", b"".join(self.usb.writes))
        # First command was retained and written
        self.assertEqual(b"".join(self.uart.writes), b"AT+ADDRESS=2\r\n")

    def test_modem_responses_forwarded_unchanged(self):
        responses = [
            b"+OK\r\n",
            b"+ERR=4\r\n",
            b"+RCV=1,36,SGVsbG8gV29ybGQgTG9SYSBNb2RlbQ==,-65,9.5\r\n",
            b"+ADDRESS=2\r\n",
            b"+READY\r\n",
        ]
        for resp in responses:
            self.bridge.feed_modem(resp)
        self.bridge.tick()

        self.assertEqual(b"".join(self.usb.writes), b"".join(responses))

    def test_host_stdin_machine(self):
        machine = HostStdinMachine(self.bridge, self.usb)

        # Host sends AT command in 1-byte fragments through stdin
        machine.host_send(b"AT+SEND=1,5,WORLD\r\n")
        machine.step_all(chunk_size=1)

        # UART received the command
        self.assertEqual(b"".join(self.uart.writes), b"AT+SEND=1,5,WORLD\r\n")

        # Modem replies with +OK
        self.bridge.feed_modem(b"+OK\r\n")
        self.bridge.tick()

        # Host receives +OK on stdout
        received = machine.host_receive()
        self.assertEqual(received, b"+OK\r\n")


if __name__ == "__main__":
    unittest.main()
