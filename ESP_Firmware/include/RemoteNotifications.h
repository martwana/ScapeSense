#pragma once

#include "DailyStats.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

struct RemoteNotificationConfig {
  uint32_t version = 1;
  bool enabled = false;
  char projectId[31]{};
  char deviceId[33]{};
  char deviceKey[44]{};
};

template <size_t N> inline bool terminated(const char (&text)[N]) {
  return std::find(text, text + N, '\0') != text + N;
}

inline bool validNotificationIdentity(const RemoteNotificationConfig &config) {
  if (!terminated(config.projectId) || !terminated(config.deviceId)) return false;
  const size_t length = std::strlen(config.projectId);
  if (length < 6 || length > 30 || config.projectId[0] < 'a' || config.projectId[0] > 'z') return false;
  for (size_t i = 0; i < length; ++i) {
    const char c = config.projectId[i];
    if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && !(c == '-' && i + 1 < length)) return false;
  }
  if (std::strlen(config.deviceId) != 32) return false;
  return std::all_of(config.deviceId, config.deviceId + 32, [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}

inline bool validNotificationConfig(const RemoteNotificationConfig &config) {
  if (config.version != 1 || !terminated(config.projectId) || !terminated(config.deviceId) || !terminated(config.deviceKey)) return false;
  if (!config.enabled && !*config.projectId && !*config.deviceId && !*config.deviceKey) return true;
  if (!validNotificationIdentity(config) || std::strlen(config.deviceKey) != 43) return false;
  return std::all_of(config.deviceKey, config.deviceKey + 43, [](char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
  });
}

inline bool mergeNotificationConfig(const RemoteNotificationConfig &current, RemoteNotificationConfig &request) {
  if (!terminated(request.projectId) || !terminated(request.deviceId) || !terminated(request.deviceKey)) return false;
  if (!*request.deviceKey && std::strcmp(current.projectId, request.projectId) == 0 && std::strcmp(current.deviceId, request.deviceId) == 0) {
    std::memcpy(request.deviceKey, current.deviceKey, sizeof(request.deviceKey));
  }
  return validNotificationConfig(request);
}

struct FlowNotification {
  bool started = false;
  uint32_t occurredAt = 0;
  uint32_t total = 0;
  float bps = 0;
};

class FlowNotificationTracker {
public:
  void restore(const DailyStatsState &state) {
    active_ = state.active;
    lastQualified_ = state.lastQualifiedEpoch;
    total_ = state.currentCount;
    day_ = state.currentDayKey;
  }
  bool observe(const DailyStatsState &state, uint32_t epoch, FlowNotification &event) {
    if (state.active) {
      const bool started = !active_;
      active_ = true;
      lastQualified_ = state.lastQualifiedEpoch;
      total_ = state.currentCount;
      day_ = state.currentDayKey;
      if (started) {
        event = {true, state.recentCount ? state.recentBubbles[0] : epoch, state.currentCount, 0};
        return true;
      }
    } else if (active_ && lastQualified_ && epoch >= lastQualified_ && epoch - lastQualified_ >= DailyStats::kStopToleranceSeconds) {
      active_ = false;
      event = {false, lastQualified_, state.currentDayKey == day_ ? state.currentCount : total_, 0};
      return true;
    }
    return false;
  }
private:
  bool active_ = false;
  uint32_t lastQualified_ = 0;
  uint32_t total_ = 0;
  uint32_t day_ = 0;
};

inline std::string notificationEventJson(const FlowNotification &event) {
  char json[240];
  const float bps = std::isfinite(event.bps) && event.bps >= 0 && event.bps <= 10000 ? event.bps : 0;
  std::snprintf(json, sizeof(json), "{\"eventId\":\"%08x%08x\",\"kind\":\"%s\",\"occurredAt\":%lu,\"total\":%lu,\"bps\":%.2f}",
    static_cast<unsigned>(event.started ? 1 : 2), static_cast<unsigned>(event.occurredAt),
    event.started ? "started" : "stopped", static_cast<unsigned long>(event.occurredAt), static_cast<unsigned long>(event.total), bps);
  return json;
}

inline bool notificationRetryable(int status) {
  return status < 0 || status == 408 || status == 429 || status >= 500;
}

inline uint32_t notificationRetryDelay(uint32_t attempt, uint32_t jitter) {
  return std::min<uint32_t>(900000, 1000u << std::min<uint32_t>(attempt, 10)) + jitter % 30000;
}

struct NotificationDeliveryState {
  uint32_t sent = 0;
  uint32_t dropped = 0;
  uint32_t queued = 0;
  int lastHttp = 0;
  bool running = false;
  bool blocked = false;
};
