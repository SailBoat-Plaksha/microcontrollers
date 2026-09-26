"""MicroPython MAVLink 2 <-> RYLR998 gateway.

Pi UART0 GP0/GP1 carries raw binary frames. Radio UART1 GP4/GP5 carries
RYLR998 AT lines. USB remains a debug console, never a packet transport.
"""

from machine import UART, Pin
from ubinascii import b2a_base64, a2b_base64
from utime import sleep_ms

EXTRAS = (139, 49, 131, 143, 158, 201, 198, 121)  # arsem.xml IDs 200000..200007
RADIO_LIMIT = 240


def crc(data):
    value = 0xffff
    for byte in data:
        tmp = byte ^ (value & 255)
        tmp ^= (tmp << 4) & 255
        value = ((value >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 65535
    return value


def valid(frame):
    if len(frame) < 13 or frame[0] != 0xfd or frame[2] != 0 or len(frame) != frame[1] + 12:
        return False
    msgid = frame[7] | (frame[8] << 8) | (frame[9] << 16)
    if 200000 <= msgid <= 200007:
        extra = EXTRAS[msgid - 200000]
    elif msgid == 0:
        extra = 50
    else:
        return False
    check = crc(frame[1:-2] + bytes((extra,)))
    return frame[-2] == (check & 255) and frame[-1] == (check >> 8)


def radio_line_to_frame(line):
    if not line.startswith(b"+RCV="):
        return None
    try:
        fields = line[5:].strip().split(b",")
        if len(fields) != 5 or int(fields[1]) != len(fields[2]) or len(fields[2]) > RADIO_LIMIT:
            return None
        frame = a2b_base64(fields[2])
        return frame if valid(frame) else None
    except (ValueError, TypeError):
        return None


def main():
    computer = UART(0, baudrate=115200, tx=Pin(0), rx=Pin(1))
    radio = UART(1, baudrate=115200, tx=Pin(4), rx=Pin(5))
    computer_frame = bytearray()
    radio_line = bytearray()
    while True:
        if computer.any():
            for byte in computer.read():
                if not computer_frame and byte != 0xfd:
                    continue
                computer_frame.append(byte)
                if len(computer_frame) >= 10:
                    expected = computer_frame[1] + 12
                    if expected > 180 or computer_frame[2] != 0:
                        computer_frame = bytearray()
                    elif len(computer_frame) == expected:
                        if valid(computer_frame):
                            encoded = b2a_base64(computer_frame).strip()
                            if len(encoded) <= RADIO_LIMIT:
                                radio.write(b"AT+SEND=0,%d," % len(encoded) + encoded + b"\r\n")
                        computer_frame = bytearray()
        if radio.any():
            for byte in radio.read():
                if byte in (10, 13):
                    if radio_line:
                        frame = radio_line_to_frame(radio_line)
                        if frame is not None:
                            computer.write(frame)
                        radio_line = bytearray()
                elif len(radio_line) < 300:
                    radio_line.append(byte)
                else:
                    radio_line = bytearray()
        sleep_ms(1)


if __name__ == "__main__":
    main()
