// Exercise the real .ino with fake time, I2C, and observable GPIO outputs.
#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "Arduino.h"
#include "Wire.h"
uint32_t hostNow = 0;
int hostPins[30] = {};
MockWire Wire;
#include "../i2c_sail_motor_test.ino"

int main() {
  setup();
  std::string action;
  while (std::cin >> action) {
    if (action == "at") std::cin >> hostNow;
    else if (action == "write") {
      std::string hex;
      std::cin >> hex;
      Wire.input.clear(); Wire.offset = 0;
      for (unsigned i = 0; i < hex.size(); i += 2)
        Wire.input.push_back(std::stoul(hex.substr(i, 2), nullptr, 16));
      receiveFrame(Wire.input.size());
    } else if (action == "tick") loop();
    else if (action == "read") {
      sendStatus();
      for (uint8_t byte : Wire.output)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
      std::cout << std::dec << '\n';
    } else return 2;
    // Assert physical output intent after every operation, including rejections.
    assert(hostPins[PWM_A] == outputPwm);
    assert(hostPins[STANDBY_PIN] == (state == 1 ? HIGH : LOW));
    assert(hostPins[INA_1] == (outputDirection == 1 ? HIGH : LOW));
    assert(hostPins[INA_2] == (outputDirection == -1 ? HIGH : LOW));
    assert(hostPins[PWM_B] == 0 && hostPins[INB_1] == 0 && hostPins[INB_2] == 0);
  }
}
