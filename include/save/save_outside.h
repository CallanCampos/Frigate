#ifndef POKEBW2_SAVE_SAVE_OUTSIDE_H
#define POKEBW2_SAVE_SAVE_OUTSIDE_H

#include "types.h"
#include "gfl/heap.h"
#include "save/mystery_gift.h"
#include "struct_decls.h"

// save_outside.c (overlay 331): data kept on the card outside the game's save, which survives deleting it. The
// mystery gift cards, in two copies, and the key system's data. The game reads it when it creates the save, and
// a new game takes the gifts and keys from it

// The mystery gifts kept outside the save, laid out like the start of MysteryGiftSaveData and encrypted the same
// way
typedef struct {
    u8 receivedFlags[MG_ID_MAX / 8];
    MysteryGift cards[MG_CARD_MAX_OUTSIDE];
    u16 checksum;
    u16 key;
} SaveOutsideGifts;

// Loads both blocks, and returns the work that keeps them. Copies that can't be read leave empty data
SaveOutside *SaveOutside_Load(HeapID heapId);
void SaveOutside_Free(SaveOutside *work);
// Starts writing the work's blocks back, and steps it until it returns TRUE
void SaveOutside_StartSave(SaveOutside *work);
BOOL SaveOutside_Save(SaveOutside *work);
// Whether the gifts were loaded, and whether both of their copies were broken
BOOL SaveOutside_IsGiftsLoaded(SaveOutside *work);
BOOL SaveOutside_IsGiftsBroken(SaveOutside *work);
SaveOutsideGifts *SaveOutside_GetGifts(SaveOutside *work);
// Copies the gifts into the save's mystery gift block
void SaveOutside_CopyGiftsToSave(SaveOutside *work, SaveControl *save);
// Erase the whole of the data on the card, and only the keys'
void SaveOutside_Erase(HeapID heapId);
void SaveOutside_EraseKeys(HeapID heapId);
// Whether the keys were loaded, and whether their block was broken
BOOL SaveOutside_IsKeysLoaded(SaveOutside *work);
BOOL SaveOutside_IsKeysBroken(SaveOutside *work);
// The key system's data: the key information, then the Memory Link's
KeySystemData *SaveOutside_GetKeyData(SaveOutside *work);
// Copies the key information into the save
void SaveOutside_CopyKeysToSave(SaveOutside *work, SaveControl *save);

#endif // POKEBW2_SAVE_SAVE_OUTSIDE_H
