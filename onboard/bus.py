"""Serialized mailbox and register bus interfaces; hardware import is optional."""

from __future__ import annotations

from collections import defaultdict, deque
from dataclasses import dataclass
from typing import Protocol

from boat.ids import MAILBOX_FRAME


class I2CBus(Protocol):
    def write_mailbox(self, address: int, data: bytes) -> None: ...
    def read_mailbox(self, address: int) -> bytes: ...
    def read_registers(self, address: int, register: int, length: int) -> bytes: ...
    def close(self) -> None: ...


class SMBus2Adapter:
    """Linux SMBus adapter using one I2C_RDWR transaction for each mailbox."""

    def __init__(self, bus_number: int = 1):
        try:
            from smbus2 import SMBus, i2c_msg
        except ImportError as exc:
            raise RuntimeError("real bus mode requires the optional smbus2 package") from exc
        self._bus = SMBus(bus_number)
        self._i2c_msg = i2c_msg

    def write_mailbox(self, address: int, data: bytes) -> None:
        if len(data) != MAILBOX_FRAME:
            raise ValueError("mailbox writes must be exactly 64 bytes")
        self._bus.i2c_rdwr(self._i2c_msg.write(address, data))

    def read_mailbox(self, address: int) -> bytes:
        message = self._i2c_msg.read(address, MAILBOX_FRAME)
        self._bus.i2c_rdwr(message)
        result = bytes(message)
        if len(result) != MAILBOX_FRAME:
            raise IOError("short mailbox read")
        return result

    def read_registers(self, address: int, register: int, length: int) -> bytes:
        write = self._i2c_msg.write(address, bytes((register,)))
        read = self._i2c_msg.read(address, length)
        self._bus.i2c_rdwr(write, read)
        return bytes(read)

    def close(self) -> None:
        self._bus.close()


@dataclass(frozen=True)
class BusOperation:
    kind: str
    address: int
    data: bytes = b""


class SimulatedBus:
    """Injectable bus; receives frames, exposes writes and deterministic faults."""

    simulated = True

    def __init__(self, *, present: tuple[int, ...] = (0x08, 0x09),
                 imu_samples: dict[int, bytes] | None = None):
        self.present = set(present)
        self.incoming: dict[int, deque[bytes]] = defaultdict(deque)
        self.snapshots: dict[int, bytes] = {}
        self.imu_samples = dict(imu_samples or {})
        self.operations: list[BusOperation] = []
        self.fail_next: dict[str, int] = defaultdict(int)
        self.closed = False

    def inject_frame(self, address: int, frame: bytes, *, snapshot: bool = False) -> None:
        padded = bytes(frame).ljust(MAILBOX_FRAME, b"\0")
        if len(padded) != MAILBOX_FRAME:
            raise ValueError("simulated frame exceeds mailbox")
        if snapshot:
            self.snapshots[address] = padded
        else:
            self.incoming[address].append(padded)

    def inject_fault(self, operation: str, count: int = 1) -> None:
        if operation not in ("read", "write", "imu") or count < 1:
            raise ValueError("fault operation must be read, write, or imu; count positive")
        self.fail_next[operation] += count

    def _fault(self, operation: str, address: int) -> None:
        self.operations.append(BusOperation(operation, address))
        if self.fail_next[operation]:
            self.fail_next[operation] -= 1
            raise OSError(f"simulated {operation} fault at 0x{address:02x}")
        if address not in self.present and not (operation == "imu" and address in self.imu_samples):
            raise OSError(f"no simulated device at 0x{address:02x}")

    def write_mailbox(self, address: int, data: bytes) -> None:
        if len(data) != MAILBOX_FRAME:
            raise ValueError("mailbox writes must be exactly 64 bytes")
        self._fault("write", address)
        self.operations[-1] = BusOperation("write", address, bytes(data))

    def read_mailbox(self, address: int) -> bytes:
        self._fault("read", address)
        if self.incoming[address]:
            return self.incoming[address].popleft()
        return self.snapshots.get(address, bytes(MAILBOX_FRAME))

    def read_registers(self, address: int, register: int, length: int) -> bytes:
        self._fault("imu", address)
        sample = self.imu_samples.get(address)
        if sample is None or len(sample) < length:
            raise OSError(f"no configured simulated IMU sample at 0x{address:02x}")
        self.operations[-1] = BusOperation("imu", address, bytes((register, length)))
        return sample[:length]

    def close(self) -> None:
        self.closed = True
