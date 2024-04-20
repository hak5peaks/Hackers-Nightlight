#include <components/system_manager.h>
#include <WiFi.h>
#include <components/WebServer/webserver.h>


void SystemManager::SetupSystem()
{
    WebServer_ = new WebServerComponent("Nightlight", "123456789");
    WebServer_->begin();
}

void SystemManager::loop()
{
    if (WebServer_)
    {
        WebServer_->handleRequests();
    }
}