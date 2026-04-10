/*
 * ESP-NOW Sender Node
 * 
 * This ESP32 sends data to another ESP32 using ESP-NOW protocol.
 * ESP-NOW is a connectionless communication protocol for fast, low-power data exchange.
 * 
 * Setup:
 * 1. Replace RECEIVER_MAC_ADDRESS with your receiver's MAC address
 * 2. Upload this code to the sender ESP32
 * 3. Open Serial Monitor at 115200 baud
 */

#include <esp_now.h>
#include <WiFi.h>

// REPLACE WITH YOUR RECEIVER MAC ADDRESS
// To get MAC address, upload the receiver code first and check Serial Monitor
uint8_t receiverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Structure to send data
// Must match the receiver structure
typedef struct struct_message {
  int nodeId;
  float temperature;
  float gasLevel;
  float estimate;
  int originatorId;
} struct_message;

struct_message myData;

// Peer info
esp_now_peer_info_t peerInfo;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Print MAC Address for reference
  Serial.print("Sender MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("ESP-NOW Sender Initialized");
  Serial.println("Sending data every 2 seconds...");
}

void loop() {
  // Simulate sensor readings
  myData.nodeId = 1;
  myData.temperature = random(200, 400) / 10.0;  // 20.0 - 40.0°C
  myData.gasLevel = random(0, 1000) / 10.0;      // 0 - 100 ppm
  myData.estimate = random(0, 100) / 10.0;       // 0.0 - 10.0
  myData.originatorId = 1;
  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
    Serial.print("  Node ID: "); Serial.println(myData.nodeId);
    Serial.print("  Temp: "); Serial.print(myData.temperature); Serial.println("°C");
    Serial.print("  Gas: "); Serial.print(myData.gasLevel); Serial.println(" ppm");
    Serial.print("  Estimate: "); Serial.println(myData.estimate);
    Serial.print("  Originator: "); Serial.println(myData.originatorId);
  } else {
    Serial.println("Error sending the data");
  }
  
  delay(2000);  // Send every 2 seconds
}
