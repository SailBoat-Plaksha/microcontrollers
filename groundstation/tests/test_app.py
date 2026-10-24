import unittest
from unittest.mock import MagicMock, patch

from boat.radio import LoRaModemTransport
from groundstation import app
from groundstation.state import GroundStationState


class AppImportTests(unittest.TestCase):
    def test_invalid_modem_settings_are_reported_without_opening_a_port(self):
        for mode_name in ("LoRa · Pico USB UART0 (AT bridge)", "LoRa · direct USB UART"):
            for settings, expected in ((dict(local_address=70000), "Local modem address"),
                                       (dict(band_hz=-1), "Modem frequency")):
                with self.subTest(mode=mode_name, settings=settings), patch.object(app, "STATE", GroundStationState()):
                    app._call_background(app._connect, mode_name,
                                         "fixture-port", 115200, "", settings).result(timeout=2)
                    self.assertFalse(app.STATE.connected)
                    self.assertIsNone(app.STATE.client)
                    self.assertTrue(app.STATE.detail.startswith(expected), app.STATE.detail)
                    self.assertNotIn("ValueError", app.STATE.detail)

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

    def test_pico_at_factory_creates_transport_with_custom_name(self):
        transport = app.TRANSPORT_FACTORIES["pico_at"](
            "/dev/ttyACM0", 115200, local_address=2, peer_address=1,
            network_id=18, band_hz=None, parameter="10,7,1,12",
            serial_factory=MagicMock()
        )
        self.assertIsInstance(transport, LoRaModemTransport)
        self.assertEqual(transport.name, "LoRa · Pico USB UART0 (AT bridge)")
        self.assertEqual(transport.port, "/dev/ttyACM0")
        self.assertEqual(transport.baudrate, 115200)

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
        original_pico_at = app.TRANSPORT_FACTORIES["pico_at"]
        original_serial = app.TRANSPORT_FACTORIES["serial"]
        calls = []
        try:
            app.TRANSPORT_FACTORIES["direct_radio"] = lambda p, b, **kw: (calls.append(("direct_radio", p, b, kw)) or Transport())
            app.TRANSPORT_FACTORIES["pico_at"] = lambda p, b, **kw: (calls.append(("pico_at", p, b, kw)) or Transport())
            app.TRANSPORT_FACTORIES["serial"] = lambda p, b: (calls.append(("serial", p, b)) or Transport())
            with patch.object(app, "BoatClient", Client):
                # 1. Pico AT bridge
                app._connect("LoRa · Pico USB UART0 (AT bridge)", "/dev/ttyACM0", 115200, "boat-pi",
                             {"local_address": 2, "peer_address": 1, "network_id": 18,
                              "band_hz": None, "parameter": "10,7,1,12"})
                self.assertEqual(calls[-1][0], "pico_at")
                self.assertEqual(calls[-1][1], "/dev/ttyACM0")
                self.assertEqual(calls[-1][3]["peer_address"], 1)

                # 2. Direct USB UART
                app._connect("LoRa · direct USB UART", "/dev/ttyUSB0", 115200, "boat-pi",
                             {"local_address": 2, "peer_address": 1, "network_id": 18,
                              "band_hz": None, "parameter": "10,7,1,12"})
                self.assertEqual(calls[-1][0], "direct_radio")
                self.assertEqual(calls[-1][1], "/dev/ttyUSB0")
                self.assertEqual(calls[-1][3]["peer_address"], 1)

                # 3. Legacy raw Pico USB (serial)
                app._connect("LoRa · ground Pico USB (raw MAVLink)", "/dev/legacy", 115200, "boat-pi")
                self.assertEqual(calls[-1], ("serial", "/dev/legacy", 115200))
        finally:
            app.TRANSPORT_FACTORIES["direct_radio"] = original_direct
            app.TRANSPORT_FACTORIES["pico_at"] = original_pico_at
            app.TRANSPORT_FACTORIES["serial"] = original_serial
            app.STATE.client = app.STATE.transport = app.STATE.unsubscribe = None
            app.STATE.connected = False

    def test_modem_health_check_updates_ground_station_detail(self):
        class FakeState:
            lock = __import__("threading").RLock()
            connected = True
            transport = type("Transport", (), {"name": "LoRa · Pico USB UART0 (AT bridge)",
                "check_health": lambda _self: {"healthy": True, "checks": {},
                    "responses": {"address": "+ADDRESS=2", "network_id": "+NETWORK=18",
                        "band_hz": "+BAND=915000000", "parameter": "+PARAMETER=10,7,1,12",
                        "version": "+VER=RYLR998_V1.0", "uid": "+UID=0123456789AB"}}})()
            detail = ""
            def enqueue(self, event): self.event = event
        state = FakeState()
        with patch.object(app, "STATE", state):
            report = app._check_modem_health()
        self.assertTrue(report["healthy"])
        self.assertIn("LoRa modem health OK", state.detail)
        self.assertIn("915000000", state.detail)


if __name__ == "__main__":
    unittest.main()
