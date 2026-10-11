#ifndef POKEBW2_SAVE_HIGH_LINK_H
#define POKEBW2_SAVE_HIGH_LINK_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The pass power in each of the three slots, or HIGH_LINK_POWER_NONE
#define HIGH_LINK_POWER_NONE 0x30
u32 func_0200c678(HighLinkSave *save, int index);
// Puts a pass power in one of the three slots
void func_0200c668(HighLinkSave *save, u32 passPower, int index);
u32 func_0200c6a0(HighLinkSave *save, u32 id);
// Copies two bytes of the save into dest
void func_0200c6d8(HighLinkSave *save, u8 *dest, u32 a2);
BOOL PassPower_IsIDValid(u32 passPower);
// Starts a pass power, bought with Pass Orbs when paid is set
void PassPower_Activate(u32 passPower, void *data, u32 paid);
// A pass power's effect and its price in Pass Orbs, from the table of PassPowerData_Create
u8 PassPowerData_GetEffect(void *data, u32 passPower);
u16 PassPowerData_GetPrice(void *data, u32 passPower);
// The pass powers that act at once: they restore the party's HP or PP
void PassPower_ApplyHPRestore(PokeParty *party);
void PassPower_ApplyPPRestore(PokeParty *party);
u32 PassPower_GetUsedIDByEffect(int effect);
BOOL PassPower_IsBW1Compatible(u32 passPower);
u32 PassPower_GetRemainingSeconds(int effect);
u32 PassPower_ApplyPrizeMoney(u32 money);
u32 PassPower_ApplyBargain(u32 price);
BOOL PassPower_IsBW1Compatible(u32 passPower);
u32 PassPower_ApplyEXP(u32 exp);
u32 PassPower_ApplyCapture(u32 rate);
// The pass powers on wild encounters: the encounter rate, the rolls for a shiny and the row of the slots' odds
u32 PassPower_ApplyEncounter(u32 rate);
u32 PassPower_ApplyLuckyShiny(u32 rolls);
u32 PassPower_ApplyLuckyEncProb(u32 row);
// The steps between phenomena, and the chance of one, in tenths of a percent
u16 PassPower_ApplyExploring(u16 steps);
u32 PassPower_ApplyExploringChance(u32 chance);
// A pass power of the table that PassPowerData_Create reads: the two levels it is unlocked by, White then Black,
// its name in message file 263 and its description in message file 264
typedef struct {
    u8 level1;
    u8 level2;
    // A number that the description shows
    u16 unk02;
    u16 name;
    u16 info;
    u8 unk08[4];
} PassPowerData;

// What func_02017208 returns: the Entralink's two levels, White then Black
typedef struct {
    u8 unk00[0xc];
    u16 level1;
    u16 level2;
} PassPowerLevel;

// The pass powers' table, read from its archive
void *PassPowerData_Create(HeapID heapId);
void PassPowerData_Free(void *data);
// Two levels of what func_02017208 returns, which the pass powers need
u16 func_0200c5dc(void *levels);
u16 func_0200c5e0(void *levels);
// The bit of a special pass power in the two bytes of func_0200c6d8, or 0xff
u32 PassPower_GetSPowerBaseID(int index);
// Overlay 12's pass_power_check.c: whether a pass power is unlocked (0), nearly (1) or not (2), and how many are
u32 CheckPassPowerUnlocked(void *data, int index, void *levels, u8 *flags);
u32 GetUnlockedPassPowerCount(void *data, void *levels, u8 *flags);

#endif // POKEBW2_SAVE_HIGH_LINK_H
