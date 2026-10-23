import io
import threading
import time
import unittest

from boat.client import BoatClient, Heartbeat, ImuSample, MotorReport, PongReply
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
        self.assertEqual(argv[-3:], ["python3", "-m", "onboard.bridge"])
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
        client.ping_all()
        deadline = time.monotonic() + 1
        while time.monotonic() < deadline and not any(isinstance(e, MotorReport) for e in events):
            time.sleep(0.01)
        self.assertEqual(len([e for e in events if isinstance(e, PongReply)]), 3)
        self.assertTrue(any(isinstance(e, Heartbeat) for e in events))
        self.assertTrue(any(isinstance(e, ImuSample) and e.source == (1, 1) for e in events))
        self.assertTrue(any(isinstance(e, MotorReport) for e in events))
        ticket = client.set_absolute_angle(0, 1234)
        deadline = time.monotonic() + 1
        while time.monotonic() < deadline and not any(
                getattr(e, "request_id", None) == ticket.request_id for e in events):
            time.sleep(0.01)
        status = [e for e in events if isinstance(e, MotorReport)][-1]
        self.assertEqual(status.measured_position_cdeg, 1234)
        self.assertEqual(status.source, (1, 154))
        client.disconnect()

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
