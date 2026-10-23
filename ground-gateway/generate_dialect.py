"""Regenerate dialect.py constants from committed generated C headers."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADERS = ROOT / "protocol" / "gen" / "C" / "arsem_standalone"
MAILBOX_HEADER = ROOT / "protocol" / "mailbox.h"
MESSAGES = {
    0: "heartbeat", 4: "ping", 26: "scaled_imu",
    200000: "node_info", 200001: "actuator_setpoint", 200002: "actuator_status",
    200003: "control_authority", 200004: "environmental_sample", 200005: "power_state",
    200006: "link_status", 200007: "bus_grant", 200008: "actuator_request",
    200009: "node_health", 200010: "request_result",
}

crc = {}
opcode_frame_offset = None
mailbox = MAILBOX_HEADER.read_text()


def mailbox_constant(name):
    match = re.search(r"#define\s+" + name + r"\s+([0-9]+)u?", mailbox)
    if not match:
        raise SystemExit("missing mailbox constant: " + name)
    return int(match.group(1))


for message_id, stem in MESSAGES.items():
    header = (HEADERS / ("mavlink_msg_" + stem + ".h")).read_text()
    name = stem.upper()
    match = re.search(r"#define MAVLINK_MSG_ID_" + name + r"_CRC (\d+)", header)
    if not match:
        raise SystemExit("missing CRC_EXTRA in generated header: " + stem)
    crc[message_id] = int(match.group(1))
    if message_id == 200008:
        opcode = re.search(
            r'"opcode".*?MAVLINK_TYPE_UINT8_T,\s*0,\s*(\d+),',
            header, re.DOTALL)
        if not opcode:
            raise SystemExit("missing generated opcode payload offset")
        opcode_frame_offset = 10 + int(opcode.group(1))

lines = [
    '"""Constants derived from generated C headers and protocol/mailbox.h."""',
    "SYSTEM_BOAT = %d" % mailbox_constant("ARSEM_SYSTEM_BOAT"),
    "COMPONENT_PI = %d" % mailbox_constant("ARSEM_COMPONENT_PI"),
    "COMPONENT_MOTOR = %d" % mailbox_constant("ARSEM_COMPONENT_MOTOR"),
    "COMPONENT_COMMS_SENSOR = %d" % mailbox_constant("ARSEM_COMPONENT_COMMS_SENSOR"),
    "SYSTEM_GROUND = %d" % mailbox_constant("ARSEM_SYSTEM_GROUND"),
    "COMPONENT_GROUND = %d" % mailbox_constant("ARSEM_COMPONENT_GROUND"),
    "MAILBOX_BYTES = %d" % mailbox_constant("ARSEM_MAILBOX_FRAME_BYTES"),
    "CRC_EXTRA = {",
]
for message_id, value in crc.items():
    lines.append("    %d: %d," % (message_id, value))
lines += [
    "}", "MSG_HEARTBEAT = 0", "MSG_PING = 4", "MSG_SCALED_IMU = 26",
    "MSG_ACTUATOR_REQUEST = 200008", "MSG_NODE_HEALTH = 200009",
    "MSG_REQUEST_RESULT = 200010", "MSG_LINK_STATUS = 200006", "",
    "# MAVLink frame offset for opcode, from the generated 19-byte message layout.",
    "ACTUATOR_REQUEST_OPCODE_FRAME_OFFSET = %d" % opcode_frame_offset, "",
]
(Path(__file__).resolve().parent / "dialect.py").write_text("\n".join(lines))
