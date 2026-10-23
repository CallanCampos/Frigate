#include "types.h"
#include "gfl/std.h"
#include "save/resh_zek.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The save block of the Reshiram or Zekrom fused with Kyurem. The ROM has no string for the file, so the name is a
// guess (swan's getReshZekBlkAddress). Block 0x2b.

u32 ReshZekSave_GetSize(void) {
    return sizeof(ReshZekSave);
}

void ReshZekSave_Init(ReshZekSave *reshZek) {
    CityState_Init(&reshZek->cityState);
}

ReshZekSave *getReshZekBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_RESH_ZEK);
}

void ReshZekSave_SetCityState(ReshZekSave *reshZek, const CityState *src) {
    MI_CpuCopy8(src, &reshZek->cityState, sizeof(reshZek->cityState));
}

void ReshZekSave_GetCityState(ReshZekSave *reshZek, CityState *dest) {
    MI_CpuCopy8(&reshZek->cityState, dest, sizeof(reshZek->cityState));
}

PartyPkm *ReshZekSave_GetPkm(ReshZekSave *reshZek) {
    return &reshZek->pkm;
}

void ReshZekSave_SetPkm(ReshZekSave *reshZek, const PartyPkm *pkm) {
    copyPartyPkm(pkm, &reshZek->pkm);
}
