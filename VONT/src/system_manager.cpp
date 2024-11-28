#include <components/system_manager.h>
#include <components/wsl_bypasser/wsl_bypasser.h>
#include <components/frame_analyzer/frame_analyzer_parser.h>
#include <components/hccapx_serializer/hccapx_serializer.h>
#include <components/pcap_serializer/pcap_serializer.h>
#include <components/webserver/webserver.h>
#include <components/wifi_controller/wifi_controller.h>
#include <WiFi.h>
#include "main/attack.h"

void SystemManager::SetupSystem()
{
    ledcSetup(0, 5000, 10);
    ledcSetup(1, 5000, 10);
    ledcSetup(2, 5000, 10);
    ledcSetup(3, 5000, 10);
    ledcSetup(4, 5000, 10);
    ledcAttachPin(5, 0); // Blue
    ledcAttachPin(6, 1); // Red
    ledcAttachPin(7, 2); // Green
    ledcAttachPin(3, 3); // warm
    ledcAttachPin(4, 4); // cold
    ledcWrite(3, 500);
    ledcWrite(4, 1000);
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

void SystemManager::loop()
{
    // TODO;
}
