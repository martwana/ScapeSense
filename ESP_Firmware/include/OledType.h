#pragma once

#include <cstdint>

enum class OledType : uint8_t {
  SSD1306_128x64 = 1,
  SH1106_128x64 = 2,
  SSD1306_128x32 = 3,
};

struct OledGeometry {
  uint16_t width;
  uint16_t height;
};

constexpr OledGeometry oledGeometry(OledType type) {
  return type == OledType::SSD1306_128x64   ? OledGeometry{128, 64}
         : type == OledType::SH1106_128x64  ? OledGeometry{128, 64}
         : type == OledType::SSD1306_128x32 ? OledGeometry{128, 32}
                                            : OledGeometry{0, 0};
}
