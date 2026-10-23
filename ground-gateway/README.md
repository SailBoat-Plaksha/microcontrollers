# Ground Pico gateway

This MicroPython program bridges USB CDC and the RYLR998 UART. Copy these
runtime files to the Pico filesystem:

- `main.py`
- `gateway.py`
- `config.py`
- `dialect.py`
- `mavlink_stream.py`

Run `main.py` from the MicroPython boot path so the bridge remains active.
The gateway runtime imports only MicroPython modules plus these local files; it
does not import CPython XML, pathlib or dataclasses modules. Regenerate the
checked-in CRC table on the host with `python3 ground-gateway/generate_dialect.py`.
That script reads CRC_EXTRA values from the committed generated C headers.

## Host serial contract

Open the Pico USB CDC serial device. Write complete unsigned MAVLink 2 frames
as a byte stream; frames may be fragmented across writes or concatenated.
Read complete raw frames from the same byte stream. Do not add text lines or
Base64 to USB traffic. The parser validates CRC_EXTRA against the 14-message
table and rejects signed, malformed, unknown or over-64-byte frames.
Outbound frames must use ground system/component IDs 255/190.

The gateway encodes Base64 only inside `AT+SEND=<boat-address>,<length>,...`
and decodes the RYLR998 `+RCV` envelope before returning raw frames to USB.
The modem's sender address must match the configured boat address. STOP
`ACTUATOR_REQUEST` frames move to the front of the bounded modem queue. The
gateway does not retry a frame after placing its one modem command on UART.

USB CDC has no baud-rate setting. The RYLR998 UART uses UART1 GP4/GP5 at 115200
baud by default. Both Pico configurations default to boat address 1, ground
address 2, network ID 18, 915000000 Hz, and modem parameter string
`10,7,1,12`. Configure a legal frequency for the operating location and match
all modem settings. Airtime and sustained throughput remain unmeasured.

## Fake-hardware tests

Run from the repository root:

```sh
python3 -m unittest discover -s ground-gateway/tests -v
```

The tests inject fragmented USB data, fragmented and coalesced UART modem
lines, configuration replies, malformed lines and frames through `FakeUART`
and `FakeUSB`. No serial port or radio is opened.
