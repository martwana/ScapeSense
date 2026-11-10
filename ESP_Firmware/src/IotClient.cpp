#include "IotClient.h"
#include "ProjectConfig.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_crt_bundle.h>
#include <esp_system.h>
#include <esp_tls.h>
#include <esp_transport_ssl.h>
#include <time.h>

namespace {
constexpr uint32_t kValidEpoch = 1609459200;
constexpr uint32_t kMaxCertBytes = 4096;
constexpr uint32_t kEventExpirySeconds = 86400;
constexpr size_t kResponseJsonBytes = 200;
constexpr int32_t kReconnectMs = 10000;
} // namespace

bool IotClient::begin(const IotConfig &config) {
  telemetry_ = xQueueCreate(1, sizeof(IotTelemetry));
  events_ = xQueueCreate(8, sizeof(FlowNotification));
  configuration_ = xQueueCreate(1, sizeof(IotConfig));
  responses_ = xQueueCreate(4, kResponseJsonBytes);
  if (!telemetry_ || !events_ || !configuration_ || !responses_) return false;
  configure(config);
  if (xTaskCreatePinnedToCore(taskEntry, "iot-client", 8192, this, 1, nullptr, 1) != pdPASS) {
    return false;
  }
  running_ = true;
  return true;
}

void IotClient::configure(const IotConfig &config) {
  enabled_ = config.enabled;
  configured_ = config.enabled && validIotConfig(config);
  if (configuration_) xQueueOverwrite(configuration_, &config);
}

void IotClient::observeTelemetry(const IotTelemetry &sample) {
  if (telemetry_) xQueueOverwrite(telemetry_, &sample);
}

void IotClient::observeFlow(const DailyStatsState &state, uint32_t epoch, float bps) {
  FlowNotification event;
  if (!tracker_.observe(state, epoch, event) || !enabled_) return;
  event.bps = bps;
  if (!events_ || xQueueSend(events_, &event, 0) != pdTRUE) ++dropped_;
  if (events_) queued_ = uxQueueMessagesWaiting(events_);
}

IotStatus IotClient::status() const {
  const bool live = liveUntilMs_.load() > millis();
  return {enabled_.load(), configured_.load(), connected_.load(), live,
          published_.load(), dropped_.load(), queued_.load(),
          commandsHandled_.load(), lastError_.load(), stackErr_.load(),
          connRc_.load(), certFlags_.load(), freeHeap_.load(),
          minFreeHeap_.load()};
}

void IotClient::taskEntry(void *self) { static_cast<IotClient *>(self)->run(); }

void IotClient::mqttEventHandler(void *ctx, esp_event_base_t, int32_t eventId,
                                 void *eventData) {
  IotClient *self = static_cast<IotClient *>(ctx);
  const esp_mqtt_event_handle_t event =
      static_cast<esp_mqtt_event_handle_t>(eventData);
  switch (static_cast<esp_mqtt_event_id_t>(eventId)) {
  case MQTT_EVENT_CONNECTED: {
    self->connected_ = true;
    self->lastError_ = 0;
    const std::string eventsTopic = iotDeviceTopic(self->deviceId_, "events");
    const std::string online = std::string("{\"deviceId\":\"") + self->deviceId_ + "\",\"kind\":\"online\"}";
    // AWS IoT rejects retained publishes, so presence rides on normal QoS1
    // events plus telemetry timestamps instead.
    esp_mqtt_client_publish(event->client, eventsTopic.c_str(), online.c_str(), 0, 1, 0);
    esp_mqtt_client_subscribe_single(
        event->client, iotDeviceTopic(self->deviceId_, "commands").c_str(), 1);
    break;
  }
  case MQTT_EVENT_DISCONNECTED:
    self->connected_ = false;
    break;
  case MQTT_EVENT_DATA: {
    const std::string topic(event->topic, event->topic_len);
    if (topic.size() >= 9 && topic.compare(topic.size() - 9, 9, "/commands") == 0) {
      self->handleCommand(event->data, event->data_len);
    }
    break;
  }
  case MQTT_EVENT_ERROR:
    if (event->error_handle) {
      // Prefer connect_return_code when present — an MQTT-level refusal
      // (e.g. policy deny) is more specific than the transport FIN that
      // follows it and lands in esp_tls_last_esp_err.
      self->connRc_ = event->error_handle->connect_return_code;
      self->stackErr_ = event->error_handle->esp_tls_stack_err;
      self->certFlags_ = event->error_handle->esp_tls_cert_verify_flags;
      self->freeHeap_ = ESP.getFreeHeap();
      self->minFreeHeap_ = ESP.getMinFreeHeap();
      self->lastError_ = event->error_handle->connect_return_code
                             ? event->error_handle->connect_return_code
                             : event->error_handle->esp_tls_last_esp_err;
    }
    break;
  default:
    break;
  }
}

bool IotClient::loadCertificates() {
  if (!LittleFS.begin(true)) return false;
  const auto read = [](const char *path, std::string &out) {
    File file = LittleFS.open(path, "r");
    if (!file) return false;
    const size_t size = file.size();
    if (size == 0 || size > kMaxCertBytes) {
      file.close();
      return false;
    }
    out.resize(size);
    const size_t got = file.read(reinterpret_cast<uint8_t *>(out.data()), size);
    file.close();
    return got == size;
  };
  return read("/iot/device.crt", certPem_) && read("/iot/device.key", keyPem_);
}

esp_mqtt_client_handle_t IotClient::openClient(const IotConfig &config) {
  std::strncpy(deviceId_, config.deviceId, sizeof(deviceId_) - 1);
  deviceId_[sizeof(deviceId_) - 1] = '\0';
  esp_mqtt_client_config_t mqttConfig = {};
  mqttConfig.broker.address.hostname = config.endpoint;
  mqttConfig.broker.address.port = 8883;
  mqttConfig.broker.address.transport = MQTT_TRANSPORT_OVER_SSL;
  mqttConfig.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
  mqttConfig.credentials.client_id = config.deviceId;
  mqttConfig.credentials.authentication.certificate = certPem_.c_str();
  mqttConfig.credentials.authentication.key = keyPem_.c_str();
  mqttConfig.session.keepalive = 60;
  mqttConfig.session.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
  mqttConfig.network.reconnect_timeout_ms = kReconnectMs;
  mqttConfig.network.timeout_ms = 10000;
  mqttConfig.buffer.size = 1024;
  mqttConfig.buffer.out_size = 1024;

  // Both must outlive openClient() — esp_mqtt_client stores the pointers.
  static std::string lwtTopic;
  static std::string lwtMessage;
  lwtTopic = iotDeviceTopic(config.deviceId, "events");
  lwtMessage = std::string("{\"deviceId\":\"") + config.deviceId + "\",\"kind\":\"offline\"}";
  mqttConfig.session.last_will.topic = lwtTopic.c_str();
  mqttConfig.session.last_will.msg = lwtMessage.c_str();
  mqttConfig.session.last_will.qos = 1;
  // AWS IoT silently drops CONNECT packets with WillRetain set — retained
  // messages (Will or publish) aren't supported, so presence is conveyed by
  // the QoS1 "online" event plus telemetry timestamps instead.
  mqttConfig.session.last_will.retain = false;

  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqttConfig);
  if (!client) return nullptr;
  // AWS negotiates TLS 1.3 with capable clients, but mbedTLS's 1.3 mutual-auth
  // path on ESP32 has proven unreliable (server FIN mid-handshake). TLS 1.2 is
  // also what AWS IoT's documented cipher suites target.
  // IPv4 is pinned so connection behaviour is deterministic on dual-stack
  // endpoints regardless of lwIP's address-ordering quirks.
  if (esp_transport_handle_t tls = esp_mqtt_client_get_transport(client, "ssl")) {
    esp_transport_ssl_set_tls_version(tls, ESP_TLS_VER_TLS_1_2);
    esp_transport_ssl_set_addr_family(tls, ESP_TLS_AF_INET);
  }
  if (esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqttEventHandler, this) != ESP_OK) {
    esp_mqtt_client_destroy(client);
    return nullptr;
  }
  if (esp_mqtt_client_start(client) != ESP_OK) {
    esp_mqtt_client_destroy(client);
    return nullptr;
  }
  return client;
}

void IotClient::requestLive(uint32_t seconds) {
  liveUntilMs_ = millis() + seconds * 1000;
}

void IotClient::publishResponse(const IotCommand &command, bool ok, const char *detail) {
  char buffer[kResponseJsonBytes];
  const std::string body = iotCommandResponseJson(command, ok, detail);
  std::snprintf(buffer, sizeof(buffer), "%s", body.c_str());
  if (xQueueSend(responses_, buffer, 0) != pdTRUE) ++dropped_;
}

void IotClient::handleCommand(const char *data, size_t length) {
  IotCommand command;
  if (!iotParseCommand(std::string(data, length), command)) {
    ++dropped_;
    return;
  }
  if (std::strcmp(command.action, "live") == 0) {
    requestLive(command.seconds);
    publishResponse(command, true, "live");
  } else {
    publishResponse(command, true, "pong");
  }
  ++commandsHandled_;
}

void IotClient::run() {
  IotConfig config{};
  uint32_t nextTelemetryAt = 0;
  bool certsLoaded = false;
  for (;;) {
    IotConfig next;
    if (xQueueReceive(configuration_, &next, 0) == pdTRUE) {
      if (mqtt_) {
        esp_mqtt_client_stop(mqtt_);
        esp_mqtt_client_destroy(mqtt_);
        mqtt_ = nullptr;
      }
      connected_ = false;
      certsLoaded = false;
      xQueueReset(events_);
      xQueueReset(responses_);
      liveUntilMs_ = 0;
      config = next;
    }
    if (!config.enabled) {
      vTaskDelay(pdMS_TO_TICKS(250));
      continue;
    }
    if (!certsLoaded) {
      certsLoaded = loadCertificates();
      if (!certsLoaded) {
        lastError_ = -2; // certificate files missing/invalid in LittleFS
        vTaskDelay(pdMS_TO_TICKS(2000));
        continue;
      }
    }
    if (WiFi.status() != WL_CONNECTED || time(nullptr) < kValidEpoch) {
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }
    const uint32_t now = millis();

    if (!mqtt_) {
      mqtt_ = openClient(config);
      if (!mqtt_) {
        lastError_ = -3; // client init/start failed
        vTaskDelay(pdMS_TO_TICKS(kReconnectMs));
        continue;
      }
      nextTelemetryAt = now; // report promptly once connected
    }

    if (!connected_) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    // Responses queued by the command callback.
    char response[kResponseJsonBytes];
    while (xQueueReceive(responses_, response, 0) == pdTRUE) {
      esp_mqtt_client_publish(mqtt_, iotDeviceTopic(config.deviceId, "responses").c_str(),
                              response, 0, 1, 0);
    }

    // Expired/queued events -> events topic (QoS 1). esp_mqtt queues outbound
    // publishes, so a transient link drop doesn't drop the event.
    FlowNotification event;
    while (xQueuePeek(events_, &event, 0) == pdTRUE) {
      const time_t epoch = time(nullptr);
      if (epoch > event.occurredAt && epoch - event.occurredAt > kEventExpirySeconds) {
        xQueueReceive(events_, &event, 0);
        ++dropped_;
        continue;
      }
      const std::string body = notificationEventJson(event);
      if (esp_mqtt_client_publish(mqtt_, iotDeviceTopic(config.deviceId, "events").c_str(),
                                  body.c_str(), 0, 1, 0) < 0) {
        lastError_ = -4; // outbound buffer full
        break;           // keep queued; retry next pass
      }
      xQueueReceive(events_, &event, 0);
      ++published_;
    }
    queued_ = uxQueueMessagesWaiting(events_);

    // Telemetry on its own cadence; live window overrides the background interval.
    const bool live = liveUntilMs_.load() > now;
    if (now >= nextTelemetryAt) {
      IotTelemetry sample;
      if (xQueueReceive(telemetry_, &sample, 0) == pdTRUE) {
        const std::string body =
            iotTelemetryJson(sample, config.deviceId, ProjectConfig::kFirmwareVersion, live);
        if (esp_mqtt_client_publish(mqtt_, iotDeviceTopic(config.deviceId, "telemetry").c_str(),
                                    body.c_str(), 0, 0, 0) < 0) {
          lastError_ = -4;
          ++dropped_;
        } else {
          ++published_;
        }
      }
      nextTelemetryAt = now + (live ? config.liveIntervalMs : config.backgroundIntervalMs);
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
