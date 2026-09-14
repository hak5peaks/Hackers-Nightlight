#include "components/WebServer/webserver.h"
#include "components/WebServer/page_index.h"

#include "components/wifi_controller/wifi_controller.h"
#include "main/attack.h"
#include "components/pcap_serializer/pcap_serializer.h"
#include "components/hccapx_serializer/hccapx_serializer.h"
#include "components/system_manager.h"
#include "generated/board_config.h"
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

// /setcolor - the only handler whose actual pin-level behavior varies by board. Which
// branch compiles in is decided entirely by board_config.h (generated from
// <NAME>/board.json), so this file itself never changes per board.
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

#if BOARD_ANIMATIONS_ENABLED
    // Manually setting a color always cancels whatever animation is running.
    SystemManager::getInstance().setAnimation(SystemManager::ANIMATION_NONE);
#endif

    // Parse brightness parameter
    cJSON *brightness_json = cJSON_GetObjectItemCaseSensitive(root, "brightness");
    if (!cJSON_IsString(brightness_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid brightness parameter");
        Serial.println("Missing or invalid brightness parameter.");
        return ESP_FAIL;
    }
    int brightness = constrain(String(brightness_json->valuestring).toInt(), 0, 1024);

#if BOARD_LED_CONTROL_ENABLED && BOARD_LED_DRIVER == LED_DRIVER_BUS
    // Bus channels (BP5758/BP6758-family chip over SDA/SCL) take 0-255; UI sliders send 0-1024.
    uint8_t brightness_255 = map(brightness, 0, 1024, 0, 255);
#endif

    // Check if warm value is provided
    cJSON *warm_json = cJSON_GetObjectItemCaseSensitive(root, "warm");
    if (cJSON_IsString(warm_json)) {
        int warm = String(warm_json->valuestring).toInt();
        if (warm > 0) {
            warm = constrain(warm, 0, 1024);

#if BOARD_LED_DRIVER == LED_DRIVER_PWM_DIRECT
            // Native ledc PWM, two independent white lines. "brightness" doubles as the
            // cold-channel value while in warm mode - this is VONT's original, shipped
            // behavior (not a new design), preserved exactly rather than "cleaned up".
            Serial.println("Setting warm light intensity");
            ledcWrite(0, 0);
            ledcWrite(1, 0);
            ledcWrite(2, 0);
            ledcWrite(3, warm);
            ledcWrite(4, brightness);
#elif BOARD_LED_DRIVER == LED_DRIVER_BUS
#if BOARD_LED_CONTROL_ENABLED
            Serial.println("Setting white intensity");
            SystemManager::getInstance().setWhite(map(warm, 0, 1024, 0, 255));
#else
            // led_control_enabled=false for this board: nobody has wired+verified dynamic
            // control on real hardware yet (see <NAME>/board.json). Preserve the original,
            // harmless no-op behavior (these channels were never ledcSetup/ledcAttachPin'd)
            // rather than silently starting to drive hardware nobody's confirmed against.
            ledcWrite(3, warm);
            ledcWrite(4, brightness);
#endif
#endif

            cJSON_Delete(root); // Cleanup cJSON
            httpd_resp_send(req, NULL, 0); // Send response
            return ESP_OK; // Return success
        }
    }

#if BOARD_LED_DRIVER == LED_DRIVER_PWM_DIRECT
    ledcWrite(3, 0);
    ledcWrite(4, 0);
#elif BOARD_LED_DRIVER == LED_DRIVER_BUS
#if BOARD_LED_CONTROL_ENABLED
    SystemManager::getInstance().setWhite(0);
#else
    ledcWrite(3, 0);
    ledcWrite(4, 0);
#endif
#endif

    // If warm light value is not provided or is 0, set RGB values
    cJSON *rgb_json = cJSON_GetObjectItemCaseSensitive(root, "rgb");
    if (!cJSON_IsObject(rgb_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid RGB parameters");
        Serial.println("Missing or invalid RGB parameters.");
        return ESP_FAIL;
    }

    // Get individual RGB values (null-checked: a malformed POST body used to crash this
    // handler with a null-pointer dereference here - fixed once, for every board).
    cJSON *r_json = cJSON_GetObjectItemCaseSensitive(rgb_json, "r");
    cJSON *g_json = cJSON_GetObjectItemCaseSensitive(rgb_json, "g");
    cJSON *b_json = cJSON_GetObjectItemCaseSensitive(rgb_json, "b");
    if (!cJSON_IsNumber(r_json) || !cJSON_IsNumber(g_json) || !cJSON_IsNumber(b_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid RGB parameters");
        Serial.println("Missing or invalid r/g/b fields.");
        return ESP_FAIL;
    }
    int red = r_json->valueint;
    int green = g_json->valueint;
    int blue = b_json->valueint;

    Serial.println("Setting RGB values");
#if BOARD_LED_DRIVER == LED_DRIVER_PWM_DIRECT
    // Scale to the native 0-1024 ledc range directly, capped by brightness (VONT's
    // original scaling - these channels are native 10-bit PWM, not routed through an
    // 8-bit bus register like the BUS driver below).
    red = map(red, 0, 255, 0, brightness);
    green = map(green, 0, 255, 0, brightness);
    blue = map(blue, 0, 255, 0, brightness);
    ledcWrite(0, blue);
    ledcWrite(1, red);
    ledcWrite(2, green);
#elif BOARD_LED_DRIVER == LED_DRIVER_BUS
#if BOARD_LED_CONTROL_ENABLED
    // Scale to the bus's 0-255 register width, capped by brightness.
    red = map(red, 0, 255, 0, brightness_255);
    green = map(green, 0, 255, 0, brightness_255);
    blue = map(blue, 0, 255, 0, brightness_255);
    SystemManager::getInstance().setRgb(red, green, blue);
#else
    // Preserve the original no-op behavior for boards whose bus control isn't verified yet.
    red = map(red, 0, 255, 0, brightness);
    green = map(green, 0, 255, 0, brightness);
    blue = map(blue, 0, 255, 0, brightness);
    ledcWrite(0, blue);
    ledcWrite(1, red);
    ledcWrite(2, green);
#endif
#endif

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

#if BOARD_ANIMATIONS_ENABLED
// /setanimation - picks a fun/spooky LED animation, or turns it off. system_manager.cpp's
// tickAnimation() (called from the main loop) does the actual per-frame color math.
esp_err_t WebServer::uri_set_animation_handler(httpd_req_t *req) {
    char buf[128];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        ret = httpd_req_recv(req, buf, MIN(remaining, (int)sizeof(buf) - 1));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    buf[req->content_len < sizeof(buf) ? req->content_len : sizeof(buf) - 1] = '\0';

    cJSON *root = cJSON_Parse(buf);
    cJSON *mode_json = root ? cJSON_GetObjectItemCaseSensitive(root, "mode") : NULL;
    if (!cJSON_IsString(mode_json)) {
        if (root) cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid mode parameter");
        return ESP_FAIL;
    }

    const char *mode = mode_json->valuestring;
    SystemManager::Animation animation = SystemManager::ANIMATION_NONE;
    if (strcmp(mode, "rainbow") == 0) animation = SystemManager::ANIMATION_RAINBOW;
    else if (strcmp(mode, "pulse") == 0) animation = SystemManager::ANIMATION_PULSE;
    else if (strcmp(mode, "flicker") == 0) animation = SystemManager::ANIMATION_FLICKER;
    // anything else (including "none") falls back to ANIMATION_NONE - turns it off.

    SystemManager::getInstance().setAnimation(animation);
    cJSON_Delete(root);
    return httpd_resp_send(req, NULL, 0);
}
#endif

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

    if (strlen(BOARD_OTA_FIRMWARE_URL) == 0) {
        Serial.println("OTA firmware URL not configured - set ota_firmware_url in app_config.json or this board's board.json");
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA firmware URL not configured");
        return ESP_FAIL;
    }

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
    String firmwareUrl = BOARD_OTA_FIRMWARE_URL;
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

    config.max_uri_handlers = 12;

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
#if BOARD_ANIMATIONS_ENABLED
    httpd_register_uri_handler(server, &uri_set_animation_post);
#endif
}
