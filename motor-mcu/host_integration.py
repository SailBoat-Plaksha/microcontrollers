#!/usr/bin/env python3
"""Exercise raw 64-byte frames against the compiled C adapter executable."""

import subprocess
import sys


def transact(proc, command):
    proc.stdin.write(command + "\n")
    proc.stdin.flush()
    line = proc.stdout.readline().strip()
    if not line:
        raise AssertionError(f"host adapter exited during {command!r}")
    return line


def result(frame):
    if len(frame) != 128:
        return None
    raw = bytes.fromhex(frame)
    msgid = raw[7] | (raw[8] << 8) | (raw[9] << 16)
    if raw[0] != 0xFD or msgid != 200010:
        return None
    request_id = int.from_bytes(raw[14:18], "little")
    return request_id, raw[27]


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: host_integration.py HOST_STDIO_EXECUTABLE")
    proc = subprocess.Popen([sys.argv[1]], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            text=True, bufsize=1)
    try:
        transact(proc, "COUNT 1 0")
        request = transact(proc, "ENCODE 1 1 1 7 1000 1000 5000 1 154 1 1 3")
        assert len(request) == 128
        assert transact(proc, "RX " + request) == "RX 1"
        transact(proc, "POLL")
        first = transact(proc, "STATE").split()
        assert first[0] == "STATE" and int(first[1]) == 2
        assert int(first[3]) == 1 and int(first[4]) == 1
        drives = int(first[9])

        # Exact retransmission is idempotent; an altered envelope under the same ID is rejected.
        assert transact(proc, "RX " + request) == "RX 1"
        duplicate = transact(proc, "STATE").split()
        assert int(duplicate[1]) == 2 and int(duplicate[9]) == drives
        changed = transact(proc, "ENCODE 1 1 1 7 1100 1000 5000 1 154 1 1 3")
        assert transact(proc, "RX " + changed) == "RX 1"

        queued = []
        for _ in range(6):
            parsed = result(transact(proc, "READ"))
            if parsed:
                queued.append(parsed)
        assert (1, 2) in queued and (1, 6) in queued, queued

        # Drained queue falls back to independent accepted and rejected persistent snapshots.
        persistent = []
        for _ in range(8):
            parsed = result(transact(proc, "READ"))
            if parsed:
                persistent.append(parsed)
        assert any(request_id == 1 and state == 3 for request_id, state in persistent), persistent
        assert any(request_id == 1 and state == 6 for request_id, state in persistent), persistent

        stop = transact(proc, "ENCODE 1 1 2 7 0 0 0 1 154 1 3 3")
        assert transact(proc, "RX " + stop) == "RX 1"
        stopped = transact(proc, "STATE").split()
        assert int(stopped[3]) == 0 and int(stopped[4]) == 0 and int(stopped[6]) == 0

        bad = bytearray.fromhex(request)
        bad[14] ^= 1
        assert transact(proc, "RX " + bad.hex()) == "RX 0"
        print("motor adapter raw-mailbox integration: PASS (64-byte MAVLink frames, move, exact/altered duplicate, queued and persistent responses, STOP, CRC rejection)")
    finally:
        try:
            transact(proc, "QUIT")
        except (BrokenPipeError, AssertionError):
            pass
        proc.wait(timeout=5)


if __name__ == "__main__":
    main()
