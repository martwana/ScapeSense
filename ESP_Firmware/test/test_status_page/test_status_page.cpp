#include "StatusPage.h"
#include <cstring>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_page_contains_scapesense_branding() {
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "<title>ScapeSense</title>"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "<span>ScapeSense</span>"));
  TEST_ASSERT_NULL(strstr(kStatusPage, "Bubble Counter"));
}

void test_page_contains_live_status_fields() {
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"bps\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"total\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"temperature\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"oled\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"firmware\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "new WebSocket"));
}

void test_page_confirms_and_posts_count_reset() {
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "confirm("));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "/api/reset-counts"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "method:'POST'"));
}

void test_page_contains_sensor_tuning_controls() {
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "Sensor tuning"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"threshold\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"hysteresis\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"separation\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"reliable\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"polarity\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "class=\"info\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "data-help="));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "10-1000 ADC"));
  TEST_ASSERT_NULL(strstr(kStatusPage, "–"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "/api/config/apply"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "/api/config/save"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "/api/config/defaults"));
}

void test_page_contains_live_sensor_diagnostics() {
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"raw\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"baseline\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"difference\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"graph\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "emaAlpha"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "thresholdLine"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "Detection threshold"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "class=\"legend"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "rgba(66,165,255,.25)"));
}

void test_page_contains_daily_history() {
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "Daily history"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "/api/daily-stats"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "Europe/London"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"historyRows\""));
}

void test_page_contains_load_cell_controls() {
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "Cylinder weighing"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"weight\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"weightUnit\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "massParts"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "g<10"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "toFixed(1)"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"loadCellRaw\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"knownMass\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"gasGauge\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"emptyCylinder\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "id=\"co2Capacity\""));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "/api/cylinder/config"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "/api/load-cell/tare"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "/api/load-cell/calibrate"));
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "knownMassG"));
}

void test_page_links_to_firmware_update() {
  TEST_ASSERT_NOT_NULL(strstr(kStatusPage, "href=\"/update\""));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_page_contains_scapesense_branding);
  RUN_TEST(test_page_contains_live_status_fields);
  RUN_TEST(test_page_confirms_and_posts_count_reset);
  RUN_TEST(test_page_contains_sensor_tuning_controls);
  RUN_TEST(test_page_contains_live_sensor_diagnostics);
  RUN_TEST(test_page_contains_daily_history);
  RUN_TEST(test_page_contains_load_cell_controls);
  RUN_TEST(test_page_links_to_firmware_update);
  return UNITY_END();
}
