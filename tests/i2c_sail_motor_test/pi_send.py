#!/usr/bin/env python3
"""Pi Zero raw I2C MAVLink bench-test sender; never retries motion writes."""
import argparse
import sys
import time

import arsem_test as dialect

ADDRESS = 0x08
READ_SIZE = 32
STATES = {0: "IDLE", 1: "RUNNING", 2: "COMPLETED", 3: "STOPPED", 4: "OVERFLOW_STOP"}
RESULTS = {0: "OK", 1: "INVALID", 2: "BUSY", 3: "STALE_ID", 4: "BAD_FRAME",
           5: "RX_OVERFLOW", 6: "WRONG_SOURCE_OR_TARGET"}


def command_frame(command_id, direction, duration_ms=0, pwm=0):
    mav = dialect.MAVLink(None, srcSystem=1, srcComponent=1)
    frame = mav.sail_motor_test_encode(command_id, duration_ms, 1, 154,
                                       direction, pwm).pack(mav)
    if frame[0] != 0xFD or len(frame) > 32:
        raise ValueError("command must be one unsigned MAVLink 2 frame <=32 bytes")
    return frame


def decode_status(raw):
    if len(raw) != READ_SIZE or raw[0] != 0xFD or raw[2] != 0:
        raise ValueError("invalid status transfer")
    length = 12 + raw[1]
    if length > READ_SIZE or any(raw[length:]):
        raise ValueError("invalid status length/padding")
    parser = dialect.MAVLink(None)
    message = parser.parse_char(bytes(raw[:length]))
    if message is None or message.get_type() != "SAIL_MOTOR_TEST_STATUS":
        raise ValueError("expected SAIL_MOTOR_TEST_STATUS")
    if (message.get_srcSystem(), message.get_srcComponent()) != (1, 154):
        raise ValueError("unexpected MCU identity")
    return message


def read_status(bus, i2c_msg, address):
    transfer = i2c_msg.read(address, READ_SIZE)
    bus.i2c_rdwr(transfer)  # Separate read transaction, no register prefix.
    return decode_status(bytes(transfer))


def show(status):
    print(f"id={status.command_id} state={STATES.get(status.state, status.state)} "
          f"dir={status.direction} pwm={status.pwm} elapsed={status.elapsed_ms}ms "
          f"remaining={status.remaining_ms}ms "
          f"result={RESULTS.get(status.result, status.result)}", flush=True)


def run_command(bus, i2c_msg, address, direction, duration_ms, pwm):
    before = read_status(bus, i2c_msg, address)
    show(before)
    if direction and before.state == 1:
        raise RuntimeError("MCU is busy; wait for completion or issue stop")
    command_id = before.command_id + 1
    if command_id > 0xFFFFFFFF:
        raise RuntimeError("command ID exhausted; reset MCU while motor is off")
    frame = command_frame(command_id, direction, duration_ms, pwm)
    print(f"TX {len(frame)} raw MAVLink bytes: {frame.hex()}", flush=True)
    bus.i2c_rdwr(i2c_msg.write(address, frame))  # Exactly one write. No retry.
    deadline = time.monotonic() + duration_ms / 1000 + 2
    observed_running = False
    while time.monotonic() < deadline:
        time.sleep(0.05)
        status = read_status(bus, i2c_msg, address)
        show(status)
        if status.command_id != command_id:
            if status.result:
                raise RuntimeError(f"MCU rejected command: {RESULTS.get(status.result)}")
            continue  # May read the preceding cached snapshot immediately after write.
        if status.result != 0:
            raise RuntimeError(f"MCU reported {RESULTS.get(status.result)}")
        if status.state == 1:
            observed_running = True
            if status.direction != direction or status.pwm != pwm:
                raise RuntimeError("MCU running outputs differ from the command")
        if status.state == (2 if direction else 3):
            if status.direction != 0 or status.pwm != 0:
                raise RuntimeError("MCU did not report outputs disabled")
            if direction and not duration_ms <= status.elapsed_ms <= duration_ms + 100:
                raise RuntimeError("MCU timing exceeded 100 ms host acceptance tolerance")
            print("PASS: matching command completed with outputs off. "
                  f"RUNNING observed={observed_running}. Verify shaft motion separately.")
            return status
    raise TimeoutError("no matching completion status; MCU's local timer still bounds motion")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("forward", "backward", "stop", "status", "demo"))
    parser.add_argument("--duration-ms", type=int, default=500)
    parser.add_argument("--pwm", type=int, default=96)
    parser.add_argument("--bus", type=int, default=1)
    parser.add_argument("--address", type=lambda text: int(text, 0), default=ADDRESS)
    parser.add_argument("--dry-run", action="store_true", help="print frames without I2C access")
    args = parser.parse_args()
    if not 1 <= args.duration_ms <= 5000 or not 1 <= args.pwm <= 128:
        parser.error("duration must be 1..5000 ms; PWM must be 1..128")
    if not 0x08 <= args.address <= 0x77:
        parser.error("address must be a non-reserved 7-bit I2C address")
    directions = {"forward": [1], "backward": [-1], "stop": [0], "demo": [1, -1]}
    if args.dry_run:
        for number, direction in enumerate(directions.get(args.action, []), 1):
            frame = command_frame(number, direction, args.duration_ms if direction else 0,
                                  args.pwm if direction else 0)
            print(f"{len(frame)} bytes: {frame.hex()}")
        return
    from smbus2 import SMBus, i2c_msg
    with SMBus(args.bus) as bus:
        if args.action == "status":
            show(read_status(bus, i2c_msg, args.address))
            return
        for index, direction in enumerate(directions[args.action]):
            if index: time.sleep(1)  # Motor is already off; pause before reverse.
            run_command(bus, i2c_msg, args.address, direction,
                        args.duration_ms if direction else 0, args.pwm if direction else 0)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, TimeoutError, dialect.MAVError) as error:
        print(f"FAIL: {error}. No motion write was retried. "
              "If needed, run stop or disconnect motor power.", file=sys.stderr)
        sys.exit(1)
