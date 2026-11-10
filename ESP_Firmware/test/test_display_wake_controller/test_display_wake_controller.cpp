#include "DisplayWakeController.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_display_starts_awake_for_configured_duration() {
  DisplayWakeController controller(60000, 30);
  TEST_ASSERT_TRUE(controller.isAwake(59999));
  TEST_ASSERT_FALSE(controller.isAwake(60000));
}

void test_short_bounce_does_not_wake_display() {
  DisplayWakeController controller(60000, 30);
  controller.update(true, 70000);
  controller.update(false, 70010);
  controller.update(false, 70040);
  TEST_ASSERT_FALSE(controller.isAwake(70040));
  TEST_ASSERT_FALSE(controller.takeWakeRequest());
}

void test_debounced_press_wakes_display_for_sixty_seconds() {
  DisplayWakeController controller(60000, 30);
  controller.update(true, 70000);
  controller.update(true, 70030);
  TEST_ASSERT_TRUE(controller.takeWakeRequest());
  TEST_ASSERT_FALSE(controller.takeWakeRequest());
  TEST_ASSERT_TRUE(controller.isAwake(130029));
  TEST_ASSERT_FALSE(controller.isAwake(130030));
}

void test_held_button_only_requests_one_wake() {
  DisplayWakeController controller(60000, 30);
  controller.update(true, 70000);
  controller.update(true, 70030);
  TEST_ASSERT_TRUE(controller.takeWakeRequest());
  controller.update(true, 71000);
  TEST_ASSERT_FALSE(controller.takeWakeRequest());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_display_starts_awake_for_configured_duration);
  RUN_TEST(test_short_bounce_does_not_wake_display);
  RUN_TEST(test_debounced_press_wakes_display_for_sixty_seconds);
  RUN_TEST(test_held_button_only_requests_one_wake);
  return UNITY_END();
}
