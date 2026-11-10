#include "MonitoringSettings.h"
#include <limits>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_temperature_defaults_and_boundaries() {
  TEST_ASSERT_TRUE(validTemperatureSettings({}));
  TEST_ASSERT_TRUE(validTemperatureSettings({0.0f, 0.1f}));
  TEST_ASSERT_TRUE(validTemperatureSettings({40.0f, 10.0f}));
  TEST_ASSERT_EQUAL_FLOAT(24.0f, TemperatureSettings{}.targetC);
  TEST_ASSERT_EQUAL_FLOAT(2.0f, TemperatureSettings{}.toleranceC);
}

void test_temperature_rejects_invalid_values() {
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float inf = std::numeric_limits<float>::infinity();
  for (const auto settings : {TemperatureSettings{-0.1f, 2}, {40.1f, 2},
       {nan, 2}, {inf, 2}, {24, 0}, {24, -1}, {24, 10.1f}, {24, nan}, {24, inf}}) {
    TEST_ASSERT_FALSE(validTemperatureSettings(settings));
  }
}

void test_resets_require_exact_confirmation_and_same_origin() {
  TEST_ASSERT_TRUE(validResetConfirmation(false, "RESET WIFI", "http://scapesense.local", "scapesense.local"));
  TEST_ASSERT_TRUE(validResetConfirmation(true, "ERASE", "http://192.168.1.2:80", "192.168.1.2:80"));
  TEST_ASSERT_FALSE(validResetConfirmation(true, "erase", "http://scapesense.local", "scapesense.local"));
  TEST_ASSERT_FALSE(validResetConfirmation(true, "RESET WIFI", "http://scapesense.local", "scapesense.local"));
  TEST_ASSERT_FALSE(validResetConfirmation(false, "ERASE", "http://scapesense.local", "scapesense.local"));
  TEST_ASSERT_FALSE(validResetConfirmation(true, "ERASE", "http://other.local", "scapesense.local"));
  TEST_ASSERT_FALSE(validResetConfirmation(true, "ERASE", "http://scapesense.local.attacker", "scapesense.local"));
  TEST_ASSERT_FALSE(validResetConfirmation(true, "ERASE", "", "scapesense.local"));
  TEST_ASSERT_FALSE(validResetConfirmation(true, "ERASE", "http://", ""));
  TEST_ASSERT_FALSE(validResetConfirmation(true, nullptr, "http://scapesense.local", "scapesense.local"));
  TEST_ASSERT_FALSE(validResetConfirmation(true, "ERASE", nullptr, "scapesense.local"));
  TEST_ASSERT_FALSE(validResetConfirmation(true, "ERASE", "http://scapesense.local", nullptr));
}

void test_optional_bubble_target_validation() {
  TEST_ASSERT_TRUE(validBubbleTarget(0));
  TEST_ASSERT_TRUE(validBubbleTarget(1.5f));
  TEST_ASSERT_TRUE(validBubbleTarget(20));
  TEST_ASSERT_FALSE(validBubbleTarget(-1));
  TEST_ASSERT_FALSE(validBubbleTarget(20.1f));
  TEST_ASSERT_FALSE(validBubbleTarget(std::numeric_limits<float>::quiet_NaN()));
  TEST_ASSERT_FALSE(validBubbleTarget(std::numeric_limits<float>::infinity()));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_optional_bubble_target_validation);
  RUN_TEST(test_temperature_defaults_and_boundaries);
  RUN_TEST(test_temperature_rejects_invalid_values);
  RUN_TEST(test_resets_require_exact_confirmation_and_same_origin);
  return UNITY_END();
}
