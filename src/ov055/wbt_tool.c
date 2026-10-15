#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "field/wbt.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "nitro/math.h"
#include "pml/poke_party.h"
#include "save/event_work.h"
#include "save/wbt_save.h"
#include "system/game_data.h"

typedef struct {
    u16 name;
    u8 count;
    u32 unk4;
} WbtBattleStyle;

static const u16 sUnk5Messages[] = { 248, 251, 254 };
static const u8 sUnk74fe[] = {
    2, 50, 24, 17, 52, 74, 57, 15, 3, 49, 25, 18, 46, 65, 62, 14, 27, 28, 70, 71, 4, 5, 35, 36, 51, 64, 66, 67, 48, 87,
};
static const WbtBattleStyle sBattleStyles[] = {
    { 0, 3, 0 },
    { 1, 4, 1 },
    { 2, 6, 2 },
    { 3, 4, 3 },
};

static int func_ov055_021e647c(int style);
static void func_ov055_021e65c8(GameData *gameData, u16 *counts);
static BOOL func_ov055_021e65e8(GameData *gameData, u32 tournament, u16 *counts);

void func_ov055_021e6388(HeapID heapId, u32 message, StrBuf *strbuf) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SCRCMD_BSUBWAY, heapId);

    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, message, strbuf);
        GFL_MsgDataFree(msgData);
    }
}

void func_ov055_021e63b4(HeapID heapId, u32 message, StrBuf *strbuf) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SCRCMD_BSUBWAY_2, heapId);

    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, message, strbuf);
        GFL_MsgDataFree(msgData);
    }
}

WbtTrainers *func_ov055_021e63e0(HeapID heapId) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(247, HEAPID_TAIL(heapId));
    u32 size = GFL_ArcToolGetDataLength(handle, 0);
    WbtTrainers *trainers = GFL_HeapAllocate(heapId, size + 4, TRUE, "wbt_tool.c", 126);

    trainers->count = size / sizeof(WbtTrainer);
    GFL_ArcToolRead(handle, 0, trainers->trainers);
    GFL_ArcToolFree(handle);
    return trainers;
}

void func_ov055_021e6438(WbtTrainers *trainers) {
    GFL_HeapFree(trainers);
}

void func_ov055_021e6440(WbtTrainers *trainers, u32 index, WbtTrainer *dest) {
    *dest = trainers->trainers[index];
}

BOOL func_ov055_021e6458(WbtTrainers *trainers, u8 index) {
    if (index < trainers->count) {
        return TRUE;
    }
    return FALSE;
}

u16 func_ov055_021e6468(WbtTrainers *trainers, u8 index) {
    return trainers->trainers[index].unk2;
}

int func_ov055_021e6470(WbtTrainers *trainers, u8 index) {
    return trainers->trainers[index].unk8_0;
}

// Meant to keep the style in range, but it can never change it
static int func_ov055_021e647c(int style) {
    if (style < 0 && style > 3) {
        style = 0;
    }
    return style;
}

void func_ov055_021e6488(int style, StrBuf *strbuf) {
    u16 message = sBattleStyles[func_ov055_021e647c(style)].name;
    MsgData *msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SCRCMD_BSUBWAY_WBT_PARTY, HEAPID_FIELDMAP);

    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, message, strbuf);
        GFL_MsgDataFree(msgData);
    }
}

u8 func_ov055_021e64c0(int style) {
    return sBattleStyles[func_ov055_021e647c(style)].count;
}

u32 func_ov055_021e64d4(int style) {
    return sBattleStyles[func_ov055_021e647c(style)].unk4;
}

int func_ov055_021e64e8(int style) {
    return func_ov055_021e647c(style);
}

u8 func_ov055_021e64f0(PokeParty *party, u64 seed) {
    u8 counts[17];
    u8 types[17];
    MATHRandContext32 rand;
    PartyPkm *pkm;
    u32 i;
    u32 count;
    u32 type1;
    u32 type2;
    u8 max;
    u8 n;

    sys_memset(counts, 0, sizeof(counts));
    count = PokeParty_GetPkmCount(party);
    for (i = 0; i < count; i++) {
        pkm = PokeParty_GetPkm(party, i);
        type1 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE1, NULL);
        type2 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE2, NULL);
        if (type1 != 17) {
            counts[type1]++;
        }
        if (type1 != type2 && type2 != 17) {
            counts[type2]++;
        }
    }
    max = 0;
    n = 0;
    sys_memset(types, 0, sizeof(types));
    for (i = 0; i < 17; i++) {
        if (counts[i] > max) {
            max = counts[i];
        }
    }
    for (i = 0; i < 17; i++) {
        if (counts[i] == max) {
            types[n] = i;
            n++;
        }
    }
    MATH_InitRand32(&rand, seed);
    return types[MATH_Rand32(&rand, n)];
}

static void func_ov055_021e65c8(GameData *gameData, u16 *counts) {
    void *save = func_020179f8(gameData);
    int i;

    for (i = 0; i < 29; i++) {
        counts[i] = func_0200feac(save, i);
    }
}

// Whether the tournament is open, by the save's counts of wins. The leaders' tournaments of the other regions open one
// after another
static BOOL func_ov055_021e65e8(GameData *gameData, u32 tournament, u16 *counts) {
    EventWork *eventWork = GameData_GetEventWork(gameData);
    BOOL allLeaders = FALSE;
    BOOL throughHoenn = FALSE;
    BOOL throughJohto = FALSE;
    BOOL throughKanto = FALSE;

    if (counts[19] != 0 && counts[20] != 0) {
        throughKanto = TRUE;
    }
    if (throughKanto && counts[21] != 0) {
        throughJohto = TRUE;
    }
    if (throughJohto && counts[22] != 0) {
        throughHoenn = TRUE;
    }
    if (throughHoenn && counts[23] != 0) {
        allLeaders = TRUE;
    }
    switch (tournament) {
    case 0:
        break;
    case 1:
        return counts[24] >= 10;
    case 2:
        return allLeaders;
    case 3:
        return counts[18] != 0;
    case 11:
        return counts[18] == 0;
    case 4:
    case 12:
    case 14:
        return counts[18] != 0;
    case 5:
        return EventWork_FlagGet(eventWork, 2400);
    case 6:
    case 7:
    case 8:
    case 9:
        return counts[19] != 0;
    case 13:
        return counts[25] != 0 && allLeaders;
    case 15:
        return counts[27] != 0 && allLeaders;
    case 10:
        return allLeaders;
    }
    return FALSE;
}

BOOL func_ov055_021e66dc(GameData *gameData, u32 tournament) {
    u16 counts[30];

    func_ov055_021e65c8(gameData, counts);
    return func_ov055_021e65e8(gameData, tournament, counts);
}

BOOL func_ov055_021e66fc(GameData *gameData, u32 won, u32 tournament) {
    u16 counts[30];
    BOOL before;
    BOOL after;
    u32 record;

    func_ov055_021e65c8(gameData, counts);
    before = func_ov055_021e65e8(gameData, tournament, counts);
    record = func_ov055_021e6750(won, 0);
    if (record != 29) {
        counts[record]++;
    }
    after = func_ov055_021e65e8(gameData, tournament, counts);
    if (before == FALSE && after != before) {
        return TRUE;
    }
    return FALSE;
}

u32 func_ov055_021e6750(u32 tournament, u32 type) {
    if (tournament != 2) {
        type = func_ov036_021c98a4(tournament)->record;
    }
    return type;
}

u32 func_ov055_021e6760(int record) {
    u32 i;

    if (record <= 16) {
        return 2;
    }
    for (i = 0; i < NELEMS(data_ov036_021d4920); i++) {
        if (record == data_ov036_021d4920[i].record) {
            return i + 1;
        }
    }
    return 4;
}

u32 func_ov055_021e6794(u32 tournament) {
    return func_ov036_021c98a4(tournament)->kind;
}

BOOL func_ov055_021e67a0(u32 tournament, u32 value) {
    const WbtTournamentInfo *info = func_ov036_021c98a4(tournament);

    if (info->unk3 == value) {
        return TRUE;
    }
    return FALSE;
}

u16 func_ov055_021e67b8(u32 tournament) {
    return sUnk5Messages[func_ov036_021c98a4(tournament)->unk5];
}

u32 func_ov055_021e67cc(u32 tournament, int style) {
    int index = func_ov055_021e647c(style);

    return func_ov036_021c98a4(tournament)->battlePoints[index];
}

u8 func_ov055_021e67e4(u32 index) {
    if (index >= NELEMS(sUnk74fe)) {
        index = 0;
    }
    return sUnk74fe[index];
}
