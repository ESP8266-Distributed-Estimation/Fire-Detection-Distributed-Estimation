#pragma once
#include <Arduino.h>
#include "Types.h"

namespace CacheManager {
    void init();
    // Returns true if this is a new alarm sequence we haven't published yet
    bool isNewAlarm(uint32_t sourceId, uint32_t seqNum);
}