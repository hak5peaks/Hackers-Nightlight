#include "components/webserver/webserver.h"
#include "components/webserver/page_index.h"

#include "components/wifi_controller/wifi_controller.h"
#include "main/attack.h"
#include "components/pcap_serializer/pcap_serializer.h"
#include "components/hccapx_serializer/hccapx_serializer.h"
#include "components/system_manager.h"
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <main/cJSON.h>
#include <drivers/wyze/bp575.h>

#define RED_PIN 6
#define GREEN_PIN 7
#define BLUE_PIN 5
#define WARM_PIN 3
#define COLD_PIN 4

#define MAX_PWM_VALUE 1024

ESP_EVENT_DEFINE_BASE(WEBSERVER_EVENTS);

esp_err_t WebServer::uri_root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char *)HTML_CONTENT, HTML_CONTENT_SIZE);
}

esp_err_t WebServer::uri_reset_head_handler(httpd_req_t *req) {
    esp_event_post(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_RESET, NULL, 0, portMAX_DELAY);
    return httpd_resp_send(req, NULL, 0);
}

esp_err_t WebServer::uri_ap_list_get_handler(httpd_req_t *req) {
    SystemManager::getInstance().wificontrollerInterface->wifictl_scan_nearby_aps();

    const wifictl_ap_records_t *ap_records;
    ap_records = SystemManager::getInstance().wificontrollerInterface->wifictl_get_ap_records();

    // 33 SSID + 6 BSSID + 1 RSSI
    char resp_chunk[40];

    httpd_resp_set_type(req, HTTPD_TYPE_OCTET);
    for(unsigned i = 0; i < ap_records->count; i++){
        memcpy(resp_chunk, ap_records->records[i].ssid, 33);
        memcpy(&resp_chunk[33], ap_records->records[i].bssid, 6);
        memcpy(&resp_chunk[39], &ap_records->records[i].rssi, 1);
        httpd_resp_send_chunk(req, resp_chunk, 40);
    }
    return httpd_resp_send_chunk(req, resp_chunk, 0);
}

esp_err_t WebServer::uri_run_attack_post_handler(httpd_req_t *req) {
    attack_request_t attack_request;
    httpd_req_recv(req, (char *)&attack_request, sizeof(attack_request_t));
    esp_err_t res = httpd_resp_send(req, NULL, 0);
    esp_event_post(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &attack_request, sizeof(attack_request_t), portMAX_DELAY);
    return res;
}

esp_err_t WebServer::uri_status_get_handler(httpd_req_t *req) {
    Serial.println("Fetching attack status...");
    const attack_status_t *attack_status;
    attack_status = attack_get_status();

    httpd_resp_set_type(req, HTTPD_TYPE_OCTET);
    // first send attack result header
    httpd_resp_send_chunk(req, (char *) attack_status, 4);
    // send attack result content
    if(((attack_status->state == FINISHED) || (attack_status->state == TIMEOUT)) && (attack_status->content_size > 0)){
        ESP_ERROR_CHECK(httpd_resp_send_chunk(req, attack_status->content, attack_status->content_size));
    }
    return httpd_resp_send_chunk(req, NULL, 0);
}

esp_err_t WebServer::uri_capture_pcap_get_handler(httpd_req_t *req){
   Serial.println("Providing PCAP file...");
    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    return httpd_resp_send(req, (char *) SystemManager::getInstance().pcapInterface->pcap_serializer_get_buffer(), SystemManager::getInstance().pcapInterface->pcap_serializer_get_size());
}

esp_err_t WebServer::uri_capture_hccapx_get_handler(httpd_req_t *req){
    Serial.println("Providing HCCAPX file...");
    ESP_ERROR_CHECK(httpd_resp_set_type(req, HTTPD_TYPE_OCTET));
    return httpd_resp_send(req, (char *) SystemManager::getInstance().hccapxInterface->hccapx_serializer_get(), sizeof(hccapx_t));
}

esp_err_t WebServer::uri_gpio_led_handler(httpd_req_t *req) {
    char buf[1024];
    int ret, remaining = req->content_len;

    // Receive the request body
    while (remaining > 0) {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }

    // Parse JSON
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
        Serial.println("Failed to parse JSON.");
        return ESP_FAIL;
    }

    // Parse brightness parameter
    cJSON *brightness_json = cJSON_GetObjectItemCaseSensitive(root, "brightness");
    if (!cJSON_IsString(brightness_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid brightness parameter");
        Serial.println("Missing or invalid brightness parameter.");
        return ESP_FAIL;
    }
    int brightness = constrain(String(brightness_json->valuestring).toInt(), 0, 1024);

    // Check if warm value is provided
    cJSON *warm_json = cJSON_GetObjectItemCaseSensitive(root, "warm");
    if (cJSON_IsString(warm_json)) {
        int warm = String(warm_json->valuestring).toInt();
        if (warm > 0) {
            // Scale warm value
            warm = constrain(warm, 0, 1024);
            int warm_pin_value = warm;

            if (SystemManager::getInstance().BulbType == Vont)
            {
                ledcWrite(0, 0);
                ledcWrite(1, 0);
                ledcWrite(2, 0);

                // Set warm pins
                Serial.println("Setting warm light intensity");
                ledcWrite(3, warm_pin_value);
                ledcWrite(4, brightness);  // Make sure cold light is off
            }
            else if (SystemManager::getInstance().BulbType == Wyze)
            {
                SystemManager::getInstance().WyzeDriver->set_channel_value(1, 0);
                SystemManager::getInstance().WyzeDriver->set_channel_value(2, 0);
                SystemManager::getInstance().WyzeDriver->set_channel_value(3, 0);


                Serial.println("Setting warm light intensity");
                SystemManager::getInstance().WyzeDriver->set_channel_value(3, warm_pin_value);

                SystemManager::getInstance().WyzeDriver->update();
            }

           
            cJSON_Delete(root); // Cleanup cJSON
            httpd_resp_send(req, NULL, 0); // Send response
            return ESP_OK; // Return success
        }
    }

    if (SystemManager::getInstance().BulbType == Vont)
    {
        ledcWrite(3, 0);
        ledcWrite(4, 0);
    }
    else if (SystemManager::getInstance().BulbType == Wyze)
    {
        SystemManager::getInstance().WyzeDriver->set_channel_value(4, 0);
        SystemManager::getInstance().WyzeDriver->update();
    }

    // If warm light value is not provided or is 0, set RGB values
    cJSON *rgb_json = cJSON_GetObjectItemCaseSensitive(root, "rgb");
    if (!cJSON_IsObject(rgb_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid RGB parameters");
        Serial.println("Missing or invalid RGB parameters.");
        return ESP_FAIL;
    }

    // Get individual RGB values
    int red = cJSON_GetObjectItemCaseSensitive(rgb_json, "r")->valueint;
    int green = cJSON_GetObjectItemCaseSensitive(rgb_json, "g")->valueint;
    int blue = cJSON_GetObjectItemCaseSensitive(rgb_json, "b")->valueint;

    // Scale RGB values to match the range [0, 1024]
    red = map(red, 0, 255, 0, brightness);
    green = map(green, 0, 255, 0, brightness);
    blue = map(blue, 0, 255, 0, brightness);


    if (SystemManager::getInstance().BulbType == Vont)
    {
        // Set RGB values using ledcWrite
        Serial.println("Setting RGB values");
        ledcWrite(0, blue);
        ledcWrite(1, red);
        ledcWrite(2, green);
    }
    else if (SystemManager::getInstance().BulbType == Wyze)
    {
        Serial.println("Setting RGB values");
        SystemManager::getInstance().WyzeDriver->set_channel_value(1, blue);
        SystemManager::getInstance().WyzeDriver->set_channel_value(2, green);
        SystemManager::getInstance().WyzeDriver->set_channel_value(3, red);
        SystemManager::getInstance().WyzeDriver->update();
    }

    // Cleanup cJSON
    cJSON_Delete(root);

    // Send response
    esp_err_t err = httpd_resp_send(req, NULL, 0);
    if (err != ESP_OK) {
        Serial.print("Failed to send response: ");
        Serial.println(err);
    }
    return err;
}

esp_err_t WebServer::uri_update_firmware_handler(httpd_req_t *req) {
    char buf[1024];
    int ret, remaining = req->content_len;

    // Receive the request body
    while (remaining > 0) {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }

    // Parse JSON
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
        return ESP_FAIL;
    }

    // Extract SSID and password from JSON
    cJSON *ssid_json = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    cJSON *password_json = cJSON_GetObjectItemCaseSensitive(root, "password");
    if (!cJSON_IsString(ssid_json) || !cJSON_IsString(password_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid parameters");
        return ESP_FAIL;
    }
    const char *ssid = ssid_json->valuestring;
    const char *password = password_json->valuestring;

    // Connect to WiFi
    unsigned long startTime = millis();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    // Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi!");
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to connect to WiFi");
        return ESP_FAIL;
    }

    Serial.println("Connected to WiFi!");

    // Firmware update
    HTTPClient http;
    String firmwareUrl = "http://ec2-3-94-161-126.compute-1.amazonaws.com/bins/firmware.bin";
    http.begin(firmwareUrl);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        Serial.println("Starting firmware update...");
        Update.begin();
        size_t size = http.getSize();
        if (Update.writeStream(http.getStream()) == size && Update.end()) {
            Serial.println("Firmware update successful!");
        } else {
            Serial.println("Firmware update failed!");
        }
    } else {
        Serial.println("Firmware download failed!");
    }
    http.end();

    // Disconnect from WiFi
    WiFi.disconnect();

    // Cleanup cJSON
    cJSON_Delete(root);

    // Send response
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

void WebServer::webserver_run(){
    Serial.println("Running webserver");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    config.max_uri_handlers = 10;

    httpd_start(&server, &config);
    httpd_register_uri_handler(server, &uri_root_get);
    httpd_register_uri_handler(server, &uri_reset_head);
    httpd_register_uri_handler(server, &uri_ap_list_get);
    httpd_register_uri_handler(server, &uri_run_attack_post);
    httpd_register_uri_handler(server, &uri_status_get);
    httpd_register_uri_handler(server, &uri_capture_pcap_get);
    httpd_register_uri_handler(server, &uri_capture_hccapx_get);
    httpd_register_uri_handler(server, &uri_gpio_led_get);
    httpd_register_uri_handler(server, &uri_update_post);
}
