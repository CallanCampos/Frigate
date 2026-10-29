// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/directions.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_acmd.h"
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
    // The direction of the current, which nothing reads
    u16 dir;
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

enum {
    WATER_CURRENT_STATE_START,
    WATER_CURRENT_STATE_FLOW,
    WATER_CURRENT_STATE_END,
};

const WaterCurrentTile *GetWaterCurrentTileConfig(u32 tileClass);
BOOL IsTileWaterCurrent_(u32 tileClass);
const WaterCurrentTileAcmd *GetWaterCurrentFlowTileConfig(u32 tileClass);
u16 WaterCurrent_GetTileAcmd(EventWaterCurrentMove *work);
u16 WaterCurrent_GetFlowAcmd(EventWaterCurrentMove *work);
void WaterCurrent_SlidePlayBumpSe(EventWaterCurrentMove *work);
void WaterCurrent_SlidePlayBumpEffectSe(EventWaterCurrentMove *work);

static const WaterCurrentTileAcmd WATER_CURRENT_FLOW_TILES[4] = {
    { TILE_CLASS_LEDGE_RIGHT, 0x78 },
    { TILE_CLASS_LEDGE_LEFT, 0x77 },
    { TILE_CLASS_LEDGE_UP, 0x75 },
    { TILE_CLASS_LEDGE_DOWN, 0x76 },
};

static const WaterCurrentTile WATER_CURRENT_TILES[4] = {
    { TILE_CLASS_WATER_CURRENT_LEFT, ACMD_WALK_3F + DIR_LEFT, DIR_LEFT },
    { TILE_CLASS_WATER_CURRENT_RIGHT, ACMD_WALK_3F + DIR_RIGHT, DIR_RIGHT },
    { TILE_CLASS_WATER_CURRENT_UP, ACMD_WALK_3F + DIR_UP, DIR_UP },
    { TILE_CLASS_WATER_CURRENT_DOWN, ACMD_WALK_3F + DIR_DOWN, DIR_DOWN },
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

const WaterCurrentTileAcmd *GetWaterCurrentFlowTileConfig(u32 tileClass) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (tileClass == WATER_CURRENT_FLOW_TILES[i].tileClass) {
            return &WATER_CURRENT_FLOW_TILES[i];
        }
    }
    return NULL;
}

u16 WaterCurrent_GetTileClass(EventWaterCurrentMove *work);
GameEventReturnCode WaterCurrent_SlideTaskMain(GameEvent *event, u32 *state, void *data);
void WaterCurrent_SlidePlayLoopSe(EventWaterCurrentMove *work);
void WaterCurrent_SlideFinishBump(EventWaterCurrentMove *work);
void WaterCurrent_SlideSelectSlideEntry(EventWaterCurrentMove *work);
void WaterCurrent_SlideSelectBumpEntry(EventWaterCurrentMove *work);
void WaterCurrent_SlideStartStep(EventWaterCurrentMove *work);
void WaterCurrent_SlideShowSurfPokemon(EventWaterCurrentMove *work);
void WaterCurrent_SlideStopLoopSe(EventWaterCurrentMove *work);

GameEventReturnCode WaterCurrent_SlideTaskMain(GameEvent *event, u32 *state, void *data) {
    EventWaterCurrentMove *work = data;

    switch (*state) {
    case WATER_CURRENT_STATE_START:
        if (!IsTileWaterCurrent_(WaterCurrent_GetTileClass(work))) {
            *state = WATER_CURRENT_STATE_END;
        } else {
            FldeffSurfPokemon_SetWakeEnabled(FieldPlayer_GetRideEffect(work->player), FALSE);
            WaterCurrent_SlidePlayLoopSe(work);
            *state = WATER_CURRENT_STATE_FLOW;
        }
        break;
    case WATER_CURRENT_STATE_FLOW:
        WaterCurrent_SlidePlayLoopSe(work);
        if (FldAct_CheckAcmdEnd(work->actor)) {
            WaterCurrent_SlideFinishBump(work);
            WaterCurrent_SlideSelectSlideEntry(work);
            WaterCurrent_SlideSelectBumpEntry(work);
            if (!work->tile && !work->flowTile) {
                WaterCurrent_SlideStopLoopSe(work);
                *state = WATER_CURRENT_STATE_END;
            } else {
                WaterCurrent_SlideStartStep(work);
            }
        }
        break;
    case WATER_CURRENT_STATE_END:
        WaterCurrent_SlideShowSurfPokemon(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventWaterCurrentMove_Create(GameEvent *parent, GameSystem *gsys, Field *field) {
    FieldPlayer *player = Field_GetPlayer(field);
    GameEvent *event = GameEvent_Create(gsys, parent, WaterCurrent_SlideTaskMain, sizeof(EventWaterCurrentMove));
    EventWaterCurrentMove *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->field = field;
    work->player = player;
    work->actor = FieldPlayer_GetActor(player);
    work->tile = NULL;
    work->flowTile = NULL;
    work->flowing = FALSE;
    work->flowPlayer = GFL_SndSeqGetPlayerIndex(SEQ_SE_DANSA);
    work->endPlayer = GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_83);
    work->loopPlayer = GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_129);
    return event;
}

BOOL IsTileWaterCurrent(u32 tileClass) {
    return IsTileWaterCurrent_(tileClass);
}

u16 WaterCurrent_GetTileAcmd(EventWaterCurrentMove *work) {
    if (work->tile) {
        return work->tile->acmd;
    }
    return 0;
}

u16 WaterCurrent_GetFlowAcmd(EventWaterCurrentMove *work) {
    if (work->flowTile) {
        return work->flowTile->acmd;
    }
    return 0;
}

u16 WaterCurrent_GetTileClass(EventWaterCurrentMove *work) {
    VecFx32 pos;

    FieldPlayer_GetWPos(work->player, &pos);
    return GetTileClass(GetTileTypeAtPos(Field_GetG3DMapper(work->field), &pos));
}

void WaterCurrent_SlideStartStep(EventWaterCurrentMove *work) {
    if (work->flowTile) {
        FldAct_SetAcmd(work->actor, WaterCurrent_GetFlowAcmd(work));
        WaterCurrent_SlidePlayBumpSe(work);
        work->flowing = TRUE;
    } else if (work->tile) {
        FldAct_SetAcmd(work->actor, WaterCurrent_GetTileAcmd(work));
    }
}

void WaterCurrent_SlideFinishBump(EventWaterCurrentMove *work) {
    if (work->flowTile) {
        FieldEffects *effects = Field_GetFieldEffects(work->field);
        u32 effectId = FLDEFF_SURF_POKEMON_SPLASH;
        VecFx32 pos;

        if (!FieldEffects_IsLoaded(effects, effectId)) {
            FieldEffects_Load(effects, &effectId, 1);
        }
        FieldPlayer_GetWPos(work->player, &pos);
        FldeffSurfPokemonSplash_StartAtPos(effects, 0, &pos);
        WaterCurrent_SlidePlayBumpEffectSe(work);
        work->flowTile = NULL;
        work->flowing = FALSE;
    }
}

void WaterCurrent_SlideSelectSlideEntry(EventWaterCurrentMove *work) {
    work->tile = GetWaterCurrentTileConfig(WaterCurrent_GetTileClass(work));
}

void WaterCurrent_SlideSelectBumpEntry(EventWaterCurrentMove *work) {
    work->flowTile = GetWaterCurrentFlowTileConfig(WaterCurrent_GetTileClass(work));
}

void WaterCurrent_SlideShowSurfPokemon(EventWaterCurrentMove *work) {
    FldeffSurfPokemon_SetWakeEnabled(FieldPlayer_GetRideEffect(work->player), TRUE);
    FieldPlayer_SetGridHopLanded(work->player);
}

void WaterCurrent_SlidePlayBumpSe(EventWaterCurrentMove *work) {
    if (!GFL_SndIsLoadThreadRunning()) {
        GFL_SndPlayerStop(work->loopPlayer);
        GFL_SEPlayKeepVol(SEQ_SE_DANSA, work->flowPlayer);
    }
}

void WaterCurrent_SlidePlayBumpEffectSe(EventWaterCurrentMove *work) {
    if (!GFL_SndIsLoadThreadRunning()) {
        GFL_SEPlayKeepVol(SEQ_SE_FLD_83, work->endPlayer);
    }
}

void WaterCurrent_SlidePlayLoopSe(EventWaterCurrentMove *work) {
    if (!GFL_SndIsLoadThreadRunning() && !work->flowing && !GFL_SndPlayerIsActive(work->loopPlayer)) {
        GFL_SEPlayKeepVol(SEQ_SE_FLD_129, work->loopPlayer);
    }
}

void WaterCurrent_SlideStopLoopSe(EventWaterCurrentMove *work) {
    GFL_SndPlayerStop(work->loopPlayer);
}
