#pragma once

#include <cmath>
#include <cstring>
#include <string>

struct TemperatureSettings {
  float targetC = 24.0f;
  float toleranceC = 2.0f;
};

inline bool validTemperatureSettings(const TemperatureSettings &settings) {
  return std::isfinite(settings.targetC) && settings.targetC >= 0.0f &&
         settings.targetC <= 40.0f && std::isfinite(settings.toleranceC) &&
         settings.toleranceC >= 0.1f && settings.toleranceC <= 10.0f;
}

inline bool validBubbleTarget(float targetBps) {
  return std::isfinite(targetBps) && targetBps >= 0.0f && targetBps <= 20.0f;
}

inline bool validResetConfirmation(bool factory, const char *confirmation,
                                   const char *origin, const char *host) {
  return confirmation && origin && host && *host &&
         std::strcmp(confirmation, factory ? "ERASE" : "RESET WIFI") == 0 &&
         std::string(origin) == std::string("http://") + host;
}
