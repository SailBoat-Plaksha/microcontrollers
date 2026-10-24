import unittest
from unittest.mock import patch

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

    def test_direct_radio_factory_receives_explicit_modem_settings(self):
        original = app.TRANSPORT_FACTORIES["direct_radio"]
        captured = {}
        class Transport:
            name = "LoRa · direct USB UART"
            is_connected = False
        def factory(port, baud, **settings):
            captured.update(port=port, baud=baud, **settings)
            return Transport()
        try:
            app.set_transport_factory("direct_radio", factory)
            transport = app.TRANSPORT_FACTORIES["direct_radio"]("/dev/fake", 115200,
                local_address=2, peer_address=1, network_id=18,
                band_hz=None, parameter="10,7,1,12")
            self.assertEqual(transport.name, "LoRa · direct USB UART")
            self.assertEqual(captured, {"port": "/dev/fake", "baud": 115200,
                "local_address": 2, "peer_address": 1, "network_id": 18,
                "band_hz": None, "parameter": "10,7,1,12"})
        finally:
            app.TRANSPORT_FACTORIES["direct_radio"] = original

    def test_connect_keeps_legacy_factory_call_shape_and_direct_settings(self):
        class Transport:
            name = "test transport"
            is_connected = True
            def open(self, _on_frame): pass
            def send_frame(self, _frame): pass
            def close(self): pass
        class Client:
            def __init__(self, transport): self.transport = transport
            def subscribe(self, _callback): return lambda: None
            def connect(self): pass
            def ping_all(self): pass
            def disconnect(self): self.transport.close()
        original_direct = app.TRANSPORT_FACTORIES["direct_radio"]
        original_serial = app.TRANSPORT_FACTORIES["serial"]
        calls = []
        try:
            app.TRANSPORT_FACTORIES["direct_radio"] = lambda p, b, **kw: (calls.append((p, b, kw)) or Transport())
            app.TRANSPORT_FACTORIES["serial"] = lambda p, b: (calls.append((p, b)) or Transport())
            with patch.object(app, "BoatClient", Client):
                app._connect("LoRa · direct USB UART", "/dev/fake", 115200, "boat-pi",
                             {"local_address": 2, "peer_address": 1, "network_id": 18,
                              "band_hz": None, "parameter": "10,7,1,12"})
                self.assertEqual(calls[-1][2]["peer_address"], 1)
                app._connect("LoRa · ground Pico USB (raw MAVLink)", "/dev/legacy", 115200, "boat-pi")
                self.assertEqual(calls[-1], ("/dev/legacy", 115200))
        finally:
            app.TRANSPORT_FACTORIES["direct_radio"] = original_direct
            app.TRANSPORT_FACTORIES["serial"] = original_serial
            app.STATE.client = app.STATE.transport = app.STATE.unsubscribe = None
            app.STATE.connected = False


if __name__ == "__main__":
    unittest.main()
