#include "BubbleAcquisition.h"
#include "BubbleCounterConfig.h"
#include "BubbleSensor.h"
#include "ConfigManager.h"
#include "DailyStats.h"
#include "DisplayManager.h"
#include "DisplayWakeController.h"
#include "IotClient.h"
#include "LoadCellSensor.h"
#include "NetworkManager.h"
#include "NotificationReporter.h"
#include "ProjectConfig.h"
#include "Sampler.h"
#include "StatusServer.h"
#include "TemperatureSensor.h"
#include <Arduino.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <esp_chip_info.h>
#include <esp_system.h>
#include <time.h>
#if ARDUINO_USB_CDC_ON_BOOT
#include <HWCDC.h>
#endif

namespace {
ConfigManager config;
Sampler sampler(config.sampleRateHz());
DisplayManager display(config);
BubbleSensor bubbleSensor(config, sampler);
BubbleAcquisition bubbleAcquisition(bubbleSensor);
StatusServer statusServer;
ScapeSenseNetworkManager networkManager(config, statusServer.server());
TemperatureSensor temperatureSensor;
LoadCellSensor loadCellSensor;
DailyStats dailyStats;
NotificationReporter notificationReporter;
IotClient iotClient;
constexpr uint32_t kIpScreenMs = 10000;
constexpr uint32_t kStatsPersistIntervalMs = 300000;
constexpr uint32_t kValidEpoch = 1609459200;
constexpr const char *kUkTimezone = "GMT0BST,M3.5.0/1,M10.5.0/2";
constexpr uint32_t kDisplayAwakeMs = 60000;
constexpr uint32_t kButtonDebounceMs = 30;
constexpr uint32_t kClearWifiHoldMs = 5000;
DisplayWakeController displayWake(kDisplayAwakeMs, kButtonDebounceMs);

const char *chipModelToString(esp_chip_model_t model) {
  switch (model) {
  case CHIP_ESP32:
    return "ESP32";
  case CHIP_ESP32S2:
    return "ESP32-S2";
  case CHIP_ESP32S3:
    return "ESP32-S3";
  case CHIP_ESP32H2:
    return "ESP32-H2";
  default:
    return "Unknown";
  }
}

void printStartupInfo() {
  esp_chip_info_t chipInfo;
  esp_chip_info(&chipInfo);

  Serial.println("=================================");
  Serial.println(" ScapeSense");
  Serial.println(" Local monitoring dashboard");
  Serial.println("=================================");
  Serial.printf("Firmware version: %s\n", ProjectConfig::kFirmwareVersion);
  Serial.printf("Chip model: %s (rev %d)\n", chipModelToString(chipInfo.model),
                chipInfo.revision);
  Serial.printf("CPU cores: %d @ %lu MHz\n", chipInfo.cores,
                static_cast<unsigned long>(ESP.getCpuFreqMHz()));
  Serial.printf(
      "Flash size: %lu MB\n",
      static_cast<unsigned long>(ESP.getFlashChipSize() / (1024 * 1024)));
  Serial.println("=================================");
}

void printSensorConfig() {
  Serial.printf("Sensor ADC pin: GPIO%u\n", config.sensorPin());
  Serial.printf("IR LED pin: GPIO%u (active-low)\n", config.ledPin());
  Serial.printf("Sample rate: %lu Hz\n",
                static_cast<unsigned long>(config.sampleRateHz()));
  Serial.printf("Threshold: %u\n", config.bubbleThreshold());
  Serial.printf("Hysteresis: %u\n", config.bubbleHysteresis());
  Serial.printf("Min separation: %u ms\n", config.minBubbleSeparationMs());
  Serial.printf("Max reliable rate: %u BPS\n", config.maxReliableBps());
  Serial.printf("Polarity: %s\n",
                config.detectionPolarity() == 0 ? "negative" : "positive");
  Serial.printf("HX711 DOUT: GPIO%u, SCK: GPIO%u\n", config.loadCellDoutPin(),
                config.loadCellSckPin());
}

void printWebSerialMetrics(const BubbleMetrics &metrics) {
  if (!statusServer.isRunning() || !WebSerial.getConnectionCount()) {
    return;
  }
  WebSerial.printf("Total: %lu | BPS: %.2f | BPM: %.1f | Baseline: %u | "
                   "Raw: %u | Reliable: %s | Status: %s\n",
                   static_cast<unsigned long>(metrics.totalBubbles),
                   metrics.bubblesPerSecond, metrics.bubblesPerMinute,
                   metrics.baseline, metrics.lastRaw,
                   metrics.rateReliable ? "yes" : "no",
                   metrics.status == BubbleStatus::OK ? "ok" : "fault");
}

void printLoadCellDiagnostics(uint32_t now) {
  const LoadCellReading reading = loadCellSensor.reading();
  const LoadCellCalibration calibration = loadCellSensor.calibration();
  char line[256];
  snprintf(line, sizeof(line),
           "[HX711] ms=%lu raw=%ld filtered=%ld grams=%.3f offset=%ld "
           "scale=%.6f ready=%u calibrated=%u busy=%u\n",
           static_cast<unsigned long>(now),
           static_cast<long>(loadCellSensor.lastRawSample()),
           static_cast<long>(reading.raw), reading.grams,
           static_cast<long>(calibration.offset), calibration.scale,
           static_cast<unsigned>(reading.ready),
           static_cast<unsigned>(reading.calibrated),
           static_cast<unsigned>(reading.busy));
  if (Serial && Serial.availableForWrite() >= static_cast<int>(strlen(line))) {
    Serial.print(line);
  }
  if (statusServer.isRunning() && WebSerial.getConnectionCount()) {
    WebSerial.print(line);
  }
}

void printMetrics(const BubbleMetrics &metrics) {
  Serial.print("Total: ");
  Serial.print(metrics.totalBubbles);
  Serial.print(" | BPS: ");
  Serial.print(metrics.bubblesPerSecond, 2);
  Serial.print(" | BPM: ");
  Serial.print(metrics.bubblesPerMinute, 1);
  Serial.print(" | Baseline: ");
  Serial.print(metrics.baseline);
  Serial.print(" | Raw: ");
  Serial.print(metrics.lastRaw);
  Serial.print(" | Reliable: ");
  Serial.print(metrics.rateReliable ? "yes" : "no");
  Serial.print(" | Status: ");
  Serial.println(metrics.status == BubbleStatus::OK ? "ok" : "fault");
}

BubbleCounterConfig counterConfigFrom(const SensorTuning &tuning) {
  BubbleCounterConfig counterConfig;
  counterConfig.threshold = tuning.threshold;
  counterConfig.hysteresis = tuning.hysteresis;
  counterConfig.minSeparationMs = tuning.minSeparationMs;
  counterConfig.maxReliableBps = tuning.maxReliableBps;
  counterConfig.polarity = tuning.polarity;
  return counterConfig;
}

bool currentLocalTime(uint32_t &dayKey, uint32_t &epoch) {
  const time_t now = time(nullptr);
  if (now < kValidEpoch) {
    return false;
  }
  struct tm localTime;
  localtime_r(&now, &localTime);
  dayKey = static_cast<uint32_t>(localTime.tm_year + 1900) * 10000 +
           static_cast<uint32_t>(localTime.tm_mon + 1) * 100 +
           static_cast<uint32_t>(localTime.tm_mday);
  epoch = static_cast<uint32_t>(now);
  return true;
}

void persistDailyStats() {
  config.saveDailyStats(dailyStats.state());
  statusServer.publishDailyStats(dailyStats.state());
}

void applyBubbleEvents(bool timeValid, uint32_t dayKey, uint32_t epoch) {
  BubbleAcquisitionEvent event;
  bool activityChanged = false;
  while (bubbleAcquisition.takeBubbleEvent(event)) {
    if (!timeValid) {
      continue;
    }
    const bool wasActive = dailyStats.state().active;
    dailyStats.onBubble(dayKey, epoch);
    activityChanged = activityChanged || wasActive != dailyStats.state().active;
  }
  if (activityChanged) {
    persistDailyStats();
  }
}

void applyTuningRequests() {
  TuningRequest request;
  while (statusServer.takeTuningRequest(request)) {
    config.setSensorTuning(request.tuning);
    bubbleAcquisition.configure(counterConfigFrom(request.tuning));
    if (request.action == TuningAction::Save) {
      config.save();
    }
    statusServer.publishTuning(request.tuning);
    Serial.printf("Sensor tuning %s: threshold=%u hysteresis=%u separation=%u "
                  "maxReliable=%u polarity=%u\n",
                  request.action == TuningAction::Save ? "saved" : "applied",
                  request.tuning.threshold, request.tuning.hysteresis,
                  request.tuning.minSeparationMs, request.tuning.maxReliableBps,
                  request.tuning.polarity);
  }
}

void applyLoadCellRequests() {
  LoadCellRequest request;
  while (statusServer.takeLoadCellRequest(request)) {
    const bool accepted = request.action == LoadCellAction::Tare
                              ? loadCellSensor.requestTare(millis())
                              : loadCellSensor.requestKnownMassCalibration(
                                    request.knownMassG, millis());
    if (!accepted) {
      statusServer.completeLoadCellOperation(false, "Load cell is unavailable or busy");
    }
    Serial.printf("Load-cell %s request %s\n",
                  request.action == LoadCellAction::Tare ? "tare"
                                                         : "calibration",
                  accepted ? "accepted" : "rejected");
  }
}

void applyCylinderConfigRequests() {
  CylinderConfigRequest request;
  while (statusServer.takeCylinderConfigRequest(request)) {
    config.setEmptyCylinderWeightG(request.emptyWeightG);
    config.setCo2CapacityG(request.capacityG);
    config.save();
    Serial.printf(
        "Cylinder configuration saved: empty=%.1f g capacity=%.1f g\n",
        request.emptyWeightG, request.capacityG);
  }
}

void applyDeviceSettings(uint32_t now) {
  RemoteNotificationConfig notifications;
  while (statusServer.takeNotificationConfigRequest(notifications)) {
    const bool saved = mergeNotificationConfig(config.notificationConfig(), notifications) && config.saveNotificationConfig(notifications);
    if (saved) notificationReporter.configure(notifications);
    statusServer.completeSettingsOperation("Notifications", saved);
  }
  IotConfig iot;
  while (statusServer.takeIotConfigRequest(iot)) {
    const bool saved = config.saveIotConfig(iot);
    if (saved) iotClient.configure(iot);
    statusServer.completeSettingsOperation("Cloud reporting", saved);
  }
  float targetBps;
  while (statusServer.takeBubbleTargetRequest(targetBps)) {
    statusServer.completeSettingsOperation("Bubble target", config.saveBubbleTarget(targetBps));
  }
  TemperatureSettings settings;
  while (statusServer.takeTemperatureConfigRequest(settings)) {
    statusServer.completeSettingsOperation("Temperature settings", config.saveTemperatureSettings(settings));
  }
  static uint32_t restartRequestedAt = 0;
  const uint8_t action = statusServer.maintenanceAction();
  if (!action) return;
  if (!restartRequestedAt) {
    restartRequestedAt = now;
    return;
  }
  if (now - restartRequestedAt < 1500) return;
  if (action == 2 && !config.eraseDeviceSettings()) {
    statusServer.completeSettingsOperation("Erase device memory", false);
    restartRequestedAt = 0;
    return;
  }
  networkManager.clearWifiCredentials();
  esp_restart();
}

void finishLoadCellCalibration() {
  LoadCellCalibrationResult result;
  while (loadCellSensor.takeCalibrationResult(result)) {
    if (!result.success) {
      statusServer.completeLoadCellOperation(false,
          "Calibration failed: check sensor connection and known mass");
      Serial.println(
          "[WARN] Load-cell calibration failed; keep the cell stable");
      continue;
    }
    const LoadCellCalibration calibration = loadCellSensor.calibration();
    config.setLoadCellOffset(calibration.offset);
    config.setLoadCellScale(calibration.scale);
    const bool saved = config.save();
    statusServer.completeLoadCellOperation(saved,
        saved ? "" : "Calibration applied but could not be saved");
    Serial.printf("Load-cell %s saved: offset=%ld scale=%.6f counts/g\n",
                  result.action == LoadCellCalibrationAction::Tare
                      ? "tare"
                      : "calibration",
                  static_cast<long>(calibration.offset), calibration.scale);
  }
}
} // namespace

void setup() {
  Serial.begin(ProjectConfig::kSerialBaudRate);
  delay(100);

  config.begin();
  printStartupInfo();
  printSensorConfig();
  DailyStatsState storedStats;
  const bool hasStoredStats = config.loadDailyStats(storedStats);
  if (hasStoredStats) {
    dailyStats.restore(storedStats);
    Serial.printf("Restored daily bubble count: %lu\n",
                  static_cast<unsigned long>(storedStats.currentCount));
  }

  if (!display.begin()) {
    Serial.println("[ERROR] OLED initialization failed");
    display.showError("OLED", "INIT FAIL");
  } else {
    Serial.println("[OK] OLED initialized");
    display.showTestMessage();
  }

  if (!bubbleSensor.begin()) {
    Serial.println("[ERROR] Bubble sensor initialization failed");
  } else {
    Serial.println("[OK] Bubble sensor initialized");
    const uint32_t initialTotal = hasStoredStats ? storedStats.currentCount : 0;
    if (!bubbleAcquisition.begin(counterConfigFrom(config.sensorTuning()),
                                 initialTotal)) {
      Serial.println("[ERROR] Bubble acquisition task failed to start");
    } else {
      Serial.println("[OK] Bubble acquisition running on core 0");
    }
  }

  if (!temperatureSensor.begin(config.temperaturePin())) {
    Serial.println("[ERROR] DS18B20 initialization failed");
  } else {
    Serial.printf("[OK] DS18B20 initialized on GPIO%u\n",
                  config.temperaturePin());
  }

  const LoadCellCalibration loadCellCalibration{config.loadCellOffset(),
                                                config.loadCellScale()};
  if (!loadCellSensor.begin(config.loadCellDoutPin(), config.loadCellSckPin(),
                            loadCellCalibration)) {
    Serial.println("[ERROR] HX711 initialization failed");
  } else {
    Serial.printf("[OK] HX711 initialized (calibrated: %s)\n",
                  loadCellSensor.calibration().scale != 0.0f ? "yes" : "no");
  }

  pinMode(ProjectConfig::kBootButtonPin, INPUT_PULLUP);

  networkManager.begin();
  if (!notificationReporter.begin(config.notificationConfig(), dailyStats.state())) {
    Serial.println("[WARN] Notification reporter unavailable; local monitoring continues");
  }
  if (!iotClient.begin(config.iotConfig())) {
    Serial.println("[WARN] AWS IoT client unavailable; local monitoring continues");
  }
}

bool isBootButtonPressed() {
  return digitalRead(ProjectConfig::kBootButtonPin) == LOW;
}

void clearWifiAndRestart() {
  Serial.println("[Wi-Fi] Clearing saved credentials and restarting...");
  networkManager.clearWifiCredentials();
  esp_restart();
}

void updateBootButton(uint32_t now) {
  static uint32_t buttonPressedSince = 0;
  displayWake.update(isBootButtonPressed(), now);
  if (displayWake.takeWakeRequest()) {
    display.setPower(true);
  }

  if (displayWake.isPressed()) {
    if (buttonPressedSince == 0) {
      buttonPressedSince = now;
    } else if (now - buttonPressedSince >= kClearWifiHoldMs) {
      clearWifiAndRestart();
    }
  } else {
    buttonPressedSince = 0;
  }

  if (!displayWake.isAwake(now)) {
    display.setPower(false);
  }
}

void loop() {
  static uint32_t lastReport = 0;
  static uint32_t lastDisplayUpdate = 0;
  static uint32_t connectedAtMs = 0;
  static uint32_t lastStatsPersistMs = 0;
  static uint32_t lastStatsPublishMs = 0;
  static bool timeConfigured = false;
  static bool statsTimeInitialized = false;
  uint32_t now = millis();
  uint32_t dayKey = 0;
  uint32_t epoch = 0;

  networkManager.update();
  updateBootButton(now);
  ElegantOTA.loop();
  if (statusServer.isRunning()) {
    WebSerial.loop();
  }

  if (networkManager.isConnected() && !timeConfigured) {
    configTzTime(kUkTimezone, "pool.ntp.org", "time.nist.gov");
    timeConfigured = true;
  }

  if (networkManager.isConnected() && !statusServer.isRunning()) {
    statusServer.publishTuning(config.sensorTuning());
    statusServer.publishDailyStats(dailyStats.state());
    statusServer.begin();
    connectedAtMs = now;
  }

  applyTuningRequests();
  applyLoadCellRequests();
  applyCylinderConfigRequests();
  applyDeviceSettings(now);

  const bool timeValid = currentLocalTime(dayKey, epoch);
  if (timeValid) {
    const bool wasActive = dailyStats.state().active;
    const bool rolledOver = dailyStats.tick(dayKey, epoch);
    if (rolledOver) {
      bubbleAcquisition.reset();
      Serial.println("Started a new daily bubble count");
    } else if (!statsTimeInitialized) {
      dailyStats.synchronizeCurrentCount(
          dayKey, bubbleAcquisition.metrics().totalBubbles);
    }
    statsTimeInitialized = true;
    if (rolledOver || wasActive != dailyStats.state().active) {
      persistDailyStats();
    }
  }

  if (statusServer.takeResetRequest()) {
    bubbleAcquisition.reset();
    dailyStats.resetCurrent(timeValid ? dayKey
                                      : dailyStats.state().currentDayKey);
    persistDailyStats();
    Serial.println("Bubble counts and daily statistics reset");
  }

  temperatureSensor.update(now);
  loadCellSensor.update(now);
  finishLoadCellCalibration();
  applyBubbleEvents(timeValid, dayKey, epoch);
  if (timeValid) {
    notificationReporter.observe(dailyStats.state(), epoch, bubbleAcquisition.metrics().bubblesPerSecond);
    iotClient.observeFlow(dailyStats.state(), epoch, bubbleAcquisition.metrics().bubblesPerSecond);
  }

  if (timeValid && now - lastStatsPersistMs >= kStatsPersistIntervalMs) {
    lastStatsPersistMs = now;
    persistDailyStats();
  } else if (statusServer.isRunning() && now - lastStatsPublishMs >= 1000) {
    lastStatsPublishMs = now;
    statusServer.publishDailyStats(dailyStats.state());
  }

  if (statusServer.isRunning()) {
    DeviceInfo device;
    device.wifiRssi = WiFi.RSSI();
    device.uptimeSeconds = now / 1000;
    device.displayAwake = display.isPoweredOn();
    device.firmwareVersion = ProjectConfig::kFirmwareVersion;
    device.temperature = temperatureSensor.reading();
    device.temperatureSettings = config.temperatureSettings();
    device.bubbleTargetBps = config.bubbleTargetBps();
    device.notifications = config.notificationConfig();
    device.notificationDelivery = notificationReporter.status();
    device.iotConfig = config.iotConfig();
    device.iot = iotClient.status();
    device.loadCell = loadCellSensor.reading();
    device.emptyCylinderWeightG = config.emptyCylinderWeightG();
    device.co2CapacityG = config.co2CapacityG();
    statusServer.publish(bubbleAcquisition.metrics(), device);

    const BubbleMetrics metrics = bubbleAcquisition.metrics();
    IotTelemetry telemetry;
    telemetry.epoch = epoch;
    telemetry.uptimeSeconds = now / 1000;
    telemetry.totalBubbles = metrics.totalBubbles;
    telemetry.bps = metrics.bubblesPerSecond;
    telemetry.bpm = metrics.bubblesPerMinute;
    telemetry.temperatureC = temperatureSensor.reading().temperatureC;
    telemetry.temperatureValid = temperatureSensor.reading().valid;
    telemetry.weightG = loadCellSensor.reading().grams;
    telemetry.weightReady = loadCellSensor.reading().ready;
    telemetry.gasRemainingPercent = gasRemainingPercent(
        loadCellSensor.reading().grams, config.emptyCylinderWeightG(),
        config.co2CapacityG());
    telemetry.flowActive = dailyStats.state().active;
    telemetry.rssi = static_cast<int16_t>(WiFi.RSSI());
    iotClient.observeTelemetry(telemetry);
  }

  if (display.isPoweredOn() && now - lastDisplayUpdate >= 250) {
    lastDisplayUpdate = now;
    if (statusServer.isRunning() && now - connectedAtMs < kIpScreenMs) {
      display.showConnected(networkManager.localIP().c_str());
    } else if (networkManager.isProvisioning()) {
      display.showProvisioning(networkManager.apSsid().c_str(),
                               networkManager.apPassword().c_str(),
                               networkManager.localIP().c_str());
    } else {
      display.showCounting(bubbleAcquisition.metrics(),
                           temperatureSensor.reading(),
                           loadCellSensor.reading());
    }
  }

  if (now - lastReport >= 1000) {
    lastReport = now;
    const BubbleMetrics metrics = bubbleAcquisition.metrics();
    printMetrics(metrics);
    printWebSerialMetrics(metrics);
    printLoadCellDiagnostics(now);
    if (networkManager.isConnected()) {
      Serial.printf("Wi-Fi IP: %s\n", networkManager.localIP().c_str());
    } else if (networkManager.isProvisioning()) {
      Serial.printf("AP %s | clients: %d\n", networkManager.apSsid().c_str(),
                    networkManager.apClientCount());
    }
  }

  yield();
}
