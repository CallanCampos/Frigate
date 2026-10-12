#include "types.h"
#include "save/key_info.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "save/key_data_savedata.h"
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

void func_02010448(void *a0) {
    sys_memset(a0, 0, sizeof(KeySystemData));
    func_02010474(func_0201046c(a0));
    zero476Bytes(func_02010470(a0));
}

KeyInfoSave *func_0201046c(void *a0) {
    return a0;
}

void *func_02010470(void *a0) {
    KeySystemData *data = a0;

    return &data->memoryLink;
}

void func_02010474(KeyInfoSave *keyInfo) {
    sys_memset(keyInfo, 0, sizeof(KeyInfoSave));
    keyInfo->seed = GFL_RandomLC(0xffffffff);
}

void func_02010490(KeyInfoSave *dest, const KeyInfoSave *src) {
    *dest = *src;
}

KeyInfoSave *getKeyInfoSaveBlk(SaveControl *save) {
    return func_020103f0(getKeyDataBlkAddress(save));
}

void func_020104b0(KeyInfoSave *keyInfo, u32 key) {
    u32 value = sUnlockKeys[key];

    keyInfo->unlocked[key] = value ^ keyInfo->seed;
}

BOOL func_020104c4(KeyInfoSave *keyInfo, u32 key) {
    u32 value = keyInfo->unlocked[key];

    if ((value ^ keyInfo->seed) == sUnlockKeys[key]) {
        return TRUE;
    }
    return FALSE;
}

void func_020104e0(KeyInfoSave *keyInfo, u32 key) {
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

void func_02010550(KeyInfoSave *keyInfo, u32 city) {
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

void func_0201058c(KeyInfoSave *keyInfo, u32 chamber) {
    u32 value = sChamberKeys[chamber];

    keyInfo->chamber = value ^ keyInfo->seed;
}

u32 func_020105a0(KeyInfoSave *keyInfo) {
    int i;

    for (i = 0; i < KEY_INFO_CHAMBER_COUNT; i++) {
        if ((keyInfo->chamber ^ keyInfo->seed) == sChamberKeys[i]) {
            return i;
        }
    }
    return 0;
}
