# Ground LoRa Quick-Check (Pico AT Bridge)

## 1. Topology & Electrical Specifications
- **Topology**: Laptop USB CDC -> Raspberry Pi Pico -> UART0 -> REYAX RYLR998 modem.
- **Pinout**: Pico GP0 (Pin 1, UART0 TX) -> RYLR998 RX; Pico GP1 (Pin 2, UART0 RX) <- RYLR998 TX.
- **Electrical**: Common GND mandatory; 3.3V LVCMOS logic levels.
- **Power**: RYLR998 module (STM32 MCU + Semtech SX1262 transceiver, REYAX AT firmware) operates at 2.8V–3.6V (3.3V typ). Current consumption peaks at ~140 mA during +20 dBm RF transmit (per REYAX datasheet). Ensure host USB / Pico 3V3 rail supplies adequate current.

## 2. Pico Firmware Deployment (~2 min)
Partner agent provides `ground-gateway/main.py` (AT default) and `at_bridge.py` (or `uart_bridge.py`).
If `mpremote` is installed on host (not auto-installed in environment):
```sh
mpremote connect /dev/ttyACM0 cp ground-gateway/main.py :main.py
mpremote connect /dev/ttyACM0 cp ground-gateway/at_bridge.py :at_bridge.py
mpremote connect /dev/ttyACM0 reset
```
*(Alternatively, copy files via Thonny or mount MicroPython filesystem; do not re-flash unless required).*

## 3. Local Modem AT Probe (~1 min, No Boat Needed)
Local modem verification tests USB CDC -> Pico UART0 bridge -> local RYLR998 without boat/RF.
Host command (implemented by partner agent):
```sh
python -m groundstation.modem_check --port /dev/ttyACM0 --timeout 1
```
**RYLR998 AT query syntax (commands terminate with `\r\n`):**
- `AT\r\n` -> `+OK\r\n` (Bridge and modem responsiveness)
- `AT+VER?\r\n` -> `+VER=<version>\r\n` (Firmware version)
- `AT+UID?\r\n` -> `+UID=<uid>\r\n` (Internal STM32 unique ID)
- `AT+ADDRESS?\r\n` -> `+ADDRESS=2\r\n` (Default ground address)
- `AT+NETWORKID?\r\n` -> `+NETWORKID=18\r\n` (Network ID)
- `AT+PARAMETER?\r\n` -> `+PARAMETER=10,7,1,12\r\n` (Spreading Factor, Bandwidth, Coding Rate, Preamble)

## 4. Local Response vs RF Path
- **Local Modem Response**: Receiving `+OK` confirms USB CDC, Pico UART0 forwarding, and local modem parsing.
- **RF Path (Two-Modem Link)**: `+OK` on `AT+SEND` only confirms modem buffer acceptance, NOT RF reception. Verifying over-the-air link requires an explicit second modem test reading incoming `+RCV` frames; never infer physical RF propagation from local `+OK`.

## 5. GUI Integration
```sh
uv sync --extra groundstation
uv run --extra groundstation boat-groundstation
```
In NiceGUI (`http://127.0.0.1:8080`), select Pico AT bridge transport, specify port (`/dev/ttyACM0`), and connect.

## 6. Common Failure Causes
1. **Wiring / UART Mismatch**: Wiring to GP4/GP5 (legacy UART1) instead of GP0/GP1 (UART0), swapped TX/RX, or missing GND.
2. **MicroPython REPL Collision**: Active REPL banner or prompt intercepting USB CDC; bridge script must own raw stdin/stdout.
3. **Power Brownout**: Pico 3V3 rail dropping when RYLR998 draws ~140 mA transmit pulse.
4. **Line Endings & AT Syntax**: Missing `\r\n` (CRLF) in AT commands; incomplete parsing of `+OK`.
5. **OS Permissions**: Linux `Permission denied` on `/dev/ttyACM*`. Fix: `sudo usermod -aG dialout $USER` (re-login).

## 7. Port Inventory Status
- Checked `/dev/ttyACM*`, `/dev/ttyUSB*`, `/dev/serial/by-id/*`: none currently enumerated in test environment.
- Use `/dev/ttyACM0` as standard example when plugging in hardware.
