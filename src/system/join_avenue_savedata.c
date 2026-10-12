#include <stddef.h>
#include "types.h"
#include "save/join_avenue.h"
#include "gfl/std.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The Join Avenue's save block, and adding a visitor, a person or an entry to it: merged into the one who is already
// there, or else added to the people met or the entries. The ROM has no string for the file, so the name is
// descriptive. Block 0x43.

#define SAVE_BLOCK_JOIN_AVENUE 0x43

// What func_020100e0 and func_02010194 return
enum {
    JOIN_AVE_ADD_FAILED = 0,
    JOIN_AVE_ADD_MERGED = 1,
    JOIN_AVE_ADD_ADDED = 2,
    // Nobody matches, so the source is to be added
    JOIN_AVE_ADD_NOT_FOUND = 3,
};

// The functions of a kind of source: whether it is valid, merging it into the person or the entry who matches, and
// adding it to the people or to the entries
typedef struct {
    BOOL (*isValid)(void *source);
    u32 (*mergePerson)(JoinAvenuePersonList *list, const void *source);
    u32 (*mergeEntry)(JoinAvenueEntryList *list, const void *source);
    u32 (*addPerson)(JoinAvenuePersonList *list, const void *source);
    u32 (*addEntry)(JoinAvenueEntryList *list, void *source);
} JoinAvenueSourceFuncs;

static u32 func_020100e0(JoinAvenueSave *joinAvenue, GameData *gameData, void *source, u32 kind);
static u32 func_02010194(JoinAvenueSave *joinAvenue, GameData *gameData, void *source, u32 kind);

static const JoinAvenueSourceFuncs sSourceFuncs[] = {
    {
        (BOOL (*)(void *))func_020379f8,
        (u32 (*)(JoinAvenuePersonList *, const void *))func_0203806c,
        (u32 (*)(JoinAvenueEntryList *, const void *))func_02037f10,
        (u32 (*)(JoinAvenuePersonList *, const void *))func_020380d8,
        NULL,
    },
    {
        (BOOL (*)(void *))func_020370a4,
        (u32 (*)(JoinAvenuePersonList *, const void *))func_02038090,
        (u32 (*)(JoinAvenueEntryList *, const void *))func_02037f24,
        (u32 (*)(JoinAvenuePersonList *, const void *))func_0203815c,
        NULL,
    },
    {
        (BOOL (*)(void *))func_02037a98,
        (u32 (*)(JoinAvenuePersonList *, const void *))func_020380b4,
        (u32 (*)(JoinAvenueEntryList *, const void *))func_02037f38,
        NULL,
        (u32 (*)(JoinAvenueEntryList *, void *))func_02037e7c,
    },
};

// The step of func_020100a4
static u32 sAddStep;

u32 func_0200fffc(void) {
    return sizeof(JoinAvenueSave);
}

void func_02010004(JoinAvenueSave *joinAvenue) {
    sys_memset(joinAvenue, 0, offsetof(JoinAvenueSave, unk13E0));
    func_02037ffc(JoinAvenue_GetPersonList(joinAvenue), JOIN_AVE_PERSON_COUNT);
    func_02037e4c(func_02010054(joinAvenue), JOIN_AVE_ENTRY_COUNT);
    func_0203880c(getAddressOfBeginningOfOccupants(joinAvenue));
    func_02038e54(JoinAvenue_GetInfo(joinAvenue));
}

JoinAvenueSave *SaveControl_GetJoinAvenue(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_JOIN_AVENUE);
}

JoinAvenuePersonList *JoinAvenue_GetPersonList(JoinAvenueSave *joinAvenue) {
    return (JoinAvenuePersonList *)joinAvenue->personList;
}

JoinAvenueEntryList *func_02010054(JoinAvenueSave *joinAvenue) {
    return (JoinAvenueEntryList *)joinAvenue->entryList;
}

JoinAvenueOccupants *getAddressOfBeginningOfOccupants(JoinAvenueSave *joinAvenue) {
    return &joinAvenue->occupants;
}

JoinAvenueInfo *JoinAvenue_GetInfo(JoinAvenueSave *joinAvenue) {
    return &joinAvenue->info;
}

u32 func_02010078(JoinAvenueSave *joinAvenue, GameData *gameData, void *entry, u32 a3) {
    u32 result = func_020100e0(joinAvenue, gameData, entry, a3);

    if (result == JOIN_AVE_ADD_NOT_FOUND) {
        result = func_02010194(joinAvenue, gameData, entry, a3);
    }
    return result;
}

void func_02010098(JoinAvenueSave *joinAvenue) {
    sAddStep = 0;
}

// The same as func_02010078 over two calls, FALSE until the result is out
BOOL func_020100a4(JoinAvenueSave *joinAvenue, GameData *gameData, void *work, u32 a3, u32 *out) {
    u32 result;

    switch (sAddStep) {
    case 0:
        result = func_020100e0(joinAvenue, gameData, work, a3);
        if (result == JOIN_AVE_ADD_NOT_FOUND) {
            sAddStep++;
            break;
        }
        *out = result;
        return TRUE;
    case 1:
        *out = func_02010194(joinAvenue, gameData, work, a3);
        return TRUE;
    }
    return FALSE;
}

// Merges the source into the person met or the entry who matches
static u32 func_020100e0(JoinAvenueSave *joinAvenue, GameData *gameData, void *source, u32 kind) {
    JoinAvenuePersonList *personList = JoinAvenue_GetPersonList(joinAvenue);
    JoinAvenuePersonList *extraList = *GameData_GetJoinAvenuePersonListPtr(gameData);
    JoinAvenueEntryList *entryList = func_02010054(joinAvenue);
    JoinAvenueOccupants *occupants = getAddressOfBeginningOfOccupants(joinAvenue);
    JoinAvenueInfo *info = JoinAvenue_GetInfo(joinAvenue);
    u32 result;

    if (!sSourceFuncs[kind].isValid(source)) {
        return JOIN_AVE_ADD_FAILED;
    }
    result = sSourceFuncs[kind].mergePerson(personList, source);
    if (result == 1) {
        return JOIN_AVE_ADD_MERGED;
    }
    if (result == 2) {
        return JOIN_AVE_ADD_FAILED;
    }
    if (extraList != NULL) {
        result = sSourceFuncs[kind].mergePerson(extraList, source);
        if (result == 1) {
            return JOIN_AVE_ADD_MERGED;
        }
        if (result == 2) {
            return JOIN_AVE_ADD_FAILED;
        }
    }
    result = sSourceFuncs[kind].mergeEntry(entryList, source);
    if (result == 1) {
        return JOIN_AVE_ADD_MERGED;
    }
    if (result == 2) {
        return JOIN_AVE_ADD_FAILED;
    }
    return JOIN_AVE_ADD_NOT_FOUND;
}

// Merges the source into the occupant who matches, or else adds it to the people met or the entries
static u32 func_02010194(JoinAvenueSave *joinAvenue, GameData *gameData, void *source, u32 kind) {
    JoinAvenuePersonList *personList = JoinAvenue_GetPersonList(joinAvenue);
    JoinAvenuePersonList *extraList = *GameData_GetJoinAvenuePersonListPtr(gameData);
    JoinAvenueEntryList *entryList = func_02010054(joinAvenue);
    JoinAvenueOccupants *occupants = getAddressOfBeginningOfOccupants(joinAvenue);
    JoinAvenueInfo *info = JoinAvenue_GetInfo(joinAvenue);
    u32 (*add)(JoinAvenuePersonList *list, const void *source);
    u32 result;

    result = func_020388e8(occupants, source, kind);
    if (result == 1) {
        return JOIN_AVE_ADD_MERGED;
    }
    if (result == 2) {
        return JOIN_AVE_ADD_FAILED;
    }
    add = sSourceFuncs[kind].addPerson;
    if (add != NULL) {
        result = add(personList, source);
        if (result == 0 && extraList != NULL) {
            result = add(extraList, source);
        }
        if (result == 1) {
            return JOIN_AVE_ADD_ADDED;
        }
    } else if (sSourceFuncs[kind].addEntry(entryList, source) == 1) {
        return JOIN_AVE_ADD_ADDED;
    }
    return JOIN_AVE_ADD_FAILED;
}
