#pragma once

#include "BubbleCounter.h"
#include "DailyStats.h"
#include "SensorTuning.h"
#include "StatusJson.h"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <atomic>
#include <string>

enum class TuningAction : uint8_t { Apply, Save };
enum class LoadCellAction : uint8_t { Tare, Calibrate };

struct LoadCellRequest {
  LoadCellRequest(LoadCellAction actionValue = LoadCellAction::Tare,
                  float knownMassValue = 0.0f)
      : action(actionValue), knownMassG(knownMassValue) {}

  LoadCellAction action;
  float knownMassG;
};

struct CylinderConfigRequest {
  CylinderConfigRequest(float emptyWeightValue = 0.0f,
                        float capacityValue = 0.0f)
      : emptyWeightG(emptyWeightValue), capacityG(capacityValue) {}

  float emptyWeightG;
  float capacityG;
};

struct TuningRequest {
  TuningRequest(const SensorTuning &tuningValue = {},
                TuningAction actionValue = TuningAction::Apply)
      : tuning(tuningValue), action(actionValue) {}

  SensorTuning tuning;
  TuningAction action;
};

class StatusServer {
public:
  StatusServer();

  AsyncWebServer &server();
  void begin();
  void publish(const BubbleMetrics &metrics, const DeviceInfo &device);
  bool isRunning() const;
  bool takeResetRequest();
  bool takeTuningRequest(TuningRequest &request);
  bool takeLoadCellRequest(LoadCellRequest &request);
  bool takeCylinderConfigRequest(CylinderConfigRequest &request);
  bool takeTemperatureConfigRequest(TemperatureSettings &settings);
  bool takeBubbleTargetRequest(float &targetBps);
  bool takeNotificationConfigRequest(RemoteNotificationConfig &settings);
  bool takeIotConfigRequest(IotConfig &settings);
  uint8_t maintenanceAction() const;
  void completeSettingsOperation(const char *operation, bool success);
  void publishTuning(const SensorTuning &tuning);
  void publishDailyStats(const DailyStatsState &stats);
  void completeLoadCellOperation(bool success, const char *error = "");

private:
  static constexpr uint32_t kHeartbeatMs = 250;
  static constexpr uint32_t kMinBubblePushMs = 40;

  AsyncWebServer server_;
  AsyncEventSource events_;
  SemaphoreHandle_t jsonLock_;
  std::string latestJson_;
  uint32_t lastPushMs_;
  uint32_t lastTotal_;
  bool running_;
  std::atomic_bool resetRequested_;
  QueueHandle_t tuningQueue_;
  QueueHandle_t loadCellQueue_;
  QueueHandle_t cylinderConfigQueue_;
  QueueHandle_t temperatureConfigQueue_;
  QueueHandle_t bubbleTargetQueue_;
  QueueHandle_t notificationConfigQueue_;
  QueueHandle_t iotConfigQueue_;
  std::atomic<uint8_t> maintenanceAction_{0};
  void queueMaintenanceRequest(AsyncWebServerRequest *request, bool factory);
  std::string latestTuningJson_;
  std::string latestDailyStatsJson_;
  std::atomic_bool loadCellPending_{false};
  uint32_t loadCellOperationId_ = 0;
  std::string latestLoadCellOperationJson_ = "{}";

  std::string latestJson();
  void storeJson(const std::string &json);
  void handleTuningRequest(AsyncWebServerRequest *request, TuningAction action);
  void queueLoadCellRequest(AsyncWebServerRequest *request,
                           const LoadCellRequest &operation);
};
