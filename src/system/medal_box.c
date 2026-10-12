#include "types.h"
#include "save/medal_box.h"
#include "gfl/std.h"
#include "save/save_control.h"

// The save block of the Medal Rally: what the player knows of each medal and the day it was obtained, the medal
// obtained last and the rank. The Pokédex's habitat list is kept at its end. The ROM has no string for the file, so
// the name follows swan's function names. Block 0x44.

#define SAVE_BLOCK_MEDAL_BOX 0x44

static MedalInfo *MedalBox_WriteMedal(MedalBox *box, u16 medal);
static const MedalInfo *MedalBox_ReadMedal(const MedalBox *box, u16 medal);
static void MedalInfo_SetStatus(MedalInfo *info, u8 status);
static void MedalInfo_SetDate(MedalInfo *info, u8 year, u8 month, u8 day);

static MedalInfo *MedalBox_WriteMedal(MedalBox *box, u16 medal) {
    return &box->medals[medal];
}

static const MedalInfo *MedalBox_ReadMedal(const MedalBox *box, u16 medal) {
    return &box->medals[medal];
}

static void MedalInfo_SetStatus(MedalInfo *info, u8 status) {
    info->status = status;
}

static void MedalInfo_SetDate(MedalInfo *info, u8 year, u8 month, u8 day) {
    info->year = year;
    info->month = month;
    info->day = day;
}

void MedalBox_Init(MedalBox *box) {
    sys_memset(box, 0, sizeof(MedalBox));
    box->lastMedal = 0xff;
    func_02010ca0(MedalBox_GetHabitatList(box));
}

u32 MedalBox_GetBlockSize(void) {
    return sizeof(MedalBox);
}

u8 MedalBox_GetMedalStatus(MedalBox *box, u16 medal) {
    return MedalBox_ReadMedal(box, medal)->status;
}

void MedalBox_GiveMedal(MedalBox *box, u16 medal) {
    if (MedalBox_GetMedalStatus(box, medal) < MEDAL_STATUS_EARNED) {
        MedalInfo_SetStatus(MedalBox_WriteMedal(box, medal), MEDAL_STATUS_EARNED);
    }
}

void MedalBox_DiscoverMedal(MedalBox *box, u16 medal) {
    MedalInfo *info = MedalBox_WriteMedal(box, medal);

    if (info->status == MEDAL_STATUS_UNKNOWN) {
        info->special = TRUE;
        MedalInfo_SetStatus(info, MEDAL_STATUS_1);
    }
}

void MedalBox_AcknowledgeMedal(MedalBox *box, u16 medal, u8 year, u8 month, u8 day) {
    MedalInfo *info = MedalBox_WriteMedal(box, medal);

    MedalInfo_SetStatus(info, MEDAL_STATUS_OBTAINED);
    MedalInfo_SetDate(info, year, month, day);
    info->special = TRUE;
}

void MedalBox_DiscoverInitialMedal(MedalBox *box, u16 medal, u8 year, u8 month, u8 day) {
    MedalInfo *info = MedalBox_WriteMedal(box, medal);

    MedalInfo_SetStatus(info, MEDAL_STATUS_DISCOVERED);
    MedalInfo_SetDate(info, year, month, day);
}

void MedalBox_SetMedalNotSpecial(MedalBox *box, u16 medal) {
    MedalBox_WriteMedal(box, medal)->special = FALSE;
}

void MedalBox_SetLastMedal(MedalBox *box, u8 medal) {
    box->lastMedal = medal;
}

void MedalBox_IncrementRank(MedalBox *box) {
    if (box->rank < MEDAL_RANK_MAX) {
        box->rank++;
    }
}

u8 MedalBox_GetLastMedal(MedalBox *box) {
    return box->lastMedal;
}

u8 MedalBox_GetRank(MedalBox *box) {
    return box->rank;
}

BOOL MedalBox_IsMedalSpecial(MedalBox *box, u16 medal) {
    return MedalBox_ReadMedal(box, medal)->special;
}

void MedalBox_GetMedalDate(MedalBox *box, u16 medal, u8 *year, u8 *month, u8 *day) {
    const MedalInfo *info = MedalBox_ReadMedal(box, medal);

    *year = info->year;
    *month = info->month;
    *day = info->day;
}

u32 MedalBox_GetNextRankRequirement(MedalBox *box) {
    switch (MedalBox_GetRank(box)) {
    case 0:
        return 49;
    case 1:
        return 99;
    case 2:
        return 149;
    case 3:
        return 199;
    case 4:
        return 200;
    }
    return 0;
}

u32 MedalBox_GetObtainedCount(MedalBox *box, BOOL countEarned) {
    u16 i;
    u16 count = 0;

    for (i = 0; i < MEDAL_COUNT; i++) {
        const MedalInfo *info = MedalBox_ReadMedal(box, i);
        u32 date = info->year + info->month + info->day;

        if (countEarned) {
            if (info->status == MEDAL_STATUS_OBTAINED || info->status == MEDAL_STATUS_DISCOVERED
                || (info->status == MEDAL_STATUS_EARNED && date != 0)) {
                count++;
            }
        } else if (info->status == MEDAL_STATUS_OBTAINED) {
            count++;
        }
    }
    return count;
}

u8 func_0200fb28(MedalBox *box) {
    return box->unk3FE;
}

void func_0200fb34(MedalBox *box) {
    box->unk3FE = TRUE;
}

MedalBox *SaveControl_GetMedalBox(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_MEDAL_BOX);
}

void *MedalBox_GetHabitatList(MedalBox *box) {
    return box->habitatList;
}
