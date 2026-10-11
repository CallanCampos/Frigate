#ifndef POKEBW2_FIELD_FLD_TRADE_H
#define POKEBW2_FIELD_FLD_TRADE_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"

// Function and type names from swan; member layout reconstructed from the game code.
// An in-game trade's offer, a file of archive 163 (data/trades/, packed by tools/data/trade_data.py)
struct FieldTradeOfferData {
    // The offer's own number in the archive
    u32 index;
    u32 species;
    u32 form;
    u32 level;
    // 0xff for a random one
    u32 ivs[6];
    // The ability's slot, 2 for the hidden ability
    u32 abilitySlot;
    // 0xff for a random one
    u32 nature;
    // The sex PML_GenPID gives the Pokémon, 0xff for a random one
    u32 sex;
    u32 trainerId;
    u32 contest[5];
    u32 heldItem;
    u32 trainerGender;
    u32 unk54;
    u32 region;
    u32 wantedSpecies;
    u32 wantedSex;
    // The lines of the trade names' message file with the Pokémon's nickname and its trainer's name
    u32 nicknameMessageId;
    u32 nameMessageId;
};

struct FieldTradeInput {
    HeapID heapId;
    u16 padding;
    u32 offerIndex;
    FieldTradeOfferData *offerData;
    void *tradeData;
    PlayerInfo *trainer;
};

extern const char data_ov033_0217c624[];

FieldTradeInput *FieldTradeInput_Create(u32 heapId, u32 offerIndex);
void FieldTradeInput_Free(FieldTradeInput *input);
u32 FieldTradeInput_GetSpecies(FieldTradeInput *input);
u32 FieldTradeInput_GetWantedSpecies(FieldTradeInput *input);
u32 FieldTradeInput_GetWantedSex(FieldTradeInput *input);
StrBuf *FieldTradeInput_LoadName(u32 heapId, u32 messageId);

#endif // POKEBW2_FIELD_FLD_TRADE_H
