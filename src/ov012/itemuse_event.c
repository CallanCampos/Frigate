#include "types.h"
#include "field/event_fishing.h"
#include "field/event_mapchange.h"
#include "field/event_sweet_scent.h"
#include "field/field.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_surf.h"
#include "field/intrude_work.h"
#include "field/itemuse_event.h"
#include "field/player_action.h"
#include "field/player_state.h"
#include "field/subscreen.h"
#include "field/zone.h"
#include "gfl/std.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// Whether a field action is blocked where the player stands
typedef enum {
    FIELD_ACTION_BLOCKED = -1,
    FIELD_ACTION_ALLOWED = 0,
    // Allowed, while the player walks with a partner
    FIELD_ACTION_ALLOWED_PAIRED = 1,
} FieldActionResult;

typedef FieldActionResult (*FieldActionBlockCheck)(GameData *gameData, Field *field, PlayerState *state);

typedef struct {
    GameEvent *(*create)(Field *field, GameSystem *gsys);
    // The field action the event takes, 0xff for none
    u32 action;
} FieldCommonEvent;

typedef struct {
    GameSystem *gsys;
    u32 timer;
} EventFieldToggleCyclingWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
} EventFieldToggleDowsingWork;

static FieldActionResult CheckFieldActionBlocked(u32 action, GameSystem *gsys, Field *field);
static FieldActionResult FieldActionBlockCheck_Cycling(GameData *gameData, Field *field, PlayerState *state);
static FieldActionResult FieldActionBlockCheck_EscapeRope(GameData *gameData, Field *field, PlayerState *state);
static FieldActionResult FieldActionBlockCheck_NormalZone(GameData *gameData, Field *field, PlayerState *state);
static FieldActionResult FieldActionBlockCheck_Surf(GameData *gameData, Field *field, PlayerState *state);
static FieldActionResult FieldActionBlockCheck_NormalZoneNotRoyalUnova(GameData *gameData, Field *field,
                                                                       PlayerState *state);
static GameEventReturnCode EventFieldToggleCycling_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldToggleDowsing_Callback(GameEvent *event, u32 *state, void *data);

static const FieldCommonEvent FIELD_COMMON_EVENTS[FIELD_COMMON_EVENT_COUNT] = {
    { EventFieldToggleCycling_Create, 0 },  { EventEntralinkWarpIn_CreateDefault, 0xff },
    { EventMapChangeEscapeRope_Create, 3 }, { EventSweetScent_Create, 0xff },
    { EventFieldFishing_Create, 5 },        { EventFieldToggleDowsing_Create, 9 },
};

static const FieldActionBlockCheck FIELD_ACTION_BLOCK_CHECKS[12] = {
    FieldActionBlockCheck_Cycling,
    NULL,
    NULL,
    FieldActionBlockCheck_EscapeRope,
    FieldActionBlockCheck_NormalZone,
    FieldActionBlockCheck_Surf,
    FieldActionBlockCheck_NormalZoneNotRoyalUnova,
    NULL,
    NULL,
    FieldActionBlockCheck_NormalZoneNotRoyalUnova,
    FieldActionBlockCheck_NormalZoneNotRoyalUnova,
    FieldActionBlockCheck_NormalZoneNotRoyalUnova,
};

void PlayerActionPerms_Create(PlayerActionPerms *perms, GameSystem *gsys, Field *field) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PlayerState *state = GameData_GetPlayerState(gameData);
    int i;

    MI_CpuFill8(perms, 0, sizeof(PlayerActionPerms));
    for (i = 0; i < 12; i++) {
        PlayerActionPerms_SetActionBlocked(perms, i, CheckFieldActionBlocked(i, gsys, field));
    }
    perms->exState = FieldPlayerState_GetExState(state);
    perms->paired = GameData_CheckPairFlag(gameData);
}

static FieldActionResult CheckFieldActionBlocked(u32 action, GameSystem *gsys, Field *field) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PlayerState *state = GameData_GetPlayerState(gameData);

    if (FIELD_ACTION_BLOCK_CHECKS[action] == NULL) {
        return FIELD_ACTION_ALLOWED;
    }
    return FIELD_ACTION_BLOCK_CHECKS[action](gameData, field, state);
}

GameEvent *CallFieldCommonEventFunc(u32 id, GameSystem *gsys, Field *field) {
    return FIELD_COMMON_EVENTS[id].create(field, gsys);
}

static FieldActionResult FieldActionBlockCheck_Cycling(GameData *gameData, Field *field, PlayerState *state) {
    u16 zoneId = PlayerState_GetZoneID(state);
    u8 exState = FieldPlayerState_GetExState(state);
    BOOL tileBlocks = MapTile_IsBlocksCycling(GetTileClass(FieldPlayer_GetTileTypeUnder(Field_GetPlayer(field))));

    if (exState == 1) {
        if (tileBlocks) {
            return FIELD_ACTION_BLOCKED;
        }
        return FIELD_ACTION_ALLOWED;
    }
    if (!GetZoneFlagsEnableCycling(zoneId)) {
        return FIELD_ACTION_BLOCKED;
    }
    if (exState != 0) {
        return FIELD_ACTION_BLOCKED;
    }
    if (tileBlocks) {
        return FIELD_ACTION_BLOCKED;
    }
    if (GameData_CheckPairFlag(gameData)) {
        return FIELD_ACTION_ALLOWED_PAIRED;
    }
    return FIELD_ACTION_ALLOWED;
}

static FieldActionResult FieldActionBlockCheck_EscapeRope(GameData *gameData, Field *field, PlayerState *state) {
    u16 zoneId = PlayerState_GetZoneID(state);

    if (!IsZoneNormal(zoneId)) {
        return FIELD_ACTION_BLOCKED;
    }
    if (!GetZoneFlagsEnableEscapeRope(zoneId)) {
        return FIELD_ACTION_BLOCKED;
    }
    if (GameData_CheckPairFlag(gameData)) {
        return FIELD_ACTION_ALLOWED_PAIRED;
    }
    return FIELD_ACTION_ALLOWED;
}

static FieldActionResult FieldActionBlockCheck_NormalZone(GameData *gameData, Field *field, PlayerState *state) {
    if (IsZoneNormal(PlayerState_GetZoneID(state))) {
        return FIELD_ACTION_ALLOWED;
    }
    return FIELD_ACTION_BLOCKED;
}

static FieldActionResult FieldActionBlockCheck_Surf(GameData *gameData, Field *field, PlayerState *state) {
    if (!IsZoneNormal(PlayerState_GetZoneID(state))) {
        return FIELD_ACTION_BLOCKED;
    }
    if (!CreateSurfPos(gameData, field, NULL)) {
        return FIELD_ACTION_BLOCKED;
    }
    if (GameData_CheckPairFlag(gameData)) {
        return FIELD_ACTION_ALLOWED_PAIRED;
    }
    return FIELD_ACTION_ALLOWED;
}

static FieldActionResult FieldActionBlockCheck_NormalZoneNotRoyalUnova(GameData *gameData, Field *field,
                                                                       PlayerState *state) {
    u16 zoneId = PlayerState_GetZoneID(state);

    if (IsZoneRoyalUnova(zoneId)) {
        return FIELD_ACTION_BLOCKED;
    }
    if (IsZoneNormal(zoneId)) {
        return FIELD_ACTION_ALLOWED;
    }
    return FIELD_ACTION_BLOCKED;
}

static GameEventReturnCode EventFieldToggleCycling_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldToggleCyclingWork *work = data;
    Field *field = GSYS_GetField(work->gsys);
    GameCommSys *commSys;

    Field_GetActorSystem(field);
    switch (*state) {
    case 0:
        commSys = GSYS_GetGameCommSystem(work->gsys);
        work->timer++;
        if (Intrude_IsSessionEndPending(commSys) == TRUE && work->timer < 30) {
            break;
        }
        Field_ToggleCycling(field);
        (*state)++;
    case 1:
        if (FieldPlayer_StepFormChange(Field_GetPlayer(field)) == TRUE) {
            (*state)++;
        }
        break;
    case 2:
        if (FieldPlayer_WaitModelReady(Field_GetPlayer(field)) == TRUE) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventFieldToggleCycling_Create(Field *field, GameSystem *gsys) {
    GameEvent *event =
        GameEvent_Create(gsys, NULL, EventFieldToggleCycling_Callback, sizeof(EventFieldToggleCyclingWork));
    EventFieldToggleCyclingWork *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->timer = 0;
    return event;
}

GameEvent *EventEntralinkWarpIn_CreateDefault(Field *field, GameSystem *gsys) {
    VecFx32 pos;

    pos.x = FX32_CONST(512);
    pos.y = 32;
    pos.z = FX32_CONST(488);
    return EventEntralinkWarpIn_Create(gsys, 0x117, &pos, 0);
}

static GameEventReturnCode EventFieldToggleDowsing_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldToggleDowsingWork *work = data;
    FieldSubscreen *subscreen = Field_GetSubscreen(work->field);

    if (FieldSubscreen_GetScreenID(subscreen) == 6) {
        FieldSubscreen_ReqChange(subscreen, 0);
    } else {
        FieldSubscreen_ReqChange(subscreen, 6);
    }
    return GAMEEVENT_DONE;
}

GameEvent *EventFieldToggleDowsing_Create(Field *field, GameSystem *gsys) {
    GameEvent *event =
        GameEvent_Create(gsys, NULL, EventFieldToggleDowsing_Callback, sizeof(EventFieldToggleDowsingWork));
    EventFieldToggleDowsingWork *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->field = field;
    return event;
}
