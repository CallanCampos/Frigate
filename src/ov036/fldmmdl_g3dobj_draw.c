// The scene node of the actors drawn as a 3D model rather than a billboard (FIELD_ACTOR_MAIN_VTABLES' twelfth): it
// adds the actor's model when the node is created or woken up, places it and steps its animation every frame, and
// removes it when the node is deleted or suspended. The ROM gives this file no name, so fldmmdl_g3dobj_draw.c, after
// fldmmdl_g3dobj.c whose models it draws, is a guess. It is a file of its own because its table lies after
// field_rail_loader.c's 48-byte RAIL_OBJ_ANIMS, past the end of that file's size-sorted .rodata
#include "types.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/field_g3dobj.h"
#include "nitro/fx.h"

static void FldmmdlDraw_InitBlactId(FieldActor *actor) {
    u16 *work = FldAct_ResetBlActWork(actor, 2);

    *work = FieldActorG3DSystem_AddObj(actor);
}

static void FldmmdlDraw_DrawEffect(FieldActor *actor) {
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
    FieldG3DObjSystem_SetObjectHidden(g3d, *work, hidden);
    if (FieldAcmdTCB_CheckAnimePaused(actor) == 0) {
        FieldG3DObjSystem_StepObjAnmLoop(g3d, *work, 0x1000);
    }
}

static void func_ov036_02197510(FieldActor *actor) {
    u16 *work = FldAct_GetBlActWorkPtr(actor);

    FieldActorG3DSystem_DelObj(actor, *work);
}

static void func_ov036_02197524(FieldActor *actor) {
    func_ov036_02197510(actor);
}

static void func_ov036_0219752c(FieldActor *actor) {
    FldmmdlDraw_InitBlactId(actor);
}

const FieldActorSceneNodeVTable data_ov036_021cf9a0 = { FldmmdlDraw_InitBlactId, FldmmdlDraw_DrawEffect,
                                                        func_ov036_02197510,     func_ov036_02197524,
                                                        func_ov036_0219752c,     NULL };
