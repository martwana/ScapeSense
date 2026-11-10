#pragma once

#include "LoadCellReading.h"
#include "MedianFilter.h"
#include <HX711.h>
#include <cstdint>

enum class LoadCellCalibrationAction : uint8_t { Tare, KnownMass };

struct LoadCellCalibrationResult {
  LoadCellCalibrationAction action = LoadCellCalibrationAction::Tare;
  int32_t rawAverage = 0;
  int32_t offset = 0;
  float scale = 0.0f;
  bool success = false;
};

class LoadCellSensor {
public:
  LoadCellSensor();
  ~LoadCellSensor();

  bool begin(uint8_t doutPin, uint8_t sckPin,
             const LoadCellCalibration &calibration);
  void update(uint32_t nowMs);
  LoadCellReading reading() const;
  int32_t lastRawSample() const { return lastRawSample_; }
  LoadCellCalibration calibration() const;
  bool isInitialized() const;
  bool isBusy() const;
  bool requestTare(uint32_t nowMs);
  bool requestKnownMassCalibration(float knownMassG, uint32_t nowMs);
  bool takeCalibrationResult(LoadCellCalibrationResult &result);

private:
  enum class Operation : uint8_t { None, Tare, Calibrate };
  static constexpr uint8_t kCalibrationSamples = 16;
  static constexpr uint32_t kReadingTimeoutMs = 1500;
  static constexpr uint32_t kCalibrationTimeoutMs = 5000;
  static constexpr float kFilterAlpha = 0.15f;

  void collectCalibrationSample(int32_t raw, uint32_t nowMs);
  void finishCalibration(int32_t rawAverage);
  void failCalibration();

  HX711 *hx711_;
  LoadCellCalibration calibration_;
  LoadCellReading reading_;
  int32_t lastRawSample_ = 0;
  Operation operation_;
  float pendingKnownMassG_;
  int64_t operationSum_;
  uint8_t operationCount_;
  uint32_t operationStartedMs_;
  uint32_t lastSampleMs_;
  bool resultPending_;
  LoadCellCalibrationResult result_;
  MedianFilter<int32_t, 7> rawMedianFilter_;
};
