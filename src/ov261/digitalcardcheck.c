#include "app/digitalcardcheck.h"
#include "types.h"
#include "app/ov262_common.h"
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
    Ov262Card *card;
    Ov262Menu *menu;
    Ov262Task *task;
    DigitalCardCheckParam param;
    u16 heapId;
};

static void DigitalCardCheck_TaskStart(Ov262Task *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType1(Ov262Task *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType2(Ov262Task *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType3(Ov262Task *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType4(Ov262Task *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskCardType5(Ov262Task *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskNoCard(Ov262Task *task, u32 *seq, void *work);
static void DigitalCardCheck_TaskEnd(Ov262Task *task, u32 *seq, void *work);
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
    func_ov262_021c29c4(check->param.seq, 3, heapId);
    check->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING, heapId);
    check->wordSet = GFL_WordSetSystemCreate(8, 64, heapId);
    check->task = func_ov262_021c5c10(check, DigitalCardCheck_TaskStart, heapId);
    return check;
}

void DigitalCardCheck_Delete(DigitalCardCheck *check) {
    func_ov262_021c5c4c(check->task);
    DigitalCardCheck_DeleteCard(check);
    GFL_WordSetSystemFree(check->wordSet);
    GFL_MsgDataFree(check->msgData);
    GFL_HeapFree(check);
}

void DigitalCardCheck_Update(DigitalCardCheck *check) {
    func_ov262_021c5c54(check->task);
    if (check->card != NULL) {
        func_ov262_021c35e4(check->card, check->param.printQueue);
    }
}

BOOL DigitalCardCheck_IsFinished(DigitalCardCheck *check) {
    return func_ov262_021c5c64(check->task);
}

static void DigitalCardCheck_TaskStart(Ov262Task *task, u32 *seq, void *work) {
    DigitalCardCheck *check = work;
    void *entry =
        func_0200b8f4(getTournamentBlockAddress(GameData_GetSaveControl(check->param.gameData)), check->param.mode);
    u32 field2 = func_0200b798(entry, 2);
    u32 cardType = func_0200b798(entry, 10);

    if (field2 != 0 && cardType == CARD_TYPE_1) {
        func_ov262_021c5c74(task, DigitalCardCheck_TaskCardType1);
    } else if (field2 != 0 && cardType == CARD_TYPE_2) {
        func_ov262_021c5c74(task, DigitalCardCheck_TaskCardType2);
    } else if (field2 != 0 && cardType == CARD_TYPE_3) {
        func_ov262_021c5c74(task, DigitalCardCheck_TaskCardType3);
    } else if (field2 != 0 && cardType == CARD_TYPE_4) {
        func_ov262_021c5c74(task, DigitalCardCheck_TaskCardType4);
    } else if (field2 != 0 && cardType == CARD_TYPE_5) {
        func_ov262_021c5c74(task, DigitalCardCheck_TaskCardType5);
    } else {
        func_ov262_021c5c74(task, DigitalCardCheck_TaskNoCard);
    }
}

static void DigitalCardCheck_TaskCardType1(Ov262Task *task, u32 *seq, void *work) {
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
        func_ov262_021c5c88(task, 4);
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
        func_ov262_021c5c88(task, 7);
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
        func_ov262_021c5c88(task, 10);
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
        func_ov262_021c5c88(task, 13);
        break;
    case 13:
        save = GameData_GetSaveControl(check->param.gameData);
        entry = func_0200b8f4(getTournamentBlockAddress(save), check->param.mode);
        box = getBattleBox(save);
        func_0200c370(box, check->param.mode == 0 ? 1 : 2);
        func_0200b830(entry, 10, 4);
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
        func_ov262_021c5c88(task, 17);
        break;
    case 17:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = 18;
        }
        break;
    case 18:
        func_ov262_021c5c74(task, DigitalCardCheck_TaskEnd);
        break;
    case 19:
        if (DigitalCardCheck_IsPrinted(check)) {
            func_ov262_021c5c8c(task);
        }
        break;
    }
}

static void DigitalCardCheck_TaskCardType2(Ov262Task *task, u32 *seq, void *work) {
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
        func_ov262_021c5c88(task, 4);
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
        func_ov262_021c5c88(task, 7);
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
        func_ov262_021c5c88(task, 10);
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
        func_ov262_021c5c88(task, 13);
        break;
    case 13:
        save = GameData_GetSaveControl(check->param.gameData);
        entry = func_0200b8f4(getTournamentBlockAddress(save), check->param.mode);
        box = getBattleBox(save);
        func_0200c370(box, check->param.mode == 0 ? 1 : 2);
        func_0200b830(entry, 10, 4);
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
        func_ov262_021c5c88(task, 17);
        break;
    case 17:
        if (DigitalCardCheck_IsCardDone(check)) {
            *seq = 18;
        }
        break;
    case 18:
        func_ov262_021c5c74(task, DigitalCardCheck_TaskEnd);
        break;
    case 19:
        if (DigitalCardCheck_IsPrinted(check)) {
            func_ov262_021c5c8c(task);
        }
        break;
    }
}

static void DigitalCardCheck_TaskCardType3(Ov262Task *task, u32 *seq, void *work) {
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
        func_ov262_021c5c74(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskCardType4(Ov262Task *task, u32 *seq, void *work) {
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
        func_ov262_021c5c74(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskCardType5(Ov262Task *task, u32 *seq, void *work) {
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
        func_ov262_021c5c74(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskNoCard(Ov262Task *task, u32 *seq, void *work) {
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
        func_ov262_021c5c74(task, DigitalCardCheck_TaskEnd);
        break;
    }
}

static void DigitalCardCheck_TaskEnd(Ov262Task *task, u32 *seq, void *work) {
    func_ov262_021c5c7c(task);
}

static void DigitalCardCheck_CreateCard(DigitalCardCheck *check) {
    SaveControl *save;
    void *tournament;
    void *entry;
    void *cardData;
    Regulation *regulation;
    RecordSave *record;
    PlayerInfo *playerInfo;
    Ov262NetLink *link;

    if (check->card != NULL) {
        return;
    }
    save = GameData_GetSaveControl(check->param.gameData);
    tournament = getTournamentBlockAddress(GameData_GetSaveControl(check->param.gameData));
    entry = func_0200b8f4(tournament, check->param.mode);
    cardData = func_0200b924(tournament, check->param.mode);
    regulation = func_0200b530(entry);
    record = getRecordBlkAddress(save);
    playerInfo = GetGameDataPlayerInfo(check->param.gameData);
    link = func_ov262_021c2668(check->param.net);
    if (check->param.mode == DIGITAL_CARD_CHECK_WIFI) {
        Ov262WifiCardInfo info;
        void *results;
        u32 first;

        sys_memset(&info, 0, sizeof(Ov262WifiCardInfo));
        sys_memcpy(func_0200b794(entry), &info, 0x92);
        results = func_0200f2d0(record);
        info.unkA0 = func_0200e870(results, 10, 0);
        first = func_0200e870(results, 10, 1);
        info.unkA4 = first + func_0200e870(results, 10, 2);
        info.unk9C = func_02008bf4(playerInfo);
        info.unk94 =
            DigitalCardCheck_PackBytes(func_0200b798(entry, 4), func_0200b798(entry, 5), func_0200b798(entry, 6));
        info.unk98 =
            DigitalCardCheck_PackBytes(func_0200b798(entry, 7), func_0200b798(entry, 8), func_0200b798(entry, 9));
        info.unkAA = func_0200b798(entry, 11);
        check->card =
            func_ov262_021c2e8c(&info, 0, playerInfo, link, check->param.seq, check->param.font,
                                check->param.printQueue, check->msgData, check->wordSet, cardData, 1, check->heapId);
    } else {
        Ov262LiveCardInfo info;
        void *results;

        sys_memset(&info, 0, sizeof(Ov262LiveCardInfo));
        sys_memcpy(func_0200b794(entry), &info, 0x92);
        results = func_0200f2d8(record);
        info.unkA0 = func_0200f1d8(results, 1);
        info.unkA2 = func_0200f1d8(results, 2);
        info.unkA4 = func_0200f1d8(results, 0);
        info.unkA6 = func_0200b574(regulation, 0x16);
        info.unk9C = func_02008bf4(playerInfo);
        info.unk94 =
            DigitalCardCheck_PackBytes(func_0200b798(entry, 4), func_0200b798(entry, 5), func_0200b798(entry, 6));
        info.unk98 =
            DigitalCardCheck_PackBytes(func_0200b798(entry, 7), func_0200b798(entry, 8), func_0200b798(entry, 9));
        info.unkA8 = func_0200b798(entry, 11);
        check->card =
            func_ov262_021c323c(&info, playerInfo, link, check->param.seq, check->param.font, check->param.printQueue,
                                check->msgData, check->wordSet, cardData, 1, check->heapId);
    }
    DigitalCardCheck_SetBGEnabled(check, 0);
}

static void DigitalCardCheck_DeleteCard(DigitalCardCheck *check) {
    if (check->card != NULL) {
        if (check->param.mode == DIGITAL_CARD_CHECK_WIFI) {
            func_ov262_021c301c(check->card);
        } else {
            func_ov262_021c33c4(check->card);
        }
        check->card = NULL;
    }
    GFL_BGSysClearScr(1);
    GFL_BGSysLoadScr(1);
}

static BOOL DigitalCardCheck_IsCardReady(DigitalCardCheck *check) {
    return func_ov262_021c35ec(check->card);
}

static BOOL DigitalCardCheck_IsCardDone(DigitalCardCheck *check) {
    return func_ov262_021c3720(check->card);
}

static void DigitalCardCheck_ShowCard(DigitalCardCheck *check, u32 page) {
    if (check->param.mode == DIGITAL_CARD_CHECK_WIFI) {
        func_ov262_021c3048(check->card, page, check->msgData, check->param.printQueue, check->param.font,
                            check->heapId);
    } else {
        func_ov262_021c33f0(check->card, page, check->msgData, check->param.printQueue, check->param.font,
                            check->heapId);
    }
}

static void DigitalCardCheck_OpenMenu(DigitalCardCheck *check, u32 kind) {
    Ov262MenuParam param;
    u32 layout;
    int x, y, width, height;

    if (check->menu != NULL) {
        return;
    }
    sys_memset(&param, 0, sizeof(Ov262MenuParam));
    param.msgData = check->msgData;
    param.font = check->param.font;
    param.printQueue = check->param.printQueue;
    param.unk20 = 0;
    param.unk22 = 15;
    param.unk24 = 13;
    param.unk26 = 1;
    switch (kind) {
    case MENU_YES_NO:
        param.msgIds[0] = EventFishing_Text_Yes_4;
        param.msgIds[1] = EventFishing_Text_No_4;
        param.count = 2;
        param.unk28 = 1;
        param.unk2C = 1;
        layout = 0;
        param.unk2E = 1;
        break;
    case MENU_BACK:
        param.msgIds[0] = EventFishing_Text_Back;
        param.count = 1;
        param.unk28 = 1;
        param.unk2C = 0;
        layout = 0;
        break;
    case MENU_UNREGISTER_BACK:
        param.msgIds[0] = EventFishing_Text_Unregister;
        param.msgIds[1] = EventFishing_Text_Back;
        param.count = 2;
        param.unk28 = 1;
        param.unk2C = 1;
        layout = 1;
        param.unk2E = 1;
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
    check->menu = func_ov262_021c5a88(&param, x, y, width, height, check->heapId);
}

static void DigitalCardCheck_CloseMenu(DigitalCardCheck *check) {
    if (check->menu != NULL) {
        func_ov262_021c5ba4(check->menu);
        check->menu = NULL;
    }
}

static s32 DigitalCardCheck_GetMenuChoice(DigitalCardCheck *check) {
    if (check->menu != NULL) {
        return func_ov262_021c5bd4(check->menu);
    }
    return -1;
}

static void DigitalCardCheck_Print(DigitalCardCheck *check, u32 msgId, u32 style) {
    func_ov262_021c5888(check->param.msgBox, check->msgData, msgId, style);
}

static BOOL DigitalCardCheck_IsPrinted(DigitalCardCheck *check) {
    return func_ov262_021c5a1c(check->param.msgBox);
}

static void DigitalCardCheck_SetBGEnabled(DigitalCardCheck *check, u32 enabled) {
    GFL_BGSysSetBGEnabled(0, enabled);
}

static void DigitalCardCheck_ClearMessage(DigitalCardCheck *check) {
    func_ov262_021c5a48(check->param.msgBox);
}
