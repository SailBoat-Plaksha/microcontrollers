import unittest

from groundstation import app


class AppImportTests(unittest.TestCase):
    def test_import_is_headless_and_transport_factory_is_replaceable(self):
        original = app.TRANSPORT_FACTORIES["simulation"]
        replacement = lambda port, baud: (port, baud)
        try:
            app.set_transport_factory("simulation", replacement)
            self.assertIs(app.TRANSPORT_FACTORIES["simulation"], replacement)
        finally:
            app.set_transport_factory("simulation", original)


if __name__ == "__main__":
    unittest.main()
