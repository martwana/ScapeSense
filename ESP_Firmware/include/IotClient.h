#pragma once

// AWS IoT connectivity: MQTT over mutual TLS to AWS IoT Core using ESP-IDF's
// bundled esp_mqtt_client (no extra library — QoS, LWT and auto-reconnect are
// built in). Owns a lightweight task for cadence + queue draining so local
// monitoring never waits on the cloud. Bounded queues, event expiry and
// observable failure counters, mirroring NotificationReporter.

#include "DailyStats.h"
#include "IotProtocol.h"
#include <Arduino.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <mqtt_client.h>
#include <string>

class IotClient {
public:
  bool begin(const IotConfig &config);
  void configure(const IotConfig &config);
  // Latest-wins telemetry snapshot; the task publishes on its own schedule.
  void observeTelemetry(const IotTelemetry &sample);
  // Flow start/stop transitions -> events topic immediately (QoS 1).
  void observeFlow(const DailyStatsState &state, uint32_t epoch, float bps);
  IotStatus status() const;

private:
  static void taskEntry(void *self);
  static void mqttEventHandler(void *ctx, esp_event_base_t base,
                               int32_t eventId, void *eventData);
  void run();
  bool loadCertificates();
  esp_mqtt_client_handle_t openClient(const IotConfig &config);
  void handleCommand(const char *data, size_t length);
  void requestLive(uint32_t seconds);
  void publishResponse(const IotCommand &command, bool ok, const char *detail);

  FlowNotificationTracker tracker_;
  QueueHandle_t telemetry_ = nullptr;     // length 1, xQueueOverwrite
  QueueHandle_t events_ = nullptr;        // length 8 FlowNotification
  QueueHandle_t configuration_ = nullptr; // length 1 IotConfig
  QueueHandle_t responses_ = nullptr;     // length 4, fixed-size JSON buffers

  esp_mqtt_client_handle_t mqtt_ = nullptr;
  char deviceId_[65]{}; // set in openClient; used by the event handler

  std::string certPem_;
  std::string keyPem_;

  std::atomic<bool> running_{false};
  std::atomic<bool> enabled_{false};
  std::atomic<bool> configured_{false};
  std::atomic<bool> connected_{false};
  std::atomic<uint32_t> published_{0};
  std::atomic<uint32_t> dropped_{0};
  std::atomic<uint32_t> queued_{0};
  std::atomic<uint32_t> commandsHandled_{0};
  std::atomic<int> lastError_{0};
  std::atomic<int32_t> stackErr_{0};
  std::atomic<int32_t> connRc_{-1};
  std::atomic<uint32_t> certFlags_{0};
  std::atomic<uint32_t> freeHeap_{0};
  std::atomic<uint32_t> minFreeHeap_{0};
  std::atomic<uint32_t> liveUntilMs_{0};
};
