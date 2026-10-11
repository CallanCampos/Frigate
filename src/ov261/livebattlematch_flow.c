#include "types.h"
#include "app/livebattlematch_flow.h"
#include "app/livebattlematch_irc.h"
#include "battle/regulation.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/tournament_block.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/pms.h"
#include "system/wordset.h"

// A heap that the team is read into for a moment
#define HEAPID_LIVEBATTLEMATCH_TEAM HEAPID_TAIL(0x56)

// The steps of a flow that wait for a message to be printed
#define STEP_MESSAGE_QUEUE_NUM 4

struct LiveBattleMatchFlow {
    Ov262Task *task;
    Ov262Menu *menu;
    u32 menuKind;
    LiveBattleMatchIrc *irc;
    Ov262Card *playerCard;
    Ov262PartnerCard *partnerCard;
    Ov262TeamList *teamList;
    Ov262LinkIcon *linkIcon;
    MsgData *msgMain;
    MsgData *msgCard;
    Font *font;
    WordSet *wordSet;
    PokeParty *party;
    void *record;
    void *entry;
    // The competition data that the infrared delivery brings
    u8 delivered[0x160];
    u32 result;
    LiveBattleMatchFlowParam param;
    HeapID heapId;
    u32 frames;
    BOOL hasVideo;
    BOOL readySent;
    BOOL readyReceived;
    u32 pickFlags;
    u32 pickIndex;
};

static void LiveBattleMatchFlow_StepInit(Ov262Task *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepEnd(Ov262Task *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepEntry(Ov262Task *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepTeam(Ov262Task *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepMenu(Ov262Task *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepLink(Ov262Task *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepRecord(Ov262Task *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepQuit(Ov262Task *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_MenuStart(LiveBattleMatchFlow *wk, u32 kind);
static void LiveBattleMatchFlow_MenuFree(LiveBattleMatchFlow *wk);
static s32 LiveBattleMatchFlow_MenuGetSelection(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_PrintMessage(LiveBattleMatchFlow *wk, u32 msgId, u32 type);
static BOOL LiveBattleMatchFlow_IsMessageDone(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_Exit(LiveBattleMatchFlow *wk, u32 result);
static void LiveBattleMatchFlow_PlayerCardCreate(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_PlayerCardCreateWait(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_PlayerCardDelete(LiveBattleMatchFlow *wk);
static BOOL LiveBattleMatchFlow_PlayerCardIsReady(LiveBattleMatchFlow *wk);
static BOOL LiveBattleMatchFlow_PlayerCardIsDone(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_PlayerCardShow(LiveBattleMatchFlow *wk, u32 a1);
static void LiveBattleMatchFlow_PartnerCardCreate(LiveBattleMatchFlow *wk, LiveBattleMatchPlayerData *data);
static void LiveBattleMatchFlow_PartnerCardDelete(LiveBattleMatchFlow *wk);
static BOOL LiveBattleMatchFlow_PartnerCardIsDone(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_TeamListCreate(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_TeamListDelete(LiveBattleMatchFlow *wk);
static BOOL LiveBattleMatchFlow_TeamListIsReady(LiveBattleMatchFlow *wk);
static BOOL LiveBattleMatchFlow_TeamListIsDone(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_LoadTeam(LiveBattleMatchFlow *wk, PokeParty *dest);
static void LiveBattleMatchFlow_FillPlayerData(LiveBattleMatchPlayerData *data, LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_SetEntryField2(LiveBattleMatchPlayerData *data, LiveBattleMatchFlow *wk);
static u32 LiveBattleMatchFlow_GetResultMessageId(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_PrintResult(LiveBattleMatchFlow *wk);

// Prints a message, and goes to the step that waits for it, after which the task goes to the step next
static inline void LiveBattleMatchFlow_PrintAndWait(LiveBattleMatchFlow *wk, Ov262Task *task, u32 *seq, u32 msgId,
                                                    u32 type, u32 waitSeq, u32 next) {
    LiveBattleMatchFlow_PrintMessage(wk, msgId, type);
    *seq = waitSeq;
    func_ov262_021c5c88(task, next);
}

LiveBattleMatchFlow *LiveBattleMatchFlow_Create(const LiveBattleMatchFlowParam *param, HeapID heapId) {
    LiveBattleMatchFlow *wk = GFL_HeapAllocate(heapId, sizeof(LiveBattleMatchFlow), FALSE, "livebattlematch_flow.c", 270);

    sys_memset(wk, 0, sizeof(LiveBattleMatchFlow));
    wk->param = *param;
    wk->heapId = heapId;
    wk->hasVideo = func_0200bb80();
    func_ov262_021c29c4(wk->param.seq, TRUE, heapId);
    wk->wordSet = GFL_WordSetSystemCreate(8, 0x40, heapId);
    wk->msgMain = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_PHRASE_INPUT, heapId);
    wk->msgCard = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING, heapId);
    wk->font = GFL_FontCreate(0x17, 3, 0, 0, heapId);
    wk->task = func_ov262_021c5c10(wk, LiveBattleMatchFlow_StepInit, heapId);
    wk->irc = LiveBattleMatchIrc_Create(wk->param.gameData, heapId);
    wk->linkIcon = func_ov262_021c4e44(func_ov262_021c2668(wk->param.net), wk->param.seq, heapId);
    func_ov262_021c4f30(wk->linkIcon, FALSE);
    wk->entry = func_0200b8f4(getTournamentBlockAddress(GameData_GetSaveControl(wk->param.gameData)), 1);
    wk->record = func_0200f2d8(getRecordBlkAddress(GameData_GetSaveControl(wk->param.gameData)));
    wk->party = PokeParty_Create(heapId);
    if (wk->param.mode == 0) {
        GCTX_HIDBlockSoftReset(8);
    }
    return wk;
}

void LiveBattleMatchFlow_Delete(LiveBattleMatchFlow *wk) {
    GFL_HeapFree(wk->party);
    if (wk->result == 1) {
        GCTX_HIDUnblockSoftReset(8);
    }
    LiveBattleMatchFlow_PlayerCardDelete(wk);
    LiveBattleMatchFlow_PartnerCardDelete(wk);
    func_ov262_021c4f14(wk->linkIcon);
    LiveBattleMatchIrc_Delete(wk->irc);
    func_ov262_021c5c4c(wk->task);
    GFL_FontFree(wk->font);
    GFL_MsgDataFree(wk->msgCard);
    GFL_MsgDataFree(wk->msgMain);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_HeapFree(wk);
}

void LiveBattleMatchFlow_Main(LiveBattleMatchFlow *wk) {
    func_ov262_021c5c54(wk->task);
    LiveBattleMatchIrc_Main(wk->irc);
    func_ov262_021c4f2c(wk->linkIcon);
    if (wk->playerCard != NULL) {
        func_ov262_021c35e4(wk->playerCard, wk->param.printQueue);
    }
    if (wk->partnerCard != NULL) {
        func_ov262_021c4898(wk->partnerCard, wk->param.printQueue);
    }
    if (wk->teamList != NULL) {
        func_ov262_021c561c(wk->teamList, wk->param.printQueue);
    }
}

u32 LiveBattleMatchFlow_GetResult(LiveBattleMatchFlow *wk) {
    if (func_ov262_021c5c64(wk->task)) {
        return wk->result;
    }
    return 0;
}

// Picks the step to start with
static void LiveBattleMatchFlow_StepInit(Ov262Task *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;

    switch (wk->param.mode) {
    case 0:
        func_ov262_021c5c74(task, LiveBattleMatchFlow_StepEntry);
        break;
    case 1:
        if (func_0200b798(wk->entry, 10) == 3) {
            func_ov262_021c5c74(task, LiveBattleMatchFlow_StepQuit);
        } else {
            func_ov262_021c5c74(task, LiveBattleMatchFlow_StepMenu);
        }
        break;
    case 2:
        func_ov262_021c5c74(task, LiveBattleMatchFlow_StepRecord);
        break;
    case 3:
        func_ov262_021c5c74(task, LiveBattleMatchFlow_StepQuit);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
}

static void LiveBattleMatchFlow_StepEnd(Ov262Task *task, u32 *seq, void *work) {
    func_ov262_021c5c7c(task);
}

// Starts the flow of a player who has no entry yet, or one who has one that is half done
static void LiveBattleMatchFlow_StepEntry(Ov262Task *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;
    s32 selection;
    u32 status;
    u32 pressed;

    switch (*seq) {
    case 0:
        switch (func_0200b798(wk->entry, 10)) {
        case 0:
        case 3:
        case 4:
        case 5:
            if (func_0200b798(wk->entry, 2) == 0) {
                *seq = 12;
            } else {
                *seq = 1;
            }
            break;
        case 1:
            func_ov262_021c5c74(task, LiveBattleMatchFlow_StepTeam);
            break;
        case 2:
            func_ov262_021c5c74(task, LiveBattleMatchFlow_StepMenu);
            break;
        }
        break;
    case 1:
        LiveBattleMatchFlow_PlayerCardCreate(wk);
        LiveBattleMatchFlow_PlayerCardShow(wk, 1);
        *seq = 2;
        break;
    case 2:
        if (LiveBattleMatchFlow_PlayerCardIsReady(wk)) {
            *seq = 3;
        }
        break;
    case 3:
        if (func_0200c394(getBattleBox(GameData_GetSaveControl(wk->param.gameData)), 2)) {
            *seq = 5;
        } else {
            *seq = 4;
        }
        break;
    case 4:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 0, TRUE, 31, 6);
        break;
    case 5:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 1, TRUE, 31, 6);
        break;
    case 6:
        LiveBattleMatchFlow_MenuStart(wk, 0);
        *seq = 7;
        break;
    case 7:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case 0:
            *seq = 8;
            break;
        case 1:
            *seq = 10;
            break;
        }
        break;
    case 8:
        if (LiveBattleMatchFlow_PlayerCardIsDone(wk)) {
            *seq = 9;
        }
        break;
    case 9:
        LiveBattleMatchFlow_PlayerCardDelete(wk);
        *seq = 12;
        break;
    case 10:
        if (LiveBattleMatchFlow_PlayerCardIsDone(wk)) {
            *seq = 11;
        }
        break;
    case 11:
        LiveBattleMatchFlow_PlayerCardDelete(wk);
        LiveBattleMatchFlow_Exit(wk, 1);
        break;
    case 12:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 2, TRUE, 31, 13);
        break;
    case 13:
        func_ov262_021c4f30(wk->linkIcon, TRUE);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 32, FALSE, 31, 14);
        break;
    case 14:
        LiveBattleMatchIrc_StartDelivery(wk->irc, wk->delivered);
        *seq = 15;
        break;
    case 15:
        status = LiveBattleMatchIrc_StepDelivery(wk->irc);
        if (status != 0) {
            func_ov262_021c4f30(wk->linkIcon, FALSE);
            if (status == 1) {
                *seq = 21;
            } else {
                *seq = 26;
            }
        }
        if (LiveBattleMatchIrc_CheckError(wk->irc) == TRUE) {
            func_ov262_021c4f30(wk->linkIcon, FALSE);
            *seq = 12;
            break;
        }
        pressed = GCTX_HIDGetPressedKeys();
        if (pressed & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = 16;
        }
        break;
    case 16:
        LiveBattleMatchIrc_ResetDeliveryEnd(wk->irc);
        *seq = 17;
        break;
    case 17:
        if (LiveBattleMatchIrc_StepDeliveryEnd(wk->irc)) {
            *seq = 18;
        }
        break;
    case 18:
        func_ov262_021c4f30(wk->linkIcon, FALSE);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 21, TRUE, 31, 19);
        break;
    case 19:
        LiveBattleMatchFlow_MenuStart(wk, 0);
        *seq = 20;
        break;
    case 20:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case 0:
            *seq = 30;
            break;
        case 1:
            *seq = 12;
            break;
        }
        break;
    case 21:
        if (func_0200b8c8(wk->delivered)) {
            if (func_0200b798(wk->delivered, 1) & LIVEBATTLEMATCH_VERSION_MASK) {
                if (func_0200b798(wk->entry, 2) == func_0200b798(wk->delivered, 2)) {
                    switch (func_0200b798(wk->entry, 10)) {
                    case 4:
                        *seq = 27;
                        break;
                    case 3:
                        *seq = 28;
                        break;
                    default:
                        *seq = 22;
                        break;
                    }
                } else {
                    *seq = 22;
                }
            } else {
                *seq = 26;
            }
        } else {
            *seq = 25;
        }
        break;
    case 22:
        clearVSPlayer();
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 10, 2, 31, 23);
        break;
    case 23: {
        void *block = getTournamentBlockAddress(GameData_GetSaveControl(wk->param.gameData));
        void *records = func_0200b924(block, 1);

        func_0200b830(wk->delivered, 10, 1);
        func_0200f1c4(wk->record);
        func_0200b918(records);
        func_0200b900(block, 1, wk->delivered);
        func_0201782c(wk->param.gameData);
        *seq = 24;
        break;
    }
    case 24:
        if (func_02017850(wk->param.gameData) == 2) {
            *seq = 29;
        }
        break;
    case 25:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 26, TRUE, 31, 30);
        break;
    case 26:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 27, TRUE, 31, 30);
        break;
    case 27:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 3, TRUE, 31, 30);
        break;
    case 28:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 4, TRUE, 31, 30);
        break;
    case 29:
        func_ov262_021c5c74(task, LiveBattleMatchFlow_StepTeam);
        break;
    case 30:
        LiveBattleMatchFlow_Exit(wk, 1);
        break;
    case 31:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            func_ov262_021c5c8c(task);
        }
        break;
    }
}

// Checks the team against the regulation
static void LiveBattleMatchFlow_StepTeam(Ov262Task *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;
    s32 selection;

    switch (*seq) {
    case 0:
        LiveBattleMatchFlow_PlayerCardCreate(wk);
        LiveBattleMatchFlow_PlayerCardShow(wk, 2);
        LiveBattleMatchFlow_TeamListCreate(wk);
        *seq = 1;
        break;
    case 1: {
        BOOL ready = TRUE;

        ready &= LiveBattleMatchFlow_PlayerCardIsReady(wk);
        if (LiveBattleMatchFlow_TeamListIsReady(wk) & ready) {
            *seq = 2;
        }
        break;
    }
    case 2:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 5, TRUE, 21, 3);
        break;
    case 3:
        LiveBattleMatchFlow_MenuStart(wk, 0);
        *seq = 4;
        break;
    case 4:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case 0:
            *seq = 8;
            break;
        case 1:
            *seq = 5;
            break;
        }
        break;
    case 5:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 8, TRUE, 21, 6);
        break;
    case 6:
        LiveBattleMatchFlow_MenuStart(wk, 0);
        *seq = 7;
        break;
    case 7:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case 0:
            *seq = 19;
            break;
        case 1:
            *seq = 2;
            break;
        }
        break;
    case 8:
        if (LiveBattleMatchFlow_TeamListIsDone(wk)) {
            LiveBattleMatchFlow_TeamListDelete(wk);
            *seq = 9;
        }
        break;
    case 9: {
        Regulation *regulation = func_0200b530(wk->entry);
        u32 problem;

        LiveBattleMatchFlow_LoadTeam(wk, wk->party);
        if (howManyPokesAreAbleToFight(wk->party) == 0) {
            *seq = 11;
            break;
        }
        problem = func_0201f424(regulation, wk->party, &wk->pickFlags);
        if (func_0201f6e8(regulation, wk->party)) {
            wk->pickFlags |= 0x20;
            problem = 5;
        }
        if (problem == 1) {
            wk->pickFlags |= 0x100;
        }
        wk->pickIndex = 0;
        if (problem == 0) {
            *seq = 13;
            break;
        }
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 41, TRUE, 21, 10);
        break;
    }
    case 10: {
        int index = wk->pickIndex;
        int mask = 1 << index;

        while (mask <= 0x100) {
            if (wk->pickFlags & mask) {
                break;
            }
            index = ++wk->pickIndex;
            mask = 1 << index;
        }
        if (mask <= 0x100) {
            LiveBattleMatchFlow_PrintAndWait(wk, task, seq, index + 42, TRUE, 21, 22);
        } else {
            *seq = 22;
        }
        break;
    }
    case 22: {
        int index = ++wk->pickIndex;
        int mask = 1 << index;

        while (mask <= 0x100) {
            if (wk->pickFlags & mask) {
                break;
            }
            index = ++wk->pickIndex;
            mask = 1 << index;
        }
        if (mask <= 0x100) {
            *seq = 10;
        } else {
            *seq = 23;
        }
        break;
    }
    case 23:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 51, TRUE, 21, 12);
        break;
    case 11:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 37, TRUE, 21, 12);
        break;
    case 12:
        if (LiveBattleMatchFlow_PlayerCardIsDone(wk)) {
            LiveBattleMatchFlow_PlayerCardDelete(wk);
            *seq = 20;
        }
        break;
    case 13: {
        SaveControl *save = GameData_GetSaveControl(wk->param.gameData);
        BattleBoxSave *battleBox = getBattleBox(save);
        void *records = func_0200b924(getTournamentBlockAddress(save), 1);
        PokeParty *party = convertBoxedPokeSetToParty(battleBox, HEAPID_LIVEBATTLEMATCH_TEAM);

        func_0200c360(battleBox, 2);
        func_0200b830(wk->entry, 10, 2);
        func_0200b9c4(records, party);
        GFL_HeapFree(party);
        *seq = 14;
        break;
    }
    case 14:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 10, 2, 21, 15);
        break;
    case 15:
        func_0201782c(wk->param.gameData);
        *seq = 16;
        break;
    case 16:
        if (func_02017850(wk->param.gameData) == 2) {
            *seq = 17;
        }
        break;
    case 17:
        LiveBattleMatchFlow_PlayerCardShow(wk, 0);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 11, TRUE, 21, 18);
        break;
    case 18:
        func_ov262_021c5c74(task, LiveBattleMatchFlow_StepMenu);
        break;
    case 19: {
        BOOL done = TRUE;

        done &= LiveBattleMatchFlow_PlayerCardIsDone(wk);
        if (LiveBattleMatchFlow_TeamListIsDone(wk) & done) {
            LiveBattleMatchFlow_PlayerCardDelete(wk);
            LiveBattleMatchFlow_TeamListDelete(wk);
            *seq = 20;
        }
        break;
    }
    case 20:
        LiveBattleMatchFlow_Exit(wk, 1);
        break;
    case 21:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            func_ov262_021c5c8c(task);
        }
        break;
    }
}

// The menu of a player who has an entry: to take part in a match, to see the card, to quit
static void LiveBattleMatchFlow_StepMenu(Ov262Task *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;
    s32 selection;

    switch (*seq) {
    case 0:
        LiveBattleMatchFlow_FillPlayerData(wk->param.playerData, wk);
        if (wk->playerCard != NULL) {
            *seq = 4;
            break;
        }
        LiveBattleMatchFlow_PlayerCardCreate(wk);
        LiveBattleMatchFlow_PlayerCardShow(wk, 0);
        *seq = 1;
        break;
    case 1:
        if (LiveBattleMatchFlow_PlayerCardIsReady(wk)) {
            *seq = 2;
        }
        break;
    case 2:
        if (wk->param.matchResult->disconnected) {
            *seq = 3;
        } else {
            *seq = 4;
        }
        break;
    case 3:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 39, TRUE, 25, 4);
        break;
    case 4: {
        Regulation *regulation = func_0200b530(wk->entry);
        u32 played = func_0200f1d8(wk->record, 0);

        if (played >= func_0200b574(regulation, 0x16)) {
            func_ov262_021c5c74(task, LiveBattleMatchFlow_StepQuit);
        } else {
            *seq = 5;
        }
        break;
    }
    case 5:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 12, TRUE, 25, 6);
        break;
    case 6:
        LiveBattleMatchFlow_MenuStart(wk, 1);
        *seq = 7;
        break;
    case 7:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        if (wk->hasVideo && !wk->param.matchResult->disconnected) {
            switch (selection) {
            case 0:
                *seq = 23;
                break;
            case 1:
                *seq = 19;
                break;
            case 2:
                *seq = 26;
                break;
            case 3:
                *seq = 8;
                break;
            }
        } else {
            switch (selection) {
            case 0:
                *seq = 23;
                break;
            case 1:
                *seq = 8;
                break;
            }
        }
        break;
    case 8:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 17, TRUE, 25, 9);
        break;
    case 9:
        LiveBattleMatchFlow_MenuStart(wk, 3);
        *seq = 10;
        break;
    case 10:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case 0:
            *seq = 11;
            break;
        case 1:
            *seq = 5;
            break;
        }
        break;
    case 11:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 18, TRUE, 25, 12);
        break;
    case 12:
        LiveBattleMatchFlow_MenuStart(wk, 3);
        *seq = 13;
        break;
    case 13:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case 0:
            *seq = 14;
            break;
        case 1:
            *seq = 5;
            break;
        }
        break;
    case 14:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 10, 2, 25, 15);
        break;
    case 15:
        func_0200b830(wk->entry, 10, 4);
        func_0200c370(getBattleBox(GameData_GetSaveControl(wk->param.gameData)), 2);
        func_0201782c(wk->param.gameData);
        *seq = 16;
        break;
    case 16:
        switch (func_02017850(wk->param.gameData)) {
        case 0:
        case 1:
            break;
        case 2:
        case 3:
            *seq = 17;
            break;
        }
        break;
    case 17:
        LiveBattleMatchFlow_PlayerCardShow(wk, 1);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 19, TRUE, 25, 18);
        break;
    case 18:
        if (LiveBattleMatchFlow_PlayerCardIsDone(wk)) {
            LiveBattleMatchFlow_PlayerCardDelete(wk);
            *seq = 24;
        }
        break;
    case 19:
        if (func_0200c1d0((u8)wk->param.partnerData->unk44)) {
            *seq = 20;
        } else {
            *seq = 21;
        }
        break;
    case 20:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 16, TRUE, 25, 22);
        break;
    case 21:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 34, TRUE, 25, 5);
        break;
    case 22:
        LiveBattleMatchFlow_Exit(wk, 4);
        break;
    case 23:
        func_ov262_021c5c74(task, LiveBattleMatchFlow_StepLink);
        break;
    case 24:
        LiveBattleMatchFlow_Exit(wk, 1);
        break;
    case 25:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            func_ov262_021c5c8c(task);
        }
        break;
    case 26:
        LiveBattleMatchFlow_PrintResult(wk);
        *seq = 25;
        func_ov262_021c5c88(task, 5);
        break;
    }
}

// Links with the other player over infrared, trades the players' data and teams and shows the players to each other
static void LiveBattleMatchFlow_StepLink(Ov262Task *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;
    s32 selection;
    int linked;
    void *received;

    if (LiveBattleMatchIrc_CheckError(wk->irc) == TRUE) {
        wk->param.matchResult->disconnected = TRUE;
        func_ov262_021c5c74(task, LiveBattleMatchFlow_StepMenu);
    }
    switch (*seq) {
    case 0:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 20, TRUE, 40, 1);
        break;
    case 1:
        LiveBattleMatchFlow_PrintMessage(wk, 33, FALSE);
        func_ov262_021c4f30(wk->linkIcon, TRUE);
        GFL_SndSEPlay(SEQ_SE_SYS_102);
        wk->readySent = FALSE;
        wk->readyReceived = FALSE;
        *seq = 40;
        func_ov262_021c5c88(task, 2);
        break;
    case 2:
        if (LiveBattleMatchIrc_ResetLink(wk->irc)) {
            *seq = 3;
        }
        break;
    case 3:
        linked = LiveBattleMatchIrc_StepLink(wk->irc);
        switch (linked) {
        case 1:
            *seq = 13;
            break;
        case 2:
            *seq = 9;
            break;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = 4;
        }
        break;
    case 4:
        LiveBattleMatchIrc_CancelLink(wk->irc);
        *seq = 5;
        break;
    case 5:
        if (LiveBattleMatchIrc_StepCancel(wk->irc)) {
            *seq = 6;
        }
        break;
    case 6:
        func_ov262_021c4f30(wk->linkIcon, FALSE);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 21, TRUE, 40, 7);
        break;
    case 7:
        LiveBattleMatchFlow_MenuStart(wk, 0);
        *seq = 8;
        break;
    case 8:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case 0:
            func_ov262_021c4f30(wk->linkIcon, FALSE);
            func_ov262_021c5c74(task, LiveBattleMatchFlow_StepMenu);
            break;
        case 1:
            *seq = 1;
            break;
        }
        break;
    case 9:
        func_ov262_021c4f30(wk->linkIcon, FALSE);
        LiveBattleMatchIrc_CancelLink(wk->irc);
        *seq = 10;
        break;
    case 10:
        if (LiveBattleMatchIrc_StepCancel(wk->irc)) {
            *seq = 11;
        }
        break;
    case 11:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 28, TRUE, 40, 12);
        break;
    case 12:
        func_ov262_021c5c74(task, LiveBattleMatchFlow_StepMenu);
        break;
    case 13:
        LiveBattleMatchFlow_SetEntryField2(wk->param.playerData, wk);
        if (LiveBattleMatchIrc_SendPlayerData(wk->irc, wk->param.playerData)) {
            *seq = 14;
        }
        break;
    case 14:
        if (LiveBattleMatchIrc_TakePlayerData(wk->irc, &received)) {
            sys_memcpy(received, wk->param.partnerData, PokeParty_GetSaveDataSize() + LIVEBATTLEMATCH_PLAYER_HEADER_SIZE);
            *seq = 15;
        }
        break;
    case 15:
        if (wk->param.playerData->entryField2 == wk->param.partnerData->entryField2) {
            u32 field12 = func_0200b798(wk->entry, 12);
            BOOL known = func_0200f254(wk->record, wk->param.partnerData->mac);

            wk->readySent = (field12 != 0 || !known);
            *seq = 16;
        } else {
            func_ov262_021c4f30(wk->linkIcon, FALSE);
            GFL_SndStop();
            *seq = 21;
        }
        break;
    case 16:
        func_02040624(func_02040440(), 0x48, 0x1f);
        *seq = 17;
        break;
    case 17:
        if (func_02040664(func_02040440(), 0x48, 0x1f)) {
            *seq = 18;
        }
        break;
    case 18:
        if (LiveBattleMatchIrc_SendReady(wk->irc, &wk->readySent)) {
            *seq = 19;
        }
        break;
    case 19:
        if (LiveBattleMatchIrc_TakeReady(wk->irc, &wk->readyReceived)) {
            *seq = 20;
        }
        break;
    case 20:
        func_ov262_021c4f30(wk->linkIcon, FALSE);
        GFL_SndStop();
        if (wk->readyReceived != 0 && wk->readySent != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_74);
            *seq = 27;
        } else {
            *seq = 24;
        }
        break;
    case 21:
        if (LiveBattleMatchIrc_ResetEnd(wk->irc)) {
            *seq = 22;
        }
        break;
    case 22:
        if (LiveBattleMatchIrc_StepEnd(wk->irc)) {
            *seq = 23;
        }
        break;
    case 23:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 22, TRUE, 40, 39);
        break;
    case 24:
        if (LiveBattleMatchIrc_ResetEnd(wk->irc)) {
            *seq = 25;
        }
        break;
    case 25:
        if (LiveBattleMatchIrc_StepEnd(wk->irc)) {
            *seq = 26;
        }
        break;
    case 26:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 36, TRUE, 40, 39);
        break;
    case 27:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 23, TRUE, 40, 28);
        break;
    case 28:
        if (wk->frames++ > 120) {
            wk->frames = 0;
            GFL_BGSysSetBGEnabled(0, FALSE);
            *seq = 29;
        }
        break;
    case 29:
        LiveBattleMatchFlow_PartnerCardCreate(wk, wk->param.partnerData);
        *seq = 30;
        break;
    case 30:
        if (LiveBattleMatchFlow_PartnerCardIsDone(wk)) {
            *seq = 31;
        }
        break;
    case 31:
        if (wk->frames++ > 300) {
            wk->frames = 0;
            *seq = 32;
        }
        break;
    case 32:
        GFL_BGSysSetBGEnabled(0, TRUE);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 24, TRUE, 40, 33);
        break;
    case 33:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 35, 2, 40, 34);
        break;
    case 34:
        func_02040624(func_02040440(), 0x47, 0x1f);
        *seq = 35;
        break;
    case 35:
        if (func_02040664(func_02040440(), 0x47, 0x1f)) {
            *seq = 36;
        }
        break;
    case 36:
        GFL_BGSysSetBGEnabled(0, TRUE);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 38, 2, 40, 37);
        break;
    case 37:
        if (wk->frames++ > 60) {
            Regulation *regulation = func_0200b530(wk->entry);

            wk->frames = 0;
            func_0200b574(regulation, 0x13);
            *seq = 38;
        }
        break;
    case 38:
        LiveBattleMatchFlow_Exit(wk, 2);
        break;
    case 39:
        func_ov262_021c5c74(task, LiveBattleMatchFlow_StepMenu);
        break;
    case 40:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            func_ov262_021c5c8c(task);
        }
        break;
    }
}

// Records how the match went, and shows the player the result
static void LiveBattleMatchFlow_StepRecord(Ov262Task *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;

    if (LiveBattleMatchIrc_CheckError(wk->irc) == TRUE) {
        wk->param.matchResult->disconnected = TRUE;
        func_ov262_021c5c74(task, LiveBattleMatchFlow_StepMenu);
    }
    switch (*seq) {
    case 0: {
        RivalDataSave *rival = getHollow_RivalData(GameData_GetSaveControl(wk->param.gameData));

        func_0200f700(rival, getIDAsUInt(&wk->param.partnerData->info));
        *seq = 1;
        break;
    }
    case 1:
        if (LiveBattleMatchIrc_ResetEnd(wk->irc)) {
            *seq = 2;
        }
        break;
    case 2:
        if (LiveBattleMatchIrc_StepEnd(wk->irc)) {
            *seq = 3;
        }
        break;
    case 3: {
        u32 won;
        u32 lost;
        u8 played = func_0200f1d8(wk->record, 0);

        func_0200f218(wk->record, played, wk->param.partnerData->mac);
        func_0200f238(wk->record, played, 0, wk->param.matchResult->unk08);
        func_0200f238(wk->record, played, 1, wk->param.matchResult->unk09);
        won = 0;
        lost = 0;
        switch (wk->param.matchResult->result) {
        case 1:
        case 4:
            won = 1;
            break;
        case 0:
        case 3:
            lost = 1;
            break;
        case 2:
            break;
        default:
            GFL_ASSERT_MSG(FALSE, "\x83\x6f\x83\x67\x83\x8b\x8c\x8b\x89\xca\x95\x73\x90\xb3\x92\x6c %d\n",
                           wk->param.matchResult->result);
            break;
        }
        func_0200f1e0(wk->record, 1, won);
        func_0200f1e0(wk->record, 2, lost);
        func_0200f1e0(wk->record, 0, 1);
        LiveBattleMatchFlow_PlayerCardCreateWait(wk);
        *seq = 4;
        break;
    }
    case 4:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 10, 2, 8, 5);
        break;
    case 5:
        func_0201782c(wk->param.gameData);
        *seq = 6;
        break;
    case 6:
        if (func_02017850(wk->param.gameData) == 2) {
            *seq = 7;
        }
        break;
    case 7:
        LiveBattleMatchFlow_Exit(wk, 3);
        break;
    case 8:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            func_ov262_021c5c8c(task);
        }
        break;
    }
}

// The flow of a player whose entry has ended
static void LiveBattleMatchFlow_StepQuit(Ov262Task *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;
    s32 selection;

    switch (*seq) {
    case 0:
        if (func_0200b798(wk->entry, 10) == 3) {
            LiveBattleMatchFlow_PlayerCardCreate(wk);
            LiveBattleMatchFlow_PlayerCardShow(wk, 1);
            while (!LiveBattleMatchFlow_PlayerCardIsReady(wk)) {
            }
            *seq = 6;
        } else {
            LiveBattleMatchFlow_PlayerCardCreateWait(wk);
            *seq = 1;
        }
        break;
    case 1: {
        Regulation *regulation = func_0200b530(wk->entry);
        u32 played = func_0200f1d8(wk->record, 0);

        if (played >= func_0200b574(regulation, 0x16)) {
            *seq = 2;
        } else {
            *seq = 16;
        }
        break;
    }
    case 2:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 10, 2, 17, 3);
        break;
    case 3:
        func_0200b830(wk->entry, 10, 3);
        func_0200c370(getBattleBox(GameData_GetSaveControl(wk->param.gameData)), 2);
        func_0201782c(wk->param.gameData);
        *seq = 4;
        break;
    case 4:
        switch (func_02017850(wk->param.gameData)) {
        case 0:
        case 1:
            break;
        case 2:
        case 3:
            *seq = 5;
            break;
        }
        break;
    case 5:
        LiveBattleMatchFlow_PlayerCardShow(wk, 1);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 25, TRUE, 17, 6);
        break;
    case 6:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 29, TRUE, 17, 7);
        break;
    case 7:
        LiveBattleMatchFlow_MenuStart(wk, 2);
        *seq = 8;
        break;
    case 8:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        if (wk->hasVideo && !wk->param.matchResult->disconnected) {
            switch (selection) {
            case 0:
                *seq = 11;
                break;
            case 1:
                *seq = 18;
                break;
            case 2:
                *seq = 9;
                break;
            }
        } else if (selection == 0) {
            *seq = 9;
        }
        break;
    case 9:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 31, TRUE, 17, 10);
        break;
    case 10:
        if (LiveBattleMatchFlow_PlayerCardIsDone(wk)) {
            LiveBattleMatchFlow_PlayerCardDelete(wk);
            *seq = 15;
        }
        break;
    case 11:
        if (func_0200c1d0((u8)wk->param.partnerData->unk44)) {
            *seq = 12;
        } else {
            *seq = 13;
        }
        break;
    case 12:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 16, TRUE, 17, 14);
        break;
    case 13:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 34, TRUE, 17, 6);
        break;
    case 14:
        LiveBattleMatchFlow_Exit(wk, 4);
        break;
    case 15:
        LiveBattleMatchFlow_Exit(wk, 1);
        break;
    case 16:
        func_ov262_021c5c74(task, LiveBattleMatchFlow_StepMenu);
        break;
    case 17:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            func_ov262_021c5c8c(task);
        }
        break;
    case 18:
        LiveBattleMatchFlow_PrintResult(wk);
        *seq = 17;
        func_ov262_021c5c88(task, 6);
        break;
    }
}

// Shows a menu of kind 0 (yes or no at the right), 1 (the menu of a player with an entry), 2 (the menu of a player whose
// entry ended) or 3 (yes or no, which can be left)
static void LiveBattleMatchFlow_MenuStart(LiveBattleMatchFlow *wk, u32 kind) {
    Ov262MenuParam param;
    u32 corner;
    u8 height;
    u8 x;
    u8 y;
    u8 width;

    sys_memset(&param, 0, sizeof(Ov262MenuParam));
    param.msgData = wk->msgMain;
    param.font = wk->param.font;
    param.printQueue = wk->param.printQueue;
    switch (kind) {
    case 3:
        param.unk2E = 1;
    case 0:
        param.msgIds[0] = 6;
        param.msgIds[1] = 7;
        param.count = 2;
        param.unk28 = 1;
        param.unk2C = 1;
        corner = 1;
        break;
    case 1:
        param.unk28 = 0;
        if (wk->hasVideo && !wk->param.matchResult->disconnected) {
            param.msgIds[0] = 13;
            param.msgIds[1] = 14;
            param.msgIds[2] = 40;
            param.msgIds[3] = 15;
            param.count = 4;
        } else {
            param.msgIds[0] = 13;
            param.msgIds[1] = 15;
            param.count = 2;
        }
        corner = 0;
        break;
    case 2:
        param.unk28 = 0;
        if (wk->hasVideo && !wk->param.matchResult->disconnected) {
            param.msgIds[0] = 14;
            param.msgIds[1] = 40;
            param.msgIds[2] = 30;
            param.count = 3;
        } else {
            param.msgIds[0] = 30;
            param.count = 1;
        }
        corner = 0;
        break;
    }
    param.unk20 = 0;
    param.unk22 = 15;
    param.unk24 = 13;
    param.unk26 = 1;
    switch (corner) {
    case 0:
        width = 28;
        height = param.count * 2;
        x = 2;
        y = 9 - height / 2;
        break;
    case 1:
        width = 12;
        height = param.count * 2;
        x = 19;
        y = 17 - height;
        break;
    }
    wk->menu = func_ov262_021c5a88(&param, x, y, width, height, wk->heapId);
    wk->menuKind = kind;
}

static void LiveBattleMatchFlow_MenuFree(LiveBattleMatchFlow *wk) {
    func_ov262_021c5ba4(wk->menu);
}

static s32 LiveBattleMatchFlow_MenuGetSelection(LiveBattleMatchFlow *wk) {
    return func_ov262_021c5bd4(wk->menu);
}

static void LiveBattleMatchFlow_PrintMessage(LiveBattleMatchFlow *wk, u32 msgId, u32 type) {
    func_ov262_021c5888(wk->param.msgBox, wk->msgMain, msgId, type);
}

static BOOL LiveBattleMatchFlow_IsMessageDone(LiveBattleMatchFlow *wk) {
    return func_ov262_021c5a1c(wk->param.msgBox);
}

// Ends the flow with a result: 1 to 4
static void LiveBattleMatchFlow_Exit(LiveBattleMatchFlow *wk, u32 result) {
    wk->result = result;
    func_ov262_021c5c74(wk->task, LiveBattleMatchFlow_StepEnd);
}

// Three bytes as characters, the first at the top
static inline u32 LiveBattleMatchFlow_PackCode(u32 a, u32 b, u32 c) {
    return a << 24 | (b & 0xff) << 16 | (c & 0xff) << 8;
}

static void LiveBattleMatchFlow_PlayerCardCreate(LiveBattleMatchFlow *wk) {
    if (wk->playerCard == NULL) {
        SaveControl *save = GameData_GetSaveControl(wk->param.gameData);
        BattleBoxSave *battleBox = getBattleBox(save);
        Regulation *regulation = func_0200b530(wk->entry);
        void *records = func_0200b924(getTournamentBlockAddress(save), 1);
        PlayerInfo *info = GetGameDataPlayerInfo(wk->param.gameData);
        Ov262NetLink *link = func_ov262_021c2668(wk->param.net);
        Ov262LiveCardInfo card;

        sys_memset(&card, 0, sizeof(Ov262LiveCardInfo));
        card.unkA0 = func_0200f1d8(wk->record, 1);
        card.unkA2 = func_0200f1d8(wk->record, 2);
        card.unkA4 = func_0200f1d8(wk->record, 0);
        card.unkA6 = func_0200b574(regulation, 0x16);
        card.unk9C = func_02008bf4(info);
        sys_memcpy(func_0200b794(wk->entry), card.data, sizeof(card.data));
        card.unk94 = LiveBattleMatchFlow_PackCode(func_0200b798(wk->entry, 4), func_0200b798(wk->entry, 5),
                                                  func_0200b798(wk->entry, 6));
        card.unk98 = LiveBattleMatchFlow_PackCode(func_0200b798(wk->entry, 7), func_0200b798(wk->entry, 8),
                                                  func_0200b798(wk->entry, 9));
        card.unkA8 = func_0200b798(wk->entry, 11);
        wk->playerCard = func_ov262_021c323c(&card, info, link, wk->param.seq, wk->param.font, wk->param.printQueue,
                                             wk->msgCard, wk->wordSet, records, 0, wk->heapId);
    }
}

static void LiveBattleMatchFlow_PlayerCardCreateWait(LiveBattleMatchFlow *wk) {
    LiveBattleMatchFlow_PlayerCardCreate(wk);
    while (!LiveBattleMatchFlow_PlayerCardIsReady(wk)) {
    }
}

static void LiveBattleMatchFlow_PlayerCardDelete(LiveBattleMatchFlow *wk) {
    if (wk->playerCard != NULL) {
        func_ov262_021c33c4(wk->playerCard);
        wk->playerCard = NULL;
    }
}

static BOOL LiveBattleMatchFlow_PlayerCardIsReady(LiveBattleMatchFlow *wk) {
    return func_ov262_021c35ec(wk->playerCard);
}

static BOOL LiveBattleMatchFlow_PlayerCardIsDone(LiveBattleMatchFlow *wk) {
    return func_ov262_021c3720(wk->playerCard);
}

static void LiveBattleMatchFlow_PlayerCardShow(LiveBattleMatchFlow *wk, u32 a1) {
    func_ov262_021c33f0(wk->playerCard, a1, wk->msgCard, wk->param.printQueue, wk->param.font, wk->heapId);
}

static void LiveBattleMatchFlow_PartnerCardCreate(LiveBattleMatchFlow *wk, LiveBattleMatchPlayerData *data) {
    if (wk->partnerCard == NULL) {
        Ov262NetLink *link = func_ov262_021c2668(wk->param.net);
        u32 field11 = func_0200b798(wk->entry, 11);

        wk->partnerCard = func_ov262_021c4644(data, link, wk->param.seq, wk->param.font, wk->param.printQueue,
                                              wk->msgCard, wk->wordSet, 1, 0, field11, wk->heapId);
    }
}

static void LiveBattleMatchFlow_PartnerCardDelete(LiveBattleMatchFlow *wk) {
    if (wk->partnerCard != NULL) {
        func_ov262_021c4834(wk->partnerCard);
        wk->partnerCard = NULL;
    }
}

static BOOL LiveBattleMatchFlow_PartnerCardIsDone(LiveBattleMatchFlow *wk) {
    return func_ov262_021c48a0(wk->partnerCard);
}

static void LiveBattleMatchFlow_TeamListCreate(LiveBattleMatchFlow *wk) {
    if (wk->teamList == NULL) {
        Ov262NetLink *link = func_ov262_021c2668(wk->param.net);
        BattleBoxSave *battleBox = getBattleBox(GameData_GetSaveControl(wk->param.gameData));

        wk->teamList = func_ov262_021c4f60(battleBox, link, wk->param.seq, wk->font, wk->param.printQueue, wk->msgCard,
                                           wk->heapId);
    }
}

static void LiveBattleMatchFlow_TeamListDelete(LiveBattleMatchFlow *wk) {
    if (wk->teamList != NULL) {
        func_ov262_021c53e0(wk->teamList);
        wk->teamList = NULL;
    }
}

static BOOL LiveBattleMatchFlow_TeamListIsReady(LiveBattleMatchFlow *wk) {
    return func_ov262_021c545c(wk->teamList);
}

static BOOL LiveBattleMatchFlow_TeamListIsDone(LiveBattleMatchFlow *wk) {
    return func_ov262_021c5544(wk->teamList);
}

// Copies the team of the battle box into dest
static void LiveBattleMatchFlow_LoadTeam(LiveBattleMatchFlow *wk, PokeParty *dest) {
    PokeParty *party = convertBoxedPokeSetToParty(getBattleBox(GameData_GetSaveControl(wk->param.gameData)), wk->heapId);

    sys_memcpy(party, dest, PokeParty_GetSaveDataSize());
    GFL_HeapFree(party);
}

// Fills what is sent to the other player
static void LiveBattleMatchFlow_FillPlayerData(LiveBattleMatchPlayerData *data, LiveBattleMatchFlow *wk) {
    GameData *gameData = wk->param.gameData;
    PokeParty *party;

    data->unk44 = 100;
    sys_memcpy(GetGameDataPlayerInfo(gameData), data, PlayerInfo_GetSize());
    func_0207c33c(data->mac);
    func_0200ef90(func_0200ef7c(GameData_GetSaveControl(gameData)), 0, &data->sentence);
    party = convertBoxedPokeSetToParty(getBattleBox(GameData_GetSaveControl(gameData)), HEAPID_LIVEBATTLEMATCH_TEAM);
    sys_memcpy(party, (u8 *)data + LIVEBATTLEMATCH_PLAYER_HEADER_SIZE, PokeParty_GetSaveDataSize());
    GFL_HeapFree(party);
    data->record1 = func_0200f1d8(wk->record, 1);
    data->record2 = func_0200f1d8(wk->record, 2);
    data->record0 = func_0200f1d8(wk->record, 0);
    data->entryField2 = func_0200b798(wk->entry, 2);
}

static void LiveBattleMatchFlow_SetEntryField2(LiveBattleMatchPlayerData *data, LiveBattleMatchFlow *wk) {
    data->entryField2 = func_0200b798(wk->entry, 2);
}

// The message for how the last match ended
static u32 LiveBattleMatchFlow_GetResultMessageId(LiveBattleMatchFlow *wk) {
    switch (wk->param.matchResult->result) {
    case 4:
        return 56;
    case 1:
        return 52;
    case 3:
        return 55;
    case 0:
        return 53;
    default:
        GFL_ASSERT_MSG(FALSE, "\x83\x6f\x83\x67\x83\x8b\x8c\x8b\x89\xca\x95\x73\x90\xb3\x92\x6c %d\n",
                       wk->param.matchResult->result);
    case 2:
        return 54;
    }
}

static void LiveBattleMatchFlow_PrintResult(LiveBattleMatchFlow *wk) {
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgMain, LiveBattleMatchFlow_GetResultMessageId(wk));
    StrBuf *text = GFL_StrBufCreate(GFL_StrBufGetCharCount(format) + 0x40, wk->heapId);
    LiveBattleMatchResult *matchResult = wk->param.matchResult;
    PlayerInfo *info = GetGameDataPlayerInfo(wk->param.gameData);
    LiveBattleMatchPlayerData *partnerData = wk->param.partnerData;

    copyVarForText(wk->wordSet, 0, info);
    copyVarForText(wk->wordSet, 1, &partnerData->info);
    WordSetNumber(wk->wordSet, 2, matchResult->unk0A, 1, 0, TRUE);
    WordSetNumber(wk->wordSet, 3, matchResult->unk08, 1, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, text, format);
    func_ov262_021c58a4(wk->param.msgBox, text, TRUE);
    GFL_StrBufFree(text);
    GFL_StrBufFree(format);
}
