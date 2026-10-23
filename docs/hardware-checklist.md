# Hardware verification checklist

This checklist records work still required before physical deployment. Source inspection and host checks do not establish board behavior. Record board identity, wiring, instrument/tool versions, date, operator and observed result for each completed item. Do not treat an unchecked item as completed.

## Identify and map hardware

- [ ] Record exact Pico variant/revision, servo or geared motor model, driver board/model, encoder model and counts specification, RC receiver/transmitter, radio model/configuration, and IMU model/address.
- [ ] With power off, trace and record all GPIO connections against the actual board. Resolve GP9 standby versus prior RC flag use, and GP21 RC mode versus standby use. Resolve `dev` GP18/GP19 RC inputs versus motor direction outputs on those same GPIOs. Confirm encoder pins GP2–GP5 and every other occupied pin.
- [ ] Confirm reference ground, logic levels, pull-ups, motor supply, polarity and driver enable/standby behavior from the actual component documentation and measurements.
- [ ] Establish safe motor disconnect/disable procedure before applying power. Confirm the boot path's `startup_tests()` motion is expected and controlled before flashing/running that path.

## Firmware build and flash

- [ ] Build each intended firmware target with its selected Pico SDK, board definition and compiler; capture the exact command and complete output. Keep build results separate from runtime results.
- [ ] Flash the identified board using the selected image; record image revision, board identity, upload result and boot/USB output. This confirms upload only, not radio, bus or motor function.
- [ ] Verify a safe reset and startup with motor power disconnected; observe startup indicators and whether the device enters the expected control state.

## Shared I2C bus

- [ ] Confirm Pi is the sole controller and both Pico peers plus IMU share the same bus. Verify the actual configured addresses do not collide; current contract proposals `0x08` and `0x09` require device confirmation.
- [ ] Inspect idle SDA/SCL voltage, common ground, pull-ups, wiring and bus rise behavior with motor power isolated. Begin at 100 kHz as specified by `docs/contract.md`.
- [ ] Exercise each peer independently, then the shared bus. Capture address acknowledgements and actual 64-byte mailbox transactions, including empty reads, padding, queue advancement, malformed-frame rejection and recovery. Do not infer this from compilation or a host harness.
- [ ] Add the IMU only after its exact model/address and electrical requirements are known; capture actual register reads and identify Pi as the acquisition owner.

## Radio

- [ ] Confirm both REYAX RYLR998 module identities and matching radio settings, UART wiring/levels, antenna and power requirements.
- [ ] Verify ground Pico USB-to-radio exchange, then boat-side reception and forwarding. Capture modem responses and decoded MAVLink frames in both directions; record link settings and observed range/latency conditions.
- [ ] Confirm disconnect/reconnect remains visible and does not replay a movement command. No radio exchange is established by source inspection.

## Motor and encoder

- [ ] With propeller/linkage or other load disconnected as appropriate, verify driver standby polarity and that reset/startup leaves outputs safe. Check direction and PWM outputs with the motor supply disabled before connecting the motor.
- [ ] Confirm encoder channel polarity and measured counts over a known shaft rotation. Reconcile the 400 versus 830 counts/revolution defaults and gear ratio; do not use either as calibrated data without measurement.
- [ ] Exercise low-power movement in both directions with physical stop available. Observe encoder counts, angle reference, stop behavior, driver temperature/current and timeout behavior. Verify reported position is measured feedback rather than the requested target.
- [ ] Verify limits, stop response, power-cycle behavior and fault handling under the actual servo/driver/encoder combination before any integrated movement command is enabled.

## RC control

- [ ] After resolving the GP9/GP21 and GP18/GP19 conflicts against the real wiring, confirm receiver channel assignments and mode-switch polarity without motor power.
- [ ] Measure pulse widths for rudder, sail and mode at neutral and both endpoints; check loss-of-signal and invalid pulse behavior.
- [ ] With motors safely restrained/disabled initially, confirm RC mode threshold, deadband, direction and proportional output mapping. Then verify RC authority takes priority and autonomous movement is denied while RC owns control.
- [ ] Test transition into and out of RC mode and receiver loss with a safe physical setup. Record actual observed behavior; do not change the protected RC implementation as part of this checklist.
