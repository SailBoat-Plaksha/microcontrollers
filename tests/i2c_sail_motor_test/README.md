# Pi Zero → I2C → MAVLink → timed sail-motor bench test

Everything for this experiment lives in this directory. The production sketch,
`protocol/arsem.xml`, and production codecs are unchanged. Open
`i2c_sail_motor_test.ino` in Arduino IDE and upload it to the motor MCU; run
`pi_send.py` on the Pi Zero.

## What the test does

```text
Pi Zero (system 1 / component 1)
  generated SAIL_MOTOR_TEST MAVLink 2 frame: direction + duration_ms + PWM
  one raw I2C write, address 0x08, no register/SMBus prefix
        ↓ SDA/SCL
Pico motor MCU (system 1 / component 154)
  validate MAVLink CRC, source, target, ID, direction, duration, PWM
  drive sail bridge → count local elapsed time → PWM 0 / standby LOW
        ↓ Pi polls with separate 32-byte raw I2C reads
  SAIL_MOTOR_TEST_STATUS: RUNNING → COMPLETED, elapsed time, outputs off
```

This is open-loop timed output, with no encoder/position control or RC mode.
Nothing moves automatically at startup. Forward means GP8 HIGH / GP7 LOW;
backward means GP8 LOW / GP7 HIGH. Actual shaft direction depends on motor wiring.
The rudder outputs are held LOW because standby is shared.

## 1. Wire the test

All motor pin numbers below are **GP numbers from your supplied table**, not
physical header numbers. Use these assignments even though branch code differs.

| Function | Pico GP | Pico header position | Connect to |
| --- | ---: | ---: | --- |
| I2C SDA | 0 | 1 | Pi GPIO2 / SDA, header pin 3 |
| I2C SCL | 1 | 2 | Pi GPIO3 / SCL, header pin 5 |
| Ground | GND | 3, for example | Pi GND, header pin 6; driver GND |
| Sail PWM_A | 6 | 9 | Driver sail PWM input |
| Sail INA_2 | 7 | 10 | Driver sail reverse input |
| Sail INA_1 | 8 | 11 | Driver sail forward input |
| Shared standby | 9 | 12 | Driver STBY input |
| Rudder INB_1 | 10 | 14 | Held LOW by sketch |
| Rudder INB_2 | 11 | 15 | Held LOW by sketch |
| Rudder PWM_B | 12 | 16 | Held LOW by sketch |

Use short SDA/SCL wiring and **3.3 V logic/pull-ups**. The Pi GPIO2/3 lines
already have pull-ups; do not connect pull-ups to 5 V. Power the Pico by USB
and the motor driver/motor from their appropriate supply, with common ground.
The motor does not draw power through an MCU GPIO. Start with the sail motor
unloaded so a timed move cannot drive the mechanism into a hard stop.

The sketch follows the existing driver's standby polarity: LOW disables it,
HIGH enables it. An external approximately 10 kΩ pulldown on GP9/STBY keeps the
driver disabled while the Pico resets and its GPIO is high impedance. A 1-second
RP2040 watchdog resets a stalled main loop. Neither software status nor this
watchdog measures motor current or shaft motion.

## 2. Flash the motor MCU

Use **Earle Philhower's Raspberry Pi Pico/RP2040/RP2350 Arduino core**, as in
the earlier `i2c_pico_led_test`; select Raspberry Pi Pico or Raspberry Pi Pico W
to match your board. This sketch uses that core's slave `Wire` support and is
not intended for the Arduino Mbed RP2040 core.

If the core is not installed, add this Boards Manager URL in Arduino IDE settings:

```text
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```

Open this folder's `i2c_sail_motor_test.ino`, select the board, then upload. The
generated MAVLink C headers are already supplied in `src/mavlink`; keep that
folder with the sketch. No regeneration or extra MAVLink Arduino library is
needed for the upload. Do not copy only the `.ino` to a different folder.

Optional Arduino CLI build, from the repository root:

```bash
arduino-cli compile --fqbn rp2040:rp2040:rpipico \
  --build-path tests/i2c_sail_motor_test/.build/pico \
  tests/i2c_sail_motor_test
```

For Pico W use `rp2040:rp2040:rpipicow`. You can also hold BOOTSEL while connecting
USB and copy the resulting `.uf2` to the `RPI-RP2` volume. Start with motor power
disconnected while flashing. After boot, the driver must remain disabled until
the Pi sends a valid command.

## 3. Set up the Pi Zero

Enable I2C with `sudo raspi-config` → Interface Options → I2C → Yes. Reboot if
prompted. Run these commands on the Pi:

```bash
sudo apt update
sudo apt install -y python3-venv i2c-tools
ls /dev/i2c-1
sudo i2cdetect -y 1
```

Expect device `08` in the scan after the Pico is powered and flashed. A scan
only proves an I2C address ACK; it does not prove MAVLink decoding or motor motion.
If `/dev/i2c-1` is absent, resolve I2C enablement before running the sender.

Copy this **whole test directory** to the Pi, for example to
`~/i2c_sail_motor_test`, excluding `.venv`, `.tools`, `.build`, and `__pycache__`.
For running the test, the only Python dependency is `smbus2`; the generated
`arsem_test.py` supplies the MAVLink implementation. In that directory:

```bash
cd ~/i2c_sail_motor_test
python3 -m venv .venv
.venv/bin/pip install smbus2==0.5.0
.venv/bin/python pi_send.py demo --dry-run
sudo .venv/bin/python pi_send.py status
```

The dry run prints two 22-byte MAVLink frames and never accesses I2C. The status
read should initially show `id=0 state=IDLE dir=0 pwm=0 result=OK`.
Root is used here for `/dev/i2c-1` permissions; if your account already has I2C
access, omit `sudo`. Keep one sender active at a time.

## 4. Run the complete test

Connect motor power, then run these commands **on the Pi**:

```bash
# Forward for 500 ms, then automatically disable the driver.
sudo .venv/bin/python pi_send.py forward --duration-ms 500 --pwm 96

# Backward for 500 ms, then automatically disable the driver.
sudo .venv/bin/python pi_send.py backward --duration-ms 500 --pwm 96

# Full demonstration: forward, stop, 1-second rest, backward, stop.
sudo .venv/bin/python pi_send.py demo --duration-ms 1000 --pwm 96

# Explicit stop can interrupt an active command; status is read-only.
sudo .venv/bin/python pi_send.py stop
sudo .venv/bin/python pi_send.py status
```

The demo sends each motion frame **once**, then reads status every 50 ms. A new
move is sent only after the previous one completed, with a 1-second rest before
reversing. Duration is configurable from 1–5000 ms and PWM from 1–128/255. Very
short moves can complete between polls; use 500–1000 ms for a visible demo.

Typical fields for a one-second command, with approximate polling times:

```text
id=1 state=RUNNING dir=1 pwm=96 elapsed=50ms remaining=950ms result=OK
...
id=1 state=COMPLETED dir=0 pwm=0 elapsed=1000ms remaining=0ms result=OK
PASS: matching command completed with outputs off. RUNNING observed=True. Verify shaft motion separately.
```

Record the Pi console and the motor together: a matching command ID, a valid
CRC-checked status frame, observed forward/backward shaft motion, and output
disable at the requested time form the end-to-end evidence. For precise timing,
measure GP6 PWM / GP9 standby with a logic analyser; Pi polling is not a timing
instrument. The sender checks MCU-reported elapsed time with a 100 ms tolerance.

To demonstrate the local timeout, run a 2000 ms forward command and press Ctrl+C
after seeing RUNNING. The motor must still turn off at its original MCU deadline.
Read `status` afterward to confirm completion. Removing only SDA/SCL while the
motor runs should have the same bounded result; retain common ground and power.

An I2C write ACK alone does not mean the MCU accepted the command. The sender
waits for a matching accepted command ID and completion. It never automatically
retries an ambiguous motion write. If the sender loses the bus, the original
MCU timer still applies; recover the bus and read status before issuing another
move. Use `stop` or remove motor power when an immediate manual stop is needed.

## Wire contract and rejection behavior

`arsem_test.xml` defines `SAIL_MOTOR_TEST` (private ID 200090, 10-byte payload,
at most 22-byte unsigned frame) and `SAIL_MOTOR_TEST_STATUS` (200091, 16-byte
payload, at most 28-byte frame). These IDs lie within the project's private
200000–200099 range documented in `../../protocol/arsem.xml`. This is a small
standalone test dialect, not a replacement for the production dialect.
`ACTUATOR_SETPOINT.timeout_ms` is a freshness timeout for position commands;
it is not repurposed as a timed open-loop motor command here.

Use raw `smbus2.i2c_msg.write` / `i2c_rdwr`, **not** `write_i2c_block_data`, which
adds SMBus command/count semantics. Commands are one complete MAVLink 2 frame
per transaction. Reads are separate 32-byte transfers, containing one generated
MAVLink status frame followed by zero padding. There is no Base64 or register
byte. Signed frames, fragments, concatenated frames, and non-test messages are
rejected. The status frame's own MAVLink payload length tells the Pi where the
frame ends before it checks CRC.

Only source system/component 1/1 and target 1/154 are accepted. IDs must be
nonzero and strictly increase within an MCU boot; the sender reads the last
accepted ID and adds one. Duplicate frames cannot restart or extend motion.
An invalid frame does not alter the active motor command or its deadline. A
new move while busy is rejected; a fresh stop with direction/duration/PWM all
zero interrupts it. Receive-mailbox overflow stops the motor and discards the
queued command. Status `result` records the most recent acceptance/rejection;
`command_id` continues to identify the last accepted command. IDs reset to zero
after reboot; only one Pi sender should control this isolated bench setup.

## Regeneration and software checks

Generated Python and C bindings are checked in. To regenerate them or run the
host checks, use Python 3.13 (verified here), or a supported Python version with
the pinned packages. On a Pi you need not install pymavlink merely to run the
sender; regeneration may require build dependencies if no package wheel exists.

From the repository root:

```bash
python3.13 -m venv tests/i2c_sail_motor_test/.venv
tests/i2c_sail_motor_test/.venv/bin/pip install -r tests/i2c_sail_motor_test/requirements.txt
tests/i2c_sail_motor_test/.venv/bin/python tests/i2c_sail_motor_test/generate.py
tests/i2c_sail_motor_test/.venv/bin/python -m unittest discover \
  -s tests/i2c_sail_motor_test -p test_bench.py -v
```

The host checks compile and execute the actual `.ino` against fake time, GPIO,
and I2C. They exercise generated Python-to-C command decoding and C-to-Python
status decoding, both directions, local timeout without more I2C writes,
duplicate/busy rejection, stop, malformed/CRC-damaged frames, invalid fields,
receive overflow, timer rollover, and sender completion/no-retry behavior.
They do not exercise electrical I2C, real watchdog resets, or actual motor motion.

Verified on 3 October 2026: all 10 new host tests passed, all 8 existing packet-link
tests passed, and Arduino-Pico core 6.2.0 compiled the sketch successfully for
both Pico and Pico W. Pico used 63,852 bytes flash / 9,320 bytes RAM; Pico W used
322,040 bytes flash / 69,996 bytes RAM. No board was flashed and no physical
I2C/motor test was performed in this session.

## References

- [Arduino-Pico Wire master/slave and pin selection](https://arduino-pico.readthedocs.io/en/stable/wire.html)
- [MAVLink's official library generator](https://mavlink.io/en/getting_started/generate_libraries.html)
- [Raspberry Pi GPIO electrical behavior and fixed GPIO2/3 pull-ups](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html)
