#include "RollingAverage.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_rolling_average_initial_empty() {
  RollingAverage<int, 4> avg;
  TEST_ASSERT_TRUE(avg.empty());
  TEST_ASSERT_EQUAL(0, avg.average());
}

void test_rolling_average_adds_values() {
  RollingAverage<int, 4> avg;
  avg.add(10);
  avg.add(20);
  TEST_ASSERT_EQUAL(15, avg.average());
  TEST_ASSERT_EQUAL(2, avg.size());
}

void test_rolling_average_rolls_over() {
  RollingAverage<int, 3> avg;
  avg.add(10);
  avg.add(20);
  avg.add(30);
  avg.add(60);
  TEST_ASSERT_EQUAL(36, avg.average());
  TEST_ASSERT_TRUE(avg.full());
}

void test_rolling_average_clear() {
  RollingAverage<int, 3> avg;
  avg.add(5);
  avg.add(15);
  avg.clear();
  TEST_ASSERT_TRUE(avg.empty());
  TEST_ASSERT_EQUAL(0, avg.average());
}

void test_rolling_average_floating_point() {
  RollingAverage<float, 2> avg;
  avg.add(1.0f);
  avg.add(2.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, avg.average());
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_rolling_average_initial_empty);
  RUN_TEST(test_rolling_average_adds_values);
  RUN_TEST(test_rolling_average_rolls_over);
  RUN_TEST(test_rolling_average_clear);
  RUN_TEST(test_rolling_average_floating_point);
  return UNITY_END();
}
