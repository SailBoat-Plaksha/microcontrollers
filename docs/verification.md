# Software verification results

## Direct USB-UART radio update

Checked on 2026-10-24 in `i2c+mavlink+midsem`, based on `d614fc7` with the scenario writer/callback synchronization change included. The source branch `i2c+mavlink` was left unchanged.

- 135 Python tests passed: root 47, Pi 42, ground station 20, optional ground gateway 16, and isolated motor bench 10.
- All 11 existing system scenarios passed. All six motor and comms C host checks passed.
- All five direct-radio check groups passed: command routing, input rejection, receive-before-ACK, transport failures/reconnect, and STOP queue priority. Four runs passed; the last three JSON summaries were byte-identical.
- The direct fixture uses two production host modem transports, the Pi radio service/router, and the compiled motor adapter through fake UART and bus interfaces. It checks completed ABSOLUTE/STEP commands, duplicate STEP suppression, active-output STOP, and no replay after uncertain sends.
- Modem health queries and the named `onboard.boat_brain` launcher passed unit tests. The earlier direct-mode browser run is recorded in [direct radio browser QA](direct-radio-qa.md); that browser run predates the health-query button.

Commands and wiring steps are in [direct radio setup](direct-radio.md). No physical UART, radio, I2C, IMU, encoder, motor, or RC operation was tested. The firmware and hardware limits below still apply.

## Earlier firmware and gateway verification

Verified on 2026-10-24, source revision `1d7d751ae1562a84da0c53aa7f367043c9e47d71`. The checked-in scenario reports were regenerated after the final IMU recovery fixture change.

## Results

| Check | Result |
| --- | --- |
| Root protocol, links and BoatClient tests | 35 passed |
| Pi router tests | 31 passed |
| Ground station tests | 15 passed |
| Ground gateway tests | 16 passed |
| Isolated motor bench regression | 10 passed |
| Motor C adapter, raw-mailbox integration and Pico callback shim | All three passed |
| Radio relay, mailbox callbacks and actual modem-service C harnesses | All three passed |
| Full software scenarios | 11 passed; 33 individual checks; no failures or pending cases |
| Deterministic reports | JSON, JUnit and JSONL byte-identical across repeated runs |
| Browser QA | 15 software checks passed, including desktop/mobile layout; no console errors |
| Firmware builds | Motor and radio relay each linked for Pico and Pico 2, producing four UF2 images |

Run the commands in [runbook](runbook.md). Machine-readable scenario results are in [results.json](../scenarios/results/results.json), [JUnit](../scenarios/results/junit.xml) and [packet trace](../scenarios/results/trace.jsonl). See [browser report](browser-qa.md) for tested revisions, fake transport boundaries and screenshots. The responsive layout retest used revision `9246460`; later changes affected the router, radio and host scenario fixtures.

## Firmware environment

Builds used CMake 3.31.6, Pico SDK 2.3.1 from the local Arduino-Pico package, and ARM GCC 16.1.0. The earlier SDK 2.2.0 combination was not verified. [Build records](firmware-builds.json) contain the firmware source revision, paths and SHA-256 checksums. Motor reference-valid options were OFF. The radio firmware's Newlib lock hooks assume one core and no allocation or stdio in interrupts.

## Limits and next physical checks

Software scenarios exercise production codecs, the ground gateway, C modem/relay, Pi router, compiled motor adapter and SSH transport/bridge through local fake I/O boundaries. They do not establish real SSH access, electrical I2C behavior, over-air radio delivery, IMU readings, encoder accuracy, motor travel or receiver takeover. No board was flashed and no physical motor was energized.

Resolve the existing GP9/GP21 standby and RC-mode disagreement before operation. Confirm device models, board variants, pin use, signal levels, radio settings, encoder reference and counts per output revolution. New motor angle commands remain disabled until reference validity is explicitly configured. Physical rate limits and RC takeover remain unverified. Complete the [hardware checklist](hardware-checklist.md).

The Pico SDK callback cannot distinguish an abnormal abort exactly as byte 64 is requested from a completed read. Prepared replies survive shorter reads; persistent state remains available for inspection. See [motor mailbox notes](../motor-mcu/README.md).

Protected RC source/header, the `rc_loop()` body, watchdog project, evidence and isolated bench were retained unchanged. No RS-485 changes were included.
