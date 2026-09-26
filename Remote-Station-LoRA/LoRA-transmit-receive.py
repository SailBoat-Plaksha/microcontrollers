#!/usr/bin/env python3
"""Direct Pi variant: raw MAVLink on motor serial, Base64 only at RYLR998."""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from protocol.codec import FrameStream, radio_command, radio_receive


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--radio-port", default="/dev/ttyAMA0")
    parser.add_argument("--motor-port", default="/dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--address", type=int, default=0)
    args = parser.parse_args()
    import serial

    with serial.Serial(args.radio_port, args.baud, timeout=0.02) as radio, \
         serial.Serial(args.motor_port, args.baud, timeout=0.02) as motor:
        stream = FrameStream()
        print(f"MAVLink/LoRa: {args.motor_port} <-> {args.radio_port}")
        try:
            while True:
                for frame, message in stream.feed(motor.read(motor.in_waiting or 1)):
                    radio.write(radio_command(frame, args.address))
                    print(f"motor -> radio: {message['name']} ({len(frame)} bytes)")
                line = radio.readline()
                if line.startswith(b"+RCV="):
                    try:
                        frame, message, link = radio_receive(line)
                    except ValueError as error:
                        print(f"discarded radio packet: {error}")
                        continue
                    motor.write(frame)
                    print(f"radio -> motor: {message['name']} RSSI={link['rssi']}")
        except KeyboardInterrupt:
            print("Bridge stopped")


if __name__ == "__main__":
    main()
