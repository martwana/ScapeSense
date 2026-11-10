#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

template <typename T, size_t N>
class RollingAverage {
public:
    static_assert(N > 0, "RollingAverage requires N > 0");

    void add(T value) {
        if (count_ == N) {
            sum_ -= buffer_[index_];
        } else {
            ++count_;
        }

        sum_ += value;
        buffer_[index_] = value;
        index_ = (index_ + 1) % N;
    }

    T average() const {
        if (count_ == 0) {
            return T{};
        }
        return sum_ / static_cast<T>(count_);
    }

    void clear() {
        index_ = 0;
        count_ = 0;
        sum_ = T{};
        buffer_.fill(T{});
    }

    size_t size() const { return count_; }
    bool full() const { return count_ == N; }
    bool empty() const { return count_ == 0; }

private:
    std::array<T, N> buffer_;
    size_t index_ = 0;
    size_t count_ = 0;
    T sum_ = T{};
};
