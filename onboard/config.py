"""Validated TOML configuration for the Pi bus service."""

from __future__ import annotations

from dataclasses import dataclass
import tomllib
from pathlib import Path

from boat.ids import BUS_HZ, COMPONENT_COMMS_SENSOR, COMPONENT_MOTOR


@dataclass(frozen=True)
class IMURegisterConfig:
    address: int
    register: int
    accel_scale_mg_per_count: float
    gyro_scale_mrad_s_per_count: float
    byte_order: str = "little"
    signed: bool = True
    sample_size: int = 12
    interval_ms: int = 100

    def __post_init__(self) -> None:
        if not 0x03 <= self.address <= 0x77:
            raise ValueError("IMU address must be a non-reserved 7-bit I2C address")
        if not 0 <= self.register <= 0xFF:
            raise ValueError("IMU register must be 0..255")
        if self.sample_size != 12:
            raise ValueError("generic IMU register layout requires exactly six 16-bit values")
        if self.byte_order not in ("little", "big"):
            raise ValueError("byte_order must be 'little' or 'big'")
        if self.accel_scale_mg_per_count <= 0 or self.gyro_scale_mrad_s_per_count <= 0:
            raise ValueError("IMU scale factors must be positive")
        if self.interval_ms < 1:
            raise ValueError("IMU interval_ms must be positive")


@dataclass(frozen=True)
class ServiceConfig:
    bus_number: int = 1
    bus_hz: int = BUS_HZ
    motor_address: int = 0x08
    comms_address: int = 0x09
    motor_component: int = COMPONENT_MOTOR
    comms_component: int = COMPONENT_COMMS_SENSOR
    poll_interval_ms: int = 20
    queue_limit: int = 128
    imu: IMURegisterConfig | None = None

    def __post_init__(self) -> None:
        if self.bus_number < 0 or self.bus_hz != BUS_HZ:
            raise ValueError("bus number must be nonnegative and bus_hz is fixed at 100000")
        addresses = (self.motor_address, self.comms_address)
        if any(not 0x03 <= address <= 0x77 for address in addresses):
            raise ValueError("peer addresses must be non-reserved 7-bit I2C addresses")
        if len(set(addresses)) != 2:
            raise ValueError("motor and comms addresses must be distinct")
        if self.imu and self.imu.address in addresses:
            raise ValueError("IMU address must be distinct from both mailbox peers")
        if self.poll_interval_ms < 1 or self.queue_limit < 4:
            raise ValueError("poll_interval_ms must be positive and queue_limit at least 4")

    @classmethod
    def from_toml(cls, path: str | Path) -> "ServiceConfig":
        with Path(path).open("rb") as stream:
            data = tomllib.load(stream)
        bus = data.get("bus", {})
        peers = data.get("peers", {})
        scheduler = data.get("scheduler", {})
        raw_imu = data.get("imu")
        if raw_imu:
            raw_imu = dict(raw_imu)
            enabled = raw_imu.pop("enabled", True)
            imu = IMURegisterConfig(**raw_imu) if enabled else None
        else:
            imu = None
        return cls(bus_number=bus.get("number", 1), bus_hz=bus.get("hz", BUS_HZ),
                   motor_address=peers.get("motor_address", 0x08),
                   comms_address=peers.get("comms_address", 0x09),
                   motor_component=peers.get("motor_component", COMPONENT_MOTOR),
                   comms_component=peers.get("comms_component", COMPONENT_COMMS_SENSOR),
                   poll_interval_ms=scheduler.get("poll_interval_ms", 20),
                   queue_limit=scheduler.get("queue_limit", 128), imu=imu)
