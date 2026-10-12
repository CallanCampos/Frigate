#include "app/digitalcardcheck.h"
#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "save/box.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/tournament_block.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/event_fishing.h"

// The kinds of cards, which field 10 of the entry of the tournament block says
enum {
    CARD_TYPE_1 = 1,
    CARD_TYPE_2,
    CARD_TYPE_3,
    CARD_TYPE_4,
    CARD_TYPE_5,
};

// What the menu holds
enum {
    MENU_YES_NO,
    MENU_BACK,
    MENU_UNREGISTER_BACK,
};

// The three bytes of a digital player ID, the first in the high byte
static inline u32 DigitalCardCheck_PackBytes(u32 high, u32 middle, u32 low) {
    return (high << 24) | ((middle & 0xff) << 16) | ((low & 0xff) << 8);
}

struct DigitalCardCheck {
    // The messages of the digital card (event_fishing.h's)
    MsgData *msgData;
    WordSet *wordSet;
    WifiBattleMatchCard *card;
    WifiBattleMatchMenu *menu;
    WifiBattleMatchTask *task;
    DigitalCardCheckParam param;
    u16 heapId;
};

static void DigitalCardCheck_TaskStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType1(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType2(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType3(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType4(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType5(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskNoCard(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work);
static void DigitalCardCheck_CreateCard(DigitalCardCheck *check);
static void DigitalCardCheck_DeleteCard(DigitalCardCheck *check);
static BOOL DigitalCardCheck_IsCardReady(DigitalCardCheck *check);
static BOOL DigitalCardCheck_IsCardDone(DigitalCardCheck *check);
static void DigitalCardCheck_ShowCard(DigitalCardCheck *check, u32 page);
static void DigitalCardCheck_OpenMenu(DigitalCardCheck *check, u32 kind);
static void DigitalCardCheck_CloseMenu(DigitalCardCheck *check);
static s32 DigitalCardCheck_GetMenuChoice(DigitalCardCheck *check);
static void DigitalCardCheck_Print(DigitalCardCheck *check, u32 msgId, u32 style);
static BOOL DigitalCardCheck_IsPrinted(DigitalCardCheck *check);
static void DigitalCardCheck_SetBGEnabled(DigitalCardCheck *check, u32 enabled);
static void DigitalCardCheck_ClearMessage(DigitalCardCheck *check);

DigitalCardCheck *DigitalCardCheck_Create(const DigitalCardCheckParam *param, HeapID heapId) {
    DigitalCardCheck *check = GFL_HeapAllocate(heapId, sizeof(DigitalCardCheck), FALSE, "digitalcardcheck.c", 155);

    sys_memset(check, 0, sizeof(DigitalCardCheck));
    check->param = *param;
    check->heapId = heapId;
    WifiBattleMatchRes_LoadBG(check->param.seq, 3, heapId);
    check->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING, heapId);
    check->wordSet = GFL_WordSetSystemCreate(8, 64, heapId);
    check->task = WifiBattleMatchTask_Create(check, DigitalCardCheck_TaskStart, heapId);
    return check;
}

void DigitalCardCheck_Delete(DigitalCardCheck *check) {
    WifiBattleMatchTask_Free(check->task);
    DigitalCardCheck_DeleteCard(check);
    GFL_WordSetSystemFree(check->wordSet);
    GFL_MsgDataFree(check->msgData);
    GFL_HeapFree(check);
}

void DigitalCardCheck_Update(DigitalCardCheck *check) {
    WifiBattleMatchTask_Run(check->task);
    if (check->card != NULL) {
        WifiBattleMatchCard_Flush(check->card, check->param.printQueue);
    }
}

BOOL DigitalCardCheck_IsFinished(DigitalCardCheck *check) {
    return WifiBattleMatchTask_IsEnded(check->task);
}

static void DigitalCardCheck_TaskStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    void *entry =
        TournamentBlock_GetEntry(getTournamentBlockAddress(GameData_GetSaveControl(check->param.gameData)), check->param.mode);
    u32 field2 = TournamentEntry_GetField(entry, 2);
    u32 cardType = TournamentEntry_GetField(entry, 10);

    if (field2 != 0 && cardType == CARD_TYPE_1) {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskCardType1);
    } else if (field2 != 0 && cardType == CARD_TYPE_2) {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskCardType2);
    } else if (field2 != 0 && cardType == CARD_TYPE_3) {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskCardType3);
    } else if (field2 != 0 && cardType == CARD_TYPE_4) {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskCardType4);
    } else if (field2 != 0 && cardType == CARD_TYPE_5) {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskCardType5);
    } else {
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskNoCard);
    }
}

static void DigitalCardCheck_TaskCardType1(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    SaveControl *save;
    BattleBoxSave *box;
    void *entry;
    s32 choice;
    int result;

    switch (*seq) {
    case 0:
        DigitalCardCheck_CreateCard(check);
        DigitalCardCheck_ShowCard(check, 1);
        *seq = 1;
        break;
    case 1:
        if (DigitalCardCheck_IsCardReady(check)) {
            *seq = 2;
        }
        break;
    case 2:
        if (GCTX_HIDGetPressedKeys() & 3) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = 3;
        }
        break;
    case 3:
        DigitalCardCheck_ClearMessage(check);
        DigitalCardCheck_SetBGEnabled(check, 1);
        DigitalCardCheck_Print(check, EventFishing_Text_ParticipatingCompetitionPleaseRegister, 1);
        *seq = 19;
        WifiBattleMatchTask_SetReturnSeq(task, 4);
        break;
    case 4:
        DigitalCardCheck_OpenMenu(check, MENU_UNREGISTER_BACK);
        *seq = 5;
        break;
    case 5:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == 0) {
            *seq = 6;
        } else if (choice == 1) {
            *seq = 17;
        }
        break;
    case 6:
        DigitalCardCheck_Print(check, EventFishing_Text_OnceUnregisteredWillNot_3, 1);
        *seq = 19;
        WifiBattleMatchTask_SetReturnSeq(task, 7);
        break;
    case 7:
        DigitalCardCheck_OpenMenu(check, MENU_YES_NO);
        *seq = 8;
        break;
    case 8:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == 0) {
            *seq = 9;
        } else if (choice == 1) {
            *seq = 17;
        }
        break;
    case 9:
        DigitalCardCheck_Print(check, EventFishing_Text_WillUnregisteredSureAbout_2, 1);
        *seq = 19;
        WifiBattleMatchTask_SetReturnSeq(task, 10);
        break;
    case 10:
        DigitalCardCheck_OpenMenu(check, MENU_YES_NO);
        *seq = 11;
        break;
    case 11:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == 0) {
            *seq = 12;
        } else if (choice == 1) {
            *seq = 17;
        }
        break;
    case 12:
        DigitalCardCheck_Print(check, EventFishing_Text_BeingUnregisteredDontTurn, 2);
        *seq = 19;
        WifiBattleMatchTask_SetReturnSeq(task, 13);
        break;
    case 13:
        save = GameData_GetSaveControl(check->param.gameData);
        entry = TournamentBlock_GetEntry(getTournamentBlockAddress(save), check->param.mode);
        box = getBattleBox(save);
        BattleBoxSave_ClearFlags(box, check->param.mode == 0 ? 1 : 2);
        TournamentEntry_SetField(entry, 10, 4);
        *seq = 14;
        break;
    case 14:
        func_0201782c(check->param.gameData);
        *seq = 15;
        break;
    case 15:
        result = func_02017850(check->param.gameData);
        if (result == 2) {
            *seq = 16;
        } else if (result == 3) {
            *seq = 16;
        }
        break;
    case 16:
        DigitalCardCheck_Print(check, EventFishing_Text_HaveBeenUnregisteredFrom, 1);
        *seq = 19;
        WifiBattleMatchTask_SetReturnSeq(task, 17);
        break;
    case 17:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = 18;
        }
        break;
    case 18:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    case 19:
        if (DigitalCardCheck_IsPrinted(check)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void DigitalCardCheck_TaskCardType2(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    SaveControl *save;
    BattleBoxSave *box;
    void *entry;
    s32 choice;
    int result;

    switch (*seq) {
    case 0:
        DigitalCardCheck_CreateCard(check);
        DigitalCardCheck_ShowCard(check, 0);
        *seq = 1;
        break;
    case 1:
        if (DigitalCardCheck_IsCardReady(check)) {
            *seq = 2;
        }
        break;
    case 2:
        if (GCTX_HIDGetPressedKeys() & 3) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = 3;
        }
        break;
    case 3:
        DigitalCardCheck_ClearMessage(check);
        DigitalCardCheck_SetBGEnabled(check, 1);
        DigitalCardCheck_Print(check, EventFishing_Text_ParticipatingCompetitionBattleBox, 1);
        *seq = 19;
        WifiBattleMatchTask_SetReturnSeq(task, 4);
        break;
    case 4:
        DigitalCardCheck_OpenMenu(check, MENU_UNREGISTER_BACK);
        *seq = 5;
        break;
    case 5:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == 0) {
            *seq = 6;
        } else if (choice == 1) {
            *seq = 17;
        }
        break;
    case 6:
        DigitalCardCheck_Print(check, EventFishing_Text_OnceUnregisteredWillNot_2, 1);
        *seq = 19;
        WifiBattleMatchTask_SetReturnSeq(task, 7);
        break;
    case 7:
        DigitalCardCheck_OpenMenu(check, MENU_YES_NO);
        *seq = 8;
        break;
    case 8:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == 0) {
            *seq = 9;
        } else if (choice == 1) {
            *seq = 17;
        }
        break;
    case 9:
        DigitalCardCheck_Print(check, EventFishing_Text_WillUnregisteredSureAbout_2, 1);
        *seq = 19;
        WifiBattleMatchTask_SetReturnSeq(task, 10);
        break;
    case 10:
        DigitalCardCheck_OpenMenu(check, MENU_YES_NO);
        *seq = 11;
        break;
    case 11:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == 0) {
            *seq = 12;
        } else if (choice == 1) {
            *seq = 17;
        }
        break;
    case 12:
        DigitalCardCheck_Print(check, EventFishing_Text_BeingUnregisteredDontTurn, 2);
        *seq = 19;
        WifiBattleMatchTask_SetReturnSeq(task, 13);
        break;
    case 13:
        save = GameData_GetSaveControl(check->param.gameData);
        entry = TournamentBlock_GetEntry(getTournamentBlockAddress(save), check->param.mode);
        box = getBattleBox(save);
        BattleBoxSave_ClearFlags(box, check->param.mode == 0 ? 1 : 2);
        TournamentEntry_SetField(entry, 10, 4);
        *seq = 14;
        break;
    case 14:
        func_0201782c(check->param.gameData);
        *seq = 15;
        break;
    case 15:
        result = func_02017850(check->param.gameData);
        if (result == 2) {
            *seq = 16;
        } else if (result == 3) {
            *seq = 16;
        }
        break;
    case 16:
        DigitalCardCheck_Print(check, EventFishing_Text_BattleBoxHasBeen_2, 1);
        *seq = 19;
        WifiBattleMatchTask_SetReturnSeq(task, 17);
        break;
    case 17:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = 18;
        }
        break;
    case 18:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    case 19:
        if (DigitalCardCheck_IsPrinted(check)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void DigitalCardCheck_TaskCardType3(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    s32 choice;

    switch (*seq) {
    case 0:
        DigitalCardCheck_CreateCard(check);
        DigitalCardCheck_ShowCard(check, 1);
        *seq = 1;
        break;
    case 1:
        if (DigitalCardCheck_IsCardReady(check)) {
            *seq = 2;
        }
        break;
    case 2:
        if (GCTX_HIDGetPressedKeys() & 3) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = 3;
        }
        break;
    case 3:
        DigitalCardCheck_ClearMessage(check);
        DigitalCardCheck_SetBGEnabled(check, 1);
        DigitalCardCheck_Print(check, EventFishing_Text_CompetitionHasAlreadyEnded, 1);
        *seq = 4;
        break;
    case 4:
        if (DigitalCardCheck_IsPrinted(check)) {
            *seq = 5;
        }
        break;
    case 5:
        DigitalCardCheck_OpenMenu(check, MENU_BACK);
        *seq = 6;
        break;
    case 6:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == 0) {
            *seq = 7;
        }
        break;
    case 7:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = 8;
        }
        break;
    case 8:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskCardType4(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    s32 choice;

    switch (*seq) {
    case 0:
        DigitalCardCheck_CreateCard(check);
        DigitalCardCheck_ShowCard(check, 1);
        *seq = 1;
        break;
    case 1:
        if (DigitalCardCheck_IsCardReady(check)) {
            *seq = 2;
        }
        break;
    case 2:
        if (GCTX_HIDGetPressedKeys() & 3) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = 3;
        }
        break;
    case 3:
        DigitalCardCheck_ClearMessage(check);
        DigitalCardCheck_SetBGEnabled(check, 1);
        DigitalCardCheck_Print(check, EventFishing_Text_UnregisteredFromCompetition, 1);
        *seq = 4;
        break;
    case 4:
        if (DigitalCardCheck_IsPrinted(check)) {
            *seq = 5;
        }
        break;
    case 5:
        DigitalCardCheck_OpenMenu(check, MENU_BACK);
        *seq = 6;
        break;
    case 6:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == 0) {
            *seq = 7;
        }
        break;
    case 7:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = 8;
        }
        break;
    case 8:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskCardType5(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    s32 choice;

    switch (*seq) {
    case 0:
        DigitalCardCheck_CreateCard(check);
        DigitalCardCheck_ShowCard(check, 1);
        *seq = 1;
        break;
    case 1:
        if (DigitalCardCheck_IsCardReady(check)) {
            *seq = 2;
        }
        break;
    case 2:
        if (GCTX_HIDGetPressedKeys() & 3) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = 3;
        }
        break;
    case 3:
        DigitalCardCheck_ClearMessage(check);
        DigitalCardCheck_SetBGEnabled(check, 1);
        DigitalCardCheck_Print(check, EventFishing_Text_YouveBeenUnregisteredFrom, 1);
        *seq = 4;
        break;
    case 4:
        if (DigitalCardCheck_IsPrinted(check)) {
            *seq = 5;
        }
        break;
    case 5:
        DigitalCardCheck_OpenMenu(check, MENU_BACK);
        *seq = 6;
        break;
    case 6:
        choice = DigitalCardCheck_GetMenuChoice(check);
        if (choice == -1) {
            break;
        }
        DigitalCardCheck_CloseMenu(check);
        if (choice == 0) {
            *seq = 7;
        }
        break;
    case 7:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = 8;
        }
        break;
    case 8:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskNoCard(WifiBattleMatchTask *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;

    switch (*seq) {
    case 0:
        DigitalCardCheck_Print(check, EventFishing_Text_DontHaveAnyDigital, 1);
        *seq = 1;
        break;
    case 1:
        if (DigitalCardCheck_IsPrinted(check)) {
            *seq = 2;
        }
        break;
    case 2:
        WifiBattleMatchTask_SetFunc(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchTask_End(task);
}

static void DigitalCardCheck_CreateCard(DigitalCardCheck *check) {
    SaveControl *save;
    void *tournament;
    void *entry;
    void *cardData;
    Regulation *regulation;
    RecordSave *record;
    PlayerInfo *playerInfo;
    ClActUnit *link;

    if (check->card != NULL) {
        return;
    }
    save = GameData_GetSaveControl(check->param.gameData);
    tournament = getTournamentBlockAddress(GameData_GetSaveControl(check->param.gameData));
    entry = TournamentBlock_GetEntry(tournament, check->param.mode);
    cardData = TournamentBlock_GetTeam(tournament, check->param.mode);
    regulation = func_0200b530(entry);
    record = getRecordBlkAddress(save);
    playerInfo = GetGameDataPlayerInfo(check->param.gameData);
    link = WifiBattleMatchGraphic_GetClActUnit(check->param.net);
    if (check->param.mode == DIGITAL_CARD_CHECK_WIFI) {
        WifiBattleMatchWifiCardInfo info;
        void *results;
        u32 first;

        sys_memset(&info, 0, sizeof(WifiBattleMatchWifiCardInfo));
        sys_memcpy(func_0200b794(entry), &info, 0x92);
        results = func_0200f2d0(record);
        info.unkA0 = RndMatchData_GetParam(results, 10, 0);
        first = RndMatchData_GetParam(results, 10, 1);
        info.unkA4 = first + RndMatchData_GetParam(results, 10, 2);
        info.look = func_02008bf4(playerInfo);
        info.startDate =
            DigitalCardCheck_PackBytes(TournamentEntry_GetField(entry, 4), TournamentEntry_GetField(entry, 5), TournamentEntry_GetField(entry, 6));
        info.endDate =
            DigitalCardCheck_PackBytes(TournamentEntry_GetField(entry, 7), TournamentEntry_GetField(entry, 8), TournamentEntry_GetField(entry, 9));
        info.variant = TournamentEntry_GetField(entry, 11);
        check->card =
            WifiBattleMatchWifiCard_Create(&info, 0, playerInfo, link, check->param.seq, check->param.font,
                                check->param.printQueue, check->msgData, check->wordSet, cardData, 1, check->heapId);
    } else {
        WifiBattleMatchLiveCardInfo info;
        void *results;

        sys_memset(&info, 0, sizeof(WifiBattleMatchLiveCardInfo));
        sys_memcpy(func_0200b794(entry), &info, 0x92);
        results = func_0200f2d8(record);
        info.unkA0 = func_0200f1d8(results, 1);
        info.unkA2 = func_0200f1d8(results, 2);
        info.unkA4 = func_0200f1d8(results, 0);
        info.unkA6 = Regulation_GetParam(regulation, 0x16);
        info.look = func_02008bf4(playerInfo);
        info.startDate =
            DigitalCardCheck_PackBytes(TournamentEntry_GetField(entry, 4), TournamentEntry_GetField(entry, 5), TournamentEntry_GetField(entry, 6));
        info.endDate =
            DigitalCardCheck_PackBytes(TournamentEntry_GetField(entry, 7), TournamentEntry_GetField(entry, 8), TournamentEntry_GetField(entry, 9));
        info.variant = TournamentEntry_GetField(entry, 11);
        check->card =
            WifiBattleMatchLiveCard_Create(&info, playerInfo, link, check->param.seq, check->param.font, check->param.printQueue,
                                check->msgData, check->wordSet, cardData, 1, check->heapId);
    }
    DigitalCardCheck_SetBGEnabled(check, 0);
}

static void DigitalCardCheck_DeleteCard(DigitalCardCheck *check) {
    if (check->card != NULL) {
        if (check->param.mode == DIGITAL_CARD_CHECK_WIFI) {
            WifiBattleMatchWifiCard_Free(check->card);
        } else {
            WifiBattleMatchLiveCard_Free(check->card);
        }
        check->card = NULL;
    }
    GFL_BGSysClearScr(1);
    GFL_BGSysLoadScr(1);
}

static BOOL DigitalCardCheck_IsCardReady(DigitalCardCheck *check) {
    return WifiBattleMatchCard_SlideIn(check->card);
}

static BOOL DigitalCardCheck_IsCardDone(DigitalCardCheck *check) {
    return WifiBattleMatchCard_SlideOut(check->card);
}

static void DigitalCardCheck_ShowCard(DigitalCardCheck *check, u32 page) {
    if (check->param.mode == DIGITAL_CARD_CHECK_WIFI) {
        WifiBattleMatchWifiCard_ShowPage(check->card, page, check->msgData, check->param.printQueue, check->param.font,
                            check->heapId);
    } else {
        WifiBattleMatchLiveCard_ShowPage(check->card, page, check->msgData, check->param.printQueue, check->param.font,
                            check->heapId);
    }
}

static void DigitalCardCheck_OpenMenu(DigitalCardCheck *check, u32 kind) {
    WifiBattleMatchMenuParam param;
    u32 layout;
    int x, y, width, height;

    if (check->menu != NULL) {
        return;
    }
    sys_memset(&param, 0, sizeof(WifiBattleMatchMenuParam));
    param.msgData = check->msgData;
    param.font = check->param.font;
    param.printQueue = check->param.printQueue;
    param.bg = 0;
    param.palette = 15;
    param.framePalette = 13;
    param.frameChar = 1;
    switch (kind) {
    case MENU_YES_NO:
        param.msgIds[0] = EventFishing_Text_Yes_4;
        param.msgIds[1] = EventFishing_Text_No_4;
        param.count = 2;
        param.canCancel = 1;
        param.cancelValue = 1;
        layout = 0;
        param.cursorRow = 1;
        break;
    case MENU_BACK:
        param.msgIds[0] = EventFishing_Text_Back;
        param.count = 1;
        param.canCancel = 1;
        param.cancelValue = 0;
        layout = 0;
        break;
    case MENU_UNREGISTER_BACK:
        param.msgIds[0] = EventFishing_Text_Unregister;
        param.msgIds[1] = EventFishing_Text_Back;
        param.count = 2;
        param.canCancel = 1;
        param.cancelValue = 1;
        layout = 1;
        param.cursorRow = 1;
        break;
    }
    switch (layout) {
    case 0:
        width = 12;
        height = param.count * 2;
        x = 19;
        y = 17 - height;
        break;
    case 1:
        width = 15;
        height = param.count * 2;
        x = 16;
        y = 17 - height;
        break;
    }
    check->menu = WifiBattleMatchMenu_Create(&param, x, y, width, height, check->heapId);
}

static void DigitalCardCheck_CloseMenu(DigitalCardCheck *check) {
    if (check->menu != NULL) {
        WifiBattleMatchMenu_Free(check->menu);
        check->menu = NULL;
    }
}

static s32 DigitalCardCheck_GetMenuChoice(DigitalCardCheck *check) {
    if (check->menu != NULL) {
        return WifiBattleMatchMenu_Update(check->menu);
    }
    return -1;
}

static void DigitalCardCheck_Print(DigitalCardCheck *check, u32 msgId, u32 style) {
    WifiBattleMatchMsgBox_Print(check->param.msgBox, check->msgData, msgId, style);
}

static BOOL DigitalCardCheck_IsPrinted(DigitalCardCheck *check) {
    return WifiBattleMatchMsgBox_IsDone(check->param.msgBox);
}

static void DigitalCardCheck_SetBGEnabled(DigitalCardCheck *check, u32 enabled) {
    GFL_BGSysSetBGEnabled(0, enabled);
}

static void DigitalCardCheck_ClearMessage(DigitalCardCheck *check) {
    WifiBattleMatchMsgBox_Clear(check->param.msgBox);
}
