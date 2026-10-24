# ARSEM packet links

## Python setup with uv

The repository root is the shared Python project. Optional dependency groups
keep the laptop UI and Raspberry Pi I2C dependency separate:

```sh
# Laptop ground station (NiceGUI + USB UART)
uv sync --extra groundstation
uv run --extra groundstation python -m groundstation.app

# Raspberry Pi service (SMBus/I2C + USB/UART)
uv sync --extra onboard
uv run --extra onboard python -m onboard.boat_brain --port /dev/ttyS0 --baud 115200

# Host tests (no physical ports or I2C devices are opened)
uv run --extra groundstation --extra onboard python -m unittest discover -s tests -v
uv run --extra groundstation --extra onboard python -m unittest discover -s groundstation/tests -v
```

`uv.lock` pins the resolved runtime dependencies. The motor bench helper under
`tests/i2c_sail_motor_test/` has its own small uv project and lock file because
it also installs `pymavlink` for dialect generation.

The current bench path is laptop USB-UART → LoRa → boat LoRa → Pi UART1
(`/dev/ttyS0`) → Pi router → shared I2C → motor Pico. The laptop and Pi each
talk directly to their RYLR998 modem. `boat/radio.py` wraps MAVLink 2 frames in
Base64 for the modem's `AT+SEND` interface and recovers them from `+RCV`.
The Pi and motor Pico exchange raw MAVLink in 64-byte I2C mailboxes.
`protocol/arsem.xml` defines the boat messages. See [direct radio setup](docs/direct-radio.md)
for Pi UART1 setup and wiring, and [software changes](docs/software-changes.md)
for the full change walkthrough and existing motor-code integration.

For the LSM9DS1 sensor board and on-demand IMU readings, see
[sensor board setup](docs/sensor-board.md). The Pi remains the only I2C
controller and can pass requested data between peripheral mailboxes.

Older LoRa gateway variants remain available: `Remote-Station-LoRA/main.c` for a Pico,
and `Remote-Station-LoRA/LoRA-transmit-receive.py` for a Pi connected directly
to the modem. `Base-Station-LoRA/main.py` is a MicroPython gateway with raw
MAVLink on UART0 (GP0/GP1) and the LoRa modem on UART1 (GP4/GP5). Use one
gateway variant on a given link.

Run host packet-link tests with `python -m unittest discover -s tests -v`.
They test frame identity, fragmentation, CRC rejection, LoRa encoding,
Python/C compatibility, and actuator command checks without radio hardware.
See `evidence/README.md` for presentation images and the remaining hardware
checks.

For current boat midsem setup, software checks, and separately labeled Pi and
firmware steps, see [`docs/runbook.md`](docs/runbook.md).
