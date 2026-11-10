#include "LoadCellSensor.h"
#include <cassert>
#include <cmath>
#include <limits>

int main() {
  LoadCellSensor sensor;
  assert(sensor.begin(10, 6, {0, 100.0f}));
  sensor.update(100);
  assert(sensor.reading().grams == 10.0f);
  assert(sensor.requestTare(200));
  assert(!sensor.requestTare(201));
  assert(!sensor.requestKnownMassCalibration(100.0f, 201));
  HX711::raw = 2000;
  for (uint32_t i = 0; i < 16; ++i) sensor.update(300 + i * 100);
  LoadCellCalibrationResult result;
  assert(sensor.takeCalibrationResult(result));
  assert(result.success);
  assert(sensor.calibration().offset == 2000);
  assert(sensor.reading().raw == 2000);
  assert(sensor.reading().grams == 0.0f);
  assert(!sensor.reading().busy);
  sensor.update(2000);
  assert(sensor.reading().grams == 0.0f);
  assert(sensor.requestKnownMassCalibration(100.0f, 2100));
  HX711::raw = 12000;
  for (uint32_t i = 0; i < 16; ++i) sensor.update(2200 + i * 100);
  assert(sensor.takeCalibrationResult(result));
  assert(result.success);
  assert(std::fabs(sensor.reading().grams - 100.0f) < 0.001f);
  assert(sensor.requestTare(4000));
  HX711::ready = false;
  sensor.update(9000);
  assert(sensor.takeCalibrationResult(result));
  assert(!result.success);
  assert(!sensor.isBusy());
  assert(sensor.calibration().offset == 2000);
  HX711::ready = true;
  assert(sensor.requestTare(10000));
  HX711::raw = std::numeric_limits<int32_t>::max();
  sensor.update(15000);
  assert(sensor.takeCalibrationResult(result));
  assert(!result.success);
  assert(!sensor.isBusy());
}
