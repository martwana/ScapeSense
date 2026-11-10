#pragma once

#include "RemoteNotifications.h"
#include <Arduino.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class NotificationReporter {
public:
  bool begin(const RemoteNotificationConfig &config, const DailyStatsState &state);
  void configure(const RemoteNotificationConfig &config);
  void observe(const DailyStatsState &state, uint32_t epoch, float bps);
  NotificationDeliveryState status() const;
private:
  static void taskEntry(void *self);
  void run();
  FlowNotificationTracker tracker_;
  QueueHandle_t events_ = nullptr;
  QueueHandle_t configuration_ = nullptr;
  std::atomic<bool> running_{false};
  std::atomic<bool> enabled_{false};
  std::atomic<bool> blocked_{false};
  std::atomic<bool> pending_{false};
  std::atomic<uint32_t> sent_{0};
  std::atomic<uint32_t> dropped_{0};
  std::atomic<int> lastHttp_{0};
};
