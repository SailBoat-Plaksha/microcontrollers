"""Hardware-independent raw MAVLink <-> RYLR998 gateway state machine."""

import ubinascii

import config
import dialect
from mavlink_stream import FrameStream, validate_frame


class Gateway:
    # Modem response deadlines are configurable in config.py. Timeout means
    # delivery is ambiguous; movement is never replayed automatically.
    def __init__(self, uart, usb_out):
        self.uart = uart
        self.usb_out = usb_out
        self.usb_parser = FrameStream()
        self.modem_line = bytearray()
        self.radio_queue = []
        self.usb_queue = []
        self.response_queue = []
        self.pending_usb_offset = 0
        self.pending_usb_kind = None
        self.config_index = 0
        self.modem_state = "config_tx"
        self.modem_deadline = 0
        self.next_send_at = 0
        self.uart_pending = (self.configs[0] + "\r\n").encode()
        self.uart_offset = 0
        self.inflight = None
        self.reservations = []
        self.sent_reservations = []
        self.usb_overflow = 0
        self.radio_overflow = 0
        self.modem_errors = 0
        self.ambiguous_sends = 0
        self.local_modem_accepted = 0

    @property
    def configs(self):
        return (
            "AT+ADDRESS=%d" % config.RADIO_ADDRESS,
            "AT+NETWORKID=%d" % config.RADIO_NETWORK_ID,
            "AT+BAND=%d" % config.RADIO_BAND_HZ,
            "AT+PARAMETER=%s" % config.RADIO_PARAMETER,
        )

    @property
    def waiting_config(self):
        return self.modem_state in ("config_tx", "config_wait")

    @property
    def modem_ready(self):
        return self.modem_state == "ready"

    def status(self):
        """Return bounded local diagnostics; modem acceptance is not delivery."""
        return {"modem_state": self.modem_state,
                "radio_queued": len(self.radio_queue),
                "usb_queued": len(self.usb_queue),
                "responses_queued": len(self.response_queue),
                "response_reservations": len(self.reservations),
                "usb_overflow": self.usb_overflow,
                "radio_overflow": self.radio_overflow,
                "modem_errors": self.modem_errors,
                "ambiguous_sends": self.ambiguous_sends,
                "modem_accepted": self.local_modem_accepted}

    def reconnect(self):
        """Explicitly restart modem configuration; never replay queued motion."""
        self.radio_overflow += len(self.radio_queue)
        for queued in self.radio_queue:
            key = queued[2]
            if key is not None and key not in self.sent_reservations:
                self._release_reservation(key)
        self.radio_queue = []
        if self.inflight is not None and self.inflight[2] is not None:
            key = self.inflight[2]
            if key in self.reservations and key not in self.sent_reservations:
                self.sent_reservations.append(key)
        self.inflight = None
        self.config_index = 0
        self.modem_state = "config_tx"
        self._set_uart_pending((self.configs[0] + "\r\n").encode())

    def _set_uart_pending(self, data):
        self.uart_pending = data
        self.uart_offset = 0

    @staticmethod
    def _deadline_after(now_ms, delay_ms):
        return (now_ms + delay_ms) & 0x3fffffff

    @staticmethod
    def _deadline_reached(now_ms, deadline):
        return ((now_ms - deadline) & 0x3fffffff) < 0x20000000

    def _queue_key(self, frame):
        msgid = frame[7] | (frame[8] << 8) | (frame[9] << 16)
        if msgid == dialect.MSG_REQUEST_RESULT and len(frame) >= 30:
            return (msgid, frame[10:18])  # ground_session and request_id
        if msgid == dialect.MSG_NODE_HEALTH and len(frame) >= 22:
            return (msgid, frame[10:14])  # latest snapshot for this boot
        if msgid == dialect.MSG_LINK_STATUS:
            return (msgid, frame[5], frame[6])
        return None

    def _queue_usb(self, frame):
        key = self._queue_key(frame)
        target_queue = self.response_queue if key and key[0] == dialect.MSG_REQUEST_RESULT else self.usb_queue
        if key is not None:
            for index, queued in enumerate(target_queue):
                if self._queue_key(queued) == key:
                    sending_this_queue = (
                        (target_queue is self.response_queue and self.pending_usb_kind == "response") or
                        (target_queue is self.usb_queue and self.pending_usb_kind == "regular"))
                    if index == 0 and self.pending_usb_offset and sending_this_queue:
                        continue
                    target_queue[index] = frame
                    return True
        limit = (config.MAX_RESPONSE_RESERVATIONS if target_queue is self.response_queue
                 else config.MAX_USB_QUEUE)
        response_capacity_full = (
            target_queue is self.response_queue and
            len(self.response_queue) + len(self.reservations) >= config.MAX_RESPONSE_RESERVATIONS)
        if len(target_queue) >= limit or response_capacity_full:
            self.usb_overflow += 1
            return False
        target_queue.append(frame)
        if key and key[0] == dialect.MSG_REQUEST_RESULT:
            self._release_reservation(frame[10:18])
        return True

    def _release_reservation(self, key):
        for index, item in enumerate(self.reservations):
            if item == key:
                self.reservations.pop(index)
                break
        if key in self.sent_reservations:
            self.sent_reservations.remove(key)

    def feed_usb(self, data):
        accepted = 0
        for frame in self.usb_parser.feed(data):
            if frame[5] != dialect.SYSTEM_GROUND or frame[6] != dialect.COMPONENT_GROUND:
                continue
            payload = ubinascii.b2a_base64(frame).strip()
            msgid = frame[7] | (frame[8] << 8) | (frame[9] << 16)
            is_request = (msgid == dialect.MSG_ACTUATOR_REQUEST and
                          len(frame) > dialect.ACTUATOR_REQUEST_OPCODE_FRAME_OFFSET)
            is_stop = is_request and frame[dialect.ACTUATOR_REQUEST_OPCODE_FRAME_OFFSET] == 3
            is_motion = is_request and frame[dialect.ACTUATOR_REQUEST_OPCODE_FRAME_OFFSET] in (1, 2)
            if len(payload) > 240:
                continue
            if is_motion and (len(self.reservations) + len(self.response_queue) >=
                              config.MAX_RESPONSE_RESERVATIONS):
                self.radio_overflow += 1
                continue
            if len(self.radio_queue) >= config.MAX_RADIO_QUEUE:
                if not is_stop:
                    self.radio_overflow += 1
                    continue
                victim = -1
                for index in range(len(self.radio_queue) - 1, -1, -1):
                    if self.radio_queue[index][0] > 0:
                        victim = index
                        break
                if victim < 0:
                    self.radio_overflow += 1
                    continue
                dropped = self.radio_queue.pop(victim)
                if dropped[2] is not None:
                    self._release_reservation(dropped[2])
                self.radio_overflow += 1
            command = ("AT+SEND=%d,%d," %
                       (config.BOAT_RADIO_ADDRESS, len(payload))).encode() + payload + b"\r\n"
            correlation = frame[10:18] if is_motion else None
            item = (0 if is_stop else 1, command, correlation)
            if is_stop:
                self.radio_queue.insert(0, item)
            else:
                self.radio_queue.append(item)
            if correlation is not None and correlation not in self.reservations:
                self.reservations.append(correlation)
            accepted += 1
        return accepted

    def _radio_frame(self, line):
        if not line.startswith(b"+RCV="):
            return
        fields = line[5:].split(b",")
        if len(fields) != 5:
            return
        try:
            if int(fields[0]) != config.BOAT_RADIO_ADDRESS:
                return
            declared = int(fields[1])
            payload = fields[2]
            if declared < 1 or declared > 240 or len(payload) != declared:
                return
            raw = ubinascii.a2b_base64(payload)
        except (ValueError, TypeError):
            return
        if validate_frame(raw):
            self._queue_usb(raw)

    def _handle_line(self, line, now_ms):
        is_error = line.startswith(b"+ERR") or line == b"ERROR"
        if self.modem_state == "config_wait":
            if line in (b"+OK", b"OK"):
                self.config_index += 1
                if self.config_index == len(self.configs):
                    self.modem_state = "ready"
                else:
                    self.modem_state = "config_tx"
                    self._set_uart_pending((self.configs[self.config_index] + "\r\n").encode())
                return
            if is_error:
                self.modem_errors += 1
                self.modem_state = "config_failed"
                return
        elif self.modem_state == "send_wait" and self.inflight is not None:
            if line == b"+OK" or line == b"OK":
                self.local_modem_accepted += 1
                key = self.inflight[2]
                if key is not None and key in self.reservations and key not in self.sent_reservations:
                    self.sent_reservations.append(key)
                self.next_send_at = self._deadline_after(now_ms, config.SEND_GAP_MS)
                self.inflight = None
                self.modem_state = "ready"
                return
            if is_error:
                self.modem_errors += 1
                if self.inflight[2] is not None:
                    self._release_reservation(self.inflight[2])
                self.next_send_at = self._deadline_after(now_ms, config.SEND_GAP_MS)
                self.inflight = None
                self.modem_state = "ready"
                return
        self._radio_frame(line)

    def feed_modem(self, data, now_ms=0):
        for byte in data:
            if byte in (10, 13):
                if self.modem_line:
                    line = bytes(self.modem_line)
                    self.modem_line = bytearray()
                    self._handle_line(line, now_ms)
            elif len(self.modem_line) < config.MAX_MODEM_LINE - 1:
                self.modem_line.append(byte)
            else:
                self.modem_line = bytearray()
                self.modem_errors += 1

    def _write_pending(self):
        if self.uart_pending is None:
            return False
        remaining = self.uart_pending[self.uart_offset:]
        try:
            written = self.uart.write(remaining)
        except (OSError, ValueError):
            written = 0
        if written is None or written <= 0:
            return False
        if written > len(remaining):
            written = len(remaining)
        self.uart_offset += written
        if self.uart_offset < len(self.uart_pending):
            return False
        self.uart_pending = None
        return True

    def tick(self, now_ms=0):
        if self.pending_usb_offset and self.pending_usb_kind == "regular":
            active_usb_queue = self.usb_queue
        else:
            active_usb_queue = self.response_queue if self.response_queue else self.usb_queue
        if self.pending_usb_offset == 0:
            self.pending_usb_kind = "response" if active_usb_queue is self.response_queue else "regular"
        if active_usb_queue:
            pending = active_usb_queue[0]
            try:
                written = self.usb_out.write(pending[self.pending_usb_offset:])
            except (OSError, ValueError):
                written = 0
            if written is not None and written > 0:
                remaining = len(pending) - self.pending_usb_offset
                self.pending_usb_offset += min(written, remaining)
                if self.pending_usb_offset >= len(pending):
                    active_usb_queue.pop(0)
                    self.pending_usb_offset = 0
                    self.pending_usb_kind = None

        if self.uart_pending is not None:
            done = self._write_pending()
            if done:
                if self.modem_state == "config_tx":
                    self.modem_state = "config_wait"
                    self.modem_deadline = self._deadline_after(now_ms, config.CONFIG_TIMEOUT_MS)
                elif self.modem_state == "send_tx":
                    self.modem_state = "send_wait"
                    self.modem_deadline = self._deadline_after(now_ms, config.SEND_TIMEOUT_MS)
            return

        if self.modem_state in ("config_wait", "send_wait"):
            if self._deadline_reached(now_ms, self.modem_deadline):
                self.modem_errors += 1
                if self.modem_state == "send_wait":
                    self.ambiguous_sends += 1
                    if (self.inflight is not None and self.inflight[2] is not None and
                            self.inflight[2] in self.reservations and
                            self.inflight[2] not in self.sent_reservations):
                        self.sent_reservations.append(self.inflight[2])
                    self.inflight = None
                self.modem_state = "config_failed" if self.config_index < len(self.configs) else "send_uncertain"
            return

        if (self.modem_state == "ready" and self.radio_queue and
                self._deadline_reached(now_ms, self.next_send_at)):
            self.inflight = self.radio_queue.pop(0)
            self._set_uart_pending(self.inflight[1])
            self.modem_state = "send_tx"
