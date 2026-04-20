#pragma once
#include <Arduino.h>
#include "Types.h"

namespace NetworkManager {
    void init();
    void loop();
    void processQueuedPackets();
}