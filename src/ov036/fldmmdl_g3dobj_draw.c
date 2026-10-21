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

const FieldActorSceneNodeVTable data_ov036_021cf9a0 = { func_ov036_0219749c, func_ov036_021974b4, func_ov036_02197510,
                                                        func_ov036_02197524, func_ov036_0219752c, NULL };
