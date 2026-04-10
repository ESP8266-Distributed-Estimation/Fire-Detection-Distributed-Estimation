#pragma once
#include <Arduino.h>

namespace SensorManager {
    void init();
    void readData(float &temperature, float &humidity, float &pressure);
}
