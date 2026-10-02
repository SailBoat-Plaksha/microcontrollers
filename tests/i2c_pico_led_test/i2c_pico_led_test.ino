// Raspberry Pi Pico / Pico W using Earle Philhower's Arduino-Pico core.
// I2C slave address: 0x08. SDA: GP0 (physical pin 1), SCL: GP1 (pin 2).
// Connect a common GND and use SDA/SCL pull-ups to 3.3 V.
// Write one byte: 0x01 (or ASCII '1') = ON, 0x00 (or ASCII '0') = OFF.
// Other values and multi-byte writes are ignored; state persists until changed.
// Master example: Wire.beginTransmission(0x08); Wire.write((uint8_t)1);
//                 Wire.endTransmission(); // Use 0 instead of 1 for OFF.


#include <Wire.h>

constexpr uint8_t I2C_ADDRESS = 0x08;
constexpr uint8_t SDA_PIN = 0;
constexpr uint8_t SCL_PIN = 1;
volatile bool ledOn = false;

void receiveCommand(int byteCount) {
  int command = -1;
  if (byteCount == 1 && Wire.available()) {
    command = Wire.read();
  }
  while (Wire.available()) {
    Wire.read();
  }

  if (command == 0x01 || command == '1') {
    ledOn = true;
  } else if (command == 0x00 || command == '0') {
    ledOn = false;
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.onReceive(receiveCommand);
  Wire.begin(I2C_ADDRESS);
}

void loop() {
  // Apply outside the receive callback, including for the Pico W onboard LED.
  digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);
}
