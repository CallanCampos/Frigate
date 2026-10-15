#ifndef POKEBW2_FIELD_FIELD_EXP_OBJ_H
#define POKEBW2_FIELD_FIELD_EXP_OBJ_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/g3d.h"
#include "struct_decls.h"

// The field's expansion objects: 3D models that zones such as gyms add, in scenes of actors with animations

void LoadFieldExpandObjData(FieldExpObjSystem *system, const G3DSceneSetup *setup, u32 scene);
void FieldExpObj_AddScene(FieldExpObjSystem *system, const G3DSceneSetup *setup, u32 scene);
void FieldExpObj_FreeScene(FieldExpObjSystem *system, u32 scene);
void FieldExpObj_StepAllAnimations(FieldExpObjSystem *system);
G3DActor *FieldExpObj_GetActor(FieldExpObjSystem *system, u16 scene, u16 actor);
SRTMatrix *FieldExpObj_GetActorMatrixPtr(FieldExpObjSystem *system, u16 scene, u16 actor);
void FieldExpObj_SetActorHidden(FieldExpObjSystem *system, u16 scene, u16 actor, BOOL hidden);
// Sets the first word of an actor's state
void func_ov036_021b8248(FieldExpObjSystem *system, u16 scene, u16 actor, u32 value);
// Sets the frame step of the animations of every actor of a scene
void func_ov036_021b82ac(FieldExpObjSystem *system, u16 scene, fx32 frameStep);
u32 func_ov036_021b8268(FieldExpObjSystem *system, u16 scene, u16 actor);
FieldExpObjAnm *FieldExpObj_GetAnmInfo(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm);
void FieldExpObj_SetAnm(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm, BOOL a4);
void FieldExpObj_SetAnmFrame(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm, fx32 frame);

// Sets the frames the animation advances per step (swan names the field FrameStep)
void FieldExpObjAnm_SetFrameStep(FieldExpObjAnm *anm, fx32 step);
void FieldExpObjAnm_SetPaused(FieldExpObjAnm *anm, u8 paused);
u8 func_ov036_021b84ec(FieldExpObjAnm *anm);
void FieldExpObjAnm_SetLooped(FieldExpObjAnm *anm, u8 looped);
BOOL FieldExpObjAnm_IsPlaybackFinished(FieldExpObjAnm *anm);
u32 func_ov036_021b8520(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm);
// The animation's last frame
fx32 func_ov036_021b8580(FieldExpObjAnm *anm);
FieldExpObjSystem *FieldExpObj_Create(u32 a0, u32 a1, HeapID heapId);
void FieldExpObj_Free(FieldExpObjSystem *system);
void FieldExpObj_Draw(FieldExpObjSystem *system);

#endif // POKEBW2_FIELD_FIELD_EXP_OBJ_H
