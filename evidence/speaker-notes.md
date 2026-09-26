# MAVLink migration — speaker notes

## Slide 1 — One frame. Every link.

Previously the Pi, LoRa bridge, and servo code used incompatible JSON or text packet paths. This branch replaces the active paths with MAVLink 2 frames. The strip shows the frame boundary: FD start byte, header, payload, and checksum. The XML dialect is the shared contract. The LoRa modem still needs ASCII AT commands, which are handled at its boundary only.

## Slide 2 — LoRa boundary.

The gateways place one complete MAVLink frame inside the RYLR998 ASCII AT+SEND payload, then restore the same binary bytes on the other side. The Pi router and servo USB link carry raw frames without JSON or Base64. The example setpoint is 26 bytes as a frame and 36 Base64 characters on the radio interface. The arrows show encoding and decoding, not a measured RF trace.

## Slide 3 — Host tests.

All eight host unit tests pass. The serial test feeds 100 frames in three-byte chunks and checks that they are recovered in order. The LoRa test round trips 100 intact Base64 envelopes and deliberately corrupts 100 frames; all damaged frames fail CRC validation. The C and Python implementations emit identical bytes. These are deterministic software tests, not radio delivery or range measurements. Run `python -m unittest discover -s tests -v` from the repository root for a terminal screenshot.

## Slide 4 — Next: hardware.

The host evidence establishes frame encoding, routing, reassembly, CRC rejection, and command validation. The firmware has not been built or run on boards in this checkout because the Pico SDK and ARM toolchain were unavailable in the initial environment. Next we need to build and flash both Picos, measure packet loss and latency over actual radio hardware, and verify motor motion and RC override. The current minimal parser supports the eight boat-specific messages plus HEARTBEAT and accepts unsigned MAVLink 2 frames.
