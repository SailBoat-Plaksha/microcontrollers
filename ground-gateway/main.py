"""USB raw MAVLink 2 <-> RYLR998 gateway for the ground Pico."""

import sys
import uselect
from machine import UART, Pin

import config
from gateway import Gateway


def run():
    uart = UART(config.UART_ID, config.UART_BAUD,
                tx=Pin(config.UART_TX_PIN), rx=Pin(config.UART_RX_PIN),
                bits=8, parity=None, stop=1, txbuf=512, rxbuf=512,
                timeout=0, timeout_char=0)
    usb_in = getattr(sys.stdin, "buffer", sys.stdin)
    usb_out = getattr(sys.stdout, "buffer", sys.stdout)
    poller = uselect.poll()
    poller.register(usb_in, uselect.POLLIN)
    gateway = Gateway(uart, usb_out)
    while True:
        for _, flags in poller.poll(0):
            if flags & uselect.POLLIN:
                # One-byte reads avoid waiting for a full buffer when the host
                # intentionally fragments a frame across serial writes.
                data = usb_in.read(1)
                if data:
                    gateway.feed_usb(data)
        while uart.any():
            data = uart.read(min(uart.any(), 128))
            if not data:
                break
            gateway.feed_modem(data)
        gateway.tick()


run()
