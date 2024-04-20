#include <Arduino.h>
#include <components/system_manager.h>

void setup() {
  Serial.println("Welcome Peaks, Booting Up...");
  SystemManager::getInstance().SetupSystem();
}

void loop() {
  // Nothing to do here yet
}