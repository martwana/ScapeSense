#include "IotProtocol.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

static IotConfig validConfig() {
  IotConfig config{};
  config.enabled = true;
  std::strcpy(config.endpoint, "a1b2c3d4e5f6-ats.iot.eu-west-2.amazonaws.com");
  std::strcpy(config.deviceId, "scapesense-dev-001");
  return config;
}

void test_iot_config_accepts_empty_and_valid() {
  IotConfig config{};
  TEST_ASSERT_TRUE(validIotConfig(config)); // disabled + empty = valid
  config = validConfig();
  TEST_ASSERT_TRUE(validIotConfig(config));
  config.enabled = false;
  TEST_ASSERT_FALSE(validIotConfig(config)); // disabled but fields set
}

void test_iot_endpoint_validation() {
  TEST_ASSERT_TRUE(validIotEndpoint("a1b2c3-ats.iot.eu-west-2.amazonaws.com"));
  TEST_ASSERT_FALSE(validIotEndpoint(""));                       // too short
  TEST_ASSERT_FALSE(validIotEndpoint("no-dots"));                // needs a dot
  TEST_ASSERT_FALSE(validIotEndpoint("-leading.iot.example"));   // label edge
  TEST_ASSERT_FALSE(validIotEndpoint("trailing-.iot.example"));
  TEST_ASSERT_FALSE(validIotEndpoint("double..dot.iot"));
  TEST_ASSERT_FALSE(validIotEndpoint("UPPER.iot.amazonaws.com"));
  TEST_ASSERT_FALSE(validIotEndpoint("bad_char!.iot.amazonaws.com"));
  TEST_ASSERT_FALSE(validIotEndpoint("spaces iot.amazonaws.com"));
  char tooLong[100]{};
  std::memset(tooLong, 'a', 97);
  TEST_ASSERT_FALSE(validIotEndpoint(tooLong));
}

void test_iot_device_id_validation() {
  TEST_ASSERT_TRUE(validIotDeviceId("scapesense-dev-001"));
  TEST_ASSERT_TRUE(validIotDeviceId("A"));
  TEST_ASSERT_TRUE(validIotDeviceId("thing_name-01"));
  TEST_ASSERT_FALSE(validIotDeviceId(""));
  TEST_ASSERT_FALSE(validIotDeviceId("bad name"));
  TEST_ASSERT_FALSE(validIotDeviceId("bad:name"));
  TEST_ASSERT_FALSE(validIotDeviceId("bad/name"));
  char tooLong[70]{};
  std::memset(tooLong, 'x', 65);
  TEST_ASSERT_FALSE(validIotDeviceId(tooLong));
}

void test_iot_interval_bounds() {
  TEST_ASSERT_TRUE(validIotIntervals(300000, 2000));
  TEST_ASSERT_TRUE(validIotIntervals(kIotBackgroundMinMs, kIotLiveMinMs));
  TEST_ASSERT_TRUE(validIotIntervals(kIotBackgroundMaxMs, kIotLiveMaxMs));
  TEST_ASSERT_FALSE(validIotIntervals(kIotBackgroundMinMs - 1, 2000));
  TEST_ASSERT_FALSE(validIotIntervals(kIotBackgroundMaxMs + 1, 2000));
  TEST_ASSERT_FALSE(validIotIntervals(300000, kIotLiveMinMs - 1));
  TEST_ASSERT_FALSE(validIotIntervals(300000, kIotLiveMaxMs + 1));
  TEST_ASSERT_FALSE(validIotIntervals(2000, 300000)); // live must not exceed background
}

void test_iot_config_rejects_bad_members() {
  IotConfig config = validConfig();
  config.version = 9;
  TEST_ASSERT_FALSE(validIotConfig(config));
  config = validConfig();
  std::strcpy(config.endpoint, "bad host name");
  TEST_ASSERT_FALSE(validIotConfig(config));
  config = validConfig();
  config.deviceId[0] = '/';
  TEST_ASSERT_FALSE(validIotConfig(config));
  config = validConfig();
  config.liveIntervalMs = config.backgroundIntervalMs + 1;
  TEST_ASSERT_FALSE(validIotConfig(config));
}

void test_iot_topics() {
  TEST_ASSERT_EQUAL_STRING("scapesense/dev-1/telemetry",
                           iotDeviceTopic("dev-1", "telemetry").c_str());
  TEST_ASSERT_EQUAL_STRING("scapesense/dev-1/commands",
                           iotDeviceTopic("dev-1", "commands").c_str());
}

void test_iot_telemetry_json() {
  IotTelemetry t{};
  t.epoch = 1762810000;
  t.uptimeSeconds = 3600;
  t.totalBubbles = 12345;
  t.bps = 0.42f;
  t.bpm = 25.2f;
  t.temperatureC = 24.5f;
  t.temperatureValid = true;
  t.weightG = 3500.25f;
  t.weightReady = true;
  t.gasRemainingPercent = 42.5f;
  t.flowActive = true;
  t.rssi = -55;
  const std::string json = iotTelemetryJson(t, "scapesense-dev-001", "1.2.3", true);
  TEST_ASSERT_TRUE(json.find("\"deviceId\":\"scapesense-dev-001\"") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"ts\":1762810000") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"bps\":0.42") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"total\":12345") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"tempC\":24.5") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"co2Pct\":42.5") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"rssi\":-55") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"fw\":\"1.2.3\"") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"mode\":\"live\"") != std::string::npos);
}

void test_iot_telemetry_json_sanitises_non_finite() {
  IotTelemetry t{};
  t.bps = std::nanf("");
  t.bpm = std::numeric_limits<float>::infinity();
  t.temperatureC = std::nanf("");
  const std::string json = iotTelemetryJson(t, "dev-1", "x", false);
  TEST_ASSERT_TRUE(json.find("nan") == std::string::npos);
  TEST_ASSERT_TRUE(json.find("inf") == std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"mode\":\"background\"") != std::string::npos);
}

void test_iot_parse_live_command() {
  IotCommand command;
  TEST_ASSERT_TRUE(iotParseCommand("{\"id\":\"cmd-7\",\"command\":\"live\",\"seconds\":120}", command));
  TEST_ASSERT_EQUAL_STRING("live", command.action);
  TEST_ASSERT_EQUAL_STRING("cmd-7", command.id);
  TEST_ASSERT_EQUAL_UINT32(120, command.seconds);
}

void test_iot_parse_ping_command() {
  IotCommand command;
  TEST_ASSERT_TRUE(iotParseCommand("{\"command\":\"ping\",\"id\":\"abc\"}", command));
  TEST_ASSERT_EQUAL_STRING("ping", command.action);
  TEST_ASSERT_EQUAL_STRING("abc", command.id);
}

void test_iot_parse_rejects_invalid_commands() {
  IotCommand command;
  TEST_ASSERT_FALSE(iotParseCommand("", command));
  TEST_ASSERT_FALSE(iotParseCommand("{}", command));
  TEST_ASSERT_FALSE(iotParseCommand("{\"command\":\"reboot\"}", command));
  TEST_ASSERT_FALSE(iotParseCommand("{\"command\":\"live\"}", command));           // missing seconds
  TEST_ASSERT_FALSE(iotParseCommand("{\"command\":\"live\",\"seconds\":5}", command));      // below min
  TEST_ASSERT_FALSE(iotParseCommand("{\"command\":\"live\",\"seconds\":7200}", command));   // above max
  TEST_ASSERT_FALSE(iotParseCommand("{\"command\":\"live\",\"seconds\":\"120\"}", command)); // not a number
  TEST_ASSERT_FALSE(iotParseCommand("{\"command\":\"live\",\"seconds\":42949672960}", command));
}

void test_iot_response_json() {
  IotCommand command{};
  std::strcpy(command.id, "cmd-7");
  std::strcpy(command.action, "live");
  const std::string json = iotCommandResponseJson(command, true, "live");
  TEST_ASSERT_TRUE(json.find("\"id\":\"cmd-7\"") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"ok\":true") != std::string::npos);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_iot_config_accepts_empty_and_valid);
  RUN_TEST(test_iot_endpoint_validation);
  RUN_TEST(test_iot_device_id_validation);
  RUN_TEST(test_iot_interval_bounds);
  RUN_TEST(test_iot_config_rejects_bad_members);
  RUN_TEST(test_iot_topics);
  RUN_TEST(test_iot_telemetry_json);
  RUN_TEST(test_iot_telemetry_json_sanitises_non_finite);
  RUN_TEST(test_iot_parse_live_command);
  RUN_TEST(test_iot_parse_ping_command);
  RUN_TEST(test_iot_parse_rejects_invalid_commands);
  RUN_TEST(test_iot_response_json);
  return UNITY_END();
}
