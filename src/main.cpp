#include <Arduino.h>
#include <components/system_manager.h>

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
  return 0;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Welcome Peaks, Booting Up...");
  SystemManager::getInstance().SetupSystem();
}

void loop() {
  delay(100);
}