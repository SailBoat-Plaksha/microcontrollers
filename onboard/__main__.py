"""Shared service and persistent raw-MAVLink stdio bridge entry point."""

from __future__ import annotations

import argparse
import fcntl
import os
from pathlib import Path
import queue
import signal
import struct
import sys
import threading
import time

from boat.ids import MAILBOX_FRAME
from protocol import codec
from protocol.codec import FrameStream

from .bus import SMBus2Adapter, SimulatedBus
from .config import ServiceConfig
from .router import BoatRouter


def _sim_bus(config: ServiceConfig) -> SimulatedBus:
    samples = {}
    if config.imu:
        # Labeled fixture values only; these are not physical measurements.
        values = (0, 0, 1000, 0, 0, 0)
        order = "<" if config.imu.byte_order == "little" else ">"
        samples[config.imu.address] = struct.pack(f"{order}6h", *values)
    return SimulatedBus(present=(config.motor_address, config.comms_address), imu_samples=samples)


def _lock_bus() -> object:
    path = Path(f"/tmp/arsem-onboard-bus-{os.getuid()}.lock")
    handle = path.open("a+")
    try:
        fcntl.flock(handle.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError as exc:
        handle.close()
        raise RuntimeError("another onboard process already owns this I2C bus") from exc
    return handle


def _run(args: argparse.Namespace) -> int:
    config = ServiceConfig.from_toml(args.config) if args.config else ServiceConfig()
    lock = _lock_bus()
    bus = _sim_bus(config) if args.simulate else SMBus2Adapter(config.bus_number)
    stopping = False

    def stop_signal(_signum, _frame):
        nonlocal stopping
        stopping = True

    signal.signal(signal.SIGINT, stop_signal)
    signal.signal(signal.SIGTERM, stop_signal)
    simulated = " SIMULATED" if args.simulate else ""
    print(f"onboard service started{simulated}; bus {config.bus_number} at {config.bus_hz} Hz",
          file=sys.stderr, flush=True)

    def on_frame(frame: bytes) -> None:
        if args.command == "bridge":
            # The service thread never waits for a slow SSH consumer. The
            # dedicated writer owns blocking stdout; priority traffic can evict
            # routine telemetry when the bounded queue fills.
            _enqueue_stdout(outgoing, frame)

    def on_event(event) -> None:
        if event.kind in ("bus_error", "imu_error", "rejected", "queue_drop"):
            print(f"{event.kind}: {event.detail}", file=sys.stderr, flush=True)
        elif event.kind == "imu_sample" and args.simulate:
            print("SIMULATED IMU sample (not a physical measurement)", file=sys.stderr, flush=True)

    outgoing = queue.Queue(maxsize=1024)
    if args.command == "bridge":
        threading.Thread(target=_write_stdout, args=(outgoing,),
                         name="mavlink-stdout", daemon=True).start()
    router = BoatRouter(bus, config, on_frame=on_frame, on_event=on_event)
    try:
        if args.command == "serve":
            router.run_forever(stop=lambda: stopping)
        else:
            _run_bridge(router, lambda: stopping)
    finally:
        router.close()
        fcntl.flock(lock.fileno(), fcntl.LOCK_UN)
        lock.close()
    return 0


def _run_bridge(router: BoatRouter, stopping) -> None:
    stream = FrameStream()
    stdin = sys.stdin.buffer
    incoming: queue.Queue[bytes | None] = queue.Queue(maxsize=32)

    def read_stdin() -> None:
        try:
            while True:
                data = os.read(stdin.fileno(), MAILBOX_FRAME * 8)
                while not stopping():
                    try:
                        incoming.put(data if data else None, timeout=0.05)
                        break
                    except queue.Full:
                        continue
                if not data:
                    return
        except OSError as exc:
            print(f"stdio read failed: {exc}", file=sys.stderr, flush=True)
            try:
                incoming.put_nowait(None)
            except queue.Full:
                pass

    threading.Thread(target=read_stdin, name="mavlink-stdin", daemon=True).start()
    while not stopping():
        try:
            data = incoming.get(timeout=0.005)
        except queue.Empty:
            data = b""
        if data is None:
            return
        for frame, _decoded in stream.feed(data):
            router.submit_frame(frame, ingress="ssh")
        router.step()


def _frame_priority(frame: bytes) -> int:
    try:
        name = codec.decode(frame)["name"]
    except (ValueError, KeyError, struct.error):
        return 1
    return 0 if name in ("REQUEST_RESULT", "ACTUATOR_STATUS", "NODE_HEALTH", "HEARTBEAT") else 1


def _enqueue_stdout(outgoing: queue.Queue, frame: bytes) -> None:
    payload = bytes(frame)
    priority = _frame_priority(payload)
    with outgoing.mutex:
        items = outgoing.queue
        if len(items) < outgoing.maxsize:
            if priority == 0:
                index = next((i for i, item in enumerate(items)
                              if _frame_priority(item) > 0), len(items))
                items.insert(index, payload)
            else:
                items.append(payload)
            outgoing.not_empty.notify()
            return
        if priority == 0:
            routine = next((index for index, item in enumerate(items)
                            if _frame_priority(item) > 0), None)
            if routine is not None:
                del items[routine]
                index = next((i for i, item in enumerate(items)
                              if _frame_priority(item) > 0), len(items))
                items.insert(index, payload)
                outgoing.not_empty.notify()
                return
            # Coalesce repeated persistent status snapshots and the latest state
            # for a correlated request while keeping the queue bounded.
            incoming = codec.decode(payload)
            identity = _stdout_identity(incoming)
            for index, item in enumerate(items):
                try:
                    if _stdout_identity(codec.decode(item)) == identity:
                        items[index] = payload
                        return
                except (ValueError, KeyError):
                    continue
    try:
        print("bridge stdout queue full; response could not be queued", file=sys.stderr, flush=True)
    except OSError:
        pass


def _stdout_identity(message: dict) -> tuple:
    name = message["name"]
    source = (message["system"], message["component"])
    if name == "REQUEST_RESULT":
        fields = message["fields"]
        return (name, fields["ground_session"], fields["request_id"])
    if name == "ACTUATOR_STATUS":
        return (name, source, message["fields"]["actuator"])
    return (name, source)


def _write_stdout(outgoing: queue.Queue) -> None:
    while True:
        frame = outgoing.get()
        try:
            # stdout is a binary MAVLink stream: no framing, labels, or log text.
            sys.stdout.buffer.write(frame)
            sys.stdout.buffer.flush()
        except OSError as exc:
            print(f"stdio write failed: {exc}", file=sys.stderr, flush=True)
            return


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="python -m onboard")
    parser.add_argument("command", choices=("serve", "bridge"),
                        help="serve the bus continuously, or bridge raw MAVLink stdin/stdout over SSH")
    parser.add_argument("--config", help="TOML bus and IMU register configuration")
    parser.add_argument("--simulate", action="store_true",
                        help="use injectable simulated peers and visibly labeled simulated IMU values")
    args = parser.parse_args(argv)
    try:
        return _run(args)
    except (OSError, RuntimeError, ValueError, TypeError) as exc:
        print(f"onboard: {exc}", file=sys.stderr, flush=True)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
