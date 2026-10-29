// The rail data of a map, loaded from ARCID_RAIL_HEADERS, and the update functions of the billboard actors and 3D
// objects of the rail maps' actors. The name is the ROM's own, from GFL_HeapAllocate's file argument. Each kind of
// scene node's table follows its update function, where the size sort lays the tables out in the ROM's order
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/arc.h"
#include "constants/directions.h"
#include "constants/obj_codes.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/field_camera.h"
#include "field/field_g3dobj.h"
#include "field/field_player.h"
#include "field/field_rail.h"
#include "field/field_rail_loader.h"
#include "field/field_script_event.h"
#include "field/zone.h"
#include "gfl/arc_util.h"
#include "gfl/billboard_actor.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/mi.h"

// The header of a rail file: the offsets of its tile width and of its points, lines, cameras and curves, each running
// up to the next
typedef struct {
    u32 unk0;
    u32 tileWidthOffset;
    u32 pointsOffset;
    u32 linesOffset;
    u32 camerasOffset;
    u32 curvesOffset;
    u32 endOffset;
} RailFileHeader;

// Layout from swan's RailLoader
struct RailLoader {
    RailFileHeader *file;
    u32 size;
    RailDataHandle data;
};

// What the animation of a billboard actor of a rail object remembers between frames
typedef struct {
    u8 dir;
    u8 state;
    u8 step;
    u8 pending;
} RailActorAnim;

// What the billboard actors of rail objects keep in their work area
typedef struct {
    u16 index;
    u8 unk2;
    u8 unk3;
    RailActorAnim anim;
} RailActorWork;

// What the billboard actors of walking rail objects keep
typedef struct {
    u16 index;
    u8 dir;
    u8 frame;
    u32 unk4;
    u32 unk8;
} RailNpcWork;

// An object code's animation rules
typedef struct {
    u16 objCode;
    u8 kind : 4;
    u8 trackDir : 4;
    u8 anim;
} RailObjAnim;

static void FieldActorMove_VanishProc(FieldActor *actor);
static void FieldActorMove_VanishDraw(FieldActor *actor);
static void FieldActorMove_VanishDelete(FieldActor *actor);
static void FieldActorMove_VanishSuspend(FieldActor *actor);
static void FieldActorMove_VanishWakeUp(FieldActor *actor);
static void RailActorAnim_InitDirAnime(FieldActor *actor);
static void RailActorAnim_DeleteDirAnime(FieldActor *actor);
static void RailActorAnim_DrawDirAnime(FieldActor *actor);
static u32 RailActorAnim_GetBillboardActorIdDirAnime(FieldActor *actor, u32 param);
static void RailActorAnim_DrawDirAnimeMoving(FieldActor *actor);
static void RailActorAnim_InitDirAnimeB(FieldActor *actor);
static void RailActorAnim_DeleteDirAnimeB(FieldActor *actor);
static void RailActorAnim_DrawDirAnimeB(FieldActor *actor);
static void RailActorAnim_DrawDirAnimeSized(FieldActor *actor);
static void RailActorAnim_DrawWalkAnime(FieldActor *actor);
static void RailActorAnim_DrawWalkAnimeB(FieldActor *actor);
static u32 RailActorAnim_GetBillboardActorIdWalkAnime(FieldActor *actor, u32 param);
static void RailActorAnim_DrawTwoFrame(FieldActor *actor);
static void RailActorAnim_DrawThreeFrame(FieldActor *actor);
static void TransformVecByCamera(G3DCamera *camera, VecFx32 *vec, VecFx32 *dest);
static void RailActorAnim_GetFrameOffsetZero(FieldActor *actor, u16 anim, u16 step, VecFx32 *dest);
static void RailActorAnim_DrawBillboardOffset(FieldActor *actor);
static void RailActorAnim_DrawStatic(FieldActor *actor);
static void RailActorAnim_DrawPositionOnly(FieldActor *actor);
static void RailActorAnim_InitFixedState(FieldActor *actor);
static void RailActorAnim_DeleteFixedState(FieldActor *actor);
static void RailActorAnim_DrawWalking(FieldActor *actor);
static void RailActorAnim_UpdateWalkOffset(FieldActor *actor, RailNpcWork *work, u8 dir);
static void RailActorAnim_ShiftOffsetByDir(FieldActor *actor, u32 dir, const FieldActorConfig *config, VecFx32 *offset);
static BOOL RailActorAnim_CheckStepFrame(RailNpcWork *work, u32 dir, const FieldActorConfig *config);
static void FldAct_SyncLights(BillboardActorSys *sys, FieldActor *actor, RailActorWork *work);
static u32 RailActorAnim_GetBillboardActorIdFixedState(FieldActor *actor, u32 param);
static void RailActorAnim_InitDirAnimeC(FieldActor *actor);
static void RailActorAnim_DeleteDirAnimeC(FieldActor *actor);
static void RailActorAnim_DrawMultiPattern(FieldActor *actor);
static u32 RailActorAnim_GetBillboardActorIdMultiPattern(FieldActor *actor, u32 param);
static void RailActorAnim_DrawTwoPattern(FieldActor *actor);
static void RailActorAnim_DrawThreePattern(FieldActor *actor);
static void RailActorAnim_Reset(RailActorAnim *anim);
static void RailActorAnim_Update(RailActorAnim *anim, FieldActor *actor, BillboardActorSys *sys, u16 index);
static void RailActorAnim_AddHeightOffset(FieldActor *actor, VecFx32 *pos);
static u8 RailActor_GetCameraRelativeDir(FieldActor *actor);
static void RailActor_UpdateVisibilityAndAnimation(FieldActor *actor, BillboardActorSys *sys, u16 index, BOOL changed,
                                                   BOOL flag);

RailLoader *FieldRailLoader_Create(u32 heapId) {
    RailLoader *loader = GFL_HeapAllocate(heapId, sizeof(RailLoader), TRUE, "field_rail_loader.c", 192);

    RailDataHandle_Reset(&loader->data);
    return loader;
}

void FieldRailLoader_Free(RailLoader *loader) {
    FieldRailLoader_Reset(loader);
    GFL_HeapFree(loader);
}

void FieldRailLoader_LoadFile(RailLoader *loader, u32 fileId, u32 heapId) {
    u32 size;

    loader->file = GFL_ArcSysReadHeapNewLZGetLen(ARCID_RAIL_HEADERS, fileId, FALSE, heapId, &size);
    loader->size = size;
    loader->data.points = (RailPoint *)((u8 *)loader->file + loader->file->pointsOffset);
    loader->data.pointCount = (loader->file->linesOffset - loader->file->pointsOffset) / sizeof(RailPoint);
    loader->data.lines = (RailLine *)((u8 *)loader->file + loader->file->linesOffset);
    loader->data.lineCount = (loader->file->camerasOffset - loader->file->linesOffset) / sizeof(RailLine);
    loader->data.cameras = (RailCamera *)((u8 *)loader->file + loader->file->camerasOffset);
    loader->data.cameraCount = (loader->file->curvesOffset - loader->file->camerasOffset) / sizeof(RailCamera);
    loader->data.curves = (RailCurve *)((u8 *)loader->file + loader->file->curvesOffset);
    loader->data.curveCount = (loader->file->endOffset - loader->file->curvesOffset) / sizeof(RailCurve);
    // The file's bytes hold the tile width at the offset its header gives (style-ok: byte-offset)
    loader->data.tileWidth = *(fx32 *)((u8 *)loader->file + loader->file->tileWidthOffset);
}

void FieldRailLoader_Reset(RailLoader *loader) {
    if (loader->file != NULL) {
        GFL_HeapFree(loader->file);
        loader->file = NULL;
    }
    RailDataHandle_Reset(&loader->data);
}

RailDataHandle *FieldRailLoader_GetRailData(RailLoader *loader) {
    return &loader->data;
}

GameEvent *CreatePlayerXActorScriptCall(GameSystem *gsys, Field *field, u32 scriptId, FieldActor *player,
                                        FieldActor *actor, HeapID heapId) {
    return EventScriptCall_Create(gsys, scriptId, actor, heapId);
}

static void FieldActorMove_VanishProc(FieldActor *actor) {
    SetActorHidden(actor, TRUE);
    SetActorMovementFlag(actor, ACTOR_MOVEMENT_FLAG_HIDE_SHADOW);
}

static void FieldActorMove_VanishDraw(FieldActor *actor) {
}

static void FieldActorMove_VanishDelete(FieldActor *actor) {
}

static void FieldActorMove_VanishSuspend(FieldActor *actor) {
}

static void FieldActorMove_VanishWakeUp(FieldActor *actor) {
}

void RailActorAnim_GetBillboardActorAnime(FieldActor *actor, u16 *anim, u16 *step) {
    if (GetActorMdlInfo(actor)->entityType == 1) {
        RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

        if (work->index != BILLBOARD_ACTOR_NONE) {
            BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));

            *anim = BillboardActorSys_GetActorAnim(sys, work->index);
            *step = BillboardActorSys_GetActorStep(sys, work->index);
            return;
        }
        *anim = 0;
        *step = 0;
        return;
    }
    *anim = 0;
    *step = 0;
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_VANISH = { FieldActorMove_VanishProc,   FieldActorMove_VanishDraw,
                                                              FieldActorMove_VanishDelete, FieldActorMove_VanishSuspend,
                                                              FieldActorMove_VanishWakeUp, NULL };

void RailActorAnim_SetBillboardActorAnime(FieldActor *actor, u16 anim, u16 step) {
    if (GetActorMdlInfo(actor)->entityType == 1) {
        RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

        if (work->index != BILLBOARD_ACTOR_NONE) {
            BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));

            BillboardActorSys_SetActorAnim(sys, work->index, anim);
            BillboardActorSys_SetActorStep(sys, work->index, step);
        }
    }
}

static void RailActorAnim_InitDirAnime(FieldActor *actor) {
    RailActorWork *work = FldAct_ResetBillboardActorWork(actor, 8);

    RailActorAnim_Reset(&work->anim);
    FldBillboardActor_AddFieldActorOrReserve(actor, &work->index);
}

static void RailActorAnim_DeleteDirAnime(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    FldBillboardActor_DeleteFieldActor(actor, work->index);
}

static void RailActorAnim_DrawDirAnime(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        VecFx32 pos;

        RailActorAnim_Update(&work->anim, actor, sys, work->index);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_DIR_ANIME_1 = {
    RailActorAnim_InitDirAnime,   RailActorAnim_DrawDirAnime, RailActorAnim_DeleteDirAnime,
    RailActorAnim_DeleteDirAnime, RailActorAnim_InitDirAnime, RailActorAnim_GetBillboardActorIdDirAnime
};
const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_DIR_ANIME_3 = {
    RailActorAnim_InitDirAnime,   RailActorAnim_DrawDirAnime, RailActorAnim_DeleteDirAnime,
    RailActorAnim_DeleteDirAnime, RailActorAnim_InitDirAnime, RailActorAnim_GetBillboardActorIdDirAnime
};

static u32 RailActorAnim_GetBillboardActorIdDirAnime(FieldActor *actor, u32 param) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    return work->index;
}

static void RailActorAnim_DrawDirAnimeMoving(FieldActor *actor) {
    BOOL changed = FALSE;
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        u8 dir = RailActor_GetCameraRelativeDir(actor);
        u16 state = FldAct_GetDrawStatus(actor);
        VecFx32 pos;

        if (work->anim.dir != dir) {
            work->anim.dir = dir;
            work->anim.state = state;
            BillboardActorSys_SetActorAnim(sys, work->index, dir);
            changed = TRUE;
        } else if (work->anim.state != state) {
            BillboardActorSys_SetActorAnim(sys, work->index, dir);
            BillboardActorSys_SetActorStep(sys, work->index, changed);
            work->anim.state = state;
        }
        RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, changed, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_DIR_ANIME_MOVING = {
    RailActorAnim_InitDirAnime,   RailActorAnim_DrawDirAnimeMoving, RailActorAnim_DeleteDirAnime,
    RailActorAnim_DeleteDirAnime, RailActorAnim_InitDirAnime,       RailActorAnim_GetBillboardActorIdDirAnime
};

static void RailActorAnim_InitDirAnimeB(FieldActor *actor) {
    RailActorWork *work = FldAct_ResetBillboardActorWork(actor, 8);

    RailActorAnim_Reset(&work->anim);
    FldBillboardActor_AddFieldActorOrReserve(actor, &work->index);
}

static void RailActorAnim_DeleteDirAnimeB(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    FldBillboardActor_DeleteFieldActor(actor, work->index);
}

static void RailActorAnim_DrawDirAnimeB(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        VecFx32 pos;

        RailActorAnim_Update(&work->anim, actor, sys, work->index);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_DIR_ANIME_B = {
    RailActorAnim_InitDirAnimeB,   RailActorAnim_DrawDirAnimeB, RailActorAnim_DeleteDirAnimeB,
    RailActorAnim_DeleteDirAnimeB, RailActorAnim_InitDirAnimeB, RailActorAnim_GetBillboardActorIdWalkAnime
};

static void RailActorAnim_DrawDirAnimeSized(FieldActor *actor) {
    BOOL changed = FALSE;
    BOOL flag = FALSE;
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        u8 dir = RailActor_GetCameraRelativeDir(actor);
        u8 nodeType;
        u16 anim;
        VecFx32 pos;

        if (FldAct_GetObjCode(actor) == OBJ_CODE_204) {
            flag = TRUE;
        }
        nodeType = GetActorMdlInfo(actor)->sceneNodeType;
        if (nodeType == 15) {
            anim = 4;
        } else if (nodeType == 5) {
            anim = 8;
        } else {
            anim = 16;
        }
        anim += dir;
        if (work->anim.dir != dir) {
            work->anim.dir = dir;
            BillboardActorSys_SetActorAnim(sys, work->index, anim);
            changed = TRUE;
        }
        RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, changed, flag);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_DIR_ANIME_SIZED = {
    RailActorAnim_InitDirAnimeB,   RailActorAnim_DrawDirAnimeSized, RailActorAnim_DeleteDirAnimeB,
    RailActorAnim_DeleteDirAnimeB, RailActorAnim_InitDirAnimeB,     RailActorAnim_GetBillboardActorIdWalkAnime
};

static void RailActorAnim_DrawWalkAnime(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        u16 state;
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        VecFx32 pos;

        state = FldAct_GetDrawStatus(actor);
        if (state >= DRAW_STATUS_ANIME_10) {
            u8 changed = FALSE;
            u8 dir = RailActor_GetCameraRelativeDir(actor);

            if (state != work->anim.state) {
                changed = TRUE;
                work->anim.dir = dir;
                work->anim.state = state;
                work->anim.step = 0;
                BillboardActorSys_SetActorAnim(sys, work->index, state + 30);
            }
            RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, changed, FALSE);
        } else {
            RailActorAnim_Update(&work->anim, actor, sys, work->index);
        }
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_WALK_ANIME = {
    RailActorAnim_InitDirAnimeB,   RailActorAnim_DrawWalkAnime, RailActorAnim_DeleteDirAnimeB,
    RailActorAnim_DeleteDirAnimeB, RailActorAnim_InitDirAnimeB, RailActorAnim_GetBillboardActorIdWalkAnime
};

static void RailActorAnim_DrawWalkAnimeB(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        u16 state = FldAct_GetDrawStatus(actor);
        VecFx32 pos;

        if (state >= DRAW_STATUS_ANIME_10) {
            u8 dir = RailActor_GetCameraRelativeDir(actor);

            if (state != work->anim.state) {
                work->anim.dir = dir;
                work->anim.state = state;
                work->anim.step = 0;
                BillboardActorSys_SetActorAnim(sys, work->index, state + 30);
            }
            RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, TRUE, FALSE);
        } else {
            RailActorAnim_Update(&work->anim, actor, sys, work->index);
        }
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_WALK_ANIME_B = {
    RailActorAnim_InitDirAnimeB,   RailActorAnim_DrawWalkAnimeB, RailActorAnim_DeleteDirAnimeB,
    RailActorAnim_DeleteDirAnimeB, RailActorAnim_InitDirAnimeB,  RailActorAnim_GetBillboardActorIdWalkAnime
};

static u32 RailActorAnim_GetBillboardActorIdWalkAnime(FieldActor *actor, u32 param) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    return work->index;
}

static void RailActorAnim_DrawTwoFrame(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        u16 state;
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        VecFx32 pos;

        state = FldAct_GetDrawStatus(actor);
        if (work->anim.state >= 2) {
            work->anim.state = 0;
            BillboardActorSys_SetActorAnim(sys, work->index, work->anim.state);
            BillboardActorSys_SetActorStep(sys, work->index, 0);
        }
        if (state < 2 && work->anim.state != state) {
            work->anim.state = state;
            BillboardActorSys_SetActorAnim(sys, work->index, work->anim.state);
            BillboardActorSys_SetActorStep(sys, work->index, 0);
        }
        RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, FALSE, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_TWO_FRAME = {
    RailActorAnim_InitDirAnime,   RailActorAnim_DrawTwoFrame, RailActorAnim_DeleteDirAnime,
    RailActorAnim_DeleteDirAnime, RailActorAnim_InitDirAnime, RailActorAnim_GetBillboardActorIdDirAnime
};

static void RailActorAnim_DrawThreeFrame(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        u16 state;
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        VecFx32 pos;

        state = FldAct_GetDrawStatus(actor);
        if (work->anim.state >= 3) {
            work->anim.state = 0;
            BillboardActorSys_SetActorAnim(sys, work->index, work->anim.state);
            BillboardActorSys_SetActorStep(sys, work->index, 0);
        }
        if (state < 3 && work->anim.state != state) {
            work->anim.state = state;
            BillboardActorSys_SetActorAnim(sys, work->index, work->anim.state);
            BillboardActorSys_SetActorStep(sys, work->index, 0);
        }
        RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, FALSE, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_THREE_FRAME = {
    RailActorAnim_InitDirAnime,   RailActorAnim_DrawThreeFrame, RailActorAnim_DeleteDirAnime,
    RailActorAnim_DeleteDirAnime, RailActorAnim_InitDirAnime,   RailActorAnim_GetBillboardActorIdDirAnime
};

void RailActorAnim_SetState(FieldActor *actor, BOOL value) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    work->unk2 = value;
}

static void TransformVecByCamera(G3DCamera *camera, VecFx32 *vec, VecFx32 *dest) {
    VecFx32 pos;
    VecFx32 up;
    VecFx32 target;
    MtxFx43 view;
    MtxFx33 inverse;

    GFL_G3DCameraGetLookatPos(camera, &pos);
    GFL_G3DCameraGetLookatUpVector(camera, &up);
    GFL_G3DCameraGetLookatTarget(camera, &target);
    MTX_LookAt(&pos, &up, &target, &view);
    MI_Copy36B(&view, &inverse);
    MTX_Inverse33(&inverse, &inverse);
    MTX_MultVec33(vec, &inverse, dest);
}

static void RailActorAnim_GetFrameOffsetZero(FieldActor *actor, u16 anim, u16 step, VecFx32 *dest) {
    dest->x = 0;
    dest->y = 0;
    dest->z = 0;
}

static void RailActorAnim_DrawBillboardOffset(FieldActor *actor) {
    BOOL changed = FALSE;
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        u8 dir = RailActor_GetCameraRelativeDir(actor);
        u16 state = FldAct_GetDrawStatus(actor);
        u16 anim = dir + state * 4;
        VecFx32 pos;
        VecFx32 offset;

        if (dir != work->anim.dir || work->anim.state != state) {
            changed = TRUE;
            work->anim.dir = dir;
            work->anim.state = state;
            BillboardActorSys_SetActorAnim(sys, work->index, anim);
            BillboardActorSys_SetActorStep(sys, work->index, 0);
        }
        RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, changed, TRUE);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        RailActorAnim_GetFrameOffsetZero(actor, BillboardActorSys_GetActorAnim(sys, work->index),
                                         BillboardActorSys_GetActorStep(sys, work->index), &offset);
        pos.x += offset.x;
        pos.y += offset.y;
        pos.z += offset.z;
        if (work->unk2 == 0) {
            VecFx32 shift = { 0, -0xd000, 0 };

            TransformVecByCamera(
                FieldCamera_GetG3DCamera(Field_GetCameraSystem(GetMMSysField(GetActorMModelSystem(actor)))), &shift,
                &shift);
            VEC_Add(&pos, &shift, &pos);
        } else {
            pos.y -= 0xc000;
            pos.z += 0x8000;
        }
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_BILLBOARD_OFFSET = {
    RailActorAnim_InitDirAnime,   RailActorAnim_DrawBillboardOffset, RailActorAnim_DeleteDirAnime,
    RailActorAnim_DeleteDirAnime, RailActorAnim_InitDirAnime,        RailActorAnim_GetBillboardActorIdDirAnime
};

static void RailActorAnim_DrawStatic(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        u8 dir = RailActor_GetCameraRelativeDir(actor);
        VecFx32 pos;

        if (work->anim.dir != dir) {
            BillboardActorSys_SetActorAnim(sys, work->index, dir);
            BillboardActorSys_SetActorStep(sys, work->index, 0);
            work->anim.dir = dir;
        }
        RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, FALSE, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_STATIC = {
    RailActorAnim_InitDirAnime,   RailActorAnim_DrawStatic,   RailActorAnim_DeleteDirAnime,
    RailActorAnim_DeleteDirAnime, RailActorAnim_InitDirAnime, RailActorAnim_GetBillboardActorIdDirAnime
};

BOOL RailActorAnim_SyncFrameGroup(FieldActor *actor, u16 time) {
    u16 step;
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);
    BillboardActorSys *sys;
    u16 current;

    if (work->index == BILLBOARD_ACTOR_NONE) {
        return FALSE;
    }
    if (time < 24) {
        if (time < 6) {
            step = 0;
        } else if (time < 12) {
            step = 1;
        } else if (time < 18) {
            step = 2;
        } else {
            step = 3;
        }
    } else if (time < 40) {
        if (time < 28) {
            step = 4;
        } else if (time < 32) {
            step = 5;
        } else if (time < 36) {
            step = 6;
        } else {
            step = 7;
        }
    } else if (time < 48) {
        if (time < 42) {
            step = 8;
        } else if (time < 44) {
            step = 9;
        } else if (time < 46) {
            step = 10;
        } else {
            step = 11;
        }
    } else {
        step = 12;
    }
    sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
    current = BillboardActorSys_GetActorStep(sys, work->index);
    if (current != step) {
        BillboardActorSys_SetActorStep(sys, work->index, 0);
    }
    return TRUE;
}

static void RailActorAnim_DrawPositionOnly(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        VecFx32 pos;

        RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, FALSE, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_POSITION_ONLY = {
    RailActorAnim_InitDirAnimeB,   RailActorAnim_DrawPositionOnly, RailActorAnim_DeleteDirAnimeB,
    RailActorAnim_DeleteDirAnimeB, RailActorAnim_InitDirAnimeB,    RailActorAnim_GetBillboardActorIdWalkAnime
};

static void RailActorAnim_InitFixedState(FieldActor *actor) {
    RailActorWork *work = FldAct_ResetBillboardActorWork(actor, 8);

    work->unk2 = 9;
    FldBillboardActor_AddFieldActorOrReserve(actor, &work->index);
}

static void RailActorAnim_DeleteFixedState(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    FldBillboardActor_DeleteFieldActor(actor, work->index);
}

static void RailActorAnim_DrawWalking(FieldActor *actor) {
    RailNpcWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        BOOL changed = FALSE;
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        u16 dir = GetActorFaceDir(actor);
        VecFx32 pos;

        if (dir != work->dir) {
            changed = TRUE;
            work->dir = dir;
            work->frame = 0;
            work->unk8 = 0;
            BillboardActorSys_SetActorAnim(sys, work->index, work->dir);
            {
                VecFx32 offset = { 0, 0, 0 };

                CopyActorWPosOffset(actor, &offset);
            }
        }
        RailActorAnim_UpdateWalkOffset(actor, work, dir);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
        RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, changed, FALSE);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_WALKING = {
    RailActorAnim_InitFixedState,   RailActorAnim_DrawWalking,    RailActorAnim_DeleteFixedState,
    RailActorAnim_DeleteFixedState, RailActorAnim_InitFixedState, RailActorAnim_GetBillboardActorIdFixedState
};

static void RailActorAnim_UpdateWalkOffset(FieldActor *actor, RailNpcWork *work, u8 dir) {
    BOOL walking;
    BOOL isPlayer;
    VecFx32 offset;
    VecFx32 base;
    Field *field;
    BOOL hidden;
    u16 zone;
    s16 gx;
    const FieldActorConfig *config;
    BOOL isPuppet;
    u16 uid;

    walking = TRUE;
    config = GetActorMdlInfo(actor);
    isPlayer = FALSE;
    offset.x = 0;
    offset.y = 0;
    offset.z = 0;
    hidden = FieldAcmdTCB_CheckAnimePaused(actor);
    CopyActorWPosOffset(actor, &base);
    gx = GetGPosX(actor);
    zone = GetActorZoneID(actor);
    uid = GetActorUID(actor);
    if (uid == ACTOR_UID_PLAYER) {
        isPlayer = TRUE;
    }
    isPuppet = uid == 0xfc ? TRUE : FALSE;
    if (isPlayer) {
        field = GetMMSysField(GetActorMModelSystem(actor));
        if (Field_HasPlayer(field)) {
            FieldPlayer *player = Field_GetPlayer(field);

            if (player != NULL) {
                walking = FieldPlayer_GetMoveStatus(player) != 0;
            }
        }
    }
    if (hidden != 0 || (base.y > 0x1800 && config->sceneNodeType != 11)) {
        offset.y = base.y;
    } else if (RailActorAnim_CheckStepFrame(work, dir, config) == 0 && walking) {
        offset.y += 0x1800;
    }
    if (!isPlayer) {
        u8 rem;

        offset.z -= 0x2000;
        if (GetZoneIsEntreeForest(zone)) {
            rem = gx % 3;
            if (rem == 0) {
                offset.z -= 0x219a;
            } else if (rem == 2) {
                offset.z += 0x219a;
            }
        }
    }
    RailActorAnim_ShiftOffsetByDir(actor, dir, config, &offset);
    if (isPuppet) {
        offset.x = 0;
        offset.y = 0;
        offset.z = 0;
    }
    SetActorWPosOffset(actor, &offset);
    work->frame++;
}

static void RailActorAnim_ShiftOffsetByDir(FieldActor *actor, u32 dir, const FieldActorConfig *config,
                                           VecFx32 *offset) {
    switch (dir) {
    case DIR_LEFT:
        offset->x -= 0x2000;
        break;
    case DIR_RIGHT:
        offset->x += 0x2000;
        break;
    }
}

static BOOL RailActorAnim_CheckStepFrame(RailNpcWork *work, u32 dir, const FieldActorConfig *config) {
    int frame = work->frame % 20;

    work->frame = frame;
    if (config->sceneNodeType == 11) {
        if (dir == DIR_DOWN) {
            if (frame < 5 || frame >= 15) {
                return TRUE;
            }
        } else if (frame < 10) {
            return TRUE;
        }
    } else if ((frame >= 5 && frame < 10) || frame >= 15) {
        return TRUE;
    }
    return FALSE;
}

static void FldAct_SyncLights(BillboardActorSys *sys, FieldActor *actor, RailActorWork *work) {
    if (FldAct_GetShadowGroup(actor) != 0) {
        BillboardActorSys_SetActorLight(sys, work->index, TRUE, 3);
        BillboardActorSys_SetActorLight(sys, work->index, FALSE, 0);
        BillboardActorSys_SetActorLight(sys, work->index, FALSE, 1);
        BillboardActorSys_SetActorLight(sys, work->index, FALSE, 2);
    } else {
        BillboardActorSys_SetActorLight(sys, work->index, TRUE, 0);
        BillboardActorSys_SetActorLight(sys, work->index, FALSE, 1);
        BillboardActorSys_SetActorLight(sys, work->index, FALSE, 2);
        BillboardActorSys_SetActorLight(sys, work->index, FALSE, 3);
    }
}

static u32 RailActorAnim_GetBillboardActorIdFixedState(FieldActor *actor, u32 param) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    return work->index;
}

static void RailActorAnim_InitDirAnimeC(FieldActor *actor) {
    RailActorWork *work = FldAct_ResetBillboardActorWork(actor, 8);

    RailActorAnim_Reset(&work->anim);
    FldBillboardActor_AddFieldActorOrReserve(actor, &work->index);
}

static void RailActorAnim_DeleteDirAnimeC(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    FldBillboardActor_DeleteFieldActor(actor, work->index);
}

static const RailObjAnim RAIL_OBJ_ANIMS[12] = {
    { OBJ_CODE_211, 0, 0, 0 }, { OBJ_CODE_213, 0, 0, 0 }, { OBJ_CODE_212, 1, 1, 0 }, { OBJ_CODE_214, 1, 1, 0 },
    { OBJ_CODE_221, 2, 0, 0 }, { OBJ_CODE_222, 2, 0, 0 }, { OBJ_CODE_364, 3, 1, 0 }, { OBJ_CODE_365, 3, 1, 0 },
    { OBJ_CODE_366, 5, 0, 0 }, { OBJ_CODE_367, 4, 0, 0 }, { OBJ_CODE_368, 4, 0, 0 }, { OBJ_CODE_121, 4, 0, 0 },
};

static void RailActorAnim_DrawMultiPattern(FieldActor *actor) {
    u16 changed = FALSE;
    BillboardActorSys *sys;
    const RailObjAnim *entry = NULL;
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index == BILLBOARD_ACTOR_NONE) {
        return;
    }
    {
        sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        u16 objCode = FldAct_GetObjCode(actor);
        u16 state = FldAct_GetDrawStatus(actor);
        u8 dir = RailActor_GetCameraRelativeDir(actor);
        u32 i;
        VecFx32 pos;

        for (i = 0; i < 12; i++) {
            if (objCode == RAIL_OBJ_ANIMS[i].objCode) {
                entry = &RAIL_OBJ_ANIMS[i];
                break;
            }
        }
        if (entry == NULL) {
            return;
        }
        if (state != work->anim.state ||
            (entry->trackDir != 0 && state < DRAW_STATUS_ANIME_10 && dir != work->anim.dir)) {
            u8 anim = entry->anim;

            switch (entry->kind) {
            case 0:
                switch (state) {
                case DRAW_STATUS_ANIME_10:
                    anim = 1;
                    break;
                case DRAW_STATUS_ANIME_11:
                    anim = 2;
                    break;
                case DRAW_STATUS_ANIME_12:
                    anim = 3;
                    break;
                }
                break;
            case 1:
                switch (state) {
                case DRAW_STATUS_ANIME_10:
                    anim = 2;
                    break;
                case DRAW_STATUS_ANIME_11:
                    anim = 3;
                    break;
                default:
                    if (dir == DIR_UP) {
                        anim = 0;
                    } else if (dir == DIR_DOWN) {
                        anim = 1;
                    }
                    break;
                }
                break;
            case 3:
                if (state == DRAW_STATUS_ANIME_10) {
                    anim = 2;
                } else if (dir == DIR_UP) {
                    anim = 1;
                } else if (dir == DIR_DOWN) {
                    anim = 0;
                }
                break;
            case 4:
                if (state == DRAW_STATUS_ANIME_10) {
                    anim = 2;
                }
                break;
            case 5:
                break;
            }
            if (entry->trackDir != 0) {
                work->anim.dir = dir;
            }
            changed = TRUE;
            work->anim.state = state;
            BillboardActorSys_SetActorAnim(sys, work->index, anim);
        }
        RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, changed, TRUE);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_MULTI_PATTERN = {
    RailActorAnim_InitDirAnimeC,   RailActorAnim_DrawMultiPattern, RailActorAnim_DeleteDirAnimeC,
    RailActorAnim_DeleteDirAnimeC, RailActorAnim_InitDirAnimeC,    RailActorAnim_GetBillboardActorIdMultiPattern
};

static u32 RailActorAnim_GetBillboardActorIdMultiPattern(FieldActor *actor, u32 param) {
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    return work->index;
}

BOOL RailActorAnim_GetBillboardActorAnimeTypeA(FieldActor *actor, u16 *anim, u16 *step) {
    u16 objCode = FldAct_GetObjCode(actor);

    if (objCode == OBJ_CODE_211 || objCode == OBJ_CODE_213) {
        RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

        if (work->index != BILLBOARD_ACTOR_NONE) {
            BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));

            *anim = BillboardActorSys_GetActorAnim(sys, work->index);
            *step = BillboardActorSys_GetActorStep(sys, work->index);
            return TRUE;
        }
        return FALSE;
    }
    *anim = 0;
    *step = 0;
    return TRUE;
}

BOOL RailActorAnim_GetBillboardActorAnimeTypeB(FieldActor *actor, u16 *anim, u16 *step) {
    u16 objCode = FldAct_GetObjCode(actor);

    if (objCode == OBJ_CODE_221 || objCode == OBJ_CODE_222) {
        RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

        if (work->index != BILLBOARD_ACTOR_NONE) {
            BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));

            *anim = BillboardActorSys_GetActorAnim(sys, work->index);
            *step = BillboardActorSys_GetActorStep(sys, work->index);
            return TRUE;
        }
        return FALSE;
    }
    *anim = 0;
    *step = 0;
    return TRUE;
}

static void RailActorAnim_DrawTwoPattern(FieldActor *actor) {
    BOOL changed = FALSE;
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        u16 state = FldAct_GetDrawStatus(actor);
        VecFx32 pos;

        if (state != work->anim.state && state < 2) {
            work->anim.state = state;
            BillboardActorSys_SetActorAnim(sys, work->index, state);
            changed = TRUE;
        }
        RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, changed, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_TWO_PATTERN = {
    RailActorAnim_InitDirAnimeB,   RailActorAnim_DrawTwoPattern, RailActorAnim_DeleteDirAnimeB,
    RailActorAnim_DeleteDirAnimeB, RailActorAnim_InitDirAnimeB,  RailActorAnim_GetBillboardActorIdWalkAnime
};

static void RailActorAnim_DrawThreePattern(FieldActor *actor) {
    BOOL changed = FALSE;
    RailActorWork *work = FldAct_GetBillboardActorWorkPtr(actor);

    if (work->index != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys *sys = FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor));
        u16 objCode = FldAct_GetObjCode(actor);
        u16 state = FldAct_GetDrawStatus(actor);
        VecFx32 pos;

        if (state != work->anim.state) {
            u16 anim = 0;

            if (objCode == OBJ_CODE_123) {
                switch (state) {
                case DRAW_STATUS_ANIME_10:
                    anim = 1;
                    break;
                case DRAW_STATUS_ANIME_11:
                    anim = 2;
                    break;
                case DRAW_STATUS_ANIME_12:
                    anim = 3;
                    break;
                }
            } else if (objCode == OBJ_CODE_374 && state == DRAW_STATUS_ANIME_13) {
                anim = 4;
            }
            work->anim.state = state;
            BillboardActorSys_SetActorAnim(sys, work->index, anim);
            changed = TRUE;
        }
        RailActor_UpdateVisibilityAndAnimation(actor, sys, work->index, changed, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        RailActorAnim_AddHeightOffset(actor, &pos);
        BillboardActorScene_SetActorPos(BillboardActorSys_GetScene(sys), work->index, &pos);
    }
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_THREE_PATTERN = {
    RailActorAnim_InitDirAnimeB,   RailActorAnim_DrawThreePattern, RailActorAnim_DeleteDirAnimeB,
    RailActorAnim_DeleteDirAnimeB, RailActorAnim_InitDirAnimeB,    RailActorAnim_GetBillboardActorIdWalkAnime
};

BOOL RailActorAnim_CheckBillboardActorReady(FieldActor *actor) {
    u16 objCode = FldAct_GetObjCode(actor);

    if (objCode != OBJ_CODE_123 && objCode != OBJ_CODE_374) {
        return TRUE;
    }
    {
        u16 index = FldAct_GetBillboardActorIdx(actor, 0);

        if (index != BILLBOARD_ACTOR_NONE) {
            u16 cmd;

            if (BillboardActorSys_GetActorLastCmd(
                    FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor)), index, &cmd) ==
                TRUE) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void RailActorAnim_ResetBillboardActorAnime(FieldActor *actor) {
    u16 objCode = FldAct_GetObjCode(actor);

    if (objCode == OBJ_CODE_123 || objCode == OBJ_CODE_374) {
        u16 index = FldAct_GetBillboardActorIdx(actor, 0);

        if (index != BILLBOARD_ACTOR_NONE) {
            BillboardActorSys_SetActorStep(
                FieldBillboardActor_GetBillboardActorSys(FldAct_GetFieldBillboardActor(actor)), index, 0);
        }
    }
}

static void RailActorAnim_Reset(RailActorAnim *anim) {
    MI_CpuFill8(anim, 0, 4);
    anim->dir = 8;
}

static void RailActorAnim_Update(RailActorAnim *anim, FieldActor *actor, BillboardActorSys *sys, u16 index) {
    BOOL changed = FALSE;
    u16 animId;
    u16 dir = RailActor_GetCameraRelativeDir(actor);
    u16 state = FldAct_GetDrawStatus(actor);

    animId = dir + state * 4;
    if (anim->pending != 0 && state == anim->state) {
        anim->pending = 0;
    }
    if (dir != anim->dir) {
        changed = TRUE;
        anim->state = state;
        anim->dir = dir;
        anim->step = 0;
        BillboardActorSys_SetActorAnim(sys, index, animId);
    } else if (state != anim->state) {
        changed = TRUE;
        if (state == DRAW_STATUS_STOP) {
            if (anim->state == DRAW_STATUS_STOP) {
                BillboardActorSys_SetActorAnim(sys, index, animId);
                anim->state = state;
                anim->pending = 0;
            } else if (anim->pending == 0) {
                anim->pending++;
                changed = FALSE;
            } else {
                u8 steps[4] = { 0, 2, 2, 0 };

                anim->step = BillboardActorSys_GetActorStep(sys, index);
                if (anim->step >= 4) {
                    anim->step = 0;
                }
                anim->step = steps[anim->step];
                BillboardActorSys_SetActorAnim(sys, index, animId);
                anim->state = state;
                anim->pending = 0;
            }
        } else {
            if (anim->state != DRAW_STATUS_STOP) {
                anim->step = BillboardActorSys_GetActorStep(sys, index);
                if (anim->step >= 2) {
                    anim->step = 0;
                } else {
                    anim->step = 2;
                }
            }
            BillboardActorSys_SetActorAnim(sys, index, animId);
            BillboardActorSys_SetActorStep(sys, index, anim->step);
            anim->state = state;
            anim->pending = 0;
        }
    }
    RailActor_UpdateVisibilityAndAnimation(actor, sys, index, changed, FALSE);
}

static void RailActorAnim_AddHeightOffset(FieldActor *actor, VecFx32 *pos) {
    pos->y += Field_GetActorYOffset(GetMMSysField(GetActorMModelSystem(actor)));
}

static u8 RailActor_GetCameraRelativeDir(FieldActor *actor) {
    u16 angle = FldActSys_GetCameraAngle(GetActorMModelSystem(actor));
    u16 dir = GetActorFaceDir(actor);

    if (CheckActorFlag(actor, ACTOR_FLAG_RAIL)) {
        return RailActor_GetDirByAngle(actor, angle);
    }
    return RotateDirByAngle(dir, angle);
}

static void RailActor_UpdateVisibilityAndAnimation(FieldActor *actor, BillboardActorSys *sys, u16 index, BOOL changed,
                                                   BOOL flag) {
    BOOL animate = TRUE;
    BOOL visible;
    u8 shown = FldBillboardActor_GetEntryHookFlag(actor);

    if (!changed) {
        if (flag == 1) {
            if (shown == 1 && CheckActorFlag(actor, 0x10)) {
                animate = FALSE;
            }
        } else if (shown == 1 && FieldAcmdTCB_CheckAnimePaused(actor) == 1) {
            animate = FALSE;
        }
    }
    BillboardActorSys_SetActorAnimating(sys, index, animate);
    visible = TRUE;
    if (IsActorHidden(actor) == 1) {
        visible = FALSE;
    }
    BillboardActorSys_SetActorVisible(sys, index, visible);
}
