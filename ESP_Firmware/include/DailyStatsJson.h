#pragma once

#include "DailyStats.h"
#include <cstdio>
#include <string>

inline std::string buildDailyStatsJson(const DailyStatsState &state) {
  char item[192];
  snprintf(item, sizeof(item),
           "{\"current\":{\"day\":%lu,\"count\":%lu,\"start\":%lu,"
           "\"stop\":%lu,\"active\":%s,\"activeSeconds\":%lu,"
           "\"flowDurationEstimated\":%s},\"history\":[",
           static_cast<unsigned long>(state.currentDayKey),
           static_cast<unsigned long>(state.currentCount),
           static_cast<unsigned long>(state.startEpoch),
           static_cast<unsigned long>(state.stopEpoch),
           state.active ? "true" : "false",
           static_cast<unsigned long>(DailyStats::currentFlowSeconds(state)),
           state.flowDurationEstimated ? "true" : "false");
  std::string json(item);
  for (size_t i = 0; i < state.historyCount; ++i) {
    const DailyRecord &record = state.history[i];
    snprintf(item, sizeof(item),
             "%s{\"day\":%lu,\"count\":%lu,\"start\":%lu,\"stop\":%lu,\"activeSeconds\":%lu,\"flowDurationEstimated\":%s}",
             i == 0 ? "" : ",", static_cast<unsigned long>(record.dayKey),
             static_cast<unsigned long>(record.bubbleCount),
             static_cast<unsigned long>(record.startEpoch),
             static_cast<unsigned long>(record.stopEpoch),
             static_cast<unsigned long>(record.activeSeconds),
             record.flowDurationEstimated ? "true" : "false");
    json += item;
  }
  json += "]}";
  return json;
}
