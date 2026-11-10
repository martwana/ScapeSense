#include "DailyStatsJson.h"
#include <cstring>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_json_contains_current_day() {
  DailyStatsState state;
  state.currentDayKey = 20261012;
  state.currentCount = 59000;
  state.startEpoch = 1000;
  state.stopEpoch = 2000;
  state.active = false;
  state.activeSeconds = 250;
  state.historyCount = 1;
  state.history[0] = {20261011, 30, 100, 200, 50};
  const std::string json = buildDailyStatsJson(state);
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"day\":20261012"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"count\":59000"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"start\":1000"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"stop\":2000"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"active\":false"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"activeSeconds\":250"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"activeSeconds\":50"));
}

void test_json_contains_completed_history() {
  DailyStatsState state;
  state.history[0] = {20261011, 58000, 100, 200};
  state.historyCount = 1;
  const std::string json = buildDailyStatsJson(state);
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"history\":[{"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"day\":20261011"));
  TEST_ASSERT_NOT_NULL(strstr(json.c_str(), "\"count\":58000"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_json_contains_current_day);
  RUN_TEST(test_json_contains_completed_history);
  return UNITY_END();
}
