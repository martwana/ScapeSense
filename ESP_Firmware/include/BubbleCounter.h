#pragma once

#include "BubbleCounterConfig.h"
#include "RollingAverage.h"
#include <algorithm>
#include <cstdint>

enum class BubbleStatus { OK, Disconnected, Saturated };

struct BubbleMetrics {
  uint32_t totalBubbles = 0;
  float bubblesPerSecond = 0.0f;
  float bubblesPerMinute = 0.0f;
  uint32_t lastBubbleTimestampMs = 0;
  uint16_t baseline = 0;
  uint16_t lastRaw = 0;
  bool rateReliable = true;
  BubbleStatus status = BubbleStatus::OK;
};

class BubbleCounter {
public:
  explicit inline BubbleCounter(const BubbleCounterConfig &config = {});

  inline void update(uint16_t raw, uint32_t nowMs);
  inline void reset();
  inline void restoreTotal(uint32_t totalBubbles);

  inline const BubbleMetrics &metrics() const;
  inline void setConfig(const BubbleCounterConfig &config);

private:
  static constexpr uint16_t kAdcMin = 0;
  static constexpr uint16_t kAdcMax = 4095;
  static constexpr uint16_t kStatusSampleCount = 50;
  static constexpr uint32_t kIdleTimeoutMs = 2000;
  static constexpr uint32_t kMsPerSecond = 1000;
  static constexpr uint32_t kSecondsPerMinute = 60;

  BubbleCounterConfig config_;
  BubbleMetrics metrics_;
  RollingAverage<float, 10> bpsAverage_;
  bool inEvent_;
  uint32_t lastEventMs_;
  uint32_t lastIntervalMs_;
  uint32_t eventStartMs_;
  uint16_t consecutiveExtremeSamples_;

  inline void updateBaseline(uint16_t raw);
  inline void detectEvent(uint16_t raw, uint32_t nowMs);
  inline void recordEvent(uint32_t nowMs);
  inline bool isTriggered(uint16_t raw) const;
  inline bool isReset(uint16_t raw) const;
  inline void updateStatus(uint16_t raw);
  inline void updateIdleRate(uint32_t nowMs);
  inline uint32_t minReliableIntervalMs() const;
};

inline BubbleCounter::BubbleCounter(const BubbleCounterConfig &config)
    : config_(config), metrics_(), bpsAverage_(), inEvent_(false),
      lastEventMs_(0), lastIntervalMs_(0), eventStartMs_(0),
      consecutiveExtremeSamples_(0) {}

inline void BubbleCounter::reset() {
  metrics_ = BubbleMetrics{};
  bpsAverage_.clear();
  inEvent_ = false;
  lastEventMs_ = 0;
  lastIntervalMs_ = 0;
  eventStartMs_ = 0;
  consecutiveExtremeSamples_ = 0;
}

inline void BubbleCounter::restoreTotal(uint32_t totalBubbles) {
  reset();
  metrics_.totalBubbles = totalBubbles;
}

inline const BubbleMetrics &BubbleCounter::metrics() const { return metrics_; }

inline void BubbleCounter::setConfig(const BubbleCounterConfig &config) {
  config_ = config;
}

inline void BubbleCounter::update(uint16_t raw, uint32_t nowMs) {
  metrics_.lastRaw = raw;
  updateBaseline(raw);
  detectEvent(raw, nowMs);
  updateStatus(raw);
  updateIdleRate(nowMs);
}

inline void BubbleCounter::updateBaseline(uint16_t raw) {
  if (metrics_.baseline == 0) {
    metrics_.baseline = raw;
    return;
  }

  if (inEvent_) {
    return;
  }

  metrics_.baseline = (static_cast<uint32_t>(metrics_.baseline) * 7 + raw) / 8;
}

inline void BubbleCounter::detectEvent(uint16_t raw, uint32_t nowMs) {
  if (isTriggered(raw) && !inEvent_ &&
      (metrics_.totalBubbles == 0 ||
       nowMs - lastEventMs_ >= config_.minSeparationMs)) {
    recordEvent(nowMs);
    inEvent_ = true;
    eventStartMs_ = nowMs;
  } else if (isReset(raw) && inEvent_) {
    inEvent_ = false;
  }

  if (inEvent_ && config_.maxReliableBps > 0) {
    const uint32_t reliableInterval = minReliableIntervalMs();
    if (nowMs - eventStartMs_ >= reliableInterval) {
      metrics_.rateReliable = false;
    }
  }
}

inline void BubbleCounter::recordEvent(uint32_t nowMs) {
  ++metrics_.totalBubbles;
  metrics_.lastBubbleTimestampMs = nowMs;

  if (metrics_.totalBubbles > 1) {
    lastIntervalMs_ = nowMs - lastEventMs_;
    metrics_.bubblesPerSecond =
        static_cast<float>(kMsPerSecond) /
        static_cast<float>(std::max<uint32_t>(lastIntervalMs_, 1u));
    bpsAverage_.add(metrics_.bubblesPerSecond);
    metrics_.bubblesPerMinute =
        bpsAverage_.average() * static_cast<float>(kSecondsPerMinute);

    if (config_.maxReliableBps > 0) {
      metrics_.rateReliable = lastIntervalMs_ >= minReliableIntervalMs();
    }
  }

  lastEventMs_ = nowMs;
}

inline bool BubbleCounter::isTriggered(uint16_t raw) const {
  const uint16_t baseline = metrics_.baseline;
  if (config_.polarity == 0) {
    return raw + config_.threshold < baseline;
  }
  return raw > baseline + config_.threshold;
}

inline bool BubbleCounter::isReset(uint16_t raw) const {
  const uint16_t baseline = metrics_.baseline;
  if (config_.polarity == 0) {
    return raw + config_.hysteresis >= baseline;
  }
  return raw <= baseline + config_.hysteresis;
}

inline void BubbleCounter::updateStatus(uint16_t raw) {
  if (raw == kAdcMin || raw == kAdcMax) {
    ++consecutiveExtremeSamples_;
  } else {
    consecutiveExtremeSamples_ = 0;
  }

  if (consecutiveExtremeSamples_ >= kStatusSampleCount) {
    metrics_.status =
        (raw == kAdcMax) ? BubbleStatus::Saturated : BubbleStatus::Disconnected;
  } else {
    metrics_.status = BubbleStatus::OK;
  }
}

inline void BubbleCounter::updateIdleRate(uint32_t nowMs) {
  if (metrics_.totalBubbles == 0) {
    return;
  }

  if (nowMs - lastEventMs_ > kIdleTimeoutMs) {
    metrics_.bubblesPerSecond = 0.0f;
    metrics_.bubblesPerMinute = 0.0f;
    metrics_.rateReliable = true;
    bpsAverage_.clear();
  }
}

inline uint32_t BubbleCounter::minReliableIntervalMs() const {
  return config_.maxReliableBps > 0 ? kMsPerSecond / config_.maxReliableBps : 0;
}
