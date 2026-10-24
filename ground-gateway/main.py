"""USB CDC <-> RYLR998 UART0 AT text bridge for the ground Pico."""

import sys
try:
    import select
except ImportError:
    import uselect as select
from machine import UART, Pin

try:
    import micropython
    micropython.kbd_intr(-1)
except (ImportError, AttributeError):
    pass

import config
from bridge import ATBridge


def run():
    uart_id = getattr(config, "BRIDGE_UART_ID", 0)
    baud = getattr(config, "BRIDGE_UART_BAUD", 115200)
    tx_pin = getattr(config, "BRIDGE_UART_TX_PIN", 0)
    rx_pin = getattr(config, "BRIDGE_UART_RX_PIN", 1)

    uart = UART(uart_id, baud,
                tx=Pin(tx_pin), rx=Pin(rx_pin),
                bits=8, parity=None, stop=1, txbuf=512, rxbuf=512,
                timeout=0, timeout_char=0)
    usb_in = getattr(sys.stdin, "buffer", sys.stdin)
    usb_out = getattr(sys.stdout, "buffer", sys.stdout)

    poller = select.poll()
    poller.register(usb_in, select.POLLIN)

    bridge = ATBridge(uart, usb_out)

    while True:
        for _, flags in poller.poll(0):
            if flags & select.POLLIN:
                data = usb_in.read(1)
                if data:
                    bridge.feed_usb(data)
        available = uart.any()
        if available:
            data = uart.read(min(available, 128))
            if data:
                bridge.feed_modem(data)
        bridge.tick()


if __name__ == "__main__":
    run()
