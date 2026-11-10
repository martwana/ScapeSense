#pragma once
#include <cstdint>

class HX711 {
public:
  inline static bool ready = true;
  inline static int32_t raw = 1000;
  void begin(uint8_t, uint8_t) {}
  bool is_ready() const { return ready; }
  long read() const { return raw; }
};
