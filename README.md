# ARSEM packet links

The active packet paths use MAVLink 2. The Pi router and servo MCU exchange raw
frames over USB serial. A LoRa gateway converts each complete frame to Base64
only for the RYLR998 ASCII `AT+SEND` interface, then restores the original
binary frame at the other end. `protocol/arsem.xml` defines the boat messages.

Two LoRa gateway variants are present: `Remote-Station-LoRA/main.c` for a Pico,
and `Remote-Station-LoRA/LoRA-transmit-receive.py` for a Pi connected directly
to the modem. `Base-Station-LoRA/main.py` is a MicroPython gateway with raw
MAVLink on UART0 (GP0/GP1) and the LoRa modem on UART1 (GP4/GP5). Use one
gateway variant on a given link.

Run host packet-link tests with `python -m unittest discover -s tests -v`.
They test frame identity, fragmentation, CRC rejection, LoRa encoding,
Python/C compatibility, and actuator command checks without radio hardware.
See `evidence/README.md` for presentation images and the remaining hardware
checks.

For current boat midsem setup, software checks, and separately labeled Pi and
firmware steps, see [`docs/runbook.md`](docs/runbook.md).
