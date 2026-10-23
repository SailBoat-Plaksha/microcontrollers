import unittest

from groundstation.cli import parse_step


class CliTests(unittest.TestCase):
    def test_step_accepts_documented_signed_forms_and_defaults_to_sail(self):
        self.assertEqual(parse_step(["step"], 5.0), (5.0, 1))
        self.assertEqual(parse_step(["step", "-5"], 5.0), (-5.0, 1))
        self.assertEqual(parse_step(["step", "+2.5", "2"], 5.0), (2.5, 2))

    def test_step_rejects_invalid_input_before_caller_can_set_pending(self):
        for parts in (["step", "0"], ["step", "400"], ["step", "5", "0"],
                      ["step", "5", "2", "extra"]):
            with self.subTest(parts=parts), self.assertRaises(ValueError):
                parse_step(parts, 5.0)


if __name__ == "__main__":
    unittest.main()
