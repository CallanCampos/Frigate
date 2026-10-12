#include "types.h"
#include "save/box.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/save_control.h"
#include "text/system/btl_pokeparam_box_main_save_init.h"

// The battle box: the box of six Pokémon kept for link and Wi-Fi battles, with its name and the flags that lock
// it. The ROM has no string for the file, so the name is a guess (swan's getBattleBox). Block 0x31.

#define SAVE_BLOCK_BATTLE_BOX 0x31

static void copyPkmIntoBattleBox(BattleBoxSave *battleBox, u32 box, u32 slot, const BoxPkm *pkm);

u32 BattleBoxSave_GetSize(void) {
    return sizeof(BattleBoxSave);
}

void BattleBoxSave_Init(BattleBoxSave *battleBox) {
    u32 i;
    MsgData *msgData;

    sys_memset(battleBox, 0, sizeof(BattleBoxSave));
    for (i = 0; i < BATTLE_BOX_SLOTS; i++) {
        PML_PkmInit(&battleBox->pkm[0][i]);
    }
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_POKEPARAM_BOX_MAIN_SAVE_INIT, HEAPID_USER);
    if (msgData != NULL) {
        GFL_MsgDataLoadRawStr(msgData, BtlPokeparamBoxMainSaveInit_Text_Box1, battleBox->name, BATTLE_BOX_NAME_LENGTH);
        GFL_MsgDataFree(msgData);
    }
}

BattleBoxSave *getBattleBox(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_BATTLE_BOX);
}

static void copyPkmIntoBattleBox(BattleBoxSave *battleBox, u32 box, u32 slot, const BoxPkm *pkm) {
    battleBox->pkm[box][slot] = *pkm;
}

PokeParty *convertBoxedPokeSetToParty(BattleBoxSave *battleBox, HeapID heapId) {
    u32 i;
    PokeParty *party = PokeParty_Create(heapId);

    PokeParty_InitCore(party, BATTLE_BOX_SLOTS);
    for (i = 0; i < BATTLE_BOX_SLOTS; i++) {
        BoxPkm *boxPkm = getBoxSlotAddress(battleBox, 0, i);
        PartyPkm *pkm;

        if (!PML_PkmGetParam(boxPkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            break;
        }
        pkm = boxPkmRegenToPartyPkm(boxPkm, HEAPID_TAIL(heapId));
        PokeParty_AddPkm(party, pkm);
        GFL_HeapFree(pkm);
    }
    return party;
}

void copySelectedPkmToBattleBlk(BattleBoxSave *battleBox, PokeParty *party) {
    u32 i;

    for (i = 0; i < BATTLE_BOX_SLOTS; i++) {
        PML_PkmInit(&battleBox->pkm[0][i]);
    }
    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        copyPkmIntoBattleBox(battleBox, 0, i, func_0201d624(PokeParty_GetPkm(party, i)));
    }
}

BOOL BattleBoxSave_HasPkm(BattleBoxSave *battleBox) {
    BOOL result = FALSE;

    if (PML_PkmGetParam(getBoxSlotAddress(battleBox, 0, 0), PKM_PARAM_SPECIES_VALID, NULL)) {
        result = TRUE;
    }
    return result;
}

void BattleBoxSave_SetFlags(BattleBoxSave *battleBox, u32 mask) {
    battleBox->flags |= (u16)mask;
}

void BattleBoxSave_ClearFlags(BattleBoxSave *battleBox, u32 mask) {
    battleBox->flags &= (u16)~mask;
}

BoxPkm *getBoxSlotAddress(BattleBoxSave *battleBox, u32 box, u32 slot) {
    return &battleBox->pkm[box][slot];
}

BOOL BattleBoxSave_CheckFlags(BattleBoxSave *battleBox, u16 flags) {
    if (battleBox->flags & flags) {
        return TRUE;
    }
    return FALSE;
}
