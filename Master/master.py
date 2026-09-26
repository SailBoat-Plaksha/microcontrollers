#!/usr/bin/env python3
"""Route complete MAVLink 2 frames between the LoRa and actuator USB links."""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from protocol.codec import FrameStream


def route(data, parser, output):
    forwarded = []
    for frame, message in parser.feed(data):
        # Only the actuator MCU should receive setpoints. All other recognized
        # telemetry can be observed on the opposite link without translation.
        if message["name"] == "ACTUATOR_SETPOINT":
            fields = message["fields"]
            if fields["target_system"] not in (0, 1) or fields["target_component"] not in (0, 154):
                continue
        output.write(frame)
        forwarded.append(message)
    return forwarded


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lora-port", required=True)
    parser.add_argument("--servo-port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()
    import serial

    with serial.Serial(args.lora_port, args.baud, timeout=0.02) as lora, \
         serial.Serial(args.servo_port, args.baud, timeout=0.02) as servo:
        radio_stream, servo_stream = FrameStream(), FrameStream()
        print(f"MAVLink router: {args.lora_port} <-> {args.servo_port}")
        try:
            while True:
                for message in route(lora.read(lora.in_waiting or 1), radio_stream, servo):
                    print(f"radio -> servo: {message['name']} seq={message['sequence']}")
                for message in route(servo.read(servo.in_waiting or 1), servo_stream, lora):
                    print(f"servo -> radio: {message['name']} seq={message['sequence']}")
        except KeyboardInterrupt:
            print("Router stopped")


if __name__ == "__main__":
    main()
