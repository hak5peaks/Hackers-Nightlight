#include <Arduino.h>
#include <components/system_manager.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Welcome Peaks, Booting Up...");
  SystemManager::getInstance().SetupSystem();
}

void loop() {
  delay(100);
}