"""Hardware-independent AT text bridge state machine for Raspberry Pi Pico.

Bridges native USB CDC and RYLR998 UART0 with bounded line buffering,
command validation, whitespace formatting normalization, AT+SEND payload preservation,
and partial write handling.
"""

try:
    import config
    DEFAULT_MAX_LINE = getattr(config, "BRIDGE_MAX_LINE", 512)
    DEFAULT_MAX_UART = getattr(config, "BRIDGE_MAX_UART_BUFFER", 1024)
    DEFAULT_MAX_USB = getattr(config, "BRIDGE_MAX_USB_BUFFER", 1024)
    DEFAULT_MAX_PAYLOAD = getattr(config, "BRIDGE_MAX_PAYLOAD", 240)
except ImportError:
    DEFAULT_MAX_LINE = 512
    DEFAULT_MAX_UART = 1024
    DEFAULT_MAX_USB = 1024
    DEFAULT_MAX_PAYLOAD = 240


class ATBridge:
    """Hardware-independent AT command bridge state machine."""

    def __init__(self, uart, usb_out,
                 max_line=DEFAULT_MAX_LINE,
                 max_uart_buffer=DEFAULT_MAX_UART,
                 max_usb_buffer=DEFAULT_MAX_USB,
                 max_payload=DEFAULT_MAX_PAYLOAD):
        self.uart = uart
        self.usb_out = usb_out
        self.max_line_length = max_line
        self.max_uart_buffer = max_uart_buffer
        self.max_usb_buffer = max_usb_buffer
        self.max_payload = max_payload

        self.usb_rx_buffer = bytearray()
        self._last_was_cr = False
        self._line_overflow = False

        self.uart_tx_buffer = bytearray()
        self.usb_tx_buffer = bytearray()

        self.rx_overflow = 0
        self.uart_overflow = 0
        self.usb_overflow = 0
        self.commands_forwarded = 0
        self.commands_rejected = 0

    def feed_usb(self, data):
        """Process incoming USB CDC byte stream."""
        if isinstance(data, str):
            data = data.encode("latin-1")
        for byte in data:
            if byte == 13:  # \r
                self._last_was_cr = True
                if self._line_overflow:
                    self._line_overflow = False
                    self.usb_rx_buffer = bytearray()
                else:
                    line = bytes(self.usb_rx_buffer)
                    self.usb_rx_buffer = bytearray()
                    self._process_usb_line(line)
            elif byte == 10:  # \n
                if self._last_was_cr:
                    self._last_was_cr = False
                    # skip LF following CR in CRLF
                else:
                    if self._line_overflow:
                        self._line_overflow = False
                        self.usb_rx_buffer = bytearray()
                    else:
                        line = bytes(self.usb_rx_buffer)
                        self.usb_rx_buffer = bytearray()
                        self._process_usb_line(line)
            else:
                self._last_was_cr = False
                if self._line_overflow:
                    continue
                if len(self.usb_rx_buffer) >= self.max_line_length:
                    self._line_overflow = True
                    self.usb_rx_buffer = bytearray()
                    self.rx_overflow += 1
                    self.commands_rejected += 1
                    self._queue_usb(b"+ERR=BRIDGE_LINE_TOO_LONG\r\n")
                else:
                    self.usb_rx_buffer.append(byte)

    def feed_modem(self, data):
        """Forward modem UART response bytes unchanged to USB CDC."""
        if isinstance(data, str):
            data = data.encode("latin-1")
        self._queue_usb(data)

    def _queue_usb(self, data):
        if len(self.usb_tx_buffer) + len(data) > self.max_usb_buffer:
            self.usb_overflow += 1
            return False
        self.usb_tx_buffer.extend(data)
        return True

    def _queue_uart(self, data):
        if len(self.uart_tx_buffer) + len(data) > self.max_uart_buffer:
            self.uart_overflow += 1
            self.commands_rejected += 1
            self._queue_usb(b"+ERR=BRIDGE_BUSY\r\n")
            return False
        self.uart_tx_buffer.extend(data)
        self.commands_forwarded += 1
        return True

    def _process_usb_line(self, line):
        lstripped = line.lstrip()
        if not lstripped:
            return

        # Reject non-ASCII bytes
        if any(b > 127 for b in line):
            self.commands_rejected += 1
            self._queue_usb(b"+ERR=BRIDGE_NON_ASCII\r\n")
            return

        # Reject control characters (0x00-0x1F except 0x09 tab, and 0x7F DEL)
        if any((b < 0x20 and b != 0x09) or b == 0x7F for b in line):
            self.commands_rejected += 1
            self._queue_usb(b"+ERR=BRIDGE_CONTROL_CHAR\r\n")
            return

        upper_lstripped = lstripped.upper()
        if not upper_lstripped.startswith(b"AT"):
            self.commands_rejected += 1
            self._queue_usb(b"+ERR=BRIDGE_INVALID_COMMAND\r\n")
            return

        if b"=" in lstripped:
            prefix, sep, rest = lstripped.partition(b"=")
        else:
            prefix = lstripped
            sep = b""
            rest = b""

        cmd_name = prefix.replace(b" ", b"").replace(b"\t", b"").upper()
        if cmd_name == b"AT+SEND":
            if not sep:
                self.commands_rejected += 1
                self._queue_usb(b"+ERR=BRIDGE_PARSE_ERROR\r\n")
                return

            parts = rest.split(b",", 2)
            if len(parts) != 3:
                self.commands_rejected += 1
                self._queue_usb(b"+ERR=BRIDGE_PARSE_ERROR\r\n")
                return

            addr_bytes = parts[0].strip()
            len_bytes = parts[1].strip()
            payload_bytes = parts[2]  # MUST NEVER BE MUTATED (preserved exactly as received)

            # Reject control characters in payload
            if any(b < 0x20 or b >= 0x7F for b in payload_bytes):
                self.commands_rejected += 1
                self._queue_usb(b"+ERR=BRIDGE_CONTROL_CHAR\r\n")
                return

            # Validate address bounds
            if not addr_bytes.isdigit():
                self.commands_rejected += 1
                self._queue_usb(b"+ERR=BRIDGE_INVALID_ADDRESS\r\n")
                return
            addr_val = int(addr_bytes)
            if not (0 <= addr_val <= 65535):
                self.commands_rejected += 1
                self._queue_usb(b"+ERR=BRIDGE_INVALID_ADDRESS\r\n")
                return

            actual_len = len(payload_bytes)
            if actual_len > self.max_payload:
                self.commands_rejected += 1
                self._queue_usb(b"+ERR=BRIDGE_PAYLOAD_TOO_LARGE\r\n")
                return

            # Recalculate length from preserved ASCII payload if wrong numeric length
            fwd = (b"AT+SEND=" + str(addr_val).encode("ascii") + b"," +
                   str(actual_len).encode("ascii") + b"," +
                   payload_bytes + b"\r\n")
            self._queue_uart(fwd)
            return

        # General AT command: normalize whitespace around formatting
        stripped = lstripped.rstrip()
        if sep:
            rest_stripped = rest.strip()
            val_parts = [p.strip() for p in rest_stripped.split(b",")]
            val_clean = b",".join(val_parts)
            fwd = cmd_name + b"=" + val_clean + b"\r\n"
        else:
            fwd = cmd_name + b"\r\n"

        self._queue_uart(fwd)

    def tick(self):
        """Service nonblocking UART and USB CDC writes with partial retention."""
        if self.usb_tx_buffer:
            try:
                written = self.usb_out.write(self.usb_tx_buffer)
            except (OSError, ValueError):
                written = 0
            if written is not None and written > 0:
                del self.usb_tx_buffer[:min(written, len(self.usb_tx_buffer))]

        if self.uart_tx_buffer:
            try:
                written = self.uart.write(self.uart_tx_buffer)
            except (OSError, ValueError):
                written = 0
            if written is not None and written > 0:
                del self.uart_tx_buffer[:min(written, len(self.uart_tx_buffer))]

    def status(self):
        """Current state metrics."""
        return {
            "uart_pending": len(self.uart_tx_buffer),
            "usb_pending": len(self.usb_tx_buffer),
            "rx_overflow": self.rx_overflow,
            "uart_overflow": self.uart_overflow,
            "usb_overflow": self.usb_overflow,
            "commands_forwarded": self.commands_forwarded,
            "commands_rejected": self.commands_rejected,
        }
