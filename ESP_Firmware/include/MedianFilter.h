#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

template <typename T, size_t Capacity> class MedianFilter {
public:
  MedianFilter() : count_(0), nextIndex_(0) {}

  void add(T value) {
    samples_[nextIndex_] = value;
    nextIndex_ = (nextIndex_ + 1) % Capacity;
    if (count_ < Capacity) {
      ++count_;
    }
  }

  T value() const {
    std::array<T, Capacity> sorted{};
    std::copy(samples_.begin(), samples_.begin() + count_, sorted.begin());
    std::sort(sorted.begin(), sorted.begin() + count_);
    return sorted[count_ / 2];
  }

  size_t count() const { return count_; }

private:
  std::array<T, Capacity> samples_{};
  size_t count_;
  size_t nextIndex_;
};
