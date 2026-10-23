#ifndef POKEBW2_FIELD_FIELD_SKILL_MAP_EFF_H
#define POKEBW2_FIELD_FIELD_SKILL_MAP_EFF_H

// Overlay 36's fieldskill_mapeff.c: the map effects of the field moves, Flash's darkness

#include "types.h"
#include "field/field_flash.h"
#include "gfl/heap.h"
#include "struct_decls.h"

FieldSkillMapEff *Field_GetSkillMapEff(Field *field);
FieldFlash *FieldSkillMapEff_GetFlash(FieldSkillMapEff *effect);
FieldSkillMapEff *FieldSkillMapEff_Create(u32 flashPerms, HeapID heapId);
void FieldSkillMapEff_Free(FieldSkillMapEff *effect);
void FieldSkillMapEff_Update(FieldSkillMapEff *effect);
void FieldSkillMapEff_Draw(FieldSkillMapEff *effect);

#endif // POKEBW2_FIELD_FIELD_SKILL_MAP_EFF_H
