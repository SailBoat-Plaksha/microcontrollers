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
`ACTUATOR_REQUEST` frames move to the front of the bounded modem queue and can
evict queued movement when that queue is full. The gateway completes each UART
command across partial writes, then waits for `+OK` or `+ERR` before sending the
next command. Config and send waits have bounded deadlines. A send `+OK` means
the modem accepted the command; it does not confirm radio delivery. A send
timeout is ambiguous and the frame is never replayed, including after an
explicit `Gateway.reconnect()` call.

`Gateway.status()` exposes modem state, queue depths, response reservations,
overflow counters and modem acceptance counts to local integration code. The
wire format stays raw MAVLink: there is no local status text on USB. The
gateway reserves bounded USB response slots for admitted movement requests and
keeps `REQUEST_RESULT` frames in a separate correlation-coalescing queue ahead
of routine telemetry. A full queue refuses new movement admission and reports
overflow through `status()`.

`config.py` provides `CONFIG_TIMEOUT_MS`, `SEND_TIMEOUT_MS`, and
`SEND_GAP_MS`. The last value limits send cadence after a modem response; it
defaults to zero additional delay, with the modem ACK still gating each send.
Configuration failure stops transmission until the integration explicitly
calls `Gateway.reconnect()` (the device entry point requires reset/restart).

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
lines, configuration replies, partial/empty UART and USB writes, modem errors,
timeouts, queue pressure, malformed lines and frames through `FakeUART` and
`FakeUSB`. No serial port or radio is opened.
