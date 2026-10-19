#include "types.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "save/key_data_savedata.h"
#include "save/key_info.h"
#include "save/save_control.h"

// The Key System's key information: the keys unlocked and enabled, and the difficulty, the city and the chamber that
// they set. Each is kept as a constant exclusive-ored with a seed. It lies in save block 0x45 and at the start of the
// key system's data that is sent over Unova Link. The ROM has no string for the file, so the name is descriptive.

// The constants of an unlocked key, an enabled key, a difficulty, a city and a chamber
static const u32 sUnlockKeys[KEY_INFO_KEY_COUNT] = { 0x35691, 0x18256, 0x59389, 0x48292, 0x9892 };
static const u32 keyArray[KEY_INFO_KEY_COUNT] = { 0x93389, 0x22843, 0x34771, 0xab031, 0xb3818 };
static const u32 UNOVA_LINK_DIFFICULTY_KEYS[KEY_INFO_DIFFICULTY_COUNT] = { 0x31239, 0x15657, 0x49589 };
static const u32 UNOVA_LINK_CITY_KEYS[KEY_INFO_CITY_COUNT] = { 0x34525, 0x11963 };
static const u32 sChamberKeys[KEY_INFO_CHAMBER_COUNT] = { 0x94525, 0x81963, 0x38569 };

void KeySystemData_Init(void *a0) {
    MI_CpuFill8(a0, 0, sizeof(KeySystemData));
    KeyInfoSave_Init(KeySystemData_GetKeyInfo(a0));
    zero476Bytes(KeySystemData_GetMemoryLink(a0));
}

KeyInfoSave *KeySystemData_GetKeyInfo(void *a0) {
    return a0;
}

void *KeySystemData_GetMemoryLink(void *a0) {
    KeySystemData *data = a0;

    return &data->memoryLink;
}

void KeyInfoSave_Init(KeyInfoSave *keyInfo) {
    MI_CpuFill8(keyInfo, 0, sizeof(KeyInfoSave));
    keyInfo->seed = GFL_RandomLC(0xffffffff);
}

void KeyInfoSave_Copy(KeyInfoSave *dest, const KeyInfoSave *src) {
    *dest = *src;
}

KeyInfoSave *getKeyInfoSaveBlk(SaveControl *save) {
    return KeyDataSave_GetKeyInfo(getKeyDataBlkAddress(save));
}

void KeyInfo_UnlockKey(KeyInfoSave *keyInfo, u32 key) {
    u32 value = sUnlockKeys[key];

    keyInfo->unlocked[key] = value ^ keyInfo->seed;
}

BOOL KeyInfo_IsKeyUnlocked(KeyInfoSave *keyInfo, u32 key) {
    u32 value = keyInfo->unlocked[key];

    if ((value ^ keyInfo->seed) == sUnlockKeys[key]) {
        return TRUE;
    }
    return FALSE;
}

void KeyInfo_EnableKey(KeyInfoSave *keyInfo, u32 key) {
    u32 value = keyArray[key];

    keyInfo->enabled[key] = value ^ keyInfo->seed;
}

BOOL keyEnabler(KeyInfoSave *keyInfo, u32 key) {
    u32 value = keyInfo->enabled[key];

    if ((value ^ keyInfo->seed) == keyArray[key]) {
        return TRUE;
    }
    return FALSE;
}

void SetGameDifficulty(KeyInfoSave *keyInfo, u32 difficulty) {
    u32 value = UNOVA_LINK_DIFFICULTY_KEYS[difficulty];

    keyInfo->difficulty = value ^ keyInfo->seed;
}

u32 GetGameDifficulty(KeyInfoSave *keyInfo) {
    int i;

    for (i = 0; i < KEY_INFO_DIFFICULTY_COUNT; i++) {
        if ((keyInfo->difficulty ^ keyInfo->seed) == UNOVA_LINK_DIFFICULTY_KEYS[i]) {
            return i;
        }
    }
    return GAME_DIFFICULTY_NORMAL;
}

void KeyInfo_SetCityKey(KeyInfoSave *keyInfo, u32 city) {
    u32 value = UNOVA_LINK_CITY_KEYS[city];

    keyInfo->city = value ^ keyInfo->seed;
}

u32 KeyInfo_GetCityKey(KeyInfoSave *keyInfo) {
    int i;

    for (i = 0; i < KEY_INFO_CITY_COUNT; i++) {
        if ((keyInfo->city ^ keyInfo->seed) == UNOVA_LINK_CITY_KEYS[i]) {
            return i;
        }
    }
    return 0;
}

void KeyInfo_SetChamberKey(KeyInfoSave *keyInfo, u32 chamber) {
    u32 value = sChamberKeys[chamber];

    keyInfo->chamber = value ^ keyInfo->seed;
}

u32 KeyInfo_GetChamberKey(KeyInfoSave *keyInfo) {
    int i;

    for (i = 0; i < KEY_INFO_CHAMBER_COUNT; i++) {
        if ((keyInfo->chamber ^ keyInfo->seed) == sChamberKeys[i]) {
            return i;
        }
    }
    return 0;
}
