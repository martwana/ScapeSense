#include "RemoteNotifications.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

static DailyStatsState activeState(uint32_t day = 20261109, uint32_t count = 50,
                                   uint32_t start = 1000, uint32_t last = 1100) {
  DailyStatsState state{};
  state.currentDayKey = day;
  state.currentCount = count;
  state.startEpoch = start;
  state.lastQualifiedEpoch = last;
  state.active = true;
  state.recentCount = 12;
  state.recentBubbles.fill(1000);
  return state;
}

void test_notification_config_accepts_defaults_and_valid_credentials() {
  RemoteNotificationConfig config{};
  TEST_ASSERT_TRUE(validNotificationConfig(config));
  TEST_ASSERT_FALSE(validNotificationIdentity(config));
  config.enabled = true;
  TEST_ASSERT_FALSE(validNotificationConfig(config));
  std::strcpy(config.projectId, "tank-project");
  std::memset(config.deviceId, 'a', 32);
  std::strcpy(config.deviceKey, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopq");
  TEST_ASSERT_EQUAL_UINT32(43, std::strlen(config.deviceKey));
  TEST_ASSERT_TRUE(validNotificationIdentity(config));
  TEST_ASSERT_TRUE(validNotificationConfig(config));
  config.enabled = false;
  TEST_ASSERT_TRUE(validNotificationConfig(config));
}

void test_notification_config_rejects_injection_truncation_and_invalid_keys() {
  RemoteNotificationConfig config{};
  config.enabled = true;
  std::strcpy(config.projectId, "tank-project");
  std::memset(config.deviceId, 'b', 32);
  std::strcpy(config.deviceKey, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopq");
  RemoteNotificationConfig bad = config;
  std::strcpy(bad.projectId, "tank.example");
  TEST_ASSERT_FALSE(validNotificationConfig(bad));
  bad = config;
  bad.deviceKey[0] = '\0';
  TEST_ASSERT_FALSE(validNotificationConfig(bad));
  bad = config;
  bad.deviceKey[42] = '.';
  TEST_ASSERT_FALSE(validNotificationConfig(bad));
  bad = config;
  std::memset(bad.deviceId, 'A', 32);
  TEST_ASSERT_FALSE(validNotificationConfig(bad));
  bad = config;
  std::memset(bad.projectId, 'x', sizeof(bad.projectId));
  TEST_ASSERT_FALSE(terminated(bad.projectId));
  TEST_ASSERT_FALSE(validNotificationConfig(bad));
}

void test_blank_key_preserves_only_existing_pairing_for_same_identity() {
  RemoteNotificationConfig saved{};
  saved.enabled = true;
  std::strcpy(saved.projectId, "tank-project");
  std::memset(saved.deviceId, 'c', 32);
  std::strcpy(saved.deviceKey, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopq");
  RemoteNotificationConfig request{};
  request.enabled = false;
  std::strcpy(request.projectId, saved.projectId);
  std::strcpy(request.deviceId, saved.deviceId);
  TEST_ASSERT_TRUE(mergeNotificationConfig(saved, request));
  TEST_ASSERT_EQUAL_STRING(saved.deviceKey, request.deviceKey);
  request.enabled = true;
  std::strcpy(request.projectId, "other-project");
  request.deviceKey[0] = '\0';
  TEST_ASSERT_FALSE(mergeNotificationConfig(saved, request));
  TEST_ASSERT_FALSE(mergeNotificationConfig(RemoteNotificationConfig{}, request));
}

void test_flow_transitions_emit_once_and_confirm_stop_after_tolerance() {
  FlowNotificationTracker tracker;
  FlowNotification event;
  DailyStatsState idle{};
  idle.currentDayKey = 20261109;
  tracker.restore(idle);
  TEST_ASSERT_FALSE(tracker.observe(idle, 1000, event));
  DailyStatsState state = activeState();
  TEST_ASSERT_TRUE(tracker.observe(state, 1100, event));
  TEST_ASSERT_TRUE(event.started);
  TEST_ASSERT_EQUAL_UINT32(1000, event.occurredAt);
  TEST_ASSERT_EQUAL_UINT32(50, event.total);
  TEST_ASSERT_FALSE(tracker.observe(state, 1200, event));
  state.active = false;
  state.stopEpoch = 1100;
  TEST_ASSERT_FALSE(tracker.observe(state, 1399, event));
  TEST_ASSERT_TRUE(tracker.observe(state, 1400, event));
  TEST_ASSERT_FALSE(event.started);
  TEST_ASSERT_EQUAL_UINT32(1100, event.occurredAt);
  TEST_ASSERT_EQUAL_UINT32(50, event.total);
  TEST_ASSERT_FALSE(tracker.observe(state, 2000, event));
}

void test_flow_that_runs_through_midnight_is_not_reported_as_stopping() {
  FlowNotificationTracker tracker;
  tracker.restore(activeState(20261109, 500, 1100, 1500));
  FlowNotification event;
  DailyStatsState nextDay{};
  nextDay.currentDayKey = 20261110;
  TEST_ASSERT_FALSE(tracker.observe(nextDay, 1700, event));
  nextDay = activeState(20261110, 12, 1760, 1800);
  nextDay.recentBubbles.fill(1760);
  TEST_ASSERT_FALSE(tracker.observe(nextDay, 1800, event));
  nextDay.active = false;
  TEST_ASSERT_TRUE(tracker.observe(nextDay, 1800 + DailyStats::kStopToleranceSeconds, event));
  TEST_ASSERT_EQUAL_UINT32(12, event.total);
}

void test_recovered_active_state_does_not_send_duplicate_start() {
  FlowNotificationTracker tracker;
  tracker.restore(activeState());
  DailyStatsState state = activeState();
  FlowNotification event;
  TEST_ASSERT_FALSE(tracker.observe(state, 1200, event));
}

void test_event_payload_has_stable_id_and_contains_no_device_secret() {
  const FlowNotification event{true, 1762689540, 1234, 1.25f};
  const std::string json = notificationEventJson(event);
  char expectedId[40];
  std::snprintf(expectedId, sizeof(expectedId), "\"eventId\":\"%08lx%08lx\"", 1UL, 1762689540UL);
  TEST_ASSERT_TRUE(json.find(expectedId) != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"kind\":\"started\"") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"total\":1234") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("\"bps\":1.25") != std::string::npos);
  TEST_ASSERT_TRUE(json.find("deviceKey") == std::string::npos);
  const std::string invalidRate = notificationEventJson({false, 1762689540, 1234, NAN});
  TEST_ASSERT_TRUE(invalidRate.find("\"kind\":\"stopped\"") != std::string::npos);
  TEST_ASSERT_TRUE(invalidRate.find("\"bps\":0.00") != std::string::npos);
}

void test_network_retries_use_capped_jittered_backoff_and_auth_failures_stop() {
  TEST_ASSERT_TRUE(notificationRetryable(-1));
  TEST_ASSERT_TRUE(notificationRetryable(408));
  TEST_ASSERT_TRUE(notificationRetryable(429));
  TEST_ASSERT_TRUE(notificationRetryable(500));
  TEST_ASSERT_FALSE(notificationRetryable(202));
  TEST_ASSERT_FALSE(notificationRetryable(401));
  TEST_ASSERT_EQUAL_UINT32(1000, notificationRetryDelay(0, 0));
  TEST_ASSERT_EQUAL_UINT32(29999, notificationRetryDelay(0, 28999));
  TEST_ASSERT_EQUAL_UINT32(900000, notificationRetryDelay(20, 0));
  TEST_ASSERT_TRUE(notificationRetryDelay(20, 1) <= 930000);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_notification_config_accepts_defaults_and_valid_credentials);
  RUN_TEST(test_notification_config_rejects_injection_truncation_and_invalid_keys);
  RUN_TEST(test_blank_key_preserves_only_existing_pairing_for_same_identity);
  RUN_TEST(test_flow_transitions_emit_once_and_confirm_stop_after_tolerance);
  RUN_TEST(test_flow_that_runs_through_midnight_is_not_reported_as_stopping);
  RUN_TEST(test_recovered_active_state_does_not_send_duplicate_start);
  RUN_TEST(test_event_payload_has_stable_id_and_contains_no_device_secret);
  RUN_TEST(test_network_retries_use_capped_jittered_backoff_and_auth_failures_stop);
  return UNITY_END();
}
