#include "BubbleCounter.h"
#include <unity.h>

BubbleCounterConfig config;
BubbleCounter *counter = nullptr;

void setUp() {
  config = BubbleCounterConfig{};
  counter = new BubbleCounter(config);
}

void tearDown() {
  delete counter;
  counter = nullptr;
}

void feedBaseline(uint16_t value, uint32_t count, uint32_t startTimeMs = 0) {
  for (uint32_t i = 0; i < count; ++i) {
    counter->update(value, startTimeMs + i);
  }
}

void test_counter_restores_persisted_total_without_rate_history() {
  counter->restoreTotal(59000);
  TEST_ASSERT_EQUAL_UINT32(59000, counter->metrics().totalBubbles);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, counter->metrics().bubblesPerSecond);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, counter->metrics().bubblesPerMinute);
}

void test_counter_starts_with_zero_bubbles() {
  TEST_ASSERT_EQUAL_UINT32(0, counter->metrics().totalBubbles);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, counter->metrics().bubblesPerSecond);
}

void test_counter_detects_negative_polarity_bubble() {
  feedBaseline(800, 20);
  counter->update(500, 100);
  TEST_ASSERT_EQUAL_UINT32(1, counter->metrics().totalBubbles);
}

void test_counter_does_not_double_count_during_same_event() {
  feedBaseline(800, 20);
  counter->update(500, 100);
  counter->update(550, 110);
  counter->update(800, 120);
  counter->update(500, 130); // within minimum separation
  TEST_ASSERT_EQUAL_UINT32(1, counter->metrics().totalBubbles);
}

void test_counter_respects_minimum_separation() {
  feedBaseline(800, 20);
  counter->update(500, 1000);
  counter->update(800, 1010);
  counter->update(500, 1100); // only 100 ms later
  TEST_ASSERT_EQUAL_UINT32(1, counter->metrics().totalBubbles);

  counter->update(800, 1110);
  counter->update(500, 1250); // 150 ms after first event
  TEST_ASSERT_EQUAL_UINT32(2, counter->metrics().totalBubbles);
}

void test_counter_computes_bubbles_per_second() {
  feedBaseline(800, 20);
  counter->update(500, 1000);
  counter->update(800, 1010);
  counter->update(500, 2000);
  TEST_ASSERT_EQUAL_UINT32(2, counter->metrics().totalBubbles);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, counter->metrics().bubblesPerSecond);
}

void test_counter_zeroes_rate_after_idle_timeout() {
  feedBaseline(800, 20);
  counter->update(500, 1000);
  counter->update(800, 1010);
  feedBaseline(800, 10, 4000);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, counter->metrics().bubblesPerSecond);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, counter->metrics().bubblesPerMinute);
}

void test_counter_detects_positive_polarity_bubble() {
  BubbleCounterConfig posConfig;
  posConfig.polarity = 1;
  posConfig.threshold = 100;
  posConfig.hysteresis = 25;
  counter->setConfig(posConfig);
  counter->reset();

  feedBaseline(800, 20);
  counter->update(950, 100);
  counter->update(800, 110);
  TEST_ASSERT_EQUAL_UINT32(1, counter->metrics().totalBubbles);
}

void test_counter_reports_saturated_status() {
  feedBaseline(800, 20);
  for (uint32_t i = 0; i < 60; ++i) {
    counter->update(4095, i);
  }
  TEST_ASSERT_EQUAL(static_cast<int>(BubbleStatus::Saturated),
                    static_cast<int>(counter->metrics().status));
}

void test_counter_flags_unreliable_at_high_rate() {
  BubbleCounterConfig cfg;
  cfg.minSeparationMs = 20;
  cfg.maxReliableBps = 10;
  counter->setConfig(cfg);
  counter->reset();

  feedBaseline(800, 20);
  counter->update(500, 100);
  counter->update(800, 110);
  counter->update(500, 150);
  TEST_ASSERT_EQUAL_UINT32(2, counter->metrics().totalBubbles);
  TEST_ASSERT_FALSE(counter->metrics().rateReliable);
}

void test_reset_clears_counts_rates_and_measurement_history() {
  feedBaseline(800, 20);
  counter->update(500, 1000);
  counter->update(800, 1010);
  counter->update(500, 1500);
  TEST_ASSERT_EQUAL_UINT32(2, counter->metrics().totalBubbles);
  TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, counter->metrics().bubblesPerSecond);

  counter->reset();

  TEST_ASSERT_EQUAL_UINT32(0, counter->metrics().totalBubbles);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, counter->metrics().bubblesPerSecond);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, counter->metrics().bubblesPerMinute);
  TEST_ASSERT_EQUAL_UINT32(0, counter->metrics().lastBubbleTimestampMs);
  TEST_ASSERT_EQUAL_UINT16(0, counter->metrics().baseline);
  TEST_ASSERT_EQUAL_UINT16(0, counter->metrics().lastRaw);
  TEST_ASSERT_TRUE(counter->metrics().rateReliable);

  feedBaseline(800, 20, 2000);
  counter->update(500, 2100);
  TEST_ASSERT_EQUAL_UINT32(1, counter->metrics().totalBubbles);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, counter->metrics().bubblesPerSecond);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, counter->metrics().bubblesPerMinute);
}

void test_counter_reports_reliable_at_low_rate() {
  BubbleCounterConfig cfg;
  cfg.minSeparationMs = 20;
  cfg.maxReliableBps = 10;
  counter->setConfig(cfg);
  counter->reset();

  feedBaseline(800, 20);
  counter->update(500, 100);
  counter->update(800, 110);
  counter->update(500, 300);
  TEST_ASSERT_EQUAL_UINT32(2, counter->metrics().totalBubbles);
  TEST_ASSERT_TRUE(counter->metrics().rateReliable);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_counter_restores_persisted_total_without_rate_history);
  RUN_TEST(test_counter_starts_with_zero_bubbles);
  RUN_TEST(test_counter_detects_negative_polarity_bubble);
  RUN_TEST(test_counter_does_not_double_count_during_same_event);
  RUN_TEST(test_counter_respects_minimum_separation);
  RUN_TEST(test_counter_computes_bubbles_per_second);
  RUN_TEST(test_counter_zeroes_rate_after_idle_timeout);
  RUN_TEST(test_counter_detects_positive_polarity_bubble);
  RUN_TEST(test_counter_reports_saturated_status);
  RUN_TEST(test_counter_flags_unreliable_at_high_rate);
  RUN_TEST(test_reset_clears_counts_rates_and_measurement_history);
  RUN_TEST(test_counter_reports_reliable_at_low_rate);
  return UNITY_END();
}
