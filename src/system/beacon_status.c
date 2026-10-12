#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "system/beacon_status.h"

// The game's beacon status, kept in GameData: among others, the greeting that the player's beacons send

// The game beacon's messages, and its default greeting ("Hiya!")
#define MSG_GAME_BEACON TEXT_BANK_GAME_BEACON
#define MSG_GAME_BEACON_DEFAULT_GREETING 21

#define BEACON_STATUS_GREETING_SIZE 9

struct BeaconStatus {
    GameBeaconInfoTable table;
    u8 unkB80[2];
    u8 unkB82;
    u32 unkB84;
    u32 unkB88;
    u32 unkB8C;
    StrBuf *unkB90;
    StrBuf *greeting;
};

BeaconStatus *BeaconStatus_Create(HeapID heapId, HeapID msgHeapId) {
    BeaconStatus *status = GFL_HeapAllocate(heapId, sizeof(BeaconStatus), TRUE, "beacon_status.c", 33);
    MsgData *msgData;
    StrBuf *greeting;
    u16 buf[BEACON_STATUS_GREETING_SIZE];

    status->greeting = GFL_StrBufCreate(BEACON_STATUS_GREETING_SIZE, heapId);
    status->unkB90 = GFL_StrBufCreate(0x10, heapId);
    status->unkB8C = 0x30;
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, MSG_GAME_BEACON, msgHeapId);
    greeting = GFL_MsgDataLoadStrbufNew(msgData, MSG_GAME_BEACON_DEFAULT_GREETING);
    GFL_StrBufStoreString(greeting, buf, BEACON_STATUS_GREETING_SIZE);
    GFL_StrBufLoadString(status->greeting, buf);
    GFL_StrBufFree(greeting);
    GFL_MsgDataFree(msgData);
    return status;
}

void BeaconStatus_Free(BeaconStatus *status) {
    GFL_StrBufFree(status->unkB90);
    GFL_StrBufFree(status->greeting);
    sys_memset(status, 0, sizeof(BeaconStatus));
    GFL_HeapFree(status);
}

GameBeaconInfoTable *func_0202d7a8(BeaconStatus *status) {
    return &status->table;
}

u8 func_0202d7ac(BeaconStatus *status) {
    return status->unkB82;
}

void func_0202d7b8(BeaconStatus *status, u8 value) {
    status->unkB82 = value;
}

StrBuf *BeaconStatus_GetGreeting(BeaconStatus *status) {
    return status->greeting;
}

u8 *func_0202d7d0(BeaconStatus *status) {
    return status->unkB80;
}
