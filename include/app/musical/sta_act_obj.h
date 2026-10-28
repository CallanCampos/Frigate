#ifndef POKEBW2_APP_MUSICAL_STA_ACT_OBJ_H
#define POKEBW2_APP_MUSICAL_STA_ACT_OBJ_H

// Overlay 209's sta_act_obj.c: the stage's objects, billboard actors that scroll with the stage

#include "types.h"
#include "gfl/billboard_actor.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

typedef struct {
    BOOL active;
    // Whether its actor needs its position again
    BOOL update;
    VecFx32 pos;
    u32 material;
    u32 actor;
} StaActObj;

struct StaActObjSys {
    HeapID heapId;
    // How far the stage has scrolled, in pixels
    u16 scroll;
    BillboardActorScene *billboardActor;
    StaActObj objs[5];
};

StaActObjSys *StaActObj_InitSystem(HeapID heapId, BillboardActorScene *billboardActor);
void StaActObj_TermSystem(StaActObjSys *sys);
void StaActObj_UpdateSystem(StaActObjSys *sys);
void StaActObj_SetScrollOffset(StaActObjSys *sys, u16 scroll);
// The id is not used, and the actor is made with the slot's material, which nothing in this file sets
StaActObj *StaActObj_AddObj(StaActObjSys *sys, u16 objId);
void StaActObj_DelObj(StaActObjSys *sys, StaActObj *obj);
void StaActObj_SetPosition(StaActObjSys *sys, StaActObj *obj, VecFx32 *pos);
void StaActObj_GetPosition(StaActObjSys *sys, StaActObj *obj, VecFx32 *pos);
void StaActObj_SetShowFlg(StaActObjSys *sys, StaActObj *obj, BOOL show);

#endif // POKEBW2_APP_MUSICAL_STA_ACT_OBJ_H
