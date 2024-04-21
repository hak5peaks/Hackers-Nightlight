#pragma once
#include <Arduino.h>
#include "esp_wifi_types.h"
#include "frame_analyzer_types.h"

enum {
    DATA_FRAME_EVENT_EAPOLKEY_FRAME,
    DATA_FRAME_EVENT_PMKID
};

typedef enum {
    SEARCH_HANDSHAKE,
    SEARCH_PMKID
} search_type_t;

ESP_EVENT_DECLARE_BASE(FRAME_ANALYZER_EVENTS);

class frame_analyzer
{
public:
    static void data_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void capture_start(search_type_t search_type_arg, const uint8_t *bssid);
    void capture_stop();
    void print_raw_frame(const wifi_promiscuous_pkt_t *frame);
    void print_mac_address(const uint8_t *a);
    bool is_frame_bssid_matching(wifi_promiscuous_pkt_t *frame, uint8_t *bssid);
    eapol_packet_t *parse_eapol_packet(data_frame_t *frame);
    eapol_key_packet_t *parse_eapol_key_packet(eapol_packet_t *eapol_packet);
    pmkid_item_t *parse_pmkid(eapol_key_packet_t *eapol_key);
    bool IsCapturing;
    search_type_t SearchType;
    uint8_t target_bssid;
};
