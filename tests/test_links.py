import importlib.util
import io
import re
import sys
import types
import binascii
import subprocess
import tempfile
import unittest
from pathlib import Path

from protocol.codec import FrameStream, MESSAGES, decode, encode, radio_command, radio_receive

ROOT = Path(__file__).resolve().parents[1]
FIELDS = dict(command_id=42, position_cdeg=9000, timeout_ms=1000,
              max_rate_cdeg_s=0, target_system=1, target_component=154,
              actuator=1, control_source=2)


class PacketLinks(unittest.TestCase):
    def test_dialect_layout_and_known_common_crc(self):
        self.assertEqual(MESSAGES[0]["crc_extra"], 50)
        self.assertEqual(MESSAGES[200001]["crc_extra"], 49)
        self.assertEqual(len(MESSAGES[200001]["fields"]), 8)
        source = (ROOT / "protocol/mavlink_min.c").read_text()
        table = re.search(r"custom\[\] = \{([^}]+)\}", source).group(1)
        self.assertEqual([int(value) for value in table.split(",")],
                         [MESSAGES[id_]["crc_extra"] for id_ in range(200000, 200008)])

    def test_serial_fragmentation_and_resynchronization(self):
        frames = [encode("ACTUATOR_SETPOINT", {**FIELDS, "command_id": n}, sequence=n) for n in range(100)]
        stream = FrameStream()
        result = []
        wire = b"noise" + b"".join(frames)
        for index in range(0, len(wire), 3):
            result.extend(stream.feed(wire[index:index + 3]))
        self.assertEqual([frame for frame, _ in result], frames)
        print("SERIAL: 100/100 fragmented frames recovered in order")

    def test_corruption_is_rejected(self):
        frame = encode("ACTUATOR_SETPOINT", FIELDS)
        corrupt = bytearray(frame)
        corrupt[13] ^= 0x20
        with self.assertRaisesRegex(ValueError, "CRC"):
            decode(corrupt)
        self.assertEqual(FrameStream().feed(corrupt), [])
        print("INTEGRITY: changed payload rejected by CRC")

    def test_lora_round_trip_and_length_limit(self):
        frame = encode("ACTUATOR_SETPOINT", FIELDS)
        command = radio_command(frame)
        encoded = command.strip().split(b",")[2]
        received, message, meta = radio_receive(b"+RCV=7," + str(len(encoded)).encode() +
                                                b"," + encoded + b",-71,9")
        self.assertEqual(received, frame)
        self.assertEqual(message["fields"]["position_cdeg"], 9000)
        self.assertEqual(meta["rssi"], -71)
        with self.assertRaisesRegex(ValueError, "length mismatch"):
            radio_receive(b"+RCV=7,1," + encoded + b",-71,9")
        print(f"LORA: 1/1 complete frame, {len(frame)} binary bytes -> {len(encoded)} ASCII bytes")

    def test_100_radio_frames_and_corruptions(self):
        accepted = 0
        rejected = 0
        for number in range(100):
            frame = encode("ACTUATOR_SETPOINT", {**FIELDS, "command_id": number + 1}, sequence=number)
            text = radio_command(frame).strip().split(b",")[2]
            line = b"+RCV=2," + str(len(text)).encode() + b"," + text + b",-80,5"
            self.assertEqual(radio_receive(line)[0], frame)
            accepted += 1
            broken = bytearray(frame)
            broken[13] ^= 1
            import base64
            damaged_text = base64.b64encode(broken)
            damaged_line = b"+RCV=2," + str(len(damaged_text)).encode() + b"," + damaged_text + b",-80,5"
            with self.assertRaisesRegex(ValueError, "CRC"):
                radio_receive(damaged_line)
            rejected += 1
        self.assertEqual((accepted, rejected), (100, 100))
        print("LORA: 100/100 intact frames delivered; 100/100 damaged frames rejected")

    def test_micropython_gateway_accepts_same_frame(self):
        fake_machine = types.ModuleType("machine")
        fake_machine.UART = object
        fake_machine.Pin = object
        fake_time = types.ModuleType("utime")
        fake_time.sleep_ms = lambda _: None
        saved = {name: sys.modules.get(name) for name in ("machine", "ubinascii", "utime")}
        try:
            sys.modules.update(machine=fake_machine, ubinascii=binascii, utime=fake_time)
            spec = importlib.util.spec_from_file_location("base_gateway", ROOT / "Base-Station-LoRA/main.py")
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            frame = encode("ACTUATOR_SETPOINT", FIELDS)
            import base64
            payload = base64.b64encode(frame)
            line = b"+RCV=2," + str(len(payload)).encode() + b"," + payload + b",-70,6"
            self.assertEqual(module.radio_line_to_frame(line), frame)
            self.assertTrue(module.valid(frame))
            self.assertIsNone(module.radio_line_to_frame(line.replace(b"+RCV=2,", b"+RCV=2,1,")))
            damaged = bytearray(frame)
            damaged[13] ^= 1
            self.assertFalse(module.valid(damaged))
        finally:
            for name, original in saved.items():
                if original is None:
                    sys.modules.pop(name, None)
                else:
                    sys.modules[name] = original
        print("MICROPYTHON GATEWAY: same frame and CRC accepted; damage rejected")

    def test_master_routes_exact_frame(self):
        spec = importlib.util.spec_from_file_location("master", ROOT / "Master/master.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        output = io.BytesIO()
        frame = encode("ACTUATOR_SETPOINT", FIELDS)
        self.assertEqual(module.route(frame[:5], FrameStream(), output), [])
        parser = FrameStream()
        module.route(frame[:5], parser, output)
        result = module.route(frame[5:], parser, output)
        self.assertEqual(output.getvalue(), frame)
        self.assertEqual(result[0]["name"], "ACTUATOR_SETPOINT")
        print("PI ROUTER: original MAVLink bytes forwarded unchanged")

    def test_c_and_python_interoperate(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = Path(directory) / "host_probe"
            command = ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "protocol"),
                       str(ROOT / "tests/host_probe.c"), str(ROOT / "protocol/mavlink_min.c"),
                       str(ROOT / "protocol/actuator_command.c"), "-o", str(binary)]
            subprocess.run(command, check=True, capture_output=True, text=True)
            result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
        lines = result.stdout.splitlines()
        self.assertEqual(bytes.fromhex(lines[0]), encode("ACTUATOR_SETPOINT", FIELDS))
        self.assertIn("PASS", lines[1])
        print("C/PYTHON: identical MAVLink frame, parser and actuator checks PASS")


if __name__ == "__main__":
    unittest.main(verbosity=2)
