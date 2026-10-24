import base64
import threading
import time
import unittest

from boat.radio import LoRaModemTransport
from protocol import codec


class FakeSerial:
    def __init__(self, *args, ack=True, partial=0, fail_write_after=None,
                 fail_prefix=None, **kwargs):
        self.args, self.kwargs = args, kwargs
        self.ack, self.partial, self.fail_write_after = ack, partial, fail_write_after
        self.fail_prefix = fail_prefix
        self.rx = bytearray()
        self.cv = threading.Condition()
        self.wire = bytearray()
        self.calls = []
        self.actual_writes = []
        self.closed = False
        self.hold_acks = False
        self.pending_acks = 0

    def read(self, count):
        with self.cv:
            if not self.rx and not self.closed:
                self.cv.wait(0.05)
            data = bytes(self.rx[:count])
            del self.rx[:count]
            return data

    def write(self, data):
        data = bytes(data)
        self.calls.append(data)
        if self.fail_write_after is not None and len(self.wire) >= self.fail_write_after:
            raise OSError("injected partial write failure")
        if self.fail_prefix and bytes(self.wire).startswith(self.fail_prefix) and self.wire:
            raise OSError("injected partial write failure")
        count = min(len(data), self.partial) if self.partial else len(data)
        actual = data[:count]
        self.actual_writes.append(actual)
        self.wire.extend(actual)
        if b"\n" in self.wire:
            while b"\n" in self.wire:
                _, rest = self.wire.split(b"\n", 1)
                self.wire = bytearray(rest)
                if self.ack:
                    if self.hold_acks:
                        self.pending_acks += 1
                    else:
                        self.inject(b"+OK\r\n")
        return count

    def flush(self):
        pass

    def inject(self, data, fragments=()):
        with self.cv:
            if fragments:
                for size in fragments:
                    self.rx.extend(data[:size])
                    data = data[size:]
                self.rx.extend(data)
            else:
                self.rx.extend(data)
            self.cv.notify_all()

    def release_ack(self):
        with self.cv:
            self.pending_acks -= 1
            self.rx.extend(b"+OK\r\n")
            self.cv.notify_all()

    def close(self):
        with self.cv:
            self.closed = True
            self.cv.notify_all()


def frame(name="PING", **fields):
    defaults = {"time_usec": 1, "seq": 2, "target_system": 0, "target_component": 0}
    defaults.update(fields)
    return codec.encode(name, defaults, system=255, component=190)


class RadioTransportTests(unittest.TestCase):
    def make(self, **serial_options):
        instances = []
        def factory(*args, **kwargs):
            serial_obj = FakeSerial(*args, **kwargs, **serial_options)
            instances.append(serial_obj)
            return serial_obj
        transport = LoRaModemTransport("fake", serial_factory=factory,
                                       command_timeout_s=.15)
        return transport, instances

    def test_config_send_and_fragmented_receive_use_codec(self):
        transport, serials = self.make(partial=7)
        received = []
        transport.open(received.append)
        modem = serials[0]
        self.assertTrue(transport.is_connected)
        config = bytes(modem.calls[0])
        self.assertEqual(config, b"AT+ADDRESS=2\r\n")
        self.assertNotIn(b"AT+BAND=", b"".join(modem.calls))
        outgoing = frame()
        transport.send_frame(outgoing)
        self.assertTrue(any(call.startswith(b"AT+SEND=") for call in modem.calls))
        self.assertEqual(transport.stats["tx_accepted"], 1)
        payload = base64.b64encode(outgoing)
        line = b"+RCV=1, " + str(len(payload)).encode() + b", " + payload + b", -80, 9\r\n"
        modem.inject(line, fragments=(2, 4, 7, 1))
        deadline = time.monotonic() + 1
        while not received and time.monotonic() < deadline:
            time.sleep(.005)
        self.assertEqual(received, [outgoing])
        self.assertEqual(transport.stats["rx_frames"], 1)
        # Partial writes advance by offset; no command is restarted at byte zero.
        self.assertTrue(all(len(call) <= 7 for call in modem.actual_writes))
        transport.close()
        self.assertFalse(transport.is_connected)

    def test_ready_requires_each_config_ack_and_err_or_timeout_closes_cleanly(self):
        transport, serials = self.make()
        transport.open(lambda _frame: None)
        self.assertEqual(transport.stats["config_commands"], 3)
        transport.close()

        failed, bad_serials = self.make(ack=False)
        failed.command_timeout_s = .03
        with self.assertRaises(TimeoutError):
            failed.open(lambda _frame: None)
        self.assertFalse(failed.is_connected)
        self.assertTrue(bad_serials[0].closed)
        self.assertIsNone(failed._serial)
        self.assertIsNone(failed._reader)

        rejected, err_serials = self.make()
        original = err_serials
        # Inject an ERR instead of accepting the first command.
        def reject_factory(*args, **kwargs):
            obj = FakeSerial(*args, **kwargs, ack=False)
            original.append(obj)
            def reject_write(data):
                obj.calls.append(bytes(data)); obj.wire.extend(data)
                obj.inject(b"+ERR=4\r\n")
                return len(data)
            obj.write = reject_write
            return obj
        rejected._serial_factory = reject_factory
        with self.assertRaisesRegex(OSError, "rejected"):
            rejected.open(lambda _frame: None)
        self.assertFalse(rejected.is_connected)

    def test_optional_band_is_configured_and_old_session_callback_is_not_reused(self):
        instances = []
        def factory(*args, **kwargs):
            serial_obj = FakeSerial(*args, **kwargs)
            instances.append(serial_obj)
            return serial_obj
        transport = LoRaModemTransport("fake", serial_factory=factory, band_hz=915000000,
                                       command_timeout_s=.15)
        old, new = [], []
        transport.open(old.append)
        self.assertEqual(b"".join(instances[0].calls),
                         b"AT+ADDRESS=2\r\nAT+NETWORKID=18\r\nAT+BAND=915000000\r\nAT+PARAMETER=10,7,1,12\r\n")
        transport.close()
        transport.open(new.append)
        payload = base64.b64encode(frame())
        line = b"+RCV=1," + str(len(payload)).encode() + b"," + payload + b",-80,9\r\n"
        instances[0].inject(line)
        instances[1].inject(line)
        deadline = time.monotonic() + 1
        while not new and time.monotonic() < deadline:
            time.sleep(.005)
        self.assertEqual(old, [])
        self.assertEqual(new, [frame()])
        transport.close()

    def test_wrong_peer_malformed_crc_and_signed_frames_are_suppressed(self):
        transport, serials = self.make()
        received = []
        transport.open(received.append)
        modem, good = serials[0], frame()
        payload = base64.b64encode(good)
        modem.inject(b"+RCV=9," + str(len(payload)).encode() + b"," + payload + b",-80,9\r\n")
        bad = bytearray(good); bad[-1] ^= 0x80
        signed = bytearray(good); signed[2] |= 1
        for candidate in (bytes(bad), bytes(signed)):
            text = base64.b64encode(candidate)
            modem.inject(b"+RCV=1," + str(len(text)).encode() + b"," + text + b",-80,9\r\n")
        time.sleep(.05)
        self.assertEqual(received, [])
        self.assertGreaterEqual(transport.stats["rx_rejected"], 3)
        transport.close()

    def test_ambiguous_partial_write_fails_once_without_full_frame_retry(self):
        transport, serials = self.make(partial=6, fail_prefix=b"AT+SEND=")
        transport.open(lambda _frame: None)
        outgoing = frame()
        with self.assertRaises((OSError, ConnectionError)):
            transport.send_frame(outgoing)
        modem = serials[0]
        self.assertFalse(transport.is_connected)
        self.assertEqual(sum(call.startswith(b"AT+SEND=") for call in modem.calls), 1)
        self.assertGreater(len(modem.calls), 1)  # config partial writes occurred
        transport.close()

    def test_stop_jumps_a_queued_move_and_close_cancels_waiters(self):
        transport, serials = self.make()
        transport.open(lambda _frame: None)
        modem = serials[0]
        modem.hold_acks = True
        first = frame()
        errors = []
        sender = threading.Thread(target=lambda: self._capture(errors, transport.send_frame, first))
        sender.start()
        self.assertTrue(self._wait_wire(modem, b"AT+SEND=1,"))
        def request(request_id, opcode, delta):
            return codec.encode("ACTUATOR_REQUEST", {"request_id": request_id,
                "ground_session": 9, "delta_cdeg": delta, "timeout_ms": 0,
                "max_rate_cdeg_s": 0, "target_system": 1, "target_component": 154,
                "actuator": 1, "opcode": opcode, "control_source": 3})
        move = request(1, 1, 1)
        stop = request(2, 3, 0)
        queued = []
        move_thread = threading.Thread(target=lambda: self._capture(queued, transport.send_frame, move))
        move_thread.start()
        time.sleep(.02)
        stop_thread = threading.Thread(target=lambda: self._capture(queued, transport.send_frame, stop))
        stop_thread.start()
        time.sleep(.02)
        modem.release_ack()
        self.assertTrue(self._wait_wire(modem, b"AT+SEND=1," , count=2))
        modem.release_ack()
        self.assertTrue(self._wait_wire(modem, b"AT+SEND=1,", count=3))
        sent = [bytes(call) for call in modem.calls if call.startswith(b"AT+SEND=")]
        self.assertEqual(sent[1], codec.radio_command(stop, address=1))
        modem.release_ack()
        for thread in (sender, move_thread, stop_thread):
            thread.join(1)
        self.assertEqual(errors, [])
        transport.close()

        other, _ = self.make()
        other.open(lambda _frame: None)
        other.close()
        with self.assertRaises(ConnectionError):
            other.send_frame(first)

    @staticmethod
    def _capture(errors, fn, *args):
        try:
            fn(*args)
        except BaseException as exc:
            errors.append(exc)

    @staticmethod
    def _wait_wire(modem, prefix, count=1):
        deadline = time.monotonic() + 1
        while time.monotonic() < deadline:
            sends = [line for line in bytes(modem.wire).splitlines() if line.startswith(prefix)]
            # Completed commands are removed from wire, so use recorded call bytes too.
            sends = [call for call in modem.calls if call.startswith(prefix)]
            if len(sends) >= count:
                return True
            time.sleep(.005)
        return False


if __name__ == "__main__":
    unittest.main()
