#include "MeshManager.h"
#include "Config.h"
#include <ESP8266WiFi.h>
#include <espnow.h>

namespace MeshManager {
    Neighbor neighbors[MAX_NEIGHBORS];
    int neighborCount = 0;
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    typedef struct {
        uint32_t sourceNodeId;
        uint32_t maxSeqNum;
        uint32_t lastReceived;
    } AlarmRecord;

    AlarmRecord alarmCache[MAX_ACTIVE_ALARMS];

    void printMac(const uint8_t *mac) {
        for (int i = 0; i < 6; i++) {
            Serial.printf("%02X", mac[i]);
            if (i < 5) Serial.print(":");
        }
    }

    int findNeighbor(const uint8_t *mac) {
        for (int i = 0; i < neighborCount; i++) {
            if (memcmp(neighbors[i].mac, mac, 6) == 0) return i;
        }
        return -1;
    }

    void processIncomingAlarm(uint32_t sourceId, uint32_t seqNum) {
        if (sourceId == 0) return;          // No alarm
        if (sourceId == NODE_ID) return;    // Ignore our own echoed alarms

        int emptyIdx = -1;
        for (int i = 0; i < MAX_ACTIVE_ALARMS; i++) {
            if (alarmCache[i].sourceNodeId == sourceId) {
                // Found existing record for this fire
                if (seqNum > alarmCache[i].maxSeqNum) {
                    alarmCache[i].maxSeqNum = seqNum;
                    alarmCache[i].lastReceived = millis();
                }
                return;
            }
            if (alarmCache[i].sourceNodeId == 0 && emptyIdx == -1) {
                emptyIdx = i;
            }
        }
        
        // Not found, add to empty slot
        if (emptyIdx != -1) {
            alarmCache[emptyIdx].sourceNodeId = sourceId;
            alarmCache[emptyIdx].maxSeqNum = seqNum;
            alarmCache[emptyIdx].lastReceived = millis();
        }
    }

    void updateNeighbor(const uint8_t *mac, const struct_message *data) {
        processIncomingAlarm(data->alarmSourceId, data->alarmSeqNum);

        int idx = findNeighbor(mac);
        if (idx != -1) {
            // Existing neighbor: update their record
            neighbors[idx].lastSeen = millis();
            neighbors[idx].lastSeq = data->seqNum;
            neighbors[idx].nodeId = data->nodeId;
            neighbors[idx].temperature = data->temperature;
            neighbors[idx].tempVariance = data->tempVariance;
            neighbors[idx].dT = data->dT;
            neighbors[idx].dtVariance = data->dtVariance;
        } else if (neighborCount < MAX_NEIGHBORS) {
            // New neighbor: register them
            memcpy(neighbors[neighborCount].mac, mac, 6);
            neighbors[neighborCount].nodeId = data->nodeId;
            neighbors[neighborCount].lastSeen = millis();
            neighbors[neighborCount].lastSeq = data->seqNum;
            neighbors[neighborCount].temperature = data->temperature;
            neighbors[neighborCount].tempVariance = data->tempVariance;
            neighbors[neighborCount].dT = data->dT;
            neighbors[neighborCount].dtVariance = data->dtVariance;
            
            Serial.print("\n[DISCOVERED] Node ID: ");
            Serial.print(data->nodeId);
            Serial.print(" MAC: ");
            printMac(mac);
            Serial.println();
            neighborCount++;
        }
    }

    void cleanStaleNeighbors() {
        uint32_t now = millis();
        for (int i = 0; i < neighborCount; i++) {
            if (now - neighbors[i].lastSeen > NEIGHBOR_TIMEOUT) {
                Serial.printf("\n[TIMEOUT] Node %d removed\n", neighbors[i].nodeId);
                // Shift remaining neighbors up
                for (int j = i; j < neighborCount - 1; j++) {
                    neighbors[j] = neighbors[j + 1];
                }
                neighborCount--;
                i--; // Re-check this index
            }
        }
    }

    void cleanStaleAlarms() {
        uint32_t now = millis();
        for (int i = 0; i < MAX_ACTIVE_ALARMS; i++) {
            if (alarmCache[i].sourceNodeId != 0) {
                if (now - alarmCache[i].lastReceived > ALARM_FORGET_MS) {
                    alarmCache[i].sourceNodeId = 0;
                    alarmCache[i].maxSeqNum = 0;
                }
            }
        }
    }

    // Callbacks must be global/static in ESP8266 ESP-NOW
    void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
        // Serial.printf("\n[RAW RECV] Packet from: %02X:%02X:%02X:%02X:%02X:%02X, Len: %d\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
        if (len != sizeof(struct_message)) {
            // Serial.printf("ERR: Recv len %d != expected %d\n", len, sizeof(struct_message));
            return;
        }
        struct_message recvData;
        memcpy(&recvData, incomingData, sizeof(recvData));
        updateNeighbor(mac, &recvData);
    }

    void OnDataSent(uint8_t *mac_addr, uint8_t status) {
        // Serial.println("Sending status: " + String(status));
        if (status != 0) {
            // Serial.printf("ERR: Send failed, status: %d\n", status);
        }
    }

    void init() {
        if (esp_now_init() != 0) {
            Serial.println("ESP-NOW Init Failed");
            return;
        }
        esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
        esp_now_register_send_cb(OnDataSent);
        esp_now_register_recv_cb(OnDataRecv);
        
        int addStatus = esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
        if (addStatus != 0) {
            Serial.printf("ESP-NOW Add Peer Failed: %d\n", addStatus);
        } else {
            Serial.println("ESP-NOW Mesh Initialized and Peer Added.");
        }
    }

    void broadcast(struct_message &data) {
        int result = esp_now_send(broadcastAddress, (uint8_t *) &data, sizeof(data));
        if (result != 0) {
            // Serial.printf("\n[ERROR] esp_now_send failed: %d\n", result);
        }
    }

    void evaluateConsensus(uint32_t localNodeId, float localTemp, float localVar, float localDt, float localDtVar, float &consensusTemp, float &consensusDt, bool &fireAlarm, uint32_t &outAlarmSourceId, uint32_t &outAlarmSeqNum) {
        float sumTempWeights = 0.0f;
        float sumWeightedTemps = 0.0f;
        float sumDtWeights = 0.0f;
        float sumWeightedDts = 0.0f;
        
        // 1. Include local node in consensus
        float localTempWeight = 1.0f / (localVar + 0.001f);
        sumTempWeights += localTempWeight;
        sumWeightedTemps += localTempWeight * localTemp;
        
        float localDtWeight = 1.0f / (localDtVar + 0.001f);
        sumDtWeights += localDtWeight;
        sumWeightedDts += localDtWeight * localDt;
        
        // 2. Iterate through neighbors
        for (int i = 0; i < neighborCount; i++) {
            float nTemp = neighbors[i].temperature;
            float nVar = neighbors[i].tempVariance;
            float nDt = neighbors[i].dT;
            float nDtVar = neighbors[i].dtVariance;
            
            float nTempWeight = 1.0f / (nVar + 0.001f);
            sumTempWeights += nTempWeight;
            sumWeightedTemps += nTempWeight * nTemp;
            
            float nDtWeight = 1.0f / (nDtVar + 0.001f);
            sumDtWeights += nDtWeight;
            sumWeightedDts += nDtWeight * nDt;
        }
        
        // 3. Finalize Output
        consensusTemp = sumWeightedTemps / sumTempWeights;
        consensusDt = sumWeightedDts / sumDtWeights;
        
        // 4. Alarm Logic (OR Condition)
        bool consensusAbsoluteAlarm = (consensusTemp > CRITICAL_TEMP);
        bool consensusFastRiseAlarm = (consensusDt > CRITICAL_DT) && (consensusTemp > BASELINE_TEMP);
        
        bool localAbsoluteAlarm = (localTemp > CRITICAL_TEMP);
        bool localFastRiseAlarm = (localDt > CRITICAL_DT) && (localTemp > BASELINE_TEMP);
        
        bool localTrigger = consensusAbsoluteAlarm || consensusFastRiseAlarm || localAbsoluteAlarm || localFastRiseAlarm;

        if (localTrigger) {
            fireAlarm = true;
            outAlarmSourceId = localNodeId;
        } else {
            // Check cache for propagated alarms
            fireAlarm = false;
            outAlarmSourceId = 0;
            outAlarmSeqNum = 0;
            for (int i = 0; i < MAX_ACTIVE_ALARMS; i++) {
                if (alarmCache[i].sourceNodeId != 0) {
                    if (millis() - alarmCache[i].lastReceived <= ALARM_TIMEOUT_MS) {
                        fireAlarm = true;
                        outAlarmSourceId = alarmCache[i].sourceNodeId;
                        outAlarmSeqNum = alarmCache[i].maxSeqNum;
                        break; // Just grab the first active one to propagate
                    }
                }
            }
        }
    }

    void printStatus(uint32_t localNodeId, float localTemp, float localDt, float consensusTemp, float consensusDt, bool fireAlarm) {
        uint32_t uptimeSec = millis() / 1000;
        Serial.printf("\n[STATUS] Node: %d | Uptime: %u s | Neighbors: %d\n", localNodeId, uptimeSec, neighborCount);
        Serial.printf("  -> Local Sensor: Temp=%.2f*C | dT=%.3f*C/s\n", localTemp, localDt);
        
        if (fireAlarm) {
            // Determine the primary cause of the alarm for logging
            bool cAbs = (consensusTemp > CRITICAL_TEMP);
            bool cFast = (consensusDt > CRITICAL_DT) && (consensusTemp > BASELINE_TEMP);
            bool lAbs = (localTemp > CRITICAL_TEMP);
            bool lFast = (localDt > CRITICAL_DT) && (localTemp > BASELINE_TEMP);
            
            String cause = "NETWORK ALARM PROPAGATED";
            if (cAbs || cFast) cause = "CONSENSUS CRITICAL";
            else if (lAbs || lFast) cause = "LOCAL FIRE DETECTED";
            else {
                for (int i = 0; i < neighborCount; i++) {
                    if (neighbors[i].temperature > CRITICAL_TEMP || 
                       (neighbors[i].dT > CRITICAL_DT && neighbors[i].temperature > BASELINE_TEMP)) {
                        cause = "NEIGHBOR " + String(neighbors[i].nodeId) + " DETECTED FIRE";
                        break;
                    }
                }
            }
            Serial.printf("  -> NETWORK CONSENSUS: Temp=%.2f*C | dT=%.3f*C/s [!!! FIRE ALARM: %s !!!]\n", consensusTemp, consensusDt, cause.c_str());
        } else {
            Serial.printf("  -> NETWORK CONSENSUS: Temp=%.2f*C | dT=%.3f*C/s [Normal]\n", consensusTemp, consensusDt);
        }

        if (neighborCount > 0) {
            Serial.println("--- Neighbor List ---");
            for (int i = 0; i < neighborCount; i++) {
                Serial.printf("  ID: %d | Temp: %.2f*C (Var: %.3f) | dT: %.3f*C/s (Var: %.3f) | Seq: %u | MAC: ", 
                              neighbors[i].nodeId, neighbors[i].temperature, neighbors[i].tempVariance, 
                              neighbors[i].dT, neighbors[i].dtVariance, neighbors[i].lastSeq);
                printMac(neighbors[i].mac);
                Serial.println();
            }
            Serial.println("---------------------");
        }

        // Print active alarms from cache
        bool headerPrinted = false;
        for (int i = 0; i < MAX_ACTIVE_ALARMS; i++) {
            if (alarmCache[i].sourceNodeId != 0) {
                uint32_t age = millis() - alarmCache[i].lastReceived;
                if (age <= ALARM_TIMEOUT_MS) {
                    if (!headerPrinted) {
                        Serial.println("--- Active Propagated Alarms ---");
                        headerPrinted = true;
                    }
                    Serial.printf("  Source ID: %d | Max Seq: %u | Age: %u ms\n", 
                                  alarmCache[i].sourceNodeId, 
                                  alarmCache[i].maxSeqNum, 
                                  age);
                }
            }
        }
        if (headerPrinted) Serial.println("--------------------------------");
    }
}
