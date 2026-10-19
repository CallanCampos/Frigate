#ifndef POKEBW2_FIELD_FIELD_EXP_OBJ_H
#define POKEBW2_FIELD_FIELD_EXP_OBJ_H

#include "types.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The field's expansion objects: 3D models that zones such as gyms add, in scenes of actors with animations

void LoadFieldExpandObjData(FieldExpObjSystem *system, const G3DSceneSetup *setup, u16 scene);
void FieldExpObj_AddScene(FieldExpObjSystem *system, const G3DSceneSetup *setup, u16 scene);
void FieldExpObj_FreeScene(FieldExpObjSystem *system, u16 scene);
void FieldExpObj_StepAllAnimations(FieldExpObjSystem *system);
G3DActor *FieldExpObj_GetActor(FieldExpObjSystem *system, u16 scene, u16 actor);
SRTMatrix *FieldExpObj_GetActorMatrixPtr(FieldExpObjSystem *system, u16 scene, u16 actor);
void FieldExpObj_SetActorHidden(FieldExpObjSystem *system, u16 scene, u16 actor, BOOL hidden);
// Sets whether FieldExpObj_Draw culls an actor by its bounding box
void FieldExpObj_SetActorCulling(FieldExpObjSystem *system, u16 scene, u16 actor, BOOL cullByBBox);
// Sets the frame step of the animations of every actor of a scene
void FieldExpObj_SetSceneFrameStep(FieldExpObjSystem *system, u16 scene, fx32 frameStep);
BOOL FieldExpObj_IsActorHidden(FieldExpObjSystem *system, u16 scene, u16 actor);
FieldExpObjAnm *FieldExpObj_GetAnmInfo(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm);
// Binds an animation to its actor, or unbinds it
void FieldExpObj_SetAnm(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm, BOOL bind);
void FieldExpObj_SetAnmFrame(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm, fx32 frame);

// Sets the frames the animation advances per step (swan names the field FrameStep)
void FieldExpObjAnm_SetFrameStep(FieldExpObjAnm *anm, fx32 step);
void FieldExpObjAnm_SetPaused(FieldExpObjAnm *anm, u8 paused);
u8 FieldExpObjAnm_IsPaused(FieldExpObjAnm *anm);
void FieldExpObjAnm_SetLooped(FieldExpObjAnm *anm, u8 looped);
BOOL FieldExpObjAnm_IsPlaybackFinished(FieldExpObjAnm *anm);
fx32 FieldExpObj_GetAnmFrame(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm);
fx32 FieldExpObjAnm_GetLastFrame(FieldExpObjAnm *anm);
FieldExpObjSystem *FieldExpObj_Create(u32 resourceLimit, u32 actorLimit, u32 heapId);
void FieldExpObj_Free(FieldExpObjSystem *system);
void FieldExpObj_Draw(FieldExpObjSystem *system);

#endif // POKEBW2_FIELD_FIELD_EXP_OBJ_H
