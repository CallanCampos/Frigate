#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/high_link.h"
#include "system/beacon_status.h"

// The game's beacon status, kept in GameData: among others, the greeting that the player's beacons send

// The game beacon's messages, and its default greeting ("Hiya!")
#define MSG_GAME_BEACON TEXT_BANK_GAME_BEACON
#define MSG_GAME_BEACON_DEFAULT_GREETING 21

#define BEACON_STATUS_GREETING_SIZE 9

BeaconStatus *BeaconStatus_Create(HeapID heapId, HeapID msgHeapId) {
    BeaconStatus *status = GFL_HeapAllocate(heapId, sizeof(BeaconStatus), TRUE, "beacon_status.c", 33);
    MsgData *msgData;
    StrBuf *greeting;
    u16 buf[BEACON_STATUS_GREETING_SIZE];

    status->greeting = GFL_StrBufCreate(BEACON_STATUS_GREETING_SIZE, heapId);
    status->givenName = GFL_StrBufCreate(0x10, heapId);
    status->givenPassPower = PASS_POWER_NONE;
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, MSG_GAME_BEACON, msgHeapId);
    greeting = GFL_MsgDataLoadStrbufNew(msgData, MSG_GAME_BEACON_DEFAULT_GREETING);
    GFL_StrBufStoreString(greeting, buf, BEACON_STATUS_GREETING_SIZE);
    GFL_StrBufLoadString(status->greeting, buf);
    GFL_StrBufFree(greeting);
    GFL_MsgDataFree(msgData);
    return status;
}

void BeaconStatus_Free(BeaconStatus *status) {
    GFL_StrBufFree(status->givenName);
    GFL_StrBufFree(status->greeting);
    MI_CpuFill8(status, 0, sizeof(BeaconStatus));
    GFL_HeapFree(status);
}

GameBeaconInfoTable *BeaconStatus_GetInfoTable(BeaconStatus *status) {
    return &status->table;
}

u8 BeaconStatus_GetLogTop(BeaconStatus *status) {
    return status->logTop;
}

void BeaconStatus_SetLogTop(BeaconStatus *status, u8 value) {
    status->logTop = value;
}

StrBuf *BeaconStatus_GetGreeting(BeaconStatus *status) {
    return status->greeting;
}

u8 *BeaconStatus_GetGreetingChanged(BeaconStatus *status) {
    return status->greetingChanged;
}
