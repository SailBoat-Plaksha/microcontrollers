"""Validated TOML configuration for the Pi bus service."""

from __future__ import annotations

from dataclasses import dataclass
import math
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
        if not isinstance(self.byte_order, str) or not isinstance(self.signed, bool):
            raise ValueError("IMU byte_order must be text and signed must be true or false")
        if not isinstance(self.address, int) or isinstance(self.address, bool) or not 0x03 <= self.address <= 0x77:
            raise ValueError("IMU address must be a non-reserved 7-bit I2C address")
        if not isinstance(self.register, int) or isinstance(self.register, bool) or not 0 <= self.register <= 0xFF:
            raise ValueError("IMU register must be 0..255")
        if not isinstance(self.sample_size, int) or isinstance(self.sample_size, bool) or self.sample_size != 12:
            raise ValueError("generic IMU register layout requires exactly six 16-bit values")
        if self.byte_order not in ("little", "big"):
            raise ValueError("byte_order must be 'little' or 'big'")
        if (not _finite_positive(self.accel_scale_mg_per_count) or
                not _finite_positive(self.gyro_scale_mrad_s_per_count)):
            raise ValueError("IMU scale factors must be positive")
        if not isinstance(self.interval_ms, int) or isinstance(self.interval_ms, bool) or self.interval_ms < 1:
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
    comms_enabled: bool = True
    imu: IMURegisterConfig | None = None

    def __post_init__(self) -> None:
        integer_fields = (self.bus_number, self.bus_hz, self.motor_address,
                          self.comms_address, self.motor_component,
                          self.comms_component, self.poll_interval_ms, self.queue_limit)
        if any(not isinstance(value, int) or isinstance(value, bool) for value in integer_fields):
            raise ValueError("bus, peer and scheduler settings must be integers")
        if not isinstance(self.comms_enabled, bool):
            raise ValueError("comms_enabled must be true or false")
        if self.bus_number < 0 or self.bus_hz != BUS_HZ:
            raise ValueError("bus number must be nonnegative and bus_hz is fixed at 100000")
        addresses = (self.motor_address, self.comms_address)
        if any(not 0x03 <= address <= 0x77 for address in addresses):
            raise ValueError("peer addresses must be non-reserved 7-bit I2C addresses")
        if len(set(addresses)) != 2:
            raise ValueError("motor and comms addresses must be distinct")
        if (not 1 <= self.motor_component <= 255 or not 1 <= self.comms_component <= 255 or
                self.motor_component == self.comms_component or
                ids_component_collision(self.motor_component, self.comms_component)):
            raise ValueError("peer components must be distinct valid MAVLink component IDs")
        if (self.motor_component != COMPONENT_MOTOR or
                self.comms_component != COMPONENT_COMMS_SENSOR):
            raise ValueError("peer components are fixed by the contract: motor 154, comms 158")
        if self.imu is not None and not isinstance(self.imu, IMURegisterConfig):
            raise ValueError("imu must be an IMURegisterConfig")
        if self.imu and self.imu.address in addresses:
            raise ValueError("IMU address must be distinct from both mailbox peers")
        if self.poll_interval_ms < 1 or self.queue_limit < 4:
            raise ValueError("poll_interval_ms must be positive and queue_limit at least 4")

    @classmethod
    def from_toml(cls, path: str | Path) -> "ServiceConfig":
        with Path(path).open("rb") as stream:
            data = tomllib.load(stream)
        unknown_top_level = set(data) - {"bus", "peers", "scheduler", "service", "imu"}
        if unknown_top_level:
            raise ValueError(f"unknown top-level settings: {sorted(unknown_top_level)}")
        bus = _table(data, "bus")
        peers = _table(data, "peers")
        scheduler = _table(data, "scheduler")
        service = _table(data, "service")
        for table_name, table, allowed in (
                ("bus", bus, {"number", "hz"}),
                ("peers", peers, {"motor_address", "comms_address", "motor_component", "comms_component"}),
                ("scheduler", scheduler, {"poll_interval_ms", "queue_limit"}),
                ("service", service, {"comms_enabled"})):
            unknown = set(table) - allowed
            if unknown:
                raise ValueError(f"unknown {table_name} settings: {sorted(unknown)}")
        raw_imu = data.get("imu")
        if raw_imu is not None:
            if not isinstance(raw_imu, dict):
                raise ValueError("imu configuration must be a TOML table")
            raw_imu = dict(raw_imu)
            enabled = raw_imu.pop("enabled", True)
            if not isinstance(enabled, bool):
                raise ValueError("imu.enabled must be true or false")
            allowed_imu = {"address", "register", "accel_scale_mg_per_count",
                           "gyro_scale_mrad_s_per_count", "byte_order", "signed",
                           "sample_size", "interval_ms"}
            if set(raw_imu) - allowed_imu:
                raise ValueError(f"unknown imu settings: {sorted(set(raw_imu) - allowed_imu)}")
            try:
                imu = IMURegisterConfig(**raw_imu) if enabled else None
            except TypeError as exc:
                raise ValueError(f"incomplete imu configuration: {exc}") from exc
        else:
            imu = None
        comms_enabled = service.get("comms_enabled", True)
        if not isinstance(comms_enabled, bool):
            raise ValueError("service.comms_enabled must be true or false")
        return cls(bus_number=bus.get("number", 1), bus_hz=bus.get("hz", BUS_HZ),
                   motor_address=peers.get("motor_address", 0x08),
                   comms_address=peers.get("comms_address", 0x09),
                   motor_component=peers.get("motor_component", COMPONENT_MOTOR),
                   comms_component=peers.get("comms_component", COMPONENT_COMMS_SENSOR),
                   poll_interval_ms=scheduler.get("poll_interval_ms", 20),
                   queue_limit=scheduler.get("queue_limit", 128),
                   comms_enabled=comms_enabled, imu=imu)


def _table(data: dict, name: str) -> dict:
    value = data.get(name, {})
    if not isinstance(value, dict):
        raise ValueError(f"{name} configuration must be a TOML table")
    return value


def ids_component_collision(motor: int, comms: int) -> bool:
    # Component 1 is the Pi itself; the peers cannot alias the bus controller.
    return motor == 1 or comms == 1


def _finite_positive(value: object) -> bool:
    if not isinstance(value, (int, float)) or isinstance(value, bool):
        return False
    try:
        return math.isfinite(value) and value > 0
    except (OverflowError, TypeError, ValueError):
        return False
