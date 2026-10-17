#ifndef POKEBW2_FIELD_FIELD_G3DOBJ_H
#define POKEBW2_FIELD_FIELD_G3DOBJ_H

// The field's 3D objects: groups of resources and objects made from them. Names and layouts from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The archives and files of a resource group
typedef struct {
    ArcTool *mdlArc;
    ArcTool *texArc;
    ArcTool *anmArc;
    u16 mdlDatID;
    u16 texDatID;
    u16 anmDatIDs[8];
    u8 anmCount;
} FieldG3DObjResRequest;

u16 FieldG3DObjSystem_AddResGroup(FieldG3DObjSystem *sys, FieldG3DObjResRequest *req, BOOL preloadTextures);
void FieldG3DObjSystem_FreeResGroup(FieldG3DObjSystem *sys, u16 resGroupIdx);
u16 FieldG3DObjSystem_AddObj(FieldG3DObjSystem *sys, u16 resGroupIdx, u16 modelIdxInRes, const VecFx32 *pos);
void FieldG3DObjSystem_FreeObj(FieldG3DObjSystem *sys, u16 objIdx);
SRTMatrix *FieldG3DObjSystem_GetObjTransform(FieldG3DObjSystem *sys, u16 objIdx);
void FieldG3DObjSystem_SetObjPosition(FieldG3DObjSystem *sys, u16 objIdx, const VecFx32 *pos);
// The frame of one of the object's animations
void FieldG3DObjSystem_SetAnmFrame(FieldG3DObjSystem *sys, u16 objIdx, int anmIdx, fx32 frame);
fx32 FieldG3DObjSystem_GetAnmFrame(FieldG3DObjSystem *sys, u16 objIdx, u32 anmIdx);
// Hide the object, or not
void func_ov036_021c0154(FieldG3DObjSystem *sys, u16 objIdx, BOOL hidden);
// TRUE until the object's animation ends
BOOL FieldG3DObjSystem_StepObjAnm(FieldG3DObjSystem *sys, u16 objIdx, fx32 addend);
BOOL FieldG3DObjSystem_StepObjAnmLoop(FieldG3DObjSystem *sys, u16 objIdx, fx32 addend);
// Leave the object out of FieldG3DObjSystem_Draw, for its owner to draw with FieldG3DObjSystem_DrawObj, or not
void func_ov036_021c02b4(FieldG3DObjSystem *sys, u16 objIdx, BOOL flag);
void FieldG3DObjSystem_DrawObj(FieldG3DObjSystem *sys, u16 objIdx);
// A flag of the object that its owner sets, and its value
void func_ov036_021c0304(FieldG3DObjSystem *sys, u16 objIdx, BOOL value);
u32 func_ov036_021c0320(FieldG3DObjSystem *sys, u16 objIdx);
void FieldG3DObjResRequest_Clear(FieldG3DObjResRequest *req);
void FieldG3DObjResRequest_SetModel(FieldG3DObjResRequest *req, ArcTool *arc, u16 datId);
void FieldG3DObjResRequest_SetAnmArc(FieldG3DObjResRequest *req, ArcTool *arc);
void FieldG3DObjResRequest_AddAnm(FieldG3DObjResRequest *req, u16 datId);
FieldG3DObjSystem *FieldG3DObjSystem_Create(HeapID heapId, u16 resGroupCount, u16 objCount);
void FieldG3DObjSystem_Free(FieldG3DObjSystem *sys);
void FieldG3DObjSystem_SetupResources(FieldG3DObjSystem *sys);
void FieldG3DObjSystem_Draw(FieldG3DObjSystem *sys);
void FieldG3DObjSystem_SetColorPostFX(FieldG3DObjSystem *sys, void *postFx);

#endif // POKEBW2_FIELD_FIELD_G3DOBJ_H
