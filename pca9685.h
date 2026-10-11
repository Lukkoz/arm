#pragma once
#include <Arduino.h>

// Minimal driver for the PCA9685 16-channel PWM controller (I2C).
// Wire must be started before begin().
class Pca9685 {
public:
  // Returns false when no device answers at `address`.
  bool begin(uint8_t address, uint32_t oscillatorHz, uint16_t frequencyHz);
  void writeMicroseconds(uint8_t channel, float microseconds);
  void off(uint8_t channel);   // no pulses: the servo goes limp
  void allOff();

private:
  void write8(uint8_t reg, uint8_t value);
  void setPwm(uint8_t channel, uint16_t on, uint16_t off);

  uint8_t address_ = 0x40;
  float usPerTick_ = 4.88f;
};
