#include "ProjectConfig.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_s3_hardware_defaults_are_forced() {
  TEST_ASSERT_EQUAL_UINT8(2, ProjectConfig::kDefaultBubbleSignalPin);
  TEST_ASSERT_EQUAL_UINT8(18, ProjectConfig::kDefaultIrEnablePin);
  TEST_ASSERT_EQUAL_UINT8(10, ProjectConfig::kDefaultLoadCellDoutPin);
  TEST_ASSERT_EQUAL_UINT8(6, ProjectConfig::kDefaultLoadCellSckPin);
  TEST_ASSERT_EQUAL_UINT8(0, ProjectConfig::kBootButtonPin);
  TEST_ASSERT_EQUAL_UINT8(1, ProjectConfig::kDefaultTemperaturePin);
  TEST_ASSERT_EQUAL_UINT8(4, ProjectConfig::kPhSensorPin);
  TEST_ASSERT_EQUAL_UINT8(5, ProjectConfig::kTdsSensorPin);
  TEST_ASSERT_EQUAL_UINT8(8, ProjectConfig::kCo2RelayPin);
  TEST_ASSERT_EQUAL_UINT8(9, ProjectConfig::kDosingPumpPin);
  TEST_ASSERT_EQUAL_UINT8(13, ProjectConfig::kStepperStepPin);
  TEST_ASSERT_EQUAL_UINT8(14, ProjectConfig::kStepperDirPin);
  TEST_ASSERT_EQUAL_UINT8(15, ProjectConfig::kStepperEnablePin);
  TEST_ASSERT_FALSE(ProjectConfig::kDisplayEnabled);
  TEST_ASSERT_TRUE(ProjectConfig::kForceHardwareDefaults);
}

void test_external_oled_defaults_are_forced() {
  TEST_ASSERT_EQUAL(OledType::SSD1306_128x32, ProjectConfig::kDefaultOledType);
  TEST_ASSERT_EQUAL_UINT8(5, ProjectConfig::kDefaultOledSdaPin);
  TEST_ASSERT_EQUAL_UINT8(4, ProjectConfig::kDefaultOledSclPin);
  TEST_ASSERT_TRUE(ProjectConfig::kForceOledDefaults);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_s3_hardware_defaults_are_forced);
  RUN_TEST(test_external_oled_defaults_are_forced);
  return UNITY_END();
}
