#!/usr/bin/env python3
"""
Simple master bridge with a strict protocol.

Accepted input format (one JSON object per line from LoRa Pico):
{
    "event": "received",
    "payload": {"cmd": "servo_angle", "angle": 90}
}

Notes:
- `payload` may also be a JSON string: "{\"cmd\":\"servo_angle\",\"angle\":90}"
- Only `cmd == "servo_angle"` is accepted.
- Outgoing Servo packet format is always: {"angle":90}
"""

import argparse
import json
import sys
import time

try:
    import serial
except ImportError:
    print("pyserial is required. Install with: pip install pyserial", file=sys.stderr)
    sys.exit(1)


def extract_command_object(packet):
    """
    Strict protocol parser.

    Required input shape:
    {
      "event": "received",
      "payload": {"cmd": "servo_angle", "angle": <int>}
    }
    """
    if not isinstance(packet, dict):
        return None, "top-level packet must be a JSON object"

    if packet.get("event") != "received":
        return None, "event must be 'received'"

    if "payload" not in packet:
        return None, "missing payload field"

    payload = packet["payload"]

    if isinstance(payload, str):
        payload = payload.strip()
        if not payload:
            return None, "payload string is empty"
        try:
            payload = json.loads(payload)
        except json.JSONDecodeError:
            return None, "payload string is not valid JSON"

    if not isinstance(payload, dict):
        return None, "payload must be a JSON object (or JSON-encoded object string)"

    return payload, None


def normalize_servo_angle(command_obj):
    if not isinstance(command_obj, dict):
        return None, "payload command must be an object"

    cmd = command_obj.get("cmd")
    if cmd != "servo_angle":
        return None, "only cmd='servo_angle' is accepted"

    if "angle" not in command_obj:
        return None, "missing angle field"

    try:
        angle = int(command_obj["angle"])
    except (TypeError, ValueError):
        return None, "angle must be an integer"

    if angle < 0:
        angle = 0
    if angle > 180:
        angle = 180

    return {"angle": angle}, None


def main():
    parser = argparse.ArgumentParser(description="Bridge LoRa Pico JSON commands to Servo Pico")
    parser.add_argument("--lora-port", required=True, help="Serial port connected to LoRa Pico (example: /dev/ttyACM0)")
    parser.add_argument("--servo-port", required=True, help="Serial port connected to Servo Pico (example: /dev/ttyACM1)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate for both serial ports")
    args = parser.parse_args()

    lora = serial.Serial(args.lora_port, args.baud, timeout=0.2)
    servo = serial.Serial(args.servo_port, args.baud, timeout=0.2)

    print(f"[master] listening on {args.lora_port}, forwarding to {args.servo_port}")

    try:
        while True:
            raw = lora.readline()
            if not raw:
                continue

            line = raw.decode("utf-8", errors="ignore").strip()
            if not line:
                continue

            try:
                top_packet = json.loads(line)
            except json.JSONDecodeError:
                continue

            cmd_obj, err = extract_command_object(top_packet)
            if err:
                print(f"[ignored] {err}")
                continue

            servo_packet, err = normalize_servo_angle(cmd_obj)
            if err:
                print(f"[ignored] {err}")
                continue

            out = json.dumps(servo_packet, separators=(",", ":")) + "\n"
            servo.write(out.encode("utf-8"))
            servo.flush()
            print(f"[forwarded] {out.strip()}")

    except KeyboardInterrupt:
        print("\n[master] stopping")
    finally:
        time.sleep(0.05)
        lora.close()
        servo.close()


if __name__ == "__main__":
    main()
