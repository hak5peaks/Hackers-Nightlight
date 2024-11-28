#pragma once

#include <Arduino.h>
#include "esp_http_server.h"

enum {
    WEBSERVER_EVENT_ATTACK_REQUEST,
    WEBSERVER_EVENT_ATTACK_RESET
};

typedef struct {
    uint8_t ap_record_id;   //< ID of chosen AP. It can be used to access ap_records array from wifi_controller - ap_scanner
    uint8_t type;           //< Chosen type of attack
    uint8_t method;         //< Chosen method of attack
    uint8_t timeout;        //< Attack timeout in seconds
} attack_request_t;

ESP_EVENT_DECLARE_BASE(WEBSERVER_EVENTS);
class WebServer
{
public:
    void webserver_run();
    static esp_err_t uri_root_get_handler(httpd_req_t *req);
    static esp_err_t uri_reset_head_handler(httpd_req_t *req);
    static esp_err_t uri_ap_list_get_handler(httpd_req_t *req);
    static esp_err_t uri_run_attack_post_handler(httpd_req_t *req);
    static esp_err_t uri_status_get_handler(httpd_req_t *req);
    static esp_err_t uri_capture_pcap_get_handler(httpd_req_t *req);
    static esp_err_t uri_capture_hccapx_get_handler(httpd_req_t *req);
    static esp_err_t uri_gpio_led_handler(httpd_req_t *req);
    static esp_err_t uri_update_firmware_handler(httpd_req_t *req);

    httpd_uri_t uri_gpio_led_get = {
    .uri = "/setcolor",
    .method = HTTP_POST,
    .handler = uri_gpio_led_handler,
    .user_ctx = NULL
    };

    httpd_uri_t uri_update_post = {
    .uri = "/update",
    .method = HTTP_POST,
    .handler = uri_update_firmware_handler,
    .user_ctx = NULL
    };


    httpd_uri_t uri_root_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = uri_root_get_handler,
    .user_ctx = NULL
    };

    httpd_uri_t uri_reset_head = {
        .uri = "/reset",
        .method = HTTP_HEAD,
        .handler = uri_reset_head_handler,
        .user_ctx = NULL
    };

    httpd_uri_t uri_ap_list_get = {
    .uri = "/ap-list",
    .method = HTTP_GET,
    .handler = uri_ap_list_get_handler,
    .user_ctx = NULL
    };

    httpd_uri_t uri_run_attack_post = {
        .uri = "/run-attack",
        .method = HTTP_POST,
        .handler = uri_run_attack_post_handler,
        .user_ctx = NULL
    };

    httpd_uri_t uri_status_get = {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = uri_status_get_handler,
    .user_ctx = NULL
    };

    httpd_uri_t uri_capture_pcap_get = {
        .uri = "/capture.pcap",
        .method = HTTP_GET,
        .handler = uri_capture_pcap_get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t uri_capture_hccapx_get = {
        .uri = "/capture.hccapx",
        .method = HTTP_GET,
        .handler = uri_capture_hccapx_get_handler,
        .user_ctx = NULL
    };

};
