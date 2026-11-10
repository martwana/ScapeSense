#include "StatusJson.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

static bool contains(const std::string &text, const std::string &part) {
  return text.find(part) != std::string::npos;
}

static BubbleMetrics sampleMetrics() {
  BubbleMetrics metrics;
  metrics.totalBubbles = 42;
  metrics.bubblesPerSecond = 2.5f;
  metrics.bubblesPerMinute = 150.0f;
  metrics.baseline = 740;
  metrics.lastRaw = 512;
  return metrics;
}

void test_json_contains_counts_and_rates() {
  std::string json = buildStatusJson(sampleMetrics(), {-55, 90});
  TEST_ASSERT_TRUE(contains(json, "\"total\":42"));
  TEST_ASSERT_TRUE(contains(json, "\"bps\":2.50"));
  TEST_ASSERT_TRUE(contains(json, "\"bpm\":150.0"));
}

void test_json_contains_sensor_values() {
  std::string json = buildStatusJson(sampleMetrics(), {-55, 90});
  TEST_ASSERT_TRUE(contains(json, "\"baseline\":740"));
  TEST_ASSERT_TRUE(contains(json, "\"raw\":512"));
}

void test_json_contains_device_values() {
  std::string json = buildStatusJson(sampleMetrics(), {-55, 90});
  TEST_ASSERT_TRUE(contains(json, "\"rssi\":-55"));
  TEST_ASSERT_TRUE(contains(json, "\"uptime\":90"));
}

void test_json_contains_temperature_values() {
  DeviceInfo device{};
  device.temperature = TemperatureReading(24.6f, true);
  device.temperatureSettings = {25.0f, 1.5f};
  device.bubbleTargetBps = 1.25f;
  device.notifications.enabled = true;
  std::strcpy(device.notifications.projectId, "tank-project");
  std::memset(device.notifications.deviceId, 'a', 32);
  std::strcpy(device.notifications.deviceKey, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopq");
  device.notificationDelivery.sent = 3;
  device.notificationDelivery.lastHttp = 202;
  const std::string json = buildStatusJson(sampleMetrics(), device);
  TEST_ASSERT_TRUE(contains(json, "\"temperatureTargetC\":25.0"));
  TEST_ASSERT_TRUE(contains(json, "\"temperatureToleranceC\":1.5"));
  TEST_ASSERT_TRUE(contains(json, "\"bubbleTargetBps\":1.25"));
  TEST_ASSERT_TRUE(contains(json, "\"notificationsEnabled\":true"));
  TEST_ASSERT_TRUE(contains(json, "\"notificationsConfigured\":true"));
  TEST_ASSERT_TRUE(contains(json, "\"notificationsSent\":3"));
  TEST_ASSERT_TRUE(contains(json, "\"notificationsLastHttp\":202"));
  TEST_ASSERT_FALSE(contains(json, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
  TEST_ASSERT_EQUAL_CHAR('}', json.back());
  TEST_ASSERT_TRUE(contains(json, "\"temperatureC\":24.6"));
  TEST_ASSERT_TRUE(contains(json, "\"temperatureValid\":true"));
}

void test_json_contains_load_cell_values() {
  DeviceInfo device{};
  device.loadCell = LoadCellReading(12345, 7200.0f, true, true, false);
  device.emptyCylinderWeightG = 6000.0f;
  device.co2CapacityG = 2000.0f;
  const std::string json = buildStatusJson(sampleMetrics(), device);
  TEST_ASSERT_TRUE(contains(json, "\"weightG\":7200.0"));
  TEST_ASSERT_TRUE(contains(json, "\"weightValid\":true"));
  TEST_ASSERT_TRUE(contains(json, "\"loadCellRaw\":12345"));
  TEST_ASSERT_TRUE(contains(json, "\"loadCellCalibrated\":true"));
  TEST_ASSERT_TRUE(contains(json, "\"gasRemainingG\":1200.0"));
  TEST_ASSERT_TRUE(contains(json, "\"gasRemainingPercent\":60.0"));
  TEST_ASSERT_TRUE(contains(json, "\"gasConfigured\":true"));
}

void test_json_contains_display_and_firmware_values() {
  DeviceInfo device{-55, 90, false, "0.2.0"};
  const std::string json = buildStatusJson(sampleMetrics(), device);
  TEST_ASSERT_TRUE(contains(json, "\"displayAwake\":false"));
  TEST_ASSERT_TRUE(contains(json, "\"firmware\":\"0.2.0\""));
}

void test_json_reports_reliable_flag() {
  BubbleMetrics metrics = sampleMetrics();
  TEST_ASSERT_TRUE(contains(buildStatusJson(metrics, {}), "\"reliable\":true"));
  metrics.rateReliable = false;
  TEST_ASSERT_TRUE(
      contains(buildStatusJson(metrics, {}), "\"reliable\":false"));
}

void test_json_names_sensor_status() {
  BubbleMetrics metrics = sampleMetrics();
  TEST_ASSERT_TRUE(contains(buildStatusJson(metrics, {}), "\"status\":\"ok\""));
  metrics.status = BubbleStatus::Saturated;
  TEST_ASSERT_TRUE(
      contains(buildStatusJson(metrics, {}), "\"status\":\"saturated\""));
  metrics.status = BubbleStatus::Disconnected;
  TEST_ASSERT_TRUE(
      contains(buildStatusJson(metrics, {}), "\"status\":\"disconnected\""));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_json_contains_counts_and_rates);
  RUN_TEST(test_json_contains_sensor_values);
  RUN_TEST(test_json_contains_device_values);
  RUN_TEST(test_json_contains_temperature_values);
  RUN_TEST(test_json_contains_load_cell_values);
  RUN_TEST(test_json_contains_display_and_firmware_values);
  RUN_TEST(test_json_reports_reliable_flag);
  RUN_TEST(test_json_names_sensor_status);
  return UNITY_END();
}
