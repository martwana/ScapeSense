#pragma once

#include <algorithm>
#include <cstdint>

class Sampler {
public:
  explicit Sampler(uint32_t rateHz = 500);

  void setRate(uint32_t rateHz);
  uint32_t rateHz() const;
  uint32_t intervalUs() const;

  bool update(uint32_t nowUs);

  void reset(uint32_t nowUs);

private:
  uint32_t intervalUs_;
  uint32_t lastSampleUs_;
};

inline Sampler::Sampler(uint32_t rateHz) : intervalUs_(0), lastSampleUs_(0) {
  setRate(rateHz);
}

inline void Sampler::setRate(uint32_t rateHz) {
  rateHz = std::max<uint32_t>(rateHz, 1u);
  intervalUs_ = 1000000u / rateHz;
}

inline uint32_t Sampler::rateHz() const { return 1000000u / intervalUs_; }

inline uint32_t Sampler::intervalUs() const { return intervalUs_; }

inline void Sampler::reset(uint32_t nowUs) { lastSampleUs_ = nowUs; }

inline bool Sampler::update(uint32_t nowUs) {
  if (nowUs - lastSampleUs_ >= intervalUs_) {
    lastSampleUs_ += intervalUs_;
    return true;
  }
  return false;
}
