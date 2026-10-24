"""Run in Pico RAM with mpremote; query the local modem without transmitting RF."""

from machine import UART, Pin
import utime


def query(uart, command, expected, timeout_ms=1000):
    # Bound the drain even if unrelated radio data keeps arriving.
    for _ in range(4):
        available = uart.any()
        if not available:
            break
        uart.read(min(available, 128))
    wire = command + b"\r\n"
    if uart.write(wire) != len(wire):
        print("UART write incomplete:", command)
        return False
    response = bytearray()
    start = utime.ticks_ms()
    while utime.ticks_diff(utime.ticks_ms(), start) < timeout_ms:
        if uart.any():
            chunk = uart.read(min(uart.any(), 128))
            if chunk:
                response.extend(chunk)
            if len(response) > 1024:
                print("Response too long:", repr(bytes(response[:128])))
                return False
            lines = bytes(response).split(b"\n")[:-1]
            if any(line.strip().startswith(b"+ERR") for line in lines):
                print(command, "->", repr(bytes(response)))
                return False
            if any(line.strip() == expected if expected == b"+OK" else
                   line.strip().startswith(expected) for line in lines):
                print(command, "->", repr(bytes(response)))
                return True
        utime.sleep_ms(10)
    print(command, "-> TIMEOUT", repr(bytes(response)))
    return False


def main():
    uart = UART(0, baudrate=115200, tx=Pin(0), rx=Pin(1),
                bits=8, parity=None, stop=1, timeout=0, timeout_char=0)
    print("Ground Pico UART0: GP0 TX -> modem RX; GP1 RX <- modem TX; 115200 baud")
    if not query(uart, b"AT", b"+OK"):
        print("LOCAL MODEM: FAIL. Check power, common ground, TX/RX and modem UART baud.")
        return
    print("LOCAL MODEM: PASS (real +OK). Wireless link has not been tested.")
    for command, prefix in ((b"AT+VER?", b"+VER="), (b"AT+UID?", b"+UID="),
                            (b"AT+ADDRESS?", b"+ADDRESS="),
                            (b"AT+NETWORKID?", b"+NETWORKID="),
                            (b"AT+BAND?", b"+BAND="),
                            (b"AT+PARAMETER?", b"+PARAMETER=")):
        query(uart, command, prefix)


main()
