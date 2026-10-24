# Direct USB-UART radio bench

Current bench topology:

Laptop NiceGUI/CLI -> USB-UART adapter -> RYLR998 -> radio -> RYLR998 -> Pi UART1 -> Pi router -> shared I2C -> motor Pico.

Neither radio hop requires a Pico gateway. The existing comms-Pico and ground-Pico firmware remain optional alternatives. The Pi remains the only I2C controller. Motor message definitions, request/session validation, STOP, bounded moves and RC protections do not change.

## Start the bench

Install the laptop dependencies from `groundstation/requirements.txt` and Pi dependencies from `onboard/requirements.txt`. Run from this checkout, or make it importable as described in [runbook](runbook.md).

On the Pi, connect the receiving modem to its 3.3 V UART or a USB-UART adapter, then run:

```sh
python3 -m onboard radio --port /dev/ttyS0 --baud 115200 --config /etc/arsem/onboard.toml
```

Use the Pi adapter's actual `/dev/ttyUSB*` path instead of `/dev/ttyS0` when applicable. For GPIO UART, disable the Linux serial login console and enable the hardware UART first. Direct mode ignores the optional comms-Pico address 0x09 even if the older config enables it. Only the motor Pico and configured IMU use I2C.

## Pi UART1 setup

The boat brain defaults to `/dev/ttyS0`: hardware UART1 (mini UART) on the Pi Zero family. `/dev/serial0` and `/dev/serial1` are model-dependent aliases. UART1 is not necessarily `/dev/serial1`.

| Pi connection | Boat modem connection |
| --- | --- |
| GPIO14 TX, header pin 8 | RX |
| GPIO15 RX, header pin 10 | TX |
| GND, e.g. header pin 6 | GND |

Use 3.3 V UART signals and the modem model's specified supply.

On Raspberry Pi OS, use `sudo raspi-config` to disable serial login and enable serial hardware. For the Pi Zero family, configure `/boot/firmware/config.txt` (older images: `/boot/config.txt`):

```ini
enable_uart=1
core_freq=250
dtoverlay=uart1,txd1_pin=14,rxd1_pin=15
```

On a plain Pi Zero, UART0 normally owns these header pins; also set `dtparam=uart0=off`. Zero W/Zero 2 W normally expose UART1 there already. Remove conflicting header-UART overlays; `miniuart-bt` assigns UART1 to Bluetooth and `disable-bt` selects UART0 for the header. These settings target Pi Zero models, not Pi 5.

Reboot, confirm `/dev/ttyS0` exists and GPIO14/15 select TXD1/RXD1 (`pinctrl get 14 15` on current Raspberry Pi OS), then launch:

```sh
uv sync --extra onboard
uv run --extra onboard boat-brain --port /dev/ttyS0 --baud 115200 --config /etc/arsem/onboard.toml
```

The program opens the configured device; it does not change boot settings or pin assignments. An explicit `--port` overrides the UART1 default.

References: [Raspberry Pi UART configuration](https://www.raspberrypi.com/documentation/computers/configuration.html#configure-uarts) and [official UART overlay definitions](https://github.com/raspberrypi/linux/blob/rpi-6.12.y/arch/arm/boot/dts/overlays/README). This setup was checked against documentation, not a live Pi.

## Ground station

On the laptop, connect the transmitting modem through a 3.3 V USB-UART adapter and start the GUI:

```sh
uv run --extra groundstation boat-groundstation
```

Open `http://127.0.0.1:8080`, select **LoRa · direct USB UART**, set the adapter port (default `/dev/ttyUSB0`) and 115200 baud, then Connect. The equivalent CLI is:

```sh
python3 -m groundstation.cli --lora /dev/ttyUSB0 --baud 115200
```

The laptop configures local address 2 and peer 1; the Pi configures local 1 and peer 2. Both use network 18 and parameters `10,7,1,12`. Set both modules to the same frequency; a blank GUI frequency / omitted `--band-hz` preserves the module's existing band. Neither end automatically detects radio settings or falls back to another transport.

Start with Ping and returned motor health/status. A local modem `+OK` is not a motor execution report. Movement remains disabled until valid, fresh motor feedback is reported. Encoder references and the existing RC/standby pin conflict still need hardware resolution before motion; [hardware checklist](hardware-checklist.md).

## Shared host modem interface

`boat.radio.LoRaModemTransport` implements the existing `Transport` API:

```python
LoRaModemTransport(port, baudrate=115200, *,
                   local_address=2, peer_address=1, network_id=18,
                   band_hz=None, parameter="10,7,1,12",
                   serial_factory=None, command_timeout_s=3.0,
                   send_gap_s=0.0, queue_limit=32)
```

Methods: `open(on_frame)`, `close()`, `send_frame(raw_mavlink)`; properties `name` and `is_connected`. `stderr_lines` is a bounded queue of modem status/error text for the GUI and Pi logs. `serial_factory` accepts normal pyserial keyword arguments. No serial device opens at import time. `open` completes modem configuration before reporting connected; blocking serial operations run outside the GUI event loop. `close` cancels queued movement and requires explicit reconnect. Queue and response limits are bounded; STOP has priority. A modem timeout, error or ambiguous write never triggers movement retransmission.

Binary MAVLink is encoded as Base64 only inside `AT+SEND` and `+RCV`. Modem `+OK` means local acceptance, not motor execution. Command results still come from the motor/Pi. Local and peer addresses are laptop 2/boat 1 by default; both radios must share network, band and modulation settings. `band_hz=None` leaves the configured modem frequency unchanged, rather than selecting an unverified band.

## Pi integration

New launcher: `python3 -m onboard radio --port /dev/ttyS0 --baud 115200 --config /etc/arsem/onboard.toml`. The Pi uses local address 1, peer 2 and the same host modem implementation. Direct-radio mode does not poll or write I2C address 0x09; the motor and optional IMU still share the bus. Incoming complete radio frames enter the same source/target/authority checks as the existing paths. Return motor status, node health, ping replies and IMU measurements use the same UART/radio link.

Physical wiring and operation remain unverified. Use the identified module's supply requirements, 3.3 V UART logic, crossed TX/RX and common ground; verify the actual USB-UART adapter logic voltage before connection. Pin assignments and encoder references remain subject to the hardware checklist.
