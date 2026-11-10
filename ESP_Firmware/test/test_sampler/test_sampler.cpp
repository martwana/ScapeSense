#include <unity.h>
#include "Sampler.h"

void setUp() {}
void tearDown() {}

void test_sampler_defaults_to_500_hz() {
    Sampler sampler;
    TEST_ASSERT_EQUAL(500u, sampler.rateHz());
    TEST_ASSERT_EQUAL(2000u, sampler.intervalUs());
}

void test_sampler_set_rate_updates_interval() {
    Sampler sampler;
    sampler.setRate(1000);
    TEST_ASSERT_EQUAL(1000u, sampler.intervalUs());
}

void test_sampler_zero_rate_clamps_to_one_hz() {
    Sampler sampler;
    sampler.setRate(0);
    TEST_ASSERT_EQUAL(1000000u, sampler.intervalUs());
}

void test_sampler_update_triggers_at_interval() {
    Sampler sampler(1000);
    sampler.reset(0);
    TEST_ASSERT_TRUE(sampler.update(1000));
    TEST_ASSERT_FALSE(sampler.update(1500));
    TEST_ASSERT_TRUE(sampler.update(2000));
}

void test_sampler_update_does_not_drift() {
    Sampler sampler(1000);
    sampler.reset(0);
    for (int i = 1; i <= 10; ++i) {
        TEST_ASSERT_TRUE(sampler.update(i * 1000));
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_sampler_defaults_to_500_hz);
    RUN_TEST(test_sampler_set_rate_updates_interval);
    RUN_TEST(test_sampler_zero_rate_clamps_to_one_hz);
    RUN_TEST(test_sampler_update_triggers_at_interval);
    RUN_TEST(test_sampler_update_does_not_drift);
    return UNITY_END();
}
