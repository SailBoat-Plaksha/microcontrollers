"""Shared service and persistent raw-MAVLink stdio bridge entry point."""

from __future__ import annotations

import argparse
from dataclasses import replace
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
    present = (config.motor_address, config.comms_address) if config.comms_enabled else (config.motor_address,)
    return SimulatedBus(present=present, imu_samples=samples)


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
    if args.command == "radio":
        config = replace(config, comms_enabled=False)
    lock = _lock_bus()
    try:
        bus = _sim_bus(config) if args.simulate else SMBus2Adapter(config.bus_number)
    except BaseException:
        fcntl.flock(lock.fileno(), fcntl.LOCK_UN)
        lock.close()
        raise
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
    transport = None
    try:
        if args.command == "serve":
            router.run_forever(stop=lambda: stopping)
        elif args.command == "radio":
            transport = _make_radio_transport(args)
            _run_radio_service(router, transport, lambda: stopping)
        else:
            _run_bridge(router, lambda: stopping)
    finally:
        try:
            router.close()
        finally:
            fcntl.flock(lock.fileno(), fcntl.LOCK_UN)
            lock.close()
    return 0


def _make_radio_transport(args: argparse.Namespace, transport_factory=None):
    """Load the shared boat transport only for the direct-radio launcher."""
    if transport_factory is None:
        try:
            from boat.radio import LoRaModemTransport
        except ImportError as exc:
            raise RuntimeError("radio mode requires boat.radio.LoRaModemTransport") from exc
        transport_factory = LoRaModemTransport
    return transport_factory(
        args.port, baudrate=args.baud, local_address=args.local_address,
        peer_address=args.peer_address, network_id=args.network_id,
        band_hz=args.band_hz, parameter=args.parameter)


def _run_radio_service(router: BoatRouter, transport, stopping, *, sleep=time.sleep,
                       incoming_limit: int = 32, outgoing_limit: int = 128) -> None:
    """Bridge the modem to the single Pi scheduler without blocking its I2C loop."""
    incoming: queue.Queue[bytes] = queue.Queue(maxsize=incoming_limit)
    outgoing: queue.Queue[bytes] = queue.Queue(maxsize=outgoing_limit)
    disconnect_reason = "radio disconnected; unsent movement cancelled before reconnect"
    previous_on_frame = router.on_frame

    def receive(frame: bytes) -> None:
        payload = bytes(frame)
        # Decode in the callback before assigning STOP priority. The router
        # still performs the full source, target, authority, and policy checks.
        try:
            message = codec.decode(payload)
            fields = message["fields"]
            is_stop = (message["name"] == "ACTUATOR_REQUEST" and
                       fields.get("opcode") == 3)
        except (ValueError, KeyError, TypeError, struct.error):
            is_stop = False
        try:
            incoming.put_nowait(payload)
        except queue.Full:
            if is_stop:
                with incoming.mutex:
                    index = next((i for i, item in enumerate(incoming.queue)
                                  if not _radio_is_stop(item)), None)
                    if index is not None:
                        del incoming.queue[index]
                        incoming.queue.append(payload)
                        incoming.not_empty.notify()
                        return
            print("radio ingress queue full; frame dropped", file=sys.stderr, flush=True)

    def send(frame: bytes) -> None:
        payload = bytes(frame)
        _enqueue_radio(outgoing, payload)

    def report_modem_errors() -> None:
        lines = getattr(transport, "stderr_lines", None)
        if lines is None:
            return
        while True:
            try:
                line = lines.get_nowait()
            except queue.Empty:
                return
            print(f"radio: {line}", file=sys.stderr, flush=True)

    router.on_frame = send
    writer_stop = threading.Event()
    writer_failed = threading.Event()
    writer: threading.Thread | None = None

    def write_radio() -> None:
        while not writer_stop.is_set():
            try:
                response = outgoing.get(timeout=0.05)
            except queue.Empty:
                continue
            if writer_stop.is_set():
                return
            try:
                # send_frame waits for the modem's +OK. Keep that wait off the
                # scheduler thread; a failed/ambiguous frame is never replayed.
                transport.send_frame(response)
            except Exception as exc:
                print(f"radio send failed; outcome may be unknown: {exc}",
                      file=sys.stderr, flush=True)
                print("radio writer stopped; reconnect required", file=sys.stderr, flush=True)
                writer_failed.set()
                return

    try:
        transport.open(receive)
        print("direct radio connected", file=sys.stderr, flush=True)
        writer = threading.Thread(target=write_radio, name="onboard-radio-tx")
        writer.start()
        delay = router.config.poll_interval_ms / 1000.0
        while not stopping() and not writer_failed.is_set():
            report_modem_errors()
            if not transport.is_connected:
                print("radio disconnected; reconnect required", file=sys.stderr, flush=True)
                break
            # Admit the already-buffered burst before one scheduler tick, so a
            # queued STOP reaches the router's priority queue before a pending
            # movement can be written. Bound each pass to its initial size.
            for _index in range(incoming.qsize()):
                try:
                    frame = incoming.get_nowait()
                except queue.Empty:
                    break
                router.submit_frame(frame, ingress="radio")
            router.step()
            sleep(delay)
    finally:
        writer_stop.set()
        try:
            removed = router.cancel_unsent_motion(disconnect_reason)
            if removed:
                print(f"cancelled {removed} unsent movement request(s)", file=sys.stderr, flush=True)
        finally:
            try:
                transport.close()
            except Exception as exc:
                print(f"radio close failed: {exc}", file=sys.stderr, flush=True)
            try:
                if writer is not None:
                    # Closing the modem first releases an in-flight send_frame
                    # waiting for +OK; joining before close could hold shutdown
                    # for its timeout.
                    writer.join()
            finally:
                router.on_frame = previous_on_frame


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


def _radio_message(frame: bytes) -> dict | None:
    try:
        return codec.decode(frame)
    except (ValueError, KeyError, TypeError, struct.error):
        return None


def _radio_is_stop(frame: bytes) -> bool:
    message = _radio_message(frame)
    return bool(message and message["name"] == "ACTUATOR_REQUEST" and
                message["fields"].get("opcode") == 3)


def _radio_identity(frame: bytes) -> tuple | None:
    message = _radio_message(frame)
    if not message:
        return None
    name, fields = message["name"], message["fields"]
    source = (message["system"], message["component"])
    if name == "REQUEST_RESULT":
        return (name, fields["ground_session"], fields["request_id"])
    if name == "ACTUATOR_STATUS":
        return (name, source, fields["actuator"])
    if name in ("NODE_HEALTH", "HEARTBEAT", "SCALED_IMU"):
        return (name, source)
    return None


def _enqueue_radio(outgoing: queue.Queue, frame: bytes) -> bool:
    """Bound radio egress, retaining latest snapshots and correlated results."""
    payload = bytes(frame)
    message = _radio_message(payload)
    identity = _radio_identity(payload)
    priority = _frame_priority(payload)
    with outgoing.mutex:
        items = outgoing.queue
        if identity is not None:
            for index, queued in enumerate(items):
                if _radio_identity(queued) == identity:
                    items[index] = payload
                    return True
        if len(items) < outgoing.maxsize:
            index = next((i for i, queued in enumerate(items)
                          if priority < _frame_priority(queued)), len(items))
            items.insert(index, payload)
            outgoing.not_empty.notify()
            return True
        if priority == 0:
            routine = next((i for i, queued in enumerate(items)
                            if _frame_priority(queued) != 0), None)
            if routine is not None:
                del items[routine]
                index = next((i for i, queued in enumerate(items)
                              if _frame_priority(queued) != 0), len(items))
                items.insert(index, payload)
                outgoing.not_empty.notify()
                return True
    name = message["name"] if message else "undecodable"
    if name == "REQUEST_RESULT":
        fields = message["fields"]
        print("delivery unknown: radio egress queue full; "
              f"REQUEST_RESULT session={fields['ground_session']} "
              f"request_id={fields['request_id']} was not queued", file=sys.stderr, flush=True)
    else:
        print(f"radio egress queue full; {name} frame was not queued", file=sys.stderr, flush=True)
    return False


def _enqueue_stdout(outgoing: queue.Queue, frame: bytes) -> bool:
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
            return True
        if priority == 0:
            routine = next((index for index, item in enumerate(items)
                            if _frame_priority(item) > 0), None)
            if routine is not None:
                del items[routine]
                index = next((i for i, item in enumerate(items)
                              if _frame_priority(item) > 0), len(items))
                items.insert(index, payload)
                outgoing.not_empty.notify()
                return True
            # Coalesce repeated persistent status snapshots and the latest state
            # for a correlated request while keeping the queue bounded.
            incoming = codec.decode(payload)
            identity = _stdout_identity(incoming)
            for index, item in enumerate(items):
                try:
                    if _stdout_identity(codec.decode(item)) == identity:
                        items[index] = payload
                        return True
                except (ValueError, KeyError):
                    continue
    try:
        try:
            message = codec.decode(payload)
            if message["name"] == "REQUEST_RESULT":
                fields = message["fields"]
                detail = ("delivery unknown: bridge stdout queue full; "
                          f"REQUEST_RESULT session={fields['ground_session']} "
                          f"request_id={fields['request_id']} was not queued")
            else:
                detail = f"bridge stdout queue full; {message['name']} frame was not queued"
        except (ValueError, KeyError, struct.error):
            detail = "bridge stdout queue full; undecodable frame was not queued"
        print(detail, file=sys.stderr, flush=True)
    except OSError:
        pass
    return False


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
    parser.add_argument("command", choices=("serve", "bridge", "radio"),
                        help="serve the bus, bridge raw MAVLink over SSH, or use a direct UART radio")
    parser.add_argument("--config", help="TOML bus and IMU register configuration")
    parser.add_argument("--simulate", action="store_true",
                        help="use injectable simulated peers and visibly labeled simulated IMU values")
    parser.add_argument("--port", default="/dev/serial0", help="UART device for direct radio mode")
    parser.add_argument("--baud", type=int, default=115200, help="UART baud rate for direct radio mode")
    parser.add_argument("--local-address", type=int, default=1, help="local modem address (default: 1)")
    parser.add_argument("--peer-address", type=int, default=2, help="ground modem address (default: 2)")
    parser.add_argument("--network-id", type=int, default=18, help="shared modem network ID")
    parser.add_argument("--band-hz", type=int, default=None,
                        help="optional modem band in Hz; omitted leaves modem frequency unchanged")
    parser.add_argument("--parameter", default="10,7,1,12", help="modem modulation parameter")
    args = parser.parse_args(argv)
    try:
        return _run(args)
    except (OSError, RuntimeError, ValueError, TypeError) as exc:
        print(f"onboard: {exc}", file=sys.stderr, flush=True)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
