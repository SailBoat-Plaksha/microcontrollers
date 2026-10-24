# Boat midsem runbook

Commands below run from the repository root. Host checks use simulation, fakes, or local harnesses; they do not open hardware by default.

## Requirements

- Python 3.11 or newer for the Python packages.
- `g++` for the legacy sail motor bench tests and a C11 compiler (`cc`) for the motor host harness.
- `cmake`, Ninja and a Raspberry Pi Pico SDK ARM toolchain for firmware builds. Motor and radio-relay firmware linked for Pico (RP2040) and Pico 2 (RP2350) with the locally bundled SDK 2.3.1 and ARM GCC 16.1.0; the earlier SDK 2.2.0 combination was not tested. No board was flashed. See `motor-mcu/README.md`, `comms-mcu/README.md`, and [recorded build results](firmware-builds.json) for rebuild commands and image checksums.

## Software-only setup and checks

The root Python tests need no project install:

The current dialect supports fourteen messages (eleven custom and three standard); each complete frame fits the 64-byte mailbox. Standard PING has CRC_EXTRA 237.

```sh
python3 -m unittest discover -s tests -v
```

Run the isolated legacy motor bench tests and the integrated motor adapter harness:

```sh
(cd tests/i2c_sail_motor_test && python3 -m unittest test_bench -v)
./motor-mcu/run_host_harness.sh
```

Run the Pi router tests against its simulated bus:

```sh
python3 -m unittest onboard.test_router -v
```

Install the optional ground station in its own virtual environment and start its local simulation:

```sh
python3 -m venv groundstation/.venv
groundstation/.venv/bin/python -m pip install -r groundstation/requirements.txt
groundstation/.venv/bin/python -m groundstation.app
```

Open `http://127.0.0.1:8080`. The app defaults to loopback and direct USB-UART LoRa. Select simulation explicitly for generated values. The fake transport tests do not open a serial port, SSH connection, or I2C bus:

```sh
python3 -m unittest discover -s groundstation/tests -v
```

The scenario runner writes JSON, JUnit, and JSONL trace files under `scenarios/results/`:

```sh
python3 -m scenarios.run
```

A scenario marked `PENDING` is unfinished, and makes the runner exit nonzero. The final integrated run passes all 11 scenarios. For the current state and commands, see [scenario notes](../scenarios/README.md).

## Pi setup and real launch

The current bench uses direct UART modems at both ends. Follow [direct-radio setup](direct-radio.md); neither radio hop needs a Pico gateway.

Boat-side UART1 uses `/dev/ttyS0`, GPIO14 TX / GPIO15 RX, on the Pi Zero family. Complete the [UART1 OS setup](direct-radio.md#pi-uart1-setup) first; `/dev/serial1` is not a fixed hardware-UART number.

The Pi service and bridge access the real configured I2C controller. Do not run these commands as part of the software-only checks. First install the Pi requirements and prepare `/etc/arsem/onboard.toml` from `onboard/config.example.toml`. Confirm the selected Linux I2C bus number, 100 kHz controller clock, peer addresses, wiring, and device presence. Keep IMU disabled until its exact model, register layout, byte order, and scale factors are known. The example configuration is not hardware calibration.

Install the Pi dependencies and launch the direct-radio service:

```sh
python3 -m pip install -r onboard/requirements.txt
python3 -m onboard radio --port /dev/ttyS0 --baud 115200 --config /etc/arsem/onboard.toml
```

The service has one process-wide bus lock. Run one of `radio`, legacy `serve`, or `bridge` at a time. The SSH bridge command for the ground station is:

```sh
python3 -m onboard bridge --config /etc/arsem/onboard.toml
```

The fixed SSH command runs from the remote account's home directory. Make this checkout importable by that account's `python3`, and install the requirements into that same interpreter. One option, run on the Pi from the repository root, is a user-site path file:

```sh
python3 - <<'PYTHON'
from pathlib import Path
import site
folder = Path(site.getusersitepackages())
folder.mkdir(parents=True, exist_ok=True)
(folder / "arsem-checkout.pth").write_text(str(Path.cwd().resolve()) + "\n")
PYTHON
(cd /tmp && python3 -c 'import onboard, boat, protocol; import smbus2')
```

Configure the GUI SSH host field with a local SSH config alias whose key authentication and host key are already set up. The GUI uses the fixed bridge command above; it does not accept a shell command. The bridge sends MAVLink bytes on stdout and logs on stderr.

For a direct USB-UART modem connection, select **LoRa · direct USB UART** in the GUI or use the CLI:

```sh
python3 -m groundstation.cli --lora /dev/ttyUSB0 --baud 115200
```

Set the device path and baud to match the attached USB-UART adapter. The optional ground-Pico gateway uses the separate raw MAVLink mode and CLI `--serial`. This sends real radio traffic and must only be used after matching both RYLR998 configurations, UART wiring/levels, antenna, and power requirements. A successful process start does not establish radio exchange or range.

## Firmware configuration

Firmware is not compiled or flashed by the host commands above. With the Pico SDK, CMake and the correct board toolchain installed, configure measured motor references before building:

```sh
cmake -S Servo-Chip -B build-motor \
  -DMOTOR_M1_REFERENCE_VALID=ON -DMOTOR_M2_REFERENCE_VALID=ON \
  -DMOTOR_M1_ZERO_COUNT=<measured-sail-zero> \
  -DMOTOR_M2_ZERO_COUNT=<measured-rudder-zero> \
  -DMOTOR_M1_COUNTS_PER_REV=<measured-sail-output-counts-per-rev> \
  -DMOTOR_M2_COUNTS_PER_REV=<measured-rudder-output-counts-per-rev>
cmake --build build-motor
```

Do not enable reference-valid options using example or guessed encoder values. Confirm actual board revision, driver, encoder CPR, travel limits, polarity, GPIO map, and safe power isolation first. The demo build disables the startup motor sweep; the legacy `startup_tests()` sweep remains in the source for builds that use that path. Resolve the current branch pin conflict before applying motor power: `main.c` and `operating_loops.c` declare RC mode GP9 while `rc_control.c` configures GP21; `main.c` assigns standby GP9, while `servo.c`/`servo.h` use standby GP21, overlapping the configured RC mode input. See [branch review](branch-review.md) and [hardware checklist](hardware-checklist.md).

A successful firmware build is not a flash, bus, radio, encoder, or motion check. Complete the hardware checklist one item at a time, recording actual observations.

For the final test totals, build environment, browser checks and remaining physical work, see [verification results](verification.md).
