#include "types.h"
#include "gfl/std.h"
#include "save/resh_zek.h"
#include "save/save_control.h"

// The save block of the Reshiram or Zekrom fused with Kyurem. The ROM has no string for the file, so the name is a
// guess (swan's getReshZekBlkAddress). Block 0x2b.

#define SAVE_BLOCK_RESH_ZEK 0x2b

u32 ReshZekSave_GetSize(void) {
    return sizeof(ReshZekSave);
}

void ReshZekSave_Init(ReshZekSave *reshZek) {
    ReshZekSave_UnresolvedInitTarget(reshZek);
}

ReshZekSave *getReshZekBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_RESH_ZEK);
}

void func_0200af8c(ReshZekSave *reshZek, const u32 *src) {
    MI_CpuCopy8(src, &reshZek->cityState, sizeof(reshZek->cityState));
}

void func_0200af9c(ReshZekSave *reshZek, u32 *dest) {
    MI_CpuCopy8(&reshZek->cityState, dest, sizeof(reshZek->cityState));
}

PartyPkm *ReshZekSave_GetPkm(ReshZekSave *reshZek) {
    return &reshZek->pkm;
}

void ReshZekSave_SetPkm(ReshZekSave *reshZek, const PartyPkm *pkm) {
    copyPartyPkm(pkm, &reshZek->pkm);
}
