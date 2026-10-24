# Boat midsem implementation status

The current implementation uses a Pi-controlled shared I2C bus, MAVLink 2 messages, a motor Pico mailbox adapter, a laptop ground station, and simulated host tests. See [runbook](runbook.md) for install and launch commands, [contract](contract.md) for message and mailbox details, and [hardware checklist](hardware-checklist.md) for work that needs real equipment.

## Current software shape

- The Pi is the only I2C controller. Its router handles the motor and comms peers; IMU register reads are owned by the Pi when enabled and configured.
- The current bench uses direct laptop USB-UART and Pi UART LoRa modems, then Pi-controlled I2C to the motor Pico. The earlier radio gateway Picos are optional. Ground station also supports SSH and simulation. See [direct-radio setup](direct-radio.md).
- Movement commands use `ACTUATOR_REQUEST` (message 200008), including absolute target, STEP, and STOP opcodes. The motor Pico resolves STEP once from fresh valid encoder feedback. The Pi forwards the delta without resolving it.
- I2C mailboxes use fixed 64-byte transfers. Each MAVLink frame must fit within the mailbox; current supported frames are at most 64 bytes. Standard PING uses CRC_EXTRA 237.
- Motor demo firmware disables the startup sweep and legacy USB setpoint movement path. Existing RC files and receiver behavior remain protected.

## Checks and remaining work

The integrated software passes 107 Python tests, six C host checks, all 11 system scenarios, and browser QA. Motor and radio firmware compile for Pico and Pico 2 using SDK 2.3.1 and ARM GCC 16.1.0. See [verification results](verification.md) for the test scope and [runbook](runbook.md) for commands. Real Pi, I2C, radio, encoder, RC and motor operation remain unverified.

Before live operation, confirm the exact boards, devices, wiring, addresses, radio settings, measured encoder calibration and pin map. Current branch conflicts are: `rc_control.c` configures mode GP21 while `main.c` and `operating_loops.c` declare GP9; `main.c` declares standby GP9 while `servo.c`/`servo.h` use GP21. Resolve these against actual wiring without changing the protected RC implementation. See [branch review](branch-review.md).

Keep changes within their assigned project areas. Protected RC files and loop behavior, Power-Board-Watchdog, evidence, and the isolated motor bench remain unchanged.
