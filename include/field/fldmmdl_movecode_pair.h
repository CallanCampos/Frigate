#ifndef POKEBW2_FIELD_FLDMMDL_MOVECODE_PAIR_H
#define POKEBW2_FIELD_FLDMMDL_MOVECODE_PAIR_H

// Overlay 36's fldmmdl_movecode_pair.c, whose name is a guess: the move codes that follow another actor or the
// player, hide an actor under a field effect, turn it and hop it

#include "types.h"
#include "struct_decls.h"

// The functions of the move codes, which fldmmdl_movedata.c's tables list
void FldmmdlMove_InitLookPlayer(FieldActor *actor);
void func_ov036_02192a78(FieldActor *actor);
void func_ov036_02192aa8(FieldActor *actor);
void FldmmdlMove_InitLookFellow(FieldActor *actor);
void func_ov036_02192cd0(FieldActor *actor);
void func_ov036_02192d00(FieldActor *actor);
void FldmmdlMove_ClearFellowTracking(FieldActor *actor);
void func_ov036_02192fd4(FieldActor *actor);
void func_ov036_02192fe0(FieldActor *actor);
void func_ov036_02192fec(FieldActor *actor);
void func_ov036_02192ff8(FieldActor *actor);
void func_ov036_02193004(FieldActor *actor);
void func_ov036_02193010(FieldActor *actor);
void func_ov036_02193034(FieldActor *actor);
void FldmmdlMove_RecoverHide(FieldActor *actor);
void func_ov036_02193150(FieldActor *actor);
void func_ov036_0219315c(FieldActor *actor);
void func_ov036_02193168(FieldActor *actor);
void func_ov036_02193174(FieldActor *actor);
void func_ov036_02193180(FieldActor *actor);
void func_ov036_0219318c(FieldActor *actor);
void func_ov036_02193198(FieldActor *actor);
void func_ov036_021931a4(FieldActor *actor);
void func_ov036_021931b0(FieldActor *actor);
void func_ov036_021932bc(FieldActor *actor);
void func_ov036_021932c8(FieldActor *actor);
void func_ov036_021932d4(FieldActor *actor);
void func_ov036_021932e0(FieldActor *actor);
void func_ov036_021932ec(FieldActor *actor);

// The field effect that the actor hides in, and the setting of the flag that it has one
FieldEffectTask *FldmmdlMove_GetHideEffect(FieldActor *actor);
void FldmmdlMove_SetHideNoEffect(FieldActor *actor);

#endif // POKEBW2_FIELD_FLDMMDL_MOVECODE_PAIR_H
