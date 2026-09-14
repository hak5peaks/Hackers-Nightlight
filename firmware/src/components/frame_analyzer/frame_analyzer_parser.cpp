#include "components/frame_analyzer/frame_analyzer_parser.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include <components/system_manager.h>
#include <components/wifi_controller/wifi_controller.h>

ESP_EVENT_DEFINE_BASE(FRAME_ANALYZER_EVENTS);

void frame_analyzer::print_raw_frame(const wifi_promiscuous_pkt_t *frame){
    for(unsigned i = 0; i < frame->rx_ctrl.sig_len; i++) {
        printf("%02x", frame->payload[i]);
    }
    printf("\n");
}

void frame_analyzer::print_mac_address(const uint8_t *a){
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
    a[0], a[1], a[2], a[3], a[4], a[5]);
    printf("\n");
}

bool frame_analyzer::is_frame_bssid_matching(wifi_promiscuous_pkt_t *frame, uint8_t *bssid) {
    data_frame_mac_header_t *mac_header = (data_frame_mac_header_t *) frame->payload;
    return memcmp(mac_header->addr3, bssid, 6) == 0;
}

eapol_packet_t* frame_analyzer::parse_eapol_packet(data_frame_t *frame)
{
    uint8_t *frame_buffer = frame->body;

    if(frame->mac_header.frame_control.protected_frame == 1) {
        Serial.println("Protected frame, skipping...");
        return NULL;
    }

    if(frame->mac_header.frame_control.subtype > 7) {
        Serial.println("QoS data frame");
        // Skipping QoS field (2 bytes)
        frame_buffer += 2;
    }

    // Skipping LLC SNAP header (6 bytes)
    frame_buffer += sizeof(llc_snap_header_t);

    // Check if frame is type of EAPoL
    if(ntohs(*(uint16_t *) frame_buffer) == ETHER_TYPE_EAPOL) {
        Serial.println("EAPOL packet");
        frame_buffer += 2;
        return (eapol_packet_t *) frame_buffer; 
    }
    return NULL;
}

eapol_key_packet_t* frame_analyzer::parse_eapol_key_packet(eapol_packet_t *eapol_packet){
    if(eapol_packet->header.packet_type != EAPOL_KEY){
        Serial.println("Not an EAPoL-Key packet.");
        return NULL;
    }
    return (eapol_key_packet_t *) eapol_packet->packet_body;
}


static pmkid_item_t* parse_pmkid_from_key_data(uint8_t *key_data, const uint16_t length){
    uint8_t *key_data_index = key_data;
    uint8_t *key_data_max_index = key_data + length;

    pmkid_item_t *pmkid_item_head = NULL;
    key_data_field_t *key_data_field;
    do{
        key_data_field = (key_data_field_t *) key_data_index;

        Serial.printf("EAPOL-Key -> Key-Data -> type=%x; length=%x; oui=%x; data_type=%x",
                    key_data_field->type, 
                    key_data_field->length, 
                    key_data_field->oui,
                    key_data_field->data_type);
        
        if(key_data_field->type != KEY_DATA_TYPE){
            Serial.printf("Wrong type %x (expected %x)", key_data_field->type, KEY_DATA_TYPE);
            continue;
        }

        if(ntohl(key_data_field->oui) != KEY_DATA_OUI_IEEE80211){
            Serial.printf("Wrong OUI %x (expected %x)", key_data_field->oui, KEY_DATA_OUI_IEEE80211);
            continue;
        }

        if(key_data_field->data_type != KEY_DATA_DATA_TYPE_PMKID_KDE){
            Serial.printf("Wrong data type %x (expected %x)", key_data_field->data_type, KEY_DATA_DATA_TYPE_PMKID_KDE);
            continue;
        }

        Serial.println("Found PMKID: ");
        pmkid_item_t *pmkid_item = (pmkid_item_t *) malloc(sizeof(pmkid_item_t));
        pmkid_item->next = pmkid_item_head;
        pmkid_item_head = pmkid_item;
        for(unsigned i = 0; i < 16; i++){
            pmkid_item->pmkid[i] = key_data_field->data[i];
            printf("%02x", pmkid_item->pmkid[i]);
        }
        printf("\n");

    } while((key_data_index = key_data_field->data + key_data_field->length - 4 + 1) < key_data_max_index); 

    return pmkid_item_head;
}

pmkid_item_t* frame_analyzer::parse_pmkid(eapol_key_packet_t *eapol_key){
    if(eapol_key->key_data_length == 0){
        Serial.println("Empty Key Data");
        return NULL;
    }

    if(eapol_key->key_information.encrypted_key_data == 1){
       Serial.println("Key Data encrypted");
        return NULL;
    }

    return parse_pmkid_from_key_data(eapol_key->key_data, ntohs(eapol_key->key_data_length));
}

void frame_analyzer::data_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    Serial.println("Handling DATA frame");
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;

    if(!SystemManager::getInstance().frameAnalyzerInterface->is_frame_bssid_matching(frame, &SystemManager::getInstance().frameAnalyzerInterface->target_bssid)){
        Serial.println("Not matching BSSIDs.");
        return;
    }

    eapol_packet_t *eapol_packet = SystemManager::getInstance().frameAnalyzerInterface->parse_eapol_packet((data_frame_t *) frame->payload);
    if(eapol_packet == NULL){
        Serial.println("Not an EAPOL packet.");
        return;
    }

    eapol_key_packet_t *eapol_key_packet = SystemManager::getInstance().frameAnalyzerInterface->parse_eapol_key_packet(eapol_packet);
    if(eapol_key_packet == NULL){
        Serial.println("Not an EAPOL-Key packet");
        return;
    }

    if(SystemManager::getInstance().frameAnalyzerInterface->SearchType == SEARCH_HANDSHAKE){
        // TODO handle timeouts properly by e.g. for cycle
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_post(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, frame, sizeof(wifi_promiscuous_pkt_t) + frame->rx_ctrl.sig_len, portMAX_DELAY));
        return;
    }

    if(SystemManager::getInstance().frameAnalyzerInterface->SearchType == SEARCH_PMKID){
        pmkid_item_t *pmkid_items;
        if((pmkid_items = SystemManager::getInstance().frameAnalyzerInterface->parse_pmkid(eapol_key_packet)) == NULL){
            return;
        }
        ESP_ERROR_CHECK(esp_event_post(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_PMKID, &pmkid_items, sizeof(pmkid_item_t *), portMAX_DELAY));
        return;
    }
}

void frame_analyzer::capture_start(search_type_t search_type_arg, const uint8_t *bssid){
    Serial.println("Frame analysis started...");
    SearchType = search_type_arg;
    memcpy(&target_bssid, bssid, 6);
    IsCapturing = true;
    esp_event_handler_register(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_DATA, &data_frame_handler, NULL);
}

void frame_analyzer::capture_stop(){
    IsCapturing = false;
    esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &data_frame_handler);
}