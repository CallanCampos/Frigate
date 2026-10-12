#include "types.h"
#include "save/records.h"
#include "app/livebattlematch_flow.h"
#include "gfl/net.h"
#include "gfl/std.h"

// The results of the live competition, which the record block (records.h) starts with: the console's MAC address,
// four counts and the last opponents. The ROM has no string for the file, so the name is a guess.

static void getOpponentMacAddress(LiveMatchData *data, u32 index, u8 *macAddress);

void func_0200f1c4(void *results) {
    LiveMatchData *data = results;

    sys_memset(data, 0, sizeof(LiveMatchData));
    func_0207c33c(data->macAddress);
}

u32 func_0200f1d8(void *results, u32 index) {
    LiveMatchData *data = results;

    return data->counts[index];
}

void func_0200f1e0(void *results, u32 index, int value) {
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
    sys_memcpy(data->opponents[index % LIVEMATCH_OPPONENT_MAX].macAddress, macAddress, BATTLEMATCH_MAC_ADDRESS_SIZE);
}

void func_0200f218(void *results, u32 index, const u8 *macAddress) {
    LiveMatchData *data = results;

    sys_memcpy(macAddress, data->opponents[index % LIVEMATCH_OPPONENT_MAX].macAddress, BATTLEMATCH_MAC_ADDRESS_SIZE);
}

void func_0200f238(void *results, u32 index, u32 field, u8 value) {
    LiveMatchData *data = results;

    data->opponents[index % LIVEMATCH_OPPONENT_MAX].params[field] = value;
}

BOOL func_0200f254(void *results, const u8 *macAddress) {
    LiveMatchData *data = results;
    u8 opponent[BATTLEMATCH_MAC_ADDRESS_SIZE];
    int i;
    int count = func_0200f1d8(data, 0);

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
