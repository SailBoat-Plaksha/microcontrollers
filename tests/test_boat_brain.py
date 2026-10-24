import unittest
from unittest.mock import patch

from onboard import boat_brain


class BoatBrainEntryPointTests(unittest.TestCase):
    def test_named_entrypoint_selects_existing_direct_radio_service(self):
        with patch.object(boat_brain, "_onboard_main", return_value=0) as run:
            result = boat_brain.main(["--port", "/dev/serial0", "--config", "/etc/arsem/onboard.toml"])
        self.assertEqual(result, 0)
        run.assert_called_once_with(["radio", "--port", "/dev/serial0",
                                    "--config", "/etc/arsem/onboard.toml"])


if __name__ == "__main__":
    unittest.main()
