"""Run deterministic boat contract checks and report unimplemented integration paths."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable
from unittest.mock import patch

from boat.client import BoatClient, ImuSample, PongReply, RequestOutcome, RequestTicket
from boat.ids import COMPONENT_GROUND, COMPONENT_MOTOR, SYSTEM_GROUND
from protocol import codec
from scenarios.fixtures import (CapturingTransport, FragmentedStream,
                                MailboxFixture, ManualClock, ModemFixture)

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "scenarios" / "results"


@dataclass
class Check:
    name: str
    status: str
    detail: str


@dataclass
class Scenario:
    number: int
    title: str
    pending_reason: str
    checks: list[Check] = field(default_factory=list)
    trace: list[dict] = field(default_factory=list)
    status: str = "PENDING"


SCENARIO_SPECS = [
    (1, "LoRa and SSH routing with correlated motor status", "Pi router, LoRa gateways, mailbox and motor firmware harnesses are not present in this checkout."),
    (2, "Pi-owned IMU sampling, units and freshness", "The Pi IMU acquisition and telemetry implementations are not present in this checkout."),
    (3, "Ping-all responses without loops", "Node responders and router forwarding behavior are not present in this checkout."),
    (4, "Invalid target, source, authority, message and CRC", "This checkout can test client decoding only; motor non-energization requires the Pi and motor harnesses."),
    (5, "Absolute, STEP deduplication, sessions and concurrency", "Production validation, motor state, deduplication and concurrent-client handling are not present."),
    (6, "STOP priority and bounded motor deadlines", "Queue scheduling and onboard motor deadline handling are not present."),
    (7, "Loss, ambiguity, truncation and transport recovery", "No production router or firmware harness is available to verify end-to-end uncertainty and recovery."),
    (8, "Node reboot and reconnect invalidation", "Production node boot/session reconciliation is not present; only client reconnect behavior is checked."),
    (9, "Missing peers, stale IMU and queue overflow", "No production bus poller, IMU reader or bounded queues are present."),
    (10, "RC takeover and autonomous ownership", "The motor adapter host harness is not present; protected RC code is intentionally outside this lane."),
    (11, "Saturated telemetry fairness", "Production queues, status scheduling and health polling are not present."),
]


def _request(client: BoatClient, transport: CapturingTransport) -> tuple[RequestTicket, dict]:
    ticket = client.step(2, -500, timeout_ms=750, max_rate_cdeg_s=1200)
    return ticket, codec.decode(transport.sent[-1])


def _frame(name: str, fields: dict[str, int], *, system: int, component: int, sequence: int = 0) -> bytes:
    return codec.encode(name, fields, system=system, component=component, sequence=sequence)


def _request_result(ticket: RequestTicket, *, target_system: int = SYSTEM_GROUND,
                    target_component: int = COMPONENT_GROUND, state: int = 2) -> bytes:
    return _frame("REQUEST_RESULT", {
        "ground_session": ticket.ground_session, "request_id": ticket.request_id,
        "motor_boot_id": 44, "local_command_id": 9, "result": 1,
        "state": state, "target_system": target_system,
        "target_component": target_component,
    }, system=1, component=1)


def _check_client_request_fields() -> tuple[str, list[dict]]:
    clock, transport = ManualClock(10), CapturingTransport("ssh-fixture")
    client = BoatClient(transport, clock=clock)
    with patch("boat.client.secrets.randbits", return_value=0x11111111):
        client.connect()
    ticket, message = _request(client, transport)
    fields = message["fields"]
    assert message["name"] == "ACTUATOR_REQUEST"
    assert message["system"] == SYSTEM_GROUND and message["component"] == COMPONENT_GROUND
    assert fields == {
        "request_id": ticket.request_id, "ground_session": ticket.ground_session,
        "delta_cdeg": -500, "timeout_ms": 750, "max_rate_cdeg_s": 1200,
        "target_system": 1, "target_component": COMPONENT_MOTOR,
        "actuator": 2, "opcode": 2, "control_source": 3,
    }
    trace = [{"event": "client_out", "transport": "ssh-fixture", "hex": transport.sent[0].hex()}]
    return "BoatClient emitted the frozen STEP request identity and correlation fields.", trace


def _check_client_ping_and_imu() -> tuple[str, list[dict]]:
    clock, transport = ManualClock(4.25), CapturingTransport("fixture")
    client, events = BoatClient(transport, clock=clock), []
    client.subscribe(events.append)
    with patch("boat.client.secrets.randbits", return_value=0x22222222):
        client.connect()
    client.ping_all()
    ping = codec.decode(transport.sent[-1])
    assert ping["name"] == "PING"
    assert ping["fields"]["target_system"] == ping["fields"]["target_component"] == 0
    clock.advance(0.125)
    transport.inject(_frame("PING", {"time_usec": ping["fields"]["time_usec"],
                                    "seq": ping["fields"]["seq"],
                                    "target_system": SYSTEM_GROUND,
                                    "target_component": COMPONENT_GROUND},
                           system=1, component=154))
    pong = [event for event in events if isinstance(event, PongReply)]
    assert len(pong) == 1 and pong[0].rtt_ms == 125.0
    assert (pong[0].system, pong[0].component) == (1, 154)
    health = _frame("NODE_HEALTH", {"boot_id": 71, "time_boot_ms": 4000,
        "fault_flags": 0, "sensor_valid_fields": 3, "queue_drops": 0,
        "reset_reason": 0, "feedback_valid": 0}, system=1, component=1)
    imu = _frame("SCALED_IMU", {"time_boot_ms": 4050, "xacc": 10, "yacc": -20,
        "zacc": 1000, "xgyro": 4, "ygyro": 5, "zgyro": 6,
        "xmag": 0, "ymag": 0, "zmag": 0, "temperature": 0}, system=1, component=1)
    transport.inject(health)
    transport.inject(imu)
    samples = [event for event in events if isinstance(event, ImuSample)]
    assert len(samples) == 1
    assert samples[0].accel_mg == (10, -20, 1000)
    assert samples[0].gyro_mrad_s == (4, 5, 6)
    assert samples[0].valid_fields == 3 and samples[0].source == (1, 1)
    return "BoatClient matched ping identity/RTT and preserved IMU units, source and validity from the reported health mask.", [
        {"event": "client_out", "name": "PING", "hex": transport.sent[0].hex()},
        {"event": "client_in", "name": "PING", "hex": transport.sent[0].hex(), "rtt_ms": 125.0},
        {"event": "client_in", "name": "SCALED_IMU", "hex": imu.hex(), "source": [1, 1], "valid_fields": 3},
    ]


def _check_invalid_and_unmatched_results() -> tuple[str, list[dict]]:
    transport, events = CapturingTransport(), []
    client = BoatClient(transport)
    client.subscribe(events.append)
    with patch("boat.client.secrets.randbits", return_value=0x33333333):
        client.connect()
    ticket = client.set_absolute_angle(1, 900)
    good = transport.sent[-1]
    damaged = good[:-1] + bytes((good[-1] ^ 0x80,))
    signed = bytearray(good)
    signed[2] |= 1
    message_id = int.from_bytes(signed[7:10], "little")
    extra = codec.MESSAGES[message_id]["crc_extra"]
    checksum = codec.crc(signed[1:10 + signed[1]] + bytes((extra,)))
    signed[10 + signed[1]:12 + signed[1]] = checksum.to_bytes(2, "little")
    signed.extend(bytes(range(13)))
    transport.inject(damaged)
    transport.inject(bytes(signed))
    transport.inject(_request_result(ticket, target_system=1, target_component=154))
    assert not any(isinstance(event, RequestOutcome) for event in events)
    transport.inject(_request_result(ticket))
    outcomes = [event for event in events if isinstance(event, RequestOutcome)]
    assert len(outcomes) == 1
    assert outcomes[0].request_id == ticket.request_id
    return "Damaged and signed frames were rejected by protocol.codec; BoatClient ignored a response addressed to another target and accepted the matching correlation.", [
        {"event": "client_out", "name": "ACTUATOR_REQUEST", "hex": good.hex()},
        {"event": "client_in_rejected", "reason": "damaged_crc", "hex": damaged.hex()},
        {"event": "client_in_rejected", "reason": "signed", "hex": bytes(signed).hex()},
        {"event": "client_in_ignored", "reason": "wrong_response_target"},
        {"event": "client_in", "name": "REQUEST_RESULT", "request_id": ticket.request_id},
    ]


def _check_reconnect_and_ambiguous_write() -> tuple[str, list[dict]]:
    clock, transport = ManualClock(), CapturingTransport()
    client = BoatClient(transport, clock=clock)
    with patch("boat.client.secrets.randbits", side_effect=(0x44444444, 0x55555555)):
        client.connect()
        first_session = client.ground_session
        transport.fail_next_write = True
        try:
            client.step(1, 500)
        except TimeoutError:
            pass
        else:
            raise AssertionError("injected ambiguous write did not surface")
        attempts_before_reconnect = len(transport.attempts)
        client.disconnect()
        client.connect()
        second_session = client.ground_session
    assert first_session != second_session
    assert len(transport.attempts) == attempts_before_reconnect
    assert len(transport.sent) == 0
    return "A failed ambiguous write surfaced to the caller; reconnect changed the client session and caused no movement replay.", [
        {"event": "write_ambiguous", "attempts": attempts_before_reconnect},
        {"event": "client_reconnect", "old_session": first_session, "new_session": second_session},
        {"event": "replay_check", "movement_frames_after_reconnect": len(transport.sent)},
    ]


def _check_fixture_transports() -> tuple[str, list[dict]]:
    frame = _frame("HEARTBEAT", {"type": 6, "autopilot": 8, "base_mode": 0,
        "custom_mode": 0, "system_status": 4, "mavlink_version": 3}, system=1, component=1)
    parser = FragmentedStream()
    assert parser.feed(b"noise" + frame[:3]) == []
    assert parser.feed(frame[3:9]) == []
    assert parser.feed(frame[9:]) == [frame]
    padded = MailboxFixture.write(frame)
    assert len(padded) == MailboxFixture.FRAME_BYTES
    read = MailboxFixture.read(padded)
    assert read.frame == frame
    assert MailboxFixture.read(bytes(64)).frame is None
    modem_line = ModemFixture.transmit(frame, address=7).replace(b"AT+SEND=7,", b"+RCV=7,")
    # The receive modem envelope carries RSSI and SNR after its encoded payload.
    payload = modem_line.split(b",", 2)[2].split(b"\r", 1)[0]
    received, decoded, radio_meta = ModemFixture.receive(b"+RCV=7," + str(len(payload)).encode() + b"," + payload + b",-80,9.5")
    assert received == frame and decoded["name"] == "HEARTBEAT"
    assert radio_meta == {"address": 7, "rssi": -80, "snr": 9.5}
    for invalid in (frame + frame, frame + b"\0", bytes([frame[0], frame[1], frame[2] | 1]) + frame[3:] + bytes(13)):
        try:
            MailboxFixture.write(invalid)
        except ValueError:
            continue
        raise AssertionError("mailbox fixture accepted malformed, concatenated or signed input")
    return "FrameStream recovered USB fragmentation; test fixtures enforced one unsigned zero-padded mailbox frame and exercised the production modem envelope codec.", [
        {"event": "stream_fragmented", "frame_hex": frame.hex()},
        {"event": "mailbox_write_read", "frame_bytes": len(frame), "transfer_bytes": 64},
        {"event": "modem_roundtrip", "address": 7, "rssi": -80, "snr": 9.5},
    ]


def _check_generated_c_header() -> tuple[str, list[dict]]:
    compiler = shutil.which(os.environ.get("CC", "cc"))
    if compiler is None:
        raise RuntimeError("C compiler not found; generated-header cross-check cannot run")
    include_dir = ROOT / "protocol" / "gen" / "C" / "arsem_standalone"
    with tempfile.TemporaryDirectory(prefix="boat-scenarios-") as tmp:
        executable = Path(tmp) / "c_codec_probe"
        subprocess.run([compiler, "-std=c99", "-I", str(include_dir),
                        str(ROOT / "scenarios" / "c_codec_probe.c"), "-o", str(executable)],
                       check=True, capture_output=True, text=True)
        raw_hex = subprocess.run([str(executable)], check=True, capture_output=True,
                                 text=True).stdout.strip()
    raw = bytes.fromhex(raw_hex)
    msg = codec.decode(raw)
    assert msg["name"] == "ACTUATOR_REQUEST"
    assert (msg["system"], msg["component"]) == (255, 190)
    assert msg["fields"] == {
        "request_id": 0x10203040, "ground_session": 0x50607080,
        "delta_cdeg": -500, "timeout_ms": 750, "max_rate_cdeg_s": 1200,
        "target_system": 1, "target_component": 154, "actuator": 2,
        "opcode": 2, "control_source": 3,
    }
    return "The generated C header serialized ACTUATOR_REQUEST bytes accepted by protocol.codec with independently specified expected fields.", [
        {"event": "generated_c_frame", "hex": raw.hex(), "message": msg["name"]},
    ]


def _execute(name: str, fn: Callable[[], tuple[str, list[dict]]]) -> tuple[Check, list[dict]]:
    try:
        detail, trace = fn()
        return Check(name, "PASS", detail), trace
    except Exception as exc:
        return Check(name, "FAIL", f"{type(exc).__name__}: {exc}"), []


def build_report() -> tuple[list[Scenario], list[dict]]:
    scenarios = [Scenario(number, title, reason) for number, title, reason in SCENARIO_SPECS]
    by_number = {scenario.number: scenario for scenario in scenarios}
    assignments = [
        (1, "BoatClient request encoding", _check_client_request_fields),
        (2, "Client ping and IMU decoding", _check_client_ping_and_imu),
        (3, "Client addressed ping matching", _check_client_ping_and_imu),
        (4, "Client invalid-frame and target filtering", _check_invalid_and_unmatched_results),
        (5, "Client movement request fields", _check_client_request_fields),
        (7, "Codec and transport fixture faults", _check_fixture_transports),
        (7, "Ambiguous write does not replay on reconnect", _check_reconnect_and_ambiguous_write),
        (8, "Client reconnect does not replay", _check_reconnect_and_ambiguous_write),
        (1, "Generated C header to Python codec", _check_generated_c_header),
    ]
    trace: list[dict] = []
    for number, name, fn in assignments:
        check, emitted = _execute(name, fn)
        by_number[number].checks.append(check)
        if check.status == "PASS":
            by_number[number].trace.extend(emitted)
            trace.extend({"scenario": number, **event} for event in emitted)
    for scenario in scenarios:
        scenario.status = "FAIL" if any(check.status == "FAIL" for check in scenario.checks) else "PENDING"
        pending = {"event": "integration_pending", "reason": scenario.pending_reason}
        scenario.trace.insert(0, pending)
        trace.append({"scenario": scenario.number, **pending})
    return scenarios, trace


def _document(scenarios: list[Scenario], trace: list[dict]) -> dict:
    return {
        "suite": "boat-midsem-scenarios-1-11",
        "overall": "FAIL" if any(s.status == "FAIL" for s in scenarios) else "PENDING",
        "exit_policy": "nonzero when any scenario is FAIL or PENDING",
        "scenarios": [{"id": s.number, "title": s.title, "status": s.status,
                       "pending_reason": s.pending_reason,
                       "checks": [vars(check) for check in s.checks],
                       "trace": s.trace} for s in scenarios],
        "trace_events": trace,
    }


def _write_junit(path: Path, scenarios: list[Scenario]) -> None:
    checks = [check for scenario in scenarios for check in scenario.checks]
    suite = ET.Element("testsuite", name="boat-midsem-scenarios-1-11",
                       tests=str(len(scenarios) + len(checks)),
                       failures=str(sum(check.status == "FAIL" for check in checks)),
                       skipped=str(sum(s.status == "PENDING" for s in scenarios)))
    for scenario in scenarios:
        case = ET.SubElement(suite, "testcase", classname=f"scenario.{scenario.number}", name="production integration")
        if scenario.status == "PENDING":
            ET.SubElement(case, "skipped", message=scenario.pending_reason)
        for check in scenario.checks:
            check_case = ET.SubElement(suite, "testcase", classname=f"scenario.{scenario.number}", name=check.name)
            if check.status == "FAIL":
                ET.SubElement(check_case, "failure", message=check.name).text = check.detail
    ET.indent(suite, space="  ")
    path.write_text(ET.tostring(suite, encoding="unicode") + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", type=Path, default=OUT / "results.json")
    parser.add_argument("--junit", type=Path, default=OUT / "junit.xml")
    parser.add_argument("--trace", type=Path, default=OUT / "trace.jsonl")
    args = parser.parse_args(argv)
    scenarios, trace = build_report()
    document = _document(scenarios, trace)
    for output in (args.json, args.junit, args.trace):
        output.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(document, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    _write_junit(args.junit, scenarios)
    args.trace.write_text("".join(json.dumps(event, sort_keys=True) + "\n" for event in trace), encoding="utf-8")
    for scenario in scenarios:
        passed_checks = sum(check.status == "PASS" for check in scenario.checks)
        failed_checks = sum(check.status == "FAIL" for check in scenario.checks)
        print(f"{scenario.number:02d} {scenario.status:7} {scenario.title} (checks: {passed_checks} PASS, {failed_checks} FAIL)")
    print(f"JSON: {args.json}\nJUnit: {args.junit}\nTrace: {args.trace}")
    return 1 if any(scenario.status in {"FAIL", "PENDING"} for scenario in scenarios) else 0


if __name__ == "__main__":
    raise SystemExit(main())
