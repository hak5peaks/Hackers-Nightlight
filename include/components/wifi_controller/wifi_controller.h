#pragma once

#include <stdint.h>
#include <unistd.h>

#include "esp_wifi_types.h"

typedef struct {
    uint16_t count;
    wifi_ap_record_t records[CONFIG_SCAN_MAX_AP];
} wifictl_ap_records_t;

enum {
    SNIFFER_EVENT_CAPTURED_DATA,
    SNIFFER_EVENT_CAPTURED_MGMT,
    SNIFFER_EVENT_CAPTURED_CTRL
};

ESP_EVENT_DECLARE_BASE(SNIFFER_EVENTS);

class wifi_controller
{
public:
    void string_to_uint8(const char *str, uint8_t *array, size_t array_size);
    void wifictl_ap_start(wifi_config_t *wifi_config);
    void wifictl_ap_stop();
    void wifictl_mgmt_ap_start();
    void wifictl_sta_connect_to_ap(const wifi_ap_record_t *ap_record, const char password[]);
    void wifictl_sta_disconnect();
    void wifictl_set_ap_mac(const uint8_t *mac_ap);
    void wifictl_get_ap_mac(uint8_t *mac_ap);
    void wifictl_restore_ap_mac();
    void wifictl_get_sta_mac(uint8_t *mac_sta);
    void wifictl_set_channel(uint8_t channel);

    // Ap Scanner Stuff


    void wifictl_scan_nearby_aps();
    const wifictl_ap_records_t *wifictl_get_ap_records();
    const wifi_ap_record_t *wifictl_get_ap_record(unsigned index);
    wifictl_ap_records_t ap_records;


    // Sniffer Stuff

    void wifictl_sniffer_filter_frame_types(bool data, bool mgmt, bool ctrl);
    void wifictl_sniffer_start(uint8_t channel);
    void wifictl_sniffer_stop();
    static void frame_handler(void *buf, wifi_promiscuous_pkt_type_t type);
    bool wifi_init = false;
    uint8_t original_mac_ap[6];
    void wifi_init_apsta();
};
