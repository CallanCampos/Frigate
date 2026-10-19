#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_water_current.h"
#include "gfl/sound.h"
#include "system/game_event.h"

typedef struct WaterCurrentTile {
    u16 tileClass;
    u16 acmd;
    u16 unused;
} WaterCurrentTile;

typedef struct WaterCurrentTileAcmd {
    u16 tileClass;
    u16 acmd;
} WaterCurrentTileAcmd;

typedef struct EventWaterCurrentMove {
    GameSystem *gsys;
    Field *field;
    FieldPlayer *player;
    FieldActor *actor;
    const WaterCurrentTile *tile;
    const WaterCurrentTileAcmd *flowTile;
    BOOL flowing;
    s32 flowPlayer;
    s32 endPlayer;
    s32 loopPlayer;
} EventWaterCurrentMove;

const WaterCurrentTile *GetWaterCurrentTileConfig(u32 tileClass);
BOOL IsTileWaterCurrent_(u32 tileClass);
const WaterCurrentTileAcmd *func_ov036_021c24f0(u32 tileClass);
u16 func_ov036_021c2610(EventWaterCurrentMove *work);
u16 func_ov036_021c2620(EventWaterCurrentMove *work);
void func_ov036_021c2714(EventWaterCurrentMove *work);
void func_ov036_021c2734(EventWaterCurrentMove *work);

static const WaterCurrentTileAcmd WATER_CURRENT_FLOW_TILES[4] = {
    { 0x72, 0x78 },
    { 0x73, 0x77 },
    { 0x74, 0x75 },
    { 0x75, 0x76 },
};

static const WaterCurrentTile WATER_CURRENT_TILES[4] = {
    { 0x95, 0x52, 0x2 },
    { 0x94, 0x53, 0x3 },
    { 0x96, 0x50, 0x0 },
    { 0x97, 0x51, 0x1 },
};

const WaterCurrentTile *GetWaterCurrentTileConfig(u32 tileClass) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (tileClass == WATER_CURRENT_TILES[i].tileClass) {
            return &WATER_CURRENT_TILES[i];
        }
    }
    return NULL;
}

BOOL IsTileWaterCurrent_(u32 tileClass) {
    if (GetWaterCurrentTileConfig(tileClass)) {
        return TRUE;
    }
    return FALSE;
}

const WaterCurrentTileAcmd *func_ov036_021c24f0(u32 tileClass) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (tileClass == WATER_CURRENT_FLOW_TILES[i].tileClass) {
            return &WATER_CURRENT_FLOW_TILES[i];
        }
    }
    return NULL;
}

u16 func_ov036_021c2630(EventWaterCurrentMove *work);
GameEventReturnCode func_ov036_021c2514(GameEvent *event, u32 *state, void *data);
void func_ov036_021c2750(EventWaterCurrentMove *work);
void func_ov036_021c268c(EventWaterCurrentMove *work);
void func_ov036_021c26dc(EventWaterCurrentMove *work);
void func_ov036_021c26ec(EventWaterCurrentMove *work);
void func_ov036_021c2654(EventWaterCurrentMove *work);
void func_ov036_021c26fc(EventWaterCurrentMove *work);
void func_ov036_021c277c(EventWaterCurrentMove *work);

GameEventReturnCode func_ov036_021c2514(GameEvent *event, u32 *state, void *data) {
    EventWaterCurrentMove *work = data;

    switch (*state) {
    case 0:
        if (!IsTileWaterCurrent_(func_ov036_021c2630(work))) {
            *state = 2;
        } else {
            func_ov036_021a4514((FieldEffectTask *)func_ov036_0219a5c4(work->player), FALSE);
            func_ov036_021c2750(work);
            *state = 1;
        }
        break;
    case 1:
        func_ov036_021c2750(work);
        if (func_ov012_02166ecc(work->actor)) {
            func_ov036_021c268c(work);
            func_ov036_021c26dc(work);
            func_ov036_021c26ec(work);
            if (!work->tile && !work->flowTile) {
                func_ov036_021c277c(work);
                *state = 2;
            } else {
                func_ov036_021c2654(work);
            }
        }
        break;
    case 2:
        func_ov036_021c26fc(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventWaterCurrentMove_Create(GameEvent *parent, GameSystem *gsys, Field *field) {
    FieldPlayer *player = Field_GetPlayer(field);
    GameEvent *event = GameEvent_Create(gsys, parent, func_ov036_021c2514, sizeof(EventWaterCurrentMove));
    EventWaterCurrentMove *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->field = field;
    work->player = player;
    work->actor = FieldPlayer_GetActor(player);
    work->tile = NULL;
    work->flowTile = NULL;
    work->flowing = FALSE;
    work->flowPlayer = GFL_SndSeqGetPlayerIndex(0x55e);
    work->endPlayer = GFL_SndSeqGetPlayerIndex(0x6e2);
    work->loopPlayer = GFL_SndSeqGetPlayerIndex(0x768);
    return event;
}

BOOL IsTileWaterCurrent(u32 tileClass) {
    return IsTileWaterCurrent_(tileClass);
}

u16 func_ov036_021c2610(EventWaterCurrentMove *work) {
    if (work->tile) {
        return work->tile->acmd;
    }
    return 0;
}

u16 func_ov036_021c2620(EventWaterCurrentMove *work) {
    if (work->flowTile) {
        return work->flowTile->acmd;
    }
    return 0;
}

u16 func_ov036_021c2630(EventWaterCurrentMove *work) {
    VecFx32 pos;

    FieldPlayer_GetWPos(work->player, &pos);
    return GetTileClass(GetTileTypeAtPos(Field_GetG3DMapper(work->field), &pos));
}

void func_ov036_021c2654(EventWaterCurrentMove *work) {
    if (work->flowTile) {
        FldAct_SetAcmd(work->actor, func_ov036_021c2620(work));
        func_ov036_021c2714(work);
        work->flowing = TRUE;
    } else if (work->tile) {
        FldAct_SetAcmd(work->actor, func_ov036_021c2610(work));
    }
}

void func_ov036_021c268c(EventWaterCurrentMove *work) {
    if (work->flowTile) {
        FieldEffects *effects = Field_GetFieldEffects(work->field);
        u32 effectId = 8;
        VecFx32 pos;

        if (!FieldEffects_IsLoaded(effects, effectId)) {
            FieldEffects_Load(effects, &effectId, 1);
        }
        FieldPlayer_GetWPos(work->player, &pos);
        func_ov036_021a4ee4(effects, 0, &pos);
        func_ov036_021c2734(work);
        work->flowTile = NULL;
        work->flowing = FALSE;
    }
}

void func_ov036_021c26dc(EventWaterCurrentMove *work) {
    work->tile = GetWaterCurrentTileConfig(func_ov036_021c2630(work));
}

void func_ov036_021c26ec(EventWaterCurrentMove *work) {
    work->flowTile = func_ov036_021c24f0(func_ov036_021c2630(work));
}

void func_ov036_021c26fc(EventWaterCurrentMove *work) {
    func_ov036_021a4514((FieldEffectTask *)func_ov036_0219a5c4(work->player), TRUE);
    func_ov036_0219ac0c(work->player);
}

void func_ov036_021c2714(EventWaterCurrentMove *work) {
    if (!func_02005cbc()) {
        GFL_SndPlayerStop(work->loopPlayer);
        GFL_SEPlayKeepVol(0x55e, work->flowPlayer);
    }
}

void func_ov036_021c2734(EventWaterCurrentMove *work) {
    if (!func_02005cbc()) {
        GFL_SEPlayKeepVol(0x6e2, work->endPlayer);
    }
}

void func_ov036_021c2750(EventWaterCurrentMove *work) {
    if (!func_02005cbc() && !work->flowing && !GFL_SndPlayerIsActive(work->loopPlayer)) {
        GFL_SEPlayKeepVol(0x768, work->loopPlayer);
    }
}

void func_ov036_021c277c(EventWaterCurrentMove *work) {
    GFL_SndPlayerStop(work->loopPlayer);
}
