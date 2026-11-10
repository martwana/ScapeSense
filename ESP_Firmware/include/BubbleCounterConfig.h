#pragma once

#include <cstdint>

struct BubbleCounterConfig {
  uint16_t threshold = 200;
  uint16_t hysteresis = 50;
  uint16_t minSeparationMs = 150;
  uint16_t maxReliableBps = 10;
  uint8_t polarity = 0; // 0 = negative deviation (bubble lowers ADC)
};
