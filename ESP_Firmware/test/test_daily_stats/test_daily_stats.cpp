#include "DailyStats.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void addActiveMinute(DailyStats &stats, uint32_t day, uint32_t start) {
  for (uint32_t i = 0; i < 12; ++i) {
    stats.onBubble(day, start + i * 5);
  }
}

void test_stray_bubbles_do_not_start_activity() {
  DailyStats stats;
  for (uint32_t i = 0; i < 11; ++i) {
    stats.onBubble(20261012, 1000 + i * 5);
  }
  TEST_ASSERT_FALSE(stats.state().active);
  TEST_ASSERT_EQUAL_UINT32(0, stats.state().startEpoch);
  TEST_ASSERT_EQUAL_UINT32(11, stats.state().currentCount);
}

void test_sustained_point_two_bps_records_first_bubble_as_start() {
  DailyStats stats;
  addActiveMinute(stats, 20261012, 1000);
  TEST_ASSERT_TRUE(stats.state().active);
  TEST_ASSERT_EQUAL_UINT32(1000, stats.state().startEpoch);
  TEST_ASSERT_EQUAL_UINT32(12, stats.state().currentCount);
}

void test_stop_requires_five_minutes_below_threshold() {
  DailyStats stats;
  addActiveMinute(stats, 20261012, 1000);
  const uint32_t lastQualified = stats.state().lastQualifiedEpoch;
  stats.tick(20261012, lastQualified + 299);
  TEST_ASSERT_TRUE(stats.state().active);
  stats.tick(20261012, lastQualified + 300);
  TEST_ASSERT_FALSE(stats.state().active);
  TEST_ASSERT_EQUAL_UINT32(lastQualified, stats.state().stopEpoch);
  TEST_ASSERT_EQUAL_UINT32(55, stats.state().activeSeconds);
}

void test_flow_duration_excludes_idle_gaps_between_sessions() {
  DailyStats stats;
  addActiveMinute(stats, 20261012, 1000);
  stats.tick(20261012, stats.state().lastQualifiedEpoch + 300);
  TEST_ASSERT_EQUAL_UINT32(55, stats.state().activeSeconds);
  addActiveMinute(stats, 20261012, 2000);
  TEST_ASSERT_EQUAL_UINT32(110, DailyStats::currentFlowSeconds(stats.state()));
  stats.tick(20261012, stats.state().lastQualifiedEpoch + 300);
  TEST_ASSERT_EQUAL_UINT32(110, stats.state().activeSeconds);
}

void test_stray_bubble_does_not_extend_active_stop_time() {
  DailyStats stats;
  addActiveMinute(stats, 20261012, 1000);
  const uint32_t lastQualified = stats.state().lastQualifiedEpoch;
  stats.onBubble(20261012, lastQualified + 120);
  stats.tick(20261012, lastQualified + 300);
  TEST_ASSERT_FALSE(stats.state().active);
  TEST_ASSERT_EQUAL_UINT32(lastQualified, stats.state().stopEpoch);
}

void test_midnight_archives_day_and_resets_current_count() {
  DailyStats stats;
  addActiveMinute(stats, 20261012, 1000);
  stats.onBubble(20261012, 1100);
  TEST_ASSERT_TRUE(stats.tick(20261013, 2000));
  TEST_ASSERT_EQUAL_UINT32(0, stats.state().currentCount);
  TEST_ASSERT_EQUAL_UINT32(20261013, stats.state().currentDayKey);
  TEST_ASSERT_EQUAL_UINT32(1, stats.historyCount());
  TEST_ASSERT_EQUAL_UINT32(13, stats.history(0).bubbleCount);
  TEST_ASSERT_EQUAL_UINT32(20261012, stats.history(0).dayKey);
}

void test_history_saves_final_flow_duration_on_day_rollover() {
  DailyStats stats;
  addActiveMinute(stats, 20261012, 1000);
  stats.tick(20261013, 2000);
  TEST_ASSERT_EQUAL_UINT32(1, stats.historyCount());
  TEST_ASSERT_EQUAL_UINT32(55, stats.history(0).activeSeconds);
  TEST_ASSERT_EQUAL_UINT32(1055, stats.history(0).stopEpoch);
}

void test_legacy_state_migrates_with_history_and_a_flow_duration_estimate() {
  LegacyDailyStatsState legacy{};
  legacy.version = 1;
  legacy.currentDayKey = 20261012;
  legacy.currentCount = 20;
  legacy.startEpoch = 100;
  legacy.stopEpoch = 350;
  legacy.historyCount = 1;
  legacy.history[0] = {20261011, 30, 50, 200};
  DailyStatsState migrated;
  TEST_ASSERT_TRUE(migrateDailyStatsV1(legacy, migrated));
  TEST_ASSERT_EQUAL_UINT32(DailyStatsState::kVersion, migrated.version);
  TEST_ASSERT_EQUAL_UINT32(20, migrated.currentCount);
  TEST_ASSERT_EQUAL_UINT32(250, migrated.activeSeconds);
  TEST_ASSERT_EQUAL_UINT32(1, migrated.historyCount);
  TEST_ASSERT_EQUAL_UINT32(150, migrated.history[0].activeSeconds);
  TEST_ASSERT_TRUE(migrated.flowDurationEstimated);
  TEST_ASSERT_TRUE(migrated.history[0].flowDurationEstimated);
  legacy.version = 0;
  TEST_ASSERT_FALSE(migrateDailyStatsV1(legacy, migrated));
}

void test_history_retains_latest_thirty_days() {
  DailyStats stats;
  for (uint32_t day = 1; day <= 32; ++day) {
    stats.onBubble(20260000 + day, day * 1000);
  }
  TEST_ASSERT_EQUAL_UINT32(30, stats.historyCount());
  TEST_ASSERT_EQUAL_UINT32(20260002, stats.history(0).dayKey);
  TEST_ASSERT_EQUAL_UINT32(20260031, stats.history(29).dayKey);
}

void test_state_can_be_restored_after_restart() {
  DailyStats stats;
  addActiveMinute(stats, 20261012, 1000);
  DailyStats restored(stats.state());
  TEST_ASSERT_EQUAL_UINT32(12, restored.state().currentCount);
  TEST_ASSERT_EQUAL_UINT32(1000, restored.state().startEpoch);
  TEST_ASSERT_TRUE(restored.state().active);
}

void test_current_count_can_be_synchronized_after_time_arrives() {
  DailyStats stats;
  stats.synchronizeCurrentCount(20261012, 42);
  TEST_ASSERT_EQUAL_UINT32(20261012, stats.state().currentDayKey);
  TEST_ASSERT_EQUAL_UINT32(42, stats.state().currentCount);
}

void test_manual_reset_clears_current_day_but_keeps_history() {
  DailyStats stats;
  stats.onBubble(20261011, 1000);
  stats.tick(20261012, 2000);
  stats.onBubble(20261012, 2100);
  stats.resetCurrent(20261012);
  TEST_ASSERT_EQUAL_UINT32(0, stats.state().currentCount);
  TEST_ASSERT_EQUAL_UINT32(1, stats.historyCount());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_stray_bubbles_do_not_start_activity);
  RUN_TEST(test_sustained_point_two_bps_records_first_bubble_as_start);
  RUN_TEST(test_stop_requires_five_minutes_below_threshold);
  RUN_TEST(test_flow_duration_excludes_idle_gaps_between_sessions);
  RUN_TEST(test_stray_bubble_does_not_extend_active_stop_time);
  RUN_TEST(test_midnight_archives_day_and_resets_current_count);
  RUN_TEST(test_history_saves_final_flow_duration_on_day_rollover);
  RUN_TEST(test_legacy_state_migrates_with_history_and_a_flow_duration_estimate);
  RUN_TEST(test_history_retains_latest_thirty_days);
  RUN_TEST(test_state_can_be_restored_after_restart);
  RUN_TEST(test_current_count_can_be_synchronized_after_time_arrives);
  RUN_TEST(test_manual_reset_clears_current_day_but_keeps_history);
  return UNITY_END();
}
