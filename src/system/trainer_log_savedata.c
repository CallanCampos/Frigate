#include "types.h"
#include "save/trainer_log.h"
#include "gfl/random.h"
#include "gfl/rtc_cache.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "save/save_control.h"
#include "system/str_tool.h"

// The log of thirty trainers met, in the order of the value the scripts match against a fortune drawn from the
// player's birthday and the date. The ROM has no string for the file or name for the block, so the name is a
// guess. Block 0x2c.

#define SAVE_BLOCK_TRAINER_LOG 0x2c

TrainerLogSave *func_0200afbc(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_LOG);
}

u32 func_0200afc8(TrainerLogSave *log) {
    s32 i;

    for (i = 0; i < TRAINER_LOG_MAX; i++) {
        if (!log->entries[i].used) {
            break;
        }
    }
    return i;
}

const u16 *func_0200afe4(TrainerLogSave *log, u32 index) {
    return log->entries[index].name;
}

u8 func_0200afec(TrainerLogSave *log, u32 index) {
    return log->entries[index].value;
}

u16 func_0200b000(TrainerLogSave *log, u32 index) {
    return log->entries[index].count;
}

BOOL func_0200b014(TrainerLogSave *log, u32 index) {
    return log->entries[index].unk10_17;
}

void func_0200b024(const TrainerLogSave *log, u32 index, u16 *unk18, u8 *unk1A, u8 *unk1B_0, u8 *unk1B_6) {
    const TrainerLogEntry *entry = &log->entries[index];

    *unk18 = entry->unk18;
    *unk1A = entry->unk1A;
    *unk1B_0 = entry->unk1B_0;
    *unk1B_6 = entry->unk1B_6;
}

u8 func_0200b04c(TrainerLogSave *log) {
    return log->lastSlot;
}

u32 func_0200b05c(TrainerLogSave *log, u8 month, u8 day, RTCDate *date) {
    u32 count = func_0200afc8(log);
    u32 fortune = func_0200b11c(month, day, date);
    u32 result = 0;
    u32 best = -1;
    u32 i;

    for (i = 0; i < count; i++) {
        s32 diff = func_0200afec(log, i) - fortune;
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

BOOL func_0200b0c0(TrainerLogSave *log, u32 id) {
    s32 i;

    for (i = 0; i < TRAINER_LOG_MAX; i++) {
        if (id == log->entries[i].id && log->entries[i].used == 1) {
            return log->entries[i].today;
        }
    }
    return FALSE;
}

BOOL func_0200b0f0(TrainerLogSave *log, u32 id) {
    s32 i;

    for (i = 0; i < TRAINER_LOG_MAX; i++) {
        if (id == log->entries[i].id && log->entries[i].used == 1) {
            return TRUE;
        }
    }
    return FALSE;
}

u8 func_0200b11c(u8 month, u8 day, const RTCDate *date) {
    RTCDate birthday = *date;
    u32 today;
    u32 birthdayDay;

    today = func_02044298(&birthday);
    birthday.month = month;
    birthday.day = day;
    birthdayDay = func_02044298(&birthday);
    if (today >= birthdayDay) {
        birthdayDay += 365;
    }
    return (FX_SinIdx((birthdayDay - today) % 30 * 0xffff / 30) + FX32_ONE) * 50 >> FX32_SHIFT;
}

u8 func_0200b184(TrainerLogSave *log, const u16 *name, u8 value, u8 unk10_17, u8 unk10_18, u8 unk10_22, u32 id,
                 u16 unk18, u8 unk1A, u8 unk1B_0, u8 unk1B_6) {
    u16 count = 0;
    TrainerLogEntry entry;
    TrainerLogEntry *logged = func_0200b3ec(log, TRAINER_LOG_MAX, id);

    if (logged != NULL) {
        count = logged->count;
    }
    func_0200b330(&entry, name, value, count, unk10_17, unk10_18, unk10_22, id, unk18, unk1A, unk1B_0, unk1B_6);
    log->lastSlot = func_0200b26c(log, TRAINER_LOG_MAX, &entry);
    if (func_0200b2fc(log, TRAINER_LOG_MAX, &entry)) {
        func_0200b41c(log, TRAINER_LOG_MAX, &entry);
    }
    return log->lastSlot;
}

void func_0200b220(TrainerLogSave *log) {
    s32 i;

    for (i = 0; i < TRAINER_LOG_MAX; i++) {
        log->entries[i].today = 0;
    }
}

u32 func_0200b240(void) {
    return sizeof(TrainerLogSave);
}

void func_0200b248(TrainerLogSave *log) {
    sys_memset(log, 0, sizeof(TrainerLogSave));
    log->lastSlot = TRAINER_LOG_NONE;
}

u8 func_0200b26c(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry) {
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
            sys_memcpy(&log->entries[i - 1], &log->entries[i], sizeof(TrainerLogEntry));
        }
    }
    sys_memcpy(entry, &log->entries[slot], sizeof(TrainerLogEntry));
    return slot;
}

BOOL func_0200b2fc(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry) {
    s32 i;

    for (i = 0; i < count; i++) {
        if (entry->id == log->entries[i].id && log->entries[i].used == 1) {
            return TRUE;
        }
    }
    return FALSE;
}

void func_0200b330(TrainerLogEntry *entry, const u16 *name, u8 value, u32 count, u8 unk10_17, u8 unk10_18,
                   u8 unk10_22, u32 id, u16 unk18, u8 unk1A, u8 unk1B_0, u8 unk1B_6) {
    sys_memset(entry, 0, sizeof(TrainerLogEntry));
    wcharsncpy(name, entry->name, 8);
    entry->value = value;
    entry->id = id;
    entry->unk10_17 = unk10_17;
    entry->unk10_18 = unk10_18;
    entry->unk10_22 = unk10_22;
    entry->count = count;
    entry->unk18 = unk18;
    entry->unk1A = unk1A;
    entry->unk1B_0 = unk1B_0;
    entry->unk1B_6 = unk1B_6 & 1;
    entry->used = 1;
    entry->today = 1;
}

TrainerLogEntry *func_0200b3ec(TrainerLogSave *log, s32 count, u32 id) {
    s32 i;

    for (i = 0; i < count; i++) {
        if (id == log->entries[i].id && log->entries[i].used == 1) {
            return &log->entries[i];
        }
    }
    return NULL;
}

void func_0200b41c(TrainerLogSave *log, s32 count, const TrainerLogEntry *entry) {
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
