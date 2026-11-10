#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

struct SensorTuning {
  SensorTuning(uint16_t thresholdValue = 150, uint16_t hysteresisValue = 25,
               uint16_t separationValue = 30, uint16_t reliableRateValue = 10,
               uint8_t polarityValue = 0)
      : threshold(thresholdValue), hysteresis(hysteresisValue),
        minSeparationMs(separationValue), maxReliableBps(reliableRateValue),
        polarity(polarityValue) {}

  uint16_t threshold;
  uint16_t hysteresis;
  uint16_t minSeparationMs;
  uint16_t maxReliableBps;
  uint8_t polarity;
};

enum class SensorTuningError {
  None,
  Threshold,
  Hysteresis,
  Separation,
  ReliableRate,
  Polarity,
};

inline SensorTuningError validateSensorTuning(const SensorTuning &tuning) {
  if (tuning.threshold < 10 || tuning.threshold > 1000) {
    return SensorTuningError::Threshold;
  }
  if (tuning.hysteresis < 5 || tuning.hysteresis > 500 ||
      tuning.hysteresis >= tuning.threshold) {
    return SensorTuningError::Hysteresis;
  }
  if (tuning.minSeparationMs < 20 || tuning.minSeparationMs > 1000) {
    return SensorTuningError::Separation;
  }
  if (tuning.maxReliableBps < 1 || tuning.maxReliableBps > 20) {
    return SensorTuningError::ReliableRate;
  }
  if (tuning.polarity > 1) {
    return SensorTuningError::Polarity;
  }
  return SensorTuningError::None;
}

inline std::string buildSensorTuningJson(const SensorTuning &tuning) {
  char json[192];
  snprintf(json, sizeof(json),
           "{\"threshold\":%u,\"hysteresis\":%u,\"minSeparationMs\":%u,"
           "\"maxReliableBps\":%u,\"polarity\":%u}",
           tuning.threshold, tuning.hysteresis, tuning.minSeparationMs,
           tuning.maxReliableBps, tuning.polarity);
  return json;
}

inline const char *sensorTuningErrorMessage(SensorTuningError error) {
  switch (error) {
  case SensorTuningError::Threshold:
    return "Threshold must be between 10 and 1000";
  case SensorTuningError::Hysteresis:
    return "Hysteresis must be between 5 and 500 and below threshold";
  case SensorTuningError::Separation:
    return "Minimum separation must be between 20 and 1000 ms";
  case SensorTuningError::ReliableRate:
    return "Reliable rate must be between 1 and 20 BPS";
  case SensorTuningError::Polarity:
    return "Polarity must be negative or positive";
  default:
    return "";
  }
}
