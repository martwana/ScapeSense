#include "DisplayManager.h"
#include "ProjectConfig.h"
#include "WeightFormat.h"

DisplayManager::DisplayManager(ConfigManager &config)
    : config_(config), u8g2_(nullptr), poweredOn_(false) {}

DisplayManager::~DisplayManager() { delete u8g2_; }

bool DisplayManager::begin() {
  if (!ProjectConfig::kDisplayEnabled) {
    return true;
  }
  if (u8g2_) {
    return true;
  }

  const uint8_t sda = ProjectConfig::kForceOledDefaults
                          ? ProjectConfig::kDefaultOledSdaPin
                          : config_.oledSdaPin();
  const uint8_t scl = ProjectConfig::kForceOledDefaults
                          ? ProjectConfig::kDefaultOledSclPin
                          : config_.oledSclPin();
  const ConfigManager::OledType oledType = ProjectConfig::kForceOledDefaults
                                               ? ProjectConfig::kDefaultOledType
                                               : config_.oledType();

  switch (oledType) {
  case ConfigManager::OledType::SSD1306_128x64:
    u8g2_ = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, scl,
                                                    sda);
    break;
  case ConfigManager::OledType::SH1106_128x64:
    u8g2_ = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, scl,
                                                   sda);
    break;
  case ConfigManager::OledType::SSD1306_128x32:
    u8g2_ = new U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE,
                                                       scl, sda);
    break;
  }

  if (!u8g2_) {
    return false;
  }

  if (!u8g2_->begin()) {
    delete u8g2_;
    u8g2_ = nullptr;
    return false;
  }

  Serial.printf("OLED type=%u SDA=%u SCL=%u%s\n",
                static_cast<unsigned>(oledType), sda, scl,
                ProjectConfig::kForceOledDefaults ? " (forced)" : "");
  u8g2_->setContrast(51);
  u8g2_->setFont(u8g2_font_6x10_tf);
  u8g2_->clearBuffer();
  u8g2_->sendBuffer();
  poweredOn_ = true;
  return true;
}

bool DisplayManager::isInitialized() const { return u8g2_ != nullptr; }

bool DisplayManager::isPoweredOn() const { return poweredOn_; }

void DisplayManager::setPower(bool on) {
  if (!u8g2_ || poweredOn_ == on) {
    return;
  }
  u8g2_->setPowerSave(on ? 0 : 1);
  poweredOn_ = on;
}

void DisplayManager::showTestMessage() {
  if (!u8g2_) {
    return;
  }

  const uint16_t width = u8g2_->getDisplayWidth();
  const uint16_t height = u8g2_->getDisplayHeight();

  u8g2_->clearBuffer();
  u8g2_->setFont(u8g2_font_6x10_tf);
  const char *title = "SCAPESENSE";
  u8g2_->drawStr((width - u8g2_->getStrWidth(title)) / 2, height / 2 - 2,
                 title);

  const char *subtitle = "CO2 MONITOR";
  u8g2_->drawStr((width - u8g2_->getStrWidth(subtitle)) / 2, height - 4,
                 subtitle);
  u8g2_->sendBuffer();
}

void DisplayManager::showDiagnostics(uint16_t rawValue, uint32_t rateHz) {
  if (!u8g2_) {
    return;
  }

  u8g2_->clearBuffer();
  u8g2_->setFont(u8g2_font_6x10_tf);
  u8g2_->drawStr(0, 10, "DIAG");

  char line[32];
  snprintf(line, sizeof(line), "ADC: %u", rawValue);
  u8g2_->drawStr(0, 24, line);

  snprintf(line, sizeof(line), "%u Hz", static_cast<unsigned>(rateHz));
  u8g2_->drawStr(0, u8g2_->getDisplayHeight() <= 32 ? 32 : 36, line);
  u8g2_->sendBuffer();
}

void DisplayManager::showCounting(const BubbleMetrics &metrics,
                                  const TemperatureReading &temperature,
                                  const LoadCellReading &loadCell) {
  if (!u8g2_) {
    return;
  }

  u8g2_->clearBuffer();
  u8g2_->setFont(u8g2_font_6x10_tf);
  u8g2_->drawStr(0, 10, "BPS");

  char line[32];
  snprintf(line, sizeof(line), "%.1f", metrics.bubblesPerSecond);
  u8g2_->setFont(u8g2_font_9x15_tf);
  u8g2_->drawStr(28, 12, line);

  if (!metrics.rateReliable) {
    u8g2_->drawStr(62, 12, "!");
  }

  u8g2_->setFont(u8g2_font_6x10_tf);
  const uint16_t height = u8g2_->getDisplayHeight();
  snprintf(line, sizeof(line), "Total: %lu",
           static_cast<unsigned long>(metrics.totalBubbles));
  u8g2_->drawStr(0, height <= 32 ? 22 : 24, line);

  char temperatureText[16];
  if (temperature.valid) {
    snprintf(temperatureText, sizeof(temperatureText), "%.1fC",
             temperature.temperatureC);
  } else {
    snprintf(temperatureText, sizeof(temperatureText), "--C");
  }
  char weightText[16];
  if (loadCell.ready && loadCell.calibrated) {
    snprintf(weightText, sizeof(weightText), "%s",
             formatWeightG(loadCell.grams).c_str());
  } else {
    snprintf(weightText, sizeof(weightText), "--g");
  }
  if (height <= 32) {
    snprintf(line, sizeof(line), "%s %s", temperatureText, weightText);
    u8g2_->drawStr(0, 31, line);
  } else {
    snprintf(line, sizeof(line), "Temp: %s", temperatureText);
    u8g2_->drawStr(0, 29, line);
    snprintf(line, sizeof(line), "Weight: %s", weightText);
    u8g2_->drawStr(0, 38, line);
  }
  u8g2_->sendBuffer();
}

void DisplayManager::showConnected(const char *ip) {
  if (!u8g2_) {
    return;
  }

  u8g2_->clearBuffer();
  u8g2_->setFont(u8g2_font_5x7_tf);
  const bool compact = u8g2_->getDisplayHeight() <= 32;
  u8g2_->drawStr(0, 8, "WIFI OK");
  u8g2_->drawStr(0, 17, ip);
  u8g2_->drawStr(0, compact ? 27 : 26,
                 compact ? "scapesense.local" : "scapesense");
  if (!compact) {
    u8g2_->drawStr(0, 35, ".local");
  }
  u8g2_->sendBuffer();
}

void DisplayManager::showProvisioning(const char *ssid, const char *password,
                                      const char *ip) {
  if (!u8g2_) {
    return;
  }

  char line[32];
  const bool compact = u8g2_->getDisplayHeight() <= 32;
  u8g2_->clearBuffer();
  u8g2_->setFont(u8g2_font_5x7_tf);
  u8g2_->drawStr(0, 8, "WIFI SETUP");
  u8g2_->drawStr(0, 17, ssid);
  snprintf(line, sizeof(line), "PW:%s", password);
  u8g2_->drawStr(0, compact ? 25 : 26, line);
  u8g2_->drawStr(0, compact ? 32 : 35, ip);
  u8g2_->sendBuffer();
}

void DisplayManager::showError(const char *line1, const char *line2) {
  if (!u8g2_) {
    return;
  }

  u8g2_->clearBuffer();
  u8g2_->setFont(u8g2_font_6x10_tf);
  u8g2_->drawStr(0, 10, line1);
  u8g2_->drawStr(0, 22, line2);
  u8g2_->sendBuffer();
}
