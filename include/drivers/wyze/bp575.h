#include <Wire.h>

static const char *const TAG = "bp5758d";

#define KHZ 100
// Define RGB, brightness, and warm light variables max 255
uint8_t red_value = 0;   
uint8_t green_value = 0; 
uint8_t blue_value = 0;  
float brightness = 0.1;
uint8_t warm_light = 255;

enum class BP5758_COLOR_IDX : uint8_t {
  BLUE = 0,
  GREEN = 1,
  RED = 2,
  WARM_WHITE = 3,
  COLD_WHITE = 4,
};

enum class BP5758_ADDRESS : uint8_t {
  OUTPUT_SLEEP = 0b10000110,
  OUTPUT_1_TO_5_ENABLEMENT = 0b10010000,
  OUTPUT_1_RANGE_SETTING = 0b10010001,
  OUTPUT_2_RANGE_SETTING = 0b10010010,
  OUTPUT_3_RANGE_SETTING = 0b10010011,
  OUTPUT_4_RANGE_SETTING = 0b10010100,
  OUTPUT_5_RANGE_SETTING = 0b10010101,
  OUTPUT_1_GRAYSCALE_SETTING = 0b10010110,
  OUTPUT_2_GRAYSCALE_SETTING = 0b10011000,
  OUTPUT_3_GRAYSCALE_SETTING = 0b10011010,
  OUTPUT_4_GRAYSCALE_SETTING = 0b10011100,
  OUTPUT_5_GRAYSCALE_SETTING = 0b10011110,
};

#include <Arduino.h>

class BP5758D {
public:
  void setup(uint8_t data_pin, uint8_t clock_pin);
  void set_channel_value(uint8_t channel, uint16_t value);
  void update();

private:
  uint8_t data_pin_;
  uint8_t clock_pin_;
  uint16_t pwm_amounts_[5] = {0, 0, 0, 0, 0};

  void write_bit_(bool value);
  void write_byte_(uint8_t data);
  void write_buffer_(uint8_t *buffer, uint8_t size);
  void start_i2c_();
  void end_i2c_();
  void wait_();
  void wait_for_ack_();
};

void BP5758D::setup(uint8_t data_pin, uint8_t clock_pin) {
  data_pin_ = data_pin;
  clock_pin_ = clock_pin;
  pinMode(data_pin_, OUTPUT);
  pinMode(clock_pin_, OUTPUT);
  end_i2c_();
  set_channel_value(0, 512); // Set channel 0 (blue) to 50% brightness
  set_channel_value(1, 256); // Set channel 1 (green) to 25% brightness
  set_channel_value(2, 768); // Set channel 2 (red) to 75% brightness
  set_channel_value(3, 1023); // Set channel 3 (cold white) to 100% brightness
  set_channel_value(4, 512); // Set channel 4 (warm white) to 50% brightness
  update();
}

void BP5758D::set_channel_value(uint8_t channel, uint16_t value) {
  if (channel < 5) {
    pwm_amounts_[channel] = value;
  }
}

void BP5758D::update() {
  uint8_t data[17] = {
    0b10010000,  // OUTPUT_1_TO_5_ENABLEMENT
    0b00011111,  // Enable all channels
    0b00010000, 0b00010000, 0b00010000, 0b00011010, 0b00011010, // Range settings
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0 // Placeholder for grayscale settings
  };

  for (uint8_t i = 0; i < 5; ++i) {
    uint16_t value = pwm_amounts_[i];
    data[7 + i * 2] = value & 0b11111;
    data[8 + i * 2] = value >> 5;
  }

  write_buffer_(data, 17);
}

void BP5758D::write_bit_(bool value) {
  digitalWrite(data_pin_, value);
  wait_();
  digitalWrite(clock_pin_, HIGH);
  wait_();
  digitalWrite(clock_pin_, LOW);
  wait_();
}

void BP5758D::write_byte_(uint8_t data) {
  for (int i = 7; i >= 0; --i) {
    write_bit_(bitRead(data, i));
  }
  wait_for_ack_();
}

void BP5758D::write_buffer_(uint8_t *buffer, uint8_t size) {
  start_i2c_();
  for (uint8_t i = 0; i < size; ++i) {
    write_byte_(buffer[i]);
  }
  end_i2c_();
}

void BP5758D::start_i2c_() {
  digitalWrite(data_pin_, LOW);
  wait_();
  digitalWrite(clock_pin_, LOW);
  wait_();
}

void BP5758D::end_i2c_() {
  digitalWrite(clock_pin_, HIGH);
  wait_();
  digitalWrite(data_pin_, HIGH);
  wait_();
}

void BP5758D::wait_() {
  delayMicroseconds(2);
}

void BP5758D::wait_for_ack_() {
  pinMode(data_pin_, INPUT);
  digitalWrite(clock_pin_, HIGH);
  wait_();
  digitalWrite(clock_pin_, LOW);
  wait_();
  pinMode(data_pin_, OUTPUT);
}