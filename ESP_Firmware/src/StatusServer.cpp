#include "StatusServer.h"
#include "DailyStatsJson.h"
#include <ESPmDNS.h>
#include <ElegantOTA.h>
#include <LittleFS.h>
#include <WebSerial.h>
#include <WiFi.h>
#include <cmath>
#include <cstdlib>

namespace {
constexpr const char *kHostname = "scapesense";

bool stationRequest(AsyncWebServerRequest *request) {
  const wifi_mode_t mode = WiFi.getMode();
  if ((mode == WIFI_AP || mode == WIFI_AP_STA) &&
      request->client()->localIP() == WiFi.softAPIP()) {
    return false;
  }
  return true;
}

bool readUintParam(AsyncWebServerRequest *request, const char *name,
                   uint16_t &value) {
  if (!request->hasParam(name, true)) {
    return false;
  }
  const String text = request->getParam(name, true)->value();
  char *end = nullptr;
  const long parsed = strtol(text.c_str(), &end, 10);
  if (*text.c_str() == '\0' || *end != '\0' || parsed < 0 || parsed > 65535) {
    return false;
  }
  value = static_cast<uint16_t>(parsed);
  return true;
}

bool readFloatParam(AsyncWebServerRequest *request, const char *name,
                    float &value) {
  if (!request->hasParam(name, true)) {
    return false;
  }
  const String text = request->getParam(name, true)->value();
  char *end = nullptr;
  const float parsed = strtof(text.c_str(), &end);
  if (*text.c_str() == '\0' || *end != '\0' || !std::isfinite(parsed)) {
    return false;
  }
  value = parsed;
  return true;
}
} // namespace

StatusServer::StatusServer()
    : server_(80), events_("/api/events"),
      jsonLock_(xSemaphoreCreateMutex()), latestJson_("{}"),
      lastPushMs_(0), lastTotal_(0), running_(false), resetRequested_(false),
      tuningQueue_(xQueueCreate(4, sizeof(TuningRequest))),
      loadCellQueue_(xQueueCreate(4, sizeof(LoadCellRequest))),
      cylinderConfigQueue_(xQueueCreate(4, sizeof(CylinderConfigRequest))),
      temperatureConfigQueue_(xQueueCreate(4, sizeof(TemperatureSettings))),
      bubbleTargetQueue_(xQueueCreate(4, sizeof(float))),
      notificationConfigQueue_(xQueueCreate(2, sizeof(RemoteNotificationConfig))),
      latestTuningJson_(buildSensorTuningJson({})),
      latestDailyStatsJson_("{}") {}

AsyncWebServer &StatusServer::server() { return server_; }

void StatusServer::begin() {
  if (running_) {
    return;
  }

  if (!LittleFS.begin(true)) {
    Serial.println("[WARN] LittleFS mount failed; React dashboard unavailable");
  }

  server_
      .on("/api/status", HTTP_GET,
          [this](AsyncWebServerRequest *request) {
            request->send(200, "application/json", latestJson().c_str());
          })
      .setFilter(stationRequest);
  server_
      .on("/api/daily-stats", HTTP_GET,
          [this](AsyncWebServerRequest *request) {
            xSemaphoreTake(jsonLock_, portMAX_DELAY);
            const std::string json = latestDailyStatsJson_;
            xSemaphoreGive(jsonLock_);
            request->send(200, "application/json", json.c_str());
          })
      .setFilter(stationRequest);
  server_
      .on("/api/reset-counts", HTTP_POST,
          [this](AsyncWebServerRequest *request) {
            resetRequested_.store(true);
            request->send(202, "application/json", "{\"reset\":true}");
          })
      .setFilter(stationRequest);
  server_
      .on("/api/config", HTTP_GET,
          [this](AsyncWebServerRequest *request) {
            xSemaphoreTake(jsonLock_, portMAX_DELAY);
            const std::string json = latestTuningJson_;
            xSemaphoreGive(jsonLock_);
            request->send(200, "application/json", json.c_str());
          })
      .setFilter(stationRequest);
  server_
      .on("/api/config/apply", HTTP_POST,
          [this](AsyncWebServerRequest *request) {
            handleTuningRequest(request, TuningAction::Apply);
          })
      .setFilter(stationRequest);
  server_
      .on("/api/config/save", HTTP_POST,
          [this](AsyncWebServerRequest *request) {
            handleTuningRequest(request, TuningAction::Save);
          })
      .setFilter(stationRequest);
  server_
      .on("/api/config/defaults", HTTP_POST,
          [this](AsyncWebServerRequest *request) {
            const TuningRequest tuningRequest{};
            xQueueSend(tuningQueue_, &tuningRequest, 0);
            request->send(202, "application/json", "{\"accepted\":true}");
          })
      .setFilter(stationRequest);
  server_
      .on("/api/load-cell/tare", HTTP_POST,
          [this](AsyncWebServerRequest *request) {
            const LoadCellRequest loadCellRequest{LoadCellAction::Tare};
            queueLoadCellRequest(request, loadCellRequest);
          })
      .setFilter(stationRequest);
  server_
      .on("/api/load-cell/calibrate", HTTP_POST,
          [this](AsyncWebServerRequest *request) {
            LoadCellRequest loadCellRequest{LoadCellAction::Calibrate};
            if (!readFloatParam(request, "knownMassG",
                                loadCellRequest.knownMassG) ||
                !isValidKnownMass(loadCellRequest.knownMassG)) {
              request->send(400, "application/json",
                            "{\"error\":\"Known mass must be 0.1-100000 g\"}");
              return;
            }
            queueLoadCellRequest(request, loadCellRequest);
          })
      .setFilter(stationRequest);
  server_
      .on("/api/cylinder/config", HTTP_POST,
          [this](AsyncWebServerRequest *request) {
            CylinderConfigRequest cylinderRequest;
            if (!readFloatParam(request, "emptyCylinderG",
                                cylinderRequest.emptyWeightG) ||
                !readFloatParam(request, "co2CapacityG",
                                cylinderRequest.capacityG) ||
                !isValidCylinderConfiguration(cylinderRequest.emptyWeightG,
                                              cylinderRequest.capacityG)) {
              request->send(400, "application/json",
                            "{\"error\":\"Invalid cylinder configuration\"}");
              return;
            }
            if (xQueueSend(cylinderConfigQueue_, &cylinderRequest, 0) !=
                pdTRUE) {
              request->send(503, "application/json",
                            "{\"error\":\"Configuration queue full\"}");
              return;
            }
            request->send(202, "application/json", "{\"accepted\":true}");
          })
      .setFilter(stationRequest);
  server_
      .on("/dashboard", HTTP_GET,
          [](AsyncWebServerRequest *request) {
            request->send(LittleFS, "/index.html", "text/html");
          })
      .setFilter(stationRequest);

  server_.on("/api/notifications/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
    const String origin = request->hasHeader("Origin") ? request->getHeader("Origin")->value() : "";
    if (origin != String("http://") + request->host()) {
      request->send(403, "application/json", "{\"error\":\"Use the local dashboard to configure notifications\"}");
      return;
    }
    RemoteNotificationConfig settings;
    const auto readText = [request](const char *name, auto &field, bool required) {
      if (!request->hasParam(name, true)) return !required;
      const String text = request->getParam(name, true)->value();
      if (text.length() >= sizeof(field) || text.length() != std::strlen(text.c_str())) return false;
      std::memcpy(field, text.c_str(), text.length() + 1);
      return true;
    };
    const String enabled = request->hasParam("enabled", true) ? request->getParam("enabled", true)->value() : "";
    settings.enabled = enabled == "1";
    if ((enabled != "0" && enabled != "1") || !readText("projectId", settings.projectId, true) ||
        !readText("deviceId", settings.deviceId, true) || !readText("deviceKey", settings.deviceKey, false)) {
      request->send(400, "application/json", "{\"error\":\"Invalid notification settings\"}");
    } else if (xQueueSend(notificationConfigQueue_, &settings, 0) != pdTRUE) {
      request->send(503, "application/json", "{\"error\":\"Configuration queue full\"}");
    } else {
      request->send(202, "application/json", "{\"accepted\":true}");
    }
  }).setFilter(stationRequest);
  server_.on("/api/bubbles/target", HTTP_POST, [this](AsyncWebServerRequest *request) {
    float target = 0;
    if (!readFloatParam(request, "targetBps", target) || !validBubbleTarget(target)) {
      request->send(400, "application/json", "{\"error\":\"Target must be 0-20 BPS; 0 disables it\"}");
    } else if (xQueueSend(bubbleTargetQueue_, &target, 0) != pdTRUE) {
      request->send(503, "application/json", "{\"error\":\"Configuration queue full\"}");
    } else {
      request->send(202, "application/json", "{\"accepted\":true}");
    }
  }).setFilter(stationRequest);
  server_.on("/api/temperature/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
    TemperatureSettings settings;
    if (!readFloatParam(request, "targetC", settings.targetC) ||
        !readFloatParam(request, "toleranceC", settings.toleranceC) || !validTemperatureSettings(settings)) {
      request->send(400, "application/json", "{\"error\":\"Target must be 0-40 C and tolerance 0.1-10 C\"}");
    } else if (xQueueSend(temperatureConfigQueue_, &settings, 0) != pdTRUE) {
      request->send(503, "application/json", "{\"error\":\"Configuration queue full\"}");
    } else {
      request->send(202, "application/json", "{\"accepted\":true}");
    }
  }).setFilter(stationRequest);
  server_.on("/api/device/reset-wifi", HTTP_POST, [this](AsyncWebServerRequest *request) {
    queueMaintenanceRequest(request, false);
  }).setFilter(stationRequest);
  server_.on("/api/device/factory-reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
    queueMaintenanceRequest(request, true);
  }).setFilter(stationRequest);

  events_.setFilter(stationRequest);
  events_.onConnect([this](AsyncEventSourceClient *client) {
    xSemaphoreTake(jsonLock_, portMAX_DELAY);
    const std::string status = latestJson_;
    const std::string tuning = latestTuningJson_;
    const std::string stats = latestDailyStatsJson_;
    const std::string operation = latestLoadCellOperationJson_;
    xSemaphoreGive(jsonLock_);
    client->send(status.c_str(), "status", 0, 2000);
    client->send(tuning.c_str(), "config");
    client->send(stats.c_str(), "daily-stats");
    client->send(operation.c_str(), "load-cell");
  });
  server_.addHandler(&events_);

  server_.serveStatic("/", LittleFS, "/")
      .setDefaultFile("index.html")
      .setCacheControl("no-cache")
      .setFilter(stationRequest);

  WebSerial.begin(&server_);
  WebSerial.onMessage([this](const String &message) {
    if (message == "status") {
      WebSerial.println(latestJson().c_str());
    } else {
      WebSerial.println("Commands: status");
    }
  });
  ElegantOTA.begin(&server_);
  server_.begin();

  if (MDNS.begin(kHostname)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("ScapeSense dashboard: http://%s.local\n", kHostname);
    WebSerial.println("ScapeSense WebSerial ready");
  }
  running_ = true;
}

bool StatusServer::isRunning() const { return running_; }

bool StatusServer::takeResetRequest() {
  return resetRequested_.exchange(false);
}

bool StatusServer::takeTuningRequest(TuningRequest &request) {
  return xQueueReceive(tuningQueue_, &request, 0) == pdTRUE;
}

bool StatusServer::takeLoadCellRequest(LoadCellRequest &request) {
  return xQueueReceive(loadCellQueue_, &request, 0) == pdTRUE;
}

bool StatusServer::takeCylinderConfigRequest(CylinderConfigRequest &request) {
  return xQueueReceive(cylinderConfigQueue_, &request, 0) == pdTRUE;
}

bool StatusServer::takeTemperatureConfigRequest(TemperatureSettings &settings) {
  return xQueueReceive(temperatureConfigQueue_, &settings, 0) == pdTRUE;
}

bool StatusServer::takeNotificationConfigRequest(RemoteNotificationConfig &settings) {
  return xQueueReceive(notificationConfigQueue_, &settings, 0) == pdTRUE;
}

bool StatusServer::takeBubbleTargetRequest(float &targetBps) {
  return xQueueReceive(bubbleTargetQueue_, &targetBps, 0) == pdTRUE;
}

uint8_t StatusServer::maintenanceAction() const { return maintenanceAction_.load(); }

void StatusServer::completeSettingsOperation(const char *operation, bool success) {
  const std::string json = std::string("{\"operation\":\"") + operation +
      "\",\"success\":" + (success ? "true" : "false") + "}";
  events_.send(json.c_str(), "settings-result");
  if (!success && std::strcmp(operation, "Erase device memory") == 0) maintenanceAction_.store(0);
}

void StatusServer::queueMaintenanceRequest(AsyncWebServerRequest *request, bool factory) {
  const String confirmation = request->hasParam("confirmation", true) ? request->getParam("confirmation", true)->value() : String();
  const String origin = request->hasHeader("Origin") ? request->getHeader("Origin")->value() : String();
  const String host = request->host();
  if (!validResetConfirmation(factory, confirmation.c_str(), origin.c_str(), host.c_str())) {
    request->send(403, "application/json", "{\"error\":\"Same-origin request and explicit reset confirmation required\"}");
    return;
  }
  uint8_t expected = 0;
  if (!maintenanceAction_.compare_exchange_strong(expected, factory ? 2 : 1)) {
    request->send(409, "application/json", "{\"error\":\"Device restart already pending\"}");
    return;
  }
  request->send(202, "application/json", "{\"accepted\":true,\"restarting\":true}");
}

void StatusServer::queueLoadCellRequest(AsyncWebServerRequest *request,
                                        const LoadCellRequest &operation) {
  if (loadCellPending_.exchange(true)) {
    request->send(409, "application/json", "{\"error\":\"Load cell is busy\"}");
    return;
  }
  xSemaphoreTake(jsonLock_, portMAX_DELAY);
  const uint32_t id = ++loadCellOperationId_;
  latestLoadCellOperationJson_ = "{\"id\":" + std::to_string(id) +
                                ",\"state\":\"running\"}";
  const bool queued = xQueueSend(loadCellQueue_, &operation, 0) == pdTRUE;
  xSemaphoreGive(jsonLock_);
  if (!queued) {
    completeLoadCellOperation(false, "Load-cell queue full");
    request->send(503, "application/json", "{\"error\":\"Load-cell queue full\"}");
    return;
  }
  const std::string response = "{\"accepted\":true,\"id\":" +
                               std::to_string(id) + "}";
  request->send(202, "application/json", response.c_str());
}

void StatusServer::completeLoadCellOperation(bool success, const char *error) {
  xSemaphoreTake(jsonLock_, portMAX_DELAY);
  latestLoadCellOperationJson_ = "{\"id\":" + std::to_string(loadCellOperationId_) +
      ",\"state\":\"" + (success ? "succeeded" : "failed") +
      "\",\"error\":\"" + error + "\"}";
  const std::string json = latestLoadCellOperationJson_;
  xSemaphoreGive(jsonLock_);
  events_.send(json.c_str(), "load-cell");
  loadCellPending_.store(false);
}

void StatusServer::publishTuning(const SensorTuning &tuning) {
  const std::string json = buildSensorTuningJson(tuning);
  xSemaphoreTake(jsonLock_, portMAX_DELAY);
  const bool changed = latestTuningJson_ != json;
  latestTuningJson_ = json;
  xSemaphoreGive(jsonLock_);
  if (running_ && changed) {
    events_.send(json.c_str(), "config");
  }
}

void StatusServer::publishDailyStats(const DailyStatsState &stats) {
  const std::string json = buildDailyStatsJson(stats);
  xSemaphoreTake(jsonLock_, portMAX_DELAY);
  const bool changed = latestDailyStatsJson_ != json;
  latestDailyStatsJson_ = json;
  xSemaphoreGive(jsonLock_);
  if (running_ && changed) {
    events_.send(json.c_str(), "daily-stats");
  }
}

void StatusServer::publish(const BubbleMetrics &metrics,
                           const DeviceInfo &device) {
  const uint32_t now = millis();
  const bool bubbleSeen = metrics.totalBubbles != lastTotal_;
  const uint32_t sinceLast = now - lastPushMs_;
  const bool due = sinceLast >= kHeartbeatMs ||
                   (bubbleSeen && sinceLast >= kMinBubblePushMs);
  if (!running_ || !due) {
    return;
  }

  lastPushMs_ = now;
  lastTotal_ = metrics.totalBubbles;
  const std::string json = buildStatusJson(metrics, device);
  storeJson(json);
  events_.send(json.c_str(), "status");
}

std::string StatusServer::latestJson() {
  xSemaphoreTake(jsonLock_, portMAX_DELAY);
  std::string copy = latestJson_;
  xSemaphoreGive(jsonLock_);
  return copy;
}

void StatusServer::storeJson(const std::string &json) {
  xSemaphoreTake(jsonLock_, portMAX_DELAY);
  latestJson_ = json;
  xSemaphoreGive(jsonLock_);
}

void StatusServer::handleTuningRequest(AsyncWebServerRequest *request,
                                       TuningAction action) {
  SensorTuning tuning;
  uint16_t polarity = 0;
  if (!readUintParam(request, "threshold", tuning.threshold) ||
      !readUintParam(request, "hysteresis", tuning.hysteresis) ||
      !readUintParam(request, "minSeparationMs", tuning.minSeparationMs) ||
      !readUintParam(request, "maxReliableBps", tuning.maxReliableBps) ||
      !readUintParam(request, "polarity", polarity) || polarity > 255) {
    request->send(400, "application/json",
                  "{\"error\":\"Missing or invalid tuning value\"}");
    return;
  }
  tuning.polarity = static_cast<uint8_t>(polarity);
  const SensorTuningError error = validateSensorTuning(tuning);
  if (error != SensorTuningError::None) {
    const String response =
        String("{\"error\":\"") + sensorTuningErrorMessage(error) + "\"}";
    request->send(400, "application/json", response);
    return;
  }

  const TuningRequest tuningRequest{tuning, action};
  if (xQueueSend(tuningQueue_, &tuningRequest, 0) != pdTRUE) {
    request->send(503, "application/json", "{\"error\":\"Tuning queue full\"}");
    return;
  }
  request->send(202, "application/json", "{\"accepted\":true}");
}
