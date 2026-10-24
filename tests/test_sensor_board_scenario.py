"""Focused production-path assertions for the compiled sensor scenario."""

import unittest

from scenarios.sensor_board import (
    run_sensor_direct_and_cache,
    run_sensor_error_and_partial,
    run_sensor_local_and_wrong_correlation,
    run_sensor_stop_priority,
)


class SensorBoardScenarioTests(unittest.TestCase):
    def test_direct_route_uses_production_driver_without_pico_and_deduplicates(self):
        evidence = run_sensor_direct_and_cache()["direct"]
        self.assertEqual(evidence["reading"]["valid_fields"], 7)
        self.assertEqual(evidence["reading"]["mag_mgauss"], (-140, 280, -420))
        self.assertEqual(evidence["mailbox_sensor_transactions"], [])
        self.assertGreaterEqual(evidence["duplicate_replay"]["fields"]["sample_age_ms"], 137)

    def test_session_zero_local_result_stays_pi_local_and_wrong_id_is_rejected(self):
        evidence = run_sensor_local_and_wrong_correlation()
        self.assertEqual(evidence["local_dependency"]["fields"]["ground_session"], 0)
        self.assertEqual(evidence["pi_target_reply"]["fields"]["target_system"], 1)
        self.assertEqual(evidence["pi_target_reply"]["fields"]["target_component"], 1)

    def test_partial_valid_and_identity_error_are_real_driver_results(self):
        evidence = run_sensor_error_and_partial()
        self.assertEqual(evidence["partial"]["valid_fields"], 1)
        self.assertEqual(evidence["identity_error_wire_frame"]["fields"]["status"], 3)

    def test_stop_reaches_motor_before_sensor_read(self):
        evidence = run_sensor_stop_priority()
        self.assertEqual(evidence["motor_stop_write"]["address"], 0x08)
        self.assertEqual(evidence["decoded"]["fields"]["opcode"], 3)
        self.assertEqual(evidence["sensor_register_reads"], [])


if __name__ == "__main__":
    unittest.main()
