#include "components/hccapx_serializer/hccapx_serializer.h"
#include "esp_err.h"
#include "components/frame_analyzer/frame_analyzer_parser.h"
#include "components/frame_analyzer/frame_analyzer_types.h"
#include <components/system_manager.h>

#define HCCAPX_SIGNATURE 0x58504348
#define HCCAPX_VERSION 4
#define HCCAPX_KEYVER_WPA 1
#define HCCAPX_KEYVER_WPA2 2
#define HCCAPX_MAX_EAPOL_SIZE 256

hccapx_t hccapx = { 
    .signature = HCCAPX_SIGNATURE, 
    .version = 4, 
    .message_pair = 255,
    .keyver = HCCAPX_KEYVER_WPA2
};

bool hccapx_serializer::is_array_zero(uint8_t *array, unsigned size){
    for(unsigned i = 0; i < size; i++){
        if(array[i] != 0){
            return false;
        }
    }
    return true;
}

void hccapx_serializer::hccapx_serializer_init(const uint8_t *ssid, unsigned size){
    hccapx.essid_len = size;
    memcpy(hccapx.essid, ssid, size);
    hccapx.message_pair = 255;
}

hccapx_t* hccapx_serializer::hccapx_serializer_get(){
    if(hccapx.message_pair == 255){
        return NULL;
    }
    
    return &hccapx;
}

int hccapx_serializer::save_eapol(eapol_packet_t *eapol_packet, eapol_key_packet_t *eapol_key_packet)
{
    unsigned eapol_len = 0;
    eapol_len = sizeof(eapol_packet_header_t) + ntohs(eapol_packet->header.packet_body_length);
    if(eapol_len > HCCAPX_MAX_EAPOL_SIZE){
        Serial.printf("EAPoL is too long (%u/%u)", eapol_len, HCCAPX_MAX_EAPOL_SIZE);
        return 1;
    }
    hccapx.eapol_len = eapol_len;
    memcpy(hccapx.eapol, eapol_packet, hccapx.eapol_len);
    memcpy(hccapx.keymic, eapol_key_packet->key_mic, 16);
    memset(&hccapx.eapol[81], 0x0, 16);
    return 0;
}

void hccapx_serializer::ap_message_m1(eapol_key_packet_t *eapol_key_packet){
    Serial.println("From AP M1");
    message_ap = 1;
    memcpy(hccapx.nonce_ap, eapol_key_packet->key_nonce, 32);
}

void hccapx_serializer::ap_message_m3(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet)
{
    Serial.println("From AP M3");
    message_ap = 3;
    if(message_ap == 0){
        // No AP message was processed yet. ANonce has to be copied into HCCAPX buffer.
        memcpy(hccapx.nonce_ap, eapol_key_packet->key_nonce, 32);
    }
    if(eapol_source == 2){
        // EAPoL packet was already saved from message #2. No need to resave it.
        hccapx.message_pair = 2;
        return;
    }
    if(save_eapol(eapol_packet, eapol_key_packet) != 0){
        return;
    }
    eapol_source = 3;
    if(message_sta == 2){
        hccapx.message_pair = 3;
    }
}

void hccapx_serializer::ap_message(data_frame_t *frame, eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    if((!is_array_zero(hccapx.mac_sta, 6)) && (memcmp(frame->mac_header.addr1, hccapx.mac_sta, 6) != 0)){
        Serial.println("Different STA");
        return;
    }
    if(message_ap == 0){
        memcpy(hccapx.mac_ap, frame->mac_header.addr2, 6);
    }
    // Determine which message this is by Key MIC
    // Key MIC is always empty in M1 and always present in M3
    // Ref: 802.11i-2004 [8.5.3]
    if(is_array_zero(eapol_key_packet->key_mic, 16)){
        ap_message_m1(eapol_key_packet);
    } 
    else {
        ap_message_m3(eapol_packet, eapol_key_packet);
    }
}


void hccapx_serializer::sta_message_m2(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    Serial.println("From STA M2");
    message_sta = 2;
    memcpy(hccapx.nonce_sta, eapol_key_packet->key_nonce, 32);
    if(save_eapol(eapol_packet, eapol_key_packet) != 0){
        return;
    }
    eapol_source = 2;
    if(message_ap == 1){
        hccapx.message_pair = 0;
        return;
    }
}


void hccapx_serializer::sta_message_m4(eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    Serial.println("From STA M4");
    if((message_sta == 2) && (eapol_source != 0)){
        // If message 2 was already fully processed, there is no need to process M4 again 
        Serial.println("Already have M2, not worth");
        return;
    }
    if(message_ap == 0){
        // If there was no AP message processed yet, ANonce will be always missing.
        Serial.println("Not enought handshake messages received.");
        return;
    }
    if(eapol_source == 3){
        hccapx.message_pair = 4;
        return;
    }
    if(save_eapol(eapol_packet, eapol_key_packet) != 0){
        return;
    }
    eapol_source = 4;
    if(message_ap == 1){
        hccapx.message_pair = 1;
    }
    if(message_ap == 3){
        hccapx.message_pair = 5;
    }
}


void hccapx_serializer::sta_message(data_frame_t *frame, eapol_packet_t* eapol_packet, eapol_key_packet_t *eapol_key_packet){
    if(is_array_zero(hccapx.mac_sta, 6)){
        memcpy(hccapx.mac_sta, frame->mac_header.addr2, 6);
    }
    else if(memcmp(frame->mac_header.addr2, hccapx.mac_sta, 6) != 0){
        Serial.println("Different STA");
        return;
    }
   
    if(!is_array_zero(eapol_key_packet->key_nonce, 16)){
        sta_message_m2(eapol_packet, eapol_key_packet);
    } 
    else {
        sta_message_m4(eapol_packet, eapol_key_packet);
    }
}


void hccapx_serializer::hccapx_serializer_add_frame(data_frame_t *frame){
    eapol_packet_t *eapol_packet = SystemManager::getInstance().frameAnalyzerInterface->parse_eapol_packet(frame);
    eapol_key_packet_t *eapol_key_packet = SystemManager::getInstance().frameAnalyzerInterface->parse_eapol_key_packet(eapol_packet);
    // Determine direction of the frame by comparing BSSID (addr3) with source address (addr2)
    if(memcmp(frame->mac_header.addr2, frame->mac_header.addr3, 6) == 0){
        ap_message(frame, eapol_packet, eapol_key_packet);
    } 
    else if(memcmp(frame->mac_header.addr1, frame->mac_header.addr3, 6) == 0){
        sta_message(frame, eapol_packet, eapol_key_packet);
    } 
    else {
        Serial.println("Unknown frame format. BSSID is not source nor destionation.");
    }
}