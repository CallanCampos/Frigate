// The events of the grid player's moves into objects: pushing a Strength boulder, and the TCBs that carry the player
// up and down a waterfall and onto the water to Surf.
#include "types.h"
#include "constants/sound.h"
#include "field/encounter.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_effect.h"
#include "field/field_map.h"
#include "field/field_move_tcb.h"
#include "field/field_player.h"
#include "field/field_player_core.h"
#include "field/field_player_grid.h"
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
static void func_ov036_021bae58(FieldActor *actor, const VecFx32 *shake);
static BOOL func_ov036_021bae8c(WaterfallTCB *waterfall);
static BOOL func_ov036_021bafe0(WaterfallTCB *waterfall);
static BOOL func_ov036_021bb06c(WaterfallTCB *waterfall);
static BOOL func_ov036_021bb0f0(WaterfallTCB *waterfall);
static BOOL func_ov036_021bb190(WaterfallTCB *waterfall);
static BOOL func_ov036_021bb1e4(WaterfallTCB *waterfall);
static void func_ov036_021bb220(WaterfallTCB *waterfall);

static BOOL (*const WATERFALL_UP_HANDLERS[6])(WaterfallTCB *) = {
    func_ov036_021bae8c, func_ov036_021bafe0, func_ov036_021bb06c,
    func_ov036_021bb0f0, func_ov036_021bb190, func_ov036_021bb1e4,
};

GameEvent *FieldPlayerGrid_CheckObjContactEvent(FieldPlayerGrid *grid, u32 pressedKeys, u32 heldKeys, u32 flags) {
    GameEvent *event;
    u16 dir;
    FieldPlayerCore *core = FieldPlayerGrid_GetPlayerCore(grid);

    FieldPlayerGrid_GetPlayerActor(grid);
    dir = FieldPlayerCore_GetMoveDirByKey(core, heldKeys);
    if (!((BOOL (*)(FieldPlayerGrid *, u16))FieldPlayerGrid_IsAcmdFinished)(grid, dir)) {
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

    if (flags & 1) {
        event = FieldPlayerGrid_CheckStrengthRockPushEvent(grid, dir, contType);
    }
    return event;
}

// Whether the tiles in front of a boulder let it move: 1 if one blocks it, 2 if it can move, and 3 if it also fills
// a hole
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
                    return 1;
                }
                hasHole = TRUE;
            } else {
                hasFree = TRUE;
            }
        }
    }
    if (hasHole == TRUE) {
        if (hasFree == FALSE) {
            return 3;
        } else {
            return 2;
        }
    }
    return 0;
}

static GameEvent *FieldPlayerGrid_CheckStrengthRockPushEvent(FieldPlayerGrid *grid, u16 dir, u32 contType) {
    FieldActor *rock;
    u32 space;
    u32 coll;
    BOOL fillsHole;

    if (contType == 1) {
        rock = FieldPlayerGrid_FindFrontActor(grid, dir);
        if (rock != NULL && IsNPCStrengthRock(FldAct_GetObjCode(rock)) == TRUE) {
            space = StrengthRockSpaceCheck(
                rock, dir, Field_GetG3DMapper(FieldPlayerCore_GetField(FieldPlayerGrid_GetPlayerCore(grid))));
            coll = ActorRouteCollCheckOneTileInDir(rock, dir) & ~1;
            if ((coll & 2) && space - 2 <= 1) {
                coll &= ~2;
            }
            if (coll == 0) {
                fillsHole = FALSE;
                if (space == 3) {
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

static GameEventReturnCode EventStrengthRockPush_Callback(GameEvent *event, u32 *state, void *data) {
    StrengthRockPushEvent *push = data;
    VecFx32 holePos;
    VecFx32 fallPos;
    VecFx32 settlePos;

    switch (*state) {
    case 0:
        if (!IsAllActorAcmdFinished(push->rock)) {
            break;
        }
        if (!IsAllActorAcmdFinished(push->player) && !func_ov036_0219ca30(push->grid)) {
            break;
        }
        FldAct_SetAcmd(push->rock, GetAcmdForDir(push->dir, 8));
        FldAct_SetAcmd(push->player, GetAcmdForDir(push->dir, 0x20));
        GFL_SndSEPlay(1652);
        (*state)++;
        break;
    case 1:
        if (!func_ov012_02166ecc(push->rock)) {
            break;
        }
        if (!func_ov012_02166ecc(push->player)) {
            break;
        }
        if (push->fillsHole == 0) {
            *state = 4;
            break;
        }
        CopyActorWPos(push->rock, &holePos);
        GetHeightFromMap(push->rock, &holePos, &push->height);
        if (FldAct_GetObjCode(push->rock) == 0x6d) {
            push->height -= FX32_CONST(14);
        } else {
            push->height -= FX32_CONST(26);
        }
        func_ov012_0216763c(push->rock, TRUE);
        func_ov012_02167580(push->rock, FALSE);
        GFL_SndSEPlay(1653);
        (*state)++;
        break;
    case 2:
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
    case 3:
        func_ov012_0216734c(push->rock, &push->offset);
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
            func_ov012_0216734c(push->rock, &push->offset);
            func_ov012_021681b0(push->rock);
            (*state)++;
        }
        break;
    case 4:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void *FieldSurfTCB_Create(FieldPlayer *player, u32 direction, u32 tileType, HeapID heapId) {
    SurfTCB *surf =
        GFL_HeapAllocate((heapId & 0x7fff) | 0x8000, sizeof(SurfTCB), TRUE, "field_player_grid_event.c", 486);

    surf->field = FieldPlayer_GetField(player);
    surf->dir = direction;
    surf->player = player;
    surf->grid = func_ov036_0219ad48(player);
    if (MapTile_IsSurfEdge(GetTileClass(tileType)) == TRUE) {
        surf->onSurfEdge = TRUE;
    }
    return GFL_TCBMgrAddTask(Field_GetTCBMgr(surf->field), FieldSurfTCB_Callback, surf, 0);
}

BOOL FieldSurfTCB_CheckEnd(void *tcb) {
    SurfTCB *surf = GFL_TCBGetData(tcb);

    return surf->finished;
}

void FieldSurfTCB_Free(void *tcb) {
    GFL_HeapFree(GFL_TCBGetData(tcb));
    GFL_TCBRemove(tcb);
}

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
    case 0:
        dir = GetActorFaceDir(actor);
        gx = GetGPosX(actor);
        gz = GetGPosZ(actor);
        CopyActorWPos(actor, &pos);
        ConvGXZToVector(gx, gz, &pos);
        distance = FX32_CONST(16);
        if (surf->onSurfEdge == 1) {
            distance *= 2;
        }
        ExpandVecInGridDir(dir, &pos, distance);
        height = 0;
        GetHeightFromMap(actor, &pos, &height);
        pos.y = height;
        task = func_ov036_021a4484(Field_GetFieldEffects(surf->field), dir, &pos, actor, 0);
        func_ov036_0219a5b8(surf->player, (TCB *)task);
        encounter = Field_GetEncountSystem(surf->field);
        gridPos[0] = HEIGHT_TO_GRID(pos.x);
        gridPos[1] = HEIGHT_TO_GRID(pos.y);
        gridPos[2] = HEIGHT_TO_GRID(pos.z);
        if (EncountState_CheckSpecialEncountPos(encounter, gridPos)) {
            func_ov036_021a23c4(encounter, 1);
        }
        surf->state++;
        break;
    case 1:
        surf->counter++;
        if (surf->counter > 15) {
            surf->state++;
        }
        break;
    case 2:
        dir = GetActorFaceDir(actor);
        acmd = GetAcmdForDir(dir, 0x34);
        if (surf->onSurfEdge == 1) {
            acmd = GetAcmdForDir(dir, 0x38);
        }
        FldAct_SetAcmd(actor, acmd);
        surf->state++;
        break;
    case 3:
        if (!func_ov012_02166ecc(actor)) {
            break;
        }
        func_ov012_02166ef8(actor);
        func_ov036_021a4504((FieldEffectTask *)func_ov036_0219a5c4(surf->player), 1);
        FieldPlayer_SetSpecialSeq(surf->player, 4);
        surf->state++;
        // fall through
    case 4:
        if (func_ov036_0219a580(surf->player) == TRUE) {
            func_ov036_0219ac0c(surf->player);
            surf->finished = TRUE;
            surf->state++;
        }
        break;
    case 5:
        break;
    }
}

void *FieldWaterfallTCB_Create(FieldPlayer *player, u32 direction, u32 param, HeapID heapId) {
    WaterfallTCB *waterfall =
        GFL_HeapAllocate((heapId & 0x7fff) | 0x8000, sizeof(WaterfallTCB), TRUE, "field_player_grid_event.c", 691);
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
    waterfall->grid = func_ov036_0219ad48(waterfall->player);
    return GFL_TCBMgrAddTask(Field_GetTCBMgr(waterfall->field), FieldWaterfallTCB_Callback, waterfall, 0);
}

BOOL FieldWaterfallTCB_CheckEnd(void *tcb) {
    WaterfallTCB *waterfall = GFL_TCBGetData(tcb);

    return waterfall->finished;
}

void FieldWaterfallTCB_Free(void *tcb) {
    GFL_HeapFree(GFL_TCBGetData(tcb));
    GFL_TCBRemove(tcb);
}

static void func_ov036_021bae58(FieldActor *actor, const VecFx32 *shake) {
    VecFx32 offset = *shake;

    offset.y += FX32_CONST(1);
    offset.z += FX32_CONST(5);
    func_ov012_0216734c(actor, &offset);
}

static BOOL func_ov036_021bae8c(WaterfallTCB *waterfall) {
    FieldActor *actor = FieldPlayer_GetActor(waterfall->player);
    BOOL one = TRUE;
    VecFx32 pos;
    VecFx32 posA;
    VecFx32 posB;
    VecFx32 posC;
    FieldEffectTask *task;
    FieldEffects *effects;
    u32 kind;
    u32 effectId;

    func_ov012_0216763c(actor, TRUE);
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
    waterfall->shake.y = one << 12;
    task = (FieldEffectTask *)func_ov036_0219a5c4(waterfall->player);
    func_ov036_021a4504(task, 2);
    kind = 1;
    if ((u16)(waterfall->dir + 0xfffe) <= 1) {
        kind = 3;
    }
    effects = Field_GetFieldEffects(waterfall->field);
    effectId = 8;
    FieldEffects_Load(effects, &effectId, 1);
    waterfall->splash = func_ov036_021a4eb0(effects, kind, task);
    FieldCameraZoomTCB_Create(waterfall->field, 16, FX32_CONST(32));
    GFL_SndSEPlay(1760);
    waterfall->state++;
    return TRUE;
}

static BOOL func_ov036_021bafe0(WaterfallTCB *waterfall) {
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
        func_ov036_021a3a70(waterfall->splash);
        task = (FieldEffectTask *)func_ov036_0219a5c4(waterfall->player);
        kind = 2;
        if ((u16)(waterfall->dir + 0xfffe) <= 1) {
            kind = 2;
        }
        waterfall->splash = func_ov036_021a4eb0(Field_GetFieldEffects(waterfall->field), kind, task);
    }
    func_ov036_021bae58(actor, &waterfall->shake);
    return FALSE;
}

static BOOL func_ov036_021bb06c(WaterfallTCB *waterfall) {
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
        func_ov036_021a3a70(waterfall->splash);
    }
    SetActorWPosValue(actor, &pos);
    return FALSE;
}

static BOOL func_ov036_021bb0f0(WaterfallTCB *waterfall) {
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
    func_ov036_021bae58(actor, &waterfall->shake);
    return FALSE;
}

static BOOL func_ov036_021bb190(WaterfallTCB *waterfall) {
    FieldActor *actor = FieldPlayer_GetActor(waterfall->player);
    FieldEffectTask *task;

    func_ov012_0216763c(actor, FALSE);
    SetActorWPosAll(actor, &waterfall->dest, waterfall->dir);
    task = (FieldEffectTask *)func_ov036_0219a5c4(waterfall->player);
    func_ov036_021a4504(task, 1);
    waterfall->splash = func_ov036_021a4eb0(Field_GetFieldEffects(waterfall->field), 0, task);
    GFL_SndSEPlay(1762);
    waterfall->state++;
    return FALSE;
}

static BOOL func_ov036_021bb1e4(WaterfallTCB *waterfall) {
    FieldEffects *effects;

    if (waterfall->splash != NULL && func_ov036_021a4f18(waterfall->splash) == TRUE) {
        effects = Field_GetFieldEffects(waterfall->field);
        func_ov036_021a3a70(waterfall->splash);
        FieldEffects_FreeEffect(effects, 8);
        func_ov036_0219ac0c(waterfall->player);
        waterfall->splash = NULL;
        waterfall->finished = TRUE;
    }
    return FALSE;
}

static void func_ov036_021bb220(WaterfallTCB *waterfall) {
    FieldActor *actor = FieldPlayer_GetActor(waterfall->player);
    BOOL one;
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
    case 0:
        one = TRUE;
        func_ov012_0216763c(actor, TRUE);
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
        waterfall->shake.y = one << 12;
        task = (FieldEffectTask *)func_ov036_0219a5c4(waterfall->player);
        func_ov036_021a4504(task, 2);
        func_ov036_021a4514(task, FALSE);
        kind = 1;
        if ((u16)(waterfall->dir + 0xfffe) <= 1) {
            kind = 3;
        }
        effects = Field_GetFieldEffects(waterfall->field);
        effectId = 8;
        FieldEffects_Load(effects, &effectId, 1);
        waterfall->splash = func_ov036_021a4eb0(effects, kind, task);
        FieldCameraZoomTCB_Create(waterfall->field, 16, FX32_CONST(32));
        GFL_SndSEPlay(1761);
        waterfall->state++;
        // fall through
    case 1:
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
            func_ov036_021a3a70(waterfall->splash);
            task = (FieldEffectTask *)func_ov036_0219a5c4(waterfall->player);
            kind = 2;
            if ((u16)(waterfall->dir + 0xfffe) <= 1) {
                kind = 2;
            }
            waterfall->splash = func_ov036_021a4eb0(Field_GetFieldEffects(waterfall->field), kind, task);
        }
        func_ov036_021bae58(actor, &waterfall->shake);
        break;
    case 2:
        CopyActorWPos(actor, &pos);
        pos.y += waterfall->stepY;
        if (pos.y <= waterfall->dest.y) {
            pos.y = waterfall->dest.y;
            waterfall->state++;
            waterfall->stepX = (waterfall->dest.x - pos.x) / 16;
            waterfall->stepZ = (waterfall->dest.z - pos.z) / 16;
            FieldCameraZoomTCB_Create(waterfall->field, 16, -FX32_CONST(32));
            func_ov036_021a3a70(waterfall->splash);
            task = (FieldEffectTask *)func_ov036_0219a5c4(waterfall->player);
            func_ov036_021a4504(task, 1);
            waterfall->splash = func_ov036_021a4eb0(Field_GetFieldEffects(waterfall->field), 0, task);
        }
        SetActorWPosValue(actor, &pos);
        break;
    case 3:
        CopyActorWPos(actor, &pos);
        pos.x += waterfall->stepX;
        pos.z += waterfall->stepZ;
        SetActorWPosValue(actor, &pos);
        waterfall->counter++;
        if (waterfall->counter >= 16) {
            effects = Field_GetFieldEffects(waterfall->field);
            func_ov012_0216763c(actor, FALSE);
            SetActorWPosAll(actor, &waterfall->dest, waterfall->dir);
            func_ov036_021a3a70(waterfall->splash);
            FieldEffects_FreeEffect(effects, 8);
            func_ov036_0219ac0c(waterfall->player);
            func_ov036_021a4514((FieldEffectTask *)func_ov036_0219a5c4(waterfall->player), TRUE);
            waterfall->splash = NULL;
            waterfall->finished = TRUE;
            GFL_SndSEPlay(1762);
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
        func_ov036_021bb220(waterfall);
    }
}

static FieldActor *FieldPlayerGrid_FindFrontActor(FieldPlayerGrid *grid, u16 dir) {
    FieldActor *player = FieldPlayerGrid_GetPlayerActor(grid);
    MMSys *system = GetActorMModelSystem(player);
    int x = GetGPosX(player) + GetDirectionVectorCompX(dir);
    int z = GetGPosZ(player) + GetDirectionVectorCompZ(dir);

    return FindActorByGPos(system, x, z, GetActorPosY(player), 1, FALSE);
}
