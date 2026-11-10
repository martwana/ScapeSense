#pragma once

#include "ConfigManager.h"
#include "RollingAverage.h"
#include "Sampler.h"
#include <cstdint>

class BubbleSensor {
public:
  BubbleSensor(ConfigManager &config, Sampler &sampler);

  bool begin();
  bool update();

  void enableLed(bool on);
  bool ledOn() const;

  uint16_t lastRaw() const;
  uint32_t average() const;
  uint16_t minRaw() const;
  uint16_t maxRaw() const;
  bool isInitialized() const;

  void printDiagnostics();

  static uint32_t rawToMillivolts(uint16_t raw);

private:
  ConfigManager &config_;
  Sampler &sampler_;
  RollingAverage<uint32_t, 10> average_;
  uint16_t lastRaw_;
  uint16_t minRaw_;
  uint16_t maxRaw_;
  bool initialized_;
  bool ledState_;
};
