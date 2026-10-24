# Sensor browser QA

**Result: PASS** for the local simulation and codec-backed UI checks below.

## Tested revision

- Checkout: `t3/8329a39f`
- Tested `HEAD`: `452ed9bc873f4734ef3a303b1c626aeb5e9a6bb0`
- The requested parent commit `1e68e15ef6b39d159144318a00fadbb1c16078c1` is not an ancestor of this checkout. The tested `groundstation/app.py`, `boat/client.py`, and `groundstation/transports.py` match that commit byte-for-byte. The current checkout's `docs/sensor-contract.md` differs, and was the contract used for this review.

## Runtime and browser

Started NiceGUI from the repository root, bound to loopback port 8091:

```sh
uv run --extra groundstation python -m groundstation.app --host 127.0.0.1 --port 8091
```

`uv` created the repository `.venv` and installed the groundstation extra. No UART, Pi network, or physical hardware was used. The server and preview tab were closed after QA.

T3 Preview was attempted first. `preview_status` initially returned `available:false`; `preview_open` then returned an available tab. Both snapshots timed out on the same step with the raw error `Preview snapshot failed: Preview automation snapshot timed out after 15000ms.` The browser fallback inventory was empty (`apps:[]`, `browsers:[]`), and opening `iab` returned the raw error `Browser is not available: iab`. Chromium headless was used as the fallback after those failures.

## Checks

- Simulation direct route: request 1 returned `OK`, source `1/1`, acquisition owner `Pi 1/1`, age `0 ms`, validity mask `0x0007`, acceleration `12, -8, 1000 mg`, angular rate `2, 0, -1 mrad/s`, and magnetic field `31, -24, 8 mgauss`. The UI showed `SIMULATED GENERATED READING`.
- Simulation Sensor Pico route: request 2 returned `OK`, source `1/159`, acquisition owner `Pi 1/1`, age `0 ms`, validity mask `0x0007`, and all nine axes in the expected units. The UI kept the Pico reporting source distinct from the Pi acquisition owner and labeled the generated reading.
- Both IMU buttons stayed enabled with a codec-backed fake transport when no motor status was present, confirming that motor readiness is not required for an IMU read.
- The fake transport returned actual codec-encoded `IMU_SAMPLE` frames through `BoatClient`. A partial direct sample (`valid mask 0x0001`) showed acceleration and marked gyro and magnetometer invalid. A partial Pico sample (`0x0005`) showed acceleration and magnetometer and marked gyro invalid.
- An `UNAVAILABLE (3)` response cleared validity to `0x0000` and displayed all axes as invalid. A withheld reply expired as `TIMEOUT (4)`, showed mask `0x0000`, and displayed all axes as invalid. This also confirmed stale valid fields are cleared on failure.
- Clicking the existing STOP control in simulation emitted one `ACTUATOR_REQUEST` event. Disconnect and reconnect did not replay the IMU request or any actuator request.
- At 1366×768 and 390×844, document and body scroll widths matched viewport width; no horizontal overflow was observed. Chromium reported zero console errors and zero uncaught page errors.

## Screenshots

- Desktop 1366×768, positioned at the IMU panel: `/tmp/groundstation-desktop-1366x768.png`
- Mobile 390×844, positioned at the IMU panel: `/tmp/groundstation-mobile-390x844.png`

The pass covers the local browser UI, generated simulation responses, and codec-backed host-side fake responses. It does not establish radio, UART, Pi, sensor-board, I2C, or physical IMU operation.
