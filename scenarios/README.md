# Boat midsem scenarios 1–11

Run from the repository root:

```sh
python3 -m scenarios.run
```

The runner writes `scenarios/results/results.json`, `junit.xml` and `trace.jsonl`.
Use `--json`, `--junit` and `--trace` to choose other output paths. Any `FAIL` or
`PENDING` scenario returns exit status 1. A scenario is `PASS` only when every
requirement assigned to that scenario is exercised. Passing checks in an
incomplete scenario are retained as granular evidence.

## Integrated driver

The driver compiles `motor-mcu/adapter.c` with `scenarios/motor_ctypes.c` into a
temporary shared library. A controlled monotonic clock, independent encoder
counts, feedback-valid flags, injected RC ownership and output callbacks cross
that thin wrapper. It connects the production `BoatClient` to the production
`BoatRouter` and an `I2CBus` whose motor endpoint is the compiled adapter. It
does not implement movement, routing, queueing, mailbox parsing or response
correlation.

Other checks cover `protocol.codec`, generated C dialect serialization,
fragmented frame parsing, the 64-byte mailbox contract and the RYLR998 envelope
format. Those byte/envelope fixtures do not establish that the real firmware
radio relay works. `CapturingTransport` checks isolated client behavior only.

Current integrated evidence includes the client/router/compiled-motor absolute
route and correlated completion; independently injected measured feedback;
Pi-owned IMU register reads and unit conversion; one actual motor ping reply;
invalid request guards; STEP resolution and duplicate suppression; ambiguous
write UNKNOWN/no replay; STOP queue priority; the compiled adapter's local
deadline; and adapter output yield under injected RC ownership.

## Remaining checks

| Scenario | Still required before full scenario PASS |
| --- | --- |
| 1 | Production SSH stdio process and full LoRa ground gateway/comms relay paths. |
| 2 | Stale-sample/deadline behavior and IMU delivery through both production routes. |
| 3 | Real comms responder and one reply per live node with no reply loops. |
| 4 | Signed and unsupported frames through the complete Pi-to-motor route. |
| 5 | Reorder/rollover, busy clients, session reset, invalid reference and stale feedback cases. |
| 7 | Bus drops, short reads, modem faults and recovery across both routes. |
| 8 | Actual Pi/motor/comms boot changes and pending-request invalidation. |
| 9 | Missing peer, stale IMU and queue-overflow isolation while healthy peers continue. |
| 11 | Saturated production queues, STOP/status/health priority and fair node polling. |

Scenarios 6 and 10 are complete for their host requirements. Scenario 10 injects
RC ownership through the compiled adapter callback; physical receiver and
protected-loop behavior remain hardware/regression checks outside this host
scenario.

No browser work is included; scenario 12 belongs to the parent. The runner does
not open serial/USB/SSH ports, connect to live I2C, transmit radio packets or
energize physical motors.

## Assertion and trace rules

- Contract identities, opcodes, target IDs, 64-byte transfers and bounds come
  from `docs/contract.md`.
- Expected correlation, IMU units and validity are asserted from injected
  requests and observed production frames/state.
- Generated-C frames are decoded with `protocol.codec`.
- Traces record raw frames, monotonic time, request/session/node boot identity,
  bus operations, injected feedback and observed motor output/status.
- Motor energization is asserted from compiled adapter output callbacks. Encoder
  counts are injected independently from requested targets.
