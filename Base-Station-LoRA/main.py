"""
LoRA Serial Bridge for Raspberry Pi Pico - RYLR998 Module
==========================================================
This code creates a bidirectional bridge between a laptop (via USB serial)
and a RYLR998 LoRA module (via UART). 

Features:
- AT commands (AT?, AT+ADDR, etc.) are passed directly to the module
- AT+SEND=<message> automatically Base64 encodes the message
- Received LoRA data is Base64 decoded before displaying

Core 0: Handles laptop -> LoRA (command routing + encoding)
Core 1: Handles LoRA -> laptop (receives and decodes)

Wiring:
  - LoRA TX -> GP5 (UART1 RX)
  - LoRA RX -> GP4 (UART1 TX)
  - LoRA VCC -> 3.3V
  - LoRA GND -> GND

Usage:
  - Type AT commands directly (AT?, AT+ADDR?, AT+BAND=915000000, etc.)
  - Type AT+SEND=<your message> to send Base64-encoded message
  - Received messages are automatically decoded from Base64
"""

import _thread
import sys
import select
from machine import UART, Pin
from ubinascii import b2a_base64, a2b_base64
from utime import sleep_ms

# ========== Configuration ==========
LORA_UART_ID = 1
LORA_TX_PIN = 4   # GP4 -> LoRA RX
LORA_RX_PIN = 5   # GP5 -> LoRA TX
LORA_BAUDRATE = 115200  # RYLR998 default baud rate

# Default target address for AT+SEND (0 = broadcast)
DEFAULT_TARGET_ADDR = 0

# ========== Global Variables ==========
lora_uart = None
running = True
led = Pin("LED", Pin.OUT)

# Lock for thread-safe UART access
uart_lock = _thread.allocate_lock()


def init_lora_uart():
    """Initialize UART for LoRA module communication."""
    global lora_uart
    lora_uart = UART(
        LORA_UART_ID,
        baudrate=LORA_BAUDRATE,
        tx=Pin(LORA_TX_PIN),
        rx=Pin(LORA_RX_PIN),
        bits=8,
        parity=None,
        stop=1
    )
    print("[SYS] LoRA UART initialized on UART{} (TX=GP{}, RX=GP{}) @ {} baud".format(
        LORA_UART_ID, LORA_TX_PIN, LORA_RX_PIN, LORA_BAUDRATE))


def encode_to_base64(data):
    """Encode bytes/string to Base64."""
    if isinstance(data, str):
        data = data.encode('utf-8')
    # b2a_base64 returns bytes with newline, strip it
    return b2a_base64(data).decode('utf-8').strip()


def decode_from_base64(data):
    """Decode Base64 string to ASCII."""
    try:
        if isinstance(data, str):
            data = data.encode('utf-8')
        return a2b_base64(data).decode('utf-8')
    except Exception as e:
        # Not valid Base64, return None
        return None


def send_raw_to_lora(command):
    """
    Send a raw command directly to the LoRA module.
    Used for AT commands that should not be modified.
    """
    global lora_uart
    
    # Ensure command ends with \r\n
    if not command.endswith('\r\n'):
        command = command.rstrip() + '\r\n'
    
    with uart_lock:
        lora_uart.write(command.encode('utf-8'))
    
    # Quick LED blink for command sent
    led.on()
    sleep_ms(300)
    led.off()


def send_message_to_lora(message, target_addr=None):
    """
    Send a message via LoRA with Base64 encoding.
    Formats: AT+SEND=<address>,<length>,<base64_data>
    
    RYLR998 format: AT+SEND=<Address>,<PayloadLength>,<Data>
    """
    if target_addr is None:
        target_addr = DEFAULT_TARGET_ADDR
    
    # Encode message to Base64
    encoded = encode_to_base64(message)
    
    # Format RYLR998 AT+SEND command
    at_command = "AT+SEND={},{},{}\r\n".format(target_addr, len(encoded), encoded)
    
    print("[TX] '{}' -> Base64: '{}'".format(message, encoded))
    
    with uart_lock:
        lora_uart.write(at_command.encode('utf-8'))
    
    # LED blink to indicate transmission
    led.on()
    sleep_ms(50)
    led.off()


# Known commands/destinations for validation
KNOWN_COMMANDS = {
    'MOTOR': 'MOTOR:',
    'GPS': 'GPS:',
    'NAV': 'NAV:'
}

def parse_input_command(input_line):
    """
    Parse input from laptop and determine how to handle it.
    """
    line = input_line.strip()
    line_upper = line.upper()
    
    # Check for quit command
    if line_upper == 'QUIT' or line_upper == 'EXIT':
        return ('QUIT', None)
    
    # Check for help command
    if line_upper == 'HELP' or line_upper == '?':
        return ('HELP', None)
    
    # Check for sendto <target> <message>
    if line_upper.startswith('SENDTO '):
        parts = line.split(' ', 2)
        if len(parts) < 3:
            print("[SYS] Usage: sendto <TARGET> <message>")
            return (None, None)
        
        target = parts[1].upper()
        message = parts[2]
        
        if target in KNOWN_COMMANDS:
            # Valid target, construct prefixed message
            prefix = KNOWN_COMMANDS[target]
            full_message = "{} {}".format(prefix, message)
            return ('AT_SEND', full_message)
        else:
            print("[SYS] Invalid target '{}'. Known targets: {}".format(
                target, ", ".join(KNOWN_COMMANDS.keys())))
            return (None, None)

    # Check for AT+SEND=<message> format (Deprecated but kept for raw fallback/debug if needed)
    if line_upper.startswith('AT+SEND='):
        # Extract the message part after AT+SEND=
        message = line[8:]
        return ('AT_SEND', message)
    
    # All other AT commands pass through directly
    if line_upper.startswith('AT'):
        return ('AT_DIRECT', line)
    
    # Non-AT input
    print("[SYS] Unknown command. Use 'sendto <TARGET> <msg>' or AT commands.")
    print("[SYS] Type 'help' for usage information.")
    return (None, None)


def show_help():
    """Display help information."""
    print("")
    print("=" * 55)
    print("  RYLR998 LoRA Bridge - Command Reference")
    print("=" * 55)
    print("")
    print("  MESSAGE SENDING:")
    print("    sendto <TARGET> <msg>  Send message to target")
    print("    Targets: {}".format(", ".join(KNOWN_COMMANDS.keys())))
    print("    Example: sendto MOTOR move_forward")
    print("")
    print("  COMMON AT COMMANDS (passed directly to module):")
    print("    AT                   Test module response")
    print("    AT+ADDR?             Query device address")
    print("    AT+ADDR=<0-65535>    Set device address")
    print("    AT+NETWORKID?        Query network ID")
    print("    AT+NETWORKID=<0-16>  Set network ID")
    print("    AT+BAND?             Query frequency band")
    print("    AT+BAND=<freq>       Set frequency (e.g., 915000000)")
    print("    AT+PARAMETER?        Query RF parameters")
    print("    AT+PARAMETER=<s,b,c,p>  Set RF parameters")
    print("    AT+IPR?              Query baud rate")
    print("    AT+IPR=<baud>        Set baud rate")
    print("    AT+RESET             Reset module")
    print("    AT+VER?              Query firmware version")
    print("")
    print("  SYSTEM COMMANDS:")
    print("    help / ?             Show this help")
    print("    quit / exit          Exit the bridge")
    print("")
    print("  RECEIVED MESSAGES:")
    print("    Incoming LoRA data is automatically decoded.")
    print("    Format: <TARGET> SAYS: <message>")
    print("=" * 55)
    print("")


def laptop_to_lora_task():
    """
    Core 0 Task: Read from laptop serial, process commands.
    - AT commands pass through directly
    - AT+SEND=<msg> encodes message to Base64 before sending
    """
    global running
    
    print("[SYS] Starting laptop -> LoRA bridge (Core 0)")
    print("[SYS] Type 'help' for command reference")
    print("-" * 50)
    
    # Buffer for incoming laptop data
    input_buffer = ""
    
    # Use select for non-blocking stdin read
    poll = select.poll()
    poll.register(sys.stdin, select.POLLIN)
    
    while running:
        try:
            # Check if there's data available on stdin
            events = poll.poll(100)  # 100ms timeout
            
            if events:
                char = sys.stdin.read(1)
                if char:
                    if char == '\n' or char == '\r':
                        if input_buffer:
                            # Parse and handle the command
                            cmd_type, data = parse_input_command(input_buffer)
                            
                            if cmd_type == 'QUIT':
                                print("[SYS] Shutting down...")
                                running = False
                                break
                            
                            elif cmd_type == 'HELP':
                                show_help()
                            
                            elif cmd_type == 'AT_DIRECT':
                                # Pass AT command directly to module
                                print("[CMD] {}".format(data))
                                send_raw_to_lora(data)
                            
                            elif cmd_type == 'AT_SEND':
                                # Encode and send message
                                send_message_to_lora(data)
                            
                            input_buffer = ""
                    else:
                        input_buffer += char
                        
        except Exception as e:
            print("[ERR] Laptop->LoRA error: {}".format(e))
            sleep_ms(100)


def lora_to_laptop_task():
    """
    Core 1 Task: Read from LoRA, decode Base64 messages, send to laptop.
    """
    global running, lora_uart
    
    print("[SYS] Starting LoRA -> laptop bridge (Core 1)")
    
    # Buffer for incoming LoRA data
    receive_buffer = ""
    
    while running:
        try:
            with uart_lock:
                if lora_uart.any():
                    # Read available data
                    data = lora_uart.read()
                    if data:
                        receive_buffer += data.decode('utf-8', 'ignore')
            
            # Process complete lines (looking for \r\n or \n)
            while '\n' in receive_buffer:
                line, receive_buffer = receive_buffer.split('\n', 1)
                line = line.strip()
                
                if line:
                    # Check for received data message
                    # RYLR998 format: +RCV=<Address>,<Length>,<Data>,<RSSI>,<SNR>
                    if line.startswith("+RCV="):
                        try:
                            # Parse +RCV response
                            content = line[5:]  # Remove "+RCV="
                            parts = content.split(',')
                            
                            if len(parts) >= 5:
                                addr = parts[0]
                                length = parts[1]
                                # Data might contain commas, so we need to be careful
                                # RSSI and SNR are the last two parts
                                rssi = parts[-2]
                                snr = parts[-1]
                                # Data is everything in between
                                data_parts = parts[2:-2]
                                raw_data = ','.join(data_parts)
                                
                                # Try to decode from Base64
                                decoded = decode_from_base64(raw_data)
                                
                                if decoded:
                                    # Check if message starts with a known prefix to format nicely
                                    formatted_output = None
                                    for target, prefix in KNOWN_COMMANDS.items():
                                        if decoded.startswith(prefix):
                                            # Strip prefix and format
                                            msg_content = decoded[len(prefix):].strip()
                                            formatted_output = "{} SAYS: {}".format(target, msg_content)
                                            break
                                    
                                    if formatted_output:
                                        print(formatted_output)
                                    else:
                                        # No known prefix, just print raw decoded
                                        print(">>> {}".format(decoded))
                                else:
                                    # Not valid Base64, show raw
                                    print("[RX] From addr {}: '{}' (raw, RSSI:{}, SNR:{})".format(
                                        addr, raw_data, rssi, snr))
                            else:
                                print("[RX] {}".format(line))
                                
                        except Exception as e:
                            print("[RX] Parse error: {} - Raw: {}".format(e, line))
                        
                        # LED blink for reception
                        led.on()
                        sleep_ms(50)
                        led.off()
                    
                    elif line.startswith("+OK") or line.startswith("+ERR"):
                        # AT command response
                        print("[RSP] {}".format(line))
                    
                    elif line.startswith("+"):
                        # Other module responses (e.g., +ADDR=, +BAND=, etc.)
                        print("[RSP] {}".format(line))
                    
                    else:
                        # Unknown format, just display
                        print("[LoRA] {}".format(line))
            
            sleep_ms(10)  # Small delay to prevent busy waiting
            
        except Exception as e:
            print("[ERR] LoRA->Laptop error: {}".format(e))
            sleep_ms(100)


def main():
    """Main entry point."""
    global running
    
    print("")
    print("=" * 55)
    print("  LoRA Serial Bridge - RYLR998")
    print("  Raspberry Pi Pico (Dual Core)")
    print("=" * 55)
    print("")
    
    # Initialize LoRA UART
    init_lora_uart()
    
    # Start Core 1 thread for LoRA -> laptop
    print("[SYS] Starting second core...")
    _thread.start_new_thread(lora_to_laptop_task, ())
    
    sleep_ms(100)  # Give Core 1 time to start
    
    # Run laptop -> LoRA on Core 0 (main thread)
    try:
        laptop_to_lora_task()
    except KeyboardInterrupt:
        print("\n[SYS] Keyboard interrupt received")
        running = False
    
    sleep_ms(200)  # Give Core 1 time to stop
    print("[SYS] Bridge shutdown complete")
    led.off()


if __name__ == "__main__":
    main()
