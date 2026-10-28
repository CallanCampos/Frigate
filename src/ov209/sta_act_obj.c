#include "types.h"
#include "app/musical/sta_act_obj.h"
#include "gfl/billboard_actor.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

// Overlay 209's sta_act_obj.c: the stage's objects, billboard actors placed in pixels and scrolled with the stage

static void StaActObj_UpdateObj(StaActObjSys *sys, StaActObj *obj);

static const BOOL STA_ACT_OBJ_SHOW = TRUE;

StaActObjSys *StaActObj_InitSystem(HeapID heapId, BillboardActorScene *billboardActor) {
    u8 i;
    StaActObjSys *sys = GFL_HeapAllocate(heapId, sizeof(StaActObjSys), FALSE, "sta_act_obj.c", 79);

    sys->heapId = heapId;
    sys->billboardActor = billboardActor;
    sys->scroll = 0;
    for (i = 0; i < 5; i++) {
        sys->objs[i].active = FALSE;
        sys->objs[i].update = FALSE;
    }
    return sys;
}

void StaActObj_TermSystem(StaActObjSys *sys) {
    u8 i;

    for (i = 0; i < 5; i++) {
        if (sys->objs[i].active == TRUE) {
            StaActObj_DelObj(sys, &sys->objs[i]);
        }
    }
    GFL_HeapFree(sys);
}

void StaActObj_UpdateSystem(StaActObjSys *sys) {
    int i;

    for (i = 0; i < 5; i++) {
        if (sys->objs[i].active == TRUE) {
            StaActObj_UpdateObj(sys, &sys->objs[i]);
        }
    }
}

static void StaActObj_UpdateObj(StaActObjSys *sys, StaActObj *obj) {
    if (obj->update == TRUE) {
        VecFx32 pos;
        fx32 scroll = FX32_CONST(sys->scroll);

        pos.x = (obj->pos.x - scroll) / 16;
        pos.y = (FX32_CONST(192) - obj->pos.y) / 16;
        pos.z = obj->pos.z;
        BillboardActorScene_SetActorPos(sys->billboardActor, obj->actor, &pos);
        obj->update = FALSE;
    }
}

void StaActObj_SetScrollOffset(StaActObjSys *sys, u16 scroll) {
    u8 i;

    sys->scroll = scroll;
    for (i = 0; i < 5; i++) {
        if (sys->objs[i].active == TRUE) {
            sys->objs[i].update = TRUE;
        }
    }
}

StaActObj *StaActObj_AddObj(StaActObjSys *sys, u16 objId) {
    u8 i;
    StaActObj *obj;

    for (i = 0; i < 5; i++) {
        if (sys->objs[i].active == FALSE) {
            break;
        }
    }
    obj = &sys->objs[i];
    sys->objs[i].active = TRUE;
    obj->update = TRUE;
    obj->pos.x = 0;
    obj->pos.y = 0;
    obj->pos.z = 0;
    obj->actor =
        BillboardActorScene_AddNewActor(sys->billboardActor, obj->material, FX32_ONE, FX32_ONE, &obj->pos, 31, 0, 0);
    BillboardActorScene_SetActorHidden(sys->billboardActor, obj->actor, &STA_ACT_OBJ_SHOW);
    return obj;
}

void StaActObj_DelObj(StaActObjSys *sys, StaActObj *obj) {
    BillboardActorScene_ClearActorMaterial(sys->billboardActor, obj->actor);
    BillboardActorScene_FreeMaterial(sys->billboardActor, obj->material);
    obj->active = FALSE;
}

void StaActObj_SetPosition(StaActObjSys *sys, StaActObj *obj, VecFx32 *pos) {
    obj->pos.x = pos->x;
    obj->pos.y = pos->y;
    obj->pos.z = pos->z;
    obj->update = TRUE;
}

void StaActObj_GetPosition(StaActObjSys *sys, StaActObj *obj, VecFx32 *pos) {
    pos->x = obj->pos.x;
    pos->y = obj->pos.y;
    pos->z = obj->pos.z;
}

void StaActObj_SetShowFlg(StaActObjSys *sys, StaActObj *obj, BOOL show) {
    BillboardActorScene_SetActorHidden(sys->billboardActor, obj->actor, &show);
}
