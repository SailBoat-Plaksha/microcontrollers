import unittest
from unittest.mock import Mock, patch

from onboard import boat_brain
from onboard import __main__ as onboard_main


class BoatBrainEntryPointTests(unittest.TestCase):
    def test_default_brain_routes_modem_to_pi_uart1(self):
        with patch.object(onboard_main, "_run", return_value=0) as run:
            self.assertEqual(boat_brain.main([]), 0)
        args = run.call_args.args[0]
        factory = Mock()
        onboard_main._make_radio_transport(args, factory)
        self.assertEqual(args.command, "radio")
        factory.assert_called_once_with("/dev/ttyS0", baudrate=115200,
            local_address=1, peer_address=2, network_id=18,
            band_hz=None, parameter="10,7,1,12")

    def test_named_entrypoint_selects_existing_direct_radio_service(self):
        with patch.object(boat_brain, "_onboard_main", return_value=0) as run:
            result = boat_brain.main(["--port", "/dev/ttyUSB0", "--config", "/etc/arsem/onboard.toml"])
        self.assertEqual(result, 0)
        run.assert_called_once_with(["radio", "--port", "/dev/ttyUSB0",
                                    "--config", "/etc/arsem/onboard.toml"])


if __name__ == "__main__":
    unittest.main()
