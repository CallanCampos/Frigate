#ifndef POKEBW2_FIELD_GYM_GIMMICK_H
#define POKEBW2_FIELD_GYM_GIMMICK_H

// The gym puzzles that overlay 36's gym script commands drive, each in its own overlay at the field gimmicks' shared
// load address. The GymElec and GymInsect names are swan's (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

// Overlay 92: the Nimbasa City gym's roller coaster. Overlay 36's gimmick table calls the first three
void GymElec_Init(Field *field);
void GymElec_End(Field *field);
void GymElec_Update(Field *field);
void GymElec_ReapplyProgress(Field *field);
void GymElec_SetFollower(Field *field, BOOL attach, u16 actorId, u32 index);
GameEvent *GymElec_SetProgress(GameSystem *gsys, u8 progress);
void GymElec_SetEffectsMode(Field *field, u32 mode);
void GymElec_ShowModel(Field *field, u32 mode);
void GymElec_ShowStageObject5(Field *field, u32 mode);
void GymElec_SetBrightness(Field *field, s32 brightness);

// Overlay 91: the Castelia City gym. Overlay 36's gimmick table calls the first three
void GymInsect_Init(Field *field);
void GymInsect_End(Field *field);
void GymInsect_Update(Field *field);
GameEvent *GymInsect_CheckRide(GameSystem *gsys, u8 dir);
void GymInsect_PlayObject(Field *field, u8 object);
void GymInsect_ShowEffect(Field *field);
BOOL GymInsect_IsRideAhead(GameSystem *gsys, u8 dir);

// Overlays 100 to 102: the other gyms' puzzles (overlay 94's are in gym_mistralton.h, overlay 96's in
// gym_driftveil.h, overlay 97's in gym_driftveil_lift.h, overlay 99's in gym_opelucid.h)
void func_ov101_021eee48(GameSystem *gsys);
void func_ov101_021eeee0(GameSystem *gsys);
GameEvent *func_ov102_021ef3e8(GameSystem *gsys, u8 a1);
void func_ov102_021ef6f8(GameSystem *gsys, u8 a1);
void func_ov102_021ef338(GameSystem *gsys);

#endif // POKEBW2_FIELD_GYM_GIMMICK_H
