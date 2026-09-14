#include <Arduino.h>
#include <math.h>
#include <components/system_manager.h>
#include <components/wsl_bypasser/wsl_bypasser.h>
#include <components/frame_analyzer/frame_analyzer_parser.h>
#include <components/hccapx_serializer/hccapx_serializer.h>
#include <components/pcap_serializer/pcap_serializer.h>
#include <components/WebServer/webserver.h>
#include <components/wifi_controller/wifi_controller.h>
#include <WiFi.h>
#include "main/attack.h"

#if BOARD_LED_DRIVER == LED_DRIVER_BUS

// BP5758/BP6758-family LED driver chip, bit-banged over a 2-wire bus. This protocol
// (register layout, timing) is shared across every board that uses this chip family -
// only the pins (board_config.h) and the red/blue swap differ per board.

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

static void set_channel(uint8_t data[], BP5758_COLOR_IDX byte_idx, uint8_t brightness) {
    uint16_t word = brightness * 4;  // Scale 0-255 to 0-1023 (10 bits)
    if (word == 0 && brightness > 0) word++;  // Ensure 1% is still on
    data[byte_idx * 2 + 7] = word & 0b11111;
    data[byte_idx * 2 + 8] = word >> 5;
}

class MyCustomLightOutput {
protected:
    uint8_t sda;
    uint8_t scl;

public:
    MyCustomLightOutput(uint8_t sda_, uint8_t scl_) {
        this->sda = sda_;
        this->scl = scl_;
    }

    void setup(uint8_t red_brightness, uint8_t green_brightness, uint8_t blue_brightness, uint8_t warm_white_brightness, uint8_t cold_white_brightness) {
        pinMode(this->sda, OUTPUT);
        pinMode(this->scl, OUTPUT);
        end_i2c();
        initializeChannels(red_brightness, green_brightness, blue_brightness, warm_white_brightness, cold_white_brightness);
    }

    void initializeChannels(uint8_t red_brightness, uint8_t green_brightness, uint8_t blue_brightness, uint8_t warm_white_brightness, uint8_t cold_white_brightness) {
        uint8_t data[17] = {BP5758_ADDRESS::OUTPUT_1_TO_5_ENABLEMENT, 0b00011111, 0b00010000, 0b00010000, 0b00010000, 0b00011010, 0b00011010};
        set_channel(data, BP5758_COLOR_IDX::RED, red_brightness);
        set_channel(data, BP5758_COLOR_IDX::GREEN, green_brightness);
        set_channel(data, BP5758_COLOR_IDX::BLUE, blue_brightness);
        set_channel(data, BP5758_COLOR_IDX::COLD_WHITE, cold_white_brightness);
        set_channel(data, BP5758_COLOR_IDX::WARM_WHITE, warm_white_brightness);
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

static MyCustomLightOutput led_bus(BOARD_LED_BUS_SDA_PIN, BOARD_LED_BUS_SCL_PIN);

// Current LED state, since every bus write re-sends a full R/G/B/warm-white/cold-white frame.
static uint8_t current_red = 0;
static uint8_t current_green = 0;
static uint8_t current_blue = 0;
static uint8_t current_white = 0;

static void push_led_state() {
    // cold_white_brightness pinned at 0: every board using this driver so far has only a
    // single confirmed white channel - only the warm-white grayscale channel is used.
    //
    // BOARD_LED_BUS_{R,G,B}_SLOT_SOURCE (0=red,1=green,2=blue) map this board's physical
    // R/G/B bus wiring (board.json's "channel_order", e.g. OREIN's confirmed "BGR") onto
    // the logical current_red/green/blue values - a data-driven stand-in for what used to
    // be a single-permutation #if/#else swap, so any future board's wiring just needs a
    // different channel_order string, not a new code branch.
    uint8_t channel_values[3] = {current_red, current_green, current_blue};
    led_bus.initializeChannels(
        channel_values[BOARD_LED_BUS_R_SLOT_SOURCE],
        channel_values[BOARD_LED_BUS_G_SLOT_SOURCE],
        channel_values[BOARD_LED_BUS_B_SLOT_SOURCE],
        current_white, 0);
}

void SystemManager::setRgb(uint8_t red, uint8_t green, uint8_t blue) {
    current_red = red;
    current_green = green;
    current_blue = blue;
    push_led_state();
}

void SystemManager::setWhite(uint8_t white) {
    current_white = white;
    push_led_state();
}

#if BOARD_ANIMATIONS_ENABLED

static SystemManager::Animation current_animation = SystemManager::ANIMATION_NONE;
static unsigned long animation_started_at = 0;

void SystemManager::setAnimation(Animation animation) {
    current_animation = animation;
    animation_started_at = millis();
}

// Hue (0-359) + fixed full saturation/value -> RGB. Only the rainbow animation needs this.
static void hue_to_rgb(uint16_t hue, uint8_t &r, uint8_t &g, uint8_t &b) {
    uint8_t region = hue / 60;
    uint8_t rise = ((hue % 60) * 255) / 60;
    uint8_t fall = 255 - rise;
    switch (region) {
        case 0: r = 255;  g = rise;  b = 0;    break;
        case 1: r = fall; g = 255;   b = 0;    break;
        case 2: r = 0;    g = 255;   b = rise; break;
        case 3: r = 0;    g = fall;  b = 255;  break;
        case 4: r = rise; g = 0;     b = 255;  break;
        default: r = 255; g = 0;     b = fall; break;
    }
}

void SystemManager::tickAnimation() {
    if (current_animation == ANIMATION_NONE) return;
    unsigned long elapsed = millis() - animation_started_at;

    switch (current_animation) {
        case ANIMATION_RAINBOW: {
            uint16_t hue = (elapsed / 20) % 360; // one full cycle every ~7.2s
            hue_to_rgb(hue, current_red, current_green, current_blue);
            current_white = 0;
            break;
        }
        case ANIMATION_PULSE: {
            // Breathing white, ~4s per breath.
            float phase = (float)(elapsed % 4000) / 4000.0f * 2 * PI;
            current_red = current_green = current_blue = 0;
            current_white = (uint8_t)((sinf(phase - PI / 2) + 1.0f) / 2.0f * 255);
            break;
        }
        case ANIMATION_FLICKER: {
            // Candle-style flicker: re-roll a random brightness every 50-150ms, no smoothing -
            // the irregularity is the point.
            static unsigned long next_flicker_at = 0;
            static uint8_t flicker_brightness = 200;
            if (millis() >= next_flicker_at) {
                flicker_brightness = 140 + random(0, 116); // 140-255
                next_flicker_at = millis() + 50 + random(0, 100);
            }
            current_red = current_green = current_blue = 0;
            current_white = flicker_brightness;
            break;
        }
        default:
            return;
    }
    push_led_state();
}

#endif // BOARD_ANIMATIONS_ENABLED

#endif // LED_DRIVER_BUS

void SystemManager::SetupSystem() {
#if BOARD_LED_DRIVER == LED_DRIVER_BUS
    uint8_t boot_white = BOARD_LED_BOOT_WHITE_BRIGHTNESS;
    led_bus.setup(0, 0, 0, boot_white, 0);
    current_red = 0;
    current_green = 0;
    current_blue = 0;
    current_white = boot_white;

#elif BOARD_LED_DRIVER == LED_DRIVER_PWM_DIRECT
    // Native ESP32 ledc PWM directly on 5 GPIOs, no external LED driver chip.
    ledcSetup(0, 5000, 10);
    ledcSetup(1, 5000, 10);
    ledcSetup(2, 5000, 10);
    ledcSetup(3, 5000, 10);
    ledcSetup(4, 5000, 10);
    ledcAttachPin(BOARD_LED_PWM_BLUE_PIN, 0);
    ledcAttachPin(BOARD_LED_PWM_RED_PIN, 1);
    ledcAttachPin(BOARD_LED_PWM_GREEN_PIN, 2);
    ledcAttachPin(BOARD_LED_PWM_WARM_PIN, 3);
    ledcAttachPin(BOARD_LED_PWM_COLD_PIN, 4);
    ledcWrite(3, 500);
    ledcWrite(4, 1000);
#endif
    // LED_DRIVER_NONE (e.g. RAZER, pending teardown): nothing to set up - no pins are
    // known to be safe to drive yet. The rest of the firmware (WiFi attacks, web UI)
    // works fully independent of LED support.

    wslBypasserInterface = new wslbypasser();
    frameAnalyzerInterface = new frame_analyzer();
    hccapxInterface = new hccapx_serializer();
    pcapInterface = new pcap_serializer();
    webinterface = new WebServer();
    wificontrollerInterface = new wifi_controller();

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wificontrollerInterface->wifictl_mgmt_ap_start();
    attack_init();
    webinterface->webserver_run();
}

void SystemManager::loop() {
#if BOARD_ANIMATIONS_ENABLED && BOARD_LED_DRIVER == LED_DRIVER_BUS
    tickAnimation();
#endif
}
