#include "types.h"
#include "constants/arc.h"
#include "field/player_state.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nnsys/snd.h"
#include "system/game_data.h"
#include "system/iss_dungeon_sys.h"
#include "system/season.h"

// The interactive sound system's dungeons. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

// The tracks whose pitch a dungeon changes: all but tracks 8 and 9
#define ISS_DUNGEON_PITCH_TRACKS 0xfcff

// A dungeon zone's BGM settings for each season, from the dungeon archive
typedef struct {
    u16 zoneId;
    u16 padding;
    // In 64ths of a semitone
    s16 pitch[SEASON_COUNT];
    // 256 is normal
    u16 tempo[SEASON_COUNT];
    // Zero in every entry, and never read
    s16 pan[SEASON_COUNT];
} ISSDungeonData;

typedef struct {
    u8 count;
    ISSDungeonData *data;
} ISSDungeonList;

struct ISSDungeonSys {
    HeapID heapId;
    PlayerState *playerState;
    GameData *gameData;
    BOOL enabled;
    u16 zoneId;
    // The zone to change to on the next update
    u16 nextZoneId;
    ISSDungeonList *list;
    // The current zone's settings, or NULL if it is not a dungeon
    ISSDungeonData *current;
    // Neutral settings that Reset sets and nothing reads
    ISSDungeonData defaultData;
};

static void ISSDungeon_SetSeason(ISSDungeonData *data, u8 season);
static ISSDungeonList *ISSDungeonSys_LoadArcData(HeapID heapId);
static void ISSDungeonList_Free(ISSDungeonList *list);
static ISSDungeonData *ISSDungeonList_FindByZone(const ISSDungeonList *list, u16 zoneId);
static void ISSDungeonSys_Reset(ISSDungeonSys *sys);
static void ISSDungeonSys_EnableCore(ISSDungeonSys *sys);
static void ISSDungeonSys_DisableCore(ISSDungeonSys *sys);
static void ISSDungeonSys_ChangeZone(ISSDungeonSys *sys, u16 zoneId);

static void ISSDungeon_SetSeason(ISSDungeonData *data, u8 season) {
    GFL_SndBGMSetParams(SND_CHANNEL_MASK_ALL, data->tempo[season], -1, 0);
    NNS_SndPlayerSetTrackPitch(GFL_SndBGMGetHandle(), ISS_DUNGEON_PITCH_TRACKS, data->pitch[season]);
}

static ISSDungeonList *ISSDungeonSys_LoadArcData(HeapID heapId) {
    int i;
    ISSDungeonList *list = GFL_HeapAllocate(heapId, sizeof(ISSDungeonList), FALSE, "iss_dungeon_sys.c", 120);

    list->count = GFL_ArcSysGetDataMax(ARCID_ISS_DUNGEON);
    list->data = GFL_HeapAllocate(heapId, sizeof(ISSDungeonData) * list->count, FALSE, "iss_dungeon_sys.c", 122);
    for (i = 0; i < list->count; i++) {
        GFL_ArcSysReadRange(&list->data[i], ARCID_ISS_DUNGEON, i, 0, sizeof(ISSDungeonData));
    }
    return list;
}

static void ISSDungeonList_Free(ISSDungeonList *list) {
    GFL_HeapFree(list->data);
    GFL_HeapFree(list);
}

static ISSDungeonData *ISSDungeonList_FindByZone(const ISSDungeonList *list, u16 zoneId) {
    int i;

    for (i = 0; i < list->count; i++) {
        if (list->data[i].zoneId == zoneId) {
            return &list->data[i];
        }
    }
    return NULL;
}

ISSDungeonSys *ISSDungeonSys_Create(GameData *gameData, PlayerState *playerState, HeapID heapId) {
    ISSDungeonSys *sys = GFL_HeapAllocate(heapId, sizeof(ISSDungeonSys), FALSE, "iss_dungeon_sys.c", 266);

    sys->heapId = heapId;
    sys->gameData = gameData;
    sys->playerState = playerState;
    sys->enabled = FALSE;
    sys->zoneId = 0xffff;
    sys->nextZoneId = 0xffff;
    sys->list = ISSDungeonSys_LoadArcData(heapId);
    sys->current = NULL;
    ISSDungeonSys_Reset(sys);
    return sys;
}

void ISSDungeonSys_Free(ISSDungeonSys *sys) {
    ISSDungeonSys_Disable(sys);
    ISSDungeonList_Free(sys->list);
    GFL_HeapFree(sys);
}

void ISSDungeonSys_Update(ISSDungeonSys *sys) {
    if (sys->enabled && sys->zoneId != sys->nextZoneId) {
        sys->zoneId = sys->nextZoneId;
        ISSDungeonSys_ChangeZone(sys, sys->zoneId);
    }
}

void ISSDungeonSys_ReqChangeZone(ISSDungeonSys *sys, u16 zoneId) {
    sys->nextZoneId = zoneId;
}

void ISSDungeonSys_Enable(ISSDungeonSys *sys) {
    ISSDungeonSys_EnableCore(sys);
}

void ISSDungeonSys_Disable(ISSDungeonSys *sys) {
    ISSDungeonSys_DisableCore(sys);
}

BOOL ISSDungeonSys_IsEnabled(ISSDungeonSys *sys) {
    return sys->enabled;
}

BOOL ISSDungeonSys_IsZoneRegist(ISSDungeonSys *sys, u16 zoneId) {
    if (sys->list == NULL) {
        return FALSE;
    }
    if (ISSDungeonList_FindByZone(sys->list, zoneId) != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void ISSDungeonSys_Reset(ISSDungeonSys *sys) {
    int i;

    sys->defaultData.zoneId = 0xffff;
    for (i = 0; i < SEASON_COUNT; i++) {
        sys->defaultData.pitch[i] = 0;
        sys->defaultData.tempo[i] = 256;
        sys->defaultData.pan[i] = 0;
    }
}

static void ISSDungeonSys_EnableCore(ISSDungeonSys *sys) {
    if (!sys->enabled) {
        sys->enabled = TRUE;
        ISSDungeonSys_ChangeZone(sys, sys->zoneId);
    }
}

static void ISSDungeonSys_DisableCore(ISSDungeonSys *sys) {
    if (sys->enabled) {
        sys->enabled = FALSE;
    }
}

static void ISSDungeonSys_ChangeZone(ISSDungeonSys *sys, u16 zoneId) {
    sys->current = ISSDungeonList_FindByZone(sys->list, zoneId);
    if (sys->current != NULL) {
        ISSDungeon_SetSeason(sys->current, GameData_GetSeason(sys->gameData));
    }
}
