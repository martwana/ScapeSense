#include "BubblePipeline.h"

#include <unity.h>

namespace {
BubbleCounterConfig makeConfig() {
  BubbleCounterConfig config;
  config.threshold = 100;
  config.hysteresis = 20;
  config.minSeparationMs = 0;
  config.maxReliableBps = 0;
  config.polarity = 0;
  return config;
}
} // namespace

void test_pipeline_reports_bubble_event() {
  BubblePipeline pipeline(makeConfig());
  TEST_ASSERT_FALSE(pipeline.update(1000, 0));
  TEST_ASSERT_TRUE(pipeline.update(800, 10));
  TEST_ASSERT_EQUAL_UINT32(1, pipeline.metrics().totalBubbles);
}

void test_pipeline_reports_event_only_when_count_increases() {
  BubblePipeline pipeline(makeConfig());
  pipeline.update(1000, 0);

  TEST_ASSERT_TRUE(pipeline.update(800, 10));
  TEST_ASSERT_FALSE(pipeline.update(800, 11));
  TEST_ASSERT_FALSE(pipeline.update(1000, 20));
  TEST_ASSERT_TRUE(pipeline.update(800, 30));
  TEST_ASSERT_EQUAL_UINT32(2, pipeline.metrics().totalBubbles);
}

void test_pipeline_reset_clears_metrics_and_events() {
  BubblePipeline pipeline(makeConfig());
  pipeline.update(1000, 0);
  pipeline.update(800, 10);
  pipeline.update(1000, 20);

  pipeline.reset();

  TEST_ASSERT_EQUAL_UINT32(0, pipeline.metrics().totalBubbles);
  TEST_ASSERT_FALSE(pipeline.update(1000, 30));
  TEST_ASSERT_TRUE(pipeline.update(800, 40));
}

void test_pipeline_restores_total_without_emitting_event() {
  BubblePipeline pipeline(makeConfig());
  pipeline.restoreTotal(1234);

  TEST_ASSERT_EQUAL_UINT32(1234, pipeline.metrics().totalBubbles);
  TEST_ASSERT_FALSE(pipeline.update(1000, 0));
}

void test_pipeline_configure_replaces_counter_settings() {
  BubblePipeline pipeline(makeConfig());
  BubbleCounterConfig updated = makeConfig();
  updated.threshold = 400;
  pipeline.configure(updated);

  pipeline.update(1000, 0);
  TEST_ASSERT_FALSE(pipeline.update(800, 10));
  TEST_ASSERT_TRUE(pipeline.update(500, 20));
  TEST_ASSERT_EQUAL_UINT32(1, pipeline.metrics().totalBubbles);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_pipeline_reports_bubble_event);
  RUN_TEST(test_pipeline_reports_event_only_when_count_increases);
  RUN_TEST(test_pipeline_reset_clears_metrics_and_events);
  RUN_TEST(test_pipeline_restores_total_without_emitting_event);
  RUN_TEST(test_pipeline_configure_replaces_counter_settings);
  return UNITY_END();
}
