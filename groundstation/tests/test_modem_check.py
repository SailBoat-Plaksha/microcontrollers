import io
import json
import threading
import unittest
from contextlib import redirect_stdout

from groundstation import modem_check


class FakeSerial:
    def __init__(
        self,
        port,
        baudrate,
        *,
        timeout=0.1,
        write_timeout=1.0,
        timeout_commands=(),
        error_commands=None,
        health_overrides=None,
    ):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.write_timeout = write_timeout
        self.timeout_commands = set(timeout_commands)
        self.error_commands = dict(error_commands or {})
        self.health_overrides = dict(health_overrides or {})
        self.commands: list[bytes] = []
        self.rx = bytearray()
        self.cv = threading.Condition()
        self.closed = False
        self.wire = bytearray()
        self.settings = {
            "address": "2",
            "network": "18",
            "band": "915000000",
            "parameter": "10,7,1,12",
        }

    def read(self, count=256) -> bytes:
        with self.cv:
            if not self.rx and not self.closed:
                self.cv.wait(0.05)
            data = bytes(self.rx[:count])
            del self.rx[:count]
            return data

    def write(self, data: bytes) -> int:
        data = bytes(data)
        with self.cv:
            self.wire.extend(data)
            while b"\n" in self.wire:
                line, rest = self.wire.split(b"\n", 1)
                self.wire = bytearray(rest)
                cmd = bytes(line).rstrip(b"\r")
                self.commands.append(cmd)
                if cmd in self.timeout_commands:
                    continue
                if cmd in self.error_commands:
                    err_resp = self.error_commands[cmd]
                    self.rx.extend(err_resp + b"\r\n")
                    self.cv.notify_all()
                    continue

                if cmd.startswith(b"AT+ADDRESS="):
                    self.settings["address"] = cmd.split(b"=", 1)[1].decode()
                    self.rx.extend(b"+OK\r\n")
                elif cmd.startswith(b"AT+NETWORKID="):
                    self.settings["network"] = cmd.split(b"=", 1)[1].decode()
                    self.rx.extend(b"+OK\r\n")
                elif cmd.startswith(b"AT+BAND="):
                    self.settings["band"] = cmd.split(b"=", 1)[1].decode()
                    self.rx.extend(b"+OK\r\n")
                elif cmd.startswith(b"AT+PARAMETER="):
                    self.settings["parameter"] = cmd.split(b"=", 1)[1].decode()
                    self.rx.extend(b"+OK\r\n")
                elif cmd == b"AT":
                    self.rx.extend(b"+OK\r\n")
                elif cmd == b"AT+ADDRESS?":
                    resp = self.health_overrides.get(cmd, f"+ADDRESS={self.settings['address']}".encode())
                    self.rx.extend(resp + b"\r\n")
                elif cmd == b"AT+NETWORKID?":
                    resp = self.health_overrides.get(cmd, f"+NETWORK={self.settings['network']}".encode())
                    self.rx.extend(resp + b"\r\n")
                elif cmd == b"AT+BAND?":
                    resp = self.health_overrides.get(cmd, f"+BAND={self.settings['band']}".encode())
                    self.rx.extend(resp + b"\r\n")
                elif cmd == b"AT+PARAMETER?":
                    resp = self.health_overrides.get(cmd, f"+PARAMETER={self.settings['parameter']}".encode())
                    self.rx.extend(resp + b"\r\n")
                elif cmd == b"AT+VER?":
                    resp = self.health_overrides.get(cmd, b"+VER=RYLR998_V1.0")
                    self.rx.extend(resp + b"\r\n")
                elif cmd == b"AT+UID?":
                    resp = self.health_overrides.get(cmd, b"+UID=0123456789AB")
                    self.rx.extend(resp + b"\r\n")
                else:
                    self.rx.extend(b"+OK\r\n")
                self.cv.notify_all()
        return len(data)

    def flush(self):
        pass

    def close(self):
        with self.cv:
            self.closed = True
            self.cv.notify_all()


class ModemCheckTests(unittest.TestCase):
    def test_modem_check_success_and_proves_no_at_send(self):
        instances: list[FakeSerial] = []

        def factory(port, baudrate, **kwargs):
            inst = FakeSerial(port, baudrate, **kwargs)
            instances.append(inst)
            return inst

        report = modem_check.verify_modem(
            port="/dev/ttyACM0",
            baudrate=115200,
            command_timeout_s=0.2,
            local_address=2,
            network_id=18,
            parameter="10,7,1,12",
            serial_factory=factory,
        )

        self.assertTrue(report["healthy"])
        self.assertEqual(len(instances), 1)
        fake = instances[0]
        self.assertTrue(fake.closed)

        # Verify configuration commands and AT queries were performed
        self.assertIn(b"AT+ADDRESS=2", fake.commands)
        self.assertIn(b"AT+NETWORKID=18", fake.commands)
        self.assertIn(b"AT+PARAMETER=10,7,1,12", fake.commands)
        self.assertIn(b"AT", fake.commands)
        self.assertIn(b"AT+ADDRESS?", fake.commands)
        self.assertIn(b"AT+NETWORKID?", fake.commands)
        self.assertIn(b"AT+BAND?", fake.commands)
        self.assertIn(b"AT+PARAMETER?", fake.commands)
        self.assertIn(b"AT+VER?", fake.commands)
        self.assertIn(b"AT+UID?", fake.commands)

        # CRITICAL SAFETY: Prove ABSOLUTELY NO AT+SEND or MAVLink transmission occurred
        for cmd in fake.commands:
            self.assertFalse(
                cmd.startswith(b"AT+SEND"),
                f"Forbidden AT+SEND command was issued during modem check: {cmd!r}",
            )
            self.assertNotIn(b"+SEND", cmd)

    def test_cli_human_output_and_disclaimer_and_band_disclosure(self):
        instances: list[FakeSerial] = []

        def factory(port, baudrate, **kwargs):
            inst = FakeSerial(port, baudrate, **kwargs)
            instances.append(inst)
            return inst

        buf = io.StringIO()
        with redirect_stdout(buf):
            code = modem_check.main(
                ["--port", "/dev/ttyACM0", "--baud", "115200", "--timeout", "0.2"],
                serial_factory=factory,
            )

        self.assertEqual(code, 0)
        output = buf.getvalue()

        # Check required disclaimer
        self.assertIn(
            "Checking the ground modem confirms local USB-UART AT communication and configuration only",
            output,
        )
        self.assertIn("does NOT prove boat RF reachability", output)

        # Check band disclosure
        self.assertIn("blank/omitted - existing modem stored band left unchanged", output)

        # Check actual AT settings reported
        self.assertIn("Local modem reachable: YES (+OK)", output)
        self.assertIn("Address:    2", output)
        self.assertIn("Network ID: 18", output)
        self.assertIn("Band:       915000000 Hz", output)
        self.assertIn("Parameters: 10,7,1,12", output)
        self.assertIn("Firmware:   RYLR998_V1.0", output)
        self.assertIn("UID:        0123456789AB", output)
        self.assertIn("STATUS: HEALTHY", output)

    def test_cli_json_output(self):
        instances: list[FakeSerial] = []

        def factory(port, baudrate, **kwargs):
            inst = FakeSerial(port, baudrate, **kwargs)
            instances.append(inst)
            return inst

        buf = io.StringIO()
        with redirect_stdout(buf):
            code = modem_check.main(
                ["--port", "/dev/ttyACM0", "--timeout", "0.2", "--json"],
                serial_factory=factory,
            )

        self.assertEqual(code, 0)
        data = json.loads(buf.getvalue())
        self.assertTrue(data["healthy"])
        self.assertTrue(data["reachable"])
        self.assertEqual(data["port"], "/dev/ttyACM0")
        self.assertIn("does NOT prove boat RF reachability", data["disclaimer"])
        self.assertIn("Blank band leaves modem frequency unchanged", data["band_disclosure"])
        self.assertEqual(data["responses"]["version"], "+VER=RYLR998_V1.0")
        self.assertTrue(data["checks"]["command_probe"])
        self.assertTrue(data["checks"]["address_matches"])

    def test_timeout_cleans_resources_and_exits_nonzero(self):
        instances: list[FakeSerial] = []

        def factory(port, baudrate, **kwargs):
            # Time out on AT probe
            inst = FakeSerial(port, baudrate, timeout_commands={b"AT"}, **kwargs)
            instances.append(inst)
            return inst

        buf = io.StringIO()
        with redirect_stdout(buf):
            code = modem_check.main(
                ["--port", "/dev/ttyACM0", "--timeout", "0.1"],
                serial_factory=factory,
            )

        self.assertNotEqual(code, 0)
        output = buf.getvalue()
        self.assertIn("RESULT: FAILED", output)
        self.assertIn("TimeoutError", output)
        self.assertEqual(len(instances), 1)
        self.assertTrue(instances[0].closed)

    def test_timeout_json_output_nonzero(self):
        instances: list[FakeSerial] = []

        def factory(port, baudrate, **kwargs):
            inst = FakeSerial(port, baudrate, timeout_commands={b"AT"}, **kwargs)
            instances.append(inst)
            return inst

        buf = io.StringIO()
        with redirect_stdout(buf):
            code = modem_check.main(
                ["--port", "/dev/ttyACM0", "--timeout", "0.1", "--json"],
                serial_factory=factory,
            )

        self.assertNotEqual(code, 0)
        data = json.loads(buf.getvalue())
        self.assertFalse(data["healthy"])
        self.assertFalse(data["reachable"])
        self.assertIn("TimeoutError", data["error"])

    def test_settings_mismatch_returns_nonzero(self):
        instances: list[FakeSerial] = []

        def factory(port, baudrate, **kwargs):
            # Address reports 99 instead of 2
            inst = FakeSerial(
                port,
                baudrate,
                health_overrides={b"AT+ADDRESS?": b"+ADDRESS=99"},
                **kwargs,
            )
            instances.append(inst)
            return inst

        buf = io.StringIO()
        with redirect_stdout(buf):
            code = modem_check.main(
                ["--port", "/dev/ttyACM0", "--timeout", "0.2"],
                serial_factory=factory,
            )

        self.assertNotEqual(code, 0)
        output = buf.getvalue()
        self.assertIn("STATUS: FAILED", output)
        self.assertIn("address_matches: FAIL", output)
        self.assertEqual(len(instances), 1)
        self.assertTrue(instances[0].closed)


if __name__ == "__main__":
    unittest.main()
