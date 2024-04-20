#include <components/system_manager.h>
#include <components/wsl_bypasser/wsl_bypasser.h>
#include <components/frame_analyzer/frame_analyzer_parser.h>
#include <components/hccapx_serializer/hccapx_serializer.h>
#include <components/pcap_serializer/pcap_serializer.h>
#include <components/webserver/webserver.h>
#include <components/wifi_controller/wifi_controller.h>

void SystemManager::SetupSystem()
{
    wslBypasserInterface = new wslbypasser();
    frameAnalyzerInterface = new frame_analyzer();
    hccapxInterface = new hccapx_serializer();
    pcapInterface = new pcap_serializer();
    webinterface = new WebServer();
    wificontrollerInterface = new wifi_controller();

    webinterface->webserver_run();
}

void SystemManager::loop()
{
    // TODO;
}