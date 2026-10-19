#include "system/comm_player_support.h"
#include "types.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "save/player_info.h"

// The support that another player gives over communication, used once by the next battle. Our names

struct CommPlayerSupport {
    u32 type;
    // The support the battle used, and the support that was set when the last battle ended
    u32 usedType;
    u32 lastType;
    PlayerInfo supporter;
    // The player whose support the battle used
    PlayerInfo usedSupporter;
};

CommPlayerSupport *CommPlayerSupport_Create(HeapID heapId) {
    CommPlayerSupport *support = GFL_HeapAllocate(heapId, sizeof(CommPlayerSupport), TRUE, "comm_player_support.c", 44);

    CommPlayerSupport_Init(support);
    return support;
}

void CommPlayerSupport_Free(CommPlayerSupport *support) {
    GFL_HeapFree(support);
}

void CommPlayerSupport_Init(CommPlayerSupport *support) {
    MI_CpuFill8(support, 0, sizeof(CommPlayerSupport));
    support->type = COMM_PLAYER_SUPPORT_NONE;
    support->usedType = COMM_PLAYER_SUPPORT_NONE;
    support->lastType = COMM_PLAYER_SUPPORT_NONE;
    PlayerInfo_Init(&support->supporter);
    PlayerInfo_Init(&support->usedSupporter);
}

PlayerInfo *CommPlayerSupport_GetSupporter(CommPlayerSupport *support) {
    return &support->supporter;
}

u32 CommPlayerSupport_GetType(CommPlayerSupport *support) {
    return support->type;
}

void CommPlayerSupport_SetUsed(CommPlayerSupport *support) {
    support->usedType = support->type;
    support->usedSupporter = support->supporter;
    support->type = COMM_PLAYER_SUPPORT_USED;
}

void CommPlayerSupport_EndBattle(CommPlayerSupport *support) {
    support->lastType = support->type;
    support->type = COMM_PLAYER_SUPPORT_NONE;
}
