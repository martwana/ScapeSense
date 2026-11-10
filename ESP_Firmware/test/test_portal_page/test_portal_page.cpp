#include "PortalPage.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

static bool contains(const std::string &text, const std::string &part) {
  return text.find(part) != std::string::npos;
}

void test_escape_html_special_characters() {
  TEST_ASSERT_EQUAL_STRING("a&lt;b&gt;&amp;&quot;c&#39;",
                           htmlEscape("a<b>&\"c'").c_str());
}

void test_normalize_removes_empty_and_duplicate_names() {
  std::vector<WifiNetwork> networks = {
      {"Home", -70, true}, {"", -40, false}, {"Home", -50, true}};
  auto result = normalizeNetworks(networks, 10);
  TEST_ASSERT_EQUAL(1, result.size());
  TEST_ASSERT_EQUAL(-50, result[0].rssi);
}

void test_normalize_sorts_strongest_first() {
  std::vector<WifiNetwork> networks = {
      {"Far", -80, true}, {"Near", -40, true}, {"Mid", -60, false}};
  auto result = normalizeNetworks(networks, 10);
  TEST_ASSERT_EQUAL_STRING("Near", result[0].ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("Mid", result[1].ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("Far", result[2].ssid.c_str());
}

void test_normalize_limits_count() {
  std::vector<WifiNetwork> networks = {
      {"A", -40, true}, {"B", -50, true}, {"C", -60, true}};
  TEST_ASSERT_EQUAL(2, normalizeNetworks(networks, 2).size());
}

void test_page_lists_networks_as_options() {
  std::string page = buildPortalPage({{"Home Net", -45, true}});
  TEST_ASSERT_TRUE(contains(page, "<option value=\"Home Net\">Home Net"));
}

void test_page_escapes_network_names() {
  std::string page = buildPortalPage({{"<script>", -45, true}});
  TEST_ASSERT_FALSE(contains(page, "<script>"));
  TEST_ASSERT_TRUE(contains(page, "&lt;script&gt;"));
}

void test_page_has_scapesense_branding() {
  std::string page = buildPortalPage({});
  TEST_ASSERT_TRUE(contains(page, "<title>ScapeSense</title>"));
  TEST_ASSERT_TRUE(contains(page, "<h1>ScapeSense Setup</h1>"));
  TEST_ASSERT_FALSE(contains(page, "Aquarium CO2"));
}

void test_page_has_form_fields() {
  std::string page = buildPortalPage({});
  TEST_ASSERT_TRUE(contains(page, "name=\"ssid\""));
  TEST_ASSERT_TRUE(contains(page, "name=\"manual\""));
  TEST_ASSERT_TRUE(contains(page, "name=\"password\""));
  TEST_ASSERT_TRUE(contains(page, "action=\"/save\""));
}

void test_page_forces_readable_colors_and_charset() {
  std::string page = buildPortalPage({});
  TEST_ASSERT_TRUE(contains(page, "charset=\"utf-8\""));
  TEST_ASSERT_TRUE(contains(page, "background:#fff"));
  TEST_ASSERT_TRUE(contains(page, "color:#111"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_escape_html_special_characters);
  RUN_TEST(test_normalize_removes_empty_and_duplicate_names);
  RUN_TEST(test_normalize_sorts_strongest_first);
  RUN_TEST(test_normalize_limits_count);
  RUN_TEST(test_page_lists_networks_as_options);
  RUN_TEST(test_page_escapes_network_names);
  RUN_TEST(test_page_has_scapesense_branding);
  RUN_TEST(test_page_has_form_fields);
  RUN_TEST(test_page_forces_readable_colors_and_charset);
  return UNITY_END();
}
