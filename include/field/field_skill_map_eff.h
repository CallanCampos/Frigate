#ifndef POKEBW2_FIELD_FIELD_SKILL_MAP_EFF_H
#define POKEBW2_FIELD_FIELD_SKILL_MAP_EFF_H

#include "types.h"
#include "field/field_flash.h"
#include "gfl/heap.h"
#include "struct_decls.h"

void *Field_GetSkillMapEff(Field *field);
void *FieldSkillMapEff_GetFlash(void *skillMapEff);
void *FieldSkillMapEff_Create(u32 flashPerms, HeapID heapId);
void FieldSkillMapEff_Free(void *effect);
void FieldSkillMapEff_Update(void *effect);
void FieldSkillMapEff_Draw(void *effect);

#endif // POKEBW2_FIELD_FIELD_SKILL_MAP_EFF_H
