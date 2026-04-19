#pragma once
#include <Arduino.h>
#include "Types.h"

namespace MeshManager {
    void init();
    void broadcast(struct_message &data);
    void cleanStaleNeighbors();
    void cleanStaleAlarms();
    void evaluateConsensus(uint32_t localNodeId, float localTemp, float localVar, float localDt, float localDtVar, float &consensusTemp, float &consensusDt, bool &fireAlarm, uint32_t &outAlarmSourceId, uint32_t &outAlarmSeqNum);
    void printStatus(uint32_t localNodeId, float localTemp, float localDt, float consensusTemp, float consensusDt, bool fireAlarm);
}
