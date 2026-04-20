#include "CacheManager.h"
#include "Config.h"

namespace CacheManager {
    typedef struct {
        uint32_t sourceNodeId;
        uint32_t lastPublishedSeqNum;
    } PublishedAlarmRecord;

    PublishedAlarmRecord publishCache[MAX_ACTIVE_ALARMS];

    void init() {
        for (int i = 0; i < MAX_ACTIVE_ALARMS; i++) {
            publishCache[i].sourceNodeId = 0;
            publishCache[i].lastPublishedSeqNum = 0;
        }
    }

    bool isNewAlarm(uint32_t sourceId, uint32_t seqNum) {
        if (sourceId == 0) return false;

        int emptyIdx = -1;

        for (int i = 0; i < MAX_ACTIVE_ALARMS; i++) {
            if (publishCache[i].sourceNodeId == sourceId) {
                if (seqNum > publishCache[i].lastPublishedSeqNum) {
                    publishCache[i].lastPublishedSeqNum = seqNum;
                    return true;
                }
                return false; // We already published this sequence or higher
            }
            if (publishCache[i].sourceNodeId == 0 && emptyIdx == -1) {
                emptyIdx = i;
            }
        }

        // It's a brand new source we haven't seen yet
        if (emptyIdx != -1) {
            publishCache[emptyIdx].sourceNodeId = sourceId;
            publishCache[emptyIdx].lastPublishedSeqNum = seqNum;
            return true;
        }

        // Cache is full, allow publish anyway (rare edge case)
        return true; 
    }
}