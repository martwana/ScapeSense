#include "LoadCellReading.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_counts_convert_to_grams() {
  const LoadCellCalibration calibration{1000, 20.0f};
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, countsToGrams(3000, calibration));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -50.0f, countsToGrams(0, calibration));
}

void test_uncalibrated_reading_has_no_weight() {
  const LoadCellCalibration calibration{1000, 0.0f};
  TEST_ASSERT_FALSE(isCalibratedLoadCell(calibration));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, countsToGrams(3000, calibration));
}

void test_calibration_calculates_scale_from_known_mass() {
  float scale = 0.0f;
  TEST_ASSERT_TRUE(calculateLoadCellScale(7000, 1000, 300.0f, scale));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, scale);
}

void test_calibration_rejects_zero_difference_or_bad_mass() {
  float scale = 2.0f;
  TEST_ASSERT_FALSE(calculateLoadCellScale(1000, 1000, 100.0f, scale));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, scale);
  TEST_ASSERT_FALSE(calculateLoadCellScale(2000, 1000, 0.0f, scale));
}

void test_gas_remaining_subtracts_empty_cylinder() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1200.0f, gasRemainingG(7200.0f, 6000.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, gasRemainingG(5900.0f, 6000.0f));
}

void test_gas_percent_uses_configured_capacity_and_clamps() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 60.0f,
                           gasRemainingPercent(7200.0f, 6000.0f, 2000.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f,
                           gasRemainingPercent(9000.0f, 6000.0f, 2000.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f,
                           gasRemainingPercent(7000.0f, 0.0f, 2000.0f));
}

void test_hx711_edge_samples_are_rejected() {
  TEST_ASSERT_FALSE(isValidHx711Sample(std::numeric_limits<int32_t>::min()));
  TEST_ASSERT_FALSE(isValidHx711Sample(std::numeric_limits<int32_t>::max()));
  TEST_ASSERT_TRUE(isValidHx711Sample(-1));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_counts_convert_to_grams);
  RUN_TEST(test_uncalibrated_reading_has_no_weight);
  RUN_TEST(test_calibration_calculates_scale_from_known_mass);
  RUN_TEST(test_calibration_rejects_zero_difference_or_bad_mass);
  RUN_TEST(test_gas_remaining_subtracts_empty_cylinder);
  RUN_TEST(test_gas_percent_uses_configured_capacity_and_clamps);
  RUN_TEST(test_hx711_edge_samples_are_rejected);
  return UNITY_END();
}
