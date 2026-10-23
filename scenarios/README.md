# Boat midsem scenarios 1–11

Run from the repository root:

```sh
python3 -m scenarios.run
```

The runner writes `scenarios/results/results.json`, `junit.xml` and
`trace.jsonl`. Use `--json`, `--junit` and `--trace` for other paths. Any
`FAIL` or `PENDING` scenario returns exit status 1. Each scenario is `PASS` only
when its assigned requirements pass. Assertions are separate report checks so
an independent check still runs after a failure.

## Production paths exercised

- **SSH:** the production `SSHTransport` argv and pipe threads run against an
  OS-pipe process facade. The facade calls the real `onboard._run_bridge` in a
  local worker thread and uses the compiled motor adapter as its I2C endpoint.
  Stdout is fragmented into five-byte writes; stderr remains a separate log
  stream. It does not open SSH or a network connection.
- **LoRa:** the production ground `Gateway` processes USB MAVLink, emits its
  actual `AT+SEND` envelope, and receives real C `modem.c`/`relay.c` processing
  through ctypes. The Pi `BoatRouter` and compiled motor adapter process the
  mailbox traffic. The modem `+RCV` lines are injected only at the virtual radio
  boundary. Actual return frames pass back through the C relay/modem and ground
  Gateway. No parser, router or motion logic is reimplemented in the fixture.
- **Motor:** `motor-mcu/adapter.c` is compiled with `scenarios/motor_ctypes.c`.
  Counts, feedback-valid flags, RC ownership and monotonic time are injected
  independently of requested motion; output assertions read the compiled
  adapter callbacks.

## Coverage

| Scenario | Host evidence |
| --- | --- |
| 1 | Correlated move and measured completion through the full LoRa path; actual `SSHTransport` stdio through `_run_bridge`; stdout fragmentation and stderr separation. |
| 2 | Pi IMU register acquisition, mg/mrad/s values, validity and read-error propagation through LoRa and SSH; stale-sample expiry assertion. |
| 3 | One matched ping reply from Pi, motor and comms for a single broadcast; addressed replies do not create relay reply storms. |
| 4 | Signed and damaged frames rejected by the relay; unsupported message, wrong target, source and authority rejected before motor output. |
| 5 | STEP resolution, exact duplicate and reordered suppression, invalid reference, stale feedback, busy second session, concurrent router client and request/command-ID wrap protection. |
| 6 | STOP queue priority and independent compiled-motor local deadline. |
| 7 | Ambiguous client and bus writes become UNKNOWN without replay; truncated mailbox read; fragmented/malformed modem input and subsequent recovery. |
| 8 | Motor boot change makes pending result UNKNOWN and retires its ground session; queued old-boot motion is removed; reconnect sends no old movement. Router and comms relay recreation use new boot identities. |
| 9 | Missing comms peer does not block healthy motor polls; IMU read failure clears sensor validity. |
| 10 | Compiled adapter yields autonomous output after injected RC ownership. |
| 11 | Saturated Pi and comms queues retain STOP priority; router writes stay within three consecutive operations and poll both peers. |

## Current result and proof limits

Current full run: 10 scenarios pass; scenario 2 has one failing stale-IMU
assertion. `onboard/router.py::_poll_imu` clears validity when a read fails,
but there is no age check that clears `imu_valid_fields` when the last
successful sample simply stops being polled. Reproduction: sample the configured
IMU at injected time `10.0`, advance the injected clock to `15.001`, then inspect
`router.imu_valid_fields`; it remains `3`. This is a product-source finding for
the onboard owner; this scenario lane records it without modifying onboard code.

The LoRa fixture drives each real Gateway configuration and send transaction
with an explicit millisecond clock. It acknowledges configuration only after
each UART line is fully written, acknowledges sends only after the complete
`AT+SEND` line is written, and completes relay terminal results at the virtual
modem acceptance boundary. Scenario 1 drains the path incrementally so both the
terminal result and actuator status traverse the actual relay and Gateway.

Host scenarios do not prove physical I2C electrical behavior, actual radio
delivery, the physical IMU, motor/encoder motion, or RC receiver behavior.
Scenarios 6 and 10 cover their stated host behavior only. No USB, serial, SSH,
live I2C, over-air radio or motor ports are opened.

## Trace and deterministic report rules

- Frames, mailbox operations, source identity, request/session/boot IDs and
  observed motor output come from exercised code or explicit fixture inputs.
- A fixture may add framing only at a physical boundary, such as the virtual
  modem's `+RCV` envelope. It does not fabricate a successful router or motor
  response.
- Fixed boot/session generators and stable frame/trace ordering keep tracked
  JSON, JUnit and JSONL reports unchanged across reruns.
- Unsupported hardware and over-air claims remain outside this host suite.
