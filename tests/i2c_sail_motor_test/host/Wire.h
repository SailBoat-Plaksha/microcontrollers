#pragma once
#include <vector>
#include <stdint.h>
struct MockWire {
  std::vector<uint8_t> input, output;
  unsigned offset = 0;
  int available() { return input.size() - offset; }
  int read() { return input.at(offset++); }
  void write(const uint8_t *bytes, unsigned length) { output.assign(bytes, bytes + length); }
  void setSDA(uint8_t) {}
  void setSCL(uint8_t) {}
  void onReceive(void (*)(int)) {}
  void onRequest(void (*)()) {}
  void begin(uint8_t) {}
};
extern MockWire Wire;
