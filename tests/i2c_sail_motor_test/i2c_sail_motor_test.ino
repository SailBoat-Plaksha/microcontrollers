// Bench test for Raspberry Pi Pico/Pico W, Earle Philhower Arduino-Pico core.
// All numbers are GP numbers from the user's pin table, NOT header positions.
#include <Arduino.h>
#include <Wire.h>
#include <hardware/watchdog.h>
#include "src/mavlink/arsem_test/mavlink.h"

constexpr uint8_t PWM_A = 6, INA_2 = 7, INA_1 = 8, STANDBY_PIN = 9;
constexpr uint8_t INB_1 = 10, INB_2 = 11, PWM_B = 12; // Keep rudder off.
constexpr uint8_t SDA_PIN = 0, SCL_PIN = 1, I2C_ADDRESS = 0x08;
constexpr uint8_t SYSTEM_ID = 1, COMPONENT_ID = 154;
constexpr uint8_t TRANSFER_SIZE = 32, MAX_PWM = 128;
constexpr uint16_t MAX_DURATION_MS = 5000;

volatile uint8_t inbox[TRANSFER_SIZE];
volatile uint8_t inboxLength = 0;
volatile bool overflow = false;
uint8_t statusBytes[TRANSFER_SIZE] = {};

uint32_t lastCommandId = 0, startedAt = 0;
uint16_t durationMs = 0, elapsedMs = 0;
uint8_t state = 0, result = 0, outputPwm = 0;
int8_t outputDirection = 0;

void stopMotor(uint8_t nextState) {
  // LOW enters standby, matching the driver's polarity in dev-watchdog.
  analogWrite(PWM_A, 0);
  digitalWrite(STANDBY_PIN, LOW);
  digitalWrite(INA_1, LOW);
  digitalWrite(INA_2, LOW);
  if (state == 1) elapsedMs = (uint16_t)(millis() - startedAt);
  outputDirection = 0;
  outputPwm = 0;
  state = nextState;
}

void receiveFrame(int byteCount) {
  // IRQ callback: copy only. Parse and operate GPIO in loop().
  if (byteCount < 1 || byteCount > TRANSFER_SIZE || inboxLength != 0) {
    while (Wire.available()) Wire.read();
    overflow = true;
    return;
  }
  uint8_t count = 0;
  while (Wire.available() && count < byteCount) inbox[count++] = Wire.read();
  if (count == byteCount) inboxLength = count; // Publish after all bytes.
  else overflow = true;
}

void sendStatus() {
  // Snapshot is published atomically by loop(); no packing in the IRQ.
  Wire.write(statusBytes, TRANSFER_SIZE);
}

void processFrame(const uint8_t *bytes, uint8_t length) {
  // One complete unsigned MAVLink 2 command per I2C transaction; no register
  // byte, no SMBus count byte, no cross-transaction stream reassembly.
  if (length < 13 || bytes[0] != MAVLINK_STX || bytes[2] != 0 ||
      bytes[3] != 0 || length != bytes[1] + 12 ||
      bytes[1] > MAVLINK_MSG_ID_SAIL_MOTOR_TEST_LEN) {
    result = 4;
    return;
  }
  mavlink_message_t parserMessage = {}, message = {};
  mavlink_status_t parserStatus = {}, status = {};
  uint8_t framing = MAVLINK_FRAMING_INCOMPLETE;
  for (uint8_t i = 0; i < length; ++i) {
    framing = mavlink_frame_char_buffer(&parserMessage, &parserStatus,
                                       bytes[i], &message, &status);
  }
  if (framing != MAVLINK_FRAMING_OK ||
      message.msgid != MAVLINK_MSG_ID_SAIL_MOTOR_TEST) {
    result = 4;
    return;
  }
  mavlink_sail_motor_test_t command = {};
  mavlink_msg_sail_motor_test_decode(&message, &command);
  if (message.sysid != 1 || message.compid != 1 ||
      command.target_system != SYSTEM_ID ||
      command.target_component != COMPONENT_ID) {
    result = 6;
    return;
  }
  bool stop = command.direction == 0;
  bool valid = stop ? (command.duration_ms == 0 && command.pwm == 0)
                    : ((command.direction == 1 || command.direction == -1) &&
                       command.duration_ms > 0 &&
                       command.duration_ms <= MAX_DURATION_MS &&
                       command.pwm > 0 && command.pwm <= MAX_PWM);
  if (!valid) { result = 1; return; }
  if (command.command_id == 0 || command.command_id <= lastCommandId) {
    result = 3;
    return;
  }
  if (!stop && state == 1) { result = 2; return; }
  lastCommandId = command.command_id;
  result = 0;
  if (stop) { stopMotor(3); return; }

  // A new move starts only from a stopped state. A stop command must precede
  // a reversal while running; this sketch never reverses an energised bridge.
  digitalWrite(STANDBY_PIN, LOW);
  analogWrite(PWM_A, 0);
  digitalWrite(INA_1, command.direction == 1 ? HIGH : LOW);
  digitalWrite(INA_2, command.direction == -1 ? HIGH : LOW);
  outputDirection = command.direction;
  outputPwm = command.pwm;
  durationMs = command.duration_ms;
  elapsedMs = 0;
  startedAt = millis();
  state = 1;
  analogWrite(PWM_A, outputPwm);
  digitalWrite(STANDBY_PIN, HIGH);
}

void publishStatus() {
  uint32_t now = millis();
  uint16_t elapsed = state == 1 ? (uint16_t)(now - startedAt) : elapsedMs;
  uint16_t remaining = (state == 1 && elapsed < durationMs) ? durationMs - elapsed : 0;
  mavlink_message_t message = {};
  mavlink_msg_sail_motor_test_status_pack(
      SYSTEM_ID, COMPONENT_ID, &message, now, lastCommandId, elapsed,
      remaining, state, outputDirection, outputPwm, result);
  uint8_t snapshot[TRANSFER_SIZE] = {};
  mavlink_msg_to_send_buffer(snapshot, &message);
  noInterrupts();
  memcpy(statusBytes, snapshot, TRANSFER_SIZE);
  interrupts();
}

void setup() {
  // Shared standby is held LOW before any bridge output is enabled.
  digitalWrite(STANDBY_PIN, LOW);
  pinMode(STANDBY_PIN, OUTPUT);
  const uint8_t pins[] = {PWM_A, INA_1, INA_2, PWM_B, INB_1, INB_2};
  for (uint8_t pin : pins) { digitalWrite(pin, LOW); pinMode(pin, OUTPUT); }
  analogWriteRange(255);
  analogWriteFreq(20000);
  stopMotor(0);
  publishStatus();
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.onReceive(receiveFrame);
  Wire.onRequest(sendStatus);
  Wire.begin(I2C_ADDRESS);
  // Reset on a stalled main loop. External standby pulldown holds the driver
  // disabled while RP2040 pins are high impedance during reset.
  watchdog_enable(1000, false);
}

void loop() {
  if (state == 1 && (uint32_t)(millis() - startedAt) >= durationMs) stopMotor(2);

  uint8_t local[TRANSFER_SIZE], length;
  bool lostFrame;
  noInterrupts();
  length = inboxLength;
  lostFrame = overflow;
  for (uint8_t i = 0; i < length; ++i) local[i] = inbox[i];
  inboxLength = 0;
  overflow = false;
  interrupts();
  if (lostFrame) { stopMotor(4); result = 5; } // Discard queued command too.
  else if (length) processFrame(local, length);

  publishStatus();
  watchdog_update();
  delay(1);
}
