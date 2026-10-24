# Ground Pico AT Bridge

MicroPython AT bridge for Raspberry Pi Pico between laptop native USB CDC and the REYAX RYLR998 LoRa transceiver. The Pico acts as a transparent local UART adapter; the host runs `LoRaModemTransport` for MAVLink framing and Base64 encoding.

## Hardware Wiring (3.3V Logic)

- **Pico GP0 (Pin 1, UART0 TX)** -> **RYLR998 RXD**
- **Pico GP1 (Pin 2, UART0 RX)** <- **RYLR998 TXD**
- **Pico GND (Pin 3 or 38)** <-> **RYLR998 GND**
- **Pico 3V3 OUT (Pin 36)** -> **RYLR998 VDD** (3.3V logic)

## Copy Files to Pico

Using `mpremote`:

```sh
mpremote connect auto cp ground-gateway/main.py ground-gateway/bridge.py ground-gateway/config.py :
```

Or copy `main.py`, `bridge.py`, and `config.py` to the Pico root filesystem using Thonny or `rshell`.

> Legacy raw MAVLink mode files are preserved in `main_mavlink.py`, `gateway.py`, `dialect.py`, and `mavlink_stream.py`.

## Quick AT Verification Check

Open the Pico USB CDC serial device (e.g. `/dev/ttyACM0` at 115200 baud):

```text
# Verify modem connection
AT
+OK

# Test query / parameter
AT+ADDRESS?
+ADDRESS=2
+OK

# Send test message to boat (address 1)
AT+SEND=1,5,HELLO
+OK
```

The bridge forwards modem responses (including `+RCV=...`) directly to USB CDC.
