# Boat ground station

Laptop NiceGUI interface and interactive CLI for `boat.BoatClient`. The GUI binds to `127.0.0.1` by default. One process-wide client and state store serve every browser tab. Serial, SSH and client event callbacks run away from the UI event loop; the UI drains a bounded event queue.

## Install and launch

Use a groundstation-local virtual environment; this does not use or modify the motor bench environment:

```sh
python3 -m venv groundstation/.venv
groundstation/.venv/bin/python -m pip install -r groundstation/requirements.txt
groundstation/.venv/bin/python -m groundstation.app
```

Open `http://127.0.0.1:8080`. To choose another local port use `--port 8081`. Binding to a non-loopback address requires an explicit `--host` argument.

The app's connection choices are:

- **LoRa · ground Pico USB**: USB serial carries raw unsigned MAVLink from the ground Pico. The Pico handles the radio modem envelope.
- **SSH · Pi bridge**: persistent SSH through a host alias in `~/.ssh/config`; SSH must already trust the host key. The only remote command is `python3 -m onboard.bridge`; stdout is raw MAVLink and stderr is bridge logging.
- **SIMULATION · generated values**: deterministic local peers encoded through `protocol.codec`; simulated motor feedback and IMU values remain labeled in the interface.

The SSH host field accepts an SSH config alias, not a shell command. The application invokes `ssh -T` with `BatchMode=yes`, `StrictHostKeyChecking=yes`, and `ClearAllForwardings=yes`. It does not disable host checks or prompt for a password.

`groundstation.app.set_transport_factory(kind, factory)` replaces the `serial`, `ssh`, or `simulation` constructor before `build_ui` / `ui.run`. The factory receives `(port_or_alias, baud)` and returns a `boat.client.Transport`. This is the seam for wiring a real onboard-router simulator while keeping traffic on the same `BoatClient` and `protocol.codec` path.

Movement buttons stay disabled until the connected transport has reported recent motor status and a motor health record with valid feedback. The visible range for absolute/step input is the signed `int16` centidegree wire range; it is not a claim about installed hardware travel. A transport reconnect creates a new client session and does not resend movement. An uncertain request needs a fresh motor status before new movement is enabled. STOP is a separate opcode with zero delta.

## CLI

From the repository root:

```sh
python3 -m groundstation.cli --simulate
python3 -m groundstation.cli --serial /dev/ttyACM0 --baud 115200
python3 -m groundstation.cli --ssh boat-pi
```

Commands are `absolute <degrees> [actuator 0|1]`, `step [+|- degrees] [actuator]`, `stop [actuator]`, `ping`, `status`, and `quit`. The CLI exposes the same `BoatClient` methods as the GUI. It records events and reports command ticket IDs; transport reality and feedback validity remain protocol observations.

## Headless checks

```sh
python3 -m unittest discover -s groundstation/tests -v
```

Tests use fake serial/subprocess objects and the codec-backed in-process simulation. They do not open a physical serial port, SSH connection, or hardware bus. Run the commands from the repository root so `boat` and `protocol` resolve.
