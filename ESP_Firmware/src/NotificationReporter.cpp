#include "NotificationReporter.h"
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>
#include <esp_system.h>
#include <time.h>

extern const uint8_t notificationBundleStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t notificationBundleEnd[] asm("_binary_x509_crt_bundle_end");

bool NotificationReporter::begin(const RemoteNotificationConfig &config, const DailyStatsState &state) {
  tracker_.restore(state);
  events_ = xQueueCreate(8, sizeof(FlowNotification));
  configuration_ = xQueueCreate(1, sizeof(RemoteNotificationConfig));
  if (!events_ || !configuration_) {
    if (events_) vQueueDelete(events_);
    if (configuration_) vQueueDelete(configuration_);
    events_ = configuration_ = nullptr;
    return false;
  }
  configure(config);
  if (xTaskCreatePinnedToCore(taskEntry, "notifications", 8192, this, 1, nullptr, 1) != pdPASS) {
    vQueueDelete(events_);
    vQueueDelete(configuration_);
    events_ = configuration_ = nullptr;
    return false;
  }
  running_ = true;
  return true;
}

void NotificationReporter::configure(const RemoteNotificationConfig &config) {
  enabled_ = config.enabled;
  blocked_ = false;
  if (configuration_) xQueueOverwrite(configuration_, &config);
}

void NotificationReporter::observe(const DailyStatsState &state, uint32_t epoch, float bps) {
  FlowNotification event;
  if (!tracker_.observe(state, epoch, event) || !enabled_) return;
  event.bps = bps;
  if (!running_ || blocked_ || xQueueSend(events_, &event, 0) != pdTRUE) ++dropped_;
}

NotificationDeliveryState NotificationReporter::status() const {
  return {sent_.load(), dropped_.load(), static_cast<uint32_t>((events_ ? uxQueueMessagesWaiting(events_) : 0) + (pending_ ? 1 : 0)), lastHttp_.load(), running_.load(), blocked_.load()};
}

void NotificationReporter::taskEntry(void *self) { static_cast<NotificationReporter *>(self)->run(); }

void NotificationReporter::run() {
  RemoteNotificationConfig config;
  FlowNotification event;
  uint32_t attempt = 0;
  uint32_t retryAt = 0;
  bool hasEvent = false;
  for (;;) {
    RemoteNotificationConfig next;
    if (xQueueReceive(configuration_, &next, 0) == pdTRUE) {
      if (hasEvent) ++dropped_;
      dropped_ += uxQueueMessagesWaiting(events_);
      xQueueReset(events_);
      config = next;
      hasEvent = false;
      pending_ = false;
      lastHttp_ = 0;
    }
    if (!config.enabled || blocked_) {
      vTaskDelay(pdMS_TO_TICKS(250));
      continue;
    }
    if (!hasEvent && xQueueReceive(events_, &event, 0) == pdTRUE) {
      hasEvent = true;
      pending_ = true;
      attempt = 0;
      retryAt = millis();
    }
    const time_t epoch = time(nullptr);
    if (hasEvent && epoch > event.occurredAt && epoch - event.occurredAt > 86400) {
      hasEvent = false;
      pending_ = false;
      ++dropped_;
    }
    if (hasEvent && epoch >= 1609459200 && WiFi.status() == WL_CONNECTED && static_cast<int32_t>(millis() - retryAt) >= 0) {
      NetworkClientSecure client;
      client.setCACertBundle(notificationBundleStart, notificationBundleEnd - notificationBundleStart);
      client.setHandshakeTimeout(4);
      client.setTimeout(3000);
      HTTPClient http;
      http.setConnectTimeout(3000);
      http.setTimeout(3000);
      http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
      const String url = String("https://") + config.projectId + ".web.app/api/device-events";
      int result = -1;
      if (http.begin(client, url)) {
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", String("Bearer ") + config.deviceId + "." + config.deviceKey);
        const std::string body = notificationEventJson(event);
        result = http.POST(String(body.c_str()));
      }
      http.end();
      lastHttp_ = result;
      if (result == 200 || result == 202) {
        ++sent_;
        hasEvent = false;
      } else if (!notificationRetryable(result)) {
        ++dropped_;
        hasEvent = false;
        if (result == 401 || result == 403) blocked_ = true;
      } else {
        retryAt = millis() + notificationRetryDelay(attempt++, esp_random());
      }
      pending_ = hasEvent;
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}
