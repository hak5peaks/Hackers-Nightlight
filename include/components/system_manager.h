#pragma once
#include <Arduino.h>

// forward declarations


class SystemManager {
public:
    static SystemManager& getInstance() {
        static SystemManager instance;
        return instance;
    }

    void SetupSystem();
    void loop();
};