#include "TemperatureReading.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_normal_temperature_is_valid() {
  TEST_ASSERT_TRUE(isValidDs18b20Temperature(24.6f));
}

void test_ds18b20_disconnected_value_is_invalid() {
  TEST_ASSERT_FALSE(isValidDs18b20Temperature(-127.0f));
}

void test_temperature_outside_sensor_range_is_invalid() {
  TEST_ASSERT_FALSE(isValidDs18b20Temperature(-55.1f));
  TEST_ASSERT_FALSE(isValidDs18b20Temperature(125.1f));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_normal_temperature_is_valid);
  RUN_TEST(test_ds18b20_disconnected_value_is_invalid);
  RUN_TEST(test_temperature_outside_sensor_range_is_invalid);
  return UNITY_END();
}
