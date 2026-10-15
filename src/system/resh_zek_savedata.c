#include "types.h"
#include "save/resh_zek.h"
#include "gfl/std.h"
#include "save/save_control.h"

// The save block of the Reshiram or Zekrom fused with Kyurem. The ROM has no string for the file, so the name is a
// guess (swan's getReshZekBlkAddress). Block 0x2b.

#define SAVE_BLOCK_RESH_ZEK 0x2b

u32 func_0200af74(void) {
    return sizeof(ReshZekSave);
}

void func_0200af78(ReshZekSave *reshZek) {
    ReshZekSave_UnresolvedInitTarget(reshZek);
}

ReshZekSave *getReshZekBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_RESH_ZEK);
}

void func_0200af8c(ReshZekSave *reshZek, const u32 *src) {
    sys_memcpy(src, &reshZek->unk0, sizeof(reshZek->unk0));
}

void func_0200af9c(ReshZekSave *reshZek, u32 *dest) {
    sys_memcpy(&reshZek->unk0, dest, sizeof(reshZek->unk0));
}

PartyPkm *func_0200afa8(ReshZekSave *reshZek) {
    return &reshZek->pkm;
}

void func_0200afac(ReshZekSave *reshZek, const PartyPkm *pkm) {
    copyPartyPkm(pkm, &reshZek->pkm);
}
