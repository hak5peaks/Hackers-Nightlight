#pragma once
#include <Arduino.h>

// forward declarations

class WebServer;
class pcap_serializer;
class hccapx_serializer;
class frame_analyzer;
class wslbypasser;
class wifi_controller;
class BP5758D;


enum LightBulbType
{
    Vont,
    Wyze
};


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
    BP5758D* WyzeDriver;
    LightBulbType BulbType;

    void SetupSystem();
    void loop();
    void SetupLights(LightBulbType Type);
};