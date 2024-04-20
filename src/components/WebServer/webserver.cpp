#include "components/WebServer/webserver.h"
#include "components/system_manager.h"

void WebServerComponent::begin()
{
    WiFi.softAP(ssid, password);

    ip = WiFi.softAPIP();
    Serial.print("Access Point IP address: ");
    Serial.println(ip);

    server.on("/", HTTP_GET, std::bind(&WebServerComponent::handleRoot, this));
    server.on("/status", HTTP_GET, std::bind(&WebServerComponent::HandleStatus, this));
    server.on("/capture.pcap", HTTP_GET, std::bind(&WebServerComponent::HandlePcap, this));
    server.on("/capture.hccapx", HTTP_GET, std::bind(&WebServerComponent::HandleHccapx, this));

    server.onNotFound(std::bind(&WebServerComponent::handleNotFound, this));

    server.begin();
}

void WebServerComponent::handleRoot()
{
    server.send(200, "text/html", index_html);
}

void WebServerComponent::handleNotFound() {
    server.send(404, "text/plain", "404: Not Found");
}

void WebServerComponent::HandleAPList()
{
    wifictl_ap_records_t ap_records;

    int16_t foundnetworks = WiFi.scanNetworks();

    char resp_chunk[40];

    for (int i = 0; i < foundnetworks; i++)
    {
        if (i > 10)
        break;

        int32_t rssi = WiFi.RSSI(i);
        memcpy(resp_chunk, WiFi.SSID(i).c_str(), 33);
        memcpy(&resp_chunk[33], WiFi.BSSIDstr(i).c_str(), 6);
        memcpy(&resp_chunk[39], &rssi, 1);
        server.send(200, "application/octet-stream", resp_chunk);
    }

    server.send(200, "application/octet-stream", 0);
}

void WebServerComponent::HandlePcap()
{
    Serial.println("Providing PCAP file...");
    // server.send(200, "application/octet-stream", );  TODO
    server.send(404, "text/plain", "Need to Implement");
}

void WebServerComponent::HandleHccapx()
{
    Serial.println("Providing HCCAPX file...");
    // server.send(200, "application/octet-stream", );  TODO
    server.send(404, "text/plain", "Need to Implement");
}
 
void WebServerComponent::HandleStatus()
{
    Serial.println("Fetching attack status...");
    attack_status_t* attack_status = &SystemManager::getInstance().attack_status;

    if (attack_status)
    {
        Serial.println("About to Send Status");
        server.send(200, "application/octet-stream", reinterpret_cast<const char*>(attack_status));
    }
    else 
    {
        Serial.println("Invalid Status");
    }
}

void WebServerComponent::handleRequests()
{
    server.handleClient();
}