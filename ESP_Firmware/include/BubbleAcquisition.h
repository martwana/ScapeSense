#pragma once

#include "BubbleCounterConfig.h"
#include "BubblePipeline.h"
#include "BubbleSensor.h"
#include <atomic>
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

struct BubbleAcquisitionEvent {
  uint32_t timestampMs;
  uint32_t totalBubbles;
  uint32_t resetSerial;
};

class BubbleAcquisition {
public:
  explicit BubbleAcquisition(BubbleSensor &sensor);

  bool begin(const BubbleCounterConfig &config, uint32_t initialTotal);
  bool configure(const BubbleCounterConfig &config);
  bool reset();
  bool isRunning() const;

  bool takeBubbleEvent(BubbleAcquisitionEvent &event);
  BubbleMetrics metrics() const;
  uint32_t droppedEventCount() const;

private:
  enum class CommandType { Configure, Reset };

  struct Command {
    CommandType type;
    BubbleCounterConfig config;
    uint32_t resetSerial;
  };

  static void taskMain(void *context);
  void run();
  void processCommands();
  void publishMetrics(const BubbleMetrics &metrics);
  void publishBubble(uint32_t timestampMs, uint32_t totalBubbles);

  BubbleSensor &sensor_;
  BubblePipeline pipeline_;
  QueueHandle_t commandQueue_;
  QueueHandle_t eventQueue_;
  SemaphoreHandle_t metricsMutex_;
  TaskHandle_t taskHandle_;
  BubbleMetrics metricsSnapshot_;
  std::atomic<uint32_t> resetSerial_;
  std::atomic<uint32_t> appliedResetSerial_;
  std::atomic<uint32_t> droppedEvents_;
};
