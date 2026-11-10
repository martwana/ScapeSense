#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct LegacyDailyRecord {
  uint32_t dayKey;
  uint32_t bubbleCount;
  uint32_t startEpoch;
  uint32_t stopEpoch;
};

struct LegacyDailyStatsState {
  uint32_t version;
  uint32_t currentDayKey;
  uint32_t currentCount;
  uint32_t startEpoch;
  uint32_t stopEpoch;
  uint32_t lastQualifiedEpoch;
  bool active;
  uint8_t recentCount;
  uint8_t historyCount;
  std::array<uint32_t, 12> recentBubbles;
  std::array<LegacyDailyRecord, 30> history;
};

struct DailyRecord {
  DailyRecord(uint32_t day = 0, uint32_t count = 0, uint32_t start = 0,
              uint32_t stop = 0, uint32_t flowing = 0,
              bool estimated = false)
      : dayKey(day), bubbleCount(count), startEpoch(start), stopEpoch(stop),
        activeSeconds(flowing), flowDurationEstimated(estimated) {}

  uint32_t dayKey;
  uint32_t bubbleCount;
  uint32_t startEpoch;
  uint32_t stopEpoch;
  uint32_t activeSeconds;
  bool flowDurationEstimated;
};

struct DailyStatsState {
  static constexpr uint32_t kVersion = 2;
  static constexpr size_t kHistoryCapacity = 30;
  static constexpr size_t kStartBubbleCount = 12;

  uint32_t version = kVersion;
  uint32_t currentDayKey = 0;
  uint32_t currentCount = 0;
  uint32_t startEpoch = 0;
  uint32_t stopEpoch = 0;
  uint32_t lastQualifiedEpoch = 0;
  uint32_t activeSeconds = 0;
  uint32_t sessionStartEpoch = 0;
  bool flowDurationEstimated = false;
  bool active = false;
  uint8_t recentCount = 0;
  uint8_t historyCount = 0;
  std::array<uint32_t, kStartBubbleCount> recentBubbles{};
  std::array<DailyRecord, kHistoryCapacity> history{};
};

inline bool migrateDailyStatsV1(const LegacyDailyStatsState &legacy,
                                DailyStatsState &state) {
  if (legacy.version != 1 ||
      legacy.historyCount > DailyStatsState::kHistoryCapacity ||
      legacy.recentCount > DailyStatsState::kStartBubbleCount) {
    return false;
  }
  state = {};
  state.currentDayKey = legacy.currentDayKey;
  state.currentCount = legacy.currentCount;
  state.startEpoch = legacy.startEpoch;
  state.stopEpoch = legacy.stopEpoch;
  state.lastQualifiedEpoch = legacy.lastQualifiedEpoch;
  state.active = legacy.active;
  state.recentCount = legacy.recentCount;
  state.historyCount = legacy.historyCount;
  state.recentBubbles = legacy.recentBubbles;
  for (size_t i = 0; i < legacy.historyCount; ++i) {
    const LegacyDailyRecord &old = legacy.history[i];
    const uint32_t duration =
        old.stopEpoch > old.startEpoch ? old.stopEpoch - old.startEpoch : 0;
    state.history[i] = {old.dayKey, old.bubbleCount, old.startEpoch,
                        old.stopEpoch, duration, old.startEpoch != 0};
  }
  if (legacy.active && legacy.lastQualifiedEpoch > legacy.startEpoch) {
    state.activeSeconds = legacy.lastQualifiedEpoch - legacy.startEpoch;
    state.sessionStartEpoch = legacy.lastQualifiedEpoch;
  } else if (!legacy.active && legacy.stopEpoch > legacy.startEpoch) {
    state.activeSeconds = legacy.stopEpoch - legacy.startEpoch;
  }
  state.flowDurationEstimated = legacy.startEpoch != 0;
  return true;
}

class DailyStats {
public:
  static constexpr uint32_t kActivityWindowSeconds = 60;
  static constexpr uint32_t kStopToleranceSeconds = 300;

  DailyStats() = default;
  explicit DailyStats(const DailyStatsState &state) { restore(state); }

  bool tick(uint32_t dayKey, uint32_t epoch) {
    const bool rolledOver = ensureDay(dayKey);
    pruneRecent(epoch);
    if (state_.active &&
        epoch - state_.lastQualifiedEpoch >= kStopToleranceSeconds) {
      finishSession();
    }
    return rolledOver;
  }

  bool onBubble(uint32_t dayKey, uint32_t epoch) {
    const bool rolledOver = ensureDay(dayKey);
    ++state_.currentCount;
    pruneRecent(epoch);
    if (state_.recentCount == state_.recentBubbles.size()) {
      for (size_t i = 1; i < state_.recentBubbles.size(); ++i) {
        state_.recentBubbles[i - 1] = state_.recentBubbles[i];
      }
      --state_.recentCount;
    }
    state_.recentBubbles[state_.recentCount++] = epoch;
    if (state_.recentCount >= DailyStatsState::kStartBubbleCount) {
      if (!state_.active && state_.startEpoch == 0) {
        state_.startEpoch = state_.recentBubbles[0];
      }
      if (!state_.active) {
        state_.sessionStartEpoch = state_.recentBubbles[0];
      }
      state_.active = true;
      state_.stopEpoch = 0;
      state_.lastQualifiedEpoch = epoch;
    }
    return rolledOver;
  }

  void synchronizeCurrentCount(uint32_t dayKey, uint32_t count) {
    ensureDay(dayKey);
    state_.currentCount = count;
  }

  void resetCurrent(uint32_t dayKey) {
    state_.currentDayKey = dayKey;
    state_.currentCount = 0;
    state_.startEpoch = 0;
    state_.stopEpoch = 0;
    state_.lastQualifiedEpoch = 0;
    state_.activeSeconds = 0;
    state_.sessionStartEpoch = 0;
    state_.flowDurationEstimated = false;
    state_.active = false;
    state_.recentCount = 0;
    state_.recentBubbles.fill(0);
  }

  void restore(const DailyStatsState &state) {
    if (state.version == DailyStatsState::kVersion &&
        state.historyCount <= DailyStatsState::kHistoryCapacity &&
        state.recentCount <= DailyStatsState::kStartBubbleCount) {
      state_ = state;
    }
  }

  const DailyStatsState &state() const { return state_; }
  const DailyRecord &history(size_t index) const {
    return state_.history[index];
  }
  size_t historyCount() const { return state_.historyCount; }

  static uint32_t currentFlowSeconds(const DailyStatsState &state) {
    if (!state.active || state.sessionStartEpoch == 0 ||
        state.lastQualifiedEpoch <= state.sessionStartEpoch) {
      return state.activeSeconds;
    }
    return state.activeSeconds + state.lastQualifiedEpoch -
           state.sessionStartEpoch;
  }

private:
  DailyStatsState state_{};

  void finishSession() {
    state_.stopEpoch = state_.lastQualifiedEpoch;
    if (state_.sessionStartEpoch && state_.stopEpoch > state_.sessionStartEpoch) {
      state_.activeSeconds += state_.stopEpoch - state_.sessionStartEpoch;
    }
    state_.sessionStartEpoch = 0;
    state_.active = false;
  }

  bool ensureDay(uint32_t dayKey) {
    if (dayKey == 0 || state_.currentDayKey == dayKey) {
      return false;
    }
    if (state_.currentDayKey != 0) {
      if (state_.active) finishSession();
      DailyRecord record{
          state_.currentDayKey, state_.currentCount, state_.startEpoch,
          state_.stopEpoch, state_.activeSeconds,
          state_.flowDurationEstimated};
      appendHistory(record);
    }
    resetCurrent(dayKey);
    return true;
  }

  void appendHistory(const DailyRecord &record) {
    if (state_.historyCount == state_.history.size()) {
      for (size_t i = 1; i < state_.history.size(); ++i) {
        state_.history[i - 1] = state_.history[i];
      }
      --state_.historyCount;
    }
    state_.history[state_.historyCount++] = record;
  }

  void pruneRecent(uint32_t epoch) {
    size_t removeCount = 0;
    while (removeCount < state_.recentCount &&
           epoch - state_.recentBubbles[removeCount] > kActivityWindowSeconds) {
      ++removeCount;
    }
    if (removeCount == 0) {
      return;
    }
    for (size_t i = removeCount; i < state_.recentCount; ++i) {
      state_.recentBubbles[i - removeCount] = state_.recentBubbles[i];
    }
    state_.recentCount -= removeCount;
  }
};
