#include "LoadCellSensor.h"

LoadCellSensor::LoadCellSensor()
    : hx711_(nullptr), reading_{}, operation_(Operation::None),
      pendingKnownMassG_(0.0f), operationSum_(0), operationCount_(0),
      operationStartedMs_(0), lastSampleMs_(0), resultPending_(false),
      result_{} {}

LoadCellSensor::~LoadCellSensor() { delete hx711_; }

bool LoadCellSensor::begin(uint8_t doutPin, uint8_t sckPin,
                           const LoadCellCalibration &calibration) {
  if (hx711_) {
    return true;
  }
  calibration_ = calibration;
  hx711_ = new HX711();
  hx711_->begin(doutPin, sckPin);
  return true;
}

void LoadCellSensor::update(uint32_t nowMs) {
  if (!hx711_) {
    return;
  }
  if (operation_ != Operation::None &&
      nowMs - operationStartedMs_ >= kCalibrationTimeoutMs) {
    failCalibration();
  }
  if (!hx711_->is_ready()) {
    if (reading_.ready && nowMs - lastSampleMs_ > kReadingTimeoutMs) {
      reading_.ready = false;
    }
    reading_.busy = operation_ != Operation::None;
    return;
  }

  const int32_t raw = static_cast<int32_t>(hx711_->read());
  lastSampleMs_ = nowMs;
  lastRawSample_ = raw;
  if (!isValidHx711Sample(raw)) {
    reading_.ready = false;
    return;
  }

  rawMedianFilter_.add(raw);
  const int32_t medianRaw = rawMedianFilter_.value();
  const float filtered =
      reading_.ready
          ? kFilterAlpha * medianRaw + (1.0f - kFilterAlpha) * reading_.raw
          : static_cast<float>(medianRaw);
  reading_.raw = static_cast<int32_t>(filtered);
  reading_.ready = true;
  reading_.calibrated = isCalibratedLoadCell(calibration_);
  reading_.grams = countsToGrams(reading_.raw, calibration_);
  reading_.busy = operation_ != Operation::None;
  if (operation_ != Operation::None) {
    collectCalibrationSample(raw, nowMs);
  }
}

LoadCellReading LoadCellSensor::reading() const { return reading_; }

LoadCellCalibration LoadCellSensor::calibration() const { return calibration_; }

bool LoadCellSensor::isInitialized() const { return hx711_ != nullptr; }

bool LoadCellSensor::isBusy() const { return operation_ != Operation::None; }

bool LoadCellSensor::requestTare(uint32_t nowMs) {
  if (!hx711_ || operation_ != Operation::None) {
    return false;
  }
  operation_ = Operation::Tare;
  operationSum_ = 0;
  operationCount_ = 0;
  operationStartedMs_ = nowMs;
  reading_.busy = true;
  return true;
}

bool LoadCellSensor::requestKnownMassCalibration(float knownMassG,
                                                 uint32_t nowMs) {
  if (!hx711_ || operation_ != Operation::None ||
      !isValidKnownMass(knownMassG)) {
    return false;
  }
  pendingKnownMassG_ = knownMassG;
  operation_ = Operation::Calibrate;
  operationSum_ = 0;
  operationCount_ = 0;
  operationStartedMs_ = nowMs;
  reading_.busy = true;
  return true;
}

bool LoadCellSensor::takeCalibrationResult(LoadCellCalibrationResult &result) {
  if (!resultPending_) {
    return false;
  }
  result = result_;
  resultPending_ = false;
  return true;
}

void LoadCellSensor::collectCalibrationSample(int32_t raw, uint32_t nowMs) {
  if (nowMs - operationStartedMs_ >= kCalibrationTimeoutMs) {
    failCalibration();
    return;
  }
  operationSum_ += raw;
  ++operationCount_;
  if (operationCount_ < kCalibrationSamples) {
    return;
  }
  finishCalibration(static_cast<int32_t>(operationSum_ / operationCount_));
  operation_ = Operation::None;
  operationSum_ = 0;
  operationCount_ = 0;
}

void LoadCellSensor::finishCalibration(int32_t rawAverage) {
  result_.rawAverage = rawAverage;
  if (operation_ == Operation::Tare) {
    result_.action = LoadCellCalibrationAction::Tare;
    result_.success = isValidHx711Sample(rawAverage);
    if (result_.success) {
      calibration_.offset = rawAverage;
    }
  } else {
    result_.action = LoadCellCalibrationAction::KnownMass;
    float scale = calibration_.scale;
    result_.success = calculateLoadCellScale(rawAverage, calibration_.offset,
                                             pendingKnownMassG_, scale);
    if (result_.success) {
      calibration_.scale = scale;
    }
  }
  if (result_.success) {
    rawMedianFilter_ = MedianFilter<int32_t, 7>();
    rawMedianFilter_.add(rawAverage);
    reading_.raw = rawAverage;
  }
  result_.offset = calibration_.offset;
  result_.scale = calibration_.scale;
  reading_.calibrated = isCalibratedLoadCell(calibration_);
  reading_.grams = countsToGrams(reading_.raw, calibration_);
  reading_.busy = false;
  resultPending_ = true;
}

void LoadCellSensor::failCalibration() {
  result_.action = operation_ == Operation::Tare
                       ? LoadCellCalibrationAction::Tare
                       : LoadCellCalibrationAction::KnownMass;
  result_.success = false;
  result_.offset = calibration_.offset;
  result_.scale = calibration_.scale;
  operation_ = Operation::None;
  operationSum_ = 0;
  operationCount_ = 0;
  reading_.busy = false;
  resultPending_ = true;
}
