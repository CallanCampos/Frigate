// The trainer card's data, which this file gathers from the save, and the procs that run overlay 186's trainer card
// screen and the screens it calls. The name is the ROM's string, from GFL_HeapAllocate's asserts

#include "types.h"
#include "app/medal_info.h"
#include "app/pms_select.h"
#include "constants/version.h"
#include "field/app_call.h"
#include "field/field.h"
#include "field/trcard_sys.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/rtc.h"
#include "save/adventure.h"
#include "save/c_gear_savedata.h"
#include "save/event_work.h"
#include "save/high_link.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/pokewood.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "save/wbt_save.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/pms.h"

// The steps of the proc, which runs the screens one after another
enum {
    TRCARD_SEQ_START,
    TRCARD_SEQ_CARD,
    TRCARD_SEQ_CARD_WAIT,
    TRCARD_SEQ_GREETING,
    TRCARD_SEQ_GREETING_WAIT,
    TRCARD_SEQ_OTHER,
    TRCARD_SEQ_OTHER_WAIT,
    TRCARD_SEQ_MEDAL,
    TRCARD_SEQ_MEDAL_WAIT,
    TRCARD_SEQ_END,
};

// The start of overlay 187's MedalInfoParam (app/medal_info.h), as far as its mode 1 reads it
typedef struct {
    u32 mode;
    u16 heapId;
    StrBuf *name;
    u16 medal;
    u16 medalCount;
    u8 rank;
    u8 year;
    u8 month;
    u8 day;
    // 1 to return to the field
    u32 result;
} TrainerCardMedalParam;

typedef struct {
    u32 heapId;
    u32 unk04;
    GameProcManager *procMgr;
    PMSSelectParam greeting;
    TrainerCardMedalParam medal;
    TrainerCardParam *param;
    // The card's unk06 when the screen opened
    u32 unk34;
} TrainerCardSysWork;

static BOOL func_ov012_02169274(GameProc *proc, u32 *state, void *param, void *work);
static BOOL func_ov012_021692a4(GameProc *proc, u32 *state, void *param, void *work);
static BOOL func_ov012_0216936c(GameProc *proc, u32 *state, void *param, void *work);
static BOOL func_ov012_021691c0(GameProc *proc, u32 *state, void *param, void *work);
static BOOL func_ov012_02169324(GameProc *proc, u32 *state, void *param, void *work);

// The order of the declarations gives the order of the tables, which MWCC sorts by size
const GameProcFunctions data_ov012_0216dd78 = {
    func_ov012_021691c0,
    func_ov012_021692a4,
    func_ov012_02169324,
};

const GameProcFunctions data_ov012_0216dd6c = {
    func_ov012_02169274,
    func_ov012_021692a4,
    func_ov012_0216936c,
};

static const GameProcFunctions data_ov012_0216dd60 = {
    func_ov186_021a75a0,
    func_ov186_021a7864,
    func_ov186_021a7a20,
};

// The records that each sum on the card adds up, ending at 0
static const u16 data_ov012_0216dd84[9][7] = {
    { 0xc, 0xd, 0x10, 0x11, 0x24, 0x25, 0x5a },
    { 0xd, 0x11, 0x25 },
    { 0xe, 0x12, 0x26 },
    { 0xf, 0x13, 0x27 },
    { 0xc, 0x10, 0x24, 0x5a },
    { 0x5 },
    { 0x6 },
    { 0x71 },
    { 0x72, 0x74 },
};

static u32 func_ov012_02169388(TrainerCardSysWork *wk, TrainerCardParam *param);
static u32 func_ov012_021693b4(TrainerCardSysWork *wk);
static u32 func_ov012_021693e0(TrainerCardSysWork *wk);
static u32 func_ov012_02169414(TrainerCardSysWork *wk);
static u32 func_ov012_02169450(TrainerCardSysWork *wk);
static u32 func_ov012_021694ac(TrainerCardSysWork *wk);
static u32 func_ov012_021694e4(TrainerCardSysWork *wk);
static u32 func_ov012_02169668(TrainerCardSysWork *wk);
static u32 func_ov012_0216970c(TrainerCardSysWork *wk);
static u32 func_ov012_02169738(GameRecords *records, u32 row, u32 max);

// Runs the screen's proc manager, and frees it when the screen has ended
static BOOL func_ov012_0216919c(GameProcManager **procMgr) {
    if (*procMgr != NULL && GFL_ProcMgrUpdate(*procMgr) == FALSE) {
        FreeGameProcManager(*procMgr);
        *procMgr = NULL;
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov012_021691c0(GameProc *proc, u32 *state, void *param, void *work) {
    TrainerCardParam *cardParam = param;
    TrainerCardSysWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_TRAINER_CARD, 0x29000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(TrainerCardSysWork), HEAPID_TRAINER_CARD);
    sys_memset(wk, 0, sizeof(TrainerCardSysWork));
    wk->heapId = HEAPID_TRAINER_CARD;
    wk->param = GFL_HeapAllocate(wk->heapId, sizeof(TrainerCardParam), TRUE, "trcard_sys.c", 178);
    *wk->param = *cardParam;
    wk->param->data = GFL_HeapAllocate(wk->heapId, sizeof(TrainerCardData), TRUE, "trcard_sys.c", 180);
    wk->param->shownData = wk->param->data;
    func_ov012_02169770(wk->param->shownData, cardParam->gameData, FALSE, wk->param->canEdit, wk->heapId);
    func_ov012_02169508(wk->param->data, cardParam->gameData, wk->heapId);
    wk->unk34 = wk->param->shownData->unk06;
    return TRUE;
}

static BOOL func_ov012_02169274(GameProc *proc, u32 *state, void *param, void *work) {
    TrainerCardSysWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_TRAINER_CARD, 0x29000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(TrainerCardSysWork), HEAPID_TRAINER_CARD);
    sys_memset(wk, 0, sizeof(TrainerCardSysWork));
    wk->heapId = HEAPID_TRAINER_CARD;
    wk->param = param;
    return TRUE;
}

static BOOL func_ov012_021692a4(GameProc *proc, u32 *state, void *param, void *work) {
    switch (*state) {
    case TRCARD_SEQ_START:
        *state = func_ov012_02169388(work, param);
        break;
    case TRCARD_SEQ_CARD:
        *state = func_ov012_021693b4(work);
        break;
    case TRCARD_SEQ_CARD_WAIT:
        *state = func_ov012_021693e0(work);
        break;
    case TRCARD_SEQ_GREETING:
        *state = func_ov012_02169414(work);
        break;
    case TRCARD_SEQ_GREETING_WAIT:
        *state = func_ov012_02169450(work);
        break;
    case TRCARD_SEQ_OTHER:
        *state = func_ov012_021694ac(work);
        break;
    case TRCARD_SEQ_OTHER_WAIT:
        *state = func_ov012_021694e4(work);
        break;
    case TRCARD_SEQ_MEDAL:
        *state = func_ov012_02169668(work);
        break;
    case TRCARD_SEQ_MEDAL_WAIT:
        *state = func_ov012_0216970c(work);
        break;
    case TRCARD_SEQ_END:
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov012_02169324(GameProc *proc, u32 *state, void *param, void *work) {
    TrainerCardParam *cardParam = param;
    TrainerCardSysWork *wk = work;

    if (cardParam->canEdit) {
        u32 value = func_02008bf4(GetGameDataPlayerInfo(cardParam->gameData));

        if (wk->unk34 != value) {
            GameBeaconSys_SetTrainerView(value);
        }
    }
    cardParam->result = wk->param->result;
    GFL_HeapFree(wk->param->shownData);
    GFL_HeapFree(wk->param);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_TRAINER_CARD);
    return TRUE;
}

static BOOL func_ov012_0216936c(GameProc *proc, u32 *state, void *param, void *work) {
    TrainerCardSysWork *wk = work;

    GFL_HeapFree(wk->param);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_TRAINER_CARD);
    return TRUE;
}

static u32 func_ov012_02169388(TrainerCardSysWork *wk, TrainerCardParam *param) {
    switch (param->appParam) {
    case 0:
        break;
    case 1:
        return TRCARD_SEQ_CARD;
    case 2:
        return TRCARD_SEQ_CARD;
    case 3:
        return TRCARD_SEQ_OTHER;
    }
    return TRCARD_SEQ_CARD;
}

static u32 func_ov012_021693b4(TrainerCardSysWork *wk) {
    if (wk->procMgr == NULL) {
        wk->procMgr = CreateGameProcManager(wk->heapId);
    }
    QueueGameProc(wk->procMgr, OVERLAY_NONE, &data_ov012_0216dd60, wk->param);
    return TRCARD_SEQ_CARD_WAIT;
}

static u32 func_ov012_021693e0(TrainerCardSysWork *wk) {
    if (!func_ov012_0216919c(&wk->procMgr)) {
        return TRCARD_SEQ_CARD_WAIT;
    }
    if (wk->param->next == 1) {
        return TRCARD_SEQ_GREETING;
    }
    if (wk->param->next == 2) {
        return TRCARD_SEQ_OTHER;
    }
    if (wk->param->next == 4) {
        return TRCARD_SEQ_MEDAL;
    }
    return TRCARD_SEQ_END;
}

static u32 func_ov012_02169414(TrainerCardSysWork *wk) {
    if (wk->procMgr == NULL) {
        wk->procMgr = CreateGameProcManager(wk->heapId);
    }
    wk->greeting.save = GameData_GetSaveControl(wk->param->gameData);
    QueueGameProc(wk->procMgr, OVERLAY_ID(185), &PMS_SELECT_PROC_FUNCTIONS, &wk->greeting);
    return TRCARD_SEQ_GREETING_WAIT;
}

static u32 func_ov012_02169450(TrainerCardSysWork *wk) {
    void *cgear;

    if (!func_ov012_0216919c(&wk->procMgr)) {
        return TRCARD_SEQ_GREETING_WAIT;
    }
    if (wk->greeting.result != NULL) {
        cgear = getCGearDataBlkAddress(GameData_GetSaveControl(wk->param->gameData));
        wk->param->shownData->greeting = *wk->greeting.result;
        CGearPhrases_Set(cgear, 0, wk->greeting.result);
        GameBeaconSys_SetCGearRecord(wk->greeting.result);
    }
    return TRCARD_SEQ_CARD;
}

static u32 func_ov012_021694ac(TrainerCardSysWork *wk) {
    if (wk->procMgr == NULL) {
        wk->procMgr = CreateGameProcManager(wk->heapId);
    }
    wk->greeting.save = GameData_GetSaveControl(wk->param->gameData);
    QueueGameProc(wk->procMgr, OVERLAY_ID(185), &data_ov186_021ad288, wk->param);
    return TRCARD_SEQ_OTHER_WAIT;
}

static u32 func_ov012_021694e4(TrainerCardSysWork *wk) {
    if (!func_ov012_0216919c(&wk->procMgr)) {
        return TRCARD_SEQ_OTHER_WAIT;
    }
    if (wk->param->next == 3) {
        return TRCARD_SEQ_CARD;
    }
    return TRCARD_SEQ_END;
}

void func_ov012_02169508(TrainerCardData *data, GameData *gameData, HeapID heapId) {
    SaveControl *save;
    MedalBox *box;
    void *wbt;
    JoinAvenueInfo *avenue;
    GameRecords *records;
    void *unk;
    EventWork *eventWork;
    u32 wins = 0;
    u32 i;
    u8 year;
    u8 month;
    u8 day;

    save = GameData_GetSaveControl(gameData);
    box = SaveControl_GetMedalBox(save);
    wbt = func_0200fea0(save);
    avenue = JoinAvenue_GetInfo(SaveControl_GetJoinAvenue(save));
    records = getTrainerCardInfoBlkAddress(save);
    unk = func_02010dec(save);
    data->lastMedal = MedalBox_GetLastMedal(box);
    data->medalCount = MedalBox_GetObtainedCount(box, 0);
    data->medalRank = MedalBox_GetRank(box);
    if (data->lastMedal < 0xff) {
        MedalBox_GetMedalDate(box, data->lastMedal, &year, &month, &day);
        data->medalYear = year;
        data->medalMonth = month;
        data->medalDay = day;
    }
    for (i = 0; i < 29; i++) {
        wins += func_0200feac(wbt, i);
    }
    if (wins > 9999) {
        wins = 9999;
    }
    data->pwtWins = wins;
    data->unk688 = RecordGet(records, 0x83);
    JoinAvenue_GetParam(avenue, 0, data->avenueName);
    data->unk68A = RecordGet(records, 0x7f);
    data->unk684 = func_02010e50(unk);
    eventWork = GameData_GetEventWork(gameData);
    if (data->pwtWins != 0) {
        data->unk690_2 = TRUE;
    }
    if (EventWork_FlagGet(eventWork, 0x986) == TRUE) {
        data->unk690_0 = TRUE;
        data->unk68_1 = TRUE;
    }
    if (data->unk688 != 0) {
        data->unk690_1 = TRUE;
    }
    if (data->unk68A != 0) {
        data->unk690_3 = TRUE;
    }
}

static u32 func_ov012_02169668(TrainerCardSysWork *wk) {
    // Read but unused
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(wk->param->gameData));

    if (wk->procMgr == NULL) {
        wk->procMgr = CreateGameProcManager(wk->heapId);
    }
    wk->medal.mode = 1;
    wk->medal.heapId = wk->heapId;
    wk->medal.name = GFL_StrBufCreate(16, wk->heapId);
    GFL_StrBufLoadString(wk->medal.name, wk->param->shownData->name);
    wk->medal.medal = wk->param->data->lastMedal;
    wk->medal.medalCount = wk->param->data->medalCount;
    wk->medal.rank = wk->param->data->medalRank;
    wk->medal.year = wk->param->data->medalYear;
    wk->medal.month = wk->param->data->medalMonth;
    wk->medal.day = wk->param->data->medalDay;
    QueueGameProc(wk->procMgr, OVERLAY_ID(187), &MedalInfo_ProcFunctions, &wk->medal);
    return TRCARD_SEQ_MEDAL_WAIT;
}

static u32 func_ov012_0216970c(TrainerCardSysWork *wk) {
    if (!func_ov012_0216919c(&wk->procMgr)) {
        return TRCARD_SEQ_MEDAL_WAIT;
    }
    GFL_StrBufFree(wk->medal.name);
    if (wk->medal.result == 1) {
        wk->param->result = 1;
        return TRCARD_SEQ_END;
    }
    return TRCARD_SEQ_CARD;
}

// Adds up a row of the records, up to max
static u32 func_ov012_02169738(GameRecords *records, u32 row, u32 max) {
    u32 sum = 0;
    int i;

    for (i = 0; i < 7; i++) {
        if (data_ov012_0216dd84[row][i] == 0) {
            break;
        }
        sum += RecordGet(records, data_ov012_0216dd84[row][i]);
        if (sum > max) {
            sum = max;
        }
    }
    return sum;
}

void func_ov012_02169770(TrainerCardData *data, GameData *gameData, BOOL isSnapshot, BOOL canEdit, HeapID heapId) {
    RTCDate date;
    RTCTime time;
    u8 passPowerFlags[2];
    SaveControl *save = GameData_GetSaveControl(gameData);
    void *timeSig = getTimeSigBlkAddress(save);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    TrainerCardSave *card = getTrainerCardDataBlkAddress(gameData);
    AdventureTime *adventureTime = getSaveAdventureTimeBlock(save);
    GameRecords *records = getTrainerCardInfoBlkAddress(save);
    void *cgear = CGearPhrases_GetBlock(save);
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);
    HighLinkSave *highLink;
    void *passPowerData;
    u8 i;
    u8 bit;

    if (!func_02008b5c(playerInfo)) {
        sys_memcpy16(GetPlayerName(playerInfo), data->name, sizeof(data->name));
    } else {
        data->name[0] = 'N';
        data->name[1] = 'o';
        data->name[2] = 'N';
        data->name[3] = 'a';
        data->name[4] = 'm';
        data->name[5] = 'e';
        data->name[6] = GFL_StrBufGetTerminator();
    }
    data->stars = func_ov012_02169b78(gameData);
    data->gender = getTrainerGender(playerInfo);
    data->trainerId = getIDAsUInt(playerInfo);
    data->money = getCash(card);
    data->badges = 0;
#ifdef BLACK2
    data->version = VERSION_BLACK2;
#else
    data->version = VERSION_WHITE2;
#endif
    for (i = 0, bit = 1; i < 8; bit <<= 1, i++) {
        if (isBadgeObtained(card, i) == TRUE) {
            data->badges |= bit;
        }
    }
    if (isSnapshot == TRUE) {
        PlayTime *playTime = (PlayTime *)func_02017a40(gameData);

        data->unk06 = func_02008bf4(playerInfo);
        data->isOwn = FALSE;
        data->playTime = NULL;
        data->playHours = func_02008cec(playTime);
        data->playMinutes = func_02008cf0(playTime);
    } else {
        data->unk06 = func_02008bf4(playerInfo);
        data->isOwn = TRUE;
        data->playTime = (PlayTime *)func_02017a40(gameData);
    }
    data->unk48 = func_020091e8(timeSig);
    data->unk04_4 = func_02009204(timeSig);
    CGearPhrases_Get(cgear, 0, &data->greeting);
    func_0207d244(&date, &time, adventureTime->startSeconds);
    data->startYear = date.year;
    data->startMonth = date.month;
    data->startDay = date.day;
    if (adventureTime->seconds == 0) {
        data->clearYear = 0;
        data->clearMonth = 0;
        data->clearDay = 0;
        data->clearHour = 0;
        data->clearMinute = 0;
    } else {
        func_0207d244(&date, &time, adventureTime->seconds);
        data->clearYear = date.year;
        data->clearMonth = date.month;
        data->clearDay = date.day;
        data->clearHour = time.hour;
        data->clearMinute = time.minute;
    }
    data->unk38 = func_ov012_02169738(records, 0, 999999999);
    data->unk3C = func_ov012_02169738(records, 1, 999999999);
    data->unk40 = func_ov012_02169738(records, 2, 999999999);
    data->unk44 = func_ov012_02169738(records, 3, 999999999);
    data->unk4C = func_ov012_02169738(records, 4, 999999999);
    data->unk50 = func_ov012_02169738(records, 5, 999999999);
    data->unk54 = func_ov012_02169738(records, 6, 999999999);
    data->unk58 = func_0200c924(card);
    data->unk5C = func_ov012_02169738(records, 7, 9999);
    highLink = getHighLinkBlockAddress(save);
    passPowerData = PassPowerData_Create(heapId);
    func_0200c6d8(highLink, passPowerFlags, 2);
    data->passPowerCount = GetUnlockedPassPowerCount(passPowerData, func_02017208(gameData), passPowerFlags);
    PassPowerData_Free(passPowerData);
    data->unk60 = func_ov012_02169738(records, 8, 9999);
    data->palParkHighScore = TrainerGameInfo_GetPalParkHighScore(card);
    data->unk64 = func_02009650(records);
    data->battleTestRank = func_02009628(records);
    data->unk68_0 = func_020098c0(func_02009924(gameData));
    if (data->unk60 != 0) {
        data->unk68_2 = TRUE;
    }
    if (RecordGet(records, 0x78) != 0) {
        data->unk68_3 = TRUE;
    }
    if (RecordGet(records, 0x79) != 0) {
        data->unk68_4 = TRUE;
    }
    if (EventWork_FlagGet(GameData_GetEventWork(gameData), 0x962) == TRUE) {
        data->unk04_2 = TRUE;
    } else {
        data->unk04_2 = FALSE;
    }
    data->seenCount = countSeenDexPokes(pokedex, heapId);
    data->unk04_3 = func_020091ac(timeSig);
    sys_memcpy(func_020091a8(timeSig), data->unk74, sizeof(data->unk74));
    data->canEdit = canEdit;
}

void *func_ov012_02169b04(GameData *gameData, HeapID heapId, BOOL canEdit) {
    TrainerCardParam *param = GFL_HeapAllocate(heapId, sizeof(TrainerCardParam), TRUE, "trcard_sys.c", 975);

    sys_memset(param, 0, sizeof(TrainerCardParam));
    param->shownData = NULL;
    param->gameData = gameData;
    param->next = 0;
    param->canEdit = canEdit;
    return param;
}

TrainerCardParam *func_ov012_02169b3c(GameData *gameData, TrainerCardData *data, HeapID heapId) {
    TrainerCardParam *param = GFL_HeapAllocate(heapId, sizeof(TrainerCardParam), TRUE, "trcard_sys.c", 1000);

    param->data = data;
    param->shownData = data;
    param->gameData = gameData;
    param->next = 0;
    data->canEdit = FALSE;
    param->shownData->unk04_7 = TRUE;
    return param;
}

// The stars on the card
u32 func_ov012_02169b78(GameData *gameData) {
    u32 stars = 0;
    SaveControl *save = GameData_GetSaveControl(gameData);
    // Read but unused
    BSubwayScoreData *bsubway = func_0201795c(gameData);
    MusicalSave *musical = getMusicalInfoBlkAddress(gameData);
    void *levels = func_02017208(gameData);
    PokewoodSave *pokewood = func_02011040(gameData);
    void *wbt = func_0200fea0(save);

    if (EventWork_FlagGet(GameData_GetEventWork(gameData), 0x960)) {
        stars++;
    }
    if (PokeDex_IsCompleteNational(GameData_GetPokedex(gameData))) {
        stars++;
    }
    if (func_0200c5dc(levels) >= 30 && func_0200c5e0(levels) >= 30) {
        stars++;
    }
    if (func_0200feac(wbt, 17) != 0) {
        stars++;
    }
    if ((int)func_020112d8(pokewood, 0) >= POKEWOOD_MOVIE_COUNT) {
        stars++;
    }
    return stars;
}

u16 func_ov012_02169c04(TrainerCardData *data) {
    return data->medalCount;
}

u8 func_ov012_02169c10(TrainerCardData *data) {
    return data->medalRank;
}
