"""Hardware-independent gateway state machine used by main.py and host tests."""

import ubinascii

import config
import dialect
from mavlink_stream import FrameStream, validate_frame


class Gateway:
    def __init__(self, uart, usb_out):
        self.uart = uart
        self.usb_out = usb_out
        self.usb_parser = FrameStream()
        self.modem_line = bytearray()
        self.radio_queue = []
        self.usb_queue = []
        self.pending_usb_offset = 0
        self.pending_usb_offset = 0
        self.configs = (
            "AT+ADDRESS=%d" % config.RADIO_ADDRESS,
            "AT+NETWORKID=%d" % config.RADIO_NETWORK_ID,
            "AT+BAND=%d" % config.RADIO_BAND_HZ,
            "AT+PARAMETER=%s" % config.RADIO_PARAMETER,
        )
        self.config_index = 0
        self.waiting_config = True
        self.uart.write((self.configs[0] + "\r\n").encode())

    def feed_usb(self, data):
        accepted = 0
        for frame in self.usb_parser.feed(data):
            if frame[5] != dialect.SYSTEM_GROUND or frame[6] != dialect.COMPONENT_GROUND:
                continue
            payload = ubinascii.b2a_base64(frame).strip()
            msgid = frame[7] | (frame[8] << 8) | (frame[9] << 16)
            is_stop = (msgid == dialect.MSG_ACTUATOR_REQUEST and
                       len(frame) > dialect.ACTUATOR_REQUEST_OPCODE_FRAME_OFFSET and
                       frame[dialect.ACTUATOR_REQUEST_OPCODE_FRAME_OFFSET] == 3)
            if len(payload) > 240:
                continue
            if len(self.radio_queue) >= config.MAX_RADIO_QUEUE:
                if not is_stop:
                    continue
                victim = -1
                for index in range(len(self.radio_queue) - 1, -1, -1):
                    if self.radio_queue[index][0] > 0:
                        victim = index
                        break
                if victim < 0:
                    continue
                self.radio_queue.pop(victim)
            command = ("AT+SEND=%d,%d," %
                       (config.BOAT_RADIO_ADDRESS, len(payload))).encode()
            item = (0 if is_stop else 1, command + payload + b"\r\n")
            if is_stop:
                self.radio_queue.insert(0, item)
            else:
                self.radio_queue.append(item)
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
            if len(self.usb_queue) < config.MAX_RADIO_QUEUE:
                self.usb_queue.append(raw)

    def feed_modem(self, data):
        for byte in data:
            if byte in (10, 13):
                if not self.modem_line:
                    continue
                line = bytes(self.modem_line)
                self.modem_line = bytearray()
                if self.waiting_config and line in (b"+OK", b"OK"):
                    self.config_index += 1
                    if self.config_index < len(self.configs):
                        command = self.configs[self.config_index] + "\r\n"
                        self.uart.write(command.encode())
                    self.waiting_config = self.config_index < len(self.configs)
                else:
                    self._radio_frame(line)
            elif len(self.modem_line) < config.MAX_MODEM_LINE - 1:
                self.modem_line.append(byte)
            else:
                self.modem_line = bytearray()

    def tick(self):
        if self.usb_queue:
            pending = self.usb_queue[0]
            written = self.usb_out.write(pending[self.pending_usb_offset:])
            if written is None:
                written = len(pending) - self.pending_usb_offset
            self.pending_usb_offset += written
            if self.pending_usb_offset >= len(pending):
                self.usb_queue.pop(0)
                self.pending_usb_offset = 0
        if not self.waiting_config and self.radio_queue:
            _, command = self.radio_queue.pop(0)
            self.uart.write(command)
