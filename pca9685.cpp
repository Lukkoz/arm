#include "pca9685.h"
#include <Wire.h>

// Registers
static const uint8_t MODE1 = 0x00;
static const uint8_t MODE2 = 0x01;
static const uint8_t LED0_ON_L = 0x06;
static const uint8_t ALL_LED_OFF_L = 0xFC;
static const uint8_t PRESCALE = 0xFE;

// MODE1 / MODE2 bits
static const uint8_t MODE1_RESTART = 0x80;
static const uint8_t MODE1_AI = 0x20;      // register auto increment
static const uint8_t MODE1_SLEEP = 0x10;
static const uint8_t MODE1_ALLCALL = 0x01; // keep answering at 0x70: clearing it loses a driver used there
static const uint8_t MODE2_OUTDRV = 0x04;  // totem pole outputs

bool Pca9685::begin(uint8_t address, uint32_t oscillatorHz, uint16_t frequencyHz) {
  address_ = address;
  Wire.beginTransmission(address_);
  if (Wire.endTransmission() != 0) return false;

  long prescale = lround((double)oscillatorHz / (4096.0 * frequencyHz)) - 1;
  prescale = constrain(prescale, 3, 255);

  write8(MODE1, MODE1_SLEEP | MODE1_ALLCALL);   // the prescaler can only be set while sleeping
  write8(PRESCALE, (uint8_t)prescale);
  write8(MODE2, MODE2_OUTDRV);
  write8(MODE1, MODE1_AI | MODE1_ALLCALL);      // wake up
  delay(1);                                     // oscillator start-up (500 us)
  write8(MODE1, MODE1_AI | MODE1_RESTART | MODE1_ALLCALL);

  usPerTick_ = (prescale + 1) * 1e6f / oscillatorHz;
  allOff();
  return true;
}

void Pca9685::writeMicroseconds(uint8_t channel, float microseconds) {
  long ticks = lround(microseconds / usPerTick_);
  setPwm(channel, 0, (uint16_t)constrain(ticks, 0L, 4095L));
}

void Pca9685::off(uint8_t channel) {
  setPwm(channel, 0, 4096);                // bit 12 of OFF = output fully off
}

void Pca9685::allOff() {
  Wire.beginTransmission(address_);
  Wire.write(ALL_LED_OFF_L);
  Wire.write(0x00);
  Wire.write(0x10);
  Wire.endTransmission();
}

void Pca9685::write8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address_);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void Pca9685::setPwm(uint8_t channel, uint16_t on, uint16_t off) {
  if (channel > 15) return;
  Wire.beginTransmission(address_);
  Wire.write(LED0_ON_L + 4 * channel);
  Wire.write(on & 0xFF);
  Wire.write(on >> 8);
  Wire.write(off & 0xFF);
  Wire.write(off >> 8);
  Wire.endTransmission();
}
