# Pi and sensor Pico integration scenario

Run the compiled sensor callback fixture with:

```sh
python3 -m scenarios.sensor_board
```

The runner compiles `sensor-mcu/sensor_node.c` and `mailbox_transport.c` into a
ctypes fixture. Every sensor mailbox request and reply passes through the real
64-byte callback. The Pi side is the production `BoatRouter`, the ground side
is `BoatClient`, and all register reads and writes use the production
`onboard.lsm9ds1.LSM9DS1` driver against an explicit register map. The map
models AG status `0x03`, magnetometer ready status `0x08`, WHO_AM_I values,
configuration readbacks, signed axis data, and magnetometer auto-increment only
when the driver sends burst address `0xA8`.

The direct route runs without a sensor Pico and checks all nine converted axes,
the exact duplicate's single acquisition and growing sample age, and the lack
of sensor mailbox traffic. Separate checks cover a partial-valid sample, a
correlated identity error with zeroed axes, session-zero local requests routed
back only to Pi, wrong-correlation rejection, and one motor STOP write taking
priority over queued IMU work.

The board route starts with the ground `BoatClient` request and waits for a
real dependency frame emitted by the compiled sensor node. It requires the
dependency to retain the nonzero ground session and matching request ID, then
checks the Pi register read, path-1 response, ground target, axis values,
duplicate behavior, and age. On the original parent Pi R1 this check is
expected to fail: `_accept_imu_request` accepts only session-zero dependencies
and guesses the sole parent, while the C firmware preserves the ground session
and request ID. The runner reports this as `FAIL` with the exact reason; it
does not fabricate Pico result frames or weaken the assertion. Re-run after the
separately owned Pi R2 integration.

The host fixture does not prove Pico electrical timing, physical I2C behavior,
LSM9DS1 presence, or radio delivery.
