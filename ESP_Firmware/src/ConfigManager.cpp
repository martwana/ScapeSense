#include "ConfigManager.h"
#include "ProjectConfig.h"

namespace {
constexpr const char *kKeyDeviceName = "device_name";
constexpr const char *kKeySensorPin = "sensor_pin";
constexpr const char *kKeyLedPin = "led_pin";
constexpr const char *kKeyTemperaturePin = "temp_pin";
constexpr const char *kKeySampleRate = "sample_rate";
constexpr const char *kKeyBubbleThreshold = "bubble_thr";
constexpr const char *kKeyBubbleHysteresis = "bubble_hys";
constexpr const char *kKeyMinBubbleSeparation = "bubble_sep";
constexpr const char *kKeyMaxReliableBps = "bubble_max";
constexpr const char *kKeyDetectionPolarity = "bubble_pol";
constexpr const char *kKeySsid = "wifi_ssid";
constexpr const char *kKeyPassword = "wifi_pass";
constexpr const char *kKeyOledType = "oled_type";
constexpr const char *kKeyOledSda = "oled_sda";
constexpr const char *kKeyOledScl = "oled_scl";
constexpr const char *kKeyDailyStats = "daily_stats";
constexpr const char *kKeyLoadCellDout = "load_dout";
constexpr const char *kKeyLoadCellSck = "load_sck";
constexpr const char *kKeyLoadCellOffset = "load_offset";
constexpr const char *kKeyLoadCellScale = "load_scale";
constexpr const char *kKeyEmptyCylinderWeight = "cyl_empty_g";
constexpr const char *kKeyCo2Capacity = "co2_cap_g";
constexpr float kDefaultCo2CapacityG = 2000.0f;
constexpr const char *kKeyTemperatureSettings = "temp_limits";

constexpr uint32_t kDefaultSampleRateHz = 1000;
constexpr uint16_t kDefaultBubbleThreshold = 150;
constexpr uint16_t kDefaultBubbleHysteresis = 25;
constexpr uint16_t kDefaultMinBubbleSeparationMs = 30;
constexpr uint16_t kDefaultMaxReliableBps = 10;
constexpr uint8_t kDefaultDetectionPolarity = 0;
} // namespace

ConfigManager::ConfigManager()
    : preferencesStarted_(false), deviceName_{},
      sensorPin_(ProjectConfig::kDefaultBubbleSignalPin),
      ledPin_(ProjectConfig::kDefaultIrEnablePin),
      temperaturePin_(ProjectConfig::kDefaultTemperaturePin),
      sampleRateHz_(kDefaultSampleRateHz),
      bubbleThreshold_(kDefaultBubbleThreshold),
      bubbleHysteresis_(kDefaultBubbleHysteresis),
      minBubbleSeparationMs_(kDefaultMinBubbleSeparationMs),
      maxReliableBps_(kDefaultMaxReliableBps),
      detectionPolarity_(kDefaultDetectionPolarity), wifiSsid_{},
      wifiPassword_{}, oledType_(ProjectConfig::kDefaultOledType),
      oledSdaPin_(ProjectConfig::kDefaultOledSdaPin),
      oledSclPin_(ProjectConfig::kDefaultOledSclPin),
      loadCellDoutPin_(ProjectConfig::kDefaultLoadCellDoutPin),
      loadCellSckPin_(ProjectConfig::kDefaultLoadCellSckPin),
      loadCellOffset_(0), loadCellScale_(0.0f), emptyCylinderWeightG_(0.0f),
      co2CapacityG_(kDefaultCo2CapacityG) {
  strlcpy(deviceName_.data(), ProjectConfig::kDeviceNameDefault,
          deviceName_.size());
}

bool ConfigManager::begin() {
  preferencesStarted_ =
      preferences_.begin(ProjectConfig::kConfigNamespace, false);
  loadDefaults();
  return preferencesStarted_;
}

void ConfigManager::loadDefaults() {
  temperatureSettings_ = {};
  bubbleTargetBps_ = 0;
  notificationConfig_ = {};
  iotConfig_ = {};
  if (!preferencesStarted_) {
    strlcpy(deviceName_.data(), ProjectConfig::kDeviceNameDefault,
            deviceName_.size());
    wifiSsid_.fill('\0');
    wifiPassword_.fill('\0');
    sensorPin_ = ProjectConfig::kDefaultBubbleSignalPin;
    ledPin_ = ProjectConfig::kDefaultIrEnablePin;
    temperaturePin_ = ProjectConfig::kDefaultTemperaturePin;
    sampleRateHz_ = kDefaultSampleRateHz;
    bubbleThreshold_ = kDefaultBubbleThreshold;
    bubbleHysteresis_ = kDefaultBubbleHysteresis;
    minBubbleSeparationMs_ = kDefaultMinBubbleSeparationMs;
    maxReliableBps_ = kDefaultMaxReliableBps;
    detectionPolarity_ = kDefaultDetectionPolarity;
    oledType_ = ProjectConfig::kDefaultOledType;
    oledSdaPin_ = ProjectConfig::kDefaultOledSdaPin;
    oledSclPin_ = ProjectConfig::kDefaultOledSclPin;
    loadCellDoutPin_ = ProjectConfig::kDefaultLoadCellDoutPin;
    loadCellSckPin_ = ProjectConfig::kDefaultLoadCellSckPin;
    loadCellOffset_ = 0;
    loadCellScale_ = 0.0f;
    emptyCylinderWeightG_ = 0.0f;
    co2CapacityG_ = kDefaultCo2CapacityG;
    return;
  }

  preferences_.getString(kKeyDeviceName, deviceName_.data(),
                         deviceName_.size());
  if (strlen(deviceName_.data()) == 0) {
    strlcpy(deviceName_.data(), ProjectConfig::kDeviceNameDefault,
            deviceName_.size());
  }

  preferences_.getString(kKeySsid, wifiSsid_.data(), wifiSsid_.size());
  preferences_.getString(kKeyPassword, wifiPassword_.data(),
                         wifiPassword_.size());

  sensorPin_ = ProjectConfig::kForceHardwareDefaults
                   ? ProjectConfig::kDefaultBubbleSignalPin
                   : preferences_.getUChar(
                         kKeySensorPin, ProjectConfig::kDefaultBubbleSignalPin);
  ledPin_ = ProjectConfig::kForceHardwareDefaults
                ? ProjectConfig::kDefaultIrEnablePin
                : preferences_.getUChar(kKeyLedPin,
                                        ProjectConfig::kDefaultIrEnablePin);
  temperaturePin_ = ProjectConfig::kForceHardwareDefaults
                        ? ProjectConfig::kDefaultTemperaturePin
                        : preferences_.getUChar(
                              kKeyTemperaturePin, ProjectConfig::kDefaultTemperaturePin);
  sampleRateHz_ = preferences_.getUInt(kKeySampleRate, kDefaultSampleRateHz);
  bubbleThreshold_ =
      preferences_.getUShort(kKeyBubbleThreshold, kDefaultBubbleThreshold);
  bubbleHysteresis_ =
      preferences_.getUShort(kKeyBubbleHysteresis, kDefaultBubbleHysteresis);
  minBubbleSeparationMs_ = preferences_.getUShort(
      kKeyMinBubbleSeparation, kDefaultMinBubbleSeparationMs);
  maxReliableBps_ =
      preferences_.getUShort(kKeyMaxReliableBps, kDefaultMaxReliableBps);
  detectionPolarity_ =
      preferences_.getUChar(kKeyDetectionPolarity, kDefaultDetectionPolarity);
  oledType_ = static_cast<OledType>(preferences_.getUChar(
      kKeyOledType, static_cast<uint8_t>(ProjectConfig::kDefaultOledType)));
  oledSdaPin_ =
      preferences_.getUChar(kKeyOledSda, ProjectConfig::kDefaultOledSdaPin);
  oledSclPin_ =
      preferences_.getUChar(kKeyOledScl, ProjectConfig::kDefaultOledSclPin);
  loadCellDoutPin_ =
      ProjectConfig::kForceHardwareDefaults
          ? ProjectConfig::kDefaultLoadCellDoutPin
          : preferences_.getUChar(kKeyLoadCellDout,
                                  ProjectConfig::kDefaultLoadCellDoutPin);
  loadCellSckPin_ =
      ProjectConfig::kForceHardwareDefaults
          ? ProjectConfig::kDefaultLoadCellSckPin
          : preferences_.getUChar(kKeyLoadCellSck,
                                  ProjectConfig::kDefaultLoadCellSckPin);
  loadCellOffset_ = preferences_.getInt(kKeyLoadCellOffset, 0);
  loadCellScale_ = preferences_.getFloat(kKeyLoadCellScale, 0.0f);
  emptyCylinderWeightG_ = preferences_.getFloat(kKeyEmptyCylinderWeight, 0.0f);
  co2CapacityG_ = preferences_.getFloat(kKeyCo2Capacity, kDefaultCo2CapacityG);
  float target = 0;
  if (preferences_.getBytesLength("bubble_target") == sizeof(target) &&
      preferences_.getBytes("bubble_target", &target, sizeof(target)) == sizeof(target) && validBubbleTarget(target)) {
    bubbleTargetBps_ = target;
  }
  RemoteNotificationConfig notifications;
  if (preferences_.getBytesLength("notify_config") == sizeof(notifications) &&
      preferences_.getBytes("notify_config", &notifications, sizeof(notifications)) == sizeof(notifications) && validNotificationConfig(notifications)) {
    notificationConfig_ = notifications;
  }
  IotConfig iot;
  if (preferences_.getBytesLength("iot_config") == sizeof(iot) &&
      preferences_.getBytes("iot_config", &iot, sizeof(iot)) == sizeof(iot) && validIotConfig(iot)) {
    iotConfig_ = iot;
  }
  TemperatureSettings stored;
  if (preferences_.getBytesLength(kKeyTemperatureSettings) == sizeof(stored) &&
      preferences_.getBytes(kKeyTemperatureSettings, &stored, sizeof(stored)) == sizeof(stored) &&
      validTemperatureSettings(stored)) {
    temperatureSettings_ = stored;
  }
}

TemperatureSettings ConfigManager::temperatureSettings() const { return temperatureSettings_; }

bool ConfigManager::saveTemperatureSettings(const TemperatureSettings &settings) {
  if (!preferencesStarted_ || !validTemperatureSettings(settings) ||
      preferences_.putBytes(kKeyTemperatureSettings, &settings, sizeof(settings)) != sizeof(settings)) {
    return false;
  }
  temperatureSettings_ = settings;
  return true;
}

float ConfigManager::bubbleTargetBps() const { return bubbleTargetBps_; }

bool ConfigManager::saveBubbleTarget(float targetBps) {
  if (!preferencesStarted_ || !validBubbleTarget(targetBps) ||
      preferences_.putBytes("bubble_target", &targetBps, sizeof(targetBps)) != sizeof(targetBps)) return false;
  bubbleTargetBps_ = targetBps;
  return true;
}

RemoteNotificationConfig ConfigManager::notificationConfig() const { return notificationConfig_; }

bool ConfigManager::saveNotificationConfig(const RemoteNotificationConfig &settings) {
  if (!preferencesStarted_ || !validNotificationConfig(settings) ||
      preferences_.putBytes("notify_config", &settings, sizeof(settings)) != sizeof(settings)) return false;
  notificationConfig_ = settings;
  return true;
}

IotConfig ConfigManager::iotConfig() const { return iotConfig_; }

bool ConfigManager::saveIotConfig(const IotConfig &settings) {
  if (!preferencesStarted_ || !validIotConfig(settings) ||
      preferences_.putBytes("iot_config", &settings, sizeof(settings)) != sizeof(settings)) return false;
  iotConfig_ = settings;
  return true;
}

bool ConfigManager::eraseDeviceSettings() {
  return preferencesStarted_ && preferences_.clear();
}

bool ConfigManager::loadDailyStats(DailyStatsState &state) {
  if (!preferencesStarted_) return false;
  const size_t storedSize = preferences_.getBytesLength(kKeyDailyStats);
  if (storedSize == sizeof(LegacyDailyStatsState)) {
    LegacyDailyStatsState legacy{};
    if (preferences_.getBytes(kKeyDailyStats, &legacy, sizeof(legacy)) !=
            sizeof(legacy) ||
        !migrateDailyStatsV1(legacy, state)) {
      return false;
    }
    saveDailyStats(state);
    return true;
  }
  if (storedSize != sizeof(state)) return false;
  DailyStatsState stored;
  if (preferences_.getBytes(kKeyDailyStats, &stored, sizeof(stored)) !=
          sizeof(stored) ||
      stored.version != DailyStatsState::kVersion ||
      stored.historyCount > DailyStatsState::kHistoryCapacity ||
      stored.recentCount > DailyStatsState::kStartBubbleCount) {
    return false;
  }
  state = stored;
  return true;
}

bool ConfigManager::saveDailyStats(const DailyStatsState &state) {
  return preferencesStarted_ &&
         preferences_.putBytes(kKeyDailyStats, &state, sizeof(state)) ==
             sizeof(state);
}

bool ConfigManager::save() {
  if (!preferencesStarted_) {
    return false;
  }

  preferences_.putString(kKeyDeviceName, deviceName_.data());
  preferences_.putString(kKeySsid, wifiSsid_.data());
  preferences_.putString(kKeyPassword, wifiPassword_.data());
  preferences_.putUChar(kKeySensorPin, sensorPin_);
  preferences_.putUChar(kKeyLedPin, ledPin_);
  preferences_.putUChar(kKeyTemperaturePin, temperaturePin_);
  preferences_.putUInt(kKeySampleRate, sampleRateHz_);
  preferences_.putUShort(kKeyBubbleThreshold, bubbleThreshold_);
  preferences_.putUShort(kKeyBubbleHysteresis, bubbleHysteresis_);
  preferences_.putUShort(kKeyMinBubbleSeparation, minBubbleSeparationMs_);
  preferences_.putUShort(kKeyMaxReliableBps, maxReliableBps_);
  preferences_.putUChar(kKeyDetectionPolarity, detectionPolarity_);
  preferences_.putUChar(kKeyOledType, static_cast<uint8_t>(oledType_));
  preferences_.putUChar(kKeyOledSda, oledSdaPin_);
  preferences_.putUChar(kKeyOledScl, oledSclPin_);
  preferences_.putUChar(kKeyLoadCellDout, loadCellDoutPin_);
  preferences_.putUChar(kKeyLoadCellSck, loadCellSckPin_);
  preferences_.putInt(kKeyLoadCellOffset, loadCellOffset_);
  preferences_.putFloat(kKeyLoadCellScale, loadCellScale_);
  preferences_.putFloat(kKeyEmptyCylinderWeight, emptyCylinderWeightG_);
  preferences_.putFloat(kKeyCo2Capacity, co2CapacityG_);
  return true;
}

const char *ConfigManager::deviceName() const { return deviceName_.data(); }

void ConfigManager::setDeviceName(const char *name) {
  strlcpy(deviceName_.data(), name, deviceName_.size());
}

uint8_t ConfigManager::sensorPin() const { return sensorPin_; }
void ConfigManager::setSensorPin(uint8_t pin) { sensorPin_ = pin; }

uint8_t ConfigManager::ledPin() const { return ledPin_; }
void ConfigManager::setLedPin(uint8_t pin) { ledPin_ = pin; }

uint8_t ConfigManager::temperaturePin() const { return temperaturePin_; }
void ConfigManager::setTemperaturePin(uint8_t pin) { temperaturePin_ = pin; }

uint32_t ConfigManager::sampleRateHz() const { return sampleRateHz_; }
void ConfigManager::setSampleRateHz(uint32_t rate) { sampleRateHz_ = rate; }

uint16_t ConfigManager::bubbleThreshold() const { return bubbleThreshold_; }
void ConfigManager::setBubbleThreshold(uint16_t threshold) {
  bubbleThreshold_ = threshold;
}

uint16_t ConfigManager::bubbleHysteresis() const { return bubbleHysteresis_; }
void ConfigManager::setBubbleHysteresis(uint16_t hysteresis) {
  bubbleHysteresis_ = hysteresis;
}

uint16_t ConfigManager::minBubbleSeparationMs() const {
  return minBubbleSeparationMs_;
}
void ConfigManager::setMinBubbleSeparationMs(uint16_t ms) {
  minBubbleSeparationMs_ = ms;
}

uint16_t ConfigManager::maxReliableBps() const { return maxReliableBps_; }
void ConfigManager::setMaxReliableBps(uint16_t bps) { maxReliableBps_ = bps; }

uint8_t ConfigManager::detectionPolarity() const { return detectionPolarity_; }
void ConfigManager::setDetectionPolarity(uint8_t polarity) {
  detectionPolarity_ = polarity;
}

SensorTuning ConfigManager::sensorTuning() const {
  return {bubbleThreshold_, bubbleHysteresis_, minBubbleSeparationMs_,
          maxReliableBps_, detectionPolarity_};
}

void ConfigManager::setSensorTuning(const SensorTuning &tuning) {
  bubbleThreshold_ = tuning.threshold;
  bubbleHysteresis_ = tuning.hysteresis;
  minBubbleSeparationMs_ = tuning.minSeparationMs;
  maxReliableBps_ = tuning.maxReliableBps;
  detectionPolarity_ = tuning.polarity;
}

const char *ConfigManager::wifiSsid() const { return wifiSsid_.data(); }
void ConfigManager::setWifiSsid(const char *ssid) {
  strlcpy(wifiSsid_.data(), ssid, wifiSsid_.size());
}

const char *ConfigManager::wifiPassword() const { return wifiPassword_.data(); }
void ConfigManager::setWifiPassword(const char *password) {
  strlcpy(wifiPassword_.data(), password, wifiPassword_.size());
}

bool ConfigManager::hasWifiCredentials() const {
  return strlen(wifiSsid_.data()) > 0;
}

void ConfigManager::clearWifiCredentials() {
  wifiSsid_.fill('\0');
  wifiPassword_.fill('\0');
  if (preferencesStarted_) {
    preferences_.remove(kKeySsid);
    preferences_.remove(kKeyPassword);
  }
}

ConfigManager::OledType ConfigManager::oledType() const { return oledType_; }
void ConfigManager::setOledType(OledType type) { oledType_ = type; }

uint8_t ConfigManager::oledSdaPin() const { return oledSdaPin_; }
void ConfigManager::setOledSdaPin(uint8_t pin) { oledSdaPin_ = pin; }

uint8_t ConfigManager::oledSclPin() const { return oledSclPin_; }
void ConfigManager::setOledSclPin(uint8_t pin) { oledSclPin_ = pin; }

uint8_t ConfigManager::loadCellDoutPin() const { return loadCellDoutPin_; }
void ConfigManager::setLoadCellDoutPin(uint8_t pin) { loadCellDoutPin_ = pin; }

uint8_t ConfigManager::loadCellSckPin() const { return loadCellSckPin_; }
void ConfigManager::setLoadCellSckPin(uint8_t pin) { loadCellSckPin_ = pin; }

int32_t ConfigManager::loadCellOffset() const { return loadCellOffset_; }
void ConfigManager::setLoadCellOffset(int32_t offset) {
  loadCellOffset_ = offset;
}

float ConfigManager::loadCellScale() const { return loadCellScale_; }
void ConfigManager::setLoadCellScale(float scale) { loadCellScale_ = scale; }

bool ConfigManager::loadCellCalibrated() const {
  return isValidLoadCellScale(loadCellScale_);
}

float ConfigManager::emptyCylinderWeightG() const {
  return emptyCylinderWeightG_;
}
void ConfigManager::setEmptyCylinderWeightG(float weightG) {
  emptyCylinderWeightG_ = weightG;
}

float ConfigManager::co2CapacityG() const { return co2CapacityG_; }
void ConfigManager::setCo2CapacityG(float capacityG) {
  co2CapacityG_ = capacityG;
}
