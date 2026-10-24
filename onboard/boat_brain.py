"""Dedicated Raspberry Pi direct-radio boat brain entry point.

This is a named launcher for the production onboard router, using Pi Zero
UART1 (/dev/ttyS0) by default. The router owns
the shared I2C bus, validates incoming MAVLink, routes commands to configured
peers, gathers node/IMU telemetry, and returns those frames through the radio.
"""

from __future__ import annotations

import sys

from .__main__ import main as _onboard_main


def main(argv: list[str] | None = None) -> int:
    """Run the onboard router in direct RYLR998 mode."""
    return _onboard_main(["radio", *(sys.argv[1:] if argv is None else argv)])


if __name__ == "__main__":
    raise SystemExit(main())
