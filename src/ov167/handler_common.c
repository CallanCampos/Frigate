#include "types.h"
#include "battle/btl_event.h"
#include "battle/btl_handler.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_server_flow.h"
#include "battle/btl_tables.h"
#include "battle/handler_common.h"
#include "constants/items.h"
#include "constants/species.h"
#include "pml/waza.h"

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

// Whether a mon is among the event's targets
BOOL func_ov167_021cde38(u32 monId) {
    u32 count = BattleEventVar_GetValue(5);
    u32 i;

    for (i = 0; i < count; i++) {
        if (monId == BattleEventVar_GetValue(6 + i)) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL IsMonLastInTurnOrder(BtlServerFlow *flow, u8 monId) {
    if (func_ov167_021abbec(flow, monId)) {
        return TRUE;
    }
    return FALSE;
}

// Whether a species holds the item that its form depends on
BOOL GiratinaArceusGenesectItemCheck(u16 species, u16 item) {
    switch (species) {
    case SPECIES_GIRATINA:
        if (item == ITEM_GRISEOUS_ORB) {
            return TRUE;
        }
        break;
    case SPECIES_ARCEUS:
        if (IsArceusPlate(item)) {
            return TRUE;
        }
        break;
    case SPECIES_GENESECT:
        if (IsGenesectDrive(item)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Whether a mon's item can't be taken away from it
// Whether a mon's item can't be taken away from it
BOOL func_ov167_021cdedc(BtlServerFlow *flow, u8 monId) {
    BattleMon *mon = GetBattleMon(flow, monId);
    u16 species = GetBattleMonSpecies(mon);
    u16 item = GetBattleMonHeldItem(mon);

    if (GiratinaArceusGenesectItemCheck(species, item)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_021cdf08(BtlServerFlow *flow, u8 monId) {
    if (func_ov167_021abca8(flow) == 0) {
        u32 client = func_ov167_0219c648(monId);

        // The original tests the client against 1 twice (`beq` to the next instruction, then `bne`), as two
        // constants of the same value would
        if (client == 1 || client == 1) {
            return TRUE;
        }
    }
    return FALSE;
}

// Whether two mons' items can't be swapped
// Whether two mons' items can't be swapped
BOOL func_ov167_021cdf28(BtlServerFlow *flow, u8 monId, u8 otherId) {
    BattleMon *mon;
    BattleMon *other;
    u16 species;
    u16 item;

    if (func_ov167_021cdf08(flow, monId)) {
        return TRUE;
    }
    if (func_ov167_021cdedc(flow, otherId)) {
        return TRUE;
    }
    mon = GetBattleMon(flow, monId);
    other = GetBattleMon(flow, otherId);
    species = GetBattleMonSpecies(mon);
    item = GetBattleMonHeldItem(other);
    if (GiratinaArceusGenesectItemCheck(species, item)) {
        return TRUE;
    }
    return FALSE;
}

// Magic Coat and Magic Bounce: bounce a status move that can be reflected back at its user
void CommonMagicCoatCheckMoveEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attacker = BattleEventVar_GetValue(2);

    if (!IsAllyMonID(attacker, monId)) {
        u16 move = BattleEventVar_GetValue(0x12);

        if (PML_MoveGetQuality(move) == 0xb && getMoveFlag(move, 4) && BattleEventVar_RewriteValue(0x22, 0x19)) {
            func_ov167_021abf74(flow, monId, attacker);
        }
    }
}

void CommonMagicCoatWait(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (monId == BattleEventVar_GetValue(4) && BattleEventVar_GetValue(0x4e) == 0 &&
        !IsSemiInvulnMove(GetBattleMon(flow, monId)) && getMoveFlag(BattleEventVar_GetValue(0x12), 4) &&
        BattleEventVar_RewriteValue(0x40, 1)) {
        u8 attacker = BattleEventVar_GetValue(3);

        func_ov167_021abf74(flow, monId, attacker);
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

// The message that a mon is protected from a move
void func_ov167_021ce044(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (monId == BattleEventVar_GetValue(2) && !IsSemiInvulnMove(GetBattleMon(flow, monId))) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x2fc);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventVar_GetValue(0x12));
        BattleHandler_PopWork(flow, param);
    }
}

void MultiplyBasePower(u32 factor) {
    factor *= BattleEventVar_GetValue(0x30);
    BattleEventVar_RewriteValue(0x30, factor);
}

void CommonRunCalcSkip(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (IsAllyMonID(BattleEventVar_GetValue(2), monId)) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

BOOL CommonCheckRunMessage(BattleEventItem *item, BtlServerFlow *flow, u8 monId) {
    if (IsAllyMonID(BattleEventVar_GetValue(2), monId) && BattleEventVar_RewriteValue(0x51, 1)) {
        return TRUE;
    }
    return FALSE;
}
