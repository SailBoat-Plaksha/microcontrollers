# Boat midsem scenarios 1–11

The direct Laptop/Pi radio topology is exercised separately with:

```sh
python3 -m scenarios.direct_radio
```

That runner opens two production `LoRaModemTransport` instances on a paired
fake serial modem boundary, runs the actual Pi `_run_radio_service` and router,
and uses the same compiled motor adapter harness as the original scenarios.
It does not start the optional ground gateway or comms relay. Its JSON summary
is deterministic and excludes host-clock RTTs and boot/session randomness.
The fixture covers both endpoint address/AT configurations, absolute and STEP
motion with terminal correlated outcomes and independently injected encoder
counts, Pi/motor ping, invalid modem frames, Pi source/target rejection,
motor-health freshness, queue bounds/STOP priority, short writes, ACK timeout,
`+ERR` rejection, disconnect, reconnect, and no replay of uncertain sends.
I2C operations must stay on motor address `0x08`; address `0x09` is not polled.

As with the original host scenarios, fake serial modems and the compiled motor
adapter do not prove electrical UART/I2C behavior, actual RYLR998 RF delivery,
physical encoders, motor movement, or live hardware reliability.

During fixture bring-up, delivering a `+RCV` line before `+OK` on the same
reader stream reproduced an ACK timeout when the receive callback was blocked
on the client's send lock. The direct runner uses ACK-before-receive ordering
for ordinary sends and does not claim to cover that concurrent line ordering;
the production reader/callback issue remains for the parent radio change to
resolve.

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

Current full run: **11 PASS, 0 FAIL, 0 PENDING**. All 33 individual checks pass. IMU samples lose validity after five seconds; sensor unavailability and recovery are verified through the actual relay and ground gateway. JSON, JUnit and JSONL outputs are byte-identical across repeated runs.

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
