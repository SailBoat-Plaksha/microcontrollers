"""Host-only checks for the Pi-owned on-demand LSM9DS1 broker."""

from __future__ import annotations

import struct
import tempfile
import unittest
from pathlib import Path

from boat.ids import COMPONENT_GROUND, COMPONENT_SENSOR, SYSTEM_GROUND
from protocol import codec
from onboard import BoatRouter, ServiceConfig, SimulatedBus
from onboard.bus import BusOperation


class Clock:
    def __init__(self):
        self.now = 10.0

    def __call__(self):
        return self.now

    def advance(self, ms):
        self.now += ms / 1000


def lsm_map(*, ag_status=3, mag_status=8):
    return {
        0x6B: {0x0F: 0x68, 0x17: ag_status,
               0x28: struct.pack("<3h", -1000, 500, 2000),
               0x18: struct.pack("<3h", 100, -200, 300)},
        0x1E: {0x0F: 0x3D, 0x27: mag_status,
               0x28: struct.pack("<3h", -1000, 2000, -3000)},
    }


def request(request_id=1, *, session=55, path=0, timeout=1000, max_age=0):
    return codec.encode("IMU_REQUEST", {
        "ground_session": session, "request_id": request_id,
        "timeout_ms": timeout, "max_age_ms": max_age, "path": path,
        "target_system": 1, "target_component": 1,
    }, system=SYSTEM_GROUND, component=COMPONENT_GROUND, sequence=request_id & 0xff)


def dependency(request_id=1, *, session=55):
    return codec.encode("IMU_REQUEST", {
        "ground_session": session, "request_id": request_id,
        "timeout_ms": 1000, "max_age_ms": 0, "path": 0,
        "target_system": 1, "target_component": 1,
    }, system=1, component=COMPONENT_SENSOR)


class SensorBrokerTests(unittest.TestCase):
    def setUp(self):
        self.clock = Clock()
        self.output = []
        self.events = []

    def direct_router(self, *, ag_status=3, mag_status=8):
        cfg = ServiceConfig(comms_enabled=False, lsm9ds1_enabled=True)
        bus = SimulatedBus(present=(0x08,), register_map=lsm_map(ag_status=ag_status, mag_status=mag_status))
        router = BoatRouter(bus, cfg, clock=self.clock, on_frame=self.output.append,
                            on_event=self.events.append)
        return router, bus

    def tick_imu(self, router):
        router._poll_index = router._poll_addresses.index(-1)
        router.step()

    def samples(self):
        return [codec.decode(frame) for frame in self.output
                if codec.decode(frame)["name"] == "IMU_SAMPLE"]

    def test_direct_sample_returns_signed_mg_mrad_s_mgauss_and_partial_mask(self):
        router, bus = self.direct_router(ag_status=1, mag_status=8)
        self.assertTrue(router.submit_frame(request(), ingress="radio"))
        self.tick_imu(router)
        fields = self.samples()[-1]["fields"]
        self.assertEqual((fields["status"], fields["path"], fields["valid_fields"]), (1, 0, 5))
        self.assertEqual((fields["xacc"], fields["yacc"], fields["zacc"]), (-61, 31, 122))
        self.assertEqual((fields["xgyro"], fields["ygyro"], fields["zgyro"]), (0, 0, 0))
        self.assertEqual((fields["xmag"], fields["ymag"], fields["zmag"]), (-140, 280, -420))
        self.assertEqual(fields["acquisition_component"], 1)
        self.assertEqual(fields["target_system"], SYSTEM_GROUND)
        self.assertEqual(fields["target_component"], COMPONENT_GROUND)
        self.assertEqual(fields["acquisition_boot_id"], router.boot_id)
        self.assertEqual(fields["source_boot_id"], router.boot_id)
        self.assertEqual(fields["sample_seq"], 1)
        self.assertTrue(any(op.kind == "imu_write" for op in bus.operations))
        self.assertFalse(any(op.address == 0x09 for op in bus.operations))

    def test_direct_duplicate_replays_cached_result_without_second_acquisition(self):
        router, bus = self.direct_router()
        original = request()
        self.assertTrue(router.submit_frame(original, ingress="radio"))
        self.tick_imu(router)
        before = len([op for op in bus.operations if op.kind in ("imu", "imu_read", "imu_write")])
        self.clock.advance(75)
        self.assertTrue(router.submit_frame(original, ingress="radio"))
        self.assertEqual(len([op for op in bus.operations if op.kind in ("imu", "imu_read", "imu_write")]), before)
        self.assertEqual(self.samples()[-1]["fields"]["sample_seq"], 1)
        self.assertEqual(self.samples()[-1]["fields"]["sample_age_ms"], 75)
        altered = request(timeout=800)
        self.assertTrue(router.submit_frame(altered, ingress="radio"))
        self.assertEqual(self.samples()[-1]["fields"]["status"], 5)
        self.assertEqual(router._imu_samples, 1)

    def test_expired_result_is_not_reacquired_and_request_queue_is_bounded(self):
        cfg = ServiceConfig(comms_enabled=False, lsm9ds1_enabled=True, queue_limit=8)
        bus = SimulatedBus(present=(0x08,), register_map=lsm_map())
        router = BoatRouter(bus, cfg, clock=self.clock, on_frame=self.output.append)
        original = request()
        self.assertTrue(router.submit_frame(original, ingress="radio"))
        self.tick_imu(router)
        reads = len([op for op in bus.operations if op.kind == "imu_read"])
        self.clock.advance(30001)
        self.assertTrue(router.submit_frame(original, ingress="radio"))
        self.assertEqual(self.samples()[-1]["fields"]["status"], 5)
        self.assertEqual(len([op for op in bus.operations if op.kind == "imu_read"]), reads)

        # A separate session can fill the bounded pending table (queue_limit / 2).
        for request_id in range(1, 5):
            self.assertTrue(router.submit_frame(request(request_id, session=77, timeout=5000), ingress="radio"))
        self.assertTrue(router.submit_frame(request(5, session=77, timeout=5000), ingress="radio"))
        self.assertEqual(len(router._imu_pending), 4)
        self.assertEqual(self.samples()[-1]["fields"]["status"], 4)

    def test_direct_not_ready_retries_until_deadline_then_correlated_timeout(self):
        router, _ = self.direct_router(ag_status=0, mag_status=0)
        self.assertTrue(router.submit_frame(request(timeout=40), ingress="radio"))
        self.tick_imu(router)
        self.assertEqual(self.samples(), [])
        self.clock.advance(41)
        router.step()
        sample = self.samples()[-1]["fields"]
        self.assertEqual((sample["status"], sample["valid_fields"], sample["ground_session"], sample["request_id"]),
                         (4, 0, 55, 1))

    def test_short_read_or_missing_sensor_returns_correlated_failure(self):
        router, bus = self.direct_router()
        # Drop the AG identity register so initialization fails repeatedly, then deadline reports failure.
        bus.register_map[0x6B][0x0F] = 0
        self.assertTrue(router.submit_frame(request(timeout=30), ingress="radio"))
        self.tick_imu(router)
        self.clock.advance(31)
        router.step()
        self.assertEqual(self.samples()[-1]["fields"]["status"], 3)
        self.assertEqual(self.samples()[-1]["fields"]["valid_fields"], 0)

    def test_truncated_axis_read_never_emits_partial_fake_values(self):
        class ShortReadBus(SimulatedBus):
            def read_registers(self, address, register, length):
                if address == 0x6B and register == 0x28 and length == 6:
                    self.operations.append(BusOperation("imu_read", address, bytes((register, 1))))
                    return b"\x01"
                return super().read_registers(address, register, length)

        bus = ShortReadBus(present=(0x08,), register_map=lsm_map(ag_status=1, mag_status=0))
        router = BoatRouter(bus, ServiceConfig(comms_enabled=False, lsm9ds1_enabled=True),
                            clock=self.clock, on_frame=self.output.append)
        self.assertTrue(router.submit_frame(request(timeout=25), ingress="radio"))
        self.tick_imu(router)
        self.clock.advance(26)
        router.step()
        fields = self.samples()[-1]["fields"]
        self.assertEqual((fields["status"], fields["valid_fields"], fields["xacc"]), (3, 0, 0))

    def test_board_path_forwards_request_polls_dependency_and_returns_only_matching_sample(self):
        cfg = ServiceConfig(comms_enabled=False, sensor_enabled=True, lsm9ds1_enabled=True)
        bus = SimulatedBus(present=(0x08, 0x0A), register_map=lsm_map())
        router = BoatRouter(bus, cfg, clock=self.clock, on_frame=self.output.append,
                            on_event=self.events.append)
        self.assertTrue(router.submit_frame(request(path=1), ingress="radio"))
        router.step()  # Pi -> sensor Pico IMU_REQUEST
        outbound = next(codec.decode(router._decode_mailbox(op.data)) for op in bus.operations
                        if op.kind == "write" and op.address == 0x0A)
        self.assertEqual((outbound["name"], outbound["system"], outbound["component"],
                          outbound["fields"]["target_component"]), ("IMU_REQUEST", 1, 1, 159))
        bus.inject_frame(0x0A, dependency(1))
        router._poll_index = router._poll_addresses.index(0x0A)
        router.step()  # poll Pico dependency
        parent = router._imu_pending[(255, 190, 55, 1)]
        self.assertEqual((parent.dependency_fields["ground_session"], parent.dependency_request_id), (55, 1))
        router._poll_index = router._poll_addresses.index(-1)
        router.step()  # Pi reads LSM and queues dependency IMU_SAMPLE to Pico
        router.step()  # write dependency result to Pico
        dep_response = next(codec.decode(router._decode_mailbox(op.data)) for op in reversed(bus.operations)
                            if op.kind == "write" and op.address == 0x0A and
                            codec.decode(router._decode_mailbox(op.data))["name"] == "IMU_SAMPLE")
        self.assertEqual((dep_response["fields"]["ground_session"], dep_response["fields"]["request_id"],
                          dep_response["fields"]["target_component"], dep_response["fields"]["path"],
                          dep_response["fields"]["acquisition_component"]), (55, 1, 159, 0, 1))
        returned = codec.encode("IMU_SAMPLE", {
            "ground_session": 55, "request_id": 1, "time_boot_ms": 12, "sample_seq": 1,
            "acquisition_boot_id": router.boot_id, "source_boot_id": 1234, "sample_age_ms": 3,
            "valid_fields": 7, "xacc": -61, "yacc": 31, "zacc": 122,
            "xgyro": 15, "ygyro": -31, "zgyro": 46,
            "xmag": -140, "ymag": 280, "zmag": -420, "status": 1,
            "path": 1, "acquisition_component": 1,
            "target_system": 255, "target_component": 190,
        }, system=1, component=159)
        bus.inject_frame(0x0A, returned)
        router._poll_index = router._poll_addresses.index(0x0A)
        router.step()
        ground_samples = self.samples()
        self.assertEqual(len(ground_samples), 1)
        self.assertEqual((ground_samples[0]["system"], ground_samples[0]["component"]), (1, 159))
        self.assertEqual(ground_samples[0]["fields"]["path"], 1)
        self.assertFalse(any(op.address == 0x09 for op in bus.operations))

    def test_unsolicited_board_sample_rejected_and_missing_pico_times_out(self):
        cfg = ServiceConfig(comms_enabled=False, sensor_enabled=True, lsm9ds1_enabled=True)
        bus = SimulatedBus(present=(0x08,), register_map=lsm_map())
        router = BoatRouter(bus, cfg, clock=self.clock, on_frame=self.output.append,
                            on_event=self.events.append)
        self.assertTrue(router.submit_frame(request(path=1, timeout=25), ingress="radio"))
        forged = codec.encode("IMU_SAMPLE", {
            "ground_session": 55, "request_id": 99, "time_boot_ms": 0, "sample_seq": 1,
            "acquisition_boot_id": router.boot_id, "source_boot_id": 1, "sample_age_ms": 0,
            "valid_fields": 0, "xacc": 0, "yacc": 0, "zacc": 0, "xgyro": 0, "ygyro": 0,
            "zgyro": 0, "xmag": 0, "ymag": 0, "zmag": 0, "status": 3, "path": 1,
            "acquisition_component": 1, "target_system": 255, "target_component": 190,
        }, system=1, component=159)
        router._observe_node_message(codec.decode(forged), forged)
        self.assertEqual(self.samples(), [])
        self.clock.advance(26)
        router.step()
        self.assertEqual(self.samples()[-1]["fields"]["status"], 4)
        self.assertEqual(self.samples()[-1]["fields"]["valid_fields"], 0)
        self.assertEqual((self.samples()[-1]["system"], self.samples()[-1]["component"]), (1, 1))

    def test_board_dependency_requires_exact_correlation_without_single_candidate_guess(self):
        cfg = ServiceConfig(comms_enabled=False, sensor_enabled=True, lsm9ds1_enabled=True)
        bus = SimulatedBus(present=(0x08, 0x0A), register_map=lsm_map())
        router = BoatRouter(bus, cfg, clock=self.clock, on_frame=self.output.append,
                            on_event=self.events.append)
        self.assertTrue(router.submit_frame(request(1, session=55, path=1), ingress="radio"))
        self.assertTrue(router.submit_frame(request(2, session=55, path=1), ingress="radio"))
        bad = dependency(2, session=99)
        self.assertFalse(router._accept_imu_request(bad, codec.decode(bad)["fields"], "sensor"))
        self.assertIsNone(router._imu_pending[(255, 190, 55, 1)].dependency_fields)
        self.assertIsNone(router._imu_pending[(255, 190, 55, 2)].dependency_fields)

        for request_id in (2, 1):
            frame = dependency(request_id)
            self.assertTrue(router._accept_imu_request(frame, codec.decode(frame)["fields"], "sensor"))
        self.assertEqual(router._imu_pending[(255, 190, 55, 1)].dependency_request_id, 1)
        self.assertEqual(router._imu_pending[(255, 190, 55, 2)].dependency_request_id, 2)
        altered = dependency(1)
        altered_fields = dict(codec.decode(altered)["fields"])
        altered_fields["timeout_ms"] = 900
        altered = codec.encode("IMU_REQUEST", altered_fields, system=1, component=159)
        self.assertFalse(router._accept_imu_request(altered, codec.decode(altered)["fields"], "sensor"))

    def test_session_zero_local_request_works_without_ground_parent_and_stays_sensor_only(self):
        cfg = ServiceConfig(comms_enabled=False, sensor_enabled=True, lsm9ds1_enabled=True)
        bus = SimulatedBus(present=(0x08, 0x0A), register_map=lsm_map())
        router = BoatRouter(bus, cfg, clock=self.clock, on_frame=self.output.append,
                            on_event=self.events.append)
        frame = dependency(1, session=0)
        self.assertTrue(router._accept_imu_request(frame, codec.decode(frame)["fields"], "sensor"))
        router._poll_index = router._poll_addresses.index(-1)
        router.step()
        router.step()
        response = next(codec.decode(router._decode_mailbox(op.data)) for op in reversed(bus.operations)
                        if op.kind == "write" and op.address == 0x0A and
                        codec.decode(router._decode_mailbox(op.data))["name"] == "IMU_SAMPLE")
        self.assertEqual((response["system"], response["component"]), (1, 1))
        self.assertEqual((response["fields"]["ground_session"], response["fields"]["request_id"],
                          response["fields"]["path"], response["fields"]["target_component"],
                          response["fields"]["status"]), (0, 1, 0, 159, 1))
        self.assertEqual(self.samples(), [])
        self.assertFalse(router._imu_pending)

    def test_session_zero_local_request_rejects_reorder_and_altered_duplicate(self):
        cfg = ServiceConfig(comms_enabled=False, sensor_enabled=True, lsm9ds1_enabled=True)
        bus = SimulatedBus(present=(0x08, 0x0A), register_map=lsm_map(ag_status=0, mag_status=0))
        router = BoatRouter(bus, cfg, clock=self.clock, on_frame=self.output.append,
                            on_event=self.events.append)
        first = dependency(2, session=0)
        self.assertTrue(router._accept_imu_request(first, codec.decode(first)["fields"], "sensor"))
        self.assertTrue(router._accept_imu_request(first, codec.decode(first)["fields"], "sensor"))
        changed = dict(codec.decode(first)["fields"])
        changed["timeout_ms"] = 900
        changed_frame = codec.encode("IMU_REQUEST", changed, system=1, component=159)
        self.assertTrue(router._accept_imu_request(changed_frame, changed, "sensor"))
        self.assertEqual(len(router._sensor_local_pending), 1)
        old = dependency(1, session=0)
        self.assertFalse(router._accept_imu_request(old, codec.decode(old)["fields"], "sensor"))

    def test_missing_sensor_mailbox_does_not_block_motor_poll(self):
        cfg = ServiceConfig(comms_enabled=False, sensor_enabled=True, lsm9ds1_enabled=True)
        bus = SimulatedBus(present=(0x08,), register_map=lsm_map())
        router = BoatRouter(bus, cfg, clock=self.clock, on_frame=self.output.append)
        router._poll_index = router._poll_addresses.index(0x0A)
        self.assertFalse(router.step())  # missing Pico returns one bus fault
        router.step()  # LSM job slot is idle
        router.step()  # round-robin reaches motor mailbox
        self.assertTrue(any(op.kind == "read" and op.address == 0x08 for op in bus.operations))

    def test_config_strictly_enables_sensor_and_rejects_generic_conflict(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "onboard.toml"
            path.write_text("[lsm9ds1]\nenabled=true\nag_address=106\nmag_address=28\n[sensor]\nenabled=true\naddress=10\ncomponent=159\n")
            cfg = ServiceConfig.from_toml(path)
            self.assertTrue(cfg.lsm9ds1_enabled)
            self.assertTrue(cfg.sensor_enabled)
            path.write_text("[lsm9ds1]\nenabled=true\n[imu]\nenabled=true\naddress=104\nregister=59\naccel_scale_mg_per_count=1.0\ngyro_scale_mrad_s_per_count=1.0\n")
            with self.assertRaisesRegex(ValueError, "mutually exclusive"):
                ServiceConfig.from_toml(path)
            path.write_text("[sensor]\nenabled=1\n")
            with self.assertRaisesRegex(ValueError, "true or false"):
                ServiceConfig.from_toml(path)


if __name__ == "__main__":
    unittest.main()
