# Boat comms Pico

This firmware is the component 158 relay at I2C address `0x09`. The Pi remains
the only I2C controller. The Pico receives ground frames from the modem and
serves them from the mailbox to the Pi; frames written by the Pi are queued for
the modem. It does not read IMU registers.

The relay uses the committed generated C dialect at
`protocol/gen/C/arsem_standalone/mavlink.h` and constants from
`protocol/mailbox.h`. Each mailbox operation is 64 bytes: one complete
unsigned MAVLink 2 frame followed by zero padding. The receive path rejects
bad CRC, signed frames, unknown messages, oversized frames and nonzero trailing
bytes. It preserves accepted frame bytes.

The comms node sends its own component 158 heartbeat and node health directly
through the radio queue. Broadcast PING requests received over radio get a
local addressed reply over radio; Pi-originated broadcast PING requests get a
local addressed reply through the mailbox. Addressed PING replies do not cause
another reply. The same heartbeat, health and link snapshots are available to
the Pi through empty mailbox reads. Pi writes that already have component 158
as their source are rejected, which prevents snapshots or ping replies from
circulating back through the relay. Radio RSSI/SNR are copied from the modem's `+RCV` fields into
`LINK_STATUS`; the fields become `INT16_MIN` when no sample has arrived in five
seconds or parsing fails. The TX packet counter increments only after a modem
`+OK` send response. This means the modem accepted the command; it does not
prove over-air delivery. `connected` means the modem accepted its configuration
commands; the module provides no association state to prove a remote radio is
reachable.

## Host harness

Run:

```sh
comms-mcu/host/run.sh
```

This compiles the production `relay.c` and `modem.c` against the generated
headers using the host C compiler. It does not open ports or access hardware.

Scenario code can use the same public functions from `include/relay.h` and
`include/modem.h`:

- Inject a Pi mailbox write with `relay_i2c_write(relay, bytes64)`.
- Inject already decoded raw radio bytes with `relay_radio_frame(relay, frame,
  length, now_ms)`.
- Inject fragmented modem UART bytes with `modem_stream_feed`; retrieve each
  complete line with `modem_stream_pop` and pass it to
  `modem_parse_rcv_line(relay, line, ground_radio_address, now_ms)`.
- Collect the next Pi mailbox read using `relay_i2c_read(relay, bytes64)`.
- Collect queued frames for the modem using `relay_radio_next`; turn them into
  an RYLR command using `modem_build_send_command`.

The host harness injects a split `+RCV` line and verifies the resulting mailbox
frame is byte-identical to the ground request. It also checks signed, bad CRC,
oversized and concatenated mailbox frames, ping replies, snapshots and terminal
response priority. A second harness compiles the production mailbox callback
and service code against SDK fakes. It checks 64-byte reads, 1–63-byte retry,
write-only transactions, overflow, staging while new radio traffic arrives,
and the optional byte-65 zero sentinel. A third compiles the actual `main.c`
modem service against fake UART/SDK calls and checks config ACKs, UART
backpressure, send ACK/ERR and ambiguous timeout behavior.

Queues are bounded to eight frames per direction, with a separate eight-frame
terminal-result queue. STOP and terminal results take priority. Routine IMU and
environment samples from a sender are coalesced. Terminal results coalesce by
`(ground_session, request_id)` while queued so the latest state is retained for
that request. When no lower-priority entry can be evicted, a new event is
rejected and the queue-drop counter is reported by `NODE_HEALTH`; the finite
terminal queue cannot retain an unbounded number of distinct results. The relay
reserves a result slot for each queued absolute/step command and refuses further
movement admission when queued results plus outstanding reservations reach
capacity; STOP remains admissible and stays higher priority than telemetry.
Empty or
completed Pi reads rotate the persistent comms-node `NODE_HEALTH`, `HEARTBEAT`
and radio `LINK_STATUS` snapshots. The IRQ serves a prepared 64-byte frame and
does no MAVLink parsing or relay work. That frame stays unchanged across
short/aborted reads and is consumed only when the 64-byte transaction finishes.
The Pico SDK calls the callback for every `RD_REQ`; a request after byte 64 gets
a zero sentinel. No client needs to request byte 65 to complete the fixed
64-byte contract.

The modem sends one command at a time. Each config or send command waits for
`+OK`, `+ERR`, or a bounded timeout before advancing. Config failure leaves the
radio unavailable until an explicit reset/reconnect. A send `+ERR` is a known
local rejection; a send timeout is ambiguous and the popped frame is never
replayed. `COMMS_RADIO_SEND_GAP_MS` optionally limits send cadence after a modem
response. Radio delivery has no application-level acknowledgement; a ground
retry reaches the Pi, where request correlation must return the cached outcome.

## Pico SDK build

The build environment needs the Raspberry Pi Pico SDK and its ARM GCC toolchain:

```sh
cmake -S comms-mcu -B /tmp/comms-mcu-build \
  -DPICO_SDK_PATH=/path/to/pico-sdk -DPICO_BOARD=pico
cmake --build /tmp/comms-mcu-build
```

The default pin settings in `config.h` are I2C0 SDA GP0/SCL GP1 and RYLR998
UART1 TX GP4/RX GP5 at 115200 baud. Confirm the Pico variant, pin use and wiring
before flashing. These are software defaults, not a verified physical pin map.

The modem settings in `config.h` and the ground Pico's `ground-gateway/config.py`
must match: 115200 baud, boat address 1, ground address 2, network ID 18,
915000000 Hz and `PARAMETER=10,7,1,12`. Change the frequency for the local
radio allocation. No radio throughput or airtime measurement has been made.

The Pico SDK exposes no reset-cause value here, so `NODE_HEALTH.reset_reason`
is `RESET_REASON_UNKNOWN`. A boot identifier is mixed from ROSC random-bit
samples and the boot timer; it is a per-boot identifier, not a cryptographic
credential. The node reports no local sensor-valid fields.

## Host-to-Pico integration

Copy `main.py`, `gateway.py`, `config.py`, `dialect.py`, and
`mavlink_stream.py` from `ground-gateway/` to the ground Pico filesystem,
renaming `ground-gateway/main.py` to device `main.py`. The Pico USB CDC
stream is the host serial endpoint. The host writes concatenated raw unsigned
MAVLink 2 frames and reads concatenated raw frames. There are no line endings or
Base64 bytes on USB. Base64 is used only in the RYLR998 `AT+SEND` and `+RCV`
envelope. The gateway accepts only the 14 committed messages and frames no
larger than the 64-byte mailbox.

The ground Pico UART defaults to UART1 GP4/GP5 at 115200 baud. USB CDC does not
use a meaningful UART baud setting. Host serial software should run the normal
MAVLink frame parser and ignore non-frame bytes that a MicroPython build may
emit during USB startup; the running gateway itself prints no diagnostics.
