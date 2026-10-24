# Sensor browser QA

**Result: PASS** for the requested local simulation and browser checks on the integrated Pi and firmware branch. This retest supersedes the earlier `452ed9b` report; it does not establish physical Pi, Pico, sensor, radio, UART, or I2C operation.

## Tested revision and runtime

- Checkout: `/home/rgcodes/.t3/worktrees/microcontrollers/t3-1b004284`
- Branch: `sensor-browser-retest`
- Tested `HEAD`: `10da3c44431248bc4b235b6d46d29faf9a5fe93d`
- Includes `2ab8250` (Pi `1/1` IMU request target and live sample age) and `a8d4eb7` (Sensor Pico health card and complete dialect packaging).
- NiceGUI ran at `http://127.0.0.1:8092/` with the codec-backed `SimulationTransport`. A temporary wrapper logged the actual encoded outbound simulation frames and delegated them to the current checkout's transport implementation. The test imported the current worktree modules; it did not reuse the older checkout's fake transport.
- T3 Preview was used first. Its two page snapshots failed with the same raw error, so headless Chromium was used for browser interaction at 1366×768 and 390×844. No hardware was accessed.

## Checks

- Both **Read IMU** and **Read via Sensor Pico** were enabled while connected.
- Direct read showed `source 1/1`, acquisition owner `Pi 1/1`, status `OK`, validity `0x0007`, and all nine generated values: acceleration `12, -8, 1000 mg`, angular rate `2, 0, -1 mrad/s`, and magnetic field `31, -24, 8 mgauss`.
- Pico read showed reporting source `1/159` and acquisition owner `Pi 1/1`, status `OK`, validity `0x0007`, and the same complete nine-axis generated values.
- All four node cards rendered: Pi `1/1`, Motor Pico `1/154`, Sensor Pico `1/159`, and Comms Pico `1/158`. In simulation, the Sensor Pico card says **No heartbeat/status**; the card's presence is verified, but simulated Sensor Pico health telemetry is not.
- The captured browser-originated `IMU_REQUEST` frame decoded to target `1/1`. Its frame bytes were `fd0f000001ffbe4b0d03ba28659801000000e80300000001018c24` (ground source `255/190`, direct path).
- Fed that captured request frame into the current `SimulationTransport`; it accepted the request and emitted an `IMU_SAMPLE` with target fields `255/190`. The encoded response frame was `fd3300000101014c0d03ba2865980100000078bbda01010000000100000001000000000007000c00f8ffe80302000000ffff1f00e8ff0800010001ffbe6147`. This is a local codec-backed simulation check.
- With no new IMU request for 5.4 seconds, the displayed age advanced from `319 ms` to `5582 ms`, the validity mask became `0x0000`, every axis displayed invalid, and the reading showed **STALE READING**.
- STOP was available and generated one `ACTUATOR_REQUEST`. The captured outbound frame log contains exactly two `IMU_REQUEST` frames (the direct and Pico reads) and that one STOP frame; disconnect/reconnect added no IMU or movement frame.
- At desktop width 1366 and mobile width 390 CSS pixels, document and body widths matched the viewport; neither layout had horizontal overflow.
- Chromium reported no console errors or uncaught page errors.

## Screenshots

- Desktop, 1366×768: `/tmp/sensor-browser-qa-desktop-1366x768.png`
- Mobile, 390×844: `/tmp/sensor-browser-qa-mobile-390x844.png`

## Raw browser and QA harness errors

T3 Preview snapshot timed out twice with the same raw error:

```text
Preview snapshot failed: Preview automation snapshot timed out after 15000ms.
```

Two initial Chromium helper assertions also timed out because the helper used an exact text match for a label containing additional text, then an over-escaped regular expression. The selectors were corrected and the complete run passed; these were helper errors, not application console errors:

```text
locator.waitFor: Timeout 10000ms exceeded.
  waiting for getByText('SIMULATED GENERATED READING', { exact: true }) to be visible
```

```text
locator.innerText: Timeout 10000ms exceeded.
  waiting for getByText(/Request \\d+ · status/).last()
```

The prior report's browser run on `452ed9b` is historical and has been replaced by this revision and these results.
