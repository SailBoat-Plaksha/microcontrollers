# Pi onboard service

This package is the single Pi process that owns the shared I2C controller. It uses the frozen 64-byte unsigned MAVLink mailbox and `protocol.codec`; it does not create a second I2C controller or send RS-485 traffic.

## Start

Run these from the repository root so `protocol` and `boat` are importable:

```sh
# Host-only simulated bus; no device nodes are opened.
python3 -m onboard serve --simulate --config onboard/config.example.toml

# Pi service, after configuring I2C controller 1 for 100 kHz and installing smbus2.
python3 -m pip install -r onboard/requirements.txt
python3 -m onboard serve --config /etc/arsem/onboard.toml

# Persistent SSH raw-MAVLink stdio bridge. MAVLink bytes only on stdout; logs on stderr.
python3 -m onboard bridge --config /etc/arsem/onboard.toml
```

The process holds a nonblocking lock at `/tmp/arsem-onboard-bus-<uid>.lock`; a second service or bridge process exits with an error instead of opening the bus. Run one mode at a time. The bridge accepts fragmented raw MAVLink frames on stdin and writes complete response frames to stdout. SSH host keys and authentication are managed by the caller's normal SSH configuration; pass a fixed remote command rather than user-provided shell text.

The Linux adapter uses `smbus2.i2c_msg` I2C_RDWR transactions. Configure the Pi controller clock to 100 kHz in the operating system before live use; the service validates the frozen rate but cannot change Raspberry Pi device-tree clock settings through smbus2. Each peer write and read is exactly 64 bytes.

## Configuration and IMU

`config.example.toml` lists the peer addresses and an optional generic register reader. No sensor model is selected. Leave `[imu].enabled = false` until the actual IMU datasheet supplies its address, starting register, six signed 16-bit value ordering, byte order and scale factors. The generic reader expects acceleration XYZ followed by angular-rate XYZ. It converts the configured counts to mg and mrad/s for `SCALED_IMU`; magnetometer and temperature stay invalid in Pi `NODE_HEALTH`.

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
router.step()  # at most one serialized bus transaction
```

`I2CBus` is the injectable protocol: `write_mailbox(address, data)`, `read_mailbox(address)`, `read_registers(address, register, length)`, and `close()`. `SimulatedBus.inject_frame()` supplies deterministic peer mailbox replies, `inject_fault()` injects read/write/IMU failures, and `operations` records bus access for scenario traces. `BoatRouter.snapshots[(system, component)][message_name]` contains the latest received status/health frame; `node_present`, `node_last_seen`, `node_boot_ids`, `node_health`, `imu_valid_fields`, `imu_last_sample` and `imu_error` expose current observations to a later scenario runner or GUI simulation adapter.

The router validates raw frames with `protocol.codec`, checks exact mailbox padding, allowlists supported messages, enforces ground source `255/190`, movement target `1/154`, and REMOTE_OPERATOR authority `3`. It changes only the MAVLink source IDs to `1/1` when forwarding a movement request. In particular, STEP's signed delta, `request_id` and `ground_session` are not changed or resolved on the Pi; the motor node must resolve STEP once against its fresh valid measured feedback and return `REQUEST_RESULT`.

Movement requests require fresh, fault-free motor `NODE_HEALTH` and a fresh, fault-free `ACTUATOR_STATUS` for the selected actuator. STEP additionally requires `feedback_valid=1`; status must report `REMOTE_OPERATOR` ownership and must not be in progress. `enabled` reports energized output and is not treated as readiness. STOP has an independent bounded priority reserve, so saturated telemetry and response queues cannot consume its capacity. If bounded protected results exhaust request storage, admission can fail safely instead of claiming a request was received.

A motor boot change marks unfinished outcomes unknown under the last known motor boot ID, clears pre-reboot actuator references, retires the active ground session, and forwards the new health frame. The ground client must reconnect to create a new session; the Pi does not rewrite or replay a request. At startup, first boot discovery is not treated as a reboot. A different ground session cannot replace an owner while its motion is queued or in flight; a fresh selected-actuator status is required before a new session can take ownership. An explicit STOP may supersede queued movement. An I2C write exception is ambiguous and is never retried. The Pi only reports a motor boot ID after observing it; it does not invent motor command IDs.

The raw SSH bridge writes MAVLink frames on stdout and logs to stderr. If its bounded stdout queue cannot retain a correlated result, it logs the affected session and request ID as `delivery unknown`; callers must inspect status and must never infer durable delivery from a service-side result alone.

## Firmware integration

Firmware and host harnesses use the checked-in generated header through `-Iprotocol/gen/C/arsem_standalone` and `#include <mavlink.h>`. `protocol/mailbox.h` provides the shared address, 64-byte frame and identity constants. Python host-side encoding/routing uses `protocol.codec` against `protocol/arsem.xml` and the generated standalone dialect. Peer firmware must serve persistent status snapshots on empty mailbox reads, keep terminal request results available, validate its own source/target/authority/session, and never replay STEP after its own uncertain reset.

Run the onboard host checks with:

```sh
python3 -m unittest onboard.test_router -v
```

Tests are deterministic, use `SimulatedBus`, and never open `/dev/i2c-*`, serial devices or hardware.
