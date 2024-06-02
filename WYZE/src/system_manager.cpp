#include <Arduino.h>
#include <components/system_manager.h>
#include <components/wsl_bypasser/wsl_bypasser.h>
#include <components/frame_analyzer/frame_analyzer_parser.h>
#include <components/hccapx_serializer/hccapx_serializer.h>
#include <components/pcap_serializer/pcap_serializer.h>
#include <components/webserver/webserver.h>
#include <components/wifi_controller/wifi_controller.h>
#include <WiFi.h>
#include "main/attack.h"

#define KHZ 100
static const char *const TAG = "tuya-globe";

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

void set_channel(uint8_t data[], BP5758_COLOR_IDX byte_idx, uint8_t brightness) {
    uint16_t word = brightness * 4;  // Scale 0-255 to 0-1023 (10 bits)
    if (word == 0 && brightness > 0) word++;  // Ensure 1% is still on
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

// Define instances for each output channel
MyCustomLightOutput output_blue(19, 18, 1, 15);
MyCustomLightOutput output_green(19, 18, 2, 15);
MyCustomLightOutput output_red(19, 18, 3, 15);
MyCustomLightOutput output_cold_white(19, 18, 4, 15);
MyCustomLightOutput output_warm_white(19, 18, 5, 15);

void SystemManager::SetupSystem() {
    // Define brightness for each channel (0 - 255)
    uint8_t red_brightness = 0;
    uint8_t green_brightness = 0;
    uint8_t blue_brightness = 0;
    uint8_t warm_white_brightness = 240;
    uint8_t cold_white_brightness = 0; //this doesnt exist, dont bother setting it 

    // Setup LED channels with defined brightness levels
    output_blue.setup(red_brightness, green_brightness, blue_brightness, warm_white_brightness, cold_white_brightness);
    output_green.setup(red_brightness, green_brightness, blue_brightness, warm_white_brightness, cold_white_brightness);
    output_red.setup(red_brightness, green_brightness, blue_brightness, warm_white_brightness, cold_white_brightness);
    output_cold_white.setup(red_brightness, green_brightness, blue_brightness, warm_white_brightness, cold_white_brightness);
    output_warm_white.setup(red_brightness, green_brightness, blue_brightness, warm_white_brightness, cold_white_brightness);

    // Initialize other components
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
    // TODO;
}