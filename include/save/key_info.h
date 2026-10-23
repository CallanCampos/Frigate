#ifndef POKEBW2_SAVE_KEY_INFO_H
#define POKEBW2_SAVE_KEY_INFO_H

#include "types.h"
#include "save/memory_link_savedata.h"
#include "struct_decls.h"

// The Key System's key information (key_info.c, a descriptive name). Every value is kept as a constant of a table
// exclusive-ored with the seed, so that a plain number in the save means nothing.

#define KEY_INFO_KEY_COUNT 5
#define KEY_INFO_CITY_COUNT 2
#define KEY_INFO_DIFFICULTY_COUNT 3
#define KEY_INFO_CHAMBER_COUNT 3

struct KeyInfoSave {
    // The keys that are unlocked, and those whose effect is enabled
    u32 unlocked[KEY_INFO_KEY_COUNT];
    u32 enabled[KEY_INFO_KEY_COUNT];
    u32 difficulty;
    u32 city;
    u32 chamber;
    u32 seed;
}; // size: 0x38

// The key system's data that ov331 reads: the key information and the Memory Link's data
struct KeySystemData {
    KeyInfoSave keyInfo;
    MemoryLinkSave memoryLink;
}; // size: 0x214

// Clears the key information and draws its seed
void KeyInfoSave_Init(KeyInfoSave *keyInfo);
// The key information in the save, which is in block 0x45
KeyInfoSave *getKeyInfoSaveBlk(SaveControl *save);
// Whether a key is unlocked
BOOL KeyInfo_IsKeyUnlocked(KeyInfoSave *keyInfo, u32 key);

// Unlocks a key, and enables what it unlocks
void KeyInfo_UnlockKey(KeyInfoSave *keyInfo, u32 key);
void KeyInfo_EnableKey(KeyInfoSave *keyInfo, u32 key);
// Whether what a key unlocks is enabled
BOOL keyEnabler(KeyInfoSave *keyInfo, u32 key);
void SetGameDifficulty(KeyInfoSave *keyInfo, u32 difficulty);
// Sets the city, and the chamber the mystery door leads to
void KeyInfo_SetCityKey(KeyInfoSave *keyInfo, u32 city);
void KeyInfo_SetChamberKey(KeyInfoSave *keyInfo, u32 chamber);
// Returns 1 if the key that switches the city is set
u32 KeyInfo_GetCityKey(KeyInfoSave *keyInfo);
// The difficulty that the keys set, GAME_DIFFICULTY_*
u32 GetGameDifficulty(KeyInfoSave *keyInfo);
// 1 or 2 if a key that opens a chamber is set
u32 KeyInfo_GetChamberKey(KeyInfoSave *keyInfo);
// Returns its argument, as the key information in data that ov331 reads
KeyInfoSave *KeySystemData_GetKeyInfo(void *a0);
// The Memory Link's data, at 0x38 in the same data
void *KeySystemData_GetMemoryLink(KeySystemData *a0);
// Clears the key system's data, and the copy of the key information into the save
void KeySystemData_Init(void *a0);
void KeyInfoSave_Copy(KeyInfoSave *dest, const KeyInfoSave *src);

#define GAME_DIFFICULTY_EASY 0
#define GAME_DIFFICULTY_NORMAL 1
#define GAME_DIFFICULTY_CHALLENGE 2

#endif // POKEBW2_SAVE_KEY_INFO_H
