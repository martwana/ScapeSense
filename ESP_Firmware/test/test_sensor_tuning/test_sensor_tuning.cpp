#include "SensorTuning.h"
#include <cstring>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_default_tuning_is_valid() {
  TEST_ASSERT_EQUAL(static_cast<int>(SensorTuningError::None),
                    static_cast<int>(validateSensorTuning({})));
}

void test_threshold_limits_are_enforced() {
  SensorTuning tuning;
  tuning.threshold = 9;
  TEST_ASSERT_EQUAL(static_cast<int>(SensorTuningError::Threshold),
                    static_cast<int>(validateSensorTuning(tuning)));
  tuning.threshold = 1001;
  TEST_ASSERT_EQUAL(static_cast<int>(SensorTuningError::Threshold),
                    static_cast<int>(validateSensorTuning(tuning)));
}

void test_hysteresis_must_be_below_threshold() {
  SensorTuning tuning;
  tuning.hysteresis = tuning.threshold;
  TEST_ASSERT_EQUAL(static_cast<int>(SensorTuningError::Hysteresis),
                    static_cast<int>(validateSensorTuning(tuning)));
}

void test_separation_and_rate_limits_are_enforced() {
  SensorTuning tuning;
  tuning.minSeparationMs = 19;
  TEST_ASSERT_EQUAL(static_cast<int>(SensorTuningError::Separation),
                    static_cast<int>(validateSensorTuning(tuning)));
  tuning.minSeparationMs = 30;
  tuning.maxReliableBps = 21;
  TEST_ASSERT_EQUAL(static_cast<int>(SensorTuningError::ReliableRate),
                    static_cast<int>(validateSensorTuning(tuning)));
}

void test_tuning_json_contains_all_values() {
  SensorTuning tuning{200, 40, 80, 8, 1};
  const std::string json = buildSensorTuningJson(tuning);
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"threshold\":200"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"hysteresis\":40"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"minSeparationMs\":80"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"maxReliableBps\":8"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"polarity\":1"));
}

void test_polarity_is_binary() {
  SensorTuning tuning;
  tuning.polarity = 2;
  TEST_ASSERT_EQUAL(static_cast<int>(SensorTuningError::Polarity),
                    static_cast<int>(validateSensorTuning(tuning)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_default_tuning_is_valid);
  RUN_TEST(test_threshold_limits_are_enforced);
  RUN_TEST(test_hysteresis_must_be_below_threshold);
  RUN_TEST(test_separation_and_rate_limits_are_enforced);
  RUN_TEST(test_tuning_json_contains_all_values);
  RUN_TEST(test_polarity_is_binary);
  return UNITY_END();
}
