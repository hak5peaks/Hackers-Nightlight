#include "components/webserver/webserver.h"
#include "components/webserver/page_index.h"

#include "components/wifi_controller/wifi_controller.h"
#include "main/attack.h"
#include "components/pcap_serializer/pcap_serializer.h"
#include "components/hccapx_serializer/hccapx_serializer.h"
#include "components/system_manager.h"

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
}
