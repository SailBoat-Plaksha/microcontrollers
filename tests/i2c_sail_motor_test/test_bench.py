"""Host verification of actual sketch logic, using generated Python/C MAVLink."""
import subprocess
import unittest
from pathlib import Path
from unittest.mock import patch

import arsem_test as dialect
from pi_send import command_frame, decode_status, run_command

ROOT = Path(__file__).resolve().parent


def custom_frame(**changes):
    fields = dict(command_id=1, duration_ms=500, target_system=1,
                  target_component=154, direction=1, pwm=96)
    fields.update(changes)
    mav = dialect.MAVLink(None, srcSystem=1, srcComponent=1)
    return dialect.MAVLink_sail_motor_test_message(**fields).pack(mav)


class SketchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        build = ROOT / ".build/host"
        build.mkdir(parents=True, exist_ok=True)
        cls.binary = build / "probe"
        subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        "-I", str(ROOT / "host"), str(ROOT / "host/probe.cpp"),
                        "-o", str(cls.binary)], check=True)

    def simulate(self, instructions):
        result = subprocess.run([str(self.binary)], input=instructions, text=True,
                                capture_output=True, check=True)
        return [decode_status(bytes.fromhex(line)) for line in result.stdout.splitlines()]

    def test_forward_backward_and_local_timeout_without_more_i2c(self):
        for direction in (1, -1):
            frame = command_frame(1, direction, 500, 96)
            states = self.simulate(f"write {frame.hex()} tick read at 499 tick read at 500 tick read")
            self.assertEqual([(s.state, s.direction, s.pwm, s.remaining_ms) for s in states],
                             [(1, direction, 96, 500), (1, direction, 96, 1), (2, 0, 0, 0)])
            self.assertEqual(states[-1].elapsed_ms, 500)

    def test_invalid_fields_and_target_never_energise(self):
        for fields, expected in (({"duration_ms": 0}, 1), ({"duration_ms": 5001}, 1),
                                 ({"pwm": 0}, 1), ({"pwm": 129}, 1),
                                 ({"direction": 2}, 1), ({"target_component": 153}, 6),
                                 ({"target_system": 0}, 6), ({"command_id": 0}, 3)):
            with self.subTest(fields=fields):
                frame = custom_frame(**fields)
                status, = self.simulate(f"write {frame.hex()} tick read")
                self.assertEqual((status.state, status.pwm, status.command_id, status.result),
                                 (0, 0, 0, expected))

    def test_corruption_partial_extra_and_signed_frames_rejected(self):
        frame = bytearray(command_frame(1, 1, 500, 96))
        corrupt = frame.copy(); corrupt[10] ^= 1
        signed = frame.copy(); signed[2] = 1
        for invalid in (corrupt, frame[:-1], frame + b"\x00", signed):
            status, = self.simulate(f"write {invalid.hex()} tick read")
            self.assertEqual((status.state, status.pwm, status.result), (0, 0, 4))

    def test_duplicate_does_not_extend_deadline_and_busy_move_rejected(self):
        forward = command_frame(1, 1, 500, 96).hex()
        backward = command_frame(2, -1, 500, 96).hex()
        states = self.simulate(f"write {forward} tick at 200 write {forward} tick read "
                               f"write {backward} tick read at 500 tick read")
        self.assertEqual([s.result for s in states[:2]], [3, 2])
        self.assertEqual([s.command_id for s in states], [1, 1, 1])
        self.assertEqual((states[-1].state, states[-1].elapsed_ms), (2, 500))

    def test_stop_interrupts_move_and_allows_later_reverse(self):
        forward = command_frame(1, 1, 500, 96).hex()
        stop = command_frame(2, 0).hex()
        reverse = command_frame(3, -1, 500, 96).hex()
        states = self.simulate(f"write {forward} tick at 100 write {stop} tick read "
                               f"at 1100 write {reverse} tick read at 1600 tick read")
        self.assertEqual((states[0].state, states[0].elapsed_ms, states[0].pwm), (3, 100, 0))
        self.assertEqual((states[1].state, states[1].direction), (1, -1))
        self.assertEqual((states[2].state, states[2].command_id), (2, 3))

    def test_receive_overflow_stops_and_discards_queued_command(self):
        frame = command_frame(1, 1, 500, 96).hex()
        next_frame = command_frame(2, 1, 500, 96).hex()
        status, = self.simulate(f"write {frame} tick at 100 write {next_frame} "
                                f"write {next_frame} tick read")
        self.assertEqual((status.state, status.pwm, status.result, status.command_id), (4, 0, 5, 1))

    def test_millis_rollover(self):
        frame = command_frame(1, 1, 500, 96).hex()
        status, = self.simulate(f"at 4294967040 write {frame} tick at 244 tick read")
        self.assertEqual((status.state, status.elapsed_ms), (2, 500))

    def test_status_crc_and_identity_validated_by_sender(self):
        mav = dialect.MAVLink(None, srcSystem=1, srcComponent=154)
        frame = mav.sail_motor_test_status_encode(0, 0, 0, 0, 0, 0, 0, 0).pack(mav)
        padded = frame.ljust(32, b"\x00")
        self.assertEqual(decode_status(padded).state, 0)
        bad = bytearray(padded); bad[10] ^= 1
        with self.assertRaises((ValueError, dialect.MAVError)): decode_status(bad)

    def test_pi_sender_reads_back_completion_and_writes_exactly_once(self):
        frame = command_frame(1, 1, 500, 96)
        responses = self.simulate(f"read write {frame.hex()} tick read at 500 tick read")

        class Transfer:
            def __init__(self, data=b"", reading=False):
                self.data, self.reading = data, reading
            def __bytes__(self): return bytes(self.data)

        class Messages:
            @staticmethod
            def read(address, length):
                assert address == 8 and length == 32
                return Transfer(reading=True)
            @staticmethod
            def write(address, data):
                assert address == 8
                return Transfer(data)

        class Bus:
            writes = []
            def i2c_rdwr(self, transfer):
                if transfer.reading:
                    status = responses.pop(0)
                    mav = dialect.MAVLink(None, srcSystem=1, srcComponent=154)
                    fields = {k: v for k, v in status.to_dict().items() if k != "mavpackettype"}
                    transfer.data = dialect.MAVLink_sail_motor_test_status_message(**fields).pack(mav).ljust(32, b"\0")
                else: self.writes.append(bytes(transfer))

        bus = Bus()
        with patch("pi_send.time.sleep"), patch("builtins.print"):
            completed = run_command(bus, Messages, 8, 1, 500, 96)
        self.assertEqual(completed.state, 2)
        self.assertEqual(bus.writes, [frame])

    def test_pi_sender_does_not_retry_ambiguous_write_failure(self):
        class Bus:
            writes = 0
            def i2c_rdwr(self, transfer):
                self.writes += 1
                raise OSError("I2C write failed, delivery unknown")
        class Messages:
            @staticmethod
            def write(address, data): return data
        initial, = self.simulate("read")
        bus = Bus()
        with patch("pi_send.read_status", return_value=initial), patch("builtins.print"):
            with self.assertRaises(OSError): run_command(bus, Messages, 8, 1, 500, 96)
        self.assertEqual(bus.writes, 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)
