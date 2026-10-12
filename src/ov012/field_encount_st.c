#include "types.h"
#include "field/encounter.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_status.h"
#include "field/hidden_event.h"
#include "field/player_action.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "save/encounter.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/zone_weather.h"

BOOL EncData_Load(EncData *encData, ArcTool *arc, u16 zoneId, u8 season) {
    u32 fileId;
    u32 count;

    sys_memset(encData, 0, sizeof(EncData));
    fileId = GetZoneEncID(zoneId);
    if (fileId == 0x1fff) {
        return FALSE;
    }
    count = GFL_ArcToolGetDataLength(arc, fileId) / sizeof(EncData);
    if (count != 1 && count != 4) {
        return FALSE;
    }
    if (count == 1) {
        season = 0;
    }
    GFL_ArcToolReadRange(arc, fileId, season * sizeof(EncData), sizeof(EncData), encData);
    encData->fishingEnable = TRUE;
    return TRUE;
}

EncountState *EncountState_Create(HeapID heapId) {
    EncountState *state = GFL_HeapAllocate(heapId, sizeof(EncountState), TRUE, "field_encount_st.c", 0x2d);
    EncountState_SetTerrain(state, 0xff);
    return state;
}

void EncountState_Free(EncountState *state) {
    GFL_HeapFree(state);
}

void func_ov012_0215917c(GameData *gameData, Field *field) {
    EncountSystem *system = Field_GetEncountSystem(field);
    FieldPlayer *player;
    EncountState *state;
    u32 terrain;

    player = Field_GetPlayer(field);
    func_ov036_021a203c(system, system->effectEncountState);
    state = GameData_GetEncountState(gameData);
    terrain = FieldPlayer_GetTileTypeUnder(player);
    EncountState_SetTerrain(state, terrain);
}

void func_ov012_021591b4(GameData *gameData) {
    EncountState *state = GameData_GetEncountState(gameData);
    state->phenomenonSteps = 0;
}

void GameData_InitEncountTerrain(GameData *gameData, Field *field) {
    FieldPlayer *player = Field_GetPlayer(field);
    EncountState *state = GameData_GetEncountState(gameData);
    u32 terrain = FieldPlayer_GetTileTypeUnder(player);
    EncountState_SetTerrain(state, terrain);
}

void EncountState_SetTerrain(EncountState *state, u32 terrain) {
    state->terrain = terrain;
    state->rateStepCounter = 0;
    state->encountRate = 1;
    state->rateBlockCounter = 0;
    state->rateStepIncrement = 0;
}

void func_ov012_021591f4(GameData *gameData) {
}

u16 EncountSave_GetRoamingPkmZone(EncountSave *save, u8 slot) {
    u32 clock = EncountSave_GetRoamingPkmZoneClock(save, slot);
    if (clock > 16) {
        return 319;
    }
    return ROAMING_POKEMON_ZONES[clock];
}

u32 func_ov012_02159218(EncountSave *save) {
    return 0;
}

void func_ov012_0215921c(GameData *gameData, u8 slot) {
}

void func_ov012_02159220(GameData *gameData) {
}

u32 GetDefaultWeatherValue(GameData *gameData, u16 zoneId) {
    return WEATHER_NONE;
}

BOOL func_ov012_0215922c(GameData *gameData) {
    return FALSE;
}

void CalcPlayerActionPossibilities(Field *field, PlayerActionPossibilities *action) {
    FieldPlayer *player;
    FieldActor *actor;
    u16 objCode;
    MMSys *actorSystem;
    VecFx32 position;
    u32 direction;
    u32 tileInFront;
    u32 tileUnder;
    u16 diveZone;
    s16 x;
    s16 y;
    s16 z;

    sys_memset(action, 0, sizeof(PlayerActionPossibilities));
    action->zoneId = Field_GetPlayerStateZoneID(field);
    action->gsys = Field_GetGameSystem(field);
    action->field = field;
    player = Field_GetPlayer(field);
    action->exState = FieldPlayer_GetExState(player);
    actor = FieldPlayer_GetActorInFront(player);
    action->actorInFront = actor;
    if (actor != NULL) {
        objCode = FldAct_GetObjCode(actor);
        actorSystem = Field_GetActorSystem(field);
        CopyActorWPos(actor, &position);
        if (IsNPCStrengthRock(objCode) && !func_ov012_0216820c(actorSystem, &position)) {
            action->flags |= 8;
        }
        if (objCode == 0x6c) {
            action->flags |= 1;
        }
    }
    direction = FieldPlayer_GetFaceDir(player);
    tileUnder = FieldPlayer_GetTileTypeUnder(player);
    tileInFront = FieldPlayer_GetTileTypeInDir(player, direction);
    if (CheckSurfHeightAllow(player, direction) == TRUE) {
        action->flags |= 2;
    }
    if (CheckCanInteractWaterfall(player, tileUnder, tileInFront) == TRUE) {
        action->flags |= 4;
    }
    if (GetZoneFlagsEnableFlyFrom(action->zoneId) == TRUE) {
        action->flags |= 0x10;
        action->flags |= 0x40;
    }
    if (GetZoneFlagsEnableEscapeRope(action->zoneId) == TRUE) {
        action->flags |= 0x80;
    }
    if (FieldStatus_GetFlashPerms(GameData_GetFieldStatus(GSYS_GetGameData(action->gsys))) & 1) {
        action->flags |= 0x20;
    }
    if (GetAbyssalRuinsDiveZoneID(field, &diveZone) == TRUE) {
        action->flags |= 0x400;
    }
    if (IsZoneAbyssalRuinsInside(action->zoneId)) {
        FieldPlayer_GetGPos(Field_GetPlayer(action->field), &x, &y, &z);
        if (IsZoneAbyssalRuinsFlashRock(action->zoneId) &&
            func_ov012_02159b70(&ABYSSAL_RUINS_FLASH_ROCK_RADIUS, x, z, 0xd7, field)) {
            action->flags |= 0x20;
        }
        if (IsZoneAbyssalRuinsStrengthRock(action->zoneId) &&
            func_ov012_02159b70(&ABYSSAL_RUINS_STRENGTH_ROCK_RADIUS, x, z, 0xd6, field)) {
            action->flags |= 8;
        }
    }
    if (!func_02018c38(action->zoneId)) {
        action->flags = 0;
    } else {
        action->flags |= 0x100;
        action->flags |= 0x200;
    }
    if (GameData_IsForceSeasonSync(GSYS_GetGameData(action->gsys)) == TRUE) {
        action->flags = 0;
    }
}
