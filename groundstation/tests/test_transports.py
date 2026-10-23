import io
import threading
import time
import unittest

from boat.client import (BoatClient, Heartbeat, ImuSample, MotorReport, PongReply,
                         RequestOutcome)
from protocol import codec
from groundstation.transports import SerialTransport, SimulationTransport, SSHTransport


def heartbeat():
    return codec.encode("HEARTBEAT", {"type": 6, "autopilot": 8, "base_mode": 0,
        "custom_mode": 0, "system_status": 4, "mavlink_version": 3}, system=1, component=154)


class FakeSerial:
    def __init__(self, *args, **kwargs):
        self.args, self.kwargs = args, kwargs
        self.chunks = [heartbeat()[:4], heartbeat()[4:]]
        self.writes = []
        self.closed = False

    def read(self, count):
        if self.chunks:
            return self.chunks.pop(0)
        time.sleep(0.005)
        return b""

    def write(self, data):
        self.writes.append(bytes(data))
        return len(data)

    def flush(self):
        pass

    def close(self):
        self.closed = True


class BlockingBytes:
    def __init__(self):
        self.closed = threading.Event()

    def read(self, count):
        self.closed.wait(1)
        return b""

    def readline(self):
        self.closed.wait(1)
        return b""


class FakeProcess:
    def __init__(self, argv, **kwargs):
        self.argv, self.kwargs = argv, kwargs
        self.stdin = io.BytesIO()
        self.stdout, self.stderr = BlockingBytes(), BlockingBytes()
        self.returncode = None

    def poll(self):
        return self.returncode

    def wait(self, timeout=None):
        self.stdout.closed.set()
        self.stderr.closed.set()
        self.returncode = 0
        return 0

    def terminate(self):
        self.returncode = 0

    def kill(self):
        self.returncode = -9


class TransportTests(unittest.TestCase):
    def test_serial_reassembles_fragmented_frames_and_writes_raw_frame(self):
        serial_holder = []
        received = threading.Event()
        frames = []

        def factory(*args, **kwargs):
            obj = FakeSerial(*args, **kwargs)
            serial_holder.append(obj)
            return obj

        transport = SerialTransport("fake-port", serial_factory=factory)
        transport.open(lambda frame: (frames.append(frame), received.set()))
        self.assertTrue(received.wait(1))
        self.assertEqual(codec.decode(frames[0])["name"], "HEARTBEAT")
        outgoing = codec.encode("PING", {"time_usec": 1, "seq": 2,
            "target_system": 0, "target_component": 0})
        transport.send_frame(outgoing)
        self.assertEqual(serial_holder[0].writes, [outgoing])
        self.assertEqual(serial_holder[0].kwargs["timeout"], 0.1)
        transport.close()
        self.assertTrue(serial_holder[0].closed)

    def test_ssh_uses_fixed_argv_and_rejects_shell_input(self):
        processes = []

        def popen(argv, **kwargs):
            process = FakeProcess(argv, **kwargs)
            processes.append(process)
            return process

        with self.assertRaises(ValueError):
            SSHTransport("host; rm -rf /")
        transport = SSHTransport("boat-pi", popen_factory=popen)
        transport.open(lambda frame: None)
        argv = processes[0].argv
        self.assertIn("StrictHostKeyChecking=yes", argv)
        self.assertEqual(argv[-6:], ["python3", "-m", "onboard", "bridge", "--config",
                                     "/etc/arsem/onboard.toml"])
        self.assertEqual(argv[0], "ssh")
        self.assertEqual(processes[0].kwargs["stderr"], -1)
        outgoing = codec.encode("PING", {"time_usec": 1, "seq": 2,
            "target_system": 0, "target_component": 0})
        transport.send_frame(outgoing)
        self.assertEqual(processes[0].stdin.getvalue(), outgoing)
        transport.close()

    def test_simulation_uses_codec_and_returns_typed_observations(self):
        transport = SimulationTransport(response_delay=0)
        client = BoatClient(transport)
        events = []
        client.subscribe(events.append)
        client.connect()
        self.addCleanup(client.disconnect)
        client.ping_all()
        deadline = time.monotonic() + 1
        while time.monotonic() < deadline and len([e for e in events if isinstance(e, PongReply)]) < 3:
            time.sleep(0.01)
        self.assertEqual(len([e for e in events if isinstance(e, PongReply)]), 3)
        self.assertTrue(any(isinstance(e, Heartbeat) for e in events))
        self.assertTrue(any(isinstance(e, ImuSample) and e.source == (1, 1) for e in events))
        self.assertTrue(any(isinstance(e, MotorReport) for e in events))
        ticket = client.set_absolute_angle(1, 1234)
        deadline = time.monotonic() + 1
        while time.monotonic() < deadline and not any(
                isinstance(e, MotorReport) and e.actuator == 1 and
                e.measured_position_cdeg == 1234 for e in events):
            time.sleep(0.01)
        status = [e for e in events if isinstance(e, MotorReport) and e.actuator == 1][-1]
        self.assertEqual(status.measured_position_cdeg, 1234)
        self.assertEqual(status.source, (1, 154))
        self.assertTrue({2, 3, 4}.issubset({e.state for e in events
                                             if isinstance(e, RequestOutcome) and
                                             e.request_id == ticket.request_id}))
        client.set_absolute_angle(2, -500)
        deadline = time.monotonic() + 1
        while time.monotonic() < deadline and not any(
                isinstance(e, MotorReport) and e.actuator == 2 and
                e.measured_position_cdeg == -500 for e in events):
            time.sleep(0.01)
        self.assertTrue(any(isinstance(e, MotorReport) and e.actuator == 1 and
                            e.measured_position_cdeg == 1234 for e in events))
        self.assertTrue(any(isinstance(e, MotorReport) and e.actuator == 2 and
                            e.measured_position_cdeg == -500 for e in events))
        client.disconnect()

    def test_simulation_periodic_telemetry_and_reconnect_discards_delayed_events(self):
        transport = SimulationTransport(response_delay=0.08, telemetry_interval=0.05)
        events = []
        first = lambda frame: events.append(("old", frame))
        transport.open(first)
        transport.send_frame(codec.encode("PING", {"time_usec": 1, "seq": 2,
            "target_system": 0, "target_component": 0}))
        old_boot = transport._boot_id
        transport.close()
        events.clear()
        transport.open(lambda frame: events.append(("new", frame)))
        self.assertNotEqual(old_boot, transport._boot_id)
        time.sleep(0.12)
        self.assertFalse(any(owner == "old" for owner, _ in events))
        self.assertTrue(any(owner == "new" for owner, _ in events))
        transport.close()

    def test_ssh_short_write_is_reported_as_uncertain_without_replay(self):
        class ShortWriter:
            def __init__(self):
                self.calls = 0
            def write(self, data):
                self.calls += 1
                return len(data) - 1
            def flush(self):
                pass
            def close(self):
                pass
        def popen(argv, **kwargs):
            proc = FakeProcess(argv, **kwargs)
            proc.stdin = ShortWriter()
            return proc
        transport = SSHTransport("boat-pi", popen_factory=popen)
        transport.open(lambda frame: None)
        outgoing = codec.encode("PING", {"time_usec": 1, "seq": 2,
            "target_system": 0, "target_component": 0})
        with self.assertRaisesRegex(OSError, "outcome uncertain"):
            transport.send_frame(outgoing)
        self.assertEqual(transport._proc.stdin.calls, 1)
        transport.close()

    def test_simulation_open_and_ambiguous_send_failures_are_visible(self):
        fail_open = BoatClient(SimulationTransport(fail_open=True))
        with self.assertRaises(OSError):
            fail_open.connect()
        transport = SimulationTransport(fail_send=True)
        client = BoatClient(transport)
        client.connect()
        with self.assertRaises(OSError):
            client.ping_all()
        client.disconnect()


if __name__ == "__main__":
    unittest.main()
