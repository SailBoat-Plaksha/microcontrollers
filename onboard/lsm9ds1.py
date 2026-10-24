"""Direct, bounded-register-access driver for the ST LSM9DS1 IMU.

The accelerometer/gyroscope and magnetometer are separate I2C targets and
register banks. ``read_sample`` never waits for a conversion: it returns the
fields whose data-ready flags are set, or raises :class:`SensorNotReady` when
none are ready. Unavailable fields are zero tuples and are excluded from
``valid_fields``; callers must use that mask before consuming a field.
"""

from __future__ import annotations

from dataclasses import dataclass
import math
import time
from typing import Protocol


class RegisterBus(Protocol):
    def read_registers(self, address: int, register: int, length: int) -> bytes: ...
    def write_registers(self, address: int, register: int, data: bytes) -> None: ...


class LSM9DS1Error(Exception):
    """Base class for sensor identity, configuration, and readiness errors."""


class SensorNotReady(LSM9DS1Error):
    """No complete sensor field had new data at the time of the read."""


class SensorIdentityError(LSM9DS1Error):
    """A register bank did not identify itself as an LSM9DS1 component."""


class ConfigurationError(LSM9DS1Error):
    """A configuration register did not retain the requested value."""


@dataclass(frozen=True)
class LSM9DS1Sample:
    accel_mg: tuple[float, float, float]
    gyro_mrad_s: tuple[float, float, float]
    mag_mgauss: tuple[float, float, float]
    valid_fields: int
    acquired_at: float


class LSM9DS1:
    """LSM9DS1 at its AG and magnetometer I2C addresses.

    Defaults use ±2 g, ±245 dps and ±4 gauss ranges with independent 119 Hz
    accel/gyro and 10 Hz magnetometer outputs. Reads are single, finite
    register transactions; the driver does not sleep, poll, reset, or open a
    hardware bus. The injected bus must provide ``write_registers``.
    """

    ACCEL_VALID = 0x01
    GYRO_VALID = 0x02
    MAG_VALID = 0x04

    _WHO_AM_I = 0x0F
    _AG_ID = 0x68
    _MAG_ID = 0x3D
    _CTRL_REG1_G = 0x10
    _CTRL_REG6_XL = 0x20
    _CTRL_REG8 = 0x22
    _STATUS_REG_AG = 0x17
    _OUT_X_L_G = 0x18
    _OUT_X_L_XL = 0x28
    _CTRL_REG1_M = 0x20
    _CTRL_REG2_M = 0x21
    _CTRL_REG3_M = 0x22
    _CTRL_REG4_M = 0x23
    _CTRL_REG5_M = 0x24
    _STATUS_REG_M = 0x27
    _OUT_X_L_M = 0x28

    _CONFIG = (
        ("AG", _CTRL_REG8, 0x44),  # BDU + register auto-increment
        ("AG", _CTRL_REG1_G, 0x60),  # 119 Hz, ±245 dps
        ("AG", _CTRL_REG6_XL, 0x60),  # 119 Hz, ±2 g
        ("M", _CTRL_REG1_M, 0x70),  # high XY performance, 10 Hz
        ("M", _CTRL_REG2_M, 0x00),  # ±4 gauss
        ("M", _CTRL_REG3_M, 0x00),  # continuous conversion
        ("M", _CTRL_REG4_M, 0x0C),  # high Z performance
        ("M", _CTRL_REG5_M, 0x40),  # magnetometer BDU
    )

    def __init__(self, bus: RegisterBus, *, ag_address: int = 0x6B,
                 mag_address: int = 0x1E, clock=time.monotonic):
        if ag_address not in (0x6A, 0x6B):
            raise ValueError("ag_address must be 0x6A or 0x6B")
        if mag_address not in (0x1C, 0x1E):
            raise ValueError("mag_address must be 0x1C or 0x1E")
        self._bus = bus
        self.ag_address = ag_address
        self.mag_address = mag_address
        self._clock = clock
        self._initialized = False

    def initialize(self) -> None:
        """Check both identities, configure fixed ranges, and verify readback."""
        self._initialized = False
        ag_id = self._read(self.ag_address, self._WHO_AM_I, 1, "AG")[0]
        if ag_id != self._AG_ID:
            raise SensorIdentityError(f"AG WHO_AM_I was 0x{ag_id:02x}, expected 0x68")
        mag_id = self._read(self.mag_address, self._WHO_AM_I, 1, "M")[0]
        if mag_id != self._MAG_ID:
            raise SensorIdentityError(f"mag WHO_AM_I was 0x{mag_id:02x}, expected 0x3D")

        for bank, register, value in self._CONFIG:
            address = self.ag_address if bank == "AG" else self.mag_address
            self._bus.write_registers(address, register, bytes((value,)))
            actual = self._read(address, register, 1, bank)[0]
            if actual != value:
                raise ConfigurationError(
                    f"{bank} register 0x{register:02x} read 0x{actual:02x}; expected 0x{value:02x}"
                )
        self._initialized = True

    def read_sample(self) -> LSM9DS1Sample:
        """Return newly ready fields, or raise ``SensorNotReady`` if none are ready.

        ``valid_fields`` uses bit 0 for acceleration, bit 1 for gyro, and bit 2
        for magnetometer. Fields without a ready flag are zero tuples and are
        not cached from an earlier call.
        """
        if not self._initialized:
            raise LSM9DS1Error("initialize() must succeed before read_sample()")

        ag_status = self._read(self.ag_address, self._STATUS_REG_AG, 1, "AG")[0]
        mag_status = self._read(self.mag_address, self._STATUS_REG_M, 1, "M")[0]
        accel = (0.0, 0.0, 0.0)
        gyro = (0.0, 0.0, 0.0)
        mag = (0.0, 0.0, 0.0)
        valid = 0

        if ag_status & 0x01:
            counts = self._read_axes(self.ag_address, self._OUT_X_L_XL, "AG")
            accel = tuple(count * 0.061 for count in counts)
            valid |= self.ACCEL_VALID
        if ag_status & 0x02:
            counts = self._read_axes(self.ag_address, self._OUT_X_L_G, "AG")
            # 8.75 mdps/LSB * pi/180 converts millidegrees/s to milliradians/s.
            gyro = tuple(count * (8.75 * math.pi / 180.0) for count in counts)
            valid |= self.GYRO_VALID
        if mag_status & 0x08:
            counts = self._read_axes(self.mag_address, self._OUT_X_L_M, "M")
            mag = tuple(count * 0.14 for count in counts)
            valid |= self.MAG_VALID

        if not valid:
            raise SensorNotReady("no accelerometer, gyroscope, or magnetometer data is ready")
        return LSM9DS1Sample(accel, gyro, mag, valid, self._clock())

    def _read_axes(self, address: int, register: int, bank: str) -> tuple[int, int, int]:
        # Magnetometer I2C burst increment uses SUB[7], unlike AG CTRL_REG8.
        if bank == "M":
            register |= 0x80
        raw = self._read(address, register, 6, bank)
        return tuple(int.from_bytes(raw[i:i + 2], "little", signed=True)
                     for i in (0, 2, 4))

    def _read(self, address: int, register: int, length: int, bank: str) -> bytes:
        raw = self._bus.read_registers(address, register, length)
        try:
            data = bytes(raw)
        except (TypeError, ValueError) as exc:
            raise IOError(f"{bank} register read returned non-byte data") from exc
        if len(data) != length:
            raise IOError(
                f"{bank} register 0x{register:02x} returned {len(data)} bytes; expected {length}"
            )
        return data
