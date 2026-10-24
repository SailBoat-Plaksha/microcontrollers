import unittest
from unittest.mock import patch

from groundstation import cli
from groundstation.cli import parse_imu, parse_step


class CliTests(unittest.TestCase):
    def test_imu_command_defaults_and_selects_only_supported_paths(self):
        self.assertEqual(parse_imu(["imu"]), "direct")
        self.assertEqual(parse_imu(["IMU", "BOARD"]), "board")
        for parts in (["imu", "radio"], ["imu", "direct", "extra"], ["read"]):
            with self.subTest(parts=parts), self.assertRaises(ValueError):
                parse_imu(parts)

    def test_step_accepts_documented_signed_forms_and_defaults_to_sail(self):
        self.assertEqual(parse_step(["step"], 5.0), (5.0, 1))
        self.assertEqual(parse_step(["step", "-5"], 5.0), (-5.0, 1))
        self.assertEqual(parse_step(["step", "+2.5", "2"], 5.0), (2.5, 2))

    def test_step_rejects_invalid_input_before_caller_can_set_pending(self):
        for parts in (["step", "0"], ["step", "400"], ["step", "5", "0"],
                      ["step", "5", "2", "extra"]):
            with self.subTest(parts=parts), self.assertRaises(ValueError):
                parse_step(parts, 5.0)

    def test_lora_cli_selects_direct_modem_and_passes_explicit_settings(self):
        args = cli.build_parser().parse_args(["--lora", "/dev/ttyUSB0", "--baud", "57600",
            "--local-address", "7", "--peer-address", "9", "--network-id", "18",
            "--parameter", "9,7,1,12", "--band-hz", "915000000"])
        sentinel = object()
        with patch.object(cli, "LoRaModemTransport", return_value=sentinel) as constructor:
            self.assertIs(cli.make_transport(args), sentinel)
        constructor.assert_called_once_with("/dev/ttyUSB0", 57600, local_address=7,
            peer_address=9, network_id=18, band_hz=915000000, parameter="9,7,1,12")


if __name__ == "__main__":
    unittest.main()
