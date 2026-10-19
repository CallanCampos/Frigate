// The rail data of a map, loaded from ARCID_RAIL_HEADERS, and the update functions of the billboard actors and 3D
// objects of the rail maps' actors. The name is the ROM's own, from GFL_HeapAllocate's file argument
#include "types.h"
#include "constants/arc.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_g3dobj.h"
#include "field/field_player.h"
#include "field/field_rail.h"
#include "field/field_rail_loader.h"
#include "field/field_script_event.h"
#include "field/zone.h"
#include "gfl/arc_util.h"
#include "gfl/blact.h"
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

// The functions of the billboard actors of an object code: what they do when created, updated and deleted, and which
// billboard actor they own
typedef struct {
    void (*create)(FieldActor *actor);
    void (*update)(FieldActor *actor);
    void (*delete)(FieldActor *actor);
    void (*deleteAlt)(FieldActor *actor);
    void (*createAlt)(FieldActor *actor);
    u16 (*getIndex)(FieldActor *actor);
} BlActFuncs;

static void func_ov036_02196324(FieldActor *actor);
static void func_ov036_0219633c(FieldActor *actor);
static void func_ov036_02196340(FieldActor *actor);
static void func_ov036_02196344(FieldActor *actor);
static void func_ov036_02196348(FieldActor *actor);
static void func_ov036_021963ec(FieldActor *actor);
static void func_ov036_02196408(FieldActor *actor);
static void func_ov036_0219641c(FieldActor *actor);
static u16 func_ov036_0219647c(FieldActor *actor);
static void func_ov036_02196488(FieldActor *actor);
static void func_ov036_0219653c(FieldActor *actor);
static void func_ov036_02196558(FieldActor *actor);
static void func_ov036_0219656c(FieldActor *actor);
static void func_ov036_021965cc(FieldActor *actor);
static void func_ov036_02196680(FieldActor *actor);
static void func_ov036_02196728(FieldActor *actor);
static u16 func_ov036_021967c8(FieldActor *actor);
static void func_ov036_021967d4(FieldActor *actor);
static void func_ov036_02196870(FieldActor *actor);
static void TransformVecByCamera(G3DCamera *camera, VecFx32 *vec, VecFx32 *dest);
static void func_ov036_0219696c(FieldActor *actor, u16 anim, u16 step, VecFx32 *dest);
static void func_ov036_02196978(FieldActor *actor);
static void func_ov036_02196a94(FieldActor *actor);
static void func_ov036_02196bb4(FieldActor *actor);
static void func_ov036_02196c0c(FieldActor *actor);
static void func_ov036_02196c24(FieldActor *actor);
static void func_ov036_02196c38(FieldActor *actor);
static void func_ov036_02196cd4(FieldActor *actor, RailNpcWork *work, u8 dir);
static void func_ov036_02196df8(FieldActor *actor, u32 dir, const FieldActorConfig *config, VecFx32 *offset);
static BOOL func_ov036_02196e1c(RailNpcWork *work, u32 dir, const FieldActorConfig *config);
static void FldAct_SyncLights(BlActSys *sys, FieldActor *actor, RailActorWork *work);
static u16 func_ov036_02196ed4(FieldActor *actor);
static void func_ov036_02196ee0(FieldActor *actor);
static void func_ov036_02196efc(FieldActor *actor);
static void func_ov036_02196f10(FieldActor *actor);
static u16 func_ov036_02197054(FieldActor *actor);
static void func_ov036_02197114(FieldActor *actor);
static void func_ov036_0219718c(FieldActor *actor);
static void func_ov036_021972cc(RailActorAnim *anim);
static void func_ov036_021972e0(RailActorAnim *anim, FieldActor *actor, BlActSys *sys, u16 index);
static void func_ov036_021973ec(FieldActor *actor, VecFx32 *pos);
static u8 func_ov036_02197404(FieldActor *actor);
static void func_ov036_0219743c(FieldActor *actor, BlActSys *sys, u16 index, BOOL changed, BOOL flag);
static void func_ov036_0219749c(FieldActor *actor);
static void func_ov036_021974b4(FieldActor *actor);
static void func_ov036_02197510(FieldActor *actor);
static void func_ov036_02197524(FieldActor *actor);
static void func_ov036_0219752c(FieldActor *actor);

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

static void func_ov036_02196324(FieldActor *actor) {
    SetActorHidden(actor, TRUE);
    SetActorMovementFlag(actor, 0x8000);
}

static void func_ov036_0219633c(FieldActor *actor) {
}

static void func_ov036_02196340(FieldActor *actor) {
}

static void func_ov036_02196344(FieldActor *actor) {
}

static void func_ov036_02196348(FieldActor *actor) {
}

void func_ov036_0219634c(FieldActor *actor, u16 *anim, u16 *step) {
    if (GetActorMdlInfo(actor)->entityType == 1) {
        RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

        if (work->index != BLACT_NONE) {
            BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));

            *anim = BlActSys_GetActorAnim(sys, work->index);
            *step = BlActSys_GetActorStep(sys, work->index);
            return;
        }
        *anim = 0;
        *step = 0;
        return;
    }
    *anim = 0;
    *step = 0;
}

void func_ov036_021963a4(FieldActor *actor, u16 anim, u16 step) {
    if (GetActorMdlInfo(actor)->entityType == 1) {
        RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

        if (work->index != BLACT_NONE) {
            BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));

            BlActSys_SetActorAnim(sys, work->index, anim);
            BlActSys_SetActorStep(sys, work->index, step);
        }
    }
}

static void func_ov036_021963ec(FieldActor *actor) {
    RailActorWork *work = FldAct_ResetBlActWork(actor, 8);

    func_ov036_021972cc(&work->anim);
    func_ov036_02193a00(actor, &work->index);
}

static void func_ov036_02196408(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    func_ov036_02193ac0(actor, work->index);
}

static void func_ov036_0219641c(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        VecFx32 pos;

        func_ov036_021972e0(&work->anim, actor, sys, work->index);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

static u16 func_ov036_0219647c(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    return work->index;
}

static void func_ov036_02196488(FieldActor *actor) {
    BOOL changed = FALSE;
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        u8 dir = func_ov036_02197404(actor);
        u16 state = FldAct_GetDrawStatus(actor);
        VecFx32 pos;

        if (work->anim.dir != dir) {
            work->anim.dir = dir;
            work->anim.state = state;
            BlActSys_SetActorAnim(sys, work->index, dir);
            changed = TRUE;
        } else if (work->anim.state != state) {
            BlActSys_SetActorAnim(sys, work->index, dir);
            BlActSys_SetActorStep(sys, work->index, changed);
            work->anim.state = state;
        }
        func_ov036_0219743c(actor, sys, work->index, changed, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

static void func_ov036_0219653c(FieldActor *actor) {
    RailActorWork *work = FldAct_ResetBlActWork(actor, 8);

    func_ov036_021972cc(&work->anim);
    func_ov036_02193a00(actor, &work->index);
}

static void func_ov036_02196558(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    func_ov036_02193ac0(actor, work->index);
}

static void func_ov036_0219656c(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        VecFx32 pos;

        func_ov036_021972e0(&work->anim, actor, sys, work->index);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

static void func_ov036_021965cc(FieldActor *actor) {
    BOOL changed = FALSE;
    BOOL flag = FALSE;
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        u8 dir = func_ov036_02197404(actor);
        u8 nodeType;
        u16 anim;
        VecFx32 pos;

        if (FldAct_GetObjCode(actor) == 0xcc) {
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
            BlActSys_SetActorAnim(sys, work->index, anim);
            changed = TRUE;
        }
        func_ov036_0219743c(actor, sys, work->index, changed, flag);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

static void func_ov036_02196680(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        u16 state;
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        VecFx32 pos;

        state = FldAct_GetDrawStatus(actor);
        if (state >= 10) {
            BOOL changed = FALSE;
            u8 dir = func_ov036_02197404(actor);

            if (state != work->anim.state) {
                changed = TRUE;
                work->anim.step = 0;
                work->anim.state = state;
                work->anim.dir = dir;
                BlActSys_SetActorAnim(sys, work->index, state + 30);
            }
            func_ov036_0219743c(actor, sys, work->index, changed, FALSE);
        } else {
            func_ov036_021972e0(&work->anim, actor, sys, work->index);
        }
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

static void func_ov036_02196728(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        u16 state = FldAct_GetDrawStatus(actor);
        VecFx32 pos;

        if (state >= 10) {
            u8 dir = func_ov036_02197404(actor);

            if (state != work->anim.state) {
                work->anim.dir = dir;
                work->anim.state = state;
                work->anim.step = 0;
                BlActSys_SetActorAnim(sys, work->index, state + 30);
            }
            func_ov036_0219743c(actor, sys, work->index, TRUE, FALSE);
        } else {
            func_ov036_021972e0(&work->anim, actor, sys, work->index);
        }
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
        FldAct_SyncLights(sys, actor, work);
    }
}

static u16 func_ov036_021967c8(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    return work->index;
}

static void func_ov036_021967d4(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        u16 state;
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        VecFx32 pos;

        state = FldAct_GetDrawStatus(actor);
        if (work->anim.state >= 2) {
            work->anim.state = 0;
            BlActSys_SetActorAnim(sys, work->index, work->anim.state);
            BlActSys_SetActorStep(sys, work->index, 0);
        }
        if (state < 2 && work->anim.state != state) {
            work->anim.state = state;
            BlActSys_SetActorAnim(sys, work->index, work->anim.state);
            BlActSys_SetActorStep(sys, work->index, 0);
        }
        func_ov036_0219743c(actor, sys, work->index, FALSE, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
    }
}

static void func_ov036_02196870(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        u16 state;
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        VecFx32 pos;

        state = FldAct_GetDrawStatus(actor);
        if (work->anim.state >= 3) {
            work->anim.state = 0;
            BlActSys_SetActorAnim(sys, work->index, work->anim.state);
            BlActSys_SetActorStep(sys, work->index, 0);
        }
        if (state < 3 && work->anim.state != state) {
            work->anim.state = state;
            BlActSys_SetActorAnim(sys, work->index, work->anim.state);
            BlActSys_SetActorStep(sys, work->index, 0);
        }
        func_ov036_0219743c(actor, sys, work->index, FALSE, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
    }
}

void func_ov036_0219690c(FieldActor *actor, BOOL value) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

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
    MAT3_Invert(&inverse, &inverse);
    MTX_MultVec33(vec, &inverse, dest);
}

static void func_ov036_0219696c(FieldActor *actor, u16 anim, u16 step, VecFx32 *dest) {
    dest->x = 0;
    dest->y = 0;
    dest->z = 0;
}

static void func_ov036_02196978(FieldActor *actor) {
    BOOL changed = FALSE;
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        u8 dir = func_ov036_02197404(actor);
        u16 state = FldAct_GetDrawStatus(actor);
        u16 anim = dir + state * 4;
        VecFx32 pos;
        VecFx32 offset;

        if (dir != work->anim.dir || work->anim.state != state) {
            changed = TRUE;
            work->anim.dir = dir;
            work->anim.state = state;
            BlActSys_SetActorAnim(sys, work->index, anim);
            BlActSys_SetActorStep(sys, work->index, 0);
        }
        func_ov036_0219743c(actor, sys, work->index, changed, TRUE);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        func_ov036_0219696c(actor, BlActSys_GetActorAnim(sys, work->index), BlActSys_GetActorStep(sys, work->index),
                            &offset);
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
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
    }
}

static void func_ov036_02196a94(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        u8 dir = func_ov036_02197404(actor);
        VecFx32 pos;

        if (work->anim.dir != dir) {
            BlActSys_SetActorAnim(sys, work->index, dir);
            BlActSys_SetActorStep(sys, work->index, 0);
            work->anim.dir = dir;
        }
        func_ov036_0219743c(actor, sys, work->index, FALSE, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
    }
}

BOOL func_ov036_02196b10(FieldActor *actor, u16 time) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);
    BlActSys *sys;

    if (work->index == BLACT_NONE) {
        return FALSE;
    }
    if (time < 24) {
        if (time < 6) {
            time = 0;
        } else if (time < 12) {
            time = 1;
        } else if (time < 18) {
            time = 2;
        } else {
            time = 3;
        }
    } else if (time < 40) {
        if (time < 28) {
            time = 4;
        } else if (time < 32) {
            time = 5;
        } else if (time < 36) {
            time = 6;
        } else {
            time = 7;
        }
    } else if (time < 48) {
        if (time < 42) {
            time = 8;
        } else if (time < 44) {
            time = 9;
        } else if (time < 46) {
            time = 10;
        } else {
            time = 11;
        }
    } else {
        time = 12;
    }
    sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
    if (BlActSys_GetActorStep(sys, work->index) != time) {
        BlActSys_SetActorStep(sys, work->index, 0);
    }
    return TRUE;
}

static void func_ov036_02196bb4(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        VecFx32 pos;

        func_ov036_0219743c(actor, sys, work->index, FALSE, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
    }
}

static void func_ov036_02196c0c(FieldActor *actor) {
    RailActorWork *work = FldAct_ResetBlActWork(actor, 8);

    work->unk2 = 9;
    func_ov036_02193a00(actor, &work->index);
}

static void func_ov036_02196c24(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    func_ov036_02193ac0(actor, work->index);
}

static void func_ov036_02196c38(FieldActor *actor) {
    RailNpcWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        BOOL changed = FALSE;
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        u16 dir = GetActorFaceDir(actor);
        VecFx32 pos;

        if (dir != work->dir) {
            changed = TRUE;
            work->dir = dir;
            work->frame = 0;
            work->unk8 = 0;
            BlActSys_SetActorAnim(sys, work->index, work->dir);
            {
                VecFx32 zero = { 0, 0, 0 };

                CopyActorWPosOffset(actor, &zero);
            }
        }
        func_ov036_02196cd4(actor, work, dir);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
        func_ov036_0219743c(actor, sys, work->index, changed, FALSE);
    }
}

static void func_ov036_02196cd4(FieldActor *actor, RailNpcWork *work, u8 dir) {
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
    hidden = func_ov036_0218ed68(actor);
    CopyActorWPosOffset(actor, &base);
    gx = GetGPosX(actor);
    zone = GetActorZoneID(actor);
    uid = GetActorUID(actor);
    if (uid == 0xff) {
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
    } else if (func_ov036_02196e1c(work, dir, config) == 0 && walking) {
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
    func_ov036_02196df8(actor, dir, config, &offset);
    if (isPuppet) {
        offset.x = 0;
        offset.y = 0;
        offset.z = 0;
    }
    SetActorWPosOffset(actor, &offset);
    work->frame++;
}

static void func_ov036_02196df8(FieldActor *actor, u32 dir, const FieldActorConfig *config, VecFx32 *offset) {
    switch (dir) {
    case 2:
        offset->x -= 0x2000;
        break;
    case 3:
        offset->x += 0x2000;
        break;
    }
}

static BOOL func_ov036_02196e1c(RailNpcWork *work, u32 dir, const FieldActorConfig *config) {
    int frame = work->frame % 20;

    work->frame = frame;
    if (config->sceneNodeType == 11) {
        if (dir == 1) {
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

static void FldAct_SyncLights(BlActSys *sys, FieldActor *actor, RailActorWork *work) {
    if (FldAct_GetShadowGroup(actor) != 0) {
        BlActSys_SetActorLight(sys, work->index, TRUE, 3);
        BlActSys_SetActorLight(sys, work->index, FALSE, 0);
        BlActSys_SetActorLight(sys, work->index, FALSE, 1);
        BlActSys_SetActorLight(sys, work->index, FALSE, 2);
    } else {
        BlActSys_SetActorLight(sys, work->index, TRUE, 0);
        BlActSys_SetActorLight(sys, work->index, FALSE, 1);
        BlActSys_SetActorLight(sys, work->index, FALSE, 2);
        BlActSys_SetActorLight(sys, work->index, FALSE, 3);
    }
}

static u16 func_ov036_02196ed4(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    return work->index;
}

static void func_ov036_02196ee0(FieldActor *actor) {
    RailActorWork *work = FldAct_ResetBlActWork(actor, 8);

    func_ov036_021972cc(&work->anim);
    func_ov036_02193a00(actor, &work->index);
}

static void func_ov036_02196efc(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    func_ov036_02193ac0(actor, work->index);
}

static const RailObjAnim RAIL_OBJ_ANIMS[12] = {
    { 0xd3, 0, 0, 0 },  { 0xd5, 0, 0, 0 },  { 0xd4, 1, 1, 0 },  { 0xd6, 1, 1, 0 },
    { 0xdd, 2, 0, 0 },  { 0xde, 2, 0, 0 },  { 0x16c, 3, 1, 0 }, { 0x16d, 3, 1, 0 },
    { 0x16e, 5, 0, 0 }, { 0x16f, 4, 0, 0 }, { 0x170, 4, 0, 0 }, { 0x79, 4, 0, 0 },
};

static void func_ov036_02196f10(FieldActor *actor) {
    u16 changed = FALSE;
    BlActSys *sys;
    const RailObjAnim *entry = NULL;
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index == BLACT_NONE) {
        return;
    }
    {
        sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        u16 objCode = FldAct_GetObjCode(actor);
        u16 state = FldAct_GetDrawStatus(actor);
        u8 dir = func_ov036_02197404(actor);
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
        if (state != work->anim.state || (entry->trackDir != 0 && state < 10 && dir != work->anim.dir)) {
            u8 anim = entry->anim;

            switch (entry->kind) {
            case 0:
                switch (state) {
                case 10:
                    anim = 1;
                    break;
                case 11:
                    anim = 2;
                    break;
                case 12:
                    anim = 3;
                    break;
                }
                break;
            case 1:
                switch (state) {
                case 10:
                    anim = 2;
                    break;
                case 11:
                    anim = 3;
                    break;
                default:
                    if (dir == 0) {
                        anim = 0;
                    } else if (dir == 1) {
                        anim = 1;
                    }
                    break;
                }
                break;
            case 3:
                if (state == 10) {
                    anim = 2;
                } else if (dir == 0) {
                    anim = 1;
                } else if (dir == 1) {
                    anim = 0;
                }
                break;
            case 4:
                if (state == 10) {
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
            BlActSys_SetActorAnim(sys, work->index, anim);
        }
        func_ov036_0219743c(actor, sys, work->index, changed, TRUE);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
    }
}

static u16 func_ov036_02197054(FieldActor *actor) {
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    return work->index;
}

BOOL func_ov036_02197060(FieldActor *actor, u16 *anim, u16 *step) {
    u16 objCode = FldAct_GetObjCode(actor);

    if (objCode == 0xd3 || objCode == 0xd5) {
        RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

        if (work->index != BLACT_NONE) {
            BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));

            *anim = BlActSys_GetActorAnim(sys, work->index);
            *step = BlActSys_GetActorStep(sys, work->index);
            return TRUE;
        }
        return FALSE;
    }
    *anim = 0;
    *step = 0;
    return TRUE;
}

BOOL func_ov036_021970b8(FieldActor *actor, u16 *anim, u16 *step) {
    u16 objCode = FldAct_GetObjCode(actor);

    if (objCode == 0xdd || objCode == 0xde) {
        RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

        if (work->index != BLACT_NONE) {
            BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));

            *anim = BlActSys_GetActorAnim(sys, work->index);
            *step = BlActSys_GetActorStep(sys, work->index);
            return TRUE;
        }
        return FALSE;
    }
    *anim = 0;
    *step = 0;
    return TRUE;
}

static void func_ov036_02197114(FieldActor *actor) {
    BOOL changed = FALSE;
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        u16 state = FldAct_GetDrawStatus(actor);
        VecFx32 pos;

        if (state != work->anim.state && state < 2) {
            work->anim.state = state;
            BlActSys_SetActorAnim(sys, work->index, state);
            changed = TRUE;
        }
        func_ov036_0219743c(actor, sys, work->index, changed, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
    }
}

static void func_ov036_0219718c(FieldActor *actor) {
    BOOL changed = FALSE;
    RailActorWork *work = FldAct_GetBlActWorkPtr(actor);

    if (work->index != BLACT_NONE) {
        BlActSys *sys = FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor));
        u16 objCode = FldAct_GetObjCode(actor);
        u16 state = FldAct_GetDrawStatus(actor);
        VecFx32 pos;

        if (state != work->anim.state) {
            u16 anim = 0;

            if (objCode == 0x7b) {
                switch (state) {
                case 10:
                    anim = 1;
                    break;
                case 11:
                    anim = 2;
                    break;
                case 12:
                    anim = 3;
                    break;
                }
            } else if (objCode == 0x176 && state == 13) {
                anim = 4;
            }
            work->anim.state = state;
            BlActSys_SetActorAnim(sys, work->index, anim);
            changed = TRUE;
        }
        func_ov036_0219743c(actor, sys, work->index, changed, FALSE);
        CopyActorPosAllAdd(actor, &pos);
        func_ov036_021973ec(actor, &pos);
        BlActScene_SetActorPos(BlActSys_GetScene(sys), work->index, &pos);
    }
}

BOOL func_ov036_02197238(FieldActor *actor) {
    u16 objCode = FldAct_GetObjCode(actor);

    if (objCode != 0x7b && objCode != 0x176) {
        return TRUE;
    }
    {
        u16 index = FldAct_GetBlActIdx(actor, 0);

        if (index != BLACT_NONE) {
            u16 cmd;

            if (BlActSys_GetActorLastCmd(FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor)), index, &cmd) == TRUE) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void func_ov036_02197288(FieldActor *actor) {
    u16 objCode = FldAct_GetObjCode(actor);

    if (objCode == 0x7b || objCode == 0x176) {
        u16 index = FldAct_GetBlActIdx(actor, 0);

        if (index != BLACT_NONE) {
            BlActSys_SetActorStep(FieldBlAct_GetBlActSys(FldAct_GetFieldBlAct(actor)), index, 0);
        }
    }
}

static void func_ov036_021972cc(RailActorAnim *anim) {
    MI_CpuFill8(anim, 0, 4);
    anim->dir = 8;
}

static void func_ov036_021972e0(RailActorAnim *anim, FieldActor *actor, BlActSys *sys, u16 index) {
    BOOL changed = FALSE;
    u16 animId;
    u8 dir = func_ov036_02197404(actor);
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
        BlActSys_SetActorAnim(sys, index, animId);
    } else if (state != anim->state) {
        changed = TRUE;
        if (state == 0) {
            if (anim->state != 0) {
                if (anim->pending == 0) {
                    anim->pending++;
                    changed = FALSE;
                    goto end;
                }
                {
                    u8 steps[4] = { 0, 2, 2, 0 };

                    anim->step = BlActSys_GetActorStep(sys, index);
                    if (anim->step >= 4) {
                        anim->step = 0;
                    }
                    anim->step = steps[anim->step];
                }
            }
            BlActSys_SetActorAnim(sys, index, animId);
        } else {
            if (anim->state != 0) {
                anim->step = BlActSys_GetActorStep(sys, index);
                if (anim->step >= 2) {
                    anim->step = 0;
                } else {
                    anim->step = 2;
                }
            }
            BlActSys_SetActorAnim(sys, index, animId);
            BlActSys_SetActorStep(sys, index, anim->step);
        }
        anim->state = state;
        anim->pending = 0;
    }
end:
    func_ov036_0219743c(actor, sys, index, changed, FALSE);
}

static void func_ov036_021973ec(FieldActor *actor, VecFx32 *pos) {
    pos->y += func_ov036_02181324(GetMMSysField(GetActorMModelSystem(actor)));
}

static u8 func_ov036_02197404(FieldActor *actor) {
    u16 angle = FldActSys_GetCameraAngle(GetActorMModelSystem(actor));
    u16 dir = GetActorFaceDir(actor);

    if (CheckActorFlag(actor, 0x2000)) {
        return func_ov036_021949a4(actor, angle);
    }
    return RotateDirByAngle(dir, angle);
}

static void func_ov036_0219743c(FieldActor *actor, BlActSys *sys, u16 index, BOOL changed, BOOL flag) {
    BOOL animate = TRUE;
    BOOL visible;
    u8 shown = func_ov036_0219381c(actor);

    if (!changed) {
        if (flag == 1) {
            if (shown == 1 && CheckActorFlag(actor, 0x10)) {
                animate = FALSE;
            }
        } else if (shown == 1 && func_ov036_0218ed68(actor) == 1) {
            animate = FALSE;
        }
    }
    BlActSys_SetActorAnimating(sys, index, animate);
    visible = TRUE;
    if (IsActorHidden(actor) == 1) {
        visible = FALSE;
    }
    BlActSys_SetActorVisible(sys, index, visible);
}

static void func_ov036_0219749c(FieldActor *actor) {
    u16 *work = FldAct_ResetBlActWork(actor, 2);

    *work = func_ov036_021948a0(actor);
}

static void func_ov036_021974b4(FieldActor *actor) {
    u16 *work = FldAct_GetBlActWorkPtr(actor);
    BOOL hidden;
    VecFx32 pos;
    FieldG3DObjSystem *g3d = FldAct_GetG3DObjSystem(actor);

    CopyActorPosAllAdd(actor, &pos);
    FieldG3DObjSystem_SetObjPosition(g3d, *work, &pos);
    hidden = FALSE;
    if (IsActorHidden(actor) == 1) {
        hidden = TRUE;
    }
    func_ov036_021c0154(g3d, *work, hidden);
    if (func_ov036_0218ed68(actor) == 0) {
        FieldG3DObjSystem_StepObjAnmLoop(g3d, *work, 0x1000);
    }
}

static void func_ov036_02197510(FieldActor *actor) {
    u16 *work = FldAct_GetBlActWorkPtr(actor);

    func_ov036_021948ec(actor, *work);
}

static void func_ov036_02197524(FieldActor *actor) {
    func_ov036_02197510(actor);
}

static void func_ov036_0219752c(FieldActor *actor) {
    func_ov036_0219749c(actor);
}

const BlActFuncs data_ov036_021cf940 = { func_ov036_021963ec, func_ov036_0219641c, func_ov036_02196408,
                                         func_ov036_02196408, func_ov036_021963ec, func_ov036_0219647c };
const BlActFuncs data_ov036_021cf928 = { func_ov036_021963ec, func_ov036_0219641c, func_ov036_02196408,
                                         func_ov036_02196408, func_ov036_021963ec, func_ov036_0219647c };
const BlActFuncs data_ov036_021cf850 = { func_ov036_021963ec, func_ov036_02196488, func_ov036_02196408,
                                         func_ov036_02196408, func_ov036_021963ec, func_ov036_0219647c };
const BlActFuncs data_ov036_021cf8f8 = { func_ov036_0219653c, func_ov036_0219656c, func_ov036_02196558,
                                         func_ov036_02196558, func_ov036_0219653c, func_ov036_021967c8 };
const BlActFuncs data_ov036_021cf8e0 = { func_ov036_0219653c, func_ov036_021965cc, func_ov036_02196558,
                                         func_ov036_02196558, func_ov036_0219653c, func_ov036_021967c8 };
const BlActFuncs data_ov036_021cf8c8 = { func_ov036_0219653c, func_ov036_02196680, func_ov036_02196558,
                                         func_ov036_02196558, func_ov036_0219653c, func_ov036_021967c8 };
const BlActFuncs data_ov036_021cf8b0 = { func_ov036_0219653c, func_ov036_02196728, func_ov036_02196558,
                                         func_ov036_02196558, func_ov036_0219653c, func_ov036_021967c8 };
const BlActFuncs data_ov036_021cf898 = { func_ov036_021963ec, func_ov036_021967d4, func_ov036_02196408,
                                         func_ov036_02196408, func_ov036_021963ec, func_ov036_0219647c };
const BlActFuncs data_ov036_021cf880 = { func_ov036_021963ec, func_ov036_02196870, func_ov036_02196408,
                                         func_ov036_02196408, func_ov036_021963ec, func_ov036_0219647c };
const BlActFuncs data_ov036_021cf868 = { func_ov036_02196324, func_ov036_0219633c, func_ov036_02196340,
                                         func_ov036_02196344, func_ov036_02196348, NULL };
const BlActFuncs data_ov036_021cf838 = { func_ov036_021963ec, func_ov036_02196978, func_ov036_02196408,
                                         func_ov036_02196408, func_ov036_021963ec, func_ov036_0219647c };
const BlActFuncs data_ov036_021cf910 = { func_ov036_021963ec, func_ov036_02196a94, func_ov036_02196408,
                                         func_ov036_02196408, func_ov036_021963ec, func_ov036_0219647c };
const BlActFuncs data_ov036_021cf820 = { func_ov036_0219653c, func_ov036_02196bb4, func_ov036_02196558,
                                         func_ov036_02196558, func_ov036_0219653c, func_ov036_021967c8 };
const BlActFuncs data_ov036_021cf808 = { func_ov036_02196c0c, func_ov036_02196c38, func_ov036_02196c24,
                                         func_ov036_02196c24, func_ov036_02196c0c, func_ov036_02196ed4 };
const BlActFuncs data_ov036_021cf7f0 = { func_ov036_0219653c, func_ov036_0219718c, func_ov036_02196558,
                                         func_ov036_02196558, func_ov036_0219653c, func_ov036_021967c8 };
const BlActFuncs data_ov036_021cf958 = { func_ov036_02196ee0, func_ov036_02196f10, func_ov036_02196efc,
                                         func_ov036_02196efc, func_ov036_02196ee0, func_ov036_02197054 };
const BlActFuncs data_ov036_021cf7d8 = { func_ov036_0219653c, func_ov036_02197114, func_ov036_02196558,
                                         func_ov036_02196558, func_ov036_0219653c, func_ov036_021967c8 };

const BlActFuncs data_ov036_021cf9a0 = { func_ov036_0219749c, func_ov036_021974b4, func_ov036_02197510,
                                         func_ov036_02197524, func_ov036_0219752c, NULL };
