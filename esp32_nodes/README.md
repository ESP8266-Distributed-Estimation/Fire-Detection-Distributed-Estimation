# ESP32 ESP-NOW Communication

Basic ESP-NOW communication setup for the Distributed Fire Detection System.

## What is ESP-NOW?

ESP-NOW is a connectionless communication protocol developed by Espressif for ESP32/ESP8266. It enables fast, low-power data exchange between devices without requiring a WiFi router.

## Project Structure

```
esp32_nodes/
├── end_node_sender/
│   └── end_node_sender.ino    # Sends data to receiver
└── end_node_receiver/
    └── end_node_receiver.ino  # Receives data from sender
```

## Hardware Requirements

- 2x ESP32 development boards
- 2x USB cables for programming
- Arduino IDE or PlatformIO

## Setup Instructions

### Step 1: Install Arduino IDE
1. Download and install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Go to File > Preferences
   - Add to "Additional Board Manager URLs": 
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools > Board > Boards Manager
   - Search for "ESP32" and install "esp32 by Espressif Systems"

### Step 2: Upload Receiver Code
1. Connect the first ESP32 to your computer
2. Open `end_node_receiver/end_node_receiver.ino`
3. Select the correct board and port:
   - Tools > Board > ESP32 Arduino > ESP32 Dev Module
   - Tools > Port > (Select your ESP32's port)
4. Click Upload
5. Open Serial Monitor (115200 baud)
6. **IMPORTANT:** Note the MAC address displayed (e.g., `24:6F:28:XX:XX:XX`)

### Step 3: Configure and Upload Sender Code
1. Disconnect the receiver ESP32
2. Connect the second ESP32
3. Open `end_node_sender/end_node_sender.ino`
4. Replace the MAC address in line 17:
   ```cpp
   uint8_t receiverAddress[] = {0x24, 0x6F, 0x28, 0xXX, 0xXX, 0xXX};
   ```
   Convert the receiver's MAC address to this format (with `0x` prefix)
5. Select board and port
6. Click Upload

### Step 4: Test Communication
1. Keep sender ESP32 connected to your computer
2. Open Serial Monitor (115200 baud) for the sender
3. Connect receiver ESP32 to power (USB or battery)
4. You should see:
   - Sender: "Sent with success" messages every 2 seconds
   - Receiver: Incoming data with temperature, gas levels, etc.

## Data Structure

Both sender and receiver use this structure:

```cpp
typedef struct struct_message {
  int nodeId;          // Unique node identifier
  float temperature;   // Temperature reading in °C
  float gasLevel;      // Gas sensor reading in ppm
  float estimate;      // Kalman filter estimate
  int originatorId;    // ID of node that originated the alert
} struct_message;
```

## Troubleshooting

### "Error initializing ESP-NOW"
- Make sure WiFi mode is set to WIFI_STA
- Try resetting the ESP32

### "Failed to add peer"
- Verify the MAC address is correct
- Check that the address format matches (6 bytes with 0x prefix)

### "Delivery Fail"
- Check that receiver ESP32 is powered on
- Ensure both ESP32s are within range (typically 100-200m line of sight)
- Verify MAC address is correct

### No data received
- Upload receiver code first and note its MAC address
- Make sure sender has the correct receiver MAC address
- Both devices should be on the same WiFi channel (channel 0 auto-selects)

## Next Steps

This basic example demonstrates one-way communication. For the mesh network, you'll need to:
1. Implement bidirectional communication
2. Add neighbor discovery and management
3. Implement Kalman filtering
4. Add consensus/diffusion algorithms
5. Implement sleep modes for battery-powered nodes

## Resources

- [ESP-NOW Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
