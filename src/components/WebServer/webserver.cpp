#include "components/WebServer/webserver.h"

void WebServerComponent::begin()
{
    WiFi.softAP(ssid, password);

    ip = WiFi.softAPIP();
    Serial.print("Access Point IP address: ");
    Serial.println(ip);

    server.on("/", HTTP_GET, std::bind(&WebServerComponent::handleRoot, this));
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
 
void WebServerComponent::HandleStatus()
{
    Serial.println("Fetching attack status...");
    const attack_status_t* attack_status;
    attack_status = attack_get_status();
}

void WebServerComponent::handleRequests()
{
    server.handleClient();
}