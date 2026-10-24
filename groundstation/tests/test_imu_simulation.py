import threading
import unittest

from boat.client import BoatClient, ImuReading
from groundstation.transports import SimulationTransport


class SimulationImuTests(unittest.TestCase):
    def test_generated_codec_responses_support_both_request_paths(self):
        transport = SimulationTransport(response_delay=0.001, telemetry_interval=10)
        client = BoatClient(transport)
        received = []
        ready = threading.Event()

        def on_event(event):
            if isinstance(event, ImuReading):
                received.append(event)
                if len(received) == 2:
                    ready.set()

        client.subscribe(on_event)
        try:
            client.connect()
            direct = client.request_imu("direct")
            board = client.request_imu("board")
            self.assertTrue(ready.wait(2), "simulation did not answer both IMU requests")
            received.sort(key=lambda item: item.request_id)
            self.assertEqual([(item.request_id, item.path, item.source) for item in received],
                             [(direct.request_id, 0, (1, 1)), (board.request_id, 1, (1, 159))])
            self.assertTrue(all(item.status == 1 and item.valid_fields == 7 for item in received))
            self.assertEqual(received[1].mag_mgauss, (31, -24, 8))
        finally:
            client.disconnect()


if __name__ == "__main__":
    unittest.main()
