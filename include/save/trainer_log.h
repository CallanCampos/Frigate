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
    // What the fortune of func_0200b11c is matched against, 0 to 100
    u32 value : 7;
    // How many times the trainer was met, up to TRAINER_LOG_COUNT_MAX
    u32 count : 10;
    // Whether the name takes no article in the scripts' text
    u32 unk10_17 : 1;
    u32 unk10_18 : 4;
    u32 unk10_22 : 4;
    u32 unk10_26 : 5;
    u32 used : 1;
    u32 id;
    u16 unk18;
    u8 unk1A;
    u8 unk1B_0 : 6;
    u8 unk1B_6 : 1;
    // Set when the trainer is met, cleared each day
    u8 today : 1;
}; // size: 0x1c

struct TrainerLogSave {
    TrainerLogEntry entries[TRAINER_LOG_MAX];
    // The slot the last trainer went into, TRAINER_LOG_NONE at first
    u32 lastSlot : 8;
    u32 unk348_8 : 24;
}; // size: 0x34c

TrainerLogSave *func_0200afbc(SaveControl *save);
// How many entries are in use, counted up to the first free one
u32 func_0200afc8(TrainerLogSave *log);
// An entry, whose name comes first
const u16 *func_0200afe4(TrainerLogSave *log, u32 index);
u8 func_0200afec(TrainerLogSave *log, u32 index);
u16 func_0200b000(TrainerLogSave *log, u32 index);
BOOL func_0200b014(TrainerLogSave *log, u32 index);
void func_0200b024(const TrainerLogSave *log, u32 index, u16 *unk18, u8 *unk1A, u8 *unk1B_0, u8 *unk1B_6);
u8 func_0200b04c(TrainerLogSave *log);
// The entry whose value is nearest the fortune of the birthday and date, at random between two as near
u32 func_0200b05c(TrainerLogSave *log, u8 month, u8 day, RTCDate *date);
// Whether the trainer with the ID was met today, and whether they are in the log
BOOL func_0200b0c0(TrainerLogSave *log, u32 id);
BOOL func_0200b0f0(TrainerLogSave *log, u32 id);
// A fortune from 0 to 100, a cosine of how far the date is from the birthday within a month
u8 func_0200b11c(u8 month, u8 day, const RTCDate *date);
// Logs a trainer met: in the slot their value earns, counting the meetings of one already logged. Returns the slot
u8 func_0200b184(TrainerLogSave *log, const u16 *name, u8 value, u8 unk10_17, u8 unk10_18, u8 unk10_22, u32 id,
                 u16 unk18, u8 unk1A, u8 unk1B_0, u8 unk1B_6);
// A day has passed
void func_0200b220(TrainerLogSave *log);
// The size of the block and its init
u32 func_0200b240(void);
void func_0200b248(TrainerLogSave *log);
// Puts the entry in the first slot whose value is not above its own, moving the rest down. Returns the slot
u8 func_0200b26c(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry);
BOOL func_0200b2fc(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry);
void func_0200b330(TrainerLogEntry *entry, const u16 *name, u8 value, u32 count, u8 unk10_17, u8 unk10_18,
                   u8 unk10_22, u32 id, u16 unk18, u8 unk1A, u8 unk1B_0, u8 unk1B_6);
TrainerLogEntry *func_0200b3ec(TrainerLogSave *log, s32 count, u32 id);
// Counts one more meeting for every entry with the entry's ID
void func_0200b41c(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry);

#endif // POKEBW2_SAVE_TRAINER_LOG_H
