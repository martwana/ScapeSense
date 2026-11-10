#include "BubbleSensor.h"

#include <Arduino.h>

namespace {
constexpr uint32_t kAdcResolution = 4095;
constexpr uint32_t kReferenceMillivolts = 3300;
} // namespace

BubbleSensor::BubbleSensor(ConfigManager &config, Sampler &sampler)
    : config_(config), sampler_(sampler), average_(), lastRaw_(0),
      minRaw_(kAdcResolution), maxRaw_(0), initialized_(false),
      ledState_(false) {}

bool BubbleSensor::begin() {
  pinMode(config_.ledPin(), OUTPUT);
  enableLed(true);

  sampler_.setRate(config_.sampleRateHz());
  sampler_.reset(micros());

  initialized_ = true;
  return initialized_;
}

bool BubbleSensor::update() {
  if (!initialized_) {
    return false;
  }

  if (!sampler_.update(micros())) {
    return false;
  }

  lastRaw_ = analogRead(config_.sensorPin());
  average_.add(lastRaw_);

  if (lastRaw_ < minRaw_) {
    minRaw_ = lastRaw_;
  }
  if (lastRaw_ > maxRaw_) {
    maxRaw_ = lastRaw_;
  }

  return true;
}

void BubbleSensor::enableLed(bool on) {
  ledState_ = on;
  digitalWrite(config_.ledPin(), on ? LOW : HIGH);
}

bool BubbleSensor::ledOn() const { return ledState_; }

uint16_t BubbleSensor::lastRaw() const { return lastRaw_; }
uint32_t BubbleSensor::average() const { return average_.average(); }
uint16_t BubbleSensor::minRaw() const { return minRaw_; }
uint16_t BubbleSensor::maxRaw() const { return maxRaw_; }
bool BubbleSensor::isInitialized() const { return initialized_; }

uint32_t BubbleSensor::rawToMillivolts(uint16_t raw) {
  return (static_cast<uint32_t>(raw) * kReferenceMillivolts) / kAdcResolution;
}

void BubbleSensor::printDiagnostics() {
  Serial.print("Raw: ");
  Serial.print(lastRaw_);
  Serial.print(" | mV: ");
  Serial.print(rawToMillivolts(lastRaw_));
  Serial.print(" | Avg: ");
  Serial.print(average());
  Serial.print(" | Min: ");
  Serial.print(minRaw_);
  Serial.print(" | Max: ");
  Serial.print(maxRaw_);
  Serial.print(" | LED: ");
  Serial.println(ledState_ ? "ON" : "OFF");
}
