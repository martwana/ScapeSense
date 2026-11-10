#include "OledType.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_external_128x32_display_has_expected_geometry() {
  const OledGeometry geometry = oledGeometry(OledType::SSD1306_128x32);
  TEST_ASSERT_EQUAL_UINT16(128, geometry.width);
  TEST_ASSERT_EQUAL_UINT16(32, geometry.height);
}

void test_external_128x32_type_value_is_stable() {
  TEST_ASSERT_EQUAL_UINT8(3, static_cast<uint8_t>(OledType::SSD1306_128x32));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_external_128x32_display_has_expected_geometry);
  RUN_TEST(test_external_128x32_type_value_is_stable);
  return UNITY_END();
}
