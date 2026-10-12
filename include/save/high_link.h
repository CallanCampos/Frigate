#ifndef POKEBW2_SAVE_HIGH_LINK_H
#define POKEBW2_SAVE_HIGH_LINK_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Block 0x35, the Entralink's save (high_link_savedata.c, a guessed name; the block's name is swan's)

#define HIGH_LINK_POWER_MAX 3
#define HIGH_LINK_POWER_NONE 0x30

// What func_02017208 returns: the Entralink's two levels, White then Black
typedef struct {
    u8 unk00[0xc];
    u16 level1;
    u16 level2;
} PassPowerLevel;

struct HighLinkSave {
    PassPowerLevel levels;
    u8 unk10[0x190];
    // The pass power in each of the three slots, or HIGH_LINK_POWER_NONE
    u8 powers[HIGH_LINK_POWER_MAX];
    u8 padding1A3;
    // A bit for each special pass power received, numbered by PassPower_GetSPowerBaseID
    u8 specialFlags[2];
    u8 unk1A6[6];
    // The play time counted by HighLinkSave_CountPlaySeconds
    u64 seconds;
};

// The block's size and its initial state; swan's names for them are misnomers
u32 getSizeofProbabilityTable(void);
void probability(HighLinkSave *save);
void setupProbabilityTable(PassPowerLevel *levels);
// Add to the White and the Black level, up to 999, and return how much was added
int HighLinkSave_AddWhiteLevel(PassPowerLevel *levels, int add);
int HighLinkSave_AddBlackLevel(PassPowerLevel *levels, int add);
// Copies the levels into the save, and out of it
void func_0200c638(SaveControl *save, const PassPowerLevel *levels);
void func_0200c650(SaveControl *save, PassPowerLevel *levels);
// Puts a pass power in one of the three slots
void HighLinkSave_SetPassPower(HighLinkSave *save, u32 passPower, int index);
// The pass power in a slot, or HIGH_LINK_POWER_NONE; a slot with an invalid one is cleared
u32 HighLinkSave_GetPassPower(HighLinkSave *save, int index);
// Sets the flag of a special pass power
void HighLinkSave_SetSpecialPowerFlag(HighLinkSave *save, u32 id);
// Copies the two bytes of the special pass powers' flags into dest
void func_0200c6d8(HighLinkSave *save, u8 *dest, u32 a2);
// Counts play time in seconds: mode 0 starts the count, 1 adds the time since the start to the save's total and
// 2 returns the total as it would be. Returns the total
u64 HighLinkSave_CountPlaySeconds(HighLinkSave *save, PlayTime *playTime, u32 mode);
BOOL PassPower_IsIDInvalid(u32 passPower);
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
