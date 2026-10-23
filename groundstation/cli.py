"""Interactive headless controller using the same BoatClient as the GUI."""
from __future__ import annotations

import argparse
import threading
import time

from boat.client import BoatClient, MotorReport, NodeHealthReport, RequestOutcome
from .transports import SerialTransport, SimulationTransport, SSHTransport


def main():
    parser = argparse.ArgumentParser(description="Boat raw-MAVLink ground station CLI")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--simulate", action="store_true", help="use conspicuously labeled simulation")
    group.add_argument("--serial", metavar="PORT", help="ground Pico USB serial port (LoRa gateway)")
    group.add_argument("--ssh", metavar="HOST_ALIAS", help="SSH config alias for the Pi")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--step-deg", type=float, default=5.0)
    args = parser.parse_args()
    if not 0.01 <= args.step_deg <= 327.67:
        parser.error("--step-deg must fit the signed centidegree request field (0.01..327.67)")
    transport = (SimulationTransport() if args.simulate else
                 SerialTransport(args.serial, args.baud) if args.serial else SSHTransport(args.ssh))
    client = BoatClient(transport)
    events = []
    event_lock = threading.Lock()
    safety = {"health": None, "motor": None, "last_motor": 0.0, "last_health": 0.0,
              "busy": False, "uncertain": True}

    def on_event(event):
        with event_lock:
            events.append((time.monotonic(), event))
            del events[:-500]
            if isinstance(event, NodeHealthReport) and event.source == (1, 154):
                safety["health"], safety["last_health"] = event, time.monotonic()
            elif isinstance(event, MotorReport):
                safety["motor"], safety["last_motor"] = event, time.monotonic()
                safety["uncertain"] = False
                safety["busy"] = event.result == 1 or bool(event.enabled)
            elif isinstance(event, RequestOutcome):
                safety["uncertain"] = event.result == 0 or event.state in (0, 1)
                if event.state in (1, 2, 3):
                    safety["busy"] = event.state in (2, 3)
                elif event.state in (4, 5, 6):
                    safety["busy"] = False

    client.subscribe(on_event)

    def require_movement_ready():
        with event_lock:
            health, motor = safety["health"], safety["motor"]
            age = time.monotonic() - safety["last_motor"]
            if (health is None or not health.feedback_valid or motor is None or age > 5 or
                    time.monotonic() - safety["last_health"] > 5):
                raise RuntimeError("movement disabled: fresh motor feedback and feedback-valid health are required")
            if safety["busy"] or safety["uncertain"]:
                raise RuntimeError("movement disabled: motor is busy or outcome is unknown")
    try:
        client.connect()
        client.ping_all()
        print(f"Connected: {transport.name}")
        if args.simulate:
            print("SIMULATION MODE: all sensor and motor values are generated")
        print("Commands: absolute <degrees> [actuator 0|1], step [+|- degrees] [actuator], stop [actuator], ping, status, quit")
        while True:
            try:
                line = input("boat> ").strip()
            except EOFError:
                break
            parts = line.split()
            if not parts:
                continue
            cmd = parts[0].lower()
            if cmd in {"quit", "exit"}:
                break
            try:
                if cmd == "ping":
                    client.ping_all()
                elif cmd == "absolute" and len(parts) in (2, 3):
                    require_movement_ready()
                    actuator = int(parts[2]) if len(parts) == 3 else 0
                    deg = float(parts[1])
                    if not -327.67 <= deg <= 327.67:
                        raise ValueError("angle is outside signed centidegree field range")
                    with event_lock:
                        safety["busy"], safety["uncertain"] = True, True
                    ticket = client.set_absolute_angle(actuator, round(deg * 100))
                    print(f"Request sent: id={ticket.request_id} session={ticket.ground_session}")
                elif cmd == "step" and len(parts) in (1, 3):
                    require_movement_ready()
                    delta = args.step_deg if len(parts) == 1 else float(parts[1])
                    actuator = 0 if len(parts) == 1 else int(parts[2])
                    if not -327.67 <= delta <= 327.67:
                        raise ValueError("step is outside signed centidegree field range")
                    with event_lock:
                        safety["busy"], safety["uncertain"] = True, True
                    ticket = client.step(actuator, round(delta * 100))
                    print(f"Request sent: id={ticket.request_id} session={ticket.ground_session}")
                elif cmd == "stop" and len(parts) in (1, 2):
                    ticket = client.stop(int(parts[1]) if len(parts) == 2 else 0)
                    print(f"STOP sent: id={ticket.request_id} session={ticket.ground_session}")
                elif cmd == "status":
                    with event_lock:
                        for at, event in events[-40:]:
                            print(f"{at:.3f} {event}")
                else:
                    print("Invalid command; type one of the listed forms.")
            except (ValueError, RuntimeError, OSError) as exc:
                if isinstance(exc, OSError):
                    with event_lock:
                        safety["uncertain"] = True
                print(f"Command failed: {exc}")
    finally:
        try:
            client.disconnect()
        except Exception as exc:
            print(f"Disconnect reported: {type(exc).__name__}: {exc}")


if __name__ == "__main__":
    main()
