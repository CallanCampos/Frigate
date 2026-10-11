#ifndef POKEBW2_SAVE_BOX_H
#define POKEBW2_SAVE_BOX_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"

// The size of a box, and a box
u32 getSizeofPokeBox(void);
u32 getSizeofBox(void);
void *BoxSaveAccessor_GetBox(BoxSaveAccessor *boxes, u32 box);
u32 howManyNormalPokesAreInAllBoxes(BoxSaveAccessor *boxes);
u32 howManyTotalPokesAreInBoxes(BoxSaveAccessor *boxes);
BOOL BoxSaveAccessor_InsertPkm(BoxSaveAccessor *boxes, BoxPkm *pkm);
BOOL BoxSaveAccessor_InsertPkmCore(BoxSaveAccessor *boxes, u32 box, BoxPkm *pkm);
// How many Pokémon the boxes have room for
u32 func_02007a38(BoxSaveAccessor *boxes);
void BoxSaveAccessor_ClearPkm(BoxSaveAccessor *boxes, u32 box, u32 slot);
BOOL BoxSaveAccessor_GetNextFreeBoxSlot(BoxSaveAccessor *boxes, int *box, int *slot);
u32 BoxSaveAccessor_GetPkmParam(BoxSaveAccessor *boxes, u32 box, u32 slot, u32 param, void *buffer);
BoxPkm *BoxSaveAccessor_GetPkm(BoxSaveAccessor *boxes, u32 box, u32 slot);
u32 BoxSaveAccessor_GetLastOpenedBox(BoxSaveAccessor *boxes);
// How many boxes are open: 8, 16 or 24
u8 BoxSaveAccessor_GetAvailableBoxCount(BoxSaveAccessor *boxes);
// Opens 8 more boxes, up to 24, and returns how many are open
u8 BoxSaveAccessor_UnlockMoreBoxes(BoxSaveAccessor *boxes);
void saveLastOpenedBoxIdx(BoxSaveAccessor *boxes, u32 box);
// A box's wallpaper
u32 getBoxNumFromIdx(BoxSaveAccessor *boxes, u32 box);
void func_02007b00(BoxSaveAccessor *boxes, u32 box, u32 wallpaper);
// Copies a box's name into a string, and back
void loadBoxNameToStrbuf(BoxSaveAccessor *boxes, u32 box, StrBuf *str);
void getBoxNameFromStrbuf(BoxSaveAccessor *boxes, u32 box, StrBuf *name);
u32 howManyPokesInGeneralAreInBox(BoxSaveAccessor *boxes, u32 box);
u32 countEmptySlotsInBox(BoxSaveAccessor *boxes, u32 box);
BOOL BoxSaveAccessor_SetPkm(BoxSaveAccessor *boxes, u32 box, u32 slot, BoxPkm *pkm);
void BoxSaveAccessor_SwapPkms(BoxSaveAccessor *boxes, u32 box1, u32 slot1, u32 box2, u32 slot2);
// Allocates a copy of a boxed Pokémon, which func_02007d84 frees
BoxPkm *copyBoxedPkmToBuf(BoxSaveAccessor *boxes, u32 box, u32 slot, HeapID heapId);
void func_02007d84(BoxPkm *pkm);
// Whether a set of extra wallpapers (1 or 2) is unlocked
BOOL func_02007da4(BoxSaveAccessor *boxes, u32 set);

// The battle box, save block 0x31
BattleBoxSave *getBattleBox(SaveControl *save);
// Clears the flags of the mask in the battle box
void func_0200c370(BattleBoxSave *battleBox, u32 mask);
// Allocates a party of the battle box's Pokémon
PokeParty *convertBoxedPokeSetToParty(BattleBoxSave *battleBox, HeapID heapId);
// Store the party in the Battle Box
void copySelectedPkmToBattleBlk(BattleBoxSave *battleBox, PokeParty *party);
// Whether any of the flags is set
BOOL func_0200c394(BattleBoxSave *battleBox, u16 flags);
BoxPkm *getBoxSlotAddress(BattleBoxSave *battleBox, u32 a1, u32 slot);
BOOL func_0200c340(BattleBoxSave *battleBox);

// The day care, save block 0x32
DayCareSave *getDaycareBlockAddress(SaveControl *save);
u32 DayCareSave_GetPkmStatus(DayCareSave *dayCare, u32 slot);
PartyPkm *DayCareSave_GetPkm(DayCareSave *dayCare, u32 slot);

// Sets flags of the box save
void func_02007d8c(BoxSaveAccessor *accessor, u32 flags);

#endif // POKEBW2_SAVE_BOX_H
