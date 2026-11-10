#pragma once

#include "OledType.h"
#include <cstdint>

#ifndef PROJECT_OLED_TYPE
#define PROJECT_OLED_TYPE 3
#endif
#ifndef PROJECT_OLED_SDA_PIN
#define PROJECT_OLED_SDA_PIN 5
#endif
#ifndef PROJECT_OLED_SCL_PIN
#define PROJECT_OLED_SCL_PIN 4
#endif
#ifndef PROJECT_OLED_FORCE
#define PROJECT_OLED_FORCE 1
#endif
#ifndef PROJECT_BUBBLE_SIGNAL_PIN
#define PROJECT_BUBBLE_SIGNAL_PIN 2
#endif
#ifndef PROJECT_IR_ENABLE_PIN
#define PROJECT_IR_ENABLE_PIN 18
#endif
#ifndef PROJECT_LOADCELL_DOUT_PIN
#define PROJECT_LOADCELL_DOUT_PIN 10
#endif
#ifndef PROJECT_LOADCELL_SCK_PIN
#define PROJECT_LOADCELL_SCK_PIN 6
#endif
#ifndef PROJECT_BOOT_BUTTON_PIN
#define PROJECT_BOOT_BUTTON_PIN 0
#endif
#ifndef PROJECT_HARDWARE_FORCE
#define PROJECT_HARDWARE_FORCE 1
#endif

namespace ProjectConfig {
constexpr const char *kDeviceNameDefault = "ScapeSense";
constexpr const char *kConfigNamespace = "BubbleCounter";
constexpr const char *kFirmwareVersion = "0.1.0-phase1";
constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint32_t kSerialWaitTimeoutMs = 2000;
constexpr uint32_t kDisplayWidth = 128;
constexpr uint32_t kDisplayHeight = 32;
constexpr OledType kDefaultOledType = static_cast<OledType>(PROJECT_OLED_TYPE);
constexpr uint8_t kDefaultOledSdaPin = PROJECT_OLED_SDA_PIN;
constexpr uint8_t kDefaultOledSclPin = PROJECT_OLED_SCL_PIN;
constexpr bool kForceOledDefaults = PROJECT_OLED_FORCE != 0;
constexpr uint8_t kDefaultBubbleSignalPin = PROJECT_BUBBLE_SIGNAL_PIN;
constexpr uint8_t kDefaultIrEnablePin = PROJECT_IR_ENABLE_PIN;
constexpr uint8_t kDefaultTemperaturePin = 1;
constexpr uint8_t kPhSensorPin = 4;
constexpr uint8_t kTdsSensorPin = 5;
constexpr uint8_t kCo2RelayPin = 8;
constexpr uint8_t kDosingPumpPin = 9;
constexpr uint8_t kStepperStepPin = 13;
constexpr uint8_t kStepperDirPin = 14;
constexpr uint8_t kStepperEnablePin = 15;
constexpr bool kDisplayEnabled = false;
constexpr uint8_t kDefaultLoadCellDoutPin = PROJECT_LOADCELL_DOUT_PIN;
constexpr uint8_t kDefaultLoadCellSckPin = PROJECT_LOADCELL_SCK_PIN;
constexpr uint8_t kBootButtonPin = PROJECT_BOOT_BUTTON_PIN;
constexpr bool kForceHardwareDefaults = PROJECT_HARDWARE_FORCE != 0;
} // namespace ProjectConfig
