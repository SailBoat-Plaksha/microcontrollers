# Boat software changes and existing motor code

This describes the integrated `i2c+mavlink+midsem` branch on 2026-10-24. The motor comparison starts at `13b58b2`, before the new demo adapter. It covers committed code visible in this repository, not another person's uncommitted checkout.

## Current command path

```text
Laptop NiceGUI or CLI
  -> USB-UART -> ground RYLR998
  -> wireless -> boat RYLR998
  -> Pi UART1 (/dev/ttyS0, GPIO14 TX / GPIO15 RX)
  -> Pi BoatRouter
  -> shared I2C bus -> motor Pico at 0x08
  -> motor driver and encoder
```

Motor results travel back through the same path. The Pi polls I2C replies; the Pico does not initiate I2C transfers. The optional sensor shares the bus and is read by the Pi. Neither radio endpoint needs a gateway Pico in this setup.

The LSM9DS1 sensor addition is described in [sensor board setup](sensor-board.md). It adds direct reads and Pico mailbox requests while keeping the Pi as the only I2C controller.

## What each software addition does

| Area | Change and reason |
| --- | --- |
| `protocol/arsem.xml`, `protocol/gen/` | Extended the boat message dictionary with actuator requests, node health and correlated results. Added official generated C/Python bindings for the 16 supported messages, including heartbeat, ping and IMU. This gives the Pi, laptop and MCUs the same field order, units and CRC definitions. |
| `protocol/codec.py`, `mavlink_min.c`, `mailbox.h` | Updated supported message encoding/validation and specified 64-byte I2C frames. The old isolated timed-motor bench still has its own definitions and 32-byte reply. |
| `boat/client.py` | Added one shared application client for GUI and CLI: connect, ping, absolute angle, relative STEP, STOP, health and telemetry. It matches results to requests, suppresses duplicates, checks fresh motor state, handles reconnect/reboot, and reports uncertain execution as UNKNOWN instead of repeating movement. |
| `boat/radio.py` | Added the direct host-side RYLR998 driver, used by laptop and Pi. It configures the modem, wraps binary MAVLink in Base64 `AT+SEND`, unwraps `+RCV`, handles fragmented input, timeouts/errors and bounded queues, and prioritizes STOP. Receive callbacks run separately from ACK parsing. Modem health queries read local settings and identity. |
| `groundstation/` | Added NiceGUI controls and an interactive CLI. Both use BoatClient. The interface has per-actuator controls, returned state/position, node age/health, ping RTT, IMU plots and JSONL export. Browser tabs share a client. Direct USB-UART is the default; SSH, raw ground-Pico USB and labeled simulation remain choices. |
| `onboard/` | Added the Pi process that owns I2C, validates and routes requests, polls motor status, reads a configured sensor, and returns telemetry. STOP has priority; queues are bounded; duplicates and stale requests are rejected. UART transmission runs on a separate worker so modem ACK waits do not block I2C polling. Direct mode does not access comms address `0x09`. |
| `onboard/boat_brain.py` | Added a named launcher for that existing radio service. Its default device is now Pi Zero UART1 `/dev/ttyS0`; `--port` remains configurable. It does not configure Linux GPIO or boot settings. |
| `motor-mcu/` and three `Servo-Chip` files | Added the nonblocking motor adapter and its I2C/encoder integration; details below. |
| `comms-mcu/`, `ground-gateway/` | Added optional Pico radio gateways before the topology was simplified. They remain available, but the direct laptop/Pi radio path does not run them. Older Base/Remote station variants also remain. |
| `scenarios/`, tests | Added host checks connecting actual client, router, modem driver and compiled motor code through fake physical interfaces. They exercise command correlation, STEP once, STOP, timeout, faults, reboot, stale telemetry and queue behavior. |
| `pyproject.toml`, `uv.lock`, READMEs | Added a shared installable Python project, separate laptop/Pi extras and launch commands. The existing isolated motor bench has its own uv project. Documentation records setup, branch review, checks and remaining physical work. |

The sensor code is a configurable register reader, not a driver selected for a confirmed sensor model. Its address, layout, scaling and reference measurements still need the actual hardware information.

## What happened to the coworker's motor code

The existing motor implementation remains in `Servo-Chip/`. Git credits the original motor/servo and RC work to Esraaj Sarkar Gupta, including `b54a9bf`, `1a10f0e`, `15fc2d3` and `d38602c`. Git authorship alone cannot identify who is currently developing outside these commits.

Compared with `13b58b2`, only these existing Servo-Chip files changed:

- `CMakeLists.txt`: adds the new adapter and its build options; selects the demo by default; removes forced board/platform overrides.
- `main.c`: starts the adapter and skips the startup motor sweep in that build.
- `operating_loops.c`: switches the existing service hook between the legacy USB command handler and the new adapter.

`servo.c`, `include/servo.h`, `helpers.c`, `rc_control.c` and `include/rc_control.h` are unchanged from that comparison. The body of `rc_loop()` is unchanged. No original motor source was deleted. The separate power-board watchdog code was also retained unchanged.

The new adapter reuses the existing `mState` motor handles, motor pin setup, initialization and PWM helper. The existing RC setup and channel-to-motor code remain active when RC mode is selected.

**The autonomous movement algorithm did change.** I2C/MAVLink commands do not call the older `Servo()`, `encoder_poll()`, `startRotate()` or `stopRotate()`. They use `motor-mcu/adapter.c` plus `pico_mailbox.c` for nonblocking movement, IRQ encoder capture and direct direction/PWM/standby callbacks. These callbacks use the existing handles and `pwmWrite()` helper. This is more than communication glue.

The reason is that the old `Servo()` blocks its caller during a move, preventing that loop from promptly processing a new STOP or RC-mode change. It also assigns the requested angle to its angle field after its loop, including after a timeout. The new adapter needs to keep processing requests and distinguish measured encoder position from the target.

## Which motor implementation runs

`BOAT_MIDSEM_DEMO=ON` is now the CMake default:

- Startup does not call the old motor sweep.
- `service_mavlink()` calls `motor_mcu_service()` rather than the legacy USB handler.
- Motor Pico I2C0 uses GP0 SDA / GP1 SCL, address `0x08`, at 100 kHz by default.
- ABSOLUTE, STEP and STOP use the new adapter. STEP resolves once from current feedback; repeated requests do not repeat it.
- Encoder IRQ capture supplies position; a local deadline stops movement independently of Pi/radio timing.
- RC ownership stops autonomous movement before the existing RC loop drives the motors.

`BOAT_MIDSEM_DEMO=OFF` selects the earlier USB setpoint handler and startup behavior. That switch restores the old path; it does not make its pin map or startup movement suitable for a powered bench without review.

## Motor items still requiring coordination

1. The demo adapter defaults to standby GP9. Legacy `standby()` uses GP21, which is also the effective RC mode input in `rc_control.c`. Keeping the RC source unchanged preserved this existing disagreement; physical takeover is not verified.
2. Encoder reference-valid flags default OFF. The 830 counts/revolution and zero-count defaults are placeholders, not measured calibration. Position commands are rejected until references are explicitly configured.
3. `main.c` still has calibration-pin placeholders `-1` and `-2`; legacy `motor_init()` passes them to GPIO initialization. These require review with the actual board and homing arrangement before flashing for live motor use.
4. Encoder IRQ timing, real travel/rate bounds, driver polarity, motor behavior and RC takeover still require physical checks. Host tests and successful firmware builds do not establish those properties.
5. The checked local refs for `dev`, `dev-watchdog` and `origin/dev-rc_control` contain no newer motor implementation missing from this integration. This review did not fetch the remote or inspect a coworker's uncommitted files.

The power-board watchdog is an AVR rail-indicator program. It was not converted into a Pico control-loop watchdog. Local motor deadlines were added, but no independent hardware watchdog claim is made.

## Install, run and evidence

Laptop: `uv sync --extra groundstation`, then `uv run --extra groundstation boat-groundstation`.

Pi: `uv sync --extra onboard`, then `uv run --extra onboard boat-brain --port /dev/ttyS0 --baud 115200 --config /etc/arsem/onboard.toml`. First complete [UART1 setup](direct-radio.md#pi-uart1-setup) and the [hardware checklist](hardware-checklist.md).

Before this UART-default update, 135 Python tests, 11 system scenarios, five direct-radio check groups and six C host checks passed. After selecting UART1, the 48 root tests, 42 Pi tests and five direct-radio groups passed again. Firmware builds and browser checks have their own tested revisions in [verification](verification.md). All physical interfaces were faked in those host scenarios. No firmware was flashed or motor energized by this work.
