// field_slope_slip.c: the event that slides the player down a slope tile
#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_map.h"
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

static const SlopeSlipTileConfig SLOPE_SLIP_TILES[4] = {
    { 0x77, 0xe, 2, 2 },
    { 0x76, 0xf, 3, 2 },
    { 0x78, 0xc, 0, 2 },
    { 0x79, 0xd, 1, 2 },
};

const SlopeSlipTileConfig *GetSlopeSlipTileConfig(u32 tileClass);
BOOL IsTileSlopeSlip_(u32 tileClass);
GameEventReturnCode func_ov036_021c22d4(GameEvent *event, u32 *state, SlopeSlipWork *work);
GameEvent *EventSlopeSlip_Create(GameEvent *parent, GameSystem *gsys, Field *field);
BOOL IsTileSlopeSlip(u32 tileClass);
u16 func_ov036_021c2394(SlopeSlipWork *work);
u16 func_ov036_021c23a4(SlopeSlipWork *work);
u16 func_ov036_021c23b4(SlopeSlipWork *work);

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

u16 func_ov036_021c23c4(SlopeSlipWork *work);
void func_ov036_021c23e8(SlopeSlipWork *work);
void func_ov036_021c242c(SlopeSlipWork *work);
void func_ov036_021c2460(SlopeSlipWork *work);
BOOL func_ov036_021c2474(SlopeSlipWork *work);
void func_ov036_021c2490(SlopeSlipWork *work);

GameEventReturnCode func_ov036_021c22d4(GameEvent *event, u32 *state, SlopeSlipWork *work) {
    switch (*state) {
    case 0:
        if (!IsTileSlopeSlip_(func_ov036_021c23c4(work))) {
            *state = 2;
        } else {
            *state = 1;
        }
        break;
    case 1:
        func_ov036_021c23e8(work);
        if (FldAct_CheckAcmdEnd(work->actor)) {
            func_ov036_021c2460(work);
            if (func_ov036_021c2474(work)) {
                GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_35));
                *state = 2;
            } else {
                func_ov036_021c242c(work);
            }
        }
        break;
    case 2:
        func_ov036_021c2490(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventSlopeSlip_Create(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, (GameEventCallback)func_ov036_021c22d4, sizeof(SlopeSlipWork));
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

u16 func_ov036_021c2394(SlopeSlipWork *work) {
    if (work->config != NULL) {
        return work->config->acmd;
    }
    return 0;
}

u16 func_ov036_021c23a4(SlopeSlipWork *work) {
    if (work->config != NULL) {
        return work->config->dir;
    }
    return 0;
}

u16 func_ov036_021c23b4(SlopeSlipWork *work) {
    if (work->config != NULL) {
        return work->config->turnInterval;
    }
    return 0;
}

u16 func_ov036_021c23c4(SlopeSlipWork *work) {
    VecFx32 pos;

    FieldPlayer_GetWPos(work->player, &pos);
    return GetTileClass(GetTileTypeAtPos(Field_GetG3DMapper(work->field), &pos));
}

void func_ov036_021c23e8(SlopeSlipWork *work) {
    u32 interval = func_ov036_021c23b4(work);

    work->frames++;
    if (work->frames % interval == 0) {
        work->dir = (work->dir + 1) % 4;
        ClearActorFlag(work->actor, 8);
        CheckSetActorFaceDir(work->actor, work->dir);
        SetActorFlag(work->actor, 8);
    }
}

void func_ov036_021c242c(SlopeSlipWork *work) {
    FldAct_SetAcmd(work->actor, func_ov036_021c2394(work));
    SetActorFlag(work->actor, 8);
    if (!GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_35))) {
        GFL_SndSEPlay(SEQ_SE_FLD_35);
    }
}

void func_ov036_021c2460(SlopeSlipWork *work) {
    const SlopeSlipTileConfig *config = GetSlopeSlipTileConfig(func_ov036_021c23c4(work));

    if (config != NULL) {
        work->config = config;
    }
}

BOOL func_ov036_021c2474(SlopeSlipWork *work) {
    if (ActorRouteCollCheckOneTileInDir(work->actor, func_ov036_021c23a4(work))) {
        return TRUE;
    }
    return FALSE;
}

void func_ov036_021c2490(SlopeSlipWork *work) {
    ClearActorFlag(work->actor, 8);
    if (work->config != NULL) {
        CheckSetActorFaceDir(work->actor, func_ov036_021c23a4(work));
    }
}
