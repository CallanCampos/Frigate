#include "types.h"
#include "gfl/random.h"
#include "gfl/rtc_cache.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "save/save_control.h"
#include "save/trainer_log.h"
#include "system/str_tool.h"

// The log of thirty trainers met, in the order of the value the scripts match against a fortune drawn from the
// player's birthday and the date. The ROM has no string for the file or name for the block, so the name is a
// guess. Block 0x2c.

#define SAVE_BLOCK_TRAINER_LOG 0x2c

TrainerLogSave *TrainerLogSave_Get(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_LOG);
}

u32 TrainerLogSave_CountEntries(TrainerLogSave *log) {
    s32 i;

    for (i = 0; i < TRAINER_LOG_MAX; i++) {
        if (!log->entries[i].used) {
            break;
        }
    }
    return i;
}

const u16 *TrainerLogSave_GetEntryName(TrainerLogSave *log, u32 index) {
    return log->entries[index].name;
}

u8 TrainerLogSave_GetEntryValue(TrainerLogSave *log, u32 index) {
    return log->entries[index].value;
}

u16 TrainerLogSave_GetEntryCount(TrainerLogSave *log, u32 index) {
    return log->entries[index].count;
}

BOOL func_0200b014(TrainerLogSave *log, u32 index) {
    return log->entries[index].isMale;
}

void func_0200b024(const TrainerLogSave *log, u32 index, u16 *species, u8 *form, u8 *sex, u8 *isEgg) {
    const TrainerLogEntry *entry = &log->entries[index];

    *species = entry->species;
    *form = entry->form;
    *sex = entry->sex;
    *isEgg = entry->isEgg;
}

u8 TrainerLogSave_GetLastSlot(TrainerLogSave *log) {
    return log->lastSlot;
}

u32 func_0200b05c(TrainerLogSave *log, u8 month, u8 day, RTCDate *date) {
    u32 count = TrainerLogSave_CountEntries(log);
    u32 fortune = TrainerLog_GetBirthdayValue(month, day, date);
    u32 result = 0;
    u32 best = -1;
    u32 i;

    for (i = 0; i < count; i++) {
        s32 diff = TrainerLogSave_GetEntryValue(log, i) - fortune;
        if (diff < 0) {
            diff = -diff;
        }
        if (best == diff) {
            u32 pair[2];
            pair[0] = result;
            pair[1] = i;
            result = pair[GFL_RandomLC(2)];
        } else if (best > diff) {
            result = i;
            best = diff;
        }
    }
    return result;
}

BOOL TrainerLogSave_WasMetToday(TrainerLogSave *log, u32 id) {
    s32 i;

    for (i = 0; i < TRAINER_LOG_MAX; i++) {
        if (id == log->entries[i].id && log->entries[i].used == 1) {
            return log->entries[i].today;
        }
    }
    return FALSE;
}

BOOL TrainerLogSave_HasId(TrainerLogSave *log, u32 id) {
    s32 i;

    for (i = 0; i < TRAINER_LOG_MAX; i++) {
        if (id == log->entries[i].id && log->entries[i].used == 1) {
            return TRUE;
        }
    }
    return FALSE;
}

u8 TrainerLog_GetBirthdayValue(u8 month, u8 day, const RTCDate *date) {
    RTCDate birthday = *date;
    u32 today;
    u32 birthdayDay;

    today = RTC_GetDayOfYear(&birthday);
    birthday.month = month;
    birthday.day = day;
    birthdayDay = RTC_GetDayOfYear(&birthday);
    if (today >= birthdayDay) {
        birthdayDay += 365;
    }
    return (FX_SinIdx((birthdayDay - today) % 30 * 0xffff / 30) + FX32_ONE) * 50 >> FX32_SHIFT;
}

u8 func_0200b184(TrainerLogSave *log, const u16 *name, u8 value, u8 isMale, u8 birthMonth, u8 birthDay, u32 id,
                 u16 species, u8 form, u8 sex, u8 isEgg) {
    u16 count = 0;
    TrainerLogEntry entry;
    TrainerLogEntry *logged = TrainerLogSave_FindEntry(log, TRAINER_LOG_MAX, id);

    if (logged != NULL) {
        count = logged->count;
    }
    func_0200b330(&entry, name, value, count, isMale, birthMonth, birthDay, id, species, form, sex, isEgg);
    log->lastSlot = TrainerLogSave_InsertEntry(log, TRAINER_LOG_MAX, &entry);
    if (TrainerLogSave_ContainsEntry(log, TRAINER_LOG_MAX, &entry)) {
        TrainerLogSave_IncrementMetCount(log, TRAINER_LOG_MAX, &entry);
    }
    return log->lastSlot;
}

void TrainerLogSave_ClearToday(TrainerLogSave *log) {
    s32 i;

    for (i = 0; i < TRAINER_LOG_MAX; i++) {
        log->entries[i].today = 0;
    }
}

u32 TrainerLogSave_GetSize(void) {
    return sizeof(TrainerLogSave);
}

void TrainerLogSave_Init(TrainerLogSave *log) {
    MI_CpuFill8(log, 0, sizeof(TrainerLogSave));
    log->lastSlot = TRAINER_LOG_NONE;
}

u8 TrainerLogSave_InsertEntry(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry) {
    s32 slot = -1;
    s32 i;

    for (i = 0; i < count; i++) {
        if (!log->entries[i].used || log->entries[i].value <= entry->value) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        return TRAINER_LOG_NO_SLOT;
    }
    for (i = count - 1; i > slot; i--) {
        if (log->entries[i].count != 0 || log->entries[i - 1].count != 0) {
            MI_CpuCopy8(&log->entries[i - 1], &log->entries[i], sizeof(TrainerLogEntry));
        }
    }
    MI_CpuCopy8(entry, &log->entries[slot], sizeof(TrainerLogEntry));
    return slot;
}

BOOL TrainerLogSave_ContainsEntry(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry) {
    s32 i;

    for (i = 0; i < count; i++) {
        if (entry->id == log->entries[i].id && log->entries[i].used == 1) {
            return TRUE;
        }
    }
    return FALSE;
}

void func_0200b330(TrainerLogEntry *entry, const u16 *name, u8 value, u32 count, u8 isMale, u8 birthMonth, u8 birthDay,
                   u32 id, u16 species, u8 form, u8 sex, u8 isEgg) {
    MI_CpuFill8(entry, 0, sizeof(TrainerLogEntry));
    wcharsncpy(name, entry->name, 8);
    entry->value = value;
    entry->id = id;
    entry->isMale = isMale;
    entry->birthMonth = birthMonth;
    entry->birthDay = birthDay;
    entry->count = count;
    entry->species = species;
    entry->form = form;
    entry->sex = sex;
    entry->isEgg = isEgg & 1;
    entry->used = 1;
    entry->today = 1;
}

TrainerLogEntry *TrainerLogSave_FindEntry(TrainerLogSave *log, s32 count, u32 id) {
    s32 i;

    for (i = 0; i < count; i++) {
        if (id == log->entries[i].id && log->entries[i].used == 1) {
            return &log->entries[i];
        }
    }
    return NULL;
}

void TrainerLogSave_IncrementMetCount(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry) {
    s32 i;

    for (i = 0; i < count; i++) {
        TrainerLogEntry *logged = &log->entries[i];
        if (entry->id == logged->id) {
            u32 met;
            logged->count = logged->count + 1;
            logged->today = 1;
            met = logged->count;
            if (met > TRAINER_LOG_COUNT_MAX) {
                met = TRAINER_LOG_COUNT_MAX;
            }
            logged->count = met;
        }
    }
}
