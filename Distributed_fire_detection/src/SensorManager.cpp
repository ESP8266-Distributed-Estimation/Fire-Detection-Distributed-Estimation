#include "SensorManager.h"
#include "Config.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

namespace SensorManager {
    Adafruit_BME280 bme;
    bool isInitialized = false;

    void init() {
        Serial.println("Initializing BME280...");
        
        // Try the default GY-BME280 address
        if (!bme.begin(BME280_I2C_ADDR)) {
            Serial.println("BME280 failed at 0x76. Trying 0x77...");
            // Try alternate address
            if (!bme.begin(0x77)) {
                Serial.println("CRITICAL: BME280 not found! Check SDA(D2) and SCL(D1).");
                isInitialized = false;
            } else {
                Serial.println("BME280 Initialized successfully at 0x77.");
                isInitialized = true;
            }
        } else {
            Serial.println("BME280 Initialized successfully at 0x76.");
            isInitialized = true;
        }

        if (isInitialized) {
            // Set up oversampling and filter initialization
            bme.setSampling(Adafruit_BME280::MODE_FORCED,
                            Adafruit_BME280::SAMPLING_X1, // temperature
                            Adafruit_BME280::SAMPLING_X1, // pressure
                            Adafruit_BME280::SAMPLING_X1, // humidity
                            Adafruit_BME280::FILTER_OFF   );
        }
    }

    void readData(float &temperature, float &humidity, float &pressure) {
        if (isInitialized) {
            // Force the sensor to take a new reading before pulling values
            bme.takeForcedMeasurement();
            
            temperature = bme.readTemperature();
            humidity = bme.readHumidity();
            pressure = bme.readPressure() / 100.0F; // Convert Pa to hPa
            
            // Basic sanity check: if reading fails completely, it often returns exactly 0.0 or stays stuck.
            // If it returns NAN or is somehow disconnected during runtime:
            if (isnan(temperature) || isnan(humidity)) {
                Serial.println("Sensor read failed, re-initializing...");
                init(); // Try to restart the sensor bus
            }
        } else {
            // Return NAN if sensor is missing so mesh doesn't break
            temperature = NAN;
            humidity = NAN;
            pressure = NAN;
        }
    }
}
