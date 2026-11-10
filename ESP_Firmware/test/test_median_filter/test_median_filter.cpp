#include "MedianFilter.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_filter_returns_middle_sample() {
  MedianFilter<int, 5> filter;
  filter.add(10);
  filter.add(2);
  filter.add(7);
  filter.add(3);
  filter.add(100);
  TEST_ASSERT_EQUAL_INT(7, filter.value());
}

void test_filter_rejects_older_outlier_after_capacity() {
  MedianFilter<int, 5> filter;
  filter.add(1000);
  filter.add(20);
  filter.add(21);
  filter.add(22);
  filter.add(23);
  TEST_ASSERT_EQUAL_INT(22, filter.value());
  filter.add(24);
  TEST_ASSERT_EQUAL_INT(22, filter.value());
}

void test_filter_works_before_full() {
  MedianFilter<int, 5> filter;
  filter.add(30);
  filter.add(10);
  filter.add(20);
  TEST_ASSERT_EQUAL_UINT(3, filter.count());
  TEST_ASSERT_EQUAL_INT(20, filter.value());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_filter_returns_middle_sample);
  RUN_TEST(test_filter_rejects_older_outlier_after_capacity);
  RUN_TEST(test_filter_works_before_full);
  return UNITY_END();
}
