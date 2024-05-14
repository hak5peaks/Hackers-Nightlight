#include <Wire.h>

static const char *const TAG = "bp5758d";

static const uint8_t BP5758D_MODEL_ID = 0b10000000;
static const uint8_t BP5758D_ADDR_START_5CH = 0b00110000;
static const uint8_t BP5758D_ALL_DATA_CHANNEL_ENABLEMENT = 0b00011111;
static const uint8_t BP5758D_DELAY = 2;

class BP5758D {
public:
  void setup(uint8_t data_pin, uint8_t clock_pin);
  void set_channel_value(uint8_t channel, uint16_t value);
  void update();

private:
  uint8_t data_pin_;
  uint8_t clock_pin_;
  uint8_t pwm_amounts_[5] = {0, 0, 0, 0, 0};

  void write_bit_(bool value);
  void write_byte_(uint8_t data);
  void write_buffer_(uint8_t *buffer, uint8_t size);
};