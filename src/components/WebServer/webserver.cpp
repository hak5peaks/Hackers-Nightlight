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
        return ESP_FAIL;
    }

    // Parse brightness parameter
    cJSON *brightness_json = cJSON_GetObjectItemCaseSensitive(root, "brightness");
    if (!cJSON_IsString(brightness_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid brightness parameter");
        return ESP_FAIL;
    }
    int brightness = String(brightness_json->valuestring).toInt();

    // Parse color parameter
    cJSON *color_json = cJSON_GetObjectItemCaseSensitive(root, "color");
    if (!cJSON_IsString(color_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid color parameter");
        return ESP_FAIL;
    }
    const char *color_str = color_json->valuestring;

    // Extract RGB values from color string
    char red[3], green[3], blue[3];
    strncpy(red, color_str, 2);
    strncpy(green, color_str + 2, 2);
    strncpy(blue, color_str + 4, 2);
    red[2] = green[2] = blue[2] = '\0';
    int r = strtol(red, NULL, 16);
    int g = strtol(green, NULL, 16);
    int b = strtol(blue, NULL, 16);

    // Set GPIO levels based on RGB values
    gpio_set_level(GPIO_NUM_6, r > 0);
    gpio_set_level(GPIO_NUM_7, g > 0);
    gpio_set_level(GPIO_NUM_5, b > 0);

    // Cleanup cJSON
    cJSON_Delete(root);

    // Send response
    return httpd_resp_send(req, NULL, 0);
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
