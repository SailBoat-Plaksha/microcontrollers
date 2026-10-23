# Scenario 12 browser QA

Date: 2026-10-24

## Tested revision and environment

- Repository revision: `fe6d2bc4123c172b515b598a3bcafb484440ccba` (`test(groundstation): wait for all ping responses before asserting`). It includes `735934b` and `883390e6815418d1ad7653ad70b70bfa22985ca8`.
- Contract: `docs/contract.md`; ground station instructions: `groundstation/README.md`.
- Production GUI server: `groundstation.app` on `127.0.0.1:8082`, using `/home/rgcodes/.t3/worktrees/microcontrollers/t3-e0a3b20c/groundstation/.venv/bin/python`.
- Browser: Chromium 154.0.8037.57, headless, via CDP fallback after T3 preview snapshots timed out twice and preview clicks timed out up to 60 seconds. T3 `preview_status` and `preview_open` succeeded, and `preview_evaluate` worked. CUA reported no available browser; the Chrome DevTools connector reported that its stable Chrome executable was missing. No product code was changed for this fallback.
- The LoRa/SSH form run used a temporary launcher on port 8083 with `app.set_transport_factory` wrappers over `SimulationTransport`. The separate transport checks imported the production `app`, `BoatClient`, `STATE`, and simulation transport, and replaced only the IO factory. No physical serial device, SSH process, hardware, or network endpoint was opened.

## Results

| Check | Result | Evidence |
| --- | --- | --- |
| Simulation connect and fresh reconciliation | PASS | Connected state showed motor status/health, feedback valid, and the simulated transport label. |
| Sail absolute, negative step, positive step, and STOP | PASS | JSONL decoded as actuator 1 with opcodes 1, 2 (`-500` / `+500` cdeg), and 3 (`delta_cdeg=0`). Measured angle progressed 12.50° → 7.50° → 12.50°. |
| Rudder absolute and STOP | PASS | JSONL decoded as actuator 2 with absolute `-625` cdeg, followed by STOP opcode 3. UI showed measured `-6.25°`. |
| Presence and RTT | PASS | Pi `1/1`, Motor Pico `1/154`, and Sensors/comms Pico `1/158` each displayed an RTT row. |
| IMU display | PASS | UI showed Pi acquisition ownership, sample age, acceleration in mg, angular rate in mrad/s, and the simulated sample label. The report displayed magnetometer/temperature as invalid rather than zero measurements. |
| JSONL export and terminal outcomes | PASS | Browser downloaded `/tmp/boat-groundstation-qa/boat-trace.jsonl` (278,788 bytes). All 1,000 exported lines parsed as JSON; 27 `RequestOutcome` records included terminal states 4 and 5. Decoding outbound frames found absolute, step, and STOP opcodes for both actuators. |
| Two browser tabs share state | PASS | T3 preview tabs `tab_2_39bc5d7c-301d-4cd8-b7d8-35507bb1e6d4` and `tab_3_39bc5d7c-301d-4cd8-b7d8-35507bb1e6d4` both showed the same connected simulation, 12.50° measured angle, and Pi IMU data. |
| LoRa form routes through factory and client | PASS | Browser selected LoRa, entered `/dev/fixture-radio` at 57600 baud, and connected the fake adapter. Factory log recorded `serial`, that port, and `57600.0`; the UI marked it as simulation. |
| SSH form routes through factory and client | PASS | Browser selected SSH, entered `fixture-ssh-alias` at 230400 baud, and connected the fake adapter. Factory log recorded `ssh`, that alias, and `230400.0`; no SSH executable was launched. |
| Open failure | PASS | Fake LoRa adapter failure appeared in the production UI as `open failed: simulated open failure`; the adapter remained closed. |
| Send failure and delayed response | PASS | Production client/state check forced an ambiguous fake write failure and confirmed uncertainty. A 350 ms response test remained guarded through accepted/running/completed states (2/3/4). |
| Stale status and invalid feedback | PASS | Production `STATE.can_move_actuator` returned false after status age exceeded five seconds and after a decoded motor health record reported `feedback_valid=0`. |
| Explicit reconnect does not replay | PASS | After an ambiguous send, explicit reconnect established a different ground session and sent zero movement requests on the new adapter. Browser reconnect also produced zero movement requests in the post-reconnect trace window. |
| Browser console | PASS | Chromium CDP session captured no console error entries or uncaught exceptions. T3 preview automation timeouts were tool failures, not page console errors. |
| Desktop/mobile horizontal overflow | PASS on focused retest | The prior failure was resolved by revision `9246460cd8d3550d16a24daa1ad1c5de7c15a47b`; see retest measurements below. |
| Physical LoRa, SSH, and boat hardware behavior | PENDING | Intentionally not exercised; all claims above are simulation or fake-IO software evidence. |

## Resolved defect: trace text expanded the page horizontally

Severity: P2.

At a 1,366 × 768 desktop viewport, the document measured 2,134 CSS pixels wide. In mobile emulation, the layout viewport was 390 × 844, but the document measured 2,036 pixels wide. Both exceed the viewport, so the no-horizontal-overflow requirement fails. The event-log text contains long JSON lines without break opportunities; its `whitespace-pre-wrap` element expands the card/page instead of staying within the available width.

Original reproduction on the pre-fix revision: connect the simulation, allow raw frame events to populate the packet/event log, then inspect `document.documentElement.scrollWidth` against `document.documentElement.clientWidth`. The same overflow occurred at the mobile breakpoint.

### Focused regression retest

- Tested revision: `9246460cd8d3550d16a24daa1ad1c5de7c15a47b` (`HEAD`), including the root column and trace-label wrapping constraints.
- Environment: local NiceGUI server on `127.0.0.1:8082`, using the supplied `t3-e0a3b20c` groundstation virtualenv. Chromium 154 via CDP fallback. Simulation transport only; no hardware or network endpoint was opened.
- T3 preview opened an owned tab and accepted resize, but `preview_snapshot` timed out twice with the raw error `Preview snapshot failed: Preview automation snapshot timed out after 15000ms.` The focused retest then used the existing local Chromium/CDP fallback.
- Desktop 1366×768: `document.documentElement.scrollWidth=1351`, `clientWidth=1351`; no horizontal overflow. The document width is smaller than the viewport because Chromium reserves the vertical scrollbar.
- Mobile 390×844: `scrollWidth=390`, `clientWidth=390`; no horizontal overflow. All eight visible controls (connect, disconnect, ping, absolute angle, both step buttons, STOP, and export) had nonzero layout boxes and remained in the vertically scrollable page.
- The populated trace label was 294 px wide with `scrollWidth=294`; its internal log viewport was 256 px high with `scrollHeight=3936`, `overflow:auto`, and `overflow-wrap:anywhere`. The card was 326 px wide with no horizontal overflow. Long JSON lines wrap, and the log scrolls inside its own viewport.
- Clicked one positive `+ STEP` command and STOP while connected; the UI recorded an actuator request and remained connected. Explicit disconnect/reconnect then returned to `CONNECTED · SIMULATION`.
- Browser console errors and uncaught exceptions: 0.

Retest screenshots and measurements: [desktop connected](</tmp/boat-groundstation-layout-regression/desktop-connected.png>), [mobile connected after reconnect](</tmp/boat-groundstation-layout-regression/mobile-connected-reconnect.png>), [mobile trace log](</tmp/boat-groundstation-layout-regression/mobile-log.png>), and [measurements JSON](</tmp/boat-groundstation-layout-regression/measurements.json>).

## Screenshots and export

- [Connected desktop](</tmp/boat-groundstation-qa/desktop-connected.png>) — 2,134 × 1,589.
- [After both actuators and movement actions](</tmp/boat-groundstation-qa/desktop-after-actions.png>) — 2,177 × 1,589.
- [Mobile viewport after reconnect](</tmp/boat-groundstation-qa/mobile-after-reconnect.png>) — full-page capture, 2,036 × 3,376; viewport was 390 × 844.
- [Downloaded JSONL trace](</tmp/boat-groundstation-qa/boat-trace.jsonl>).

Temporary fixture scripts and their factory-call report are under `/tmp`; no fixture files are tracked in the repository.
