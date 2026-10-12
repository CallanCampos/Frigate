#ifndef POKEBW2_FIELD_FMISSION_LOG_ACCESSOR_FV_H
#define POKEBW2_FIELD_FMISSION_LOG_ACCESSOR_FV_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/fest_mission_beacon.h"

// How the Funfest mission's view reads the festival's lists, overlay 85, named after the ROM's
// "fmission_log_accessor_fv.c": the queue of new log entries, the ring of entries the view shows, the Pass Powers
// that were given, and the fields of a log entry. swan has no names for this overlay; the names are ours

// The entries the view's log holds, and the entry its list starts at, counted back from the newest
u8 FMissionLogFV_GetCount(FestMissionLog *log);
u8 FMissionLogFV_GetTop(FestMissionLog *log);
void FMissionLogFV_SetTop(FestMissionLog *log, u8 top);
FestMissionLogEntry *FMissionLogFV_AllocEntry(HeapID heapId);
// Takes the oldest entry out of the queue of new ones; FALSE when it is empty
BOOL FMissionLogFV_PopEntry(FestMissionLog *log, FestMissionLogEntry *dest);
// The index of the entry `offset` entries older than the newest of the view's log, which is a ring
int FMissionLogFV_GetIndex(FestMissionLog *log, int offset);
// Adds an entry to the view's log as its newest, and returns its offset, 0
int FMissionLogFV_PushEntry(FestMissionLog *log, FestMissionLogEntry *entry);
// Copies the entry `offset` entries older than the newest; FALSE when there is none
BOOL FMissionLogFV_GetEntry(FestMissionLog *log, FestMissionLogEntry *dest, int offset);
// The next Pass Power to offer the player, the player's own first, or PASS_POWER_NONE. `name` gets who gave it and
// *received whether a beacon did
u16 FMissionLogFV_GetPassPower(FestMissionPassPowers *passPowers, StrBuf *name, int *received);
// Drops the Pass Power that FMissionLogFV_GetPassPower gave
void FMissionLogFV_RemovePassPower(FestMissionPassPowers *passPowers, BOOL received);

void FMissionLogEntryFV_GetText(FestMissionLogEntry *entry, StrBuf *dest);
// The player's look, from 8 on for a girl
u8 FMissionLogEntryFV_GetLook(FestMissionLogEntry *entry);
u8 FMissionLogEntryFV_GetGender(FestMissionLogEntry *entry);
BOOL FMissionLogEntryFV_IsOwn(FestMissionLogEntry *entry);
u8 FMissionLogEntryFV_GetKind(FestMissionLogEntry *entry);
u16 FMissionLogEntryFV_GetValue(FestMissionLogEntry *entry);
u16 FMissionLogEntryFV_GetScore(FestMissionLogEntry *entry);
// Sets the words of the entry's message: what it is about, by its kind
void FMissionLogEntryFV_SetWords(FestMissionLogEntry *entry, WordSet *wordSet, HeapID heapId);

#endif // POKEBW2_FIELD_FMISSION_LOG_ACCESSOR_FV_H
