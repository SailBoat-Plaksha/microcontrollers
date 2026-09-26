# Slide evidence

Run `python evidence/render.py` from the repository root to repeat the host
tests and regenerate four 1600 × 900 PNG images. The images are suitable for
paste into slides:

1. `01-host-test-results.png`: counts from the actual unit-test run.
2. `02-link-path.png`: raw MAVLink, LoRa ASCII envelope, raw MAVLink.
3. `03-packet-trace.png`: one concrete setpoint frame and its AT+SEND payload.
4. `04-test-console.png`: rendered capture of the exact test runner output.

`host-test-results.txt` contains the complete test output. These results are
software tests on the host computer. The Pico firmware could not be built here
because the Pico SDK and ARM toolchain are unavailable. Real radio range,
packet loss, link timing, and motor response have not been measured.
