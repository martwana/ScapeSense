#pragma once

#include "DailyStats.h"
#include "LoadCellReading.h"
#include "OledType.h"
#include "IotProtocol.h"
#include "MonitoringSettings.h"
#include "RemoteNotifications.h"
#include "SensorTuning.h"
#include <Preferences.h>
#include <array>
#include <cstdint>

class ConfigManager {
public:
  using OledType = ::OledType;

  ConfigManager();
  bool begin();
  bool save();
  bool loadDailyStats(DailyStatsState &state);
  bool saveDailyStats(const DailyStatsState &state);
  void loadDefaults();
  bool eraseDeviceSettings();
  TemperatureSettings temperatureSettings() const;
  bool saveTemperatureSettings(const TemperatureSettings &settings);
  float bubbleTargetBps() const;
  bool saveBubbleTarget(float targetBps);
  RemoteNotificationConfig notificationConfig() const;
  bool saveNotificationConfig(const RemoteNotificationConfig &settings);
  IotConfig iotConfig() const;
  bool saveIotConfig(const IotConfig &settings);

  const char *deviceName() const;
  void setDeviceName(const char *name);

  uint8_t sensorPin() const;
  void setSensorPin(uint8_t pin);

  uint8_t ledPin() const;
  void setLedPin(uint8_t pin);

  uint8_t temperaturePin() const;
  void setTemperaturePin(uint8_t pin);

  uint32_t sampleRateHz() const;
  void setSampleRateHz(uint32_t rate);

  uint16_t bubbleThreshold() const;
  void setBubbleThreshold(uint16_t threshold);

  uint16_t bubbleHysteresis() const;
  void setBubbleHysteresis(uint16_t hysteresis);

  uint16_t minBubbleSeparationMs() const;
  void setMinBubbleSeparationMs(uint16_t ms);

  uint16_t maxReliableBps() const;
  void setMaxReliableBps(uint16_t bps);

  uint8_t detectionPolarity() const;
  void setDetectionPolarity(uint8_t polarity);
  SensorTuning sensorTuning() const;
  void setSensorTuning(const SensorTuning &tuning);

  const char *wifiSsid() const;
  void setWifiSsid(const char *ssid);

  const char *wifiPassword() const;
  void setWifiPassword(const char *password);

  bool hasWifiCredentials() const;
  void clearWifiCredentials();

  OledType oledType() const;
  void setOledType(OledType type);

  uint8_t oledSdaPin() const;
  void setOledSdaPin(uint8_t pin);

  uint8_t oledSclPin() const;
  void setOledSclPin(uint8_t pin);

  uint8_t loadCellDoutPin() const;
  void setLoadCellDoutPin(uint8_t pin);

  uint8_t loadCellSckPin() const;
  void setLoadCellSckPin(uint8_t pin);

  int32_t loadCellOffset() const;
  void setLoadCellOffset(int32_t offset);

  float loadCellScale() const;
  void setLoadCellScale(float scale);
  bool loadCellCalibrated() const;

  float emptyCylinderWeightG() const;
  void setEmptyCylinderWeightG(float weightG);

  float co2CapacityG() const;
  void setCo2CapacityG(float capacityG);

private:
  Preferences preferences_;
  bool preferencesStarted_;
  std::array<char, 32> deviceName_;
  uint8_t sensorPin_;
  uint8_t ledPin_;
  uint8_t temperaturePin_;
  uint32_t sampleRateHz_;
  uint16_t bubbleThreshold_;
  uint16_t bubbleHysteresis_;
  uint16_t minBubbleSeparationMs_;
  uint16_t maxReliableBps_;
  uint8_t detectionPolarity_;
  std::array<char, 64> wifiSsid_;
  std::array<char, 64> wifiPassword_;
  OledType oledType_;
  uint8_t oledSdaPin_;
  uint8_t oledSclPin_;
  uint8_t loadCellDoutPin_;
  uint8_t loadCellSckPin_;
  int32_t loadCellOffset_;
  float loadCellScale_;
  float emptyCylinderWeightG_;
  float co2CapacityG_;
  TemperatureSettings temperatureSettings_{};
  float bubbleTargetBps_ = 0;
  RemoteNotificationConfig notificationConfig_{};
  IotConfig iotConfig_{};
};
