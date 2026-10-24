# Boat ground station

Laptop NiceGUI interface and interactive CLI for `boat.BoatClient`. The GUI binds to `127.0.0.1` by default. One process-wide client and state store serve every browser tab. Serial, SSH and client event callbacks run away from the UI event loop; the UI drains a bounded event queue.

## Install and launch

Use the repository's uv environment from the repository root. The ground station
extra installs NiceGUI; shared radio code and dependencies come from the root
project:

```sh
uv sync --extra groundstation
uv run --extra groundstation python -m groundstation.app
```

Open `http://127.0.0.1:8080`. To choose another local port use `--port 8081`. Binding to a non-loopback address requires an explicit `--host` argument.

The app's connection choices are:

- **LoRa · Pico USB UART0 (AT bridge)**: default primary ground connection. The ground hardware topology is laptop USB -> Pico native USB CDC -> UART0 GP0/1 -> RYLR998. The Pico runs a transparent AT text adapter (NOT a raw MAVLink gateway). The laptop host runs the full `LoRaModemTransport` host AT/Base64/MAVLink stack behind the Pico USB CDC port (default `/dev/ttyACM0`). The Pico transparently streams text commands and receives lines without secondary wrapping; no double Base64 encoding occurs. Local address `2`, expected boat peer `1`, network ID `18`, and parameters `10,7,1,12` are configured on open. An empty band frequency leaves the modem's stored band unchanged. The optional Comms Pico card (1, 158) is not used in this direct host AT mode.
- **LoRa · direct USB UART**: alternative direct-radio path for a direct USB-UART dongle (e.g. `/dev/ttyUSB0`) attached directly to the RYLR998 without the intermediate Pico adapter. Shares the identical underlying `LoRaModemTransport` driver and settings.
- **LoRa · ground Pico USB (raw MAVLink)**: optional legacy mode. USB serial carries raw unsigned MAVLink to a ground Pico gateway running gateway firmware that handles modem framing on the Pico.
- **SSH · Pi bridge**: persistent SSH through a host alias in `~/.ssh/config`; SSH must already trust the host key. The fixed remote command is `python3 -m onboard bridge --config /etc/arsem/onboard.toml`; stdout is raw MAVLink and stderr is bridge logging.
- **SIMULATION · generated values**: local peers encoded through `protocol.codec`; each connection gets a random simulated boot identity, and periodic motor, health, and IMU values remain labeled in the interface.

Direct-radio and Pico AT bridge `send_frame` waits synchronously for the modem ACK and can block up to the configured command timeout. Call it from a worker outside GUI event loops and outside the Pi's sole I2C thread. The groundstation and Pi integration keep this operation off their UI/I2C execution paths.

The SSH host field accepts an SSH config alias, not a shell command. The application invokes `ssh -T` with `BatchMode=yes`, `StrictHostKeyChecking=yes`, and `ClearAllForwardings=yes`. It does not disable host checks or prompt for a password. The Pi installation must expose the `onboard` package to `python3` from its configured environment and support the documented `onboard bridge --config` command.

`groundstation.app.set_transport_factory(kind, factory)` replaces the `pico_at`, `direct_radio`, `serial`, `ssh`, or `simulation` constructor before `build_ui` / `ui.run`. The `serial`, `ssh`, and `simulation` factories receive `(port_or_alias, baud)`; `pico_at` and `direct_radio` also accept modem settings as keyword arguments. Each returns a `boat.client.Transport`. This is the seam for injecting a radio serial fake or onboard-router simulator while keeping traffic on the same `BoatClient` and `protocol.codec` path.

On-demand IMU reads are independent of motor readiness. **Read IMU** sends a direct Pi sensor request; **Read via Sensor Pico** uses the Pi-polled 0x0A sensor board route. The Pi remains the only I2C controller. Each correlated reply shows request ID, status, reporting source, Pi acquisition owner, sample age, validity mask, and all nine axes in mg, mrad/s, and mgauss. Errors and timeouts clear the displayed validity mask. Simulation generates and labels replies for both paths.

Movement buttons stay disabled until the connected transport has reported recent status for the selected actuator and a motor health record with valid feedback. Actuator IDs are sail `1` and rudder `2`. The visible range for absolute/step input is the signed `int16` centidegree wire range; it is not a claim about installed hardware travel. A transport reconnect creates a new client session and does not resend movement. An uncertain request needs fresh motor status before new movement is enabled. STOP is a separate opcode with zero delta.

## CLI

From the repository root:

```sh
python3 -m groundstation.cli --simulate
python3 -m groundstation.cli --lora /dev/ttyACM0 --baud 115200   # Pico USB UART0 AT bridge
python3 -m groundstation.cli --lora /dev/ttyUSB0 --baud 115200   # direct USB-UART adapter
python3 -m groundstation.cli --lora /dev/ttyACM0 --band-hz 915000000
python3 -m groundstation.cli --serial /dev/ttyACM0 --baud 115200  # legacy raw gateway Pico
python3 -m groundstation.cli --ssh boat-pi
```

The existing `--lora` argument accepts `/dev/ttyACM0` directly when using the Pico USB UART0 transparent AT bridge.

Commands are `absolute <degrees> [actuator 1|2]`, `step [degrees [actuator 1|2]]`, `stop [actuator 1|2]`, `imu [direct|board]`, `ping`, `status`, and `quit`. `imu` defaults to the direct Pi route; `imu board` routes through the sensor Pico. The CLI reports each result, including explicit errors and timeouts. A bare `step` uses `--step-deg` (default 5); `step -5` sends a negative five degree step. The CLI exposes the same `BoatClient` methods as the GUI. It records events and reports command ticket IDs; transport reality and feedback validity remain protocol observations.

## Fast Standalone Modem Check CLI

To verify local ground modem reachability and query its AT identity/settings quickly without connecting `BoatClient` or generating any MAVLink motor traffic, use the standalone `groundstation.modem_check` module:

```sh
python3 -m groundstation.modem_check --port /dev/ttyACM0 --baud 115200
python3 -m groundstation.modem_check --port /dev/ttyACM0 --timeout 1.0 --json
```

- **Safety & Scope**: Uses `LoRaModemTransport.open()` and `check_health()` directly WITHOUT initializing `BoatClient`, WITHOUT transmitting any `AT+SEND` commands, and WITHOUT generating any MAVLink motor or telemetry packets.
- **Diagnostics**: Queries and displays the modem's actual reported AT parameters (`AT`, `AT+ADDRESS?`, `AT+NETWORKID?`, `AT+BAND?`, `AT+PARAMETER?`, `AT+VER?`, `AT+UID?`).
- **Band Disclosure**: Open configuration sets local address 2, network ID 18, and parameters `10,7,1,12`. Leaving `--band-hz` blank (default) leaves the modem's existing stored frequency unchanged.
- **Timeout**: Accepts `--timeout` (default 1.0 second), configuring the driver constructor `command_timeout_s`.
- **Exit Code**: Returns `0` on successful verification with all checks passing; returns non-zero (`1`) if the modem is missing, invalid, times out, or reports mismatched parameters. All serial resources and background threads are cleanly closed upon exit.
- **RF Boundary**: Checking the local ground modem proves local USB-UART AT communication and configuration only; it does **NOT** prove boat RF reachability, remote modem connectivity, or over-the-air link quality.

## Headless checks

```sh
uv run --extra groundstation python -m unittest discover -s groundstation/tests -v
```

Tests use fake serial/subprocess objects and the codec-backed in-process simulation. They do not open a physical serial port, SSH connection, or hardware bus. Run the commands from the repository root so `boat` and `protocol` resolve.

The RYLR998 command framing, ACK behavior, frequency units and receiver-line format follow REYAX's [RYLR998/RYLR498 AT Command Guide](https://reyax.com/upload/products_download/download_file/LoRa_AT_Command_RYLR998_RYLR498_EN.pdf). Physical modem operation, RF compatibility at a site, and regulatory band/power compliance are not established by the host tests.

In direct USB-UART mode, use **Check LoRa modem AT health** in the GUI or enter `modem` in the CLI. It runs the documented `AT` liveness probe, then reads `AT+ADDRESS?`, `AT+NETWORKID?`, `AT+BAND?`, `AT+PARAMETER?`, `AT+VER?`, and `AT+UID?` one at a time. The report checks the command interface, expected local address/network/parameters, configured frequency when one was specified, and that the modem reports version and UID. If frequency is omitted, the current band is shown without comparing it to an assumed value. This checks the attached modem's UART command path and local settings, not remote radio or motor reachability. The manufacturer guide documents `AT` as its liveness probe; the UI does not assume `AT?` is supported.
