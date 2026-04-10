# Distributed Fire Detection using Edge Kalman Filters & Diffusion Estimation

## 1. Project Overview

This project is a **Lab-Based Project (LBP)** in the **ECE domain**, focused on **distributed estimation and communication-efficient edge intelligence**.

The system implements a **3-Layer Hierarchical Distributed Fire Detection System** using a **True Distributed Mesh** topology at the edge.

---

## 2. Current Implementation Status (Updated)

*   **Phase 1 (Communication):** Complete. Implemented a unified ESP-NOW mesh network using NodeMCU (ESP8266). Nodes can broadcast pings, discover peers, maintain an active neighbor table, and handle timeouts.
*   **Phase 2 (Sensor Integration):** Complete. Integrated GY-BME280 sensor (via I2C) for real-time Temperature, Humidity, and Pressure readings. Handled sensor lockups using Forced Mode and active re-initialization.
*   **Phase 3 (Edge Intelligence):** Pending. Next steps include adding the Scalar Kalman Filter and Diffusion (Consensus) logic across the mesh.

---

## 3. Finalized Architecture

### Layer 1: Central Server (CS)
*   **Device:** Laptop (User's Machine).
*   **Software:** Go (Golang) Backend + Mosquitto MQTT Broker.
*   **Role:**
    *   Global visualization and logging.
    *   Receives high-level "Zone Alerts" from Edge Nodes.
    *   **Does not** participate in the raw estimation loop.

### Layer 2: Edge Nodes (EN)
*   **Device:** NodeMCU ESP8266 (or ESP32, Mains Powered).
*   **Role:** **The Gateway / Sink**.
    *   **Downlink (to Mesh):** Uses **ESP-NOW** to receive alerts/status from the local End Node cluster.
    *   **Uplink (to Server):** Uses **Wi-Fi + MQTT** to forward confirmed events to the Central Server.
    *   **Logic:** Aggregates the "Consensus" result from the mesh.

### Layer 3: End Nodes (The Mesh)
*   **Device:** **NodeMCU ESP8266** *(Currently used for development and mesh testing)*.
*   **Sensor:** **GY-BME280** (Temperature, Humidity, Pressure via I2C).
*   **Connectivity:** **ESP-NOW** (Peer-to-Peer).
*   **Topology:** **Sparse Graph (Mesh).**
    *   Nodes are *not* all connected to the Edge.
    *   Node A connects to Node B. Node B connects to Edge.
    *   Communication is strictly constrained by physical range or a static "Neighbor Table."
*   **Role:**
    *   **Sensing:** BME280 Environmental reading.
    *   **Distributed Estimation:** Runs **Consensus/Diffusion** algorithms (e.g., Average Consensus) by exchanging states with immediate neighbors via ESP-NOW.
    *   **Decision:** The mesh collectively decides if there is a fire.

---

## 4. Communication Protocols

| Link | Protocol | Type | Description |
| :--- | :--- | :--- | :--- |
| **End $\leftrightarrow$ End** | **ESP-NOW** | Peer-to-Peer | Fast, Connectionless. Shares `{estimate, originator_id}`. |
| **End $\to$ Edge** | **ESP-NOW** | Unicast | "Sink" communication. Sends `{gateway_id, originator_id, confidence}`. |
| **Edge $\to$ Server** | **MQTT / Wi-Fi** | TCP/IP | Reliable uplink. Topic: `fire/alert/{originator_id}`. |

---

## 4. Algorithm: Distributed Consensus & Originator Tracking

The End Nodes operate as a **Distributed Graph**.

1.  **Wake Up:** Nodes wake up (synchronized or asynchronous gossip).
2.  **Sense:** Read local sensor $z_k$.
3.  **Filter:** Update local Kalman Estimate $x_{k|k}$.
4.  **Diffuse (The Mesh Step):**
    *   **Exchange:** Send $\{x_i, o_i\}$ to Neighbors $N_i$. Receive $\{x_j, o_j\}$.
    *   **Update State:** $x_i \leftarrow w_{ii}x_i + \sum_{j \in N_i} w_{ij} x_j$.
    *   **Update Originator:** If a neighbor $j$ reports $x_j$ significantly higher than $x_i$, adopt $o_j$ as the new originator $o_i$.
5.  **Check:** If $x_i > Threshold$ AND node has `is_gateway_link == true`, forward alert to Edge Node.
6.  **Sleep:** Return to deep sleep.

### 4.1 Sparse Graph Alerting Logic
*   **Gateways:** Only nodes physically close to the Edge Node are configured as Gateways (`is_gateway_link = true`).
*   **Propagation:** Fire detected by a distant node propagates its ID ($o_i$) and confidence ($x_i$) through the graph via Diffusion.
*   **Edge Deduplication:** The Edge Node implements a **10-second cooldown** per `originator_id` to prevent redundant MQTT alerts.

---

## 6. Development Stack

*   **End Nodes:** C++ / Arduino Framework (NodeMCU ESP8266).
    *   Lib: `espnow.h`, `ESP8266WiFi.h`, `Adafruit BME280`.
*   **Edge Nodes:** C++ / Arduino Framework.
    *   Lib: `espnow.h`, `PubSubClient` (MQTT).
*   **Backend (Server):** Go (Golang).
    *   Lib: `paho.mqtt.golang`.

---

## 7. Project References & Links

*   **IEEE Core Paper:** [MQTT-Based Adaptive Estimation Over Distributed Network Using Raspberry Pi Pico W](https://ieeexplore.ieee.org/document/10704725)
*   **Power Reference:** [ESP32/Pico W Power Consumption Comparison (YouTube)](https://youtu.be/yYFHAEojDTM?si=kKqEwqERP_9eMPVK)
