#pragma once
#include <Arduino.h>
#include "Types.h"

namespace MeshManager {
    void init();
    void broadcast(struct_message &data);
    void cleanStaleNeighbors();
    void printStatus(uint32_t localNodeId, float localTemp, float localHum, float localPres);
}
