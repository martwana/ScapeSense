#pragma once

#include "BubbleCounter.h"
#include "ConfigManager.h"
#include "LoadCellReading.h"
#include "TemperatureReading.h"
#include <U8g2lib.h>

class DisplayManager {
public:
  explicit DisplayManager(ConfigManager &config);
  ~DisplayManager();

  bool begin();
  bool isInitialized() const;
  bool isPoweredOn() const;
  void setPower(bool on);

  void showTestMessage();
  void showDiagnostics(uint16_t rawValue, uint32_t rateHz);
  void showCounting(const BubbleMetrics &metrics,
                    const TemperatureReading &temperature,
                    const LoadCellReading &loadCell);
  void showProvisioning(const char *ssid, const char *password, const char *ip);
  void showConnected(const char *ip);
  void showError(const char *line1, const char *line2);

private:
  ConfigManager &config_;
  U8G2 *u8g2_;
  bool poweredOn_;
};
