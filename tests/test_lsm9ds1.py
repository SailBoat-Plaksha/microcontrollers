from __future__ import annotations

import math

import pytest

from onboard.lsm9ds1 import (
    ConfigurationError,
    LSM9DS1,
    LSM9DS1Error,
    SensorIdentityError,
    SensorNotReady,
)


def axes(*values: int) -> bytes:
    assert len(values) == 3
    return b"".join(value.to_bytes(2, "little", signed=True) for value in values)


class FakeRegisterBus:
    def __init__(self):
        self.registers: dict[tuple[int, int], int] = {
            (0x6B, 0x0F): 0x68,
            (0x1E, 0x0F): 0x3D,
        }
        self.reads: list[tuple[int, int, int]] = []
        self.writes: list[tuple[int, int, bytes]] = []
        self.short_reads: dict[tuple[int, int], bytes] = {}
        self.ignored_writes: set[tuple[int, int]] = set()

    def read_registers(self, address: int, register: int, length: int) -> bytes:
        self.reads.append((address, register, length))
        if (address, register) in self.short_reads:
            return self.short_reads[(address, register)]
        return bytes(self.registers.get((address, register + offset), 0)
                     for offset in range(length))

    def write_registers(self, address: int, register: int, data: bytes) -> None:
        self.writes.append((address, register, bytes(data)))
        if (address, register) not in self.ignored_writes:
            for offset, value in enumerate(data):
                self.registers[(address, register + offset)] = value

    def set_block(self, address: int, register: int, data: bytes) -> None:
        for offset, value in enumerate(data):
            self.registers[(address, register + offset)] = value


def initialized_sensor(bus: FakeRegisterBus, *, clock=lambda: 123.5) -> LSM9DS1:
    sensor = LSM9DS1(bus, clock=clock)
    sensor.initialize()
    return sensor


def test_initialize_checks_both_ids_and_configures_separate_banks():
    bus = FakeRegisterBus()
    sensor = LSM9DS1(bus)

    sensor.initialize()

    assert bus.reads[:2] == [(0x6B, 0x0F, 1), (0x1E, 0x0F, 1)]
    assert bus.writes == [
        (0x6B, 0x22, b"\x44"),
        (0x6B, 0x10, b"\x60"),
        (0x6B, 0x20, b"\x60"),
        (0x1E, 0x20, b"\x70"),
        (0x1E, 0x21, b"\x00"),
        (0x1E, 0x22, b"\x00"),
        (0x1E, 0x23, b"\x0c"),
        (0x1E, 0x24, b"\x40"),
    ]
    assert sensor._initialized


def test_rejects_wrong_component_identity():
    bus = FakeRegisterBus()
    bus.registers[(0x1E, 0x0F)] = 0x68

    with pytest.raises(SensorIdentityError, match="expected 0x3D"):
        LSM9DS1(bus).initialize()


@pytest.mark.parametrize("kwargs", [
    {"ag_address": 0x69},
    {"mag_address": 0x1D},
    {"ag_address": 0x1E},
])
def test_rejects_bad_i2c_addresses(kwargs):
    with pytest.raises(ValueError):
        LSM9DS1(FakeRegisterBus(), **kwargs)


def test_reads_each_register_bank_and_converts_signed_values_to_documented_units():
    bus = FakeRegisterBus()
    sensor = initialized_sensor(bus)
    bus.set_block(0x6B, 0x17, b"\x09")  # accel and gyro ready
    bus.set_block(0x1E, 0x27, b"\x08")  # mag ready
    bus.set_block(0x6B, 0x28, axes(1000, -1000, -32768))
    bus.set_block(0x6B, 0x18, axes(-2000, 2000, -1))
    bus.set_block(0x1E, 0x28, axes(100, -100, -32768))

    sample = sensor.read_sample()

    assert sample.accel_mg == pytest.approx((61.0, -61.0, -32768 * 0.061))
    gyro_scale = 8.75 * math.pi / 180.0
    assert sample.gyro_mrad_s == pytest.approx((-2000 * gyro_scale, 2000 * gyro_scale, -gyro_scale))
    assert sample.mag_mgauss == pytest.approx((14.0, -14.0, -32768 * 0.14))
    assert sample.valid_fields == 0x07
    assert sample.acquired_at == 123.5
    assert (0x6B, 0x28, 6) in bus.reads
    assert (0x6B, 0x18, 6) in bus.reads
    assert (0x1E, 0x28, 6) in bus.reads


def test_returns_only_ready_fields_without_reusing_previous_values():
    bus = FakeRegisterBus()
    sensor = initialized_sensor(bus)
    bus.set_block(0x6B, 0x17, b"\x01")
    bus.set_block(0x1E, 0x27, b"\x00")
    bus.set_block(0x6B, 0x28, axes(10, 20, 30))

    first = sensor.read_sample()
    assert first.valid_fields == LSM9DS1.ACCEL_VALID
    assert first.accel_mg == pytest.approx((0.61, 1.22, 1.83))
    assert first.gyro_mrad_s == (0.0, 0.0, 0.0)
    assert first.mag_mgauss == (0.0, 0.0, 0.0)

    bus.set_block(0x6B, 0x17, b"\x00")
    with pytest.raises(SensorNotReady):
        sensor.read_sample()


def test_raises_not_ready_if_all_data_ready_flags_are_clear():
    bus = FakeRegisterBus()
    sensor = initialized_sensor(bus)
    bus.set_block(0x6B, 0x17, b"\x00")
    bus.set_block(0x1E, 0x27, b"\x00")

    with pytest.raises(SensorNotReady):
        sensor.read_sample()


def test_rejects_short_status_or_data_read():
    bus = FakeRegisterBus()
    sensor = initialized_sensor(bus)
    bus.short_reads[(0x6B, 0x17)] = b""
    with pytest.raises(IOError, match="returned 0 bytes; expected 1"):
        sensor.read_sample()

    bus.short_reads.clear()
    bus.set_block(0x6B, 0x17, b"\x01")
    bus.set_block(0x1E, 0x27, b"\x00")
    bus.short_reads[(0x6B, 0x28)] = b"\x00\x00"
    with pytest.raises(IOError, match="returned 2 bytes; expected 6"):
        sensor.read_sample()


def test_configuration_mismatch_is_an_error_and_does_not_mark_initialized():
    bus = FakeRegisterBus()
    bus.ignored_writes.add((0x6B, 0x10))

    with pytest.raises(ConfigurationError, match="register 0x10"):
        LSM9DS1(bus).initialize()


def test_sample_read_requires_successful_initialization():
    with pytest.raises(LSM9DS1Error, match=r"initialize\(\) must succeed"):
        LSM9DS1(FakeRegisterBus()).read_sample()
