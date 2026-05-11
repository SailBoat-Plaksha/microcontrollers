
import serial
import time
import threading
import sys
import base64
import json

# --- Configuration ---
# Update this to the correct UART port for your Pi 5 setup
SERIAL_PORT = '/dev/ttyAMA0' 
MOTOR_SERIAL_PORT = '/dev/ttyUSB0'  # Default USB serial for motor board
# Known commands/destinations for validation
KNOWN_COMMANDS = {
    'MOTOR': 'MOTOR:',
    'GPS': 'GPS:',
    'NAV': 'NAV:'
}
BAUD_RATE=115200
MOTOR_BAUD_RATE=115200

# --- Motor Board Interface (Real) ---
class MotorBoard:
    """
    Interface for the Motor Daughter Board connected via USB serial.
    Accepts JSON commands (e.g., {"angle": 90}) and relays feedback via LoRA.
    """
    def __init__(self, lora_handler, port=MOTOR_SERIAL_PORT, baud=MOTOR_BAUD_RATE):
        self.lora_handler = lora_handler
        self.running = True
        self.ser = None
        try:
            self.ser = serial.Serial(port, baud, timeout=1)
            print(f"[MotorBoard] Serial opened on {port} at {baud} baud")
        except Exception as e:
            print(f"[MotorBoard] Failed to open motor serial: {e}")

    def send_to_board(self, command_data):
        """
        Send data TO the motor board. Ensures the command is valid JSON with 'angle'.
        """
        print(f"[MotorBoard] Received command to forward: {command_data}")
        if not self.ser:
            print("[MotorBoard] Serial not connected, cannot send.")
            return

        payload = None
        command_data = command_data.strip()
        
        try:
            parsed = json.loads(command_data)
            if "angle" in parsed:
                angle = max(0, min(180, int(parsed["angle"])))
                payload = json.dumps({"angle": angle})
        except json.JSONDecodeError:
            # Try to interpret as raw integer just in case
            if command_data.isdigit():
                angle = max(0, min(180, int(command_data)))
                payload = json.dumps({"angle": angle})
                
        if payload:
            try:
                data_to_send = payload + '\n'
                self.ser.write(data_to_send.encode('utf-8'))
                print(f"[MotorBoard] Sent to driver: {payload}")
            except Exception as e:
                print(f"[MotorBoard] Write error: {e}")
        else:
            print(f"[MotorBoard] Invalid command discarded (must contain 'angle'): {command_data}")

    def listen_from_board(self):
        """
        Continuously read from the motor board's serial interface and relay to base station.
        """
        print("[MotorBoard] Listener started")
        while self.running:
            if self.ser and self.ser.in_waiting > 0:
                try:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"[MotorBoard Rx] {line}")
                        self.relay_to_base(line)
                except Exception as e:
                    print(f"[MotorBoard] Read Error: {e}")
                    time.sleep(0.1)
            else:
                time.sleep(0.1)

    def relay_to_base(self, message):
        """
        Helper to transmit motor feedback back to the base station via LoRA.
        """
        print(f"[MotorBoard] Relaying to base: {message}")
        prefix = "MOTOR SAYS:"
        full_msg = f"{prefix} {message}"
        encoded_msg = base64.b64encode(full_msg.encode('utf-8')).decode('utf-8')
        
        # Defaulting to broadcast address 0
        target_address = 0 
        self.lora_handler.send_message(target_address, encoded_msg)

    def stop(self):
        self.running = False
        if self.ser:
            self.ser.close()

    def simulate_motor_message(self, message):
        """Legacy helper matching old code structure for 'sim_motor'."""
        self.relay_to_base(message)


# --- Command Dispatcher ---
class CommandDispatcher:
    """
    Parses incoming LoRA messages and routes them.
    """
    def __init__(self, motor_board):
        self.motor_board = motor_board

    def handle_message(self, sender_address, payload_length, rssi, snr, data_str):
        """
        Process a received message packet.
        Format expected: +RCV=<addr>,<len>,<data>,<rssi>,<snr>
        """
        try:
            # Attempt to decode Base64
            # The data logic described: "Prefix... then Base64" 
            # OR "Base64 encoded string which contains the prefix inside"?
            # Prompt says: "Decode incoming base64 commands, which will be prefixed by who they are being addressed to"
            # It implies the RAW payload is Base64. We decode it, THEN check the prefix.
            
            decoded_bytes = base64.b64decode(data_str)
            decoded_str = decoded_bytes.decode('utf-8', errors='ignore') # Use ignore to be safe
            
            print(f"[LoRA] Decoded Payload: {decoded_str}")

            # 1) Decode incoming base64 commands, which will be prefixed by who they are being addressed to
            # Parsing "MOTOR: <array>"
            if decoded_str.startswith("MOTOR:"):
                command_content = decoded_str[len("MOTOR:"):].strip()
                print(f"[Dispatcher] Routing to MOTOR: {command_content}")
                self.motor_board.send_to_board(command_content)
            else:
                print(f"[Dispatcher] Unknown addressee or format: {decoded_str}")

        except Exception as e:
            print(f"[Dispatcher] Error decoding/routing message: {e}")
            print(f"[Dispatcher] Raw data was: {data_str}")


# --- LoRA Handler ---
class LoRAHandler:
    def __init__(self, port, baud):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = True
        self.dispatcher = None # Set after init

        try:
            self.ser = serial.Serial(port, baud, timeout=1)
            print(f"[LoRA] Serial opened on {port}")
        except Exception as e:
            print(f"[LoRA] Failed to open serial: {e}")
            sys.exit(1)

    def set_dispatcher(self, dispatcher):
        self.dispatcher = dispatcher

    def send_at(self, cmd):
        """Send a raw AT command to the LoRA module."""
        if self.ser:
            full_cmd = cmd + "\r\n"
            self.ser.write(full_cmd.encode('utf-8'))
            # print(f"[LoRA] Sent: {cmd}") # Optional debug

    def send_message(self, address, data_b64):
        """
        Send a message via LoRA.
        Format: AT+SEND=<Address>,<PayloadLength>,<Data>
        """
        length = len(data_b64)
        cmd = f"AT+SEND={address},{length},{data_b64}"
        self.send_at(cmd)
        print(f"[LoRA] Transmitting: {cmd}")

    def listen(self):
        """Background thread to read lines from LoRA serial."""
        print("[LoRA] Listener thread started")
        while self.running and self.ser:
            try:
                if self.ser.in_waiting > 0:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if not line:
                        continue
                    
                    print(f"[LoRA RX] {line}")

                    # Check for +RCV
                    # Format: +RCV=<Address>,<Length>,<Data>,<RSSI>,<SNR>
                    if line.startswith("+RCV="):
                        parts = line[5:].split(',')
                        if len(parts) >= 3:
                            addr = parts[0]
                            length = parts[1]
                            data = parts[2]
                            rssi = parts[3] if len(parts) > 3 else "0"
                            snr = parts[4] if len(parts) > 4 else "0"
                            
                            if self.dispatcher:
                                self.dispatcher.handle_message(addr, length, rssi, snr, data)
            except Exception as e:
                print(f"[LoRA] Read Error: {e}")
                time.sleep(0.1)

    def stop(self):
        self.running = False
        if self.ser:
            self.ser.close()


# --- Main ---
def main():
    print("--- Pi 5 LoRA Gateway Started ---")
    
    # Initialize Core Components
    lora = LoRAHandler(SERIAL_PORT, BAUD_RATE)
    motor = MotorBoard(lora)
    dispatcher = CommandDispatcher(motor)
    
    lora.set_dispatcher(dispatcher)

    # Start Threads
    t_lora = threading.Thread(target=lora.listen, daemon=True)
    t_lora.start()

    t_motor = threading.Thread(target=motor.listen_from_board, daemon=True)
    t_motor.start()

    print("--- Type AT commands below (or 'exit' / 'sim_motor') ---")
    
    try:
        while True:
            user_input = input() # Blocking console input
            
            if user_input.lower() == 'exit':
                break
            
            elif user_input.lower().startswith('sendto '):
                # Usage: sendto <ADDRESS> <message>
                parts = user_input.split(' ', 2)
                if len(parts) < 3:
                     print("Usage: sendto <ADDRESS> <message>")
                     continue

                target_addr = parts[1]
                msg_content = parts[2]
                
                # Base64 encode the message
                encoded_msg = base64.b64encode(msg_content.encode('utf-8')).decode('utf-8')
                lora.send_message(target_addr, encoded_msg)

            elif user_input.lower().startswith('sim_motor '):
                # Legacy helper for simulation
                msg = user_input[10:]
                motor.simulate_motor_message(msg)
                
            elif user_input.lower().startswith('send '):
                 # Usage: send <message> (Broadcasts to 0)
                 msg_content = user_input[5:]
                 encoded_msg = base64.b64encode(msg_content.encode('utf-8')).decode('utf-8')
                 lora.send_message(0, encoded_msg)
                 
            else:
                # Treat as AT command
                lora.send_at(user_input)
                
    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        lora.stop()
        motor.stop()
        print("Done.")

if __name__ == "__main__":
    main()