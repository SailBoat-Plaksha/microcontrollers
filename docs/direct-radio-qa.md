# Direct radio browser QA

- Tested revision: `295d936729749c01604793c52c24f2ecc1166869` (`fix(groundstation): default to direct modem and clarify optional Pico`)
- App: NiceGUI on `127.0.0.1:8084`
- Browser: T3 preview opened an agent-owned tab and reached the app, but `preview_snapshot` timed out twice (`Preview automation snapshot timed out after 15000ms`). The CUA browser inventory was empty, so the screenshots and interactions below used Chromium headless through Playwright as the documented fallback.
- Fixture: a `/tmp` wrapper added the visible `TEST FIXTURE` banner, kept `LoRaModemTransport` and the direct-radio factory active, and supplied a fake serial factory. Fake UART ACKed configuration and `AT+SEND`; a `SimulationTransport` peer behind the fake modem I/O returned codec-generated Pi/motor replies. No product files or production transport code were changed.

## Results

- **Direct defaults and settings:** first load selected `LoRa · direct USB UART`, `/dev/ttyUSB0`, 115200 baud, local address 2, peer 1, network ID 18, blank band, and `10,7,1,12`. Selecting `/dev/ttyUSB-QA` and 57600 baud reached the real driver through `serial_factory`. Fixture writes were `AT+ADDRESS=2`, `AT+NETWORKID=18`, and `AT+PARAMETER=10,7,1,12`; blank band produced no `AT+BAND` command.
- **Available modes:** the transport menu listed direct USB UART, raw ground Pico USB, SSH Pi bridge, and simulation.
- **Connect and ready:** driver completed config ACKs and displayed `CONNECTED · LoRa · direct USB UART` and `RYLR998 configured and ready; waiting for boat frames`.
- **Ping and status:** outgoing frames were decoded `PING`; simulated peer replies updated Pi and motor RTT, node health, motor status, and Pi-owned IMU. The optional Comms Pico card displayed `Not used in direct USB-UART mode`.
- **STOP and reconnect:** the fake UART captured one `ACTUATOR_REQUEST` with STOP opcode 3; peer result/status frames returned. After disconnect/reconnect, the new session emitted a fresh `PING` and no movement request was replayed. The fixture log contained one actuator request total.
- **Invalid settings:** text frequency `abc` produced the friendly notification `Check modem addresses, network ID and frequency.` An address of 70000 and numeric frequency -1 were rejected before fake serial open, but the detail field displayed raw `ValueError: local_address must be an integer from 0 to 65535` and `ValueError: band_hz must be a positive integer or None`. This is the QA issue to address: present these constructor validation errors as friendly modem-setting messages.
- **Browser diagnostics:** zero browser console errors, zero page errors, and zero failed requests.
- **Layout:** expanded settings and a fixture-only 593-character log message were checked at 1366×768 and 390×844. `documentElement.scrollWidth` equaled viewport width at both sizes (1366 and 390); no horizontal overflow was observed. The long log wrapped inside the trace region.

## Screenshots

Screenshots are in `/tmp` and are not committed to the repository:

- Initial disconnected UI, 1366×768: `/tmp/direct-radio-qa-1366.png`
- Connected UI with expanded settings and long fixture log, 1366 wide: `/tmp/direct-radio-qa-1366-connected.png`
- Connected UI with expanded settings and long fixture log, 390 wide: `/tmp/direct-radio-qa-390-connected.png`

## Limits

This verifies the NiceGUI browser flow, direct-radio factory wiring, real `LoRaModemTransport` behavior against a fake UART, codec framing, and UI rendering against generated peer responses only. It does not verify physical USB-UART, either RYLR998, RF compatibility/range, Pi UART, real Pi motor-health replies, I2C, motor motion, or site regulations. The known callback ACK bug did not reproduce in this fixture run. No product tests were added or run.

## QA verdict

**SHIP WITH FIXES** for this focused browser pass: direct-mode flow and layout worked; replace raw constructor `ValueError` details for invalid address/frequency with friendly setting validation feedback before treating the invalid-settings check as passed.
