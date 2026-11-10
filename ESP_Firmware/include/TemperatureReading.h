#pragma once

struct TemperatureReading {
  TemperatureReading(float temperatureValue = 0.0f, bool validValue = false)
      : temperatureC(temperatureValue), valid(validValue) {}

  float temperatureC;
  bool valid;
};

constexpr bool isValidDs18b20Temperature(float temperatureC) {
  return temperatureC >= -55.0f && temperatureC <= 125.0f;
}
