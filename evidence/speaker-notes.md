# MAVLink migration — speaker notes

## Slide 1 — One packet format, end to end

We had JSON and text packet paths that did not agree across the Pi, radio, and servo board. This branch gives those active paths one MAVLink 2 wire format. The only translation is Base64 around the RYLR998 modem, because that modem accepts ASCII AT commands. This is a committed local branch, not a hardware test claim.

## Slide 2 — The wire path

Walk left to right. On binary-safe USB and UART links we send the MAVLink frame directly. At the RYLR998 interface, a complete frame is Base64 encoded into one AT+SEND packet, then decoded at the receiving gateway. The example setpoint is 26 binary bytes and 36 ASCII characters. The servo accepts a command only after frame and command checks; the old cJSON paths are no longer active.

## Slide 3 — Packet behavior is repeatable

The tests are repeatable on a host computer. They cover serial chunking and reassembly, Base64 radio wrapping, corrupted packet rejection, the MicroPython gateway validator, Pi routing, and C/Python frame agreement. The 100 out of 100 figures are deterministic test cases, not measured radio delivery rates. For a terminal screenshot, run the command shown on this slide from the repository root.

## Slide 4 — Ready for a board-level trial

The branch is ready for a board-level trial, with the software packet path and evidence committed together. The host test proves bytes and validation behavior, but this machine does not have the Pico SDK or ARM toolchain, so firmware build and hardware measurements are still outstanding. The parser deliberately supports the eight boat messages plus HEARTBEAT and unsigned MAVLink 2 frames. The next useful evidence is a flashed-board run measuring actual radio delivery and motor response.
