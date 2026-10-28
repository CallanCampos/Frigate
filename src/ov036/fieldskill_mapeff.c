// The map effects of the field moves, of which there is only Flash's darkness. The name is the ROM's string
#include "types.h"
#include "field/field_flash.h"
#include "field/field_skill_map_eff.h"
#include "gfl/heap.h"

struct FieldSkillMapEff {
    FieldFlash *flash;
};

void FieldSkillMapEff_SetupFlash(FieldSkillMapEff *effect, u32 flashPerms, HeapID heapId);
void FieldSkillMapEff_FreeFlash(FieldSkillMapEff *effect);
void FieldSkillMapEff_UpdateCore(FieldSkillMapEff *effect);
void FieldSkillMapEff_DrawCore(FieldSkillMapEff *effect);

FieldSkillMapEff *FieldSkillMapEff_Create(u32 flashPerms, HeapID heapId) {
    FieldSkillMapEff *effect = GFL_HeapAllocate(heapId, sizeof(FieldSkillMapEff), TRUE, "fieldskill_mapeff.c", 73);

    FieldSkillMapEff_SetupFlash(effect, flashPerms, heapId);
    return effect;
}

void FieldSkillMapEff_Free(FieldSkillMapEff *effect) {
    FieldSkillMapEff_FreeFlash(effect);
    GFL_HeapFree(effect);
}

void FieldSkillMapEff_Update(FieldSkillMapEff *effect) {
    FieldSkillMapEff_UpdateCore(effect);
}

void FieldSkillMapEff_Draw(FieldSkillMapEff *effect) {
    FieldSkillMapEff_DrawCore(effect);
}

FieldFlash *FieldSkillMapEff_GetFlash(FieldSkillMapEff *effect) {
    return effect->flash;
}

void FieldSkillMapEff_SetupFlash(FieldSkillMapEff *effect, u32 flashPerms, HeapID heapId) {
    if (flashPerms & 3) {
        effect->flash = FieldFlash_Create(heapId);
        if (flashPerms & 2) {
            FieldFlash_RequestState(effect->flash, FIELD_FLASH_REQUEST_LIT);
        } else {
            FieldFlash_RequestState(effect->flash, FIELD_FLASH_REQUEST_DARK);
        }
    } else {
        effect->flash = NULL;
    }
}

void FieldSkillMapEff_FreeFlash(FieldSkillMapEff *effect) {
    FieldFlash_Free(effect->flash);
    effect->flash = NULL;
}

void FieldSkillMapEff_UpdateCore(FieldSkillMapEff *effect) {
    FieldFlash_Update(effect->flash);
}

void FieldSkillMapEff_DrawCore(FieldSkillMapEff *effect) {
    FieldFlash_Draw(effect->flash);
}
