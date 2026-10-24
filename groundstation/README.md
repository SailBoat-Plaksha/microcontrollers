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

- **LoRa · direct USB UART**: primary direct-radio path. The laptop opens the RYLR998 modem on USB-UART, configures its local address, network ID and modulation parameters, then sends codec-validated MAVLink frames as Base64 modem payloads. Local address `2`, expected boat peer `1`, and network ID `18` are the defaults. Both radios must use matching network, frequency and RF parameters. Optional band frequency is sent only when entered; an empty field leaves the modem's stored band unchanged. The Pi is the boat-side modem host and remains the only I2C controller. Modem `+OK` means local acceptance; motor status/outcome is separate Pi/MAVLink feedback.

Direct-radio `send_frame` waits synchronously for the modem ACK and can block up to the configured command timeout. Call it from a worker outside GUI event loops and outside the Pi's sole I2C thread. The groundstation and Pi integration must keep this operation off their UI/I2C execution paths.
- **LoRa · ground Pico USB (raw MAVLink)**: optional legacy mode. USB serial carries raw unsigned MAVLink to a ground Pico gateway, which handles the modem envelope.
- **SSH · Pi bridge**: persistent SSH through a host alias in `~/.ssh/config`; SSH must already trust the host key. The fixed remote command is `python3 -m onboard bridge --config /etc/arsem/onboard.toml`; stdout is raw MAVLink and stderr is bridge logging.
- **SIMULATION · generated values**: local peers encoded through `protocol.codec`; each connection gets a random simulated boot identity, and periodic motor, health, and IMU values remain labeled in the interface.

The SSH host field accepts an SSH config alias, not a shell command. The application invokes `ssh -T` with `BatchMode=yes`, `StrictHostKeyChecking=yes`, and `ClearAllForwardings=yes`. It does not disable host checks or prompt for a password. The Pi installation must expose the `onboard` package to `python3` from its configured environment and support the documented `onboard bridge --config` command.

`groundstation.app.set_transport_factory(kind, factory)` replaces the `serial`, `direct_radio`, `ssh`, or `simulation` constructor before `build_ui` / `ui.run`. The existing `serial`, `ssh`, and `simulation` factories receive `(port_or_alias, baud)`; `direct_radio` also accepts the modem settings as keyword arguments. Each returns a `boat.client.Transport`. This is the seam for injecting a radio serial fake or onboard-router simulator while keeping traffic on the same `BoatClient` and `protocol.codec` path.

Movement buttons stay disabled until the connected transport has reported recent status for the selected actuator and a motor health record with valid feedback. Actuator IDs are sail `1` and rudder `2`. The visible range for absolute/step input is the signed `int16` centidegree wire range; it is not a claim about installed hardware travel. A transport reconnect creates a new client session and does not resend movement. An uncertain request needs fresh motor status before new movement is enabled. STOP is a separate opcode with zero delta.

## CLI

From the repository root:

```sh
python3 -m groundstation.cli --simulate
python3 -m groundstation.cli --lora /dev/ttyUSB0 --baud 115200 --local-address 2 --peer-address 1 --network-id 18 --parameter 10,7,1,12
python3 -m groundstation.cli --lora /dev/ttyUSB0 --band-hz 915000000
python3 -m groundstation.cli --serial /dev/ttyACM0 --baud 115200  # optional raw gateway Pico
python3 -m groundstation.cli --ssh boat-pi
```

Commands are `absolute <degrees> [actuator 1|2]`, `step [degrees [actuator 1|2]]`, `stop [actuator 1|2]`, `ping`, `status`, and `quit`. A bare `step` uses `--step-deg` (default 5); `step -5` sends a negative five degree step. The CLI exposes the same `BoatClient` methods as the GUI. It records events and reports command ticket IDs; transport reality and feedback validity remain protocol observations.

## Headless checks

```sh
python3 -m unittest discover -s groundstation/tests -v
```

Tests use fake serial/subprocess objects and the codec-backed in-process simulation. They do not open a physical serial port, SSH connection, or hardware bus. Run the commands from the repository root so `boat` and `protocol` resolve.

The RYLR998 command framing, ACK behavior, frequency units and receiver-line format follow REYAX's [RYLR998/RYLR498 AT Command Guide](https://reyax.com/upload/products_download/download_file/LoRa_AT_Command_RYLR998_RYLR498_EN.pdf). Physical modem operation, RF compatibility at a site, and regulatory band/power compliance are not established by the host tests.

In direct USB-UART mode, use **Check LoRa modem AT health** in the GUI or enter `modem` in the CLI. It runs the documented `AT` liveness probe, then reads `AT+ADDRESS?`, `AT+NETWORKID?`, `AT+BAND?`, `AT+PARAMETER?`, `AT+VER?`, and `AT+UID?` one at a time. The report checks the command interface, expected local address/network/parameters, configured frequency when one was specified, and that the modem reports version and UID. If frequency is omitted, the current band is shown without comparing it to an assumed value. This checks the attached modem's UART command path and local settings, not remote radio or motor reachability. The manufacturer guide documents `AT` as its liveness probe; the UI does not assume `AT?` is supported.
