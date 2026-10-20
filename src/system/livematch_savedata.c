#include "types.h"
#include "app/livebattlematch_flow.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "save/records.h"

// The results of the live competition, which the record block (records.h) starts with: the console's MAC address,
// four counts and the last opponents. The ROM has no string for the file, so the name is a guess.

static void getOpponentMacAddress(LiveMatchData *data, u32 index, u8 *macAddress);

void LiveMatchData_Init(void *results) {
    LiveMatchData *data = results;

    MI_CpuFill8(data, 0, sizeof(LiveMatchData));
    OS_GetMacAddress(data->macAddress);
}

u32 LiveMatchData_GetCount(void *results, u32 index) {
    LiveMatchData *data = results;

    return data->counts[index];
}

void LiveMatchData_AddCount(void *results, u32 index, int value) {
    LiveMatchData *data = results;
    int count = data->counts[index] + value;

    if (count > LIVEMATCH_COUNT_MAX) {
        count = LIVEMATCH_COUNT_MAX;
    } else if (count < 0) {
        count = 0;
    }
    data->counts[index] = count;
}

static void getOpponentMacAddress(LiveMatchData *data, u32 index, u8 *macAddress) {
    MI_CpuCopy8(data->opponents[index % LIVEMATCH_OPPONENT_MAX].macAddress, macAddress, BATTLEMATCH_MAC_ADDRESS_SIZE);
}

void LiveMatchData_SetOpponentMacAddress(void *results, u32 index, const u8 *macAddress) {
    LiveMatchData *data = results;

    MI_CpuCopy8(macAddress, data->opponents[index % LIVEMATCH_OPPONENT_MAX].macAddress, BATTLEMATCH_MAC_ADDRESS_SIZE);
}

void LiveMatchData_SetOpponentParam(void *results, u32 index, u32 field, u8 value) {
    LiveMatchData *data = results;

    data->opponents[index % LIVEMATCH_OPPONENT_MAX].params[field] = value;
}

BOOL LiveMatchData_HasOpponent(void *results, const u8 *macAddress) {
    LiveMatchData *data = results;
    u8 opponent[BATTLEMATCH_MAC_ADDRESS_SIZE];
    int i;
    int count = LiveMatchData_GetCount(data, 0);

    if (count > LIVEMATCH_OPPONENT_MAX) {
        count = LIVEMATCH_OPPONENT_MAX;
    }
    for (i = 0; i < count; i++) {
        getOpponentMacAddress(data, i, opponent);
        if (GFL_STD_MemCmp(macAddress, opponent, BATTLEMATCH_MAC_ADDRESS_SIZE) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}
