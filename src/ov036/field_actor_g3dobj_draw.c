// The scene node of the actors drawn as a 3D model rather than a billboard (FIELD_ACTOR_MAIN_VTABLES' twelfth): it
// adds the actor's model when the node is created or woken up, places it and steps its animation every frame, and
// removes it when the node is deleted or suspended. The ROM gives this file no name, so field_actor_g3dobj_draw.c,
// after field_actor_g3dobj.c whose models it draws, is a guess. It is a file of its own because its table lies after
// field_rail_loader.c's 48-byte RAIL_OBJ_ANIMS, past the end of that file's size-sorted .rodata
#include "types.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/field_g3dobj.h"
#include "nitro/fx.h"

static void FieldActorG3DObjDraw_Create(FieldActor *actor) {
    u16 *work = FldAct_ResetBillboardActorWork(actor, 2);

    *work = FieldActorG3DSystem_AddObj(actor);
}

static void FieldActorG3DObjDraw_Update(FieldActor *actor) {
    u16 *work = FldAct_GetBillboardActorWorkPtr(actor);
    BOOL hidden;
    VecFx32 pos;
    FieldG3DObjSystem *g3d = FldAct_GetG3DObjSystem(actor);

    CopyActorPosAllAdd(actor, &pos);
    FieldG3DObjSystem_SetObjPosition(g3d, *work, &pos);
    hidden = FALSE;
    if (IsActorHidden(actor) == TRUE) {
        hidden = TRUE;
    }
    FieldG3DObjSystem_SetObjectHidden(g3d, *work, hidden);
    if (!FieldAcmdTCB_CheckAnimePaused(actor)) {
        FieldG3DObjSystem_StepObjAnmLoop(g3d, *work, 0x1000);
    }
}

static void FieldActorG3DObjDraw_Delete(FieldActor *actor) {
    u16 *work = FldAct_GetBillboardActorWorkPtr(actor);

    FieldActorG3DSystem_DelObj(actor, *work);
}

static void FieldActorG3DObjDraw_Suspend(FieldActor *actor) {
    FieldActorG3DObjDraw_Delete(actor);
}

static void FieldActorG3DObjDraw_WakeUp(FieldActor *actor) {
    FieldActorG3DObjDraw_Create(actor);
}

const FieldActorSceneNodeVTable FIELD_ACTOR_VTABLE_G3D_OBJ = {
    FieldActorG3DObjDraw_Create,  FieldActorG3DObjDraw_Update, FieldActorG3DObjDraw_Delete,
    FieldActorG3DObjDraw_Suspend, FieldActorG3DObjDraw_WakeUp, NULL
};
