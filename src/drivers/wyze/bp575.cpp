#include "drivers/wyze/bp575.h"
#include <Arduino.h>


void BP5758D::setup(uint8_t data_pin, uint8_t clock_pin) {
  Serial.println("Setting up BP5758D Output Component...");
  this->data_pin_ = data_pin;
  this->clock_pin_ = clock_pin;
  pinMode(this->data_pin_, OUTPUT);
  digitalWrite(this->data_pin_, LOW);
  delayMicroseconds(BP5758D_DELAY);
  pinMode(this->clock_pin_, OUTPUT);
  digitalWrite(this->clock_pin_, LOW);
  delayMicroseconds(BP5758D_DELAY);
}

void BP5758D::set_channel_value(uint8_t channel, uint16_t value) {
  this->pwm_amounts_[channel] = value;
}

void BP5758D::update() {
  uint8_t data[17] = {0};
  data[0] = BP5758D_MODEL_ID + BP5758D_ADDR_START_5CH;
  data[1] = BP5758D_ALL_DATA_CHANNEL_ENABLEMENT;
  
  for (int i = 0; i < 5; ++i) {
    data[2 + i] = this->pwm_amounts_[i] ? (this->pwm_amounts_[i] & 0x1F) + 34 : 0;
  }

  this->write_buffer_(data, 17);
}

void BP5758D::write_bit_(bool value) {
  digitalWrite(this->data_pin_, value);
  delayMicroseconds(BP5758D_DELAY);
  digitalWrite(this->clock_pin_, HIGH);
  delayMicroseconds(BP5758D_DELAY);
  digitalWrite(this->clock_pin_, LOW);
  delayMicroseconds(BP5758D_DELAY);
}

void BP5758D::write_byte_(uint8_t data) {
  for (uint8_t mask = 0x80; mask; mask >>= 1) {
    this->write_bit_(data & mask);
  }

  // ack bit
  pinMode(this->data_pin_, INPUT);
  digitalWrite(this->clock_pin_, HIGH);
  delayMicroseconds(BP5758D_DELAY);
  digitalWrite(this->clock_pin_, LOW);
  delayMicroseconds(BP5758D_DELAY);
  pinMode(this->data_pin_, OUTPUT);
}

void BP5758D::write_buffer_(uint8_t *buffer, uint8_t size) {
  digitalWrite(this->data_pin_, LOW);
  delayMicroseconds(BP5758D_DELAY);
  digitalWrite(this->clock_pin_, LOW);
  delayMicroseconds(BP5758D_DELAY);

  for (uint32_t i = 0; i < size; i++) {
    this->write_byte_(buffer[i]);
  }

  digitalWrite(this->clock_pin_, HIGH);
  delayMicroseconds(BP5758D_DELAY);
  digitalWrite(this->data_pin_, HIGH);
  delayMicroseconds(BP5758D_DELAY);
}