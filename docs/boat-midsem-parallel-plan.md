# Boat midsem: parallel software delivery plan

Status: implementation authorized on 2026-10-24. Continue from the session handoff and `docs/contract.md`.
Base branch: `i2c+mavlink`, starting at `13b58b2`.
Implementation: GPT-6 Luna through Codex, or models explicitly marked free through OpenCode. Earlier paid-provider preferences no longer apply. Each delecheckd task uses a separate worktree. The parent reviews, integrates and runs QA.

## Current requirements

- One physical 100 kHz I2C bus connects the Pi, both addressed Pico peers and IMU. The Pi is the sole controller.
- REYAX RYLR998 radios; one ground Pico provides USB-to-radio communication.
- Provide servo absolute-angle and angular step commands through NiceGUI and CLI, with LoRa, SSH and labeled simulation transports.
- Preserve existing RC code and receiver behavior. Inspect RC and watchdog branches; exclude RS-485 work.
- Exact IMU, servo/driver/encoder models, occupied pins and Pico variants still need physical confirmation. Report firmware, simulation and physical results separately.
- Complete headless and browser QA. No hardware operation is authorized by the software tests.

## Architecture to freeze

Ground: browser -> laptop NiceGUI -> one selected transport:

- LoRa: USB raw MAVLink -> ground Pico -> UART modem/Base64 -> boat modem -> Pico 1 -> shared I2C mailbox -> Pi.
- SSH: persistent key-authenticated SSH process -> Pi bridge; raw MAVLink on stdin/stdout, logs on stderr. Reuse local SSH configuration and host-key verification. Never launch an arbitrary shell command from a GUI input.

Boat: Pi is the sole controller of one shared 100 kHz I2C bus. Pico 1 (sensors/comms) and Pico 2 (motors) are independently addressed peers. Provisional addresses: motor `0x08`, sensors/comms `0x09`; configurable and verified against real devices before wiring.

The Pi polls Pico 1 for received wireless commands, validates/routes them, writes Pico 2, reads motor status, and returns status through Pico 1. SSH bypasses only the radio hop, not routing, identity, authority or motor validation.

The IMU is another independently addressed peripheral on those same wires. Because the Pi is the sole controller, the Pi reads the IMU registers and timestamps/packages samples. Pico 1 remains the sensor/comms node by role and may consume forwarded samples, but it does not independently acquire IMU registers. The IMU may be physically mounted on its daughter board. GUI labels and packet sources must identify the actual acquisition/processing owner. Samples go directly to SSH clients or through Pico 1 for LoRa; do not add a pointless Pi-to-Pico-to-Pi round trip. If Pico 1 itself must initiate IMU reads, that conflicts with the chosen single-controller topology and is outside this plan.

UART inside each RYLR998 endpoint remains a hardware interface exception. Binary transports carry raw MAVLink; Base64 appears only inside the modem's ASCII interface. Configure and document matching radio settings; do not assume a chosen airtime/telemetry rate is physically proven.

## Contract freeze: first 30-45 minutes

One protocol owner produces a reviewed commit before the dependent implementation lanes start. Every worktree begins from that commit, including the generated bindings. Agents must not independently edit the XML or invent competing transport envelopes.

Keep the eight existing production messages compatible and retain the isolated motor bench as a regression fixture. Add an integrated-demo dialect/firmware path with one authoritative include chain; do not silently redefine bench IDs or position-control fields. Generate/pin C and Python bindings from XML and verify against official generation.

Minimum messages and semantics:

| Function | Minimum data | Contract decision |
| --- | --- | --- |
| Node identity | source system/component; role, capabilities, schema and firmware version | Reuse `NODE_INFO`; source IDs identify the reporting node, not the relay |
| Presence | standard `HEARTBEAT` fields | Per-source last-seen uses local monotonic receive time; presence alone is not whole-system stability |
| Ping | standard `PING.time_usec`, `seq`, `target_system`, `target_component` | Standard broadcast request and addressed response; match echoed timestamp/sequence and responder identity; never reply to a reply |
| Absolute angle | `ACTUATOR_SETPOINT`: command ID, target IDs, actuator, position in centidegrees, freshness timeout, maximum slew rate, control source | Retain existing semantics; implement and test the fields that production currently ignores |
| Relative step / STOP | command/session ID, target IDs, actuator, opcode (STEP or STOP), signed delta centidegrees for STEP, freshness, slew limit, control source | New compact control message unless a suitable standard command is selected at freeze; Pi resolves a relative step exactly once against valid fresh motor feedback, then issues a bounded absolute target. STOP is distinct from a zero-degree target |
| Motor response | request correlation, boot/session, active/last command ID, state, result, requested/measured angle, measured-angle validity, output-enabled, faults, authority | Report received/accepted/running/completed/stopped/rejected/unknown separately; identify rejected request independently of last accepted action; never substitute commanded angle for measured feedback |
| IMU | sample boot time, acceleration x/y/z, angular-rate x/y/z, field validity, sensor identity | Prefer standard `SCALED_IMU` with documented absent-field/health handling; a compact private sample is justified only if required validity semantics cannot be represented cleanly. Freeze exact types, units and unsupported-field behavior in this check |
| Health | boot/session identifier, uptime, fault bits, reset reason when actually known, RX/TX/error/overflow counters | Keep under mailbox bound; reuse `LINK_STATUS` where applicable; unsupported measurements remain unknown |

Recommended first revision uses standard `HEARTBEAT`, `PING` and `SCALED_IMU`, existing `NODE_INFO`, `ACTUATOR_SETPOINT`, `ACTUATOR_STATUS`, `CONTROL_AUTHORITY` and `LINK_STATUS`, plus compact new STEP/STOP, node-health and request-correlation messages. Do not add battery, GPS, navigation or sensor fusion in this delivery. For `SCALED_IMU`, use boot milliseconds, acceleration in mg and angular rate in mrad/s; mark unmeasured magnetometer/temperature fields invalid through the documented health/validity message rather than showing zero as a real measurement. Add I2C to the link-type enum without changing existing values.

Suggested minimum new fields to settle in the freeze commit:

- STEP/STOP request: `request_id:uint32`, `ground_session:uint32`, `delta_cdeg:int16`, `timeout_ms:uint16`, `max_rate_cdeg_s:uint16`, `target_system:uint8`, `target_component:uint8`, `actuator:uint8`, `opcode:uint8`, `control_source:uint8`. STOP requires zero delta. Source identity is already in the MAVLink header.
- Node health: `boot_id:uint32`, `time_boot_ms:uint32`, `fault_flags:uint16`, `sensor_valid_fields:uint16`, `queue_drops:uint16`, `reset_reason:uint8`, `feedback_valid:uint8`. Reserve an explicit unknown reset reason; the boot identifier must change per boot, not be hardcoded. Link packet/error counters remain in `LINK_STATUS`.
- Request correlation: `ground_session:uint32`, `request_id:uint32`, `motor_boot_id:uint32`, `local_command_id:uint32`, `result:uint8`, `state:uint8`, `target_system:uint8`, `target_component:uint8`. Target the originating ground client. Define completion, denied, invalid, stale, unknown and superseded results without modifying the meaning of existing enum values.

For absolute angle requests, add ground-session and motor-boot correlation through backward-compatible extension fields or a single agreed request envelope; choose exactly one in the freeze commit, then encode it identically in all lanes. No lane independently chooses either option. This is the remaining wire-design review, not permission to maintain two formats.

Standard PING uses zero targets for a request and nonzero targets for a reply; do not treat an addressed reply as a directed request. Ping-all can populate separate per-chip RTT rows. If individually directed probes are a firm requirement, specify a distinct request/reply message or routing envelope at the contract check rather than changing standard PING semantics.

Start with existing boat system ID 1, Pi component 1 and motor component 154; select a noncolliding sensors/comms component and a separate ground system ID. Pi maps an upstream request to one local motor ID under source 1/1. Preserve correlation in responses and deduplicate across both transport modes. Default STEP means an angular jog (for example +/-5 degrees), not raw encoder counts; step size is configurable. Confirm calibrated/relative reference and feedback validity before enabling absolute commands.

Reboots and reconnections invalidate queued movement and require fresh status/session reconciliation. Never replay motion after reconnect or automatically fail over an uncertain movement from LoRa to SSH. A failed/ambiguous write becomes unknown, followed by status inspection, not another movement write.

Freeze mailbox contract: 64-byte fixed reads provisionally, at most one whole unsigned MAVLink frame plus zero padding; reject oversized, signed, malformed or concatenated writes. Define empty reads, queue advancement and uncertain-read handling explicitly. Use persistent motor state snapshots plus a bounded event/response queue; preserve terminal state until acknowledged/queried. Ground retries must not depend on a destructive read delivering an event exactly once. Sensor samples can coalesce; STOP/status outrank routine telemetry, with fair polling so other nodes remain visible. Short callbacks copy bytes/serve prepared buffers; parsing, IMU reads, radio work and motor control occur outside ISR callbacks. Verify the actual core's peripheral read-buffer capacity with build/tests before freezing 64 bytes.

## Implementation worktrees

These are planned lane/branch names. Actual worktree paths and task IDs are recorded when T3 launches each task.

| Lane | Exclusive ownership | Deliverable and check |
| --- | --- | --- |
| `demo/protocol` | `protocol/`, generated bindings, contract docs | Protocol checks: stable XML, identity/session rules, Python/C cross-decode, encoded sizes and CRC fixtures |
| `demo/pi-router` | new onboard Python package, Pi launcher/config, IMU driver | Single bus owner including IMU reads; address table; allowlist; priority/fair scheduler; STEP-to-angle resolution; status correlation; persistent SSH bridge; shared CLI service; simulated and real bus adapters |
| `demo/motor` | new motor transport/autonomous adapter and explicit integration hooks | Existing servo/encoder hardware abstraction; nonblocking autonomous angle moves; no startup movement; bounded travel/runtime; duplicate protection; STOP; preserve RC override code and behavior; actual firmware host harness |
| `demo/comms-sensor` | new Pico 1 firmware and ground-Pico checkway directories | I2C peripheral mailbox; nonblocking RYLR998 driver; bounded radio queues and health; forwarded sensor telemetry; ground USB checkway using same dialect; no IMU-controller role |
| `demo/ground-station` | new NiceGUI package and its dependencies | USB LoRa, SSH and visibly labeled simulation adapters behind the same interface; absolute-angle/STEP/STOP controls and typed CLI; node/RTT/health cards; IMU plots; trace log; export |
| `demo/scenarios` | new scenario suite, fixtures, validation/runbook docs | Deterministic topology/fault runner; consumes production codecs and actual firmware harnesses; independent expectations; machine-readable report and packet trace |

The protocol lane merges first. Remaining lanes run in parallel from that shared commit. Test scaffolding may begin earlier, but final fixtures must use the frozen contract. Avoid cross-lane edits; integration fixes return to the owning implementation agent as a new delecheckd task carrying the original brief, prior result, review findings and unresolved issues. Each review round receives its own stable request ID and task ID.

Freeze the Python-facing interface with the wire contract: one `BoatClient` exposes connect/disconnect, absolute-angle, step, stop, ping-all and event subscription; serial/SSH/simulation implement the same raw-frame transport interface. GUI and CLI call this shared client, not one another and not I2C directly. One Pi process owns the real bus; its router consumes complete validated frames and emits observable events/status. Scenario tests inject clock/bus/transport interfaces. Firmware shares generated messages and mailbox constants rather than depending on Python internals. Record exact signatures and import paths in the contract commit before parallel coding.

All implementation uses T3 `delecheck_task` with `isolation:"worktree"`, `mode:"async"`, explicit provider/model, and retained task IDs. No ordinary top-level conversations substitute for child tasks. No automatic code changes to unrelated production firmware, watchdog project or evidence decks.

RC boundary: no changes to `Servo-Chip/rc_control.c`, its header, the existing `rc_loop()` body, receiver mappings or override semantics. Add the MAVLink/I2C autonomous adapter around this boundary, with an exclusive motor ownership check; it must yield before entering the unchanged RC loop. Do not call the existing blocking `Servo()` from the communication service and claim prompt STOP/override. Inspect the actual call graph and preserve shared primitive behavior. If meeting a requirement would require changing protected RC code, report the precise conflict instead of making that change. Current pin definitions disagree across files (including standby/RC-related GP9/GP21 assignments); resolve the physical map explicitly without silently rewriting protected receiver code.

## GUI acceptance

Use NiceGUI components, locally bound by default, with transport configuration and a conspicuous mode label. Keep GUI/event-loop work separate from blocking serial/SSH/I2C operations. All browser clients share one command owner and state store.

- Connect/disconnect to the selected transport; no automatic movement replay or silent transport fallback.
- Absolute angle entry, +/- angular STEP buttons with configurable increment, and STOP; control limits match firmware. CLI exposes the same operations and validation. Disable new moves while busy, feedback/reference is invalid, or state is uncertain; leave STOP usable according to authority policy.
- Show request sent, controller running, controller completed and rejected/unknown as different observations.
- Per-node heartbeat age, RTT, boot/session, faults and errors. Never paint a sensor healthy merely because its checkway answers a ping.
- Acceleration/angular-rate plots with units, sample age and missing/stale state. Do not invent calibrated attitude/heading from raw IMU readings.
- Packet path/event log with source, destination, request ID, timestamp, transport, result and rejection reason; export JSONL.
- Reconnecting, stale data and unavailable hardware remain visible. Simulated measurements are labeled continuously.

## Packet-Tracer-style software validation

Build a deterministic headless virtual topology with injectable clock and transport faults. Every scenario declares preconditions, stimulus, expected route/state/deadline, observed trace, and PASS/FAIL. The topology display and traces may visualize these results, but are not themselves proof of correctness.

Required scenarios:

1. Ground -> LoRa checkways -> Pico 1 mailbox -> Pi -> Pico 2 -> correlated return status; same test over SSH.
2. Pi reads IMU registers on the shared bus -> telemetry -> SSH ground or Pico 1/LoRa -> ground, with correct units, source identity and freshness.
3. Ping-all yields one matching reply per responding node; replies do not cause storms or forwarding loops.
4. Wrong target/source/authority, unsupported message and damaged CRC cannot energize a motor.
5. Absolute angle and relative STEP obey limits and reference validity. Duplicate/reordered STEP never applies the delta twice; busy motor, command-ID rollover/session reset and concurrent ground clients do not restart or extend an action.
6. STOP preempts routine queued work; motor deadline remains bounded during missing Pi polls/radio loss. Local timeout and remote STOP are distinct claims.
7. Dropped request, dropped acknowledgement, truncated/padded read, fragmented USB stream, malformed modem response and delayed radio delivery produce correct uncertainty/recovery without replay.
8. Motor, sensor, comms Pico and Pi reboots clear stale movement; command/session correlation remains sound after reconnect.
9. IMU disconnect/stale samples, one missing I2C node and queue overflow are shown independently; healthy peers continue being serviced.
10. Inject existing RC mode events into the autonomous adapter, verify it yields ownership and denies MAVLink movement while RC owns the motor, and regression-test the unchanged RC code where feasible. Report existing RC pulse-loss behavior without rewriting it. Real receiver takeover remains a separate physical check.
11. Saturated telemetry cannot starve motor status, STOP or health polls; radio throughput/airtime settings remain configurable until measured.
12. Browser QA: both transport forms, simulation mode, motor transitions, ping, stale/error states, disconnect/reconnect, logs, multiple clients and responsive layout; no unexplained console errors.

Use real generated codecs throughout, actual firmware compiled into host harnesses where feasible, and pseudo-terminal/SSH-process integration fixtures for host transports. Avoid a separate idealized motor implementation that merely agrees with itself.

One documented command runs software scenarios and returns a nonzero status on failure, with JSON/JUnit results and JSONL traces. A separate hardware checklist records firmware compile, flash, I2C exchange, radio exchange, motor motion and RC takeover independently. Tests must not open live ports or energize hardware by default.

## Six-hour delivery checks (from implementation start)

- 0:00-0:45: contract/schema/API freeze and generated bindings.
- 0:45-2:45: four parallel implementation lanes plus scenario work, sharing the frozen contract.
- 2:45-3:45: integrate reviewed commits into `i2c+mavlink`, run regression/scenario suites and firmware builds. Merge one lane at a time; preserve unrelated edits.
- 3:45-4:45: orchestrator browser QA and fault review; implementation agents fix concrete findings in fresh delecheckd rounds.
- 4:45-6:00: freeze, package launch/flash commands, power-on runbook, reproducible reports, screenshots and a hardware proof checklist. Physical rehearsal requires an awake operator and wired boards.

If integration slips, preserve the complete SSH path before optional UI polish; LoRa remains a required deliverable with an explicit pass/pending result. Never substitute a successful SSH or simulated path for real LoRa proof.

## Evidence at planning time

Fresh checks: 8 link tests and 10 motor-bench tests passed using the repository's existing unittest commands. These prove host behavior, not physical radio/I2C/motor operation. No new implementation or browser QA has run yet.

Official references reviewed:

- https://mavlink.io/en/services/ping.html
- https://mavlink.io/en/messages/common.html
- https://arduino-pico.readthedocs.io/en/latest/wire.html
- https://github.com/zauberzeug/nicegui
