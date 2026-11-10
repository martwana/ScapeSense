#pragma once

#include "BubbleCounter.h"

class BubblePipeline {
public:
  explicit BubblePipeline(const BubbleCounterConfig &config) : counter_(config) {}

  bool update(uint16_t raw, uint32_t nowMs) {
    const uint32_t previousTotal = counter_.metrics().totalBubbles;
    counter_.update(raw, nowMs);
    return counter_.metrics().totalBubbles > previousTotal;
  }

  void configure(const BubbleCounterConfig &config) { counter_.setConfig(config); }
  void reset() { counter_.reset(); }
  void restoreTotal(uint32_t totalBubbles) {
    counter_.restoreTotal(totalBubbles);
  }
  const BubbleMetrics &metrics() const { return counter_.metrics(); }

private:
  BubbleCounter counter_;
};
