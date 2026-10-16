#include "app/local_tvt/local_tvt_calldata.h"
#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "app/local_tvt_local.h"
#include "constants/zones.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "save/hollow_rival_savedata.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/zone_weather.h"

// The table of the calls the player can make on the Xtransceiver and the save data of the calls made, named after
// the ROM's "local_tvt_calldata.c". The table has an entry for each person and count of calls made with it, with
// the messages of that call and where the person is

// The archive of the table
#define ARC_LOCAL_TVT_CALL_DATA 240
#define CALL_DATA_COUNT 142

// Where the face of a person is
enum {
    CALL_BACKGROUND_ZONE,
    CALL_BACKGROUND_0,
    CALL_BACKGROUND_1,
};

struct LocalTvtCallEntry {
    u16 person;
    u16 callCount;
    u16 background;
    u16 zoneId;
    u16 listed;
    u16 greetingMsg;
    u16 chatMsg;
    u16 byeMsg;
    u16 extraMsg;
    u16 unk12;
};

struct LocalTvtCallData {
    GameData *gameData;
    LocalTvtCallEntry *entries;
    GameSystem *gsys;
    HollowRivalSave *save;
};

static LocalTvtCallEntry *LocalTvtCallData_GetEntry(LocalTvtCallData *data, u16 person);

// Where LOCAL_TVT_CALL_SECRET can be called from once the calls with it have reached the last stage
// clang-format off
static const u32 sLocalTvtSecretZones[] = {
    ZONE_NIMBASA_CITY, ZONE_DRIFTVEIL_CITY, ZONE_MISTRALTON_CITY, ZONE_LACUNOSA_TOWN, ZONE_UNDELLA_TOWN,
    ZONE_LENTIMAS_TOWN, ZONE_ROUTE_5,       ZONE_ROUTE_6,         ZONE_ROUTE_7,       ZONE_ROUTE_9,
    ZONE_ROUTE_11,      ZONE_ROUTE_12,      ZONE_ROUTE_13,        ZONE_ROUTE_14,      ZONE_ROUTE_16,
};

// clang-format on

LocalTvtCallData *LocalTvtCallData_Create(GameSystem *gsys, GameData *gameData, HeapID heapId) {
    LocalTvtCallData *data = GFL_HeapAllocate(heapId, sizeof(LocalTvtCallData), TRUE, "local_tvt_calldata.c", 103);

    data->gameData = gameData;
    data->gsys = gsys;
    data->save = getHollow_RivalBlk(GameData_GetSaveControl(gameData));
    data->entries = GFL_ArcSysReadHeapNewLZ(ARC_LOCAL_TVT_CALL_DATA, 0, FALSE, heapId);
    return data;
}

void LocalTvtCallData_Free(LocalTvtCallData *data) {
    GFL_HeapFree(data->entries);
    GFL_HeapFree(data);
}

u32 LocalTvtCallData_GetBackground(LocalTvtCallData *data, u16 person) {
    LocalTvtCallEntry *entry = LocalTvtCallData_GetEntry(data, person);

    switch (entry->background) {
    case CALL_BACKGROUND_0:
        return 0;
    case CALL_BACKGROUND_1:
        return 1;
    case CALL_BACKGROUND_ZONE:
        return LocalTvtCallData_GetZoneBackground(data, entry->zoneId);
    }
    return 0;
}

u32 LocalTvtCallData_GetObject(LocalTvtCallData *data, u16 person) {
    LocalTvtCallEntry *entry = LocalTvtCallData_GetEntry(data, person);

    return LocalTvtCallData_GetZoneObject(data, GetVersionedMapChangeZoneNum2(entry->zoneId));
}

u32 LocalTvtCallData_GetBadgeCount(LocalTvtCallData *data) {
    return getBadgeCount(getTrainerGameInfoAddress(GameData_GetSaveControl(data->gameData)));
}

GameData *LocalTvtCallData_GetGameData(LocalTvtCallData *data) {
    return data->gameData;
}

u16 LocalTvtCallData_GetGreetingMsg(LocalTvtCallData *data, u16 person) {
    return LocalTvtCallData_GetEntry(data, person)->greetingMsg;
}

u16 LocalTvtCallData_GetChatMsg(LocalTvtCallData *data, u16 person) {
    return LocalTvtCallData_GetEntry(data, person)->chatMsg;
}

u16 LocalTvtCallData_GetByeMsg(LocalTvtCallData *data, u16 person) {
    return LocalTvtCallData_GetEntry(data, person)->byeMsg;
}

u16 LocalTvtCallData_GetExtraMsg(LocalTvtCallData *data, u16 person) {
    return LocalTvtCallData_GetEntry(data, person)->extraMsg;
}

BOOL LocalTvtCallData_CanCall(LocalTvtCallData *data, u16 person) {
    if (person == LOCAL_TVT_CALL_SECRET) {
        BOOL unk74 = func_0200ff74(data->save) == 0;
        BOOL flag = func_0200ff60(data->save, LOCAL_TVT_CALL_SECRET);
        u32 callCount = LocalTvtCallData_GetCallCount(data, LOCAL_TVT_CALL_SECRET);
        u32 stage = LocalTvtSecret_GetStage(callCount);
        u32 unk0378 = LocalTvtSecret_GetStage0378(stage);
        u32 zoneId = PlayerState_GetZoneID(GameData_GetPlayerState(data->gameData));
        BOOL inZone = func_02017af0(data->gameData, zoneId);

        if (callCount == LocalTvtSecret_GetStageCount(stage)) {
            u32 i;

            for (i = 0; i < NELEMS(sLocalTvtSecretZones); i++) {
                if (zoneId == sLocalTvtSecretZones[i]) {
                    inZone = TRUE;
                    break;
                }
            }
        }
        if (unk74 && flag && inZone && unk0378) {
            return TRUE;
        }
        return FALSE;
    }
    return func_0200ff60(data->save, person);
}

BOOL LocalTvtCallData_IsListed(LocalTvtCallData *data, u16 person) {
    if (LocalTvtCallData_GetEntry(data, person)->listed != 0) {
        return TRUE;
    }
    return FALSE;
}

u32 LocalTvtCallData_GetZoneBackground(LocalTvtCallData *data, u16 zoneId) {
    switch (GetZoneBattleBGID(zoneId)) {
    case 0:
    case 1:
    case 6:
        return 3;
    case 2:
    case 3:
        return 2;
    case 4:
    case 5:
        return 1;
    case 7:
        return 4;
    case 8:
        return 5;
    case 9:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
        return 0;
    case 10:
        return 6;
    }
    return 3;
}

u32 LocalTvtCallData_GetZoneObject(LocalTvtCallData *data, u16 zoneId) {
    switch (GetWeatherAll(data->gsys, zoneId)) {
    case 0:
    case 9:
    case 10:
    case 11:
    case 13:
    case 14:
        return LOCAL_TVT_OBJECT_NONE;
    case 1:
    case 4:
    case 5:
    case 8:
        return 2;
    case 2:
    case 6:
    case 7:
        return 0;
    case 3:
    case 12:
        return 1;
    }
    return LOCAL_TVT_OBJECT_NONE;
}

u16 LocalTvtCallData_GetCallCount(LocalTvtCallData *data, u16 person) {
    return func_0200ff54(data->save, person);
}

BOOL LocalTvtCallData_IsPlayerMale(LocalTvtCallData *data) {
    if (getTrainerGender(GetGameDataPlayerInfo(data->gameData)) == 0) {
        return TRUE;
    }
    return FALSE;
}

void LocalTvtCallData_AddCall(LocalTvtCallData *data, u16 person, u32 max) {
    u32 callCount = func_0200ff54(data->save, person);

    if (callCount < max) {
        func_0200ff50(data->save, person, callCount + 1);
    }
}

void LocalTvtCallData_Set0200ff68(LocalTvtCallData *data, u32 value) {
    func_0200ff68(data->save, value);
}

static LocalTvtCallEntry *LocalTvtCallData_GetEntry(LocalTvtCallData *data, u16 person) {
    u16 callCount = func_0200ff54(data->save, person);
    int i;

    for (i = 0; i < CALL_DATA_COUNT; i++) {
        LocalTvtCallEntry *entry = &data->entries[i];

        if (entry->person == person && entry->callCount == callCount) {
            return entry;
        }
    }
    return NULL;
}

void LocalTvtCallData_Set0200ff78(LocalTvtCallData *data) {
    func_0200ff78(data->save, 1);
}

BOOL LocalTvtCallData_Get0200ff8c(LocalTvtCallData *data) {
    return func_0200ff8c(data->save);
}

void LocalTvtCallData_Reset0200ffcc(LocalTvtCallData *data) {
    if (func_0200ffcc(data->save)) {
        func_0200ff68(data->save, 0);
        func_0200ff94(data->save, 0);
        func_0200ffb8(data->save, 0);
    }
}

BOOL LocalTvtCallData_IsSecretUnknown(LocalTvtCallData *data) {
    if (func_0200ff60(data->save, LOCAL_TVT_CALL_SECRET) == 0) {
        return TRUE;
    }
    return FALSE;
}
