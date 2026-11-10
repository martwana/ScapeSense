#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

struct LoadCellReading {
  LoadCellReading(int32_t rawValue = 0, float gramsValue = 0.0f,
                  bool readyValue = false, bool calibratedValue = false,
                  bool busyValue = false)
      : raw(rawValue), grams(gramsValue), ready(readyValue),
        calibrated(calibratedValue), busy(busyValue) {}

  int32_t raw;
  float grams;
  bool ready;
  bool calibrated;
  bool busy;
};

struct LoadCellCalibration {
  LoadCellCalibration(int32_t offsetValue = 0, float scaleValue = 0.0f)
      : offset(offsetValue), scale(scaleValue) {}

  int32_t offset;
  float scale;
};

constexpr float kMinKnownLoadCellMassG = 0.1f;
constexpr float kMaxKnownLoadCellMassG = 100000.0f;
constexpr float kMinimumLoadCellScale = 0.000001f;
constexpr float kMaxCylinderParameterG = 100000.0f;

inline bool isValidCylinderParameter(float grams) {
  return std::isfinite(grams) && grams >= 0.0f &&
         grams <= kMaxCylinderParameterG;
}

inline bool isValidCylinderConfiguration(float emptyCylinderG,
                                         float capacityG) {
  return isValidCylinderParameter(emptyCylinderG) && emptyCylinderG > 0.0f &&
         isValidCylinderParameter(capacityG) && capacityG > 0.0f;
}

inline float gasRemainingG(float grossWeightG, float emptyCylinderG) {
  if (!isValidCylinderParameter(grossWeightG) ||
      !isValidCylinderParameter(emptyCylinderG)) {
    return 0.0f;
  }
  return grossWeightG > emptyCylinderG ? grossWeightG - emptyCylinderG : 0.0f;
}

inline float gasRemainingPercent(float grossWeightG, float emptyCylinderG,
                                 float capacityG) {
  if (!isValidCylinderConfiguration(emptyCylinderG, capacityG)) {
    return 0.0f;
  }
  const float percent =
      gasRemainingG(grossWeightG, emptyCylinderG) * 100.0f / capacityG;
  return percent > 100.0f ? 100.0f : percent;
}

inline bool isValidKnownMass(float grams) {
  return std::isfinite(grams) && grams >= kMinKnownLoadCellMassG &&
         grams <= kMaxKnownLoadCellMassG;
}

inline bool isValidLoadCellScale(float scale) {
  return std::isfinite(scale) && std::fabs(scale) > kMinimumLoadCellScale;
}

inline bool isCalibratedLoadCell(const LoadCellCalibration &calibration) {
  return isValidLoadCellScale(calibration.scale);
}

constexpr bool isValidHx711Sample(int32_t raw) {
  return raw > std::numeric_limits<int32_t>::min() &&
         raw < std::numeric_limits<int32_t>::max();
}

inline float countsToGrams(int32_t raw,
                           const LoadCellCalibration &calibration) {
  return isValidHx711Sample(raw) && isCalibratedLoadCell(calibration)
             ? (static_cast<float>(raw) -
                static_cast<float>(calibration.offset)) /
                   calibration.scale
             : 0.0f;
}

inline bool calculateLoadCellScale(int32_t loadedRaw, int32_t offset,
                                   float knownMassG, float &scale) {
  if (!isValidHx711Sample(loadedRaw) || !isValidKnownMass(knownMassG)) {
    return false;
  }
  const int64_t difference =
      static_cast<int64_t>(loadedRaw) - static_cast<int64_t>(offset);
  if (difference == 0) {
    return false;
  }
  const float candidate = static_cast<float>(difference) / knownMassG;
  if (!isValidLoadCellScale(candidate)) {
    return false;
  }
  scale = candidate;
  return true;
}
