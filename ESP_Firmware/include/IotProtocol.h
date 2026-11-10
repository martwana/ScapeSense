#pragma once

// AWS IoT protocol surface: config validation, telemetry/command payloads and
// topic layout. Header-only so the native test env can cover all of it.
//
// deviceId is the IoT Thing name, the MQTT client ID and the topic segment:
//   scapesense/{deviceId}/telemetry|events|commands|responses

#include "RemoteNotifications.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

struct IotConfig {
  uint32_t version = 1;
  bool enabled = false;
  char endpoint[97]{};                    // <id>-ats.iot.<region>.amazonaws.com
  char deviceId[65]{};                    // Thing name == MQTT client ID
  uint32_t backgroundIntervalMs = 300000; // ~5 min idle reporting
  uint32_t liveIntervalMs = 2000;         // ~2 s while a portal viewer is active
};

constexpr uint32_t kIotBackgroundMinMs = 10000;
constexpr uint32_t kIotBackgroundMaxMs = 3600000;
constexpr uint32_t kIotLiveMinMs = 500;
constexpr uint32_t kIotLiveMaxMs = 30000;
constexpr uint32_t kIotLiveWindowMaxSeconds = 3600; // live mode can never stick

inline bool validIotEndpoint(const char *endpoint) {
  const size_t length = std::strlen(endpoint);
  if (length < 10 || length > 96) return false;
  if (endpoint[0] == '-' || endpoint[0] == '.' || endpoint[length - 1] == '-' ||
      endpoint[length - 1] == '.') return false;
  bool sawDot = false;
  for (size_t i = 0; i < length; ++i) {
    const char c = endpoint[i];
    if (c == '.') {
      if (i > 0 && endpoint[i - 1] == '.') return false;
      sawDot = true;
    } else if (c == '-') {
      // Labels must not start or end with a hyphen.
      if (i == 0 || i + 1 == length || endpoint[i - 1] == '.' ||
          endpoint[i + 1] == '.') return false;
    } else if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9')) {
      return false;
    }
  }
  return sawDot;
}

inline bool validIotDeviceId(const char *deviceId) {
  const size_t length = std::strlen(deviceId);
  if (length < 1 || length > 64) return false;
  return std::all_of(deviceId, deviceId + length, [](char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '-';
  });
}

inline bool validIotIntervals(uint32_t backgroundMs, uint32_t liveMs) {
  return backgroundMs >= kIotBackgroundMinMs && backgroundMs <= kIotBackgroundMaxMs &&
         liveMs >= kIotLiveMinMs && liveMs <= kIotLiveMaxMs && liveMs <= backgroundMs;
}

inline bool validIotConfig(const IotConfig &config) {
  if (config.version != 1 || !terminated(config.endpoint) ||
      !terminated(config.deviceId)) return false;
  if (!validIotIntervals(config.backgroundIntervalMs, config.liveIntervalMs)) return false;
  if (!config.enabled && !*config.endpoint && !*config.deviceId) return true;
  if (!config.enabled) return false;
  return validIotEndpoint(config.endpoint) && validIotDeviceId(config.deviceId);
}

struct IotTelemetry {
  uint32_t epoch = 0;
  uint32_t uptimeSeconds = 0;
  uint32_t totalBubbles = 0;
  float bps = 0;
  float bpm = 0;
  float temperatureC = 0;
  bool temperatureValid = false;
  float weightG = 0;
  bool weightReady = false;
  float gasRemainingPercent = -1; // negative = cylinder monitoring unconfigured
  bool flowActive = false;
  int16_t rssi = 0;
};

inline std::string iotDeviceTopic(const char *deviceId, const char *leaf) {
  return std::string("scapesense/") + deviceId + "/" + leaf;
}

inline std::string iotTelemetryJson(const IotTelemetry &t, const char *deviceId,
                                    const char *firmware, bool live) {
  char json[384];
  const float bps = std::isfinite(t.bps) ? t.bps : 0;
  const float bpm = std::isfinite(t.bpm) ? t.bpm : 0;
  const float tempC = std::isfinite(t.temperatureC) ? t.temperatureC : 0;
  const float weightG = std::isfinite(t.weightG) ? t.weightG : 0;
  const float gasPct = std::isfinite(t.gasRemainingPercent) ? t.gasRemainingPercent : -1;
  std::snprintf(
      json, sizeof(json),
      "{\"v\":1,\"deviceId\":\"%s\",\"ts\":%lu,\"uptime\":%lu,"
      "\"bps\":%.2f,\"bpm\":%.1f,\"total\":%lu,"
      "\"tempC\":%.1f,\"tempOk\":%s,"
      "\"weightG\":%.1f,\"weightOk\":%s,\"co2Pct\":%.1f,"
      "\"flowActive\":%s,\"rssi\":%d,\"fw\":\"%s\",\"mode\":\"%s\"}",
      deviceId, static_cast<unsigned long>(t.epoch),
      static_cast<unsigned long>(t.uptimeSeconds), bps, bpm,
      static_cast<unsigned long>(t.totalBubbles), tempC,
      t.temperatureValid ? "true" : "false", weightG,
      t.weightReady ? "true" : "false", gasPct,
      t.flowActive ? "true" : "false", static_cast<int>(t.rssi), firmware,
      live ? "live" : "background");
  return json;
}

// Minimal command parsing — commands come only from our backend over the
// device-scoped commands topic, so a bounded hand parser beats pulling a full
// JSON dependency into the hot path (and into native tests).
struct IotCommand {
  char id[33]{};
  char action[17]{}; // "live" | "ping"
  uint32_t seconds = 0;
};

inline bool iotJsonStringField(const std::string &json, const char *key, char *out,
                               size_t outSize) {
  const std::string needle = std::string("\"") + key + "\"";
  size_t pos = json.find(needle);
  if (pos == std::string::npos) return false;
  pos += needle.size();
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) ++pos;
  if (pos >= json.size() || json[pos] != '"') return false;
  ++pos;
  size_t end = pos;
  while (end < json.size() && json[end] != '"') {
    if (json[end] == '\\') ++end; // skip escaped chars entirely
    ++end;
  }
  if (end >= json.size() || end - pos >= outSize) return false;
  std::memcpy(out, json.data() + pos, end - pos);
  out[end - pos] = '\0';
  return true;
}

inline bool iotJsonNumberField(const std::string &json, const char *key,
                               uint32_t &value) {
  const std::string needle = std::string("\"") + key + "\"";
  size_t pos = json.find(needle);
  if (pos == std::string::npos) return false;
  pos += needle.size();
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) ++pos;
  uint64_t parsed = 0;
  size_t end = pos;
  while (end < json.size() && json[end] >= '0' && json[end] <= '9') {
    parsed = parsed * 10 + static_cast<uint32_t>(json[end] - '0');
    if (parsed > 0xFFFFFFFFull) return false;
    ++end;
  }
  if (end == pos) return false;
  value = static_cast<uint32_t>(parsed);
  return true;
}

inline bool iotParseCommand(const std::string &json, IotCommand &command) {
  char name[17]{};
  if (!iotJsonStringField(json, "command", name, sizeof(name))) return false;
  if (std::strcmp(name, "live") == 0) {
    uint32_t seconds = 0;
    if (!iotJsonNumberField(json, "seconds", seconds) || seconds < 10 ||
        seconds > kIotLiveWindowMaxSeconds) return false;
    std::strcpy(command.action, "live");
    command.seconds = seconds;
  } else if (std::strcmp(name, "ping") == 0) {
    std::strcpy(command.action, "ping");
  } else {
    return false;
  }
  iotJsonStringField(json, "id", command.id, sizeof(command.id)); // optional
  return true;
}

inline std::string iotCommandResponseJson(const IotCommand &command, bool ok,
                                        const char *detail) {
  char json[192];
  std::snprintf(json, sizeof(json),
                "{\"id\":\"%s\",\"command\":\"%s\",\"ok\":%s,\"detail\":\"%s\"}",
                command.id, command.action, ok ? "true" : "false", detail);
  return json;
}

struct IotStatus {
  bool enabled = false;
  bool configured = false;
  bool connected = false;
  bool liveActive = false;
  uint32_t published = 0;
  uint32_t dropped = 0;
  uint32_t queued = 0;
  uint32_t commandsHandled = 0;
  int lastError = 0;
};
