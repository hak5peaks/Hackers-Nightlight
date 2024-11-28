#pragma once
#include <Arduino.h>
#include <stdint.h>
#include "components/frame_analyzer/frame_analyzer_types.h"

typedef struct __attribute__((__packed__)){
    uint32_t signature;
    uint32_t version;
    uint8_t message_pair;
    uint8_t essid_len;
    uint8_t essid[32];
    uint8_t keyver;
    uint8_t keymic[16];
    uint8_t mac_ap[6];
    uint8_t nonce_ap[32];
    uint8_t mac_sta[6];
    uint8_t nonce_sta[32];
    uint16_t eapol_len;
    uint8_t eapol[256];
} hccapx_t;

class hccapx_serializer
{
public:
void hccapx_serializer_init(const uint8_t *ssid, unsigned size);
hccapx_t *hccapx_serializer_get();
void hccapx_serializer_add_frame(data_frame_t *frame);
bool is_array_zero(uint8_t *array, unsigned size);
unsigned message_ap = 0;
unsigned message_sta = 0;
unsigned eapol_source = 0;
int save_eapol(eapol_packet_t *eapol_packet, eapol_key_packet_t *eapol_key_packet);
void ap_message_m1(eapol_key_packet_t *eapol_key_packet);
void ap_message_m3(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet);
void sta_message(data_frame_t *frame, eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet);
void sta_message_m4(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet);
void sta_message_m2(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet);
void ap_message(data_frame_t *frame, eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet);
};
