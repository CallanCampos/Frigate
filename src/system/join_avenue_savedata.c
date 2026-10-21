#include "types.h"
#include "gfl/std.h"
#include "save/join_avenue.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include <stddef.h>

// The Join Avenue's save block, and adding a visitor, a person or an entry to it: merged into the one who is already
// there, or else added to the people met or the entries. The ROM has no string for the file, so the name is
// descriptive. Block 0x43.

#define SAVE_BLOCK_JOIN_AVENUE 0x43

// What JoinAvenue_MergeExisting and JoinAvenue_MergeOrAdd return
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

static u32 JoinAvenue_MergeExisting(JoinAvenueSave *joinAvenue, GameData *gameData, void *source, u32 kind);
static u32 JoinAvenue_MergeOrAdd(JoinAvenueSave *joinAvenue, GameData *gameData, void *source, u32 kind);

static const JoinAvenueSourceFuncs sSourceFuncs[] = {
    {
        (BOOL (*)(void *))JoinAvenueVisitor_IsValid,
        (u32 (*)(JoinAvenuePersonList *, const void *))JoinAvenuePersonList_MergeVisitor,
        (u32 (*)(JoinAvenueEntryList *, const void *))JoinAvenueEntryList_HasVisitor,
        (u32 (*)(JoinAvenuePersonList *, const void *))JoinAvenuePersonList_AddVisitor,
        NULL,
    },
    {
        (BOOL (*)(void *))JoinAvenuePerson_IsValid,
        (u32 (*)(JoinAvenuePersonList *, const void *))JoinAvenuePersonList_MergePerson,
        (u32 (*)(JoinAvenueEntryList *, const void *))JoinAvenueEntryList_HasPerson,
        (u32 (*)(JoinAvenuePersonList *, const void *))JoinAvenuePersonList_AddPerson,
        NULL,
    },
    {
        (BOOL (*)(void *))JoinAvenueEntry_IsValid,
        (u32 (*)(JoinAvenuePersonList *, const void *))JoinAvenuePersonList_MergeEntry,
        (u32 (*)(JoinAvenueEntryList *, const void *))JoinAvenueEntryList_MergeEntry,
        NULL,
        (u32 (*)(JoinAvenueEntryList *, void *))JoinAvenueEntryList_Add,
    },
};

// The step of JoinAvenue_AddStepwise
static u32 sAddStep;

u32 JoinAvenueSave_GetSize(void) {
    return sizeof(JoinAvenueSave);
}

void JoinAvenueSave_Init(JoinAvenueSave *joinAvenue) {
    MI_CpuFill8(joinAvenue, 0, offsetof(JoinAvenueSave, unk13E0));
    JoinAvenuePersonList_Init(JoinAvenue_GetPersonList(joinAvenue), JOIN_AVE_PERSON_COUNT);
    JoinAvenueEntryList_Init(JoinAvenue_GetEntryList(joinAvenue), JOIN_AVE_ENTRY_COUNT);
    JoinAvenueOccupants_Init(getAddressOfBeginningOfOccupants(joinAvenue));
    JoinAvenueInfo_Init(JoinAvenue_GetInfo(joinAvenue));
}

JoinAvenueSave *SaveControl_GetJoinAvenue(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_JOIN_AVENUE);
}

JoinAvenuePersonList *JoinAvenue_GetPersonList(JoinAvenueSave *joinAvenue) {
    return (JoinAvenuePersonList *)joinAvenue->personList;
}

JoinAvenueEntryList *JoinAvenue_GetEntryList(JoinAvenueSave *joinAvenue) {
    return (JoinAvenueEntryList *)joinAvenue->entryList;
}

JoinAvenueOccupants *getAddressOfBeginningOfOccupants(JoinAvenueSave *joinAvenue) {
    return &joinAvenue->occupants;
}

JoinAvenueInfo *JoinAvenue_GetInfo(JoinAvenueSave *joinAvenue) {
    return &joinAvenue->info;
}

u32 JoinAvenue_Add(JoinAvenueSave *joinAvenue, GameData *gameData, void *entry, u32 a3) {
    u32 result = JoinAvenue_MergeExisting(joinAvenue, gameData, entry, a3);

    if (result == JOIN_AVE_ADD_NOT_FOUND) {
        result = JoinAvenue_MergeOrAdd(joinAvenue, gameData, entry, a3);
    }
    return result;
}

void JoinAvenue_ResetAddStep(JoinAvenueSave *joinAvenue) {
    sAddStep = 0;
}

// The same as JoinAvenue_Add over two calls, FALSE until the result is out
BOOL JoinAvenue_AddStepwise(JoinAvenueSave *joinAvenue, GameData *gameData, void *work, u32 a3, u32 *out) {
    u32 result;

    switch (sAddStep) {
    case 0:
        result = JoinAvenue_MergeExisting(joinAvenue, gameData, work, a3);
        if (result == JOIN_AVE_ADD_NOT_FOUND) {
            sAddStep++;
            break;
        }
        *out = result;
        return TRUE;
    case 1:
        *out = JoinAvenue_MergeOrAdd(joinAvenue, gameData, work, a3);
        return TRUE;
    }
    return FALSE;
}

// Merges the source into the person met or the entry who matches
static u32 JoinAvenue_MergeExisting(JoinAvenueSave *joinAvenue, GameData *gameData, void *source, u32 kind) {
    JoinAvenuePersonList *personList = JoinAvenue_GetPersonList(joinAvenue);
    JoinAvenuePersonList *extraList = *GameData_GetJoinAvenuePersonListPtr(gameData);
    JoinAvenueEntryList *entryList = JoinAvenue_GetEntryList(joinAvenue);
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
static u32 JoinAvenue_MergeOrAdd(JoinAvenueSave *joinAvenue, GameData *gameData, void *source, u32 kind) {
    JoinAvenuePersonList *personList = JoinAvenue_GetPersonList(joinAvenue);
    JoinAvenuePersonList *extraList = *GameData_GetJoinAvenuePersonListPtr(gameData);
    JoinAvenueEntryList *entryList = JoinAvenue_GetEntryList(joinAvenue);
    JoinAvenueOccupants *occupants = getAddressOfBeginningOfOccupants(joinAvenue);
    JoinAvenueInfo *info = JoinAvenue_GetInfo(joinAvenue);
    u32 (*add)(JoinAvenuePersonList *list, const void *source);
    u32 result;

    result = JoinAvenueOccupants_Merge(occupants, source, kind);
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
