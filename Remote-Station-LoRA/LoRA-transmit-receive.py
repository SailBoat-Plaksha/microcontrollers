
import serial
import time
import threading
import sys
import base64

# --- Configuration ---
# Update this to the correct UART port for your Pi 5 setup
SERIAL_PORT = '/dev/ttyAMA0' 
BAUD_RATE = 115200

# --- Motor Board Interface (Mock) ---
class MotorBoard:
    """
    Interface for the Motor Daughter Board.
    Currently mocked to print to console.
    """
    def __init__(self, lora_handler):
        self.lora_handler = lora_handler
        self.running = True
        # In a real implementation, you might open another serial port or SPI/I2C connection here.

    def send_to_board(self, command_data):
        """
        Send data TO the motor board.
        input: command_data (str or bytes) - decoded data intended for motor
        """
        print(f"[MotorBoard] RECEIVED CMD: {command_data}")
        # Real implementation: serial_motor.write(command_data)

    def listen_from_board(self):
        """
        Simulate receiving data FROM the motor board.
        In a real implementation, this would be a loop reading from the motor board's interface.
        """
        print("[MotorBoard] Listener started (Mock implementation)")
        while self.running:
            # SImulate random data transmission for testing or wait for real input
            # For now, we just sleep. You can implement a key-trigger or timer to simulate data.
            time.sleep(1)

    def stop(self):
        self.running = False

    def simulate_motor_message(self, message):
        """
        Helper to manually trigger a message FROM the motor board,
        which should then be sent OUT via LoRA.
        """
        print(f"[MotorBoard] Sending info: {message}")
        prefix = "MOTOR SAYS:"
        full_msg = f"{prefix} {message}"
        encoded_msg = base64.b64encode(full_msg.encode('utf-8')).decode('utf-8')
        
        # Address to send to? Defaulting to broadcast or specific address 0 for now
        target_address = 0 
        self.lora_handler.send_message(target_address, encoded_msg)


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
            elif user_input.lower().startswith('sim_motor '):
                # Manual test trigger for motor sending
                # Usage: sim_motor Hello World
                msg = user_input[10:]c
                motor.simulate_motor_message(msg)
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