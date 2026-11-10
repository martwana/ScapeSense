#include "BubbleAcquisition.h"

#include <Arduino.h>

namespace {
constexpr UBaseType_t kAcquisitionTaskPriority = configMAX_PRIORITIES - 2;
constexpr BaseType_t kAcquisitionCore = 0;
constexpr uint32_t kAcquisitionStackDepth = 4096;
constexpr UBaseType_t kCommandQueueLength = 8;
constexpr UBaseType_t kEventQueueLength = 64;
constexpr TickType_t kCommandWaitTicks = pdMS_TO_TICKS(10);
} // namespace

BubbleAcquisition::BubbleAcquisition(BubbleSensor &sensor)
    : sensor_(sensor), pipeline_(BubbleCounterConfig{}), commandQueue_(nullptr),
      eventQueue_(nullptr), metricsMutex_(nullptr), taskHandle_(nullptr),
      metricsSnapshot_(), resetSerial_(0), appliedResetSerial_(0),
      droppedEvents_(0) {}

bool BubbleAcquisition::begin(const BubbleCounterConfig &config,
                              uint32_t initialTotal) {
  pipeline_.configure(config);
  pipeline_.restoreTotal(initialTotal);

  commandQueue_ = xQueueCreate(kCommandQueueLength, sizeof(Command));
  eventQueue_ = xQueueCreate(kEventQueueLength, sizeof(BubbleAcquisitionEvent));
  metricsMutex_ = xSemaphoreCreateMutex();
  if (!commandQueue_ || !eventQueue_ || !metricsMutex_) {
    return false;
  }

  publishMetrics(pipeline_.metrics());
  const BaseType_t created = xTaskCreatePinnedToCore(
      taskMain, "bubble-acq", kAcquisitionStackDepth, this,
      kAcquisitionTaskPriority, &taskHandle_, kAcquisitionCore);
  if (created != pdPASS) {
    taskHandle_ = nullptr;
    return false;
  }
  return true;
}

bool BubbleAcquisition::configure(const BubbleCounterConfig &config) {
  if (!commandQueue_) {
    return false;
  }
  const Command command{CommandType::Configure, config,
                        resetSerial_.load(std::memory_order_relaxed)};
  return xQueueSend(commandQueue_, &command, kCommandWaitTicks) == pdPASS;
}

bool BubbleAcquisition::reset() {
  if (!commandQueue_ || !eventQueue_) {
    return false;
  }
  const uint32_t previousSerial = resetSerial_.load(std::memory_order_relaxed);
  const uint32_t serial = previousSerial + 1;
  resetSerial_.store(serial, std::memory_order_relaxed);
  xQueueReset(eventQueue_);
  const Command command{CommandType::Reset, BubbleCounterConfig{}, serial};
  if (xQueueSend(commandQueue_, &command, kCommandWaitTicks) != pdPASS) {
    resetSerial_.store(previousSerial, std::memory_order_relaxed);
    return false;
  }
  return true;
}

bool BubbleAcquisition::isRunning() const { return taskHandle_ != nullptr; }

bool BubbleAcquisition::takeBubbleEvent(BubbleAcquisitionEvent &event) {
  if (!eventQueue_) {
    return false;
  }
  const uint32_t expectedSerial = resetSerial_.load(std::memory_order_relaxed);
  while (xQueueReceive(eventQueue_, &event, 0) == pdPASS) {
    if (event.resetSerial == expectedSerial) {
      return true;
    }
  }
  return false;
}

BubbleMetrics BubbleAcquisition::metrics() const {
  BubbleMetrics metrics = metricsSnapshot_;
  if (metricsMutex_ &&
      xSemaphoreTake(metricsMutex_, kCommandWaitTicks) == pdPASS) {
    metrics = metricsSnapshot_;
    xSemaphoreGive(metricsMutex_);
  }
  return metrics;
}

uint32_t BubbleAcquisition::droppedEventCount() const {
  return droppedEvents_.load(std::memory_order_relaxed);
}

void BubbleAcquisition::taskMain(void *context) {
  static_cast<BubbleAcquisition *>(context)->run();
}

void BubbleAcquisition::run() {
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    processCommands();
    if (sensor_.update()) {
      const uint32_t nowMs = millis();
      const uint16_t raw = sensor_.lastRaw();
      if (pipeline_.update(raw, nowMs)) {
        publishBubble(nowMs, pipeline_.metrics().totalBubbles);
      }
      publishMetrics(pipeline_.metrics());
    }
    vTaskDelayUntil(&lastWake, 1);
  }
}

void BubbleAcquisition::processCommands() {
  Command command;
  while (xQueueReceive(commandQueue_, &command, 0) == pdPASS) {
    if (command.type == CommandType::Configure) {
      pipeline_.configure(command.config);
    } else {
      pipeline_.reset();
      appliedResetSerial_.store(command.resetSerial, std::memory_order_relaxed);
    }
    publishMetrics(pipeline_.metrics());
  }
}

void BubbleAcquisition::publishMetrics(const BubbleMetrics &metrics) {
  if (xSemaphoreTake(metricsMutex_, kCommandWaitTicks) == pdPASS) {
    metricsSnapshot_ = metrics;
    xSemaphoreGive(metricsMutex_);
  }
}

void BubbleAcquisition::publishBubble(uint32_t timestampMs,
                                      uint32_t totalBubbles) {
  const BubbleAcquisitionEvent event{
      timestampMs, totalBubbles,
      appliedResetSerial_.load(std::memory_order_relaxed)};
  if (xQueueSend(eventQueue_, &event, 0) != pdPASS) {
    droppedEvents_.fetch_add(1, std::memory_order_relaxed);
  }
}
