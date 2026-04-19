#include "SensorManager.h"
#include "Config.h"
#include <DHT.h>

namespace SensorManager {
    DHT dht(DHT_PIN, DHT_TYPE);
    bool isInitialized = false;

    void init() {
        Serial.println("Initializing DHT11...");
        dht.begin();
        isInitialized = true;
    }

    void readData(float &temperature, float &humidity, float &pressure) {
        if (isInitialized) {
            float t = dht.readTemperature();
            float h = dht.readHumidity();

            if (isnan(t) || isnan(h)) {
                Serial.println("DHT read failed!");
                temperature = NAN;
                humidity = NAN;
                pressure = NAN;
                return;
            }

            temperature = t;
            humidity = h;
            pressure = 0.0f; // not used
        } else {
            temperature = NAN;
            humidity = NAN;
            pressure = NAN;
        }
    }
}
