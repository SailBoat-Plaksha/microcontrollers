# Boat midsem scenarios 1–11

Run from the repository root:

```sh
python3 -m scenarios.run
```

The runner writes `scenarios/results/results.json`, `junit.xml` and `trace.jsonl`.
Use `--json`, `--junit` and `--trace` to choose other output paths. Any `FAIL` or
`PENDING` scenario returns exit status 1. `PENDING` is not a pass: it means the
production path needed for that scenario is not available to this runner yet.

The runner has direct checks for the frozen `BoatClient`, `protocol.codec`, the
generated C dialect header, fragmented frame parsing, the 64-byte frame format
and the RYLR998 envelope helpers. It does not implement an alternate boat
router, queue scheduler, mailbox firmware or motor model. `MailboxFixture` only
checks the byte contract. `CapturingTransport` only captures or faults a client
write. Their behavior is not evidence that production implementations behave
the same way.

## Current integration state

Scenarios 1–11 report `PENDING` until they run against the actual implementations
and host harnesses. Passing checks are listed inside each scenario result and
are limited to the named client/codec behavior. In this worktree, generated C
header serialization and Python decode are checked by compiling
`c_codec_probe.c` against `protocol/gen/C/arsem_standalone/mavlink.h`.

Remaining production evidence is:

| Scenario | Required implementation evidence |
| --- | --- |
| 1 | Same correlated request and status through production SSH and LoRa routes, actual mailbox adapters and the compiled motor harness. |
| 2 | Pi bus/IMU adapter acquisition owner, mg and mrad/s values, source IDs, validity mask, and freshness/deadline behavior through both output paths. |
| 3 | Actual responders and router show one matching reply per live node, with no reply-to-reply or forwarding loop. |
| 4 | Pi and compiled motor harness reject wrong target/source/authority, unsupported and bad-CRC requests without energizing output. |
| 5 | Motor harness and router prove limits, fresh feedback, once-only STEP, duplicate/reordered requests, session reset, busy clients, and rollover behavior. |
| 6 | Production queue proves STOP priority; motor harness proves local deadline under missing polls and radio loss, with local timeout distinguished from remote STOP. |
| 7 | SSH/LoRa transports, Pi router, mailbox harness and modem fixture prove dropped ACK/request, ambiguous writes, truncated/padded reads, malformed modem input, delay and recovery without movement replay. |
| 8 | Actual Pi, motor, sensor and comms boot IDs/session changes invalidate queued motion and force status reconciliation. |
| 9 | Actual I2C poller, IMU reader and queues isolate missing peer, stale sensor and overflow while continuing to service healthy nodes. |
| 10 | Motor adapter host harness exercises injected RC ownership and proves autonomous output yields; protected RC regression evidence stays with its owner. |
| 11 | Production bounded queues prove STOP, motor status and health polls remain serviced under saturated telemetry and fair node polling. |

No browser work is included; scenario 12 belongs to the parent. This runner does
not open serial/USB/SSH ports, connect to live I2C, transmit radio packets,
energize motors or modify protected RC code.

## Adapter interface for integration

Wire scenarios to the implementations owned by their lanes. Keep the test
driver thin: it injects bytes and time, then observes production outputs. The
integrated adapter should expose these operations (concrete classes may be
split across Python and compiled-harness wrappers):

```python
class IntegratedBoatAdapter:
    def reset(self, *, seed: int, now: float) -> None: ...
    def connect_ground(self, path: str) -> None: ...  # "ssh" or "lora"
    def send_ground_bytes(self, path: str, data: bytes) -> None: ...
    def inject_fault(self, name: str, *, node: str | None = None,
                     count: int = 1, delay_ms: int = 0) -> None: ...
    def advance(self, milliseconds: int) -> None: ...
    def read_ground_frames(self, path: str) -> list[bytes]: ...
    def snapshot(self) -> dict: ...
    def trace(self) -> list[dict]: ...
```

The adapter must bind to the real Pi router and mailbox code, generated C
protocol headers, compiled motor/comms host harnesses, and actual ground
transport/client implementations. Faults must be applied at transport, mailbox,
modem, clock or node-reset boundaries. Do not satisfy scenario expectations by
implementing idealized movement or forwarding logic inside `scenarios/`.

Recommended assertion sources are independent of production decisions:

- Fixed contract identities, opcodes, targets, exact mailbox length and time
  bounds come from `docs/contract.md`.
- Expected correlation, no-replay, source IDs, IMU units and validity are
  asserted from injected requests and observed output frames/state snapshots.
- Generated C bytes are decoded by `protocol.codec`; client bytes should also
  be decoded by the compiled generated-header harness once that is available.
- Traces must include injected fault, input/output frame identity, monotonic
  timestamp, node boot/session identity and observed result. Never infer motor
  energization from a request or commanded position; use harness output state
  and measured feedback validity.
