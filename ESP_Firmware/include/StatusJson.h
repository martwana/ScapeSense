#pragma once

#include "BubbleCounter.h"
#include "LoadCellReading.h"
#include "TemperatureReading.h"
#include "IotProtocol.h"
#include "MonitoringSettings.h"
#include "RemoteNotifications.h"
#include <cstdint>
#include <cstdio>
#include <string>

struct DeviceInfo {
  int wifiRssi = 0;
  uint32_t uptimeSeconds = 0;
  bool displayAwake = true;
  const char *firmwareVersion = "";
  TemperatureReading temperature{};
  TemperatureSettings temperatureSettings{};
  LoadCellReading loadCell{};
  float emptyCylinderWeightG = 0.0f;
  float co2CapacityG = 2000.0f;
  float bubbleTargetBps = 0;
  RemoteNotificationConfig notifications{};
  NotificationDeliveryState notificationDelivery{};
  IotConfig iotConfig{};
  IotStatus iot{};
};

inline const char *statusName(BubbleStatus status) {
  switch (status) {
  case BubbleStatus::Saturated:
    return "saturated";
  case BubbleStatus::Disconnected:
    return "disconnected";
  default:
    return "ok";
  }
}

inline std::string buildStatusJson(const BubbleMetrics &metrics,
                                   const DeviceInfo &device) {
  const float remainingG =
      gasRemainingG(device.loadCell.grams, device.emptyCylinderWeightG);
  const float remainingPercent = gasRemainingPercent(
      device.loadCell.grams, device.emptyCylinderWeightG, device.co2CapacityG);
  const bool gasConfigured =
      device.loadCell.ready && device.loadCell.calibrated &&
      isValidCylinderConfiguration(device.emptyCylinderWeightG,
                                   device.co2CapacityG);
  char json[1792];
  snprintf(json, sizeof(json),
           "{\"total\":%lu,\"bps\":%.2f,\"bpm\":%.1f,\"baseline\":%u,"
           "\"raw\":%u,\"reliable\":%s,\"status\":\"%s\","
           "\"temperatureC\":%.1f,\"temperatureValid\":%s,"
           "\"temperatureTargetC\":%.1f,\"temperatureToleranceC\":%.1f,\"bubbleTargetBps\":%.2f,"
           "\"weightG\":%.1f,\"weightValid\":%s,\"loadCellRaw\":%ld,"
           "\"loadCellCalibrated\":%s,\"loadCellBusy\":%s,"
           "\"emptyCylinderG\":%.1f,\"co2CapacityG\":%.1f,"
           "\"gasRemainingG\":%.1f,\"gasRemainingPercent\":%.1f,"
           "\"gasConfigured\":%s,"
           "\"rssi\":%d,\"uptime\":%lu,\"displayAwake\":%s,"
           "\"firmware\":\"%s\",\"notificationsEnabled\":%s,\"notificationsConfigured\":%s,"
           "\"notificationsProjectId\":\"%s\",\"notificationsDeviceId\":\"%s\","
           "\"notificationsSent\":%lu,\"notificationsDropped\":%lu,\"notificationsQueued\":%lu,"
           "\"notificationsLastHttp\":%d,\"notificationsRunning\":%s,\"notificationsBlocked\":%s,"
           "\"iotEnabled\":%s,\"iotConfigured\":%s,\"iotDeviceId\":\"%s\","
           "\"iotConnected\":%s,\"iotLive\":%s,\"iotPublished\":%lu,"
           "\"iotDropped\":%lu,\"iotQueued\":%lu,\"iotCommands\":%lu,\"iotLastError\":%d}",
           static_cast<unsigned long>(metrics.totalBubbles),
           metrics.bubblesPerSecond, metrics.bubblesPerMinute, metrics.baseline,
           metrics.lastRaw, metrics.rateReliable ? "true" : "false",
           statusName(metrics.status), device.temperature.temperatureC,
           device.temperature.valid ? "true" : "false",
           device.temperatureSettings.targetC, device.temperatureSettings.toleranceC, device.bubbleTargetBps,
           device.loadCell.grams,
           device.loadCell.ready ? "true" : "false",
           static_cast<long>(device.loadCell.raw),
           device.loadCell.calibrated ? "true" : "false",
           device.loadCell.busy ? "true" : "false", device.emptyCylinderWeightG,
           device.co2CapacityG, remainingG, remainingPercent,
           gasConfigured ? "true" : "false", device.wifiRssi,
           static_cast<unsigned long>(device.uptimeSeconds),
           device.displayAwake ? "true" : "false", device.firmwareVersion,
           device.notifications.enabled ? "true" : "false",
           validNotificationIdentity(device.notifications) && validNotificationConfig(device.notifications) ? "true" : "false",
           device.notifications.projectId, device.notifications.deviceId,
           static_cast<unsigned long>(device.notificationDelivery.sent),
           static_cast<unsigned long>(device.notificationDelivery.dropped),
           static_cast<unsigned long>(device.notificationDelivery.queued),
           device.notificationDelivery.lastHttp,
           device.notificationDelivery.running ? "true" : "false",
           device.notificationDelivery.blocked ? "true" : "false",
           device.iotConfig.enabled ? "true" : "false",
           device.iot.configured ? "true" : "false",
           device.iotConfig.deviceId,
           device.iot.connected ? "true" : "false",
           device.iot.liveActive ? "true" : "false",
           static_cast<unsigned long>(device.iot.published),
           static_cast<unsigned long>(device.iot.dropped),
           static_cast<unsigned long>(device.iot.queued),
           static_cast<unsigned long>(device.iot.commandsHandled),
           device.iot.lastError);
  return json;
}
