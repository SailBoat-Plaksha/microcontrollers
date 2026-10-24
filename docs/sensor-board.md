# LSM9DS1 sensor board on the shared bus

The Pi is the only I2C controller. The motor Pico, sensor Pico and LSM9DS1 are addressed peripherals on the same SDA/SCL pair. The Pi receives requests from the ground station over its UART1 LoRa modem and returns measurements over that link.

## Wiring and schematic checks

Connect Pi GPIO2 SDA (header pin 3), GPIO3 SCL (pin 5) and ground to the board's corresponding signals. Start at 100 kHz with short wiring. Sensor Pico firmware defaults to GP0 SDA / GP1 SCL; change its build configuration if the board uses other GPIOs. The screenshot does not label the Pico pins or identify which IMU line is SDA, so wire from actual pin labels or the KiCad netlist rather than line colors.

The LSM9DS1 exposes two I2C addresses: accelerometer/gyroscope at 0x6A or 0x6B and magnetometer at 0x1C or 0x1E. Default software settings use 0x6B and 0x1E. The sensor Pico uses 0x0A, leaving the motor at 0x08. Verify the address straps before enabling the driver.

The screenshot leaves several electrical details unresolved:

- R1 and R2 appear labeled 470 kΩ. Those would be very weak as the only bus pull-ups. Check the actual resistor values and existing Pi/breakout pull-ups before selecting the combined pull-up resistance.
- Use 3.3 V I2C signals and common ground. Check the actual IMU module's supply requirements; a module and a bare LSM9DS1 package are different wiring tasks.
- A bare LSM9DS1 needs its I2C interface-select and address pins configured. The simplified four-wire IMU block does not show those straps or the complete application circuit.
- The LED path does not show a series resistor. Confirm current limiting in the real circuit.
- No Pi bus connector or exact Pico GPIO assignments are visible. Make the shared SDA/SCL connection explicit in the physical wiring.

The register addresses, identity checks, ranges and units follow the [ST LSM9DS1 datasheet](https://www.st.com/resource/en/datasheet/lsm9ds1.pdf). Firmware defaults are not proof that the pictured board is correctly wired.

## Two ways to request a reading

**Direct:** ground asks the Pi for IMU data. The Pi reads the LSM9DS1 registers and returns a correlated nine-axis sample. The sensor Pico can be absent for this mode.

**Through the sensor Pico:** ground asks for a board response. Pi writes the request to the Pico's mailbox. The Pico queues a request for IMU data; Pi collects that request, reads the LSM9DS1 and writes the result back to the Pico. The Pico validates and reports the supplied readings. Pi polls the response and returns it to ground.

The second mode demonstrates data passing between peripherals through the Pi broker. The Pico never starts I2C traffic and never claims to have acquired the sensor reading. Every response identifies both the reporting node and Pi acquisition owner. A maximum cached age of zero requests a new acquisition; a nonzero age permits an explicitly dated cached sample.

## Pi and ground station

Use the existing onboard configuration with the following sections added. Keep the older generic `[imu]` reader disabled when LSM9DS1 is enabled.

```toml
[lsm9ds1]
enabled = true
ag_address = 0x6B
mag_address = 0x1E

[sensor]
enabled = true
address = 0x0A
component = 159
```

Set `sensor.enabled = false` for direct-only operation. The normal direct LoRa mode continues to omit the optional comms Pico at 0x09; it does not disable the sensor Pico.

Start the Pi brain after configuring UART1 and I2C:

```sh
uv sync --extra onboard
uv run --extra onboard boat-brain --port /dev/ttyS0 --baud 115200 --config /etc/arsem/onboard.toml
```

On the laptop:

```sh
uv sync --extra groundstation
uv run --extra groundstation boat-groundstation
```

Connect through direct USB-UART LoRa and use the IMU read controls. The CLI exposes the same requests:

```sh
uv run boat-groundstation-cli --lora /dev/ttyUSB0
# At the interactive prompt:
imu direct
imu board
```

Readings show acceleration in mg, angular rate in mrad/s, and magnetic field in milligauss. Check validity bits before using any axis group. Missing hardware, no ready conversion, expired requests and malformed replies produce explicit failures rather than replacement measurements.

See [sensor firmware](../sensor-mcu/README.md) for build and host checks, and [message contract](sensor-contract.md) for request fields and broker behavior. Physical board operation and over-air exchange require separate verification.
