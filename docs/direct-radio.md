# Direct USB-UART radio bench

Current bench topology:

Laptop NiceGUI/CLI -> USB-UART adapter -> RYLR998 -> radio -> RYLR998 -> Pi UART (or Pi USB-UART) -> Pi router -> shared I2C -> motor Pico.

Neither radio hop requires a Pico gateway. The existing comms-Pico and ground-Pico firmware remain optional alternatives. The Pi remains the only I2C controller. Motor message definitions, request/session validation, STOP, bounded moves and RC protections do not change.

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

New launcher: `python3 -m onboard radio --port /dev/serial0 --baud 115200 --config /etc/arsem/onboard.toml`. The Pi uses local address 1, peer 2 and the same host modem implementation. Direct-radio mode does not poll or write I2C address 0x09; the motor and optional IMU still share the bus. Incoming complete radio frames enter the same source/target/authority checks as the existing paths. Return motor status, node health, ping replies and IMU measurements use the same UART/radio link.

Physical wiring and operation remain unverified. Use the identified module's supply requirements, 3.3 V UART logic, crossed TX/RX and common ground; verify the actual USB-UART adapter logic voltage before connection. Pin assignments and encoder references remain subject to the hardware checklist.
