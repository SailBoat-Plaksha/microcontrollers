"""Interactive headless controller using the same BoatClient as the GUI."""
from __future__ import annotations

import argparse
import threading
import time

from boat.client import BoatClient, MotorReport, NodeHealthReport, RequestOutcome
from boat.radio import LoRaModemTransport
from .transports import SerialTransport, SimulationTransport, SSHTransport


def parse_step(parts, default_step):
    """Parse ``step [degrees [actuator]]`` without setting pending state."""
    if len(parts) not in (1, 2, 3) or parts[0].lower() != "step":
        raise ValueError("use step [degrees [actuator 1|2]]")
    delta = default_step if len(parts) == 1 else float(parts[1])
    actuator = int(parts[2]) if len(parts) == 3 else 1
    if not -327.67 <= delta <= 327.67 or delta == 0:
        raise ValueError("step must be non-zero and fit signed centidegrees")
    if actuator not in (1, 2):
        raise ValueError("actuator must be sail (1) or rudder (2)")
    return delta, actuator


def build_parser():
    parser = argparse.ArgumentParser(description="Boat ground station CLI")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--simulate", action="store_true", help="use conspicuously labeled simulation")
    group.add_argument("--serial", metavar="PORT", help="ground Pico USB serial port (LoRa gateway)")
    group.add_argument("--lora", metavar="PORT", help="direct RYLR998 USB-UART modem port")
    group.add_argument("--ssh", metavar="HOST_ALIAS", help="SSH config alias for the Pi")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--step-deg", type=float, default=5.0)
    parser.add_argument("--local-address", type=int, default=2)
    parser.add_argument("--peer-address", type=int, default=1)
    parser.add_argument("--network-id", type=int, default=18)
    parser.add_argument("--band-hz", type=int, default=None,
                        help="optional modem frequency in Hz; omitted leaves its current band")
    parser.add_argument("--parameter", default="10,7,1,12", metavar="SF,BW,CR,PREAMBLE")
    return parser


def make_transport(args):
    return (SimulationTransport() if args.simulate else
            LoRaModemTransport(args.lora, args.baud, local_address=args.local_address,
                peer_address=args.peer_address, network_id=args.network_id,
                band_hz=args.band_hz, parameter=args.parameter) if args.lora else
            SerialTransport(args.serial, args.baud) if args.serial else SSHTransport(args.ssh))


def main():
    parser = build_parser()
    args = parser.parse_args()
    if not 0.01 <= args.step_deg <= 327.67:
        parser.error("--step-deg must fit the signed centidegree request field (0.01..327.67)")
    transport = make_transport(args)
    client = BoatClient(transport)
    events = []
    event_lock = threading.Lock()
    safety = {"health": None, "motors": {}, "last_motor": {}, "last_health": 0.0,
              "busy": False, "uncertain": True}

    def on_event(event):
        with event_lock:
            events.append((time.monotonic(), event))
            del events[:-500]
            if isinstance(event, NodeHealthReport) and event.source == (1, 154):
                previous = safety["health"]
                if previous is not None and previous.boot_id != event.boot_id:
                    safety["motors"].clear()
                    safety["last_motor"].clear()
                    safety["busy"], safety["uncertain"] = False, True
                safety["health"], safety["last_health"] = event, time.monotonic()
            elif isinstance(event, MotorReport):
                safety["motors"][event.actuator] = event
                safety["last_motor"][event.actuator] = time.monotonic()
                safety["uncertain"] = False
                safety["busy"] = any(item.result == 1 or bool(item.enabled)
                                      for item in safety["motors"].values())
            elif isinstance(event, RequestOutcome):
                safety["uncertain"] = event.result == 0 or event.state in (0, 1)
                if event.state in (1, 2, 3):
                    safety["busy"] = True
                elif event.state in (4, 5, 6):
                    safety["busy"] = any(item.result == 1 or bool(item.enabled)
                                          for item in safety["motors"].values())

    client.subscribe(on_event)

    def require_movement_ready(actuator):
        with event_lock:
            health = safety["health"]
            motor = safety["motors"].get(actuator)
            age = time.monotonic() - safety["last_motor"].get(actuator, 0)
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
        elif args.lora:
            print("Direct RYLR998 mode: +OK confirms modem acceptance; motor outcomes come from Pi/MAVLink.")
        print("Commands: absolute <degrees> [actuator 1|2], step [degrees [actuator 1|2]], stop [actuator 1|2], ping, status, modem, quit")
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
                elif cmd == "modem":
                    check_health = getattr(transport, "check_health", None)
                    if not callable(check_health):
                        raise RuntimeError("modem AT health is available only in direct --lora mode")
                    report = check_health()
                    print(f"LoRa modem health: {'OK' if report['healthy'] else 'CHECK FAILED'}")
                    for name, response in report["responses"].items():
                        print(f"  {name}: {response}")
                elif cmd == "absolute" and len(parts) in (2, 3):
                    deg = float(parts[1])
                    actuator = int(parts[2]) if len(parts) == 3 else 1
                    if not -327.67 <= deg <= 327.67:
                        raise ValueError("angle is outside signed centidegree field range")
                    if actuator not in (1, 2):
                        raise ValueError("actuator must be sail (1) or rudder (2)")
                    require_movement_ready(actuator)
                    with event_lock:
                        safety["busy"], safety["uncertain"] = True, True
                    ticket = client.set_absolute_angle(actuator, round(deg * 100))
                    print(f"Request sent: id={ticket.request_id} session={ticket.ground_session}")
                elif cmd == "step" and len(parts) in (1, 2, 3):
                    delta, actuator = parse_step(parts, args.step_deg)
                    require_movement_ready(actuator)
                    with event_lock:
                        safety["busy"], safety["uncertain"] = True, True
                    ticket = client.step(actuator, round(delta * 100))
                    print(f"Request sent: id={ticket.request_id} session={ticket.ground_session}")
                elif cmd == "stop" and len(parts) in (1, 2):
                    actuator = int(parts[1]) if len(parts) == 2 else 1
                    if actuator not in (1, 2):
                        raise ValueError("actuator must be sail (1) or rudder (2)")
                    ticket = client.stop(actuator)
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
