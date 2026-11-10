#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

struct WifiNetwork {
  std::string ssid;
  int rssi;
  bool secure;
};

inline std::string htmlEscape(const std::string &text) {
  std::string out;
  for (char c : text) {
    switch (c) {
    case '<':
      out += "&lt;";
      break;
    case '>':
      out += "&gt;";
      break;
    case '&':
      out += "&amp;";
      break;
    case '"':
      out += "&quot;";
      break;
    case '\'':
      out += "&#39;";
      break;
    default:
      out += c;
    }
  }
  return out;
}

inline std::vector<WifiNetwork>
normalizeNetworks(std::vector<WifiNetwork> networks, size_t maxCount) {
  std::sort(networks.begin(), networks.end(),
            [](const WifiNetwork &a, const WifiNetwork &b) {
              return a.rssi > b.rssi;
            });

  std::vector<WifiNetwork> result;
  for (const auto &network : networks) {
    const bool duplicate = std::any_of(
        result.begin(), result.end(),
        [&](const WifiNetwork &known) { return known.ssid == network.ssid; });
    if (!network.ssid.empty() && !duplicate && result.size() < maxCount) {
      result.push_back(network);
    }
  }
  return result;
}

inline std::string buildPortalPage(const std::vector<WifiNetwork> &networks) {
  std::string page =
      "<!DOCTYPE html><html><head>"
      "<meta charset=\"utf-8\">"
      "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
      "<title>ScapeSense</title>"
      "<style>body{font-family:sans-serif;padding:1em;background:#fff;color:#"
      "111}"
      "select,input,button{display:block;width:100%;margin:.5em 0;padding:.6em;"
      "box-sizing:border-box;font-size:1em}</style></head><body>"
      "<h1>ScapeSense Setup</h1>"
      "<form action=\"/save\" method=\"POST\">"
      "<label>Choose your Wi-Fi network</label><select name=\"ssid\">";

  for (const auto &network : networks) {
    const std::string name = htmlEscape(network.ssid);
    page += "<option value=\"" + name + "\">" + name +
            (network.secure ? " (secured)" : " (open)") + "</option>";
  }

  page += "</select>"
          "<label>Or type a hidden network name</label>"
          "<input name=\"manual\" autocapitalize=\"none\">"
          "<label>Password</label>"
          "<input type=\"password\" name=\"password\">"
          "<button type=\"submit\">Connect</button></form></body></html>";
  return page;
}
