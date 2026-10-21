#ifndef POKEBW2_SAVE_TRAINER_LOG_H
#define POKEBW2_SAVE_TRAINER_LOG_H

#include "types.h"
#include "nitro/rtc.h"
#include "struct_decls.h"

// Save block 0x2c: a log of thirty trainers, each with a name, an ID, a value the scripts match against a fortune
// drawn from the player's birthday and the date, how many times they were met, and whether they were met today.
// The ROM has no string for the file or name for the block, so the names here are guesses

#define TRAINER_LOG_MAX 30
#define TRAINER_LOG_COUNT_MAX 999
#define TRAINER_LOG_NONE 0x7f
#define TRAINER_LOG_NO_SLOT 0xff

struct TrainerLogEntry {
    u16 name[8];
    // What the fortune of TrainerLog_GetBirthdayValue is matched against, 0 to 100
    u32 value : 7;
    // How many times the trainer was met, up to TRAINER_LOG_COUNT_MAX
    u32 count : 10;
    // Whether the trainer is male, and the trainer's birthday
    u32 isMale : 1;
    u32 birthMonth : 4;
    u32 birthDay : 4;
    u32 unk10_26 : 5;
    u32 used : 1;
    u32 id;
    // The trainer's Pokémon
    u16 species;
    u8 form;
    u8 sex : 6;
    u8 isEgg : 1;
    // Set when the trainer is met, cleared each day
    u8 today : 1;
}; // size: 0x1c

struct TrainerLogSave {
    TrainerLogEntry entries[TRAINER_LOG_MAX];
    // The slot the last trainer went into, TRAINER_LOG_NONE at first
    u32 lastSlot : 8;
    u32 unk348_8 : 24;
}; // size: 0x34c

TrainerLogSave *TrainerLogSave_Get(SaveControl *save);
// How many entries are in use, counted up to the first free one
u32 TrainerLogSave_CountEntries(TrainerLogSave *log);
// An entry, whose name comes first
const u16 *TrainerLogSave_GetEntryName(TrainerLogSave *log, u32 index);
u8 TrainerLogSave_GetEntryValue(TrainerLogSave *log, u32 index);
u16 TrainerLogSave_GetEntryCount(TrainerLogSave *log, u32 index);
BOOL TrainerLogSave_IsEntryMale(TrainerLogSave *log, u32 index);
void TrainerLogSave_GetEntryPokemon(const TrainerLogSave *log, u32 index, u16 *species, u8 *form, u8 *sex, u8 *isEgg);
u8 TrainerLogSave_GetLastSlot(TrainerLogSave *log);
// The entry whose value is nearest the fortune of the birthday and date, at random between two as near
u32 TrainerLogSave_FindFortuneEntry(TrainerLogSave *log, u8 month, u8 day, RTCDate *date);
// Whether the trainer with the ID was met today, and whether they are in the log
BOOL TrainerLogSave_WasMetToday(TrainerLogSave *log, u32 id);
BOOL TrainerLogSave_HasId(TrainerLogSave *log, u32 id);
// A fortune from 0 to 100, a cosine of how far the date is from the birthday within a month
u8 TrainerLog_GetBirthdayValue(u8 month, u8 day, const RTCDate *date);
// Logs a trainer met: in the slot their value earns, counting the meetings of one already logged. Returns the slot
u8 TrainerLogSave_AddTrainer(TrainerLogSave *log, const u16 *name, u8 value, u8 isMale, u8 birthMonth, u8 birthDay,
                             u32 id, u16 species, u8 form, u8 sex, u8 isEgg);
// A day has passed
void TrainerLogSave_ClearToday(TrainerLogSave *log);
// The size of the block and its init
u32 TrainerLogSave_GetSize(void);
void TrainerLogSave_Init(TrainerLogSave *log);
// Puts the entry in the first slot whose value is not above its own, moving the rest down. Returns the slot
u8 TrainerLogSave_InsertEntry(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry);
BOOL TrainerLogSave_ContainsEntry(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry);
void TrainerLogEntry_Init(TrainerLogEntry *entry, const u16 *name, u8 value, u32 count, u8 isMale, u8 birthMonth,
                          u8 birthDay, u32 id, u16 species, u8 form, u8 sex, u8 isEgg);
TrainerLogEntry *TrainerLogSave_FindEntry(TrainerLogSave *log, s32 count, u32 id);
// Counts one more meeting for every entry with the entry's ID
void TrainerLogSave_IncrementMetCount(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry);

#endif // POKEBW2_SAVE_TRAINER_LOG_H
