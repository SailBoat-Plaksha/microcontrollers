#pragma once
#include <stdint.h>
#include <cstring>
constexpr int LOW = 0, HIGH = 1, OUTPUT = 1;
extern uint32_t hostNow;
extern int hostPins[30];
inline uint32_t millis() { return hostNow; }
inline void digitalWrite(uint8_t pin, int value) { hostPins[pin] = value; }
inline void analogWrite(uint8_t pin, int value) { hostPins[pin] = value; }
inline void pinMode(uint8_t, int) {}
inline void analogWriteRange(int) {}
inline void analogWriteFreq(int) {}
inline void noInterrupts() {}
inline void interrupts() {}
inline void delay(int) {}
