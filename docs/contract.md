# Boat integrated-demo contract freeze

Status: FROZEN for the midsem parallel delivery. Base: `i2c+mavlink` at `13b58b2` plus this commit. Any change to the wire contract returns to the protocol owner; lanes never edit `protocol/` independently.

Source of truth for messages: `protocol/arsem.xml` (schema `1.1.0`: the eight existing custom messages are byte-identical to schema `1.0.0`; `ACTUATOR_REQUEST`, `NODE_HEALTH`, `REQUEST_RESULT`, new enums and `LINK_TYPE_I2C` are additive).

## 1. Identities

| Node | system | component | I2C address | Role |
| --- | --- | --- | --- | --- |
| Pi (boat brain, sole bus controller) | 1 | 1 | controller | routes, validates, owns IMU reads |
| Motor Pico (Pico 2) | 1 | 154 | 0x08 | servo/encoder actuator node |
| Optional comms Pico (Pico 1) | 1 | 158 | 0x09 | radio relay + telemetry forwarder |
| IMU | — | — | device-specific (driver-configurable, provisional) | passive peripheral; the Pi reads its registers |
| Ground station (GUI/CLI) | 255 | 190 | — | sole command origin for movement requests |

`NODE_INFO.source` IDs identify the reporting node, never the relay. The Pi maps an upstream ground request (system 255) to local motor target (system 1, component 154) under its own source (1,1) and preserves correlation in both directions. Component 158 is provisional and noncolliding with 1 and 154.

## 2. Message set

Supported: custom 200000–200010 plus standard `HEARTBEAT` (0), `PING` (4), `SCALED_IMU` (26). `ARSEM_MAILBOX_FRAME_BYTES` is 64, so every supported frame must encode to ≤ 64 bytes (including `LINK_STATUS`, which may encode to 36 bytes).

- **Movement**: ground issues ONLY `ACTUATOR_REQUEST` (200008). `opcode` selects ABSOLUTE (`delta_cdeg` = target), STEP (signed delta, resolved exactly once by the motor node against valid fresh feedback, then a bounded absolute target), or STOP (delta must be zero; STOP is distinct from a zero-degree target and distinct from a local timeout). Default GUI/CLI STEP is an angular jog (default ±5° = ±500 cdeg, configurable). `ACTUATOR_SETPOINT` (200001) keeps its existing bench semantics; production lanes do not send it.
- **Motor response**: `ACTUATOR_STATUS` (200002) remains the motor snapshot. The fields production previously ignored — `timeout_ms`, `max_rate_cdeg_s` — are honored by the new motor adapter.
- **Correlation**: `REQUEST_RESULT` (200010) reports `received/accepted/running/completed/stopped/rejected/unknown` as separate `MOTOR_STATE` observations with `CORRELATION_RESULT` outcome, targeted at the originating ground client (255,190). A rejected request is identified independently of the last accepted action. A changed `motor_boot_id` means outcome unknown.
- **Health**: `NODE_HEALTH` (200009) per node; `boot_id` is regenerated every boot (never hardcoded); `reset_reason` uses `RESET_REASON_UNKNOWN` when not actually known. Link counters stay in `LINK_STATUS` (200006) with `link_type=LINK_TYPE_I2C` where applicable.
- **IMU**: standard `SCALED_IMU` (26), mg and mrad/s, boot-ms timestamps, produced by the Pi (sole controller reads IMU registers on the shared bus; Pico 1 never acquires IMU registers, it only forwards). Unmeasured magnetometer/temperature fields are zero on the wire but marked invalid through the Pi's `NODE_HEALTH.sensor_valid_fields`; consumers must not display them as real zeros. GUI labels identify the Pi as acquisition owner.
- **Ping**: standard `PING` broadcast request (targets 0/0) → addressed reply from each responding node (targets = requester), echoing `time_usec`/`seq`; never reply to a reply; replies carry the responder's own source IDs. Ping-all populates one RTT row per node.
- **Presence**: `HEARTBEAT` per node; per-source last-seen uses the receiver's local monotonic clock. Presence alone is never treated as whole-system stability.

### Hard behavioral rules

1. Reboots/reconnects invalidate queued movement: a `boot_id` or `ground_session` change makes pending requests STALE/UNKNOWN; fresh status/session reconciliation precedes new movement. New movement requires a valid reference and fresh feedback, with no active move, fault, or stale/unknown request. `ACTUATOR_STATUS.enabled=0` is the normal idle state and does not by itself mean the motor is unready.
2. Never replay motion after reconnect; never auto-fail-over an uncertain movement between transports. A failed/ambiguous write becomes UNKNOWN, followed by status inspection — not another movement write.
3. Duplicate/reordered requests never apply twice (`request_id` + `ground_session` + `boot_id` correlation at Pi and motor). Busy motors do not restart/extend an action for a concurrent second client; the newer request may only supersede explicitly (SUPERSEDED result to the older one).
4. Wrong target/source/authority, unsupported message, signed frame, or damaged CRC can never energize a motor. SSH bypasses only the radio hop — not routing, identity, authority, or validation.
5. RC override: while RC owns the motor (`mode_pulse_us` above switch threshold), MAVLink movement is DENIED. The autonomous adapter yields before the unchanged RC loop. The protected RC code (`Servo-Chip/rc_control.c`, its header, `rc_loop()` body, receiver mappings) is never modified.

## 3. I2C mailbox contract

One shared 100 kHz bus, Pi is the sole controller. Each peer mailbox transfer is exactly 64 bytes:

- **Pi → peer write**: 64 bytes = one whole unsigned MAVLink 2 frame, zero-padded. Oversized (>64 or payload >52), signed, malformed (bad CRC), or concatenated (more than one frame) writes are rejected entirely and counted in the peer's error/queue-drop counters. No partial application.
- **Pi ← peer read**: 64 bytes = the peer's next queued event/response frame, zero-padded. Empty queue serves the peer's latest persistent status snapshot (`ACTUATOR_STATUS` / `NODE_HEALTH` / heartbeat-shaped availability), so ground retries never depend on a destructive read delivering an event exactly once. No snapshot yet → all zeros ("empty read", distinct from a frame).
- **Priority**: STOP and status/responses outrank routine telemetry; fair round-robin polling keeps every node visible. Sensor samples may coalesce when the queue is full; terminal movement states never drop.
- **ISR discipline**: I2C/UART callbacks only copy bytes or serve prepared buffers; parsing, radio work, IMU reads, and motor control run outside callbacks.
- 64 bytes is frozen provisionally: RP2040 I2C-FIFO is 16 bytes deep, so firmware must drain per-byte IRQs; the host harness tests the 64-byte contract logically and the hardware checklist re-verifies it on wire before physical deployment.

## 4. Transports

- **LoRa (REYAX RYLR998)**: binary MAVLink end-to-end; Base64 only inside the modem's ASCII `AT+SEND`/`+RCV` envelope (240-char payload → ≤168-byte unsigned payload, all supported frames fit the 64-byte mailbox). The current bench uses the shared host modem driver directly over laptop USB-UART and Pi UART, with no radio gateway Pico required. The older ground/comms Pico route is optional. See `docs/direct-radio.md`.
- **SSH**: persistent key-authenticated SSH process to the Pi bridge; raw MAVLink on stdin/stdout, logs on stderr. Uses local SSH config and host-key verification; the GUI never launches an arbitrary shell command from user input.
- **Simulation**: a visibly labeled in-process transport implementing the identical interface; simulated measurements labeled continuously.
- No silent transport fallback; a disconnect stays visible and requires operator reconnect.

## 5. Frozen Python-facing interface (package `boat/`)

`boat` is ground-agnostic client logic shared by GUI, CLI, and scenario tests. Exact signatures (frozen; lanes implement, not redesign):

```python
# boat/ids.py — mirrors protocol/mailbox.h exactly (test-enforced)
SYSTEM_BOAT, COMPONENT_PI, COMPONENT_MOTOR, COMPONENT_COMMS_SENSOR = 1, 1, 154, 158
SYSTEM_GROUND, COMPONENT_GROUND = 255, 190
I2C_ADDR_MOTOR, I2C_ADDR_COMMS, MAILBOX_FRAME, BUS_HZ = 0x08, 0x09, 64, 100_000

# boat/client.py
class Transport(Protocol):
    name: str
    def open(self, on_frame: Callable[[bytes], None]) -> None: ...
    def close(self) -> None: ...
    def send_frame(self, frame: bytes) -> None: ...

@dataclass
class RequestTicket:            # returned by movement calls
    request_id: int
    ground_session: int

class BoatClient:
    def __init__(self, transport: Transport, *, system: int = SYSTEM_GROUND,
                 component: int = COMPONENT_GROUND) -> None: ...
    def connect(self) -> None: ...          # new random ground_session each call
    def disconnect(self) -> None: ...
    @property
    def ground_session(self) -> int: ...
    def set_absolute_angle(self, actuator: int, position_cdeg: int, *,
                           timeout_ms: int = 0, max_rate_cdeg_s: int = 0) -> RequestTicket: ...
    def step(self, actuator: int, delta_cdeg: int, *,
             timeout_ms: int = 0, max_rate_cdeg_s: int = 0) -> RequestTicket: ...
    def stop(self, actuator: int) -> RequestTicket: ...
    def ping_all(self) -> None: ...
    def subscribe(self, callback: Callable[[object], None]) -> Callable[[], None]: ...
```

`subscribe` callbacks receive typed records: `Heartbeat(system, component, age)`, `PongReply(system, component, seq, time_usec, rtt_ms|None)`, `MotorReport` (ACTUATOR_STATUS fields + decode time), `RequestOutcome` (REQUEST_RESULT fields), `NodeHealthReport`, `ImuSample(time_boot_ms, accel_mg, gyro_mrad_s, valid_fields, source=(system,component))`, `RawFrameEvent(bytes, transport, direction)` for the trace log, `TransportEvent(state, detail)`. Encoding uses `protocol.codec` (which reads `arsem.xml`); the client owns `request_id` monotonicity, session generation, pending-ticket correlation, and duplicate suppression. GUI and CLI call `BoatClient`; neither talks to I2C or the other.

## 6. Generated bindings

`protocol/gen/build_dialect.py` mechanically builds `protocol/gen/arsem_standalone.xml` from `arsem.xml` (custom messages/enums) plus the vendored official `HEARTBEAT`/`PING`/`SCALED_IMU` definitions and referenced enums (`protocol/gen/upstream/`, pinned in `SHA256SUMS`), then runs official `mavgen`. Generated C headers are under `protocol/gen/C/arsem_standalone/`; generated Python is `protocol/gen/arsem.py`. `tests/test_contract.py` pins each CRC_EXTRA (200000–200010, HEARTBEAT 50, PING and SCALED_IMU from official generation) and cross-decodes `protocol.codec` with generated Python and C on the host. Firmware includes the generated headers (or `mailbox.h` constants with `mavlink_min.c` on the legacy bench path only).

## 7. Lane ownership (exclusive; no cross-lane edits)

| Lane | Owns | Protected (never touched) |
| --- | --- | --- |
| protocol (Protocol setup, this commit) | `protocol/`, `boat/`, `docs/contract.md` | — |
| pi-router | `onboard/` (new Pi package, launcher, config, IMU driver, SSH bridge, CLI service, bus adapters) | everything else |
| motor | `motor-mcu/` (adapter + host harness) and minimal hook edits in `Servo-Chip/main.c`, `operating_loops.c`, `CMakeLists.txt` | `Servo-Chip/rc_control.c`, `include/rc_control.h`, `rc_loop()` body, receiver mappings, `servo.c` behavior, watchdog project, evidence decks |
| comms-sensor | `comms-mcu/` (Pico 1 firmware) and `ground-gateway/` (ground Pico) | everything else |
| ground-station | `groundstation/` (NiceGUI app, serial/SSH transports, CLI) | everything else |
| scenarios | `scenarios/` (runner, fixtures, runbook) | everything else |

Integration fixes return to the owning lane as a fresh task carrying the original brief and review findings.

## 8. Regression and delivery commands

- Existing regression (must stay green, untouched): `python3 -m unittest discover -s tests -p "test_links.py"` and `cd tests/i2c_sail_motor_test && python3 -m unittest test_bench`.
- Contract tests (Protocol setup): `python3 -m unittest tests.test_contract -v`.
- Scenario suite (one documented command, nonzero on failure, JSON/JUnit + JSONL trace output): `python3 -m scenarios.run` (defined by the scenarios lane).
- Tests never open live ports or energize hardware by default; simulated bus adapters only. Physical checks live in the hardware checklist (`docs/hardware-checklist.md`).

## 9. Remaining checks

GUI browser QA, integration reviews, and firmware compilation passed as recorded in `docs/verification.md`. Physical I2C, radio, IMU, motor, encoder, and RC checks remain unverified. Exact device models, occupied pins, and Pico variants need confirmation before live deployment. Resolve the current pin assignments against the actual board and wiring without changing protected RC code; see `docs/branch-review.md` and `docs/hardware-checklist.md`.
