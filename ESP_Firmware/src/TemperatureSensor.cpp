#include "TemperatureSensor.h"

TemperatureSensor::TemperatureSensor()
    : oneWire_(nullptr), sensors_(nullptr), conversionStartedMs_(0),
      lastRequestMs_(0), conversionPending_(false) {}

TemperatureSensor::~TemperatureSensor() {
  delete sensors_;
  delete oneWire_;
}

bool TemperatureSensor::begin(uint8_t pin) {
  if (sensors_) {
    return true;
  }
  oneWire_ = new OneWire(pin);
  sensors_ = new DallasTemperature(oneWire_);
  sensors_->begin();
  sensors_->setResolution(12);
  sensors_->setWaitForConversion(false);
  return true;
}

void TemperatureSensor::update(uint32_t nowMs) {
  if (!sensors_) {
    return;
  }
  if (!conversionPending_) {
    if (nowMs - lastRequestMs_ < kSampleIntervalMs) {
      return;
    }
    sensors_->requestTemperatures();
    conversionStartedMs_ = nowMs;
    lastRequestMs_ = nowMs;
    conversionPending_ = true;
    return;
  }
  if (nowMs - conversionStartedMs_ < kConversionMs) {
    return;
  }
  const float temperatureC = sensors_->getTempCByIndex(0);
  reading_ = TemperatureReading(temperatureC,
                                isValidDs18b20Temperature(temperatureC));
  conversionPending_ = false;
}

TemperatureReading TemperatureSensor::reading() const { return reading_; }

bool TemperatureSensor::isInitialized() const { return sensors_ != nullptr; }
