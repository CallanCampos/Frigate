// The events of the grid player's moves into objects: pushing a Strength boulder, and the TCBs that carry the player
// up and down a waterfall and onto the water to Surf.
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/obj_codes.h"
#include "constants/sound.h"
#include "field/encounter.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_map.h"
#include "field/field_move_tcb.h"
#include "field/field_player.h"
#include "field/field_player_core.h"
#include "field/field_player_grid.h"
#include "field/fieldmap_tcb_camera_zoom.h"
#include "field/player_action.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "system/game_event.h"

#define HEIGHT_TO_GRID(height) ((s16)(((height) >> 4) / FX32_ONE))

typedef struct {
    FieldPlayerGrid *grid;
    u16 dir;
    // Whether the boulder falls into a hole at the end of the push
    u16 fillsHole;
    s16 counter;
    fx32 height;
    VecFx32 offset;
    FieldActor *rock;
    FieldActor *player;
} StrengthRockPushEvent;

typedef struct {
    u16 state;
    s16 counter;
    u16 finished;
    u16 dir;
    u16 onSurfEdge;
    Field *field;
    FieldPlayer *player;
    FieldPlayerGrid *grid;
} SurfTCB;

typedef struct {
    // Set when the player goes up the fall, or else down
    u16 up;
    s16 state;
    u16 dir;
    u16 finished;
    u16 counter;
    u32 unk0C;
    fx32 accel;
    u32 unk14;
    VecFx32 shake;
    VecFx32 dest;
    fx32 stepX;
    fx32 stepY;
    fx32 stepZ;
    FieldPlayer *player;
    Field *field;
    FieldPlayerGrid *grid;
    FieldEffectTask *splash;
} WaterfallTCB;

static FieldActor *FieldPlayerGrid_FindFrontActor(FieldPlayerGrid *grid, u16 dir);
static GameEvent *FieldPlayerGrid_CheckObjContactEventCore(FieldPlayerGrid *grid, u16 dir, u32 flags);
static u32 StrengthRockSpaceCheck(FieldActor *rock, u16 dir, FieldG3DMapper *mapper);
static GameEvent *FieldPlayerGrid_CheckStrengthRockPushEvent(FieldPlayerGrid *grid, u16 dir, u32 contType);
static GameEvent *EventStrengthRockPush_Create(FieldPlayerGrid *grid, u16 dir, FieldActor *rock, BOOL fillsHole);
static GameEventReturnCode EventStrengthRockPush_Callback(GameEvent *event, u32 *state, void *data);
static void FieldSurfTCB_Callback(TCB *tcb, void *work);
static void FieldWaterfallTCB_Callback(TCB *tcb, void *work);
static void Waterfall_SetShakeOffset(FieldActor *actor, const VecFx32 *shake);
static BOOL FieldPlayerGrid_JumpUpStart(WaterfallTCB *waterfall);
static BOOL FieldPlayerGrid_JumpUpApproach(WaterfallTCB *waterfall);
static BOOL FieldPlayerGrid_JumpUpRise(WaterfallTCB *waterfall);
static BOOL FieldPlayerGrid_JumpUpArc(WaterfallTCB *waterfall);
static BOOL FieldPlayerGrid_JumpUpLand(WaterfallTCB *waterfall);
static BOOL FieldPlayerGrid_JumpUpWaitEffect(WaterfallTCB *waterfall);
static void FieldPlayerGrid_UpdateJumpDown(WaterfallTCB *waterfall);

static BOOL (*const WATERFALL_UP_HANDLERS[6])(WaterfallTCB *) = {
    FieldPlayerGrid_JumpUpStart, FieldPlayerGrid_JumpUpApproach, FieldPlayerGrid_JumpUpRise,
    FieldPlayerGrid_JumpUpArc,   FieldPlayerGrid_JumpUpLand,     FieldPlayerGrid_JumpUpWaitEffect,
};

GameEvent *FieldPlayerGrid_CheckObjContactEvent(FieldPlayerGrid *grid, u32 pressedKeys, u32 heldKeys, u32 flags) {
    GameEvent *event;
    u16 dir;
    FieldPlayerCore *core = FieldPlayerGrid_GetPlayerCore(grid);

    FieldPlayerGrid_GetPlayerActor(grid);
    dir = FieldPlayerCore_GetMoveDirByKey(core, heldKeys);
    if (!FieldPlayerGrid_IsAcmdFinished(grid, dir)) {
        return NULL;
    }
    event = FieldPlayerGrid_CheckObjContactEventCore(grid, dir, flags);
    if (event != NULL) {
        return event;
    }
    return NULL;
}

static GameEvent *FieldPlayerGrid_CheckObjContactEventCore(FieldPlayerGrid *grid, u16 dir, u32 flags) {
    GameEvent *event = NULL;
    u32 contType = FieldPlayerGrid_GetMoveDirContType(grid, dir);

    if (flags & OBJ_CONTACT_STRENGTH) {
        event = FieldPlayerGrid_CheckStrengthRockPushEvent(grid, dir, contType);
    }
    return event;
}

// What the tiles in front of a boulder are: free, blocked by one that isn't a hole, partly holes, or all holes
enum {
    STRENGTH_ROCK_SPACE_FREE,
    STRENGTH_ROCK_SPACE_BLOCKED,
    STRENGTH_ROCK_SPACE_PART_HOLE,
    STRENGTH_ROCK_SPACE_HOLE,
};

static u32 StrengthRockSpaceCheck(FieldActor *rock, u16 dir, FieldG3DMapper *mapper) {
    VecFx32 pos;
    VecFx32 tilePos;
    u32 tileType;
    u32 tileClass;
    u8 width;
    u8 j;
    BOOL hasFree;
    BOOL hasHole;
    u8 i;
    u8 height;

    CopyActorWPos(rock, &pos);
    ExpandVecInGridDir(dir, &pos, FX32_CONST(16));
    width = GetActorCollWidth(rock);
    height = GetActorCollHeight(rock);
    hasFree = FALSE;
    hasHole = FALSE;
    for (i = 0; i < height; i++, pos.z -= FX32_CONST(16)) {
        tilePos = pos;
        for (j = 0; j < width; j++, tilePos.x += FX32_CONST(16)) {
            tileType = GetTileTypeAtPos(mapper, &tilePos);
            tileClass = GetTileClass(tileType);
            if (MapTile_BlocksCollision(tileType)) {
                if (!MapTile_IsStrengthHole(tileClass)) {
                    return STRENGTH_ROCK_SPACE_BLOCKED;
                }
                hasHole = TRUE;
            } else {
                hasFree = TRUE;
            }
        }
    }
    if (hasHole == TRUE) {
        if (hasFree == FALSE) {
            return STRENGTH_ROCK_SPACE_HOLE;
        } else {
            return STRENGTH_ROCK_SPACE_PART_HOLE;
        }
    }
    return STRENGTH_ROCK_SPACE_FREE;
}

static GameEvent *FieldPlayerGrid_CheckStrengthRockPushEvent(FieldPlayerGrid *grid, u16 dir, u32 contType) {
    FieldActor *rock;
    u32 space;
    u32 coll;
    BOOL fillsHole;

    if (contType == MOVE_DIR_CONT_MOVE) {
        rock = FieldPlayerGrid_FindFrontActor(grid, dir);
        if (rock != NULL && IsNPCStrengthRock(FldAct_GetObjCode(rock)) == TRUE) {
            space = StrengthRockSpaceCheck(
                rock, dir, Field_GetG3DMapper(FieldPlayerCore_GetField(FieldPlayerGrid_GetPlayerCore(grid))));
            coll = ActorRouteCollCheckOneTileInDir(rock, dir) & ~ACTOR_COLL_WALK_AREA;
            if ((coll & ACTOR_COLL_BLOCKED) &&
                (space == STRENGTH_ROCK_SPACE_PART_HOLE || space == STRENGTH_ROCK_SPACE_HOLE)) {
                coll &= ~ACTOR_COLL_BLOCKED;
            }
            if (coll == 0) {
                fillsHole = FALSE;
                if (space == STRENGTH_ROCK_SPACE_HOLE) {
                    fillsHole = TRUE;
                }
                return EventStrengthRockPush_Create(grid, dir, rock, fillsHole);
            }
        }
    }
    return NULL;
}

static GameEvent *EventStrengthRockPush_Create(FieldPlayerGrid *grid, u16 dir, FieldActor *rock, BOOL fillsHole) {
    FieldPlayerCore *core = FieldPlayerGrid_GetPlayerCore(grid);
    GameEvent *event = GameEvent_Create(FieldPlayerCore_GetGameSystem(core), NULL, EventStrengthRockPush_Callback,
                                        sizeof(StrengthRockPushEvent));
    StrengthRockPushEvent *push = GameEvent_GetData(event);

    MI_CpuFill8(push, 0, sizeof(StrengthRockPushEvent));
    push->grid = grid;
    push->dir = dir;
    push->fillsHole = fillsHole;
    push->rock = rock;
    push->player = FieldPlayerCore_GetActor(core);
    return event;
}

// The steps of a boulder's push: push it a tile, then let it fall into the hole if it fills one, and shake it until it
// settles there
enum {
    STRENGTH_PUSH_STATE_START,
    STRENGTH_PUSH_STATE_WAIT_PUSH,
    STRENGTH_PUSH_STATE_FALL,
    STRENGTH_PUSH_STATE_SETTLE,
    STRENGTH_PUSH_STATE_END,
};

static GameEventReturnCode EventStrengthRockPush_Callback(GameEvent *event, u32 *state, void *data) {
    StrengthRockPushEvent *push = data;
    VecFx32 holePos;
    VecFx32 fallPos;
    VecFx32 settlePos;

    switch (*state) {
    case STRENGTH_PUSH_STATE_START:
        if (!IsAllActorAcmdFinished(push->rock)) {
            break;
        }
        if (!IsAllActorAcmdFinished(push->player) && !FieldPlayerGrid_StopTurnOrBlocked(push->grid)) {
            break;
        }
        FldAct_SetAcmd(push->rock, GetAcmdForDir(push->dir, ACMD_WALK_16F));
        FldAct_SetAcmd(push->player, GetAcmdForDir(push->dir, ACMD_STAY_WALK_8F));
        GFL_SndSEPlay(SEQ_SE_FLD_03);
        (*state)++;
        break;
    case STRENGTH_PUSH_STATE_WAIT_PUSH:
        if (!FldAct_CheckAcmdEnd(push->rock)) {
            break;
        }
        if (!FldAct_CheckAcmdEnd(push->player)) {
            break;
        }
        if (push->fillsHole == FALSE) {
            *state = STRENGTH_PUSH_STATE_END;
            break;
        }
        CopyActorWPos(push->rock, &holePos);
        GetHeightFromMap(push->rock, &holePos, &push->height);
        if (FldAct_GetObjCode(push->rock) == OBJ_CODE_STRENGTH_ROCK) {
            push->height -= FX32_CONST(14);
        } else {
            push->height -= FX32_CONST(26);
        }
        FldAct_SetHeightCalcDisabled(push->rock, TRUE);
        SetActorCollisionEnabled(push->rock, FALSE);
        GFL_SndSEPlay(SEQ_SE_FLD_04);
        (*state)++;
        break;
    case STRENGTH_PUSH_STATE_FALL:
        CopyActorWPos(push->rock, &fallPos);
        if (push->height < fallPos.y) {
            fallPos.y -= FX32_CONST(4);
            if (push->height > fallPos.y) {
                fallPos.y = push->height;
            }
        } else if (push->height > fallPos.y) {
            fallPos.y += FX32_CONST(4);
            if (push->height < fallPos.y) {
                fallPos.y = push->height;
            }
        }
        SetActorWPosValue(push->rock, &fallPos);
        if (push->height == fallPos.y) {
            push->offset.y = FX32_CONST(2);
            (*state)++;
        }
        break;
    case STRENGTH_PUSH_STATE_SETTLE:
        FldAct_SetOuterDrawOffset(push->rock, &push->offset);
        push->offset.y = -push->offset.y;
        push->counter++;
        if (push->counter & 4) {
            if (push->offset.y < 0) {
                push->offset.y += FX32_CONST(1);
            } else if (push->offset.y > 0) {
                push->offset.y -= FX32_CONST(1);
            }
        }
        if (push->offset.y == 0) {
            CopyActorWPos(push->rock, &settlePos);
            SetActorWPosAll(push->rock, &settlePos, push->dir);
            FldAct_SetOuterDrawOffset(push->rock, &push->offset);
            SaveActorStrengthRockPosition(push->rock);
            (*state)++;
        }
        break;
    case STRENGTH_PUSH_STATE_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

TCB *FieldSurfTCB_Create(FieldPlayer *player, u32 direction, u32 tileType, HeapID heapId) {
    SurfTCB *surf = GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(SurfTCB), TRUE, "field_player_grid_event.c", 486);

    surf->field = FieldPlayer_GetField(player);
    surf->dir = direction;
    surf->player = player;
    surf->grid = FieldPlayer_GetGrid(player);
    if (MapTile_IsSurfEdge(GetTileClass(tileType)) == TRUE) {
        surf->onSurfEdge = TRUE;
    }
    return GFL_TCBMgrAddTask(Field_GetTCBMgr(surf->field), FieldSurfTCB_Callback, surf, 0);
}

BOOL FieldSurfTCB_CheckEnd(TCB *tcb) {
    SurfTCB *surf = GFL_TCBGetData(tcb);

    return surf->finished;
}

void FieldSurfTCB_Free(TCB *tcb) {
    GFL_HeapFree(GFL_TCBGetData(tcb));
    GFL_TCBRemove(tcb);
}

// The steps of the surf: put the Pokémon on the water, wait, jump onto it, change to the surfing form, and stay done
enum {
    SURF_STATE_START,
    SURF_STATE_WAIT,
    SURF_STATE_JUMP,
    SURF_STATE_WAIT_JUMP,
    SURF_STATE_FORM_CHANGE,
    SURF_STATE_END,
};

static void FieldSurfTCB_Callback(TCB *tcb, void *work) {
    SurfTCB *surf = work;
    FieldActor *actor = FieldPlayer_GetActor(surf->player);
    u16 dir;
    u16 acmd;
    fx32 height;
    fx32 distance;
    VecFx32 pos;
    s16 gridPos[3];
    s16 gx;
    s16 gz;
    FieldEffectTask *task;
    EncountSystem *encounter;

    switch (surf->state) {
    case SURF_STATE_START:
        dir = GetActorFaceDir(actor);
        gx = GetGPosX(actor);
        gz = GetGPosZ(actor);
        CopyActorWPos(actor, &pos);
        ConvGXZToVector(gx, gz, &pos);
        distance = FX32_CONST(16);
        if (surf->onSurfEdge == TRUE) {
            distance *= 2;
        }
        ExpandVecInGridDir(dir, &pos, distance);
        height = 0;
        GetHeightFromMap(actor, &pos, &height);
        pos.y = height;
        task = FldeffSurfPokemon_Start(Field_GetFieldEffects(surf->field), dir, &pos, actor, SURF_POKEMON_FOLLOW_NONE);
        FieldPlayer_SetRideEffect(surf->player, task);
        encounter = Field_GetEncountSystem(surf->field);
        gridPos[0] = HEIGHT_TO_GRID(pos.x);
        gridPos[1] = HEIGHT_TO_GRID(pos.y);
        gridPos[2] = HEIGHT_TO_GRID(pos.z);
        if (EncountState_CheckSpecialEncountPos(encounter, gridPos)) {
            EffectEncount_SetEffectHidden(encounter, TRUE);
        }
        surf->state++;
        break;
    case SURF_STATE_WAIT:
        surf->counter++;
        if (surf->counter > 15) {
            surf->state++;
        }
        break;
    case SURF_STATE_JUMP:
        dir = GetActorFaceDir(actor);
        acmd = GetAcmdForDir(dir, ACMD_JUMP_1);
        if (surf->onSurfEdge == TRUE) {
            acmd = GetAcmdForDir(dir, ACMD_JUMP_2);
        }
        FldAct_SetAcmd(actor, acmd);
        surf->state++;
        break;
    case SURF_STATE_WAIT_JUMP:
        if (!FldAct_CheckAcmdEnd(actor)) {
            break;
        }
        FldAct_EndAcmd(actor);
        FldeffSurfPokemon_SetFollowMode(FieldPlayer_GetRideEffect(surf->player), SURF_POKEMON_FOLLOW_BOB);
        FieldPlayer_SetSpecialSeq(surf->player, 4);
        surf->state++;
        // fall through
    case SURF_STATE_FORM_CHANGE:
        if (FieldPlayer_StepFormChange(surf->player) == TRUE) {
            FieldPlayer_SetGridHopLanded(surf->player);
            surf->finished = TRUE;
            surf->state++;
        }
        break;
    case SURF_STATE_END:
        break;
    }
}

TCB *FieldWaterfallTCB_Create(FieldPlayer *player, u32 direction, u32 param, HeapID heapId) {
    WaterfallTCB *waterfall =
        GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(WaterfallTCB), TRUE, "field_player_grid_event.c", 691);
    FieldActor *actor;
    VecFx32 pos;
    VecFx32 ahead;
    fx32 height;

    waterfall->dir = direction;
    waterfall->player = player;
    waterfall->field = FieldPlayer_GetField(player);
    actor = FieldPlayer_GetActor(player);
    CopyActorWPos(actor, &pos);
    ahead = pos;
    ExpandVecInGridDir(waterfall->dir, &ahead, FX32_CONST(32));
    GetHeightFromMap(actor, &ahead, &height);
    if (height > pos.y) {
        waterfall->up = TRUE;
    }
    waterfall->grid = FieldPlayer_GetGrid(waterfall->player);
    return GFL_TCBMgrAddTask(Field_GetTCBMgr(waterfall->field), FieldWaterfallTCB_Callback, waterfall, 0);
}

BOOL FieldWaterfallTCB_CheckEnd(TCB *tcb) {
    WaterfallTCB *waterfall = GFL_TCBGetData(tcb);

    return waterfall->finished;
}

void FieldWaterfallTCB_Free(TCB *tcb) {
    GFL_HeapFree(GFL_TCBGetData(tcb));
    GFL_TCBRemove(tcb);
}

static void Waterfall_SetShakeOffset(FieldActor *actor, const VecFx32 *shake) {
    VecFx32 offset = *shake;

    offset.y += FX32_CONST(1);
    offset.z += FX32_CONST(5);
    FldAct_SetOuterDrawOffset(actor, &offset);
}

static BOOL FieldPlayerGrid_JumpUpStart(WaterfallTCB *waterfall) {
    FieldActor *actor = FieldPlayer_GetActor(waterfall->player);
    VecFx32 pos;
    VecFx32 posA;
    VecFx32 posB;
    VecFx32 posC;
    FieldEffectTask *task;
    FieldEffects *effects;
    u32 kind;
    u32 effectId;

    FldAct_SetHeightCalcDisabled(actor, TRUE);
    CopyActorWPos(actor, &pos);
    posA = pos;
    posB = posA;
    posC = posA;
    ExpandVecInGridDir(waterfall->dir, &posB, FX32_CONST(16));
    ExpandVecInGridDir(waterfall->dir, &posC, FX32_CONST(32));
    ConvGXZToVector(HEIGHT_TO_GRID(posB.x), HEIGHT_TO_GRID(posB.z), &posB);
    ConvGXZToVector(HEIGHT_TO_GRID(posC.x), HEIGHT_TO_GRID(posC.z), &waterfall->dest);
    waterfall->dest = posC;
    GetHeightFromMap(actor, &posC, &waterfall->dest.y);
    waterfall->stepX = (posB.x - pos.x) / 2 / 16;
    waterfall->stepZ = (posB.z - pos.z) / 2 / 16;
    waterfall->stepY = FX32_CONST(6);
    waterfall->shake.y = FX32_ONE;
    task = FieldPlayer_GetRideEffect(waterfall->player);
    FldeffSurfPokemon_SetFollowMode(task, SURF_POKEMON_FOLLOW_KEEP_OFFSET);
    kind = 1;
    if (waterfall->dir == DIR_LEFT || waterfall->dir == DIR_RIGHT) {
        kind = 3;
    }
    effects = Field_GetFieldEffects(waterfall->field);
    effectId = FLDEFF_SURF_POKEMON_SPLASH;
    FieldEffects_Load(effects, &effectId, 1);
    waterfall->splash = FldeffSurfPokemonSplash_StartOnEffect(effects, kind, task);
    FieldCameraZoomTCB_Create(waterfall->field, 16, FX32_CONST(32));
    GFL_SndSEPlay(SEQ_SE_FLD_81);
    waterfall->state++;
    return TRUE;
}

static BOOL FieldPlayerGrid_JumpUpApproach(WaterfallTCB *waterfall) {
    FieldActor *actor = FieldPlayer_GetActor(waterfall->player);
    VecFx32 pos;
    FieldEffectTask *task;
    u32 kind;

    CopyActorWPos(actor, &pos);
    pos.x += waterfall->stepX;
    pos.z += waterfall->stepZ;
    SetActorWPosValue(actor, &pos);
    waterfall->shake.y = -waterfall->shake.y;
    waterfall->counter++;
    if (waterfall->counter >= 16) {
        waterfall->counter = 0;
        waterfall->shake.y = 0;
        waterfall->state++;
        FieldEffects_DeleteObject(waterfall->splash);
        task = FieldPlayer_GetRideEffect(waterfall->player);
        kind = 2;
        if (waterfall->dir == DIR_LEFT || waterfall->dir == DIR_RIGHT) {
            kind = 2;
        }
        waterfall->splash = FldeffSurfPokemonSplash_StartOnEffect(Field_GetFieldEffects(waterfall->field), kind, task);
    }
    Waterfall_SetShakeOffset(actor, &waterfall->shake);
    return FALSE;
}

static BOOL FieldPlayerGrid_JumpUpRise(WaterfallTCB *waterfall) {
    FieldActor *actor = FieldPlayer_GetActor(waterfall->player);
    VecFx32 pos;

    CopyActorWPos(actor, &pos);
    pos.y += waterfall->stepY;
    if (pos.y >= waterfall->dest.y) {
        pos.y = waterfall->dest.y;
        waterfall->state++;
        waterfall->stepX = (waterfall->dest.x - pos.x) / 16;
        waterfall->stepZ = (waterfall->dest.z - pos.z) / 16;
        waterfall->accel = -0x8000;
        waterfall->stepY = FX32_CONST(6);
        waterfall->accel = waterfall->accel / 11;
        FieldCameraZoomTCB_Create(waterfall->field, 16, -0x8000 * 4);
        FieldEffects_DeleteObject(waterfall->splash);
    }
    SetActorWPosValue(actor, &pos);
    return FALSE;
}

static BOOL FieldPlayerGrid_JumpUpArc(WaterfallTCB *waterfall) {
    FieldActor *actor = FieldPlayer_GetActor(waterfall->player);
    VecFx32 pos;

    CopyActorWPos(actor, &pos);
    if (pos.x < waterfall->dest.x) {
        pos.x += waterfall->stepX;
        if (pos.x > waterfall->dest.x) {
            pos.x = waterfall->dest.x;
        }
    } else {
        pos.x += waterfall->stepX;
        if (pos.x < waterfall->dest.x) {
            pos.x = waterfall->dest.x;
        }
    }
    if (pos.z < waterfall->dest.z) {
        pos.z += waterfall->stepZ;
        if (pos.z > waterfall->dest.z) {
            pos.z = waterfall->dest.z;
        }
    } else {
        pos.z += waterfall->stepZ;
        if (pos.z < waterfall->dest.z) {
            pos.z = waterfall->dest.z;
        }
    }
    pos.y += waterfall->stepY;
    waterfall->stepY += waterfall->accel;
    waterfall->counter++;
    if (waterfall->counter >= 16) {
        pos = waterfall->dest;
        waterfall->shake.x = 0;
        waterfall->shake.y = 0;
        waterfall->shake.z = 0;
        waterfall->state++;
    }
    SetActorWPosValue(actor, &pos);
    Waterfall_SetShakeOffset(actor, &waterfall->shake);
    return FALSE;
}

static BOOL FieldPlayerGrid_JumpUpLand(WaterfallTCB *waterfall) {
    FieldActor *actor = FieldPlayer_GetActor(waterfall->player);
    FieldEffectTask *task;

    FldAct_SetHeightCalcDisabled(actor, FALSE);
    SetActorWPosAll(actor, &waterfall->dest, waterfall->dir);
    task = FieldPlayer_GetRideEffect(waterfall->player);
    FldeffSurfPokemon_SetFollowMode(task, SURF_POKEMON_FOLLOW_BOB);
    waterfall->splash = FldeffSurfPokemonSplash_StartOnEffect(Field_GetFieldEffects(waterfall->field), 0, task);
    GFL_SndSEPlay(SEQ_SE_FLD_83);
    waterfall->state++;
    return FALSE;
}

static BOOL FieldPlayerGrid_JumpUpWaitEffect(WaterfallTCB *waterfall) {
    FieldEffects *effects;

    if (waterfall->splash != NULL && FldeffSurfPokemonSplash_IsFinished(waterfall->splash) == TRUE) {
        effects = Field_GetFieldEffects(waterfall->field);
        FieldEffects_DeleteObject(waterfall->splash);
        FieldEffects_FreeEffect(effects, FLDEFF_SURF_POKEMON_SPLASH);
        FieldPlayer_SetGridHopLanded(waterfall->player);
        waterfall->splash = NULL;
        waterfall->finished = TRUE;
    }
    return FALSE;
}

// The steps of going down a waterfall: start the splash, move to its edge, drop to the foot and move off onto the water
enum {
    WATERFALL_DOWN_START,
    WATERFALL_DOWN_APPROACH,
    WATERFALL_DOWN_DROP,
    WATERFALL_DOWN_LAND,
};

static void FieldPlayerGrid_UpdateJumpDown(WaterfallTCB *waterfall) {
    FieldActor *actor = FieldPlayer_GetActor(waterfall->player);
    VecFx32 pos;
    VecFx32 startPos;
    VecFx32 posB;
    VecFx32 posC;
    VecFx32 posA;
    FieldEffectTask *task;
    FieldEffects *effects;
    u32 kind;
    u32 effectId;

    switch (waterfall->state) {
    case WATERFALL_DOWN_START:
        FldAct_SetHeightCalcDisabled(actor, TRUE);
        CopyActorWPos(actor, &startPos);
        posA = startPos;
        posB = posA;
        posC = posA;
        ExpandVecInGridDir(waterfall->dir, &posB, FX32_CONST(16));
        ExpandVecInGridDir(waterfall->dir, &posC, FX32_CONST(32));
        ConvGXZToVector(HEIGHT_TO_GRID(posB.x), HEIGHT_TO_GRID(posB.z), &posB);
        ExpandVecInGridDir(waterfall->dir, &posB, FX32_CONST(4));
        ConvGXZToVector(HEIGHT_TO_GRID(posC.x), HEIGHT_TO_GRID(posC.z), &posC);
        waterfall->dest = posC;
        GetHeightFromMap(actor, &posC, &waterfall->dest.y);
        waterfall->stepX = (posB.x - startPos.x) / 16;
        waterfall->stepZ = (posB.z - startPos.z) / 16;
        waterfall->stepY = -0x6000;
        waterfall->shake.y = FX32_ONE;
        task = FieldPlayer_GetRideEffect(waterfall->player);
        FldeffSurfPokemon_SetFollowMode(task, SURF_POKEMON_FOLLOW_KEEP_OFFSET);
        FldeffSurfPokemon_SetWakeEnabled(task, FALSE);
        kind = 1;
        if (waterfall->dir == DIR_LEFT || waterfall->dir == DIR_RIGHT) {
            kind = 3;
        }
        effects = Field_GetFieldEffects(waterfall->field);
        effectId = FLDEFF_SURF_POKEMON_SPLASH;
        FieldEffects_Load(effects, &effectId, 1);
        waterfall->splash = FldeffSurfPokemonSplash_StartOnEffect(effects, kind, task);
        FieldCameraZoomTCB_Create(waterfall->field, 16, FX32_CONST(32));
        GFL_SndSEPlay(SEQ_SE_FLD_82);
        waterfall->state++;
        // fall through
    case WATERFALL_DOWN_APPROACH:
        CopyActorWPos(actor, &pos);
        pos.x += waterfall->stepX;
        pos.z += waterfall->stepZ;
        SetActorWPosValue(actor, &pos);
        waterfall->shake.y = -waterfall->shake.y;
        waterfall->counter++;
        if (waterfall->counter >= 16) {
            waterfall->counter = 0;
            waterfall->shake.y = 0;
            waterfall->state++;
            FieldEffects_DeleteObject(waterfall->splash);
            task = FieldPlayer_GetRideEffect(waterfall->player);
            kind = 2;
            if (waterfall->dir == DIR_LEFT || waterfall->dir == DIR_RIGHT) {
                kind = 2;
            }
            waterfall->splash =
                FldeffSurfPokemonSplash_StartOnEffect(Field_GetFieldEffects(waterfall->field), kind, task);
        }
        Waterfall_SetShakeOffset(actor, &waterfall->shake);
        break;
    case WATERFALL_DOWN_DROP:
        CopyActorWPos(actor, &pos);
        pos.y += waterfall->stepY;
        if (pos.y <= waterfall->dest.y) {
            pos.y = waterfall->dest.y;
            waterfall->state++;
            waterfall->stepX = (waterfall->dest.x - pos.x) / 16;
            waterfall->stepZ = (waterfall->dest.z - pos.z) / 16;
            FieldCameraZoomTCB_Create(waterfall->field, 16, -FX32_CONST(32));
            FieldEffects_DeleteObject(waterfall->splash);
            task = FieldPlayer_GetRideEffect(waterfall->player);
            FldeffSurfPokemon_SetFollowMode(task, SURF_POKEMON_FOLLOW_BOB);
            waterfall->splash = FldeffSurfPokemonSplash_StartOnEffect(Field_GetFieldEffects(waterfall->field), 0, task);
        }
        SetActorWPosValue(actor, &pos);
        break;
    case WATERFALL_DOWN_LAND:
        CopyActorWPos(actor, &pos);
        pos.x += waterfall->stepX;
        pos.z += waterfall->stepZ;
        SetActorWPosValue(actor, &pos);
        waterfall->counter++;
        if (waterfall->counter >= 16) {
            effects = Field_GetFieldEffects(waterfall->field);
            FldAct_SetHeightCalcDisabled(actor, FALSE);
            SetActorWPosAll(actor, &waterfall->dest, waterfall->dir);
            FieldEffects_DeleteObject(waterfall->splash);
            FieldEffects_FreeEffect(effects, FLDEFF_SURF_POKEMON_SPLASH);
            FieldPlayer_SetGridHopLanded(waterfall->player);
            FldeffSurfPokemon_SetWakeEnabled(FieldPlayer_GetRideEffect(waterfall->player), TRUE);
            waterfall->splash = NULL;
            waterfall->finished = TRUE;
            GFL_SndSEPlay(SEQ_SE_FLD_83);
            waterfall->state++;
        }
        break;
    }
}

static void FieldWaterfallTCB_Callback(TCB *tcb, void *work) {
    WaterfallTCB *waterfall = work;

    if (waterfall->up != 0) {
        do {
        } while (WATERFALL_UP_HANDLERS[waterfall->state](waterfall) == TRUE);
    } else {
        FieldPlayerGrid_UpdateJumpDown(waterfall);
    }
}

static FieldActor *FieldPlayerGrid_FindFrontActor(FieldPlayerGrid *grid, u16 dir) {
    FieldActor *player = FieldPlayerGrid_GetPlayerActor(grid);
    MMSys *system = GetActorMModelSystem(player);
    int x = GetGPosX(player) + GetDirectionVectorCompX(dir);
    int z = GetGPosZ(player) + GetDirectionVectorCompZ(dir);

    return FindActorByGPos(system, x, z, GetActorPosY(player), 1, FALSE);
}
