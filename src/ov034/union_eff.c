// The Union Room's effects on the field (the file's name is from its heap allocation): a focus on the player that the
// others look at, and the marks over the players who are in a trade or a battle, drawn as models of one archive. The
// work is allocated by UnionEff_Create and kept by the field. Before them, three checks for the Union Room's actors
#include "field/union_eff.h"
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/player_state.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "save/wifi_list.h"
#include "system/game_system.h"

// The archive of the effects' models and animations
#define UNION_EFF_ARC_ID 0x58
#define UNION_EFF_FOCUS_NSBMD 0x14
#define UNION_EFF_FOCUS_NSBCA 0x15
#define UNION_EFF_MARK_TOMO_NSBMD 0x16
#define UNION_EFF_MARK_ASONDA_NSBMD 0x17

#define FOCUS_SLOT_MAX 1
#define MARK_SLOT_MAX 10
// The frames that the focus is shown for
#define FOCUS_FRAMES 0x5A

// One effect on an actor
typedef struct {
    FieldActor *actor;
    G3DModel *model;
    void *anm;
    G3DActor *g3dActor;
    u16 serial; // the oldest is freed when there is no room
    u16 frame;
} UnionEffSlot;

typedef struct {
    UnionEffSlot slot[FOCUS_SLOT_MAX];
    void *res_nsbmd;
    void *res_nsbca;
    u16 serial;
} FocusMan;

typedef struct {
    UnionEffSlot slot[MARK_SLOT_MAX];
    void *res_nsbmd_tomo;
    void *res_nsbmd_asonda;
    u16 serial;
} MarkMan;

typedef struct {
    FocusMan focus;
    MarkMan mark;
} UnionEffect;

static void UnionEff_LoadFocus(FocusMan *focus_man, HeapID heapId);
static void UnionEff_UnloadFocus(FocusMan *focus_man);
static void UnionEff_StartFocus(FocusMan *focus_man, FieldActor *actor);
static void UnionEff_InitFocusSlot(FocusMan *focus_man, UnionEffSlot *slot, FieldActor *actor, u16 serial);
static void UnionEff_FreeFocusSlot(UnionEffSlot *slot);
static void UnionEff_StepFocusSlot(UnionEffSlot *slot);
static void UnionEff_DrawFocusSlot(UnionEffSlot *slot, SRTMatrix *mtx);
static void UnionEff_ExitFocus(FocusMan *focus_man);
static void UnionEff_UpdateFocus(FocusMan *focus_man);
static void UnionEff_DrawFocus(FocusMan *focus_man);
static UnionEffSlot *UnionEff_GetFreeFocusSlot(FocusMan *focus_man);
static void UnionEff_LoadMarks(MarkMan *mark_man, HeapID heapId);
static void UnionEff_UnloadMarks(MarkMan *mark_man);
static void UnionEff_StartMark(MarkMan *mark_man, FieldActor *actor, BOOL asonda);
static void UnionEff_InitMarkSlot(MarkMan *mark_man, UnionEffSlot *slot, FieldActor *actor, u16 serial, BOOL asonda);
static UnionEffSlot *UnionEff_GetFreeMarkSlot(MarkMan *mark_man);
static void UnionEff_FreeMarkSlot(UnionEffSlot *slot);
static void UnionEff_StepMarkSlot(UnionEffSlot *slot);
static void UnionEff_DrawMarkSlot(UnionEffSlot *slot, SRTMatrix *mtx);
static void UnionEff_ExitMarks(MarkMan *mark_man);
static void UnionEff_UpdateMarks(MarkMan *mark_man);
static void UnionEff_DrawMarks(MarkMan *mark_man);

BOOL UnionEff_IsPlayerOnSpot(Field *field) {
    static const s16 spots[1][2] = { { 11, 15 } };
    FieldPlayer *player = Field_GetPlayer(field);
    PlayerState *state = GSYS_GetPlayerState(Field_GetGameSystem(field));
    VecFx32 pos;
    u32 i;

    FieldPlayer_GetWPos(player, &pos);
    pos.x = (pos.x >> 4) / FX32_ONE;
    pos.z = (pos.z >> 4) / FX32_ONE;
    func_0201754c(state);
    for (i = 0; i < 1; i++) {
        if (pos.x == spots[i][0] && pos.z == spots[i][1]) {
            return TRUE;
        }
    }
    return FALSE;
}

int UnionEff_GetKindStep(int kind) {
    if (kind >= 4 && kind <= 0x17) {
        return 1;
    }
    switch (kind) {
    case 3:
        return 4;
    case 0x18:
        return 2;
    case 0x19:
        return 5;
    case 0x1a:
        return 3;
    default:
        return 0;
    }
}

BOOL UnionEff_IsFriendBeacon(WifiList *wifiList, const UnionBeacon *beacon) {
    u32 friendIndex;

    if (func_0200a46c(wifiList, beacon->name, beacon->trainerId, beacon->gender, &friendIndex) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

void *UnionEff_Create(HeapID heapId) {
    UnionEffect *eff = GFL_HeapAllocate(heapId, sizeof(UnionEffect), TRUE, "union_eff.c", 142);

    UnionEff_LoadFocus(&eff->focus, heapId);
    UnionEff_LoadMarks(&eff->mark, heapId);
    return eff;
}

void UnionEff_Free(void *work) {
    UnionEffect *eff = work;

    if (eff != NULL) {
        UnionEff_ExitMarks(&eff->mark);
        UnionEff_UnloadMarks(&eff->mark);
        UnionEff_ExitFocus(&eff->focus);
        UnionEff_UnloadFocus(&eff->focus);
        GFL_HeapFree(eff);
    }
}

void UnionEff_Update(void *work) {
    UnionEffect *eff = work;

    if (eff != NULL) {
        UnionEff_UpdateMarks(&eff->mark);
        UnionEff_UpdateFocus(&eff->focus);
    }
}

void UnionEff_Draw(void *work) {
    UnionEffect *eff = work;

    if (eff != NULL) {
        UnionEff_DrawMarks(&eff->mark);
        UnionEff_DrawFocus(&eff->focus);
    }
}

void UnionEff_AddFocus(void *work, FieldActor *actor) {
    UnionEffect *eff = work;

    if (eff != NULL) {
        UnionEff_StartFocus(&eff->focus, actor);
    }
}

void UnionEff_AddMark(void *work, FieldActor *actor, BOOL asonda) {
    UnionEffect *eff = work;

    if (eff != NULL) {
        UnionEff_StartMark(&eff->mark, actor, asonda);
    }
}

void UnionEff_RemoveMark(void *work, FieldActor *actor) {
    UnionEffect *eff = work;
    MarkMan *mark_man = &eff->mark;
    int i;

    for (i = 0; i < MARK_SLOT_MAX; i++) {
        if (mark_man->slot[i].actor == actor) {
            UnionEff_FreeMarkSlot(&mark_man->slot[i]);
            return;
        }
    }
}

static void UnionEff_LoadFocus(FocusMan *focus_man, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(UNION_EFF_ARC_ID, heapId);
    BOOL ret;

    focus_man->res_nsbmd = GFL_G3DSysReadArcToolResource(arc, UNION_EFF_FOCUS_NSBMD);
    focus_man->res_nsbca = GFL_G3DSysReadArcToolResource(arc, UNION_EFF_FOCUS_NSBCA);
    ret = GFL_G3DResUploadTexData(focus_man->res_nsbmd);
    GFL_ASSERT(ret);
    GFL_ArcToolFree(arc);
}

static void UnionEff_UnloadFocus(FocusMan *focus_man) {
    GFL_G3DResFreeTexData(focus_man->res_nsbmd);
    GFL_G3DResFree(focus_man->res_nsbca);
    focus_man->res_nsbca = NULL;
    GFL_G3DResFree(focus_man->res_nsbmd);
    focus_man->res_nsbmd = NULL;
}

static void UnionEff_StartFocus(FocusMan *focus_man, FieldActor *actor) {
    UnionEffSlot *slot;

    GFL_ASSERT(focus_man->res_nsbmd != NULL);
    slot = UnionEff_GetFreeFocusSlot(focus_man);
    UnionEff_InitFocusSlot(focus_man, slot, actor, focus_man->serial);
    focus_man->serial++;
}

static void UnionEff_InitFocusSlot(FocusMan *focus_man, UnionEffSlot *slot, FieldActor *actor, u16 serial) {
    sys_memset(slot, 0, sizeof(UnionEffSlot));
    slot->actor = actor;
    slot->serial = serial;
    slot->model = GFL_G3DMdlCreate(focus_man->res_nsbmd, 0, focus_man->res_nsbmd);
    slot->anm = GFL_G3DAnmCreate(slot->model, focus_man->res_nsbca, 0);
    slot->g3dActor = GFL_G3DActorCreate(slot->model, &slot->anm, 1);
    GFL_G3DActorBindAnm(slot->g3dActor, 0);
}

static void UnionEff_FreeFocusSlot(UnionEffSlot *slot) {
    GFL_G3DAnmFree(slot->anm);
    GFL_G3DActorFree(slot->g3dActor);
    GFL_G3DMdlFree(slot->model);
    sys_memset(slot, 0, sizeof(UnionEffSlot));
}

static void UnionEff_StepFocusSlot(UnionEffSlot *slot) {
    GFL_G3DActorStepAnmFrameLoop(slot->g3dActor, 0, FX32_ONE);
    slot->frame++;
    if (slot->frame > FOCUS_FRAMES) {
        UnionEff_FreeFocusSlot(slot);
    }
}

static void UnionEff_DrawFocusSlot(UnionEffSlot *slot, SRTMatrix *mtx) {
    CopyActorWPos(slot->actor, &mtx->translation);
    mtx->translation.y -= 6 * FX32_ONE;
    GFL_G3DSysDrawObjBBoxCull(slot->g3dActor, mtx);
}

static void UnionEff_ExitFocus(FocusMan *focus_man) {
    if (focus_man->slot[0].g3dActor != NULL) {
        UnionEff_FreeFocusSlot(&focus_man->slot[0]);
    }
}

static void UnionEff_UpdateFocus(FocusMan *focus_man) {
    UnionEffSlot *slot = &focus_man->slot[0];

    if (slot->g3dActor != NULL) {
        if (func_ov012_0216749c(slot->actor) == TRUE) {
            UnionEff_StepFocusSlot(slot);
        } else {
            UnionEff_FreeFocusSlot(slot);
        }
    }
}

static void UnionEff_DrawFocus(FocusMan *focus_man) {
    SRTMatrix mtx = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE }, { 0 } };

    MTX_Identity33_(&mtx.rotation);
    if (focus_man->slot[0].g3dActor != NULL && func_ov012_0216749c(focus_man->slot[0].actor) == TRUE) {
        UnionEff_DrawFocusSlot(&focus_man->slot[0], &mtx);
    }
}

static UnionEffSlot *UnionEff_GetFreeFocusSlot(FocusMan *focus_man) {
    int i;
    int oldest = 0;

    for (i = 0; i < FOCUS_SLOT_MAX; i++) {
        if (focus_man->slot[i].g3dActor == NULL) {
            return &focus_man->slot[i];
        }
        if (focus_man->slot[i].serial < focus_man->slot[oldest].serial) {
            oldest = i;
        }
    }
    UnionEff_FreeFocusSlot(&focus_man->slot[oldest]);
    return &focus_man->slot[oldest];
}

static void UnionEff_LoadMarks(MarkMan *mark_man, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(UNION_EFF_ARC_ID, heapId);
    BOOL ret;

    mark_man->res_nsbmd_tomo = GFL_G3DSysReadArcToolResource(arc, UNION_EFF_MARK_TOMO_NSBMD);
    mark_man->res_nsbmd_asonda = GFL_G3DSysReadArcToolResource(arc, UNION_EFF_MARK_ASONDA_NSBMD);
    ret = GFL_G3DResUploadTexData(mark_man->res_nsbmd_tomo);
    GFL_ASSERT(ret);
    ret = GFL_G3DResUploadTexData(mark_man->res_nsbmd_asonda);
    GFL_ASSERT(ret);
    GFL_ArcToolFree(arc);
}

static void UnionEff_UnloadMarks(MarkMan *mark_man) {
    GFL_G3DResFreeTexData(mark_man->res_nsbmd_tomo);
    GFL_G3DResFreeTexData(mark_man->res_nsbmd_asonda);
    GFL_G3DResFree(mark_man->res_nsbmd_tomo);
    GFL_G3DResFree(mark_man->res_nsbmd_asonda);
    mark_man->res_nsbmd_tomo = NULL;
    mark_man->res_nsbmd_asonda = NULL;
}

static void UnionEff_StartMark(MarkMan *mark_man, FieldActor *actor, BOOL asonda) {
    UnionEffSlot *slot;

    GFL_ASSERT(mark_man->res_nsbmd_tomo != NULL);
    GFL_ASSERT(mark_man->res_nsbmd_asonda != NULL);
    slot = UnionEff_GetFreeMarkSlot(mark_man);
    UnionEff_InitMarkSlot(mark_man, slot, actor, mark_man->serial, asonda);
    mark_man->serial++;
}

static void UnionEff_InitMarkSlot(MarkMan *mark_man, UnionEffSlot *slot, FieldActor *actor, u16 serial, BOOL asonda) {
    sys_memset(slot, 0, sizeof(UnionEffSlot));
    slot->actor = actor;
    slot->serial = serial;
    if (asonda == FALSE) {
        slot->model = GFL_G3DMdlCreate(mark_man->res_nsbmd_tomo, 0, mark_man->res_nsbmd_tomo);
    } else {
        slot->model = GFL_G3DMdlCreate(mark_man->res_nsbmd_asonda, 0, mark_man->res_nsbmd_asonda);
    }
    slot->g3dActor = GFL_G3DActorCreate(slot->model, NULL, 0);
}

static void UnionEff_FreeMarkSlot(UnionEffSlot *slot) {
    GFL_G3DActorFree(slot->g3dActor);
    GFL_G3DMdlFree(slot->model);
    sys_memset(slot, 0, sizeof(UnionEffSlot));
}

static void UnionEff_StepMarkSlot(UnionEffSlot *slot) {
}

static void UnionEff_DrawMarkSlot(UnionEffSlot *slot, SRTMatrix *mtx) {
    CopyActorWPos(slot->actor, &mtx->translation);
    mtx->translation.y -= FX32_CONST(6.5);
    GFL_G3DSysDrawObjBBoxCull(slot->g3dActor, mtx);
}

static void UnionEff_ExitMarks(MarkMan *mark_man) {
    int i;

    for (i = 0; i < MARK_SLOT_MAX; i++) {
        if (mark_man->slot[i].g3dActor != NULL) {
            UnionEff_FreeMarkSlot(&mark_man->slot[i]);
        }
    }
}

static void UnionEff_UpdateMarks(MarkMan *mark_man) {
    int i;

    for (i = 0; i < MARK_SLOT_MAX; i++) {
        UnionEffSlot *slot = &mark_man->slot[i];

        if (slot->g3dActor != NULL) {
            if (func_ov012_0216749c(slot->actor) == TRUE) {
                UnionEff_StepMarkSlot(slot);
            } else {
                UnionEff_FreeMarkSlot(slot);
            }
        }
    }
}

static void UnionEff_DrawMarks(MarkMan *mark_man) {
    SRTMatrix mtx = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE }, { 0 } };
    int i;

    MTX_Identity33_(&mtx.rotation);
    for (i = 0; i < MARK_SLOT_MAX; i++) {
        UnionEffSlot *slot = &mark_man->slot[i];

        if (slot->g3dActor != NULL && func_ov012_0216749c(slot->actor) == TRUE) {
            UnionEff_DrawMarkSlot(slot, &mtx);
        }
    }
}

static UnionEffSlot *UnionEff_GetFreeMarkSlot(MarkMan *mark_man) {
    int i;
    int oldest = 0;

    for (i = 0; i < MARK_SLOT_MAX; i++) {
        if (mark_man->slot[i].g3dActor == NULL) {
            return &mark_man->slot[i];
        }
        if (mark_man->slot[i].serial < mark_man->slot[oldest].serial) {
            oldest = i;
        }
    }
    UnionEff_FreeMarkSlot(&mark_man->slot[oldest]);
    return &mark_man->slot[oldest];
}
