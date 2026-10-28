#ifndef POKEBW2_FIELD_FLD_WIPE_3DOBJ_H
#define POKEBW2_FIELD_FLD_WIPE_3DOBJ_H

// Overlay 36's fld_wipe_3dobj.c: the sphere model of field_flash.c's darkness, and the script commands that play a
// field effect

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The sphere, drawn around the camera at a scale, with an alpha from 0 to 31
FieldFlashG3D *FieldFlashG3D_Create(HeapID heapId);
void FieldFlashG3D_Free(FieldFlashG3D *flash);
void FieldFlashG3D_Draw(FieldFlashG3D *flash, fx32 scale, u32 alpha);
BOOL Scrcmd_FldWipe3dobj_PlayFieldEffect(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FldWipe3dobj_PlayHMCutInEffect(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FldWipe3dobj_PlayAlderFlyEffect(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FldWipe3dobj_FieldEffect(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_FLD_WIPE_3DOBJ_H
