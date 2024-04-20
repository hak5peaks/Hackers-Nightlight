#pragma once
#include <Arduino.h>
#include "global_structs.h"

// forward declarations
class WebServerComponent;

class SystemManager {
public:
    static SystemManager& getInstance() {
        static SystemManager instance;
        return instance;
    }
    attack_status_t attack_status;
    WebServerComponent* WebServer_;

    void SetupSystem();
    void loop();
};