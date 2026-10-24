"""Fast standalone local RYLR998 modem verification tool.

Probes the local RYLR998 LoRa modem connected via USB-UART or Pico USB-UART0
bridge using LoRaModemTransport.open and check_health WITHOUT initializing
BoatClient, sending MAVLink traffic, or transmitting any AT+SEND frames.

NOTE: This checks local modem AT communication and configuration only;
it does NOT establish or verify boat RF reachability or remote motor reachability.
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from typing import Any

from boat.radio import LoRaModemTransport

DISCLAIMER = (
    "Checking the ground modem confirms local USB-UART AT communication and configuration only; "
    "it does NOT prove boat RF reachability, remote modem connectivity, or over-the-air link quality."
)


def verify_modem(
    port: str = "/dev/ttyACM0",
    baudrate: int = 115200,
    *,
    command_timeout_s: float = 1.0,
    local_address: int = 2,
    peer_address: int = 1,
    network_id: int = 18,
    band_hz: int | None = None,
    parameter: str = "10,7,1,12",
    serial_factory: Any = None,
) -> dict[str, Any]:
    """Open the local modem, configure parameters, and query AT health.

    Runs WITHOUT BoatClient, MAVLink traffic, or any AT+SEND command.
    Always closes the transport cleanly upon completion or error.
    """
    transport = LoRaModemTransport(
        port=port,
        baudrate=baudrate,
        local_address=local_address,
        peer_address=peer_address,
        network_id=network_id,
        band_hz=band_hz,
        parameter=parameter,
        serial_factory=serial_factory,
        command_timeout_s=command_timeout_s,
    )
    try:
        transport.open(on_frame=lambda _f: None)
        report = transport.check_health()
        return report
    finally:
        transport.close()


def send_raw_at_command(
    command: str,
    port: str = "/dev/ttyACM0",
    baudrate: int = 115200,
    *,
    timeout: float = 1.0,
    serial_factory: Any = None,
) -> str:
    """Send an arbitrary AT command to the serial port and return the response."""
    clean = (command or "").strip()
    if not clean:
        raise ValueError("Command cannot be empty")
    if not clean.upper().startswith("AT"):
        raise ValueError("Command must start with 'AT'")
    if serial_factory is None:
        import serial
        serial_factory = serial.Serial
    ser = serial_factory(port, baudrate, timeout=timeout)
    try:
        crlf = chr(13) + chr(10)
        ser.write(clean.encode("ascii") + crlf.encode("ascii"))
        ser.flush()
        start = time.monotonic()
        lines = []
        while time.monotonic() - start < timeout:
            raw_line = ser.readline()
            if raw_line:
                decoded = raw_line.decode("ascii", errors="replace").strip()
                if decoded:
                    lines.append(decoded)
                    if decoded == "+OK" or decoded.startswith("+ERR=") or decoded.startswith("+"):
                        break
        return " | ".join(lines) if lines else "(no response)"
    finally:
        ser.close()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="python -m groundstation.modem_check",
        description="Fast local RYLR998 LoRa modem verification (probe AT identity and settings).",
    )
    parser.add_argument("--port", default="/dev/ttyACM0", help="serial port (default: /dev/ttyACM0)")
    parser.add_argument("--baud", type=int, default=115200, help="serial baudrate (default: 115200)")
    parser.add_argument(
        "--timeout",
        type=float,
        default=1.0,
        help="modem command timeout in seconds (default: 1.0)",
    )
    parser.add_argument("--local-address", type=int, default=2, help="local modem address (default: 2)")
    parser.add_argument("--peer-address", type=int, default=1, help="expected boat peer address (default: 1)")
    parser.add_argument("--network-id", type=int, default=18, help="network ID (default: 18)")
    parser.add_argument(
        "--band-hz",
        type=int,
        default=None,
        help="optional band frequency in Hz; omitted leaves modem's stored band unchanged",
    )
    parser.add_argument(
        "--parameter",
        default="10,7,1,12",
        metavar="SF,BW,CR,PREAMBLE",
        help="RF parameters (default: 10,7,1,12)",
    )
    parser.add_argument("--at", metavar="COMMAND", help="send arbitrary AT command and print response")
    parser.add_argument("--json", action="store_true", help="output structured JSON")
    return parser


def format_report_text(
    args: argparse.Namespace,
    report: dict[str, Any] | None,
    error: BaseException | None = None,
) -> str:
    lines = [
        "======================================================================",
        "RYLR998 Local Ground Modem Verification",
        f"Port: {args.port} @ {args.baud} baud (command timeout: {args.timeout}s)",
        "======================================================================",
    ]

    band_disclosure = (
        f"Band frequency: {args.band_hz} Hz"
        if args.band_hz is not None
        else "Band frequency: (blank/omitted - existing modem stored band left unchanged)"
    )
    lines.append("Configuration applied:")
    lines.append(f"  - Local address: {args.local_address}")
    lines.append(f"  - Network ID:    {args.network_id}")
    lines.append(f"  - Parameters:    {args.parameter}")
    lines.append(f"  - {band_disclosure}")
    lines.append("")

    if error is not None:
        lines.append(f"RESULT: FAILED ({type(error).__name__}: {error})")
        lines.append("Local modem was not reachable or failed configuration / query.")
    elif report is None:
        lines.append("RESULT: FAILED (No report returned)")
    else:
        responses = report.get("responses", {})
        checks = report.get("checks", {})
        healthy = bool(report.get("healthy", False))

        def _val(name: str) -> str:
            raw = responses.get(name, "")
            return raw.partition("=")[2] if "=" in raw else raw

        lines.append("Local modem reachable: YES (+OK)")
        lines.append("Actual AT identity and settings reported by modem:")
        lines.append(f"  - Address:    {_val('address') or 'unknown'}")
        lines.append(f"  - Network ID: {_val('network_id') or 'unknown'}")
        lines.append(f"  - Band:       {(_val('band_hz') + ' Hz') if _val('band_hz') else 'unknown'}")
        lines.append(f"  - Parameters: {_val('parameter') or 'unknown'}")
        lines.append(f"  - Firmware:   {_val('version') or 'unknown'}")
        lines.append(f"  - UID:        {_val('uid') or 'unknown'}")
        lines.append("")
        lines.append("Check results:")
        for check_name, check_ok in checks.items():
            lines.append(f"  - {check_name}: {'OK' if check_ok else 'FAIL'}")
        lines.append("")
        if healthy:
            lines.append("STATUS: HEALTHY (Local modem verified)")
        else:
            lines.append("STATUS: FAILED (One or more configuration checks did not match)")

    lines.append("")
    lines.append(f"DISCLAIMER: {DISCLAIMER}")
    lines.append("======================================================================")
    return "\n".join(lines)


def main(argv: list[str] | None = None, *, serial_factory: Any = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.at:
        try:
            resp = send_raw_at_command(
                args.at,
                port=args.port,
                baudrate=args.baud,
                timeout=args.timeout,
                serial_factory=serial_factory,
            )
            print(resp)
            return 0
        except Exception as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 1

    report = None
    error = None
    try:
        report = verify_modem(
            port=args.port,
            baudrate=args.baud,
            command_timeout_s=args.timeout,
            local_address=args.local_address,
            peer_address=args.peer_address,
            network_id=args.network_id,
            band_hz=args.band_hz,
            parameter=args.parameter,
            serial_factory=serial_factory,
        )
    except Exception as exc:
        error = exc

    healthy = bool(report and report.get("healthy", False) and error is None)

    if args.json:
        band_disclosure = (
            f"Configured to {args.band_hz} Hz."
            if args.band_hz is not None
            else "Blank band leaves modem frequency unchanged."
        )
        data: dict[str, Any] = {
            "healthy": healthy,
            "reachable": error is None and report is not None,
            "port": args.port,
            "baudrate": args.baud,
            "command_timeout_s": args.timeout,
            "applied_settings": {
                "local_address": args.local_address,
                "peer_address": args.peer_address,
                "network_id": args.network_id,
                "band_hz": args.band_hz,
                "parameter": args.parameter,
            },
            "band_disclosure": band_disclosure,
            "disclaimer": DISCLAIMER,
        }
        if report is not None:
            data["responses"] = report.get("responses", {})
            data["checks"] = report.get("checks", {})
        if error is not None:
            data["error"] = f"{type(error).__name__}: {error}"
        print(json.dumps(data, indent=2))
    else:
        print(format_report_text(args, report, error))

    return 0 if healthy else 1


if __name__ == "__main__":
    sys.exit(main())
