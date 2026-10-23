# Motor Pico adapter

The adapter uses the generated unsigned MAVLink 2 C dialect in
`protocol/gen/C/arsem_standalone/mavlink.h`. Its public host API is in
`adapter.h`:

- `motor_adapter_init()` supplies the I/O callbacks, per-actuator reference
  configuration and a boot identifier.
- `motor_adapter_receive()` validates one zero-padded 64-byte mailbox frame.
- `motor_adapter_poll()` advances nonblocking moves and refreshes status.
- `motor_adapter_mailbox_read()` returns the next queued frame or cycles through
  the two latest correlated request results and two actuator status snapshots.
- `motor_adapter_process_message()` supports direct generated-message tests.

Pico firmware starts the I2C0 slave at address `0x08`, uses 100 kHz bus timing
set by the Pi controller, and receives/sends exactly 64 bytes per mailbox
transfer. The SDK interrupt callback only copies mailbox bytes and fills the
prepared transmit buffer. MAVLink parsing and actuator work run from the main
loop. The demo build disables the USB `ACTUATOR_SETPOINT` motion handler and
the startup motor sweep.

Run the deterministic host executable with:

```sh
./motor-mcu/run_host_harness.sh
```

For a Pico 2 build, configure with the Raspberry Pi Pico SDK 2.2.0 available
to CMake, then build `Servo-Chip`. The intended demo is on by default. Angle
movement stays disabled until encoder reference validity is explicitly set:

```sh
cmake -S Servo-Chip -B build-motor \
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

- No Pico SDK or CMake executable was available in the implementation
  environment, so only the real generated-header host adapter was compiled.
  Firmware was not built or flashed.
- Encoder reference is unknown by default. Do not set either reference-valid
  option until zero count and output-shaft counts per revolution are measured.
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
