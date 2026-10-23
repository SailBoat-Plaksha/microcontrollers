"""Small unsigned MAVLink 2 validator for MicroPython stream relays."""

import dialect

STX = 0xFD


def _x25_accumulate(byte, crc):
    tmp = byte ^ (crc & 0xFF)
    tmp = (tmp ^ ((tmp << 4) & 0xFF)) & 0xFF
    return ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF


def frame_length(frame):
    """Return full unsigned MAVLink 2 frame length, or zero if incomplete/bad."""
    if len(frame) < 3 or frame[0] != STX:
        return 0
    if frame[2] != 0:  # signed bit and unknown incompatibility flags
        return -1
    return frame[1] + 12


def validate_frame(frame):
    """Accept exactly one known, unsigned MAVLink 2 frame, preserving bytes."""
    size = frame_length(frame)
    if size < 0 or size != len(frame) or size > dialect.MAILBOX_BYTES:
        return False
    msgid = frame[7] | (frame[8] << 8) | (frame[9] << 16)
    extra = dialect.CRC_EXTRA.get(msgid)
    if extra is None:
        return False
    crc = 0xFFFF
    for byte in frame[1:10 + frame[1]]:
        crc = _x25_accumulate(byte, crc)
    crc = _x25_accumulate(extra, crc)
    received = frame[10 + frame[1]] | (frame[11 + frame[1]] << 8)
    return crc == received


class FrameStream:
    """Fragment-safe byte stream parser; damaged packets resync at the next FD."""

    def __init__(self):
        self.buffer = bytearray()
        self.dropped = 0

    def feed(self, data):
        frames = []
        for byte in data:
            if not self.buffer:
                if byte != STX:
                    continue
                self.buffer.append(byte)
                continue
            self.buffer.append(byte)
            if len(self.buffer) == 3 and self.buffer[2] != 0:
                self.dropped += 1
                self.buffer = bytearray()
                if byte == STX:
                    self.buffer.append(byte)
                continue
            if len(self.buffer) >= 3:
                expected = self.buffer[1] + 12
                if expected > dialect.MAILBOX_BYTES:
                    self.dropped += 1
                    self.buffer = bytearray()
                    continue
                if len(self.buffer) == expected:
                    candidate = bytes(self.buffer)
                    if validate_frame(candidate):
                        frames.append(candidate)
                    else:
                        self.dropped += 1
                    self.buffer = bytearray()
                elif len(self.buffer) > expected:
                    self.dropped += 1
                    self.buffer = bytearray()
        return frames
