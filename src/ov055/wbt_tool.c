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

static int WbtStyle_GetIndex(int style);
static void WbtTournament_LoadWinCounts(GameData *gameData, u16 *counts);
static BOOL WbtTournament_IsOpenByCounts(GameData *gameData, u32 tournament, u16 *counts);

void WbtTool_LoadTrainerName(HeapID heapId, u32 message, StrBuf *strbuf) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SCRCMD_BSUBWAY, heapId);

    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, message, strbuf);
        GFL_MsgDataFree(msgData);
    }
}

void WbtTool_LoadTrainerClassName(HeapID heapId, u32 message, StrBuf *strbuf) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SCRCMD_BSUBWAY_2, heapId);

    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, message, strbuf);
        GFL_MsgDataFree(msgData);
    }
}

WbtTrainers *WbtTrainers_Load(HeapID heapId) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(247, HEAPID_TAIL(heapId));
    u32 size = GFL_ArcToolGetDataLength(handle, 0);
    WbtTrainers *trainers = GFL_HeapAllocate(heapId, size + 4, TRUE, "wbt_tool.c", 126);

    trainers->count = size / sizeof(WbtTrainer);
    GFL_ArcToolRead(handle, 0, trainers->trainers);
    GFL_ArcToolFree(handle);
    return trainers;
}

void WbtTrainers_Free(WbtTrainers *trainers) {
    GFL_HeapFree(trainers);
}

void WbtTrainers_Get(WbtTrainers *trainers, u32 index, WbtTrainer *dest) {
    *dest = trainers->trainers[index];
}

BOOL WbtTrainers_IsValid(WbtTrainers *trainers, u8 index) {
    if (index < trainers->count) {
        return TRUE;
    }
    return FALSE;
}

u16 WbtTrainers_GetObjCode(WbtTrainers *trainers, u8 index) {
    return trainers->trainers[index].unk2;
}

int WbtTrainers_GetRank(WbtTrainers *trainers, u8 index) {
    return trainers->trainers[index].unk8_0;
}

// Meant to keep the style in range, but it can never change it
static int WbtStyle_GetIndex(int style) {
    if (style < 0 && style > 3) {
        style = 0;
    }
    return style;
}

void WbtStyle_LoadName(int style, StrBuf *strbuf) {
    u16 message = sBattleStyles[WbtStyle_GetIndex(style)].name;
    MsgData *msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SCRCMD_BSUBWAY_WBT_PARTY, HEAPID_FIELDMAP);

    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, message, strbuf);
        GFL_MsgDataFree(msgData);
    }
}

u8 WbtStyle_GetPokemonCount(int style) {
    return sBattleStyles[WbtStyle_GetIndex(style)].count;
}

u32 WbtStyle_GetBattleMode(int style) {
    return sBattleStyles[WbtStyle_GetIndex(style)].unk4;
}

int WbtStyle_Clamp(int style) {
    return WbtStyle_GetIndex(style);
}

u8 WbtTool_GetPartyMainType(PokeParty *party, u64 seed) {
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

    MI_CpuFill8(counts, 0, sizeof(counts));
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
    MI_CpuFill8(types, 0, sizeof(types));
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

static void WbtTournament_LoadWinCounts(GameData *gameData, u16 *counts) {
    void *save = GameData_GetWbtSave(gameData);
    int i;

    for (i = 0; i < 29; i++) {
        counts[i] = WbtSave_GetCount(save, i);
    }
}

// Whether the tournament is open, by the save's counts of wins. The leaders' tournaments of the other regions open one
// after another
static BOOL WbtTournament_IsOpenByCounts(GameData *gameData, u32 tournament, u16 *counts) {
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

BOOL WbtTournament_IsOpen(GameData *gameData, u32 tournament) {
    u16 counts[30];

    WbtTournament_LoadWinCounts(gameData, counts);
    return WbtTournament_IsOpenByCounts(gameData, tournament, counts);
}

BOOL WbtTournament_WouldOpen(GameData *gameData, u32 won, u32 tournament) {
    u16 counts[30];
    BOOL before;
    BOOL after;
    u32 record;

    WbtTournament_LoadWinCounts(gameData, counts);
    before = WbtTournament_IsOpenByCounts(gameData, tournament, counts);
    record = WbtTournament_GetRecord(won, 0);
    if (record != 29) {
        counts[record]++;
    }
    after = WbtTournament_IsOpenByCounts(gameData, tournament, counts);
    if (before == FALSE && after != before) {
        return TRUE;
    }
    return FALSE;
}

u32 WbtTournament_GetRecord(u32 tournament, u32 type) {
    if (tournament != 2) {
        type = GetPWTTournamentInfo(tournament)->record;
    }
    return type;
}

u32 WbtTournament_FromRecord(int record) {
    u32 i;

    if (record <= 16) {
        return 2;
    }
    for (i = 0; i < NELEMS(PWT_TOURNAMENT_INFO); i++) {
        if (record == PWT_TOURNAMENT_INFO[i].record) {
            return i + 1;
        }
    }
    return 4;
}

u32 WbtTournament_GetKind(u32 tournament) {
    return GetPWTTournamentInfo(tournament)->kind;
}

BOOL WbtTournament_IsInCategory(u32 tournament, u32 value) {
    const WbtTournamentInfo *info = GetPWTTournamentInfo(tournament);

    if (info->unk3 == value) {
        return TRUE;
    }
    return FALSE;
}

u16 WbtTournament_GetTrainerArc(u32 tournament) {
    return sUnk5Messages[GetPWTTournamentInfo(tournament)->unk5];
}

u32 WbtTournament_GetBattlePoints(u32 tournament, int style) {
    int index = WbtStyle_GetIndex(style);

    return GetPWTTournamentInfo(tournament)->battlePoints[index];
}

u8 WbtTool_GetDownloadedTrainerClass(u32 index) {
    if (index >= NELEMS(sUnk74fe)) {
        index = 0;
    }
    return sUnk74fe[index];
}
