# Current execution instructions (2026-10-24)

Continue the implementation using GPT-6 Luna through Codex or OpenCode models explicitly marked free. Use separate worktrees. Earlier model preferences and browser-QA deferrals below are superseded. Use simple technical wording in new code and documentation. The historical notes below describe the interrupted session, not current authorization.

# URGENT HANDOFF — midsem parallel delivery, session interrupted 2026-10-24

For the next agent (orchestrator). Read together with: `docs/boat-midsem-parallel-plan.md` (the plan being executed) and `docs/contract.md` (the Protocol setup freeze, complete). Repo: `/home/rgcodes/Robotics/microcontrollers`, branch `i2c+mavlink`, base commit `13b58b2`. Nothing committed yet — all Protocol setup work is in the working tree only.

## User directives governing this job

1. Execute `docs/boat-midsem-parallel-plan.md` in full, as if sole coding agent on the job.
2. UPDATED mid-session: **"Use 5.3 flash subagents to execute"** — implementation goes to GLM-5.3-Flash subagents (`account:zai-start-plan/GLM-5.3-Flash` from ListModels). The session model (GLM-5.3) orchestrates: contract, reviews, integration, commits.
3. **All visual/browser UI checks are deferred to a later GLM5.3-flash/openai run** (scenario 12, GUI acceptance in browser). Do NOT do browser QA.

## State: DONE (by orchestrator, this session)

Baseline verified green (run both before continuing):
- `python3 -m unittest discover -s tests -p "test_links.py"` (8 tests; run from repo root)
- `cd tests/i2c_sail_motor_test && python3 -m unittest test_bench` (10 tests; MUST run with cwd = that dir; it imports `arsem_test`/`pi_send` directly and g++-compiles `host/probe.cpp`)

Protocol setup contract artifacts AUTHORED (working tree, uncommitted):
- `protocol/arsem.xml` — added: `LINK_TYPE_I2C=5` (existing values untouched); enums `MOVEMENT_OPCODE` (1 ABSOLUTE, 2 STEP, 3 STOP), `MOTOR_STATE` (0 UNKNOWN,1 RECEIVED,2 ACCEPTED,3 RUNNING,4 COMPLETED,5 STOPPED,6 REJECTED), `CORRELATION_RESULT` (0 UNKNOWN,1 COMPLETED,2 DENIED,3 INVALID,4 STALE,5 SUPERSEDED), `RESET_REASON` (0 UNKNOWN…4 BROWNOUT), `SENSOR_VALID_FIELDS` (bitmask 1 accel,2 gyro,4 mag,8 temp); messages `200008 ACTUATOR_REQUEST` (unified envelope: request_id u32, ground_session u32, delta_cdeg i16, timeout_ms u16, max_rate_cdeg_s u16, target_system u8, target_component u8, actuator u8, opcode u8, control_source u8 — the chosen option (b) "single agreed request envelope"; ACTUATOR_SETPOINT untouched for the bench), `200009 NODE_HEALTH` (boot_id u32, time_boot_ms u32, fault_flags u16, sensor_valid_fields u16, queue_drops u16, reset_reason u8, feedback_valid u8), `200010 REQUEST_RESULT` (ground_session u32, request_id u32, motor_boot_id u32, local_command_id u32, result u8, state u8, target_system u8, target_component u8 — targeted at ground 255/190).
- **Verified**: existing CRCs for 200000–200007 unchanged (139,49,131,143,158,201,198,121). New CRC_EXTRA via codec.py: **200008=188 (payload 19B, frame 31B), 200009=90 (16B/28B), 200010=56 (20B/32B)** — all fit the 64-byte mailbox.
- `protocol/mailbox.h` — identities: boat sys 1; Pi comp 1; motor comp 154; comms/sensors comp 158; ground 255/190; I2C addrs motor 0x08, comms 0x09; bus 100 kHz; frame 64 B.
- `protocol/gen/upstream/` — vendored official `common.xml` (sha256 2f19f8c3…), `minimal.xml` (d4c7c734…), `standard.xml` (0c08a5be…) + `SHA256SUMS`. Fact: **HEARTBEAT (id 0) and enums MAV_TYPE/MAV_AUTOPILOT/MAV_MODE_FLAG/MAV_STATE live in minimal.xml; PING (id 4) and SCALED_IMU (id 26) in common.xml.** PING/SCALED_IMU crc_extra are expected 87/170 but NOT yet verified — verify from generation output.
- `docs/contract.md` — the complete freeze document (message set + hard rules, mailbox contract, transports, frozen `boat/` Python interface §5, lane ownership §7, commands §8, deferrals §9). Lane briefs for subagents should point at this file.

## State: NOT DONE — exact next actions

### Step 1 — Protocol setup derived code (one GLM-5.3-Flash subagent; do NOT start lanes before this is committed)

Brief the agent to deliver (all specified in my design; contract.md is the reference):
1. `protocol/gen/build_dialect.py`: mechanically build `protocol/gen/arsem_standalone.xml` = custom messages/enums from `arsem.xml` + verbatim HEARTBEAT/PING/SCALED_IMU + their referenced enums extracted from vendored upstream XML; then official mavgen → C into `protocol/gen/C/` and Python3 into `protocol/gen/arsem.py`. Must run with the venv interpreter `tests/i2c_sail_motor_test/.venv/bin/python` (pymavlink 2.4.49). Generation API that works (see `tests/i2c_sail_motor_test/generate.py`): `from pymavlink.generator import mavgen; mavgen.Opts(output, wire_protocol="2.0", language="C"|"Python3"); mavgen.mavgen(opts, [xml])`. (`python -m pymavlink.tools.mavgen` does NOT work in this venv.)
2. `boat/` package per contract.md §5 exactly: `ids.py` (mirrors mailbox.h), `client.py` (Transport protocol, record dataclasses, BoatClient with connect/disconnect/set_absolute_angle/step/stop/ping_all/subscribe, monotonic request_ids, per-connect random ground_session, REQUEST_RESULT correlation + duplicate suppression, injectable clock).
3. `tests/test_contract.py`: pin all CRC_EXTRA (200000–200010 = values above; HEARTBEAT 50; PING/SCALED_IMU from generated), codec↔generated-Python cross-decode for all 14 messages, frame sizes ≤64, mailbox.h↔boat.ids.py equality, gcc probe cross-decode codec↔generated-C for ACTUATOR_REQUEST+PING.
4. `tests/test_boat_client.py` with a fake in-memory transport (encode correctness, session/request-id behavior, correlation dispatch, dedup).
5. `protocol/mavlink_min.{h,c}`: extend `extra_for` custom[] to 11 entries (add 188, 90, 56), accept PING + SCALED_IMU ids, add defines MAV_ACTUATOR_REQUEST/MAV_NODE_HEALTH/MAV_REQUEST_RESULT/MAV_PING/MAV_SCALED_IMU.
6. `tests/test_links.py`: update the custom[]-table assertion range from `range(200000, 200008)` to `range(200000, 200011)` (regex pulls the whole table).
7. `protocol/README.md`: schema 1.1.0, new messages, generated bindings, upstream pin.
8. Checks: link tests + new tests green; bench regression untouched and green. Fix rounds ≤3 inside the workflow via world.run on failures.

Then orchestrator reviews the diff, fixes anything critical, commits the freeze ("protocol lane merges first" per plan).

### Step 2 — five parallel GLM-5.3-Flash lanes (second workflow run, from the freeze commit)

Exclusive ownership per contract.md §7; briefs point at docs/contract.md + this handoff:
- **pi-router** → `onboard/`: Pi sole-bus-owner package (address table, allowlist, priority/fair scheduler over 0x08/0x09, STEP resolution to bounded absolute, status correlation, persistent SSH bridge (raw MAVLink stdin/stdout, logs stderr), shared CLI service, simulated + smbus2 real bus adapters, configurable IMU driver reading registers from the Pi). smbus2 available in the bench venv.
- **motor** → `motor-mcu/` (+ minimal hooks in `Servo-Chip/main.c`, `operating_loops.c`, `CMakeLists.txt`): I2C-peripheral mailbox firmware adapter, nonblocking autonomous angle moves honoring timeout_ms/max_rate_cdeg_s, STOP, no startup movement, bounded travel/runtime, duplicate protection, RC-owner check that yields before the unchanged RC loop, host harness (pattern: `tests/i2c_sail_motor_test` host shims). PROTECTED, never modify: `Servo-Chip/rc_control.c`, `include/rc_control.h`, `rc_loop()` body, receiver mappings, `servo.c` behavior. Do not call blocking `Servo()` from the comms service. Report (don't fix) the GP9/GP21 pin conflict.
- **comms-sensor** → `comms-mcu/` + `ground-checkway/`: Pico 1 I2C mailbox peripheral (0x09), nonblocking RYLR998 UART driver (Base64 only inside modem envelope), bounded queues + health, telemetry forwarding, no IMU-controller role; ground Pico USB↔radio checkway in same dialect (MicroPython, pattern `Base-Station-LoRA/main.py`).
- **ground-station** → `groundstation/`: NiceGUI app + serial-LoRa/SSH/simulation transports behind the frozen Transport interface, absolute/STEP(±5° default)/STOP controls, node/RTT/health cards, IMU plots with validity/age, JSONL trace export, CLI sharing BoatClient. No blocking I/O on the UI loop; mode always visibly labeled. Headless import/smoke tests only (browser QA deferred).
- **scenarios** → `scenarios/`: deterministic headless runner for plan scenarios 1–11 (12 deferred), injectable clock/transport faults, real codecs + firmware host harnesses, independent expectations, `python3 -m scenarios.run` returning nonzero on failure with JSON/JUnit + JSONL traces; fixtures build in parallel, wiring/integration after lanes land.

Stage 2 of that workflow: scenarios agent wires lane outputs, runs suite; failures loop back (≤3 rounds) to per-lane fixer subagents carrying original brief + failure output.

### Step 3 — orchestrator integration
Merge one lane at a time into `i2c+mavlink`, keep unrelated edits out, run all checks, then packaging: `docs/hardware-checklist.md`, launch/runbook, reproducible reports. Tests must never open live ports or energize hardware. Never commit `__pycache__`.

## Hard protections (violating these = redo)

No changes to: `Servo-Chip/rc_control.c`, `include/rc_control.h`, `rc_loop()` body, receiver mappings, `Power-Board-Watchdog/`, `evidence/`, `Base-Station-LoRA/`, `Remote-Station-LoRA/`, `Master/`, `tests/i2c_sail_motor_test/` (isolated regression fixture), `docs/boat-midsem-parallel-plan.md`. No MAVLink XML edits outside Protocol setup. No signed frames accepted anywhere. STOP ≠ zero-degree target; unknown outcome → status inspection, never a repeat movement write.

## Mechanics for the next session

- Workflow tooling: reload the `dynamic-workflows` skill, then `CreateWorkflow` with `subagent_model: "account:zai-start-plan/GLM-5.3-Flash"` (or per-agent `agent(name, {model:"GLM-5.3-Flash"})`). Phases required; use `world.run` for test checks (nonzero exit = value, not exception); catch per-lane failures inside Promise.all callbacks; ≤3 fix rounds; subagents share the working tree — lanes own disjoint dirs, that is the safety mechanism.
- Venv: `tests/i2c_sail_motor_test/.venv/bin/python` (pymavlink 2.4.49, smbus2, lxml; NO bundled message_definitions — that's why upstream XML is vendored). System python3 is 3.14.7, gcc 16.2.1.
- Known gotcha: bash cwd persists between calls in this harness — use absolute paths.
- Open physical unknowns (report, don't guess): exact IMU model, servo/driver/encoder parts, occupied pins, Pico variants, GP9/GP21 standby/RC pin conflict, LoRa airtime (unmeasured).
