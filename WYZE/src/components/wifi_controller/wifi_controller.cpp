#include "components/wifi_controller/wifi_controller.h"
#include <Arduino.h>
#include <esp_wifi.h>

#define CONFIG_MGMT_AP_SSID "Nightlight"
#define CONFIG_MGMT_AP_PASSWORD "Nightlight12345"

ESP_EVENT_DEFINE_BASE(SNIFFER_EVENTS);

void wifi_controller::frame_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    ESP_LOGV(TAG, "Captured frame %d.", (int) type);

    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) buf;

    int32_t event_id;
    switch (type) {
        case WIFI_PKT_DATA:
            event_id = SNIFFER_EVENT_CAPTURED_DATA;
            break;
        case WIFI_PKT_MGMT:
            event_id = SNIFFER_EVENT_CAPTURED_MGMT;
            break;
        case WIFI_PKT_CTRL:
            event_id = SNIFFER_EVENT_CAPTURED_CTRL;
            break;
        default:
            return;
    }

    ESP_ERROR_CHECK(esp_event_post(SNIFFER_EVENTS, event_id, frame, frame->rx_ctrl.sig_len + sizeof(wifi_promiscuous_pkt_t), portMAX_DELAY));
}

void wifi_controller::wifictl_scan_nearby_aps(){
    Serial.println("Scanning nearby APs...");

    ap_records.count = CONFIG_SCAN_MAX_AP;

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE
    };
    
    esp_wifi_scan_start(&scan_config, true);
    esp_wifi_scan_get_ap_records(&ap_records.count, ap_records.records);
    Serial.printf("Found %u APs.\n", ap_records.count);
    Serial.println("Scan done.");
}

const wifictl_ap_records_t * wifi_controller::wifictl_get_ap_records() {
    return &ap_records;
}

const wifi_ap_record_t * wifi_controller::wifictl_get_ap_record(unsigned index) {
    if(index > ap_records.count){
        Serial.printf("Index out of bounds! %u records available, but %u requested", ap_records.count, index);
        return NULL;
    }
    return &ap_records.records[index];
}

static void frame_handler(void *buf, wifi_promiscuous_pkt_type_t type)
{
    Serial.printf("Captured frame %d.", (int) type);

    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) buf;

    int32_t event_id;
    switch (type) {
        case WIFI_PKT_DATA:
            event_id = SNIFFER_EVENT_CAPTURED_DATA;
            break;
        case WIFI_PKT_MGMT:
            event_id = SNIFFER_EVENT_CAPTURED_MGMT;
            break;
        case WIFI_PKT_CTRL:
            event_id = SNIFFER_EVENT_CAPTURED_CTRL;
            break;
        default:
            return;
    }

    esp_event_post(SNIFFER_EVENTS, event_id, frame, frame->rx_ctrl.sig_len + sizeof(wifi_promiscuous_pkt_t), portMAX_DELAY);
}

void wifi_controller::wifictl_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl) {
    wifi_promiscuous_filter_t filter = { .filter_mask = 0 };
    if(data) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
    }
    else if(mgmt) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT;
    }
    else if(ctrl) {
        filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL;
    }
    esp_wifi_set_promiscuous_filter(&filter);
}

void wifi_controller::wifictl_sniffer_start(uint8_t channel) {
    Serial.println("Starting promiscuous mode...");
    // ESP32 cannot switch port, if there is some STA connected to AP
    Serial.println("Kicking all connected STAs from AP");
    ESP_ERROR_CHECK(esp_wifi_deauth_sta(0));
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&frame_handler);
}

void wifi_controller::wifictl_sniffer_stop() {
    Serial.println("Stopping promiscuous mode...");
    esp_wifi_set_promiscuous(false);
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data){

}

void wifi_controller::wifi_init_apsta(){
    esp_netif_init();

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);

    // save original AP MAC address
    esp_wifi_get_mac(WIFI_IF_AP, original_mac_ap);

    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_init = true;
}

void wifi_controller::wifictl_ap_start(wifi_config_t *wifi_config) {
    Serial.println("Starting AP...");
    if(!wifi_init){
        wifi_init_apsta();
    }

    esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_AP, wifi_config);
    Serial.printf("AP started with SSID=%s", wifi_config->ap.ssid);
}

void wifi_controller::wifictl_ap_stop(){
    Serial.println("Stopping AP...");
    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = 0
        },
    };
    esp_wifi_set_config((wifi_interface_t)1, &wifi_config);
    Serial.println("AP stopped");
}

uint8_t* wifi_controller::string_to_uint8(const char *str, size_t array_size)
{
    // Allocate memory for the array
    uint8_t* result = (uint8_t*)malloc(array_size * sizeof(uint8_t));
    if (result == NULL) {
        // Handle memory allocation failure
        return NULL;
    }

    size_t i;
    for (i = 0; i < array_size - 1 && str[i] != '\0'; ++i) {
        result[i] = (uint8_t)str[i];
    }
    // Make sure the array is properly terminated with '\0'
    result[i] = '\0';

    return result;
}

void wifi_controller::wifictl_mgmt_ap_start(){
    wifi_config_t mgmt_wifi_config = {
    .ap = {
        .ssid = {0},
        .password = {0},
        .ssid_len = strlen(CONFIG_MGMT_AP_SSID),
        .authmode = WIFI_AUTH_WPA2_PSK
    }
};

    strncpy((char *)mgmt_wifi_config.ap.ssid, CONFIG_MGMT_AP_SSID, sizeof(mgmt_wifi_config.ap.ssid) - 1);
    strncpy((char *)mgmt_wifi_config.ap.password, CONFIG_MGMT_AP_PASSWORD, sizeof(mgmt_wifi_config.ap.password) - 1);

    mgmt_wifi_config.ap.max_connection = (uint8_t)CONFIG_MGMT_AP_MAX_CONNECTIONS;
    wifictl_ap_start(&mgmt_wifi_config);
}

void wifi_controller::wifictl_sta_connect_to_ap(const wifi_ap_record_t *ap_record, const char password[]){
    Serial.println("Connecting STA to AP...");
    if(!wifi_init){
        wifi_init_apsta();
    }

    wifi_config_t sta_wifi_config = {
        .sta = {
            .scan_method = WIFI_FAST_SCAN,
            .channel = ap_record->primary
        },
    };

    sta_wifi_config.sta.pmf_cfg.required = false;

    mempcpy(sta_wifi_config.sta.ssid, ap_record->ssid, 32);

    if(password != NULL){
        if(strlen(password) >= 64) {
            Serial.println("Password is too long. Max supported length is 64");
            return;
        }
        memcpy(sta_wifi_config.sta.password, password, strlen(password) + 1);
    }

    Serial.printf(".ssid=%s", sta_wifi_config.sta.ssid);

    esp_wifi_set_config((wifi_interface_t)1, &sta_wifi_config);
    esp_wifi_connect();

}

void wifi_controller::wifictl_sta_disconnect(){
    ESP_ERROR_CHECK(esp_wifi_disconnect());
}

void wifi_controller::wifictl_set_ap_mac(const uint8_t *mac_ap){
    Serial.println("Changing AP MAC address...");
    ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, mac_ap));
}

void wifi_controller::wifictl_get_ap_mac(uint8_t *mac_ap){
    esp_wifi_get_mac(WIFI_IF_AP, mac_ap);
}

void wifi_controller::wifictl_restore_ap_mac(){
    Serial.println("Restoring original AP MAC address...");
    esp_wifi_set_mac(WIFI_IF_AP, original_mac_ap);
}

void wifi_controller::wifictl_get_sta_mac(uint8_t *mac_sta){
    esp_wifi_get_mac(WIFI_IF_STA, mac_sta);
}

void wifi_controller::wifictl_set_channel(uint8_t channel){
    if((channel == 0) || (channel >  13)){
        Serial.printf("Channel out of range. Expected value from <1,13> but got %u", channel);
        return;
    }
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

