"""Small deterministic transport fixtures; these do not emulate boat routing or motors."""

from __future__ import annotations

from dataclasses import dataclass

from boat.client import Transport
from protocol import codec


class ManualClock:
    def __init__(self, now: float = 0.0) -> None:
        self.now = now

    def __call__(self) -> float:
        return self.now

    def advance(self, seconds: float) -> None:
        if seconds < 0:
            raise ValueError("clock cannot move backwards")
        self.now += seconds


class CapturingTransport(Transport):
    """A frame transport for testing BoatClient calls, not a system router."""

    def __init__(self, name: str = "fixture") -> None:
        self.name = name
        self.sent: list[bytes] = []
        self.on_frame = None
        self.opened = False
        self.fail_next_write = False
        self.attempts: list[bytes] = []

    def open(self, on_frame) -> None:
        self.on_frame = on_frame
        self.opened = True

    def close(self) -> None:
        self.opened = False
        self.on_frame = None

    def send_frame(self, frame: bytes) -> None:
        if not self.opened:
            raise ConnectionError("fixture transport is closed")
        self.attempts.append(bytes(frame))
        if self.fail_next_write:
            self.fail_next_write = False
            raise TimeoutError("injected ambiguous write")
        self.sent.append(bytes(frame))

    def inject(self, frame: bytes) -> None:
        if self.opened and self.on_frame is not None:
            self.on_frame(bytes(frame))


class FragmentedStream:
    """Deterministic USB-like fragmentation using the production frame parser."""

    def __init__(self) -> None:
        self.parser = codec.FrameStream()

    def feed(self, data: bytes) -> list[bytes]:
        return [frame for frame, _message in self.parser.feed(data)]


@dataclass(frozen=True)
class MailboxRead:
    frame: bytes | None
    raw: bytes


class MailboxFixture:
    """64-byte contract fixture; it is not an implementation of the MCU mailbox."""

    FRAME_BYTES = 64

    @classmethod
    def write(cls, raw: bytes) -> bytes:
        if not raw or len(raw) > cls.FRAME_BYTES:
            raise ValueError("mailbox write must contain one frame of at most 64 bytes")
        decoded = codec.decode(raw)
        if len(raw) > 64 or raw[2] & 1:
            raise ValueError("mailbox accepts only unsigned frames within 64 bytes")
        if codec.FrameStream().feed(raw) == []:
            raise ValueError("mailbox write is not one complete frame")
        stream = codec.FrameStream()
        frames = stream.feed(raw)
        if len(frames) != 1 or stream.buffer:
            raise ValueError("mailbox write must contain exactly one frame")
        if not decoded["name"]:
            raise ValueError("unknown mailbox message")
        return raw + bytes(cls.FRAME_BYTES - len(raw))

    @classmethod
    def read(cls, padded: bytes) -> MailboxRead:
        if len(padded) != cls.FRAME_BYTES:
            raise ValueError("mailbox reads are exactly 64 bytes")
        if not any(padded):
            return MailboxRead(None, padded)
        stream = codec.FrameStream()
        frames = stream.feed(padded)
        if len(frames) != 1 or any(stream.buffer):
            raise ValueError("mailbox read must contain one frame and zero padding")
        frame = frames[0][0]
        if padded[len(frame):] != bytes(cls.FRAME_BYTES - len(frame)):
            raise ValueError("mailbox padding must be zero")
        return MailboxRead(frame, padded)


class ModemFixture:
    """RYLR998 envelope fixture around the production Base64 codec helpers."""

    @staticmethod
    def transmit(frame: bytes, address: int = 0) -> bytes:
        return codec.radio_command(frame, address)

    @staticmethod
    def receive(line: bytes):
        return codec.radio_receive(line)
