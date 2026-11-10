#include "WeightFormat.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_weight_format_uses_whole_grams() {
  TEST_ASSERT_EQUAL_STRING("1 g", formatWeightG(1.0f).c_str());
  TEST_ASSERT_EQUAL_STRING("8 g", formatWeightG(8.44f).c_str());
  TEST_ASSERT_EQUAL_STRING("500 g", formatWeightG(500.0f).c_str());
  TEST_ASSERT_EQUAL_STRING("-8 g", formatWeightG(-8.44f).c_str());
  TEST_ASSERT_EQUAL_STRING("0 g", formatWeightG(-0.2f).c_str());
}

void test_weight_format_uses_compact_kilograms() {
  TEST_ASSERT_EQUAL_STRING("0.6 kg", formatWeightG(600.0f).c_str());
  TEST_ASSERT_EQUAL_STRING("1.1 kg", formatWeightG(1100.0f).c_str());
  TEST_ASSERT_EQUAL_STRING("5.12 kg", formatWeightG(5120.0f).c_str());
  TEST_ASSERT_EQUAL_STRING("1 kg", formatWeightG(1000.0f).c_str());
  TEST_ASSERT_EQUAL_STRING("1.25 kg", formatWeightG(1250.0f).c_str());
  TEST_ASSERT_EQUAL_STRING("-0.6 kg", formatWeightG(-600.0f).c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_weight_format_uses_whole_grams);
  RUN_TEST(test_weight_format_uses_compact_kilograms);
  return UNITY_END();
}
