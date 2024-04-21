#include "components/webserver/webserver.h"
#include "components/webserver/page_index.h"

#include "components/wifi_controller/wifi_controller.h"
#include "main/attack.h"
#include "components/pcap_serializer/pcap_serializer.h"
#include "components/hccapx_serializer/hccapx_serializer.h"
#include "components/system_manager.h"
#include <HTTPClient.h>
#include <HTTPUpdate.h>

ESP_EVENT_DEFINE_BASE(WEBSERVER_EVENTS);

esp_err_t WebServer::uri_root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char *)page_index, page_index_len);
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

    // Parse brightness parameter
    char *brightness_start = strstr(buf, "\"brightness\":");
    if (brightness_start == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing brightness parameter");
        return ESP_FAIL;
    }
    int brightness;
    if (sscanf(brightness_start + 13, "%d", &brightness) != 1) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid brightness value");
        return ESP_FAIL;
    }

    // Parse color parameter
    char *color_start = strstr(buf, "\"color\":\"");
    if (color_start == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing color parameter");
        return ESP_FAIL;
    }
    char color_str[8]; // Increased size to accommodate '#' prefix if present
    strncpy(color_str, color_start + 9, 7); // Increased length to 7 to include '#' if present
    color_str[7] = '\0';

    // Remove '#' if present
    if (color_str[0] == '#') {
        memmove(color_str, color_str + 1, strlen(color_str)); // Shift characters one place left to remove '#'
    }

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

    // Find the position of "ssid" and "password" in the request body
    char *ssid_start = strstr(buf, "\"ssid\":\"");
    char *password_start = strstr(buf, "\"password\":\"");
    if (ssid_start == NULL || password_start == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing parameters");
        return ESP_FAIL;
    }

    // Extract SSID and password from the request body
    ssid_start += 8; // Move pointer to the beginning of SSID value
    char *ssid_end = strchr(ssid_start, '\"'); // Find the end of SSID value
    if (ssid_end == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    *ssid_end = '\0'; // Null terminate the SSID value
    char ssid[32];
    strncpy(ssid, ssid_start, 32);

    password_start += 12; // Move pointer to the beginning of password value
    char *password_end = strchr(password_start, '\"'); // Find the end of password value
    if (password_end == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    *password_end = '\0'; // Null terminate the password value
    char password[64];
    strncpy(password, password_start, 64);

    // Attempt to connect to WiFi
    unsigned long startTime = millis();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    // Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi!");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to connect to WiFi");
        return ESP_FAIL;
    }

    Serial.println("Connected to WiFi!");


    HTTPClient http;
    String firmwareUrl = "http://ec2-3-94-161-126.compute-1.amazonaws.com/firmware.bin";
    http.begin(firmwareUrl);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        Serial.println("Starting firmware update...");
        Update.begin();
        size_t size = http.getSize();
        if (Update.writeStream(http.getStream()) == size) {
            if (Update.end()) {
                Serial.println("Firmware update successful!");
            } else {
                Serial.println("Firmware update failed (2)!");
            }
        } else {
            Serial.println("Firmware update failed (1)!");
        }
    } else {
        Serial.println("Firmware download failed!");
    }
    http.end();

    // Disconnect from WiFi
    WiFi.disconnect();

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
