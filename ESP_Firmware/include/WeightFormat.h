#pragma once

#include <cmath>
#include <cstdio>
#include <string>

inline std::string formatWeightG(float grams) {
  char text[32];
  const float magnitude = std::fabs(grams);
  if (!std::isfinite(grams)) {
    return "--";
  }
  if (magnitude <= 500.0f) {
    snprintf(text, sizeof(text), "%.0f g", magnitude < 0.5f ? 0.0f : grams);
  } else {
    snprintf(text, sizeof(text), "%.2f", grams / 1000.0f);
    std::string kilograms(text);
    while (kilograms.back() == '0') kilograms.pop_back();
    if (kilograms.back() == '.') kilograms.pop_back();
    return kilograms + " kg";
  }
  return text;
}
