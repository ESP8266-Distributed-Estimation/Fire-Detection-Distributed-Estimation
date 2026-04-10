/*
 * ESP-NOW Receiver Node
 * 
 * This ESP32 receives data from another ESP32 using ESP-NOW protocol.
 * 
 * Setup:
 * 1. Upload this code to the receiver ESP32 first
 * 2. Open Serial Monitor at 115200 baud
 * 3. Note down the MAC address displayed
 * 4. Update the sender code with this MAC address
 */

#include <esp_now.h>
#include <WiFi.h>

// Structure to receive data
// Must match the sender structure
typedef struct struct_message {
  int nodeId;
  float temperature;
  float gasLevel;
  float estimate;
  int originatorId;
} struct_message;

struct_message incomingData;

// Callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  
  Serial.println("\n--- Data Received ---");
  Serial.print("From MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  
  Serial.print("Node ID: "); Serial.println(incomingData.nodeId);
  Serial.print("Temperature: "); Serial.print(incomingData.temperature); Serial.println(" °C");
  Serial.print("Gas Level: "); Serial.print(incomingData.gasLevel); Serial.println(" ppm");
  Serial.print("Estimate: "); Serial.println(incomingData.estimate);
  Serial.print("Originator ID: "); Serial.println(incomingData.originatorId);
  Serial.println("--------------------");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Print MAC Address - IMPORTANT: Copy this to sender code
  Serial.println("\n=================================");
  Serial.println("ESP-NOW Receiver");
  Serial.println("=================================");
  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Copy this address to the sender code!");
  Serial.println("=================================\n");
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register callback function for when data is received
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("ESP-NOW Receiver Ready");
  Serial.println("Waiting for data...\n");
}

void loop() {
  // Nothing to do here, all handled by callback
  delay(100);
}
