# Sensor Pico firmware and host harness

This folder implements the sensor Pico as an I2C **slave**. The Raspberry Pi is the only controller on the shared bus. The Pi also talks directly to the LSM9DS1 AG and magnetometer addresses; the sensor Pico never reads IMU registers and never switches or initiates I2C control.

## Shared bus defaults

| Setting | Default |
| --- | --- |
| Bus | I2C0 at 100 kHz |
| SDA / SCL | GP0 / GP1 |
| Sensor Pico address | `0x0A` / component 159 |
| Frame | One unsigned MAVLink 2 frame, zero-padded to 64 bytes |

These are software defaults, not verified board wiring. The schematic does not label the sensor Pico GPIO pins. Override `SENSOR_I2C`, `SENSOR_I2C_SDA_PIN`, `SENSOR_I2C_SCL_PIN`, and `SENSOR_I2C_ADDRESS` at build time after checking the actual board. Confirm the address does not collide with another device. The LSM9DS1 remains directly visible to the Pi (AG `0x6A` or `0x6B`; magnetometer `0x1C` or `0x1E`).

The schematic shows R1/R2 as 470 kΩ. Treat the pull-ups and bus voltage as unverified: measure SDA/SCL idle voltage and effective pull-up resistance with the actual assembled bus before connecting it. Do not assume those schematic values are working I2C pull-ups.

## Request flow

For an incoming ground `IMU_REQUEST` (source 255/190, path 1, destination Pi 1/1), the Pi forwards a request to this node (source 1/1, destination 1/159). This node queues the same session/request correlation as an `IMU_REQUEST` from 1/159 to Pi 1/1, path 0. It does no sensor acquisition. The Pi returns its `IMU_SAMPLE` to 1/159; this firmware checks source, target, correlation, path, acquisition owner, status, and matching nonzero acquisition/source boot IDs. It zeroes axes whose validity bits are clear, preserves Pi timestamp, sequence, and acquisition boot ID, sets its own source boot ID, adds time spent resident in the Pico, and returns path 1 to ground 255/190.

`max_age_ms == 0` always queues a fresh Pi dependency. A positive maximum age may use only a still-valid cached Pi sample. Duplicate requests with identical payload are served from the cached outcome and do not create a second dependency; changed payload under the same key is rejected. Requests and cached outcomes are bounded and expire. Deadline results and drop/reject counters are reported through `NODE_HEALTH`; heartbeat and targeted/broadcast PING handling are enabled. Telemetry can coalesce under output pressure while dependencies and results are retained in the bounded protected queue or accounted in counters.

The public C API also exposes `sensor_node_request_local(node, request_id, timeout_ms, max_age_ms)`. It reserves session 0 for board-local use, queues an equivalent Pi dependency, and routes its result back to Pi 1/1. Ground session 0 is rejected on the mailbox path.

The IRQ callback only stages complete writes and serves bytes from a prepared reply. The main loop parses and advances requests. Short reads retain the staged reply from byte zero; a complete 64-byte read consumes it; additional bytes are zero. This behavior is covered by the host callback test.

## Host build and integration harness

Run host tests and build the stdio harness:

```sh
sensor-mcu/host/run.sh
```

The executable is written to `${TMPDIR:-/tmp}/sensor-mcu-host/sensor-harness`. Its line protocol is:

```text
I <128 hexadecimal characters>  # inject exactly one 64-byte mailbox write
T <absolute_ms>                  # advance injected clock and run main-loop service
O                                # fetch one queued 64-byte reply as 128 lowercase hex characters
```

The harness is built from the same public C node API and mailbox callback used by firmware. It can be driven from Python over stdin/stdout and does not require Pico hardware.

## Pico SDK builds

With the SDK and toolchain named in the integration request:

```sh
export PICO_SDK_PATH=/home/rgcodes/.arduino15/packages/rp2040/hardware/rp2040/6.3.0/pico-sdk
export PATH=/tmp/motor-pico-tools/bin:/home/rgcodes/.arduino15/packages/rp2040/tools/pqt-gcc/5.0.0-9576866/bin:$PATH
cmake -S sensor-mcu -B /tmp/sensor-mcu-pico -G Ninja -DPICO_PLATFORM=rp2040 -DPICO_BOARD=pico
cmake --build /tmp/sensor-mcu-pico
cmake -S sensor-mcu -B /tmp/sensor-mcu-pico2 -G Ninja -DPICO_PLATFORM=rp2350 -DPICO_BOARD=pico2
cmake --build /tmp/sensor-mcu-pico2
```

Builds establish compile/link readiness only. No firmware was flashed and no physical bus or board behavior was tested.
