#include "types.h"
#include "field/fmission_log_accessor_fv.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/high_link.h"
#include "system/fest_mission_beacon.h"
#include "system/wordset.h"

// How the Funfest mission's view reads the festival's lists, named after the ROM's "fmission_log_accessor_fv.c". The
// names are ours

// What FestMission_GetLogCategory says a log entry's value is
enum {
    LOG_VALUE_NONE,
    LOG_VALUE_ITEM,
    LOG_VALUE_ZONE,
    LOG_VALUE_MOVE,
    LOG_VALUE_TEXT,
    LOG_VALUE_PASS_POWER,
    LOG_VALUE_SPECIES,
};

u8 FMissionLogFV_GetCount(FestMissionLog *log) {
    return log->count;
}

u8 FMissionLogFV_GetTop(FestMissionLog *log) {
    return log->unk4EE;
}

void FMissionLogFV_SetTop(FestMissionLog *log, u8 top) {
    log->unk4EE = top;
}

FestMissionLogEntry *FMissionLogFV_AllocEntry(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(FestMissionLogEntry), TRUE, "fmission_log_accessor_fv.c", 91);
}

BOOL FMissionLogFV_PopEntry(FestMissionLog *log, FestMissionLogEntry *dest) {
    if (log->entries[0].kind == 0) {
        return FALSE;
    }
    *dest = log->entries[0];
    MI_CpuCopy8(&log->entries[1], &log->entries[0], sizeof(FestMissionLogEntry) * (FEST_MISSION_LOG_MAX - 1));
    FestMissionLogEntry_Clear(&log->entries[FEST_MISSION_LOG_MAX - 1]);
    log->count--;
    return TRUE;
}

int FMissionLogFV_GetIndex(FestMissionLog *log, int offset) {
    int index = log->unk4EC;

    if (offset < 0) {
        index = index + offset + FEST_MISSION_LOG_MAX;
    } else {
        index = index + offset;
    }
    return index % FEST_MISSION_LOG_MAX;
}

int FMissionLogFV_PushEntry(FestMissionLog *log, FestMissionLogEntry *entry) {
    int index = FMissionLogFV_GetIndex(log, -1);

    log->entries[index] = *entry;
    log->unk4EC = index;
    if (log->count < FEST_MISSION_LOG_MAX) {
        log->count++;
    }
    return 0;
}

BOOL FMissionLogFV_GetEntry(FestMissionLog *log, FestMissionLogEntry *dest, int offset) {
    FestMissionLogEntry *entry = &log->entries[FMissionLogFV_GetIndex(log, offset)];

    if (entry->kind == LOG_KIND_NONE) {
        return FALSE;
    }
    *dest = *entry;
    return TRUE;
}

u16 FMissionLogFV_GetPassPower(FestMissionPassPowers *passPowers, StrBuf *name, int *received) {
    if (passPowers->own.passPower != PASS_POWER_NONE) {
        GFL_StrBufLoadFixedString(name, passPowers->own.name, NELEMS(passPowers->own.name));
        *received = passPowers->own.received;
        return passPowers->own.passPower;
    }
    if (passPowers->count == 0) {
        return PASS_POWER_NONE;
    }
    GFL_StrBufLoadFixedString(name, passPowers->others[0].name, NELEMS(passPowers->others[0].name));
    *received = passPowers->others[0].received;
    return passPowers->others[0].passPower;
}

void FMissionLogFV_RemovePassPower(FestMissionPassPowers *passPowers, BOOL received) {
    if (received) {
        if (passPowers->count != 0) {
            MI_CpuCopy8(&passPowers->others[1], &passPowers->others[0], sizeof(FestMissionPassPower));
            FestMissionPassPower_Clear(&passPowers->others[1]);
            passPowers->count--;
        }
    } else {
        FestMissionPassPower_Clear(&passPowers->own);
    }
}

void FMissionLogEntryFV_GetText(FestMissionLogEntry *entry, StrBuf *dest) {
    GFL_StrBufLoadFixedString(dest, entry->text, NELEMS(entry->text));
}

u8 FMissionLogEntryFV_GetLook(FestMissionLogEntry *entry) {
    if (entry->gender == 0) {
        return entry->look;
    }
    return entry->look + 8;
}

u8 FMissionLogEntryFV_GetGender(FestMissionLogEntry *entry) {
    return entry->gender;
}

BOOL FMissionLogEntryFV_IsOwn(FestMissionLogEntry *entry) {
    return entry->own;
}

u8 FMissionLogEntryFV_GetKind(FestMissionLogEntry *entry) {
    return entry->kind;
}

u16 FMissionLogEntryFV_GetValue(FestMissionLogEntry *entry) {
    return entry->value;
}

u16 FMissionLogEntryFV_GetScore(FestMissionLogEntry *entry) {
    return entry->score;
}

void FMissionLogEntryFV_SetWords(FestMissionLogEntry *entry, WordSet *wordSet, HeapID heapId) {
    StrBuf *name;

    GFL_WordSetClearAll(wordSet);
    name = GFL_StrBufCreate(11, HEAPID_TAIL(heapId));
    if (name == NULL) {
        return;
    }
    switch (FestMission_GetLogCategory(entry->kind)) {
    case LOG_VALUE_ITEM:
        loadItemTextNameToStrbuf(wordSet, 0, entry->value);
        break;
    case LOG_VALUE_ZONE:
        loadLocationNameToStrbuf(wordSet, 0, ZoneData_GetPlaceNameID(entry->value));
        break;
    case LOG_VALUE_MOVE:
        loadMoveNameToStrbuf(wordSet, 0, entry->value);
        break;
    case LOG_VALUE_TEXT:
        FMissionLogEntryFV_GetText(entry, name);
        GFL_WordSetSetStrbuf(wordSet, 0, name, entry->gender, 1, 2);
        break;
    case LOG_VALUE_PASS_POWER:
        FestMissionLogEntry_GetName(entry, name);
        GFL_WordSetSetStrbuf(wordSet, 0, name, entry->gender, 1, 2);
        loadPassPowerToStrbuf(wordSet, 1, entry->value);
        break;
    case LOG_VALUE_SPECIES:
        WordSet_LoadSpeciesName(wordSet, 0, entry->value);
        break;
    }
    GFL_StrBufFree(name);
}
