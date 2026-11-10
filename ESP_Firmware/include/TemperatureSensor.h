#pragma once

#include "TemperatureReading.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#include <cstdint>

class TemperatureSensor {
public:
  TemperatureSensor();
  ~TemperatureSensor();

  bool begin(uint8_t pin);
  void update(uint32_t nowMs);
  TemperatureReading reading() const;
  bool isInitialized() const;

private:
  static constexpr uint32_t kConversionMs = 800;
  static constexpr uint32_t kSampleIntervalMs = 1000;

  OneWire *oneWire_;
  DallasTemperature *sensors_;
  TemperatureReading reading_;
  uint32_t conversionStartedMs_;
  uint32_t lastRequestMs_;
  bool conversionPending_;
};
