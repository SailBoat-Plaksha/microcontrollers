"""Regress the stdio bridge through the actual 64-byte mailbox callbacks."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from protocol import codec  # noqa: E402


def raw64(name: str, fields: dict[str, int], *, system: int, component: int) -> str:
    frame = codec.encode(name, fields, system=system, component=component)
    return (frame + bytes(64 - len(frame))).hex()


def decode64(raw: str) -> dict:
    frame = bytes.fromhex(raw)
    return codec.decode(frame[:12 + frame[1]])


def main() -> None:
    binary = Path(sys.argv[1])
    request = raw64("IMU_REQUEST", {
        "ground_session": 77, "request_id": 5, "timeout_ms": 400,
        "max_age_ms": 0, "path": 1, "target_system": 1,
        "target_component": 159,
    }, system=1, component=1)
    sample = raw64("IMU_SAMPLE", {
        "ground_session": 77, "request_id": 5, "time_boot_ms": 1234,
        "sample_seq": 8, "acquisition_boot_id": 991, "source_boot_id": 991,
        "sample_age_ms": 0, "valid_fields": 7,
        "xacc": 10, "yacc": -20, "zacc": 1000,
        "xgyro": 4, "ygyro": 5, "zgyro": 6,
        "xmag": -7, "ymag": 8, "zmag": 9, "status": 1,
        "path": 0, "acquisition_component": 1,
        "target_system": 1, "target_component": 159,
    }, system=1, component=1)
    run = subprocess.run([str(binary)], input=f"I {request}\nO\nI {sample}\nO\n",
                         text=True, capture_output=True, check=True)
    lines = run.stdout.splitlines()
    assert lines[0].startswith("READY "), lines
    assert lines[1] == "OK", lines
    dependency = decode64(lines[2])
    assert dependency["name"] == "IMU_REQUEST", dependency
    assert dependency["system"] == 1 and dependency["component"] == 159
    assert dependency["fields"]["ground_session"] == 77
    assert dependency["fields"]["request_id"] == 5
    assert dependency["fields"]["path"] == 0
    assert lines[3] == "OK", lines
    result = decode64(lines[4])
    assert result["name"] == "IMU_SAMPLE", result
    assert result["system"] == 1 and result["component"] == 159
    assert result["fields"]["path"] == 1
    assert result["fields"]["target_system"] == 255
    assert result["fields"]["target_component"] == 190
    assert result["fields"]["xacc"] == 10

    malformed = subprocess.run([str(binary)], input="I 00\nT 12x\nO extra\n",
                                text=True, capture_output=True, check=True)
    assert malformed.stdout.splitlines()[1:] == ["ERR hex", "ERR time", "ERR command"]
    print("sensor stdio harness callback regression: PASS")


if __name__ == "__main__":
    main()
