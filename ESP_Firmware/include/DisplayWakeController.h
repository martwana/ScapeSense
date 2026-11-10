#pragma once

#include <cstdint>

class DisplayWakeController {
public:
  DisplayWakeController(uint32_t awakeDurationMs, uint32_t debounceMs)
      : awakeDurationMs_(awakeDurationMs), debounceMs_(debounceMs),
        awakeUntilMs_(awakeDurationMs), lastChangeMs_(0), rawPressed_(false),
        stablePressed_(false), wakeRequested_(false) {}

  void update(bool pressed, uint32_t nowMs) {
    if (pressed != rawPressed_) {
      rawPressed_ = pressed;
      lastChangeMs_ = nowMs;
    }

    if (rawPressed_ != stablePressed_ && nowMs - lastChangeMs_ >= debounceMs_) {
      stablePressed_ = rawPressed_;
      if (stablePressed_) {
        awakeUntilMs_ = nowMs + awakeDurationMs_;
        wakeRequested_ = true;
      }
    }
  }

  bool isAwake(uint32_t nowMs) const {
    return static_cast<int32_t>(awakeUntilMs_ - nowMs) > 0;
  }

  bool isPressed() const { return stablePressed_; }

  bool takeWakeRequest() {
    const bool requested = wakeRequested_;
    wakeRequested_ = false;
    return requested;
  }

private:
  uint32_t awakeDurationMs_;
  uint32_t debounceMs_;
  uint32_t awakeUntilMs_;
  uint32_t lastChangeMs_;
  bool rawPressed_;
  bool stablePressed_;
  bool wakeRequested_;
};
