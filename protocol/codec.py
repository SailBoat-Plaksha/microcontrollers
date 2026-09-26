"""Small MAVLink 2 codec for the ARSEM dialect and its serial/LoRa bridges.

The XML is the source of truth for custom payload layout and CRC_EXTRA.  This
module deliberately handles unsigned MAVLink 2 frames only; signed frames are
passed through by routers but never accepted as actuator commands here.
"""

import base64
import binascii
import re
import struct
import xml.etree.ElementTree as ET
from pathlib import Path

MAGIC = 0xFD
MAX_RADIO_TEXT = 240
TYPE_FORMAT = {"uint8_t": "B", "int8_t": "b", "uint16_t": "H", "int16_t": "h",
               "uint32_t": "I", "int32_t": "i", "uint64_t": "Q", "int64_t": "q",
               "float": "f", "double": "d"}


def crc(data, initial=0xFFFF):
    value = initial
    for byte in data:
        tmp = byte ^ (value & 0xFF)
        tmp ^= (tmp << 4) & 0xFF
        value = ((value >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF
    return value


def definitions():
    root = ET.parse(Path(__file__).with_name("arsem.xml")).getroot()
    result = {}
    for message in root.findall("./messages/message"):
        fields = []
        for field in message.findall("field"):
            field_type = field.attrib["type"]
            if field_type not in TYPE_FORMAT:
                raise ValueError("unsupported field type: " + field_type)
            fields.append((field_type, field.attrib["name"]))
        fields.sort(key=lambda field: -struct.calcsize(TYPE_FORMAT[field[0]]))
        seed = (message.attrib["name"] + " ").encode()
        for field_type, name in fields:
            seed += (field_type + " " + name + " ").encode()
        check = crc(seed)
        result[int(message.attrib["id"])] = {
            "name": message.attrib["name"],
            "fields": fields,
            "format": "<" + "".join(TYPE_FORMAT[field_type] for field_type, _ in fields),
            "crc_extra": (check & 0xFF) ^ (check >> 8),
        }
    return result


MESSAGES = definitions()
BY_NAME = {definition["name"]: (id_, definition) for id_, definition in MESSAGES.items()}
# HEARTBEAT from common.xml. Used only for inspection and a known CRC check.
MESSAGES[0] = {"name": "HEARTBEAT", "fields": [], "format": "", "crc_extra": 50}


def encode(name, fields, *, system=1, component=1, sequence=0):
    message_id, definition = BY_NAME[name]
    payload = struct.pack(definition["format"], *(fields[field_name] for _, field_name in definition["fields"]))
    payload = payload.rstrip(b"\0") or b"\0"
    header = bytes((len(payload), 0, 0, sequence, system, component,
                    message_id & 0xFF, (message_id >> 8) & 0xFF, (message_id >> 16) & 0xFF))
    checksum = crc(header + payload + bytes((definition["crc_extra"],)))
    return bytes((MAGIC,)) + header + payload + struct.pack("<H", checksum)


def decode(frame):
    if len(frame) < 13 or frame[0] != MAGIC:
        raise ValueError("invalid MAVLink 2 frame")
    length, flags = frame[1], frame[2]
    if flags & ~1:
        raise ValueError("unsupported incompatibility flags")
    expected = 12 + length + (13 if flags & 1 else 0)
    if len(frame) != expected:
        raise ValueError("frame length mismatch")
    message_id = int.from_bytes(frame[7:10], "little")
    definition = MESSAGES.get(message_id)
    if definition is None:
        raise ValueError("unknown message ID")
    checksum = crc(frame[1:10 + length] + bytes((definition["crc_extra"],)))
    if checksum != int.from_bytes(frame[10 + length:12 + length], "little"):
        raise ValueError("CRC mismatch")
    if flags & 1:
        raise ValueError("signed frame requires signature verification")
    fields = {}
    if definition["fields"]:
        size = struct.calcsize(definition["format"])
        if length > size:
            raise ValueError("payload too long")
        values = struct.unpack(definition["format"], frame[10:10 + length].ljust(size, b"\0"))
        fields = dict(zip((name for _, name in definition["fields"]), values))
    return {"name": definition["name"], "id": message_id, "system": frame[5],
            "component": frame[6], "sequence": frame[4], "fields": fields}


class FrameStream:
    def __init__(self):
        self.buffer = bytearray()

    def feed(self, data):
        self.buffer.extend(data)
        frames = []
        while self.buffer:
            start = self.buffer.find(MAGIC)
            if start < 0:
                self.buffer.clear()
                break
            del self.buffer[:start]
            if len(self.buffer) < 10:
                break
            if self.buffer[2] & ~1:
                del self.buffer[0]
                continue
            size = 12 + self.buffer[1] + (13 if self.buffer[2] & 1 else 0)
            if len(self.buffer) < size:
                break
            candidate = bytes(self.buffer[:size])
            try:
                decoded = decode(candidate)
            except ValueError:
                del self.buffer[0]
                continue
            frames.append((candidate, decoded))
            del self.buffer[:size]
        return frames


def radio_command(frame, address=0):
    decode(frame)
    encoded = base64.b64encode(frame).decode("ascii")
    if len(encoded) > MAX_RADIO_TEXT:
        raise ValueError("frame exceeds RYLR998 240-character payload limit")
    return f"AT+SEND={address},{len(encoded)},{encoded}\r\n".encode("ascii")


def radio_receive(line):
    match = re.fullmatch(rb"\+RCV=(\d+),(\d+),([A-Za-z0-9+/=]+),(-?\d+),(-?\d+(?:\.\d+)?)", line.strip())
    if match is None:
        raise ValueError("malformed RYLR998 receive line")
    address, length, encoded, rssi, snr = match.groups()
    if int(length) != len(encoded) or len(encoded) > MAX_RADIO_TEXT:
        raise ValueError("radio payload length mismatch")
    try:
        frame = base64.b64decode(encoded, validate=True)
    except binascii.Error as exc:
        raise ValueError("invalid Base64") from exc
    return frame, decode(frame), {"address": int(address), "rssi": int(rssi), "snr": float(snr)}
