# Pi onboard service

This package is the single Pi process that owns the shared I2C controller. It uses the frozen 64-byte unsigned MAVLink mailbox and `protocol.codec`; it does not create a second I2C controller or send RS-485 traffic.

## Start

Run these from the repository root so `protocol` and `boat` are importable:

```sh
# Host-only simulated bus; no device nodes are opened.
python3 -m onboard serve --simulate --config onboard/config.example.toml

# Pi service, after configuring I2C controller 1 for 100 kHz.
uv sync --extra onboard
uv run --extra onboard python -m onboard serve --config /etc/arsem/onboard.toml

# Persistent SSH raw-MAVLink stdio bridge. MAVLink bytes only on stdout; logs on stderr.
uv run --extra onboard python -m onboard bridge --config /etc/arsem/onboard.toml

# Direct laptop USB-UART -> LoRa -> boat LoRa -> Pi UART; no comms Pico access.
uv run --extra onboard python -m onboard radio --port /dev/ttyS0 --baud 115200 --config /etc/arsem/onboard.toml

# Named entry point for the main Raspberry Pi boat-brain process.
uv run --extra onboard python -m onboard.boat_brain --port /dev/ttyS0 --baud 115200 --config /etc/arsem/onboard.toml
```

`onboard.boat_brain` is a named launcher for the production direct-radio service. It opens the configured I2C bus once, validates incoming MAVLink and applies the existing source/target/session/STOP checks, routes accepted requests to the configured I2C peers, polls their status plus the optional Pi-owned IMU, then sends returned MAVLink telemetry through the Pi's RYLR998 UART. `serve --simulate` opens no hardware. In `radio` / `boat-brain` mode, `--simulate` replaces the I2C bus only; the selected UART modem is still opened. Use the host scenarios for a fully fake radio path.

The default modem port is `/dev/ttyS0`, UART1 (mini UART) on the Pi Zero family.
It uses GPIO14 TX (header pin 8) and GPIO15 RX (header pin 10) after OS pin setup.
`/dev/serial1` is a secondary-port alias and does not mean hardware UART1.
See [Pi UART1 setup](../docs/direct-radio.md#pi-uart1-setup) before starting.
An explicit `--port` still supports a USB-UART adapter or another configured device.

Direct radio mode lazily imports the shared `boat.radio.LoRaModemTransport` and
uses its frozen constructor:

```python
LoRaModemTransport(port, baudrate=115200, *, local_address=1,
    peer_address=2, network_id=18, band_hz=None, parameter="10,7,1,12",
    serial_factory=None, command_timeout_s=3.0, send_gap_s=0.0, queue_limit=32)
```

Use `--local-address`, `--peer-address`, `--network-id`, `--band-hz` and
`--parameter` to override modem settings. Omitting `--band-hz` leaves the
configured modem frequency unchanged. The modem callback copies complete raw
MAVLink frames into a bounded handoff queue; only the serialized Pi service
loop calls `BoatRouter.submit_frame(..., ingress="radio")` and performs I2C.
Pi replies, motor results/status, health, ping replies and optional IMU samples
return through the same transport. Direct-radio mode sets `comms_enabled=false`
regardless of TOML, so address `0x09` is excluded from polling and writes. On
disconnect or queue overflow the error is logged; unsent movement is cancelled
before the transport closes. Restart the command explicitly to reconnect; no
movement is replayed.

The process holds a nonblocking lock at `/tmp/arsem-onboard-bus-<uid>.lock`; a second service or bridge process exits with an error instead of opening the bus. Run one mode at a time. The bridge accepts fragmented raw MAVLink frames on stdin and writes complete response frames to stdout. SSH host keys and authentication are managed by the caller's normal SSH configuration; pass a fixed remote command rather than user-provided shell text.

The Linux adapter uses `smbus2.i2c_msg` I2C_RDWR transactions. Configure the Pi controller clock to 100 kHz in the operating system before live use; the service validates the frozen rate but cannot change Raspberry Pi device-tree clock settings through smbus2. Each peer write and read is exactly 64 bytes.

## Configuration and IMU

`config.example.toml` lists peer addresses and optional sensor services. The generic `[imu]` reader and `[lsm9ds1]` driver are mutually exclusive. The latter defaults to AG `0x6B` and magnetometer `0x1E`; addresses `0x6A`/`0x1C` are also supported. It uses the onboard `LSM9DS1` driver, which checks both WHO_AM_I values, verifies configuration readbacks, and performs no conversion waits. Keep both drivers disabled until the actual device and wiring are confirmed.

`[sensor]` enables the polled sensor Pico at I2C `0x0A`, MAVLink component 159. Ground clients send `IMU_REQUEST` (message 200011) to Pi 1/1. Path 0 reads the LSM9DS1 directly; path 1 forwards to the sensor Pico, services its session-zero dependency only when polled, then accepts only the matching Pico response. In both paths the Pi remains the sole I2C controller and the `IMU_SAMPLE` (200012) carries the Pi acquisition boot ID, source boot ID, timestamp, age, sequence, status and per-field validity mask. Acceleration is mg, angular rate is mrad/s, and magnetic field is mgauss. `NOT_READY` retries until the request deadline; errors and timeouts return correlated samples with `valid_fields=0`.

Simulation uses the same configured register layout. Its deterministic sample (`0, 0, 1000 mg`, zero angular rate) is a fixture and service logs label every sample as simulated. Simulation does not choose or emulate a specific IMU model.

## Public injection API

```python
from onboard import BoatRouter, ServiceConfig, SimulatedBus

bus = SimulatedBus()
router = BoatRouter(
    bus,
    ServiceConfig(),
    clock=my_monotonic_clock,
    on_frame=handle_raw_mavlink_frame,
    on_event=handle_router_event,
)
router.submit_frame(raw_frame, ingress="ssh")  # or ingress="comms"
router.step()  # one fair scheduler action; sensor actions use bounded transfers
```

`ingress="radio"` uses the same ground source, target, authority, session and
freshness checks. `ServiceConfig.comms_enabled` defaults to `True` for existing
serve/SSH deployments; direct-radio mode explicitly disables the comms peer.

`I2CBus` is the injectable protocol: mailbox read/write, register read/write, and `close()`. `SimulatedBus.inject_frame()` supplies deterministic peer mailbox replies, `register_map` provides writable simulated register banks for WHO_AM_I and configuration readbacks, `inject_fault()` injects read/write/IMU failures, and `operations` records bus access for scenario traces. `BoatRouter.snapshots[(system, component)][message_name]` contains the latest received status/health frame; `node_present`, `node_last_seen`, `node_boot_ids`, `node_health`, `imu_valid_fields`, `imu_last_sample` and `imu_error` expose current observations to a later scenario runner or GUI simulation adapter.

The router validates raw frames with `protocol.codec`, checks exact mailbox padding, allowlists supported messages, enforces ground source `255/190`, movement target `1/154`, and REMOTE_OPERATOR authority `3`. It changes only the MAVLink source IDs to `1/1` when forwarding a movement request. In particular, STEP's signed delta, `request_id` and `ground_session` are not changed or resolved on the Pi; the motor node must resolve STEP once against its fresh valid measured feedback and return `REQUEST_RESULT`.

Movement requests require fresh, fault-free motor `NODE_HEALTH` and a fresh, fault-free `ACTUATOR_STATUS` for the selected actuator. STEP additionally requires `feedback_valid=1`; status must report `REMOTE_OPERATOR` ownership and must not be in progress. `enabled` reports energized output and is not treated as readiness. STOP has an independent bounded priority reserve, so saturated telemetry and response queues cannot consume its capacity. If bounded protected results exhaust request storage, admission can fail safely instead of claiming a request was received.

A motor boot change marks unfinished outcomes unknown under the last known motor boot ID, clears pre-reboot actuator references, retires the active ground session, and forwards the new health frame. The ground client must reconnect to create a new session; the Pi does not rewrite or replay a request. At startup, first boot discovery is not treated as a reboot. A different ground session cannot replace an owner while its motion is queued or in flight; a fresh selected-actuator status is required before a new session can take ownership. An explicit STOP may supersede queued movement. An I2C write exception is ambiguous and is never retried. The Pi only reports a motor boot ID after observing it; it does not invent motor command IDs.

The raw SSH bridge writes MAVLink frames on stdout and logs to stderr. If its bounded stdout queue cannot retain a correlated result, it logs the affected session and request ID as `delivery unknown`; callers must inspect status and must never infer durable delivery from a service-side result alone.

## Firmware integration

Firmware and host harnesses use the checked-in generated header through `-Iprotocol/gen/C/arsem_standalone` and `#include <mavlink.h>`. `protocol/mailbox.h` provides the shared address, 64-byte frame and identity constants. Python host-side encoding/routing uses `protocol.codec` against `protocol/arsem.xml` and the generated standalone dialect. Peer firmware must serve persistent status snapshots on empty mailbox reads, keep terminal request results available, validate its own source/target/authority/session, and never replay STEP after its own uncertain reset.

Run the onboard host checks with:

```sh
uv run --extra onboard python -m unittest onboard.test_router -v
uv run --extra onboard python -m unittest onboard.test_sensor_broker -v
```

Tests are deterministic, use `SimulatedBus`, and never open `/dev/i2c-*`, serial devices or hardware.
