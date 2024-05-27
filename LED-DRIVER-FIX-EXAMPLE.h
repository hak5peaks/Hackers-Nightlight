#include <Arduino.h>

#define KHZ 100
// Define RGB, brightness, and warm light variables max 255
uint8_t red_value = 0;   
uint8_t green_value = 0; 
uint8_t blue_value = 0;  
float brightness = 0.1;    // shit dont work
uint8_t warm_light = 255;  

enum BP5758_COLOR_IDX : uint8_t {
  BLUE = 0,
  GREEN = 1,
  RED = 2,
  WARM_WHITE = 3,
  COLD_WHITE = 4,
};

enum BP5758_ADDRESS : uint8_t {
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

void set_channel(uint8_t data[], BP5758_COLOR_IDX byte_idx, float brightness) {
  uint16_t word = brightness * 1023;
  if (word == 0 && brightness > 0) word++; 
  data[byte_idx * 2 + 7] = word & 0b11111;
  data[byte_idx * 2 + 8] = word >> 5;
}

class MyCustomLightOutput {
 protected:
  uint8_t sda;
  uint8_t scl;
  uint8_t channel;
  uint8_t current;

 public:
  MyCustomLightOutput(uint8_t sda_, uint8_t scl_, uint8_t channel_, uint8_t current_) {
    this->sda = sda_;
    this->scl = scl_;
    this->channel = channel_;
    this->current = current_;
  }

  void setup() {
    pinMode(this->sda, OUTPUT);
    pinMode(this->scl, OUTPUT);
    end_i2c();
    initializeChannels();
  }

  void initializeChannels() {
    uint8_t data[17] = {BP5758_ADDRESS::OUTPUT_1_TO_5_ENABLEMENT, 0b00011111, 0b00010000, 0b00010000, 0b00010000, 0b00011010, 0b00011010};
    
    set_channel(data, BP5758_COLOR_IDX::RED, (red_value / 255.0) * brightness);
    set_channel(data, BP5758_COLOR_IDX::GREEN, (green_value / 255.0) * brightness);
    set_channel(data, BP5758_COLOR_IDX::BLUE, (blue_value / 255.0) * brightness);
    set_channel(data, BP5758_COLOR_IDX::COLD_WHITE, brightness);
    set_channel(data, BP5758_COLOR_IDX::WARM_WHITE, warm_light / 255.0);

    send(data, 17);
  }

 protected:
  void send(uint8_t data[], uint8_t size) {
    start_i2c();
    for (uint8_t i = 0; i < size; i++) {
      uint8_t the_byte = data[i];
      for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
        bool bit = bitRead(the_byte, bit_idx);
        digitalWrite(this->sda, bit);
        wait();
        digitalWrite(this->scl, HIGH);
        wait();
        digitalWrite(this->scl, LOW);
        wait();
      }
      wait_for_ack();
    }
    end_i2c();
  }

  void wait() {
    delayMicroseconds(2);
  }

  void start_i2c() {
    digitalWrite(this->sda, LOW);
    wait();
    digitalWrite(this->scl, LOW);
    wait();
  }

  void end_i2c() {
    digitalWrite(this->scl, HIGH);
    wait();
    digitalWrite(this->sda, HIGH);
    wait();
  }

  void wait_for_ack() {
    pinMode(this->sda, INPUT);
    digitalWrite(this->scl, HIGH);
    wait();
    digitalWrite(this->scl, LOW);
    wait();
    pinMode(this->sda, OUTPUT);
  }
};

MyCustomLightOutput output_blue(19, 18, 1, 15);
MyCustomLightOutput output_green(19, 18, 2, 15);
MyCustomLightOutput output_red(19, 18, 3, 15);
MyCustomLightOutput output_white(19, 18, 4, 15);

void setup() {
  output_blue.setup();
  output_green.setup();
  output_red.setup();
  output_white.setup();
}

void loop() {

}
