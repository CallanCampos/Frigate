// field_slope_slip.c: the event that slides the player down a slope tile
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/directions.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_map.h"
#include "field/field_slope_slip.h"
#include "gfl/sound.h"
#include "system/game_event.h"

typedef struct {
    u16 tileClass;
    // The actor command that moves the player
    u16 acmd;
    // The direction the player faces
    u16 dir;
    // How many frames pass before the player turns
    u16 turnInterval;
} SlopeSlipTileConfig;

typedef struct {
    GameSystem *gsys;
    Field *field;
    FieldPlayer *player;
    FieldActor *actor;
    const SlopeSlipTileConfig *config;
    u16 dir;
    u32 frames;
} SlopeSlipWork;

enum {
    SLOPE_SLIP_STATE_START,
    SLOPE_SLIP_STATE_SLIDE,
    SLOPE_SLIP_STATE_END,
};

static const SlopeSlipTileConfig SLOPE_SLIP_TILES[4] = {
    { TILE_CLASS_SLOPE_SLIP_LEFT, ACMD_WALK_8F + DIR_LEFT, DIR_LEFT, 2 },
    { TILE_CLASS_SLOPE_SLIP_RIGHT, ACMD_WALK_8F + DIR_RIGHT, DIR_RIGHT, 2 },
    { TILE_CLASS_SLOPE_SLIP_UP, ACMD_WALK_8F + DIR_UP, DIR_UP, 2 },
    { TILE_CLASS_SLOPE_SLIP_DOWN, ACMD_WALK_8F + DIR_DOWN, DIR_DOWN, 2 },
};

const SlopeSlipTileConfig *GetSlopeSlipTileConfig(u32 tileClass);
BOOL IsTileSlopeSlip_(u32 tileClass);
GameEventReturnCode SlopeSlip_SpinTaskMain(GameEvent *event, u32 *state, void *data);
u16 SlopeSlip_GetAcmd(SlopeSlipWork *work);
u16 SlopeSlip_GetDirection(SlopeSlipWork *work);
u16 SlopeSlip_GetTurnInterval(SlopeSlipWork *work);

const SlopeSlipTileConfig *GetSlopeSlipTileConfig(u32 tileClass) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (tileClass == SLOPE_SLIP_TILES[i].tileClass) {
            return &SLOPE_SLIP_TILES[i];
        }
    }
    return NULL;
}

BOOL IsTileSlopeSlip_(u32 tileClass) {
    if (GetSlopeSlipTileConfig(tileClass) != NULL) {
        return TRUE;
    }
    return FALSE;
}

u16 SlopeSlip_GetTileClass(SlopeSlipWork *work);
void SlopeSlip_SpinTick(SlopeSlipWork *work);
void SlopeSlip_SpinStartStep(SlopeSlipWork *work);
void SlopeSlip_SpinSelectEntry(SlopeSlipWork *work);
BOOL SlopeSlip_SpinIsBlocked(SlopeSlipWork *work);
void SlopeSlip_SpinFinish(SlopeSlipWork *work);

GameEventReturnCode SlopeSlip_SpinTaskMain(GameEvent *event, u32 *state, void *data) {
    SlopeSlipWork *work = data;
    switch (*state) {
    case SLOPE_SLIP_STATE_START:
        if (!IsTileSlopeSlip_(SlopeSlip_GetTileClass(work))) {
            *state = SLOPE_SLIP_STATE_END;
        } else {
            *state = SLOPE_SLIP_STATE_SLIDE;
        }
        break;
    case SLOPE_SLIP_STATE_SLIDE:
        SlopeSlip_SpinTick(work);
        if (FldAct_CheckAcmdEnd(work->actor)) {
            SlopeSlip_SpinSelectEntry(work);
            if (SlopeSlip_SpinIsBlocked(work)) {
                GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_35));
                *state = SLOPE_SLIP_STATE_END;
            } else {
                SlopeSlip_SpinStartStep(work);
            }
        }
        break;
    case SLOPE_SLIP_STATE_END:
        SlopeSlip_SpinFinish(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventSlopeSlip_Create(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, SlopeSlip_SpinTaskMain, sizeof(SlopeSlipWork));
    SlopeSlipWork *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->field = field;
    work->player = Field_GetPlayer(field);
    work->actor = FieldPlayer_GetActor(work->player);
    work->config = NULL;
    work->dir = GetActorFaceDir(work->actor);
    work->frames = 0;
    return event;
}

BOOL IsTileSlopeSlip(u32 tileClass) {
    return IsTileSlopeSlip_(tileClass);
}

u16 SlopeSlip_GetAcmd(SlopeSlipWork *work) {
    if (work->config != NULL) {
        return work->config->acmd;
    }
    return 0;
}

u16 SlopeSlip_GetDirection(SlopeSlipWork *work) {
    if (work->config != NULL) {
        return work->config->dir;
    }
    return 0;
}

u16 SlopeSlip_GetTurnInterval(SlopeSlipWork *work) {
    if (work->config != NULL) {
        return work->config->turnInterval;
    }
    return 0;
}

u16 SlopeSlip_GetTileClass(SlopeSlipWork *work) {
    VecFx32 pos;

    FieldPlayer_GetWPos(work->player, &pos);
    return GetTileClass(GetTileTypeAtPos(Field_GetG3DMapper(work->field), &pos));
}

void SlopeSlip_SpinTick(SlopeSlipWork *work) {
    u32 interval = SlopeSlip_GetTurnInterval(work);

    work->frames++;
    if (work->frames % interval == 0) {
        work->dir = (work->dir + 1) % 4;
        ClearActorFlag(work->actor, ACTOR_FLAG_FACE_DIR_LOCK);
        CheckSetActorFaceDir(work->actor, work->dir);
        SetActorFlag(work->actor, ACTOR_FLAG_FACE_DIR_LOCK);
    }
}

void SlopeSlip_SpinStartStep(SlopeSlipWork *work) {
    FldAct_SetAcmd(work->actor, SlopeSlip_GetAcmd(work));
    SetActorFlag(work->actor, ACTOR_FLAG_FACE_DIR_LOCK);
    if (!GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_35))) {
        GFL_SndSEPlay(SEQ_SE_FLD_35);
    }
}

void SlopeSlip_SpinSelectEntry(SlopeSlipWork *work) {
    const SlopeSlipTileConfig *config = GetSlopeSlipTileConfig(SlopeSlip_GetTileClass(work));

    if (config != NULL) {
        work->config = config;
    }
}

BOOL SlopeSlip_SpinIsBlocked(SlopeSlipWork *work) {
    if (ActorRouteCollCheckOneTileInDir(work->actor, SlopeSlip_GetDirection(work))) {
        return TRUE;
    }
    return FALSE;
}

void SlopeSlip_SpinFinish(SlopeSlipWork *work) {
    ClearActorFlag(work->actor, ACTOR_FLAG_FACE_DIR_LOCK);
    if (work->config != NULL) {
        CheckSetActorFaceDir(work->actor, SlopeSlip_GetDirection(work));
    }
}
