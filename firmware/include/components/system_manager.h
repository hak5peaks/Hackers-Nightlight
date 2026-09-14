#pragma once
#include <Arduino.h>
#include "generated/board_config.h"

// forward declarations

class WebServer;
class pcap_serializer;
class hccapx_serializer;
class frame_analyzer;
class wslbypasser;
class wifi_controller;


class SystemManager {
public:
    static SystemManager& getInstance() {
        static SystemManager instance;
        return instance;
    }

    WebServer* webinterface;
    pcap_serializer* pcapInterface;
    hccapx_serializer* hccapxInterface;
    frame_analyzer* frameAnalyzerInterface;
    wslbypasser* wslBypasserInterface;
    wifi_controller* wificontrollerInterface;

    void SetupSystem();
    void loop();

#if BOARD_LED_DRIVER == LED_DRIVER_BUS
    // LED bus driver (BP5758/BP6758-family chip over a bit-banged 2-wire bus).
    // Pins, channel_order, and white-channel count all come from board_config.h -
    // see <NAME>/board.json (repo root) for the per-board facts these were generated from.
    void setRgb(uint8_t red, uint8_t green, uint8_t blue);
    void setWhite(uint8_t white);
#if BOARD_ANIMATIONS_ENABLED
    // Fun/spooky LED animations, opt-in per board (see board.json's led.animations.enabled).
    // Manually setting a color (webserver.cpp's /setcolor) always cancels the current one.
    enum Animation : uint8_t { ANIMATION_NONE, ANIMATION_RAINBOW, ANIMATION_PULSE, ANIMATION_FLICKER };
    void setAnimation(Animation animation);
    void tickAnimation(); // called every loop() iteration; no-ops when ANIMATION_NONE
#endif
#endif
};
