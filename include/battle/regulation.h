#ifndef POKEBW2_BATTLE_REGULATION_H
#define POKEBW2_BATTLE_REGULATION_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"

// The parameters of a regulation, for Regulation_GetParam, Regulation_SetParam and Regulation_CheckParamBit
enum {
    // The message of the regulation's name in the regulations' text bank
    REGULATION_PARAM_NAME,
    REGULATION_PARAM_1,
    REGULATION_PARAM_2,
    REGULATION_PARAM_3,
    REGULATION_PARAM_4,
    REGULATION_PARAM_5,
    REGULATION_PARAM_6,
    REGULATION_PARAM_7,
    REGULATION_PARAM_8,
    // Bit arrays, which only Regulation_CheckParamBit reads
    REGULATION_PARAM_BITS_A,
    REGULATION_PARAM_BITS_5C,
    REGULATION_PARAM_11,
    REGULATION_PARAM_12,
    REGULATION_PARAM_13,
    // A bit array as well
    REGULATION_PARAM_BITS_AC,
    REGULATION_PARAM_15,
    REGULATION_PARAM_16,
    REGULATION_PARAM_17,
    REGULATION_PARAM_18,
    REGULATION_PARAM_SHOW_PARTNERS,
    REGULATION_PARAM_TIME_LIMIT,
    REGULATION_PARAM_BATTLE_TYPE,
    REGULATION_PARAM_22
};

struct Regulation {
    u8 name;
    u8 unk1;
    u8 unk2;
    u8 unk3;
    u8 unk4;
    u8 unk5;
    u16 unk6;
    u8 unk8;
    u8 unk9;
    u8 unkA[0x52];
    u8 unk5C[0x4c];
    u16 unkA8;
    u8 unkAA;
    u8 unkAB;
    u8 unkAC[0x7];
    u8 unkB3;
    u8 unkB4;
    u8 unkB5;
    u8 unkB6;
    u8 unkB7;
    // Whether the partners' teams are shown in the selection, and its time limit
    u8 showPartners;
    u8 timeLimit;
    u8 unkBA;
    u8 unkBB;
}; // size: 0xbc

// What func_0200b760 fills in from a regulation
typedef struct {
    u8 unk0[0x7];
    u8 unk7;
} RegulationUnkAC;

// The rules of a battle, such as the level cap
Regulation *Regulation_Create(HeapID heapId);
void Regulation_Copy(const Regulation *src, Regulation *dest);
// The regulation's name
StrBuf *makeStrbufLoadBattleTypeText(Regulation *regulation, HeapID heapId);
// A parameter of the regulation, which is 0 for the bit arrays
u32 Regulation_GetParam(Regulation *regulation, u32 param);
// Whether a bit of one of the regulation's bit arrays is set
BOOL Regulation_CheckParamBit(Regulation *regulation, u32 param, int index);
void func_0200b760(Regulation *regulation, RegulationUnkAC *out);
// Reads a regulation from arc 106
void func_0201f744(u32 fileId, Regulation *regulation);
// Allocates a regulation read from arc 106
Regulation *func_0201f734(u32 fileId, HeapID heapId);
// Checks the Pokémon picked from a party, by their slots from 1, against the regulation
// Checks a Pokémon against the regulation, returning 0 if it may join
u32 func_0201f14c(Regulation *regulation, void *a1, PartyPkm *pkm);
u32 func_0201f1e8(Regulation *regulation, PokeParty *party, const u8 *picked);
u32 func_0201f268(Regulation *regulation, PokeParty *party);
u32 func_0201f424(Regulation *regulation, PokeParty *party, u32 *a2);
// Picks Pokémon of a party that the regulation allows, by their slots from 1, and returns how many it picked
u32 func_0201f97c(Regulation *regulation, PokeParty *party, u8 *picked);
// Values out of the parameter's range are ignored
void Regulation_SetParam(Regulation *regulation, u32 param, int value);

#endif // POKEBW2_BATTLE_REGULATION_H
