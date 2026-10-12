#ifndef POKEBW2_SAVE_BOX_H
#define POKEBW2_SAVE_BOX_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "struct_decls.h"

// The PC boxes, save blocks 0 (the main block) and 1 to 24 (the boxes). The names of the limits are the ones the
// assertions of box_savedata.c keep
#define BOX_MAX_TRAY 24
#define BOX_MAX_POS 30
#define BOX_NAME_LENGTH 20
// The box that is open, where a function takes a box
#define BOXDAT_TRAYNUM_CURRENT 0xffffffff

typedef struct BoxMainSave BoxMainSave;

void BoxMainSave_InitBlock(BoxMainSave *main);
u32 BoxMainSave_GetBlockSize(void);
// Puts a Pokémon in the first free slot from the open box on, opening more boxes when all are full
BOOL BoxSaveAccessor_InsertPkm(BoxSaveAccessor *boxes, BoxPkm *pkm);
// Puts a Pokémon in the first free slot of a box
BOOL BoxSaveAccessor_InsertPkmCore(BoxSaveAccessor *boxes, u32 box, BoxPkm *pkm);
BOOL BoxSaveAccessor_SetPkm(BoxSaveAccessor *boxes, u32 box, u32 slot, BoxPkm *pkm);
void BoxSaveAccessor_SwapPkms(BoxSaveAccessor *boxes, u32 box1, u32 slot1, u32 box2, u32 slot2);
void BoxSaveAccessor_ClearPkm(BoxSaveAccessor *boxes, u32 box, u32 slot);
u32 BoxSaveAccessor_GetLastOpenedBox(BoxSaveAccessor *boxes);
// How many boxes are open: 8, 16 or 24
u8 BoxSaveAccessor_GetAvailableBoxCount(BoxSaveAccessor *boxes);
// Opens 8 more boxes, up to 24, and returns how many are open
u8 BoxSaveAccessor_UnlockMoreBoxes(BoxSaveAccessor *boxes);
// Finds the first free slot from *box and *slot on, opening more boxes when all are full
BOOL BoxSaveAccessor_GetNextFreeBoxSlot(BoxSaveAccessor *boxes, int *box, int *slot);
// How many Pokémon the boxes have room for
u32 BoxSaveAccessor_CountEmptySlots(BoxSaveAccessor *boxes);
u32 countEmptySlotsInBox(BoxSaveAccessor *boxes, u32 box);
void saveLastOpenedBoxIdx(BoxSaveAccessor *boxes, u32 box);
// A box's wallpaper
u32 getBoxNumFromIdx(BoxSaveAccessor *boxes, u32 box);
void BoxSaveAccessor_SetWallpaper(BoxSaveAccessor *boxes, u32 box, u32 wallpaper);
// Copies a box's name into a string, and back
void loadBoxNameToStrbuf(BoxSaveAccessor *boxes, u32 box, StrBuf *str);
void getBoxNameFromStrbuf(BoxSaveAccessor *boxes, u32 box, StrBuf *name);
u32 howManyPokesInGeneralAreInBox(BoxSaveAccessor *boxes, u32 box);
u32 howManyTotalPokesAreInBoxes(BoxSaveAccessor *boxes);
// Without the eggs
u32 howManyNormalPokesAreInAllBoxes(BoxSaveAccessor *boxes);
u32 BoxSaveAccessor_GetPkmParam(BoxSaveAccessor *boxes, u32 box, u32 slot, u32 param, void *buffer);
BoxPkm *BoxSaveAccessor_GetPkm(BoxSaveAccessor *boxes, u32 box, u32 slot);
// Allocates a copy of a boxed Pokémon, which BoxSaveAccessor_FreePkmCopy frees
BoxPkm *copyBoxedPkmToBuf(BoxSaveAccessor *boxes, u32 box, u32 slot, HeapID heapId);
void BoxSaveAccessor_FreePkmCopy(BoxPkm *pkm);
// Sets flags of the main block: the sets of extra wallpapers that are unlocked
void BoxSaveAccessor_SetFlags(BoxSaveAccessor *boxes, u32 flags);
// Whether a set of extra wallpapers (1 or 2) is unlocked
BOOL BoxSaveAccessor_CheckFlags(BoxSaveAccessor *boxes, u32 set);
// The size of a box
u32 getSizeofPokeBox(void);
BoxSaveAccessor *BoxSaveAccessor_Create(HeapID heapId, SaveControl *save);
void BoxSaveAccessor_Free(BoxSaveAccessor *boxes);
// A box's block: its size, its init and its pointer
void initFullBox(void *tray);
u32 getSizeofBox(void);
void *BoxSaveAccessor_GetBox(BoxSaveAccessor *boxes, u32 box);

// The battle box, save block 0x31: one box of six Pokémon, its name and its flags
#define BATTLE_BOX_COUNT 1
#define BATTLE_BOX_SLOTS 6
#define BATTLE_BOX_NAME_LENGTH 20

struct BattleBoxSave {
    BoxPkm pkm[BATTLE_BOX_COUNT][BATTLE_BOX_SLOTS];
    u16 name[BATTLE_BOX_NAME_LENGTH];
    u16 flags;
    u8 padding35A[2];
};

u32 BattleBoxSave_GetSize(void);
void BattleBoxSave_Init(BattleBoxSave *battleBox);
BattleBoxSave *getBattleBox(SaveControl *save);
// Allocates a party of the battle box's Pokémon
PokeParty *convertBoxedPokeSetToParty(BattleBoxSave *battleBox, HeapID heapId);
// Store the party in the Battle Box
void copySelectedPkmToBattleBlk(BattleBoxSave *battleBox, PokeParty *party);
// Whether the battle box has a Pokémon
BOOL BattleBoxSave_HasPkm(BattleBoxSave *battleBox);
// Sets the flags of the mask in the battle box, and clears them
void BattleBoxSave_SetFlags(BattleBoxSave *battleBox, u32 mask);
void BattleBoxSave_ClearFlags(BattleBoxSave *battleBox, u32 mask);
BoxPkm *getBoxSlotAddress(BattleBoxSave *battleBox, u32 box, u32 slot);
// Whether any of the flags is set
BOOL BattleBoxSave_CheckFlags(BattleBoxSave *battleBox, u16 flags);

// The day care, save block 0x32: its two Pokémon, whether an egg waits, and the seed the egg is made from
#define DAY_CARE_SLOTS 2

typedef struct {
    BOOL used;
    PartyPkm pkm;
    u32 steps;
} DayCareSlot;

struct DayCareSave {
    DayCareSlot slots[DAY_CARE_SLOTS];
    BOOL eggSpawned;
    u64 seed;
};

void DayCareSave_SetSpawnFlag(DayCareSave *dayCare);
// Draws a new seed and clears the egg
void DayCareSave_ResetSeed(DayCareSave *dayCare);
// A slot's Pokémon, or the first slot's when the slot is empty
PartyPkm *DayCareSave_GetPkm(DayCareSave *dayCare, int slot);
void DayCareSave_AddPkm(DayCareSave *dayCare, int slot, const PartyPkm *pkm);
void DayCareSave_DisablePkmStatus(DayCareSave *dayCare, int slot);
BOOL DayCareSave_CheckSpawnFlag(DayCareSave *dayCare);
BOOL DayCareSave_GetPkmStatus(DayCareSave *dayCare, u32 slot);
u32 DayCareSave_GetPkmCount(DayCareSave *dayCare);
u32 DayCareSave_GetPkmStepCounter(DayCareSave *dayCare, u32 slot);
u64 DayCareSave_GetSeed(DayCareSave *dayCare);
// Swaps two slots
void DayCareSave_SwapSlots(DayCareSave *dayCare, int slot1, int slot2);
// Adds steps to the counters of the Pokémon in the day care
void DayCareSave_UpdateStepCount(DayCareSave *dayCare, u32 steps);
u32 getSizeofBreedingBlock(void);
void setEggArea(DayCareSave *dayCare);
DayCareSave *getDaycareBlockAddress(SaveControl *save);

#endif // POKEBW2_SAVE_BOX_H
