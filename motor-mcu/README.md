# Motor Pico adapter

The adapter uses the generated unsigned MAVLink 2 C dialect in
`protocol/gen/C/arsem_standalone/mavlink.h`. Its public host API is in
`adapter.h`:

- `motor_adapter_init()` supplies the I/O callbacks, per-actuator reference
  configuration and a boot identifier.
- `motor_adapter_receive()` validates one zero-padded 64-byte mailbox frame.
- `motor_adapter_poll()` advances nonblocking moves and refreshes status.
- `motor_adapter_mailbox_read()` returns the next queued frame or cycles through
  accepted-command results, rejected-request observations, and actuator status
  snapshots.
- `motor_adapter_process_message()` supports direct generated-message tests.

Pico firmware initializes I2C0 at 100 kHz before entering slave mode at address
`0x08` and uses fixed 64-byte mailboxes. The I2C ISR only copies incoming bytes
and serves a prepared reply. Write-only transactions leave the prepared reply
untouched. Short reads reset the transmit cursor but retain the reply for a
complete retry. A normal read advances the reply after 64 request events; the
master does not need to request a 65th byte. The callback returns zero padding
for over-reads and while the next reply is not ready, so it does not leave the
empty TX FIFO holding SCL low. MAVLink parsing and actuator work run from the
main loop. The demo build disables the USB `ACTUATOR_SETPOINT` motion handler
and the startup motor sweep.

The event handling follows the installed Pico SDK's `pico_i2c_slave` source:
`RD_REQ` invokes one request callback when the master asks for data and the TX
FIFO is empty; the helper invokes `FINISH` on STOP, repeated START, or TX abort.
Raspberry Pi's RP2040 datasheet, section 4.3.10.1.4, specifies that another
`RD_REQ` occurs only after the master ACKs the previous byte and asks for more;
a final NACK ends the read and flushes leftover TX FIFO data. Because this
firmware writes one byte for each request, a normal fixed 64-byte transaction
has 64 requests and needs no sentinel read. The SDK `FINISH` callback itself
does not identify which ending event occurred or report an acknowledged-byte
count. Thus an abnormal abort at the 64th request cannot be distinguished from
a completed read in this API. The Pi's normal exact-length transfer is covered;
aborted transfers still require host-level retry/reconciliation.

`run_host_harness.sh` also compiles the production `pico_mailbox.c` callback and
service code against a fake Pico SDK. Its FIFO model copies callback writes
through a bounded TX FIFO into the master's received bytes, and checks literal
byte expectations, exact and short reads, write-only and repeated-start
transactions, RX overflow, over-read padding, and reads before a reply is
ready. It does not prove physical I2C timing or an actual controller exchange.

Rebuild both targets from this checkout with the installed SDK/toolchain:

```sh
PICO_CMAKE=/tmp/motor-pico-tools/bin/cmake
PICO_SDK=/home/rgcodes/.arduino15/packages/rp2040/hardware/rp2040/6.3.0/pico-sdk
PICO_GCC=/home/rgcodes/.arduino15/packages/rp2040/tools/pqt-gcc/5.0.0-9576866/bin
PICO_NINJA=/tmp/motor-pico-tools/bin/ninja
$PICO_CMAKE -G Ninja -S Servo-Chip -B /tmp/motor-build-pico-txfix2 \
  -DPICO_BOARD=pico -DPICO_SDK_PATH="$PICO_SDK" \
  -DPICO_TOOLCHAIN_PATH="$PICO_GCC" -DCMAKE_MAKE_PROGRAM="$PICO_NINJA" \
  -DCMAKE_TOOLCHAIN_FILE="$PICO_SDK/cmake/preload/toolchains/pico_arm_cortex_m0plus_gcc.cmake"
$PICO_CMAKE --build /tmp/motor-build-pico-txfix2
$PICO_CMAKE -G Ninja -S Servo-Chip -B /tmp/motor-build-231-txfix2 \
  -DPICO_BOARD=pico2 -DPICO_SDK_PATH="$PICO_SDK" \
  -DPICO_TOOLCHAIN_PATH="$PICO_GCC" -DCMAKE_MAKE_PROGRAM="$PICO_NINJA" \
  -DCMAKE_TOOLCHAIN_FILE="$PICO_SDK/cmake/preload/toolchains/pico_arm_cortex_m33_gcc.cmake"
$PICO_CMAKE --build /tmp/motor-build-231-txfix2
```

Both commands above completed successfully with GCC 16.1.0. The RP2040 output
is `/tmp/motor-build-pico-txfix2/Servo-Chip.uf2`; RP2350 output is
`/tmp/motor-build-231-txfix2/Servo-Chip.uf2`. These are build artifacts only;
no board was flashed.

Encoder phases use GPIO edge interrupts on the four configured encoder pins.
The adapter counts both phases and does not sample quadrature from the 10 ms
service loop. A skipped two-bit transition or count saturation invalidates that
actuator's feedback and stops an autonomous move. The raw GPIO handler is
registered only for the encoder pins and coexists with the existing RC GPIO
callback.

Run the deterministic host executable with:

```sh
./motor-mcu/run_host_harness.sh
```

Configure with the Raspberry Pi Pico SDK available to CMake, then build
`Servo-Chip`. `PICO_BOARD` is selected through the CMake cache and defaults to
`pico2`; the SDK chooses the matching platform. The intended demo is on by
default. Angle movement stays disabled until encoder reference validity is
explicitly set:

```sh
cmake -S Servo-Chip -B build-motor \
  -DPICO_BOARD=pico2 \
  -DMOTOR_M1_REFERENCE_VALID=ON -DMOTOR_M2_REFERENCE_VALID=ON \
  -DMOTOR_M1_ZERO_COUNT=<measured-sail-zero> \
  -DMOTOR_M2_ZERO_COUNT=<measured-rudder-zero> \
  -DMOTOR_M1_COUNTS_PER_REV=<measured-sail-output-counts-per-rev> \
  -DMOTOR_M2_COUNTS_PER_REV=<measured-rudder-output-counts-per-rev>
cmake --build build-motor
```

SDA/SCL, standby GPIO, encoder counts per revolution, zero counts, rate limits
and travel limits are CMake cache settings. Defaults are GP0/GP1 for I2C0,
GP9 for motor standby, 830 encoder counts per output revolution and invalid
references. The 830 value is retained only as a disabled default; it must be
replaced by a measured calibration before enabling movement.

## Integration

`main.c` starts `motor_mcu_start(sail_motor, rudder_motor)` after motor and RC
setup. `operating_loops.c` calls `motor_mcu_service()` through the existing
loop hook. In the intended build, the hook cannot execute the legacy USB
motion path. It is serviced before `rc_loop()` and inside its unchanged body;
when the mode pulse is above 1500 us, an active autonomous move is stopped
before the RC loop applies its control.

## Limits

- Encoder reference is unknown by default. Do not set either reference-valid
  option until zero count and output-shaft counts per revolution are measured.
- `NODE_HEALTH.feedback_valid` is deliberately conservative and becomes true
  only when both actuator references and both encoder captures are valid.
- Rate limiting uses encoder feedback to pause output above the requested rate
  and a PWM cap. Driver, motor and load calibration is still needed to prove a
  physical angular-rate bound.
- A motor reboot changes `boot_id` and clears RAM deduplication state. The Pi
  must observe the new boot ID, reconcile status and issue a new ground session
  before forwarding movement. The frozen request has no motor-boot field, so
  the motor alone cannot identify an old request delayed across its reboot.
- The source map conflict remains: `main.c` currently declares standby GP9,
  while `servo.c` and `servo.h` use GP21, which is the RC mode input configured
  in `rc_control.c`. The new autonomous adapter uses its configurable standby
  GPIO and stops before RC control, but unchanged `rc_loop()`/RC primitives
  still call the GP21 implementation. Hardware RC takeover is unsafe until
  that existing map is resolved without changing the protected RC code.
- The software names GP0/GP1 as I2C0 defaults and GP9 as standby default, but
  board routing, pull-up voltage and motor-driver wiring remain unverified.
- Host tests cover the adapter and generated codec. They do not prove I2C
  electrical timing, real encoder calibration, motor movement, RC takeover or
  watchdog behavior.

`run_host_harness.sh` also compiles `host_stdio.c` and runs
`host_integration.py`. The line-based host interface accepts `NOW`, `COUNT`,
`FRESH`, `RC`, and `POLL` controls, plus `ENCODE`/`RX`/`READ` commands for
generated MAVLink requests and raw 64-byte frames. This drives the actual C
adapter through its public frame API for external host scenarios.
