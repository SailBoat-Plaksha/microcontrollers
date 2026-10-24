# Shared-bus sensor request contract

The Pi is the only I2C controller. It receives ground requests over LoRa/UART1, reads sensor registers, polls MCU mailboxes, and forwards data between addressed devices. A slave requests data by placing a MAVLink request in its outgoing mailbox; it never starts an I2C transfer.

## Addresses and identities

| Device | I2C address | MAVLink system/component |
| --- | --- | --- |
| Pi controller | None | 1/1 |
| Motor Pico | 0x08 | 1/154 |
| Optional comms Pico | 0x09 | 1/158 |
| Sensor Pico | 0x0A | 1/159 |
| LSM9DS1 accel/gyro | 0x6A or 0x6B | None; Pi reads registers |
| LSM9DS1 magnetometer | 0x1C or 0x1E | None; Pi reads registers |
| Ground client | None | 255/190 |

Sensor Pico defaults to GP0 SDA / GP1 SCL. The schematic image does not label GPIOs, so these are configurable defaults. Sixteen MAVLink messages are now supported. Every MCU mailbox transfer remains 64 bytes, carrying one unsigned frame padded with zeros.

## Messages

The XML in `protocol/arsem.xml` is authoritative. Generated CRC extras are 71 for `IMU_REQUEST` (200011) and 32 for `IMU_SAMPLE` (200012). Their maximum frame lengths are 27 and 63 bytes.

`IMU_REQUEST` carries a ground session and request ID, a timeout of 1–5000 ms, maximum cached age, path, and recipient. Path 0 selects a direct Pi read; path 1 selects the sensor Pico response. A maximum age of zero requires a new sensor read. Session zero is reserved for requests generated locally by the sensor Pico. Sensor and motor request counters and trackers are independent.

`IMU_SAMPLE` carries the correlation IDs, acquisition timestamp, sample sequence, acquisition and reporting boot identities, sample age, validity mask, nine signed axis readings, result, path, acquisition owner, and recipient. Units are mg for acceleration, mrad/s for angular rate, and milligauss for magnetic field. Validity bits are acceleration 1, gyroscope 2, and magnetometer 4. Temperature is not included. Invalid axes are zero and must not be interpreted as measurements.

Status values are 1 OK, 2 NOT_READY, 3 UNAVAILABLE, 4 TIMEOUT, and 5 REJECTED. A non-OK response has validity zero. Partial fresh samples are allowed and explicitly identified by their validity mask.

The acquisition component is always Pi component 1. Acquisition time uses the Pi's boot-relative monotonic clock; `acquisition_boot_id` identifies that clock. The reporting node supplies `source_boot_id`. Sample sequence increments only after a new successful acquisition. Age saturates at 65535 ms. A Pico response preserves the Pi acquisition fields and adds time spent in its cache; it does not claim to have read the IMU.

## Direct path

1. Ground sends `IMU_REQUEST`, path 0, to Pi 1/1.
2. The Pi schedules the sensor below STOP and motor traffic, reads the LSM9DS1, and returns `IMU_SAMPLE` to ground.
3. Sensor errors produce a correlated failure with invalid measurements. A conversion that is not ready can be retried within the request deadline without sleeping in the scheduler.

## Pico path and data brokering

1. Ground sends a path-1 request to the Pi.
2. Pi writes the request to the sensor Pico at 0x0A, using source 1/1 and target 1/159.
3. When a new reading is required, the Pico queues a path-0 dependency request to Pi 1/1 with the same correlation IDs.
4. Pi polls that mailbox, validates the dependency against its pending request, reads the IMU, then writes an `IMU_SAMPLE` addressed to the Pico.
5. The Pico validates the supplied sample, clears invalid axes, and queues a path-1 response addressed to ground. Its reporting boot ID is its own; the acquisition owner remains Pi.
6. Pi polls and validates the response, then transmits it to ground.

The Pico may answer from cache only when a nonzero maximum age permits it. A board-local session-zero dependency is answered only to that Pico and must not create an unsolicited ground reply. This implements the same broker pattern other MCU data consumers can use; it does not make the IMU understand MAVLink.

## Validation and scheduling

Bound and expire request tables, queues and response caches. An exact duplicate must not trigger another acquisition; an altered duplicate must be rejected. Check source, target, path, request correlation, deadlines, age, and boot identity. Late replies cannot satisfy a new request. A Pi-generated broker failure may report a board-path error when the Pico is unavailable; successful board-path readings must come from component 159.

MCU callbacks only transfer bytes; parsing and processing run in the main loop. Prepared replies survive short reads and write-only transactions, advance after a complete 64-byte read, and return zeros for over-reads. Queue pressure and sample expiry are visible through health/counters. Sensor traffic must leave motor polling and STOP responsive.

## Configuration and software API

Add optional `[lsm9ds1]` settings `enabled`, `ag_address`, and `mag_address`, and `[sensor]` settings `enabled`, `address`, and `component`. Both default to disabled. The legacy generic IMU reader and LSM9DS1 reader are mutually exclusive. Direct LSM9DS1 access works without a Pico. Direct-radio mode still disables comms address 0x09, but leaves the separately configured sensor node enabled.

The driver is `onboard.lsm9ds1.LSM9DS1(bus, ag_address=..., mag_address=..., clock=...)`. It exposes `initialize()` and `read_sample()`. Samples contain acceleration, gyroscope and magnetometer tuples, validity, and acquisition time. `SensorNotReady` is retryable within a deadline. The bus supplies `read_registers()` and `write_registers()`.

`BoatClient.request_imu(path="direct" | "board", timeout_ms=1000, max_age_ms=0)` returns a request ticket. A separate typed `ImuReading` event contains the full response; the older streamed `ImuSample` record remains compatible. GUI and CLI show all nine axes, units, status, correlation, reporting source, acquisition owner, age and validity. CLI syntax is `imu [direct|board]`. Generated simulation samples stay labeled.

This addition must not edit motor, RC or watchdog firmware. Host tests and firmware builds do not establish physical I2C, IMU or radio operation.
