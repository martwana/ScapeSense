#include "UrlDecode.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_decode_plain_text_is_unchanged() {
  char out[32];
  urlDecode(out, sizeof(out), "HomeWifi");
  TEST_ASSERT_EQUAL_STRING("HomeWifi", out);
}

void test_decode_plus_becomes_space() {
  char out[32];
  urlDecode(out, sizeof(out), "My+Network");
  TEST_ASSERT_EQUAL_STRING("My Network", out);
}

void test_decode_percent_escapes() {
  char out[32];
  urlDecode(out, sizeof(out), "p%40ss%21");
  TEST_ASSERT_EQUAL_STRING("p@ss!", out);
}

void test_decode_ignores_malformed_escape() {
  char out[32];
  urlDecode(out, sizeof(out), "100%");
  TEST_ASSERT_EQUAL_STRING("100%", out);
}

void test_decode_truncates_to_buffer() {
  char out[4];
  urlDecode(out, sizeof(out), "abcdef");
  TEST_ASSERT_EQUAL_STRING("abc", out);
}

void test_form_field_extracts_value() {
  char out[32];
  TEST_ASSERT_TRUE(formField(out, sizeof(out), "ssid=Home+Net&password=abc", "ssid"));
  TEST_ASSERT_EQUAL_STRING("Home Net", out);
  TEST_ASSERT_TRUE(formField(out, sizeof(out), "ssid=Home+Net&password=abc", "password"));
  TEST_ASSERT_EQUAL_STRING("abc", out);
}

void test_form_field_missing_returns_false() {
  char out[32];
  TEST_ASSERT_FALSE(formField(out, sizeof(out), "ssid=Home", "password"));
}

void test_form_field_empty_value() {
  char out[32];
  TEST_ASSERT_TRUE(formField(out, sizeof(out), "ssid=Home&password=", "password"));
  TEST_ASSERT_EQUAL_STRING("", out);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_decode_plain_text_is_unchanged);
  RUN_TEST(test_decode_plus_becomes_space);
  RUN_TEST(test_decode_percent_escapes);
  RUN_TEST(test_decode_ignores_malformed_escape);
  RUN_TEST(test_decode_truncates_to_buffer);
  RUN_TEST(test_form_field_extracts_value);
  RUN_TEST(test_form_field_missing_returns_false);
  RUN_TEST(test_form_field_empty_value);
  return UNITY_END();
}
