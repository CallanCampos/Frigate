// wifibattlematch_link_random.c, in overlay 260 (a descriptive name: the ROM names only the next file,
// wifibattlematch_net.c): the random matchups of the Wi-Fi battle competitions, which the field starts. The player
// picks the free matchups or the ones by rating, and the link finds an opponent through the Global Link server,
// checks the team and the player's data that the other sends, and leaves overlay 290 to pick the teams and battle.
// Each step is a task of overlay 262 (wifibattlematch_util.c) that goes through its states with a message box and
// menus. After the battle the link comes back to send the result

#include "types.h"
#include "app/wifibattlematch_graphic.h"
#include "app/wifibattlematch_link.h"
#include "app/wifibattlematch_net.h"
#include "app/wifibattlematch_util.h"
#include "app/wifibattlematch_view.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "dwc/dwc.h"
#include "gfl/bg_sys.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/nhttp_rap_evilcheck.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/dream_world.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "system/country_region.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/pms.h"
#include "system/pms_data.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/event_fishing.h"

// The modes of the random matchups (WifiBattleMatchLinkParam.mode); the 2 is overlay 290's own
enum {
    LINK_MODE_RATING,
    LINK_MODE_FREE,
    LINK_MODE_QUIT = 3,
};

// The menus
enum {
    MENU_YES_NO,
    MENU_MODE,
};

// What the check of a B press during the search says
enum {
    CANCEL_NONE,
    CANCEL_BUSY,
    CANCEL_DECLINED,
    CANCEL_CONFIRMED,
};

// What a sub task has left while it runs
#define SUB_TASK_RUNNING 0xffff

// The work of the random matchups
typedef struct {
    WifiBattleMatchGraphic *graphic;
    WifiBattleMatchRes *res;
    Font *font;
    PrintQueue *printQueue;
    MsgData *msgData;
    WordSet *wordSet;
    WifiBattleMatchCard *card;
    WifiBattleMatchPartnerCard *partnerCard;
    WifiBattleMatchLinkIcon *linkIcon;
    WifiBattleMatchMsgBox *msgBox;
    WifiBattleMatchMenu *menu;
    u32 menuKind;
    WifiBattleMatchNet *net;
    WifiBattleMatchTask *task;
    WifiBattleMatchTask *subTask;
    u32 subTaskResult;
    WifiBattleMatchLinkParam *param;
    WifiBattleMatchNetCheck signature;
    u32 timer;
    u32 cancelSeq;
    u32 idleTimer;
    u32 unkEC;
    BOOL recordChanged;
    u32 exchangeResult;
    BOOL hadExchangeResult;
    u32 exchangeDone;
} WifiBattleMatchLinkRandom;

static BOOL WifiBattleMatchLink_RandomInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchLink_RandomExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchLink_RandomMain(GameProc *proc, u32 *state, void *param, void *work);
static void WifiBattleMatchLink_TaskStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskSelectMode(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingConnect(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingSearch(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingResult(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingContinue(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingQuitConfirm(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingQuitSave(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeSearch(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeResult(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeContinue(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeQuitConfirm(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRelogin(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskSignTeam(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CreateRankCard(WifiBattleMatchLinkRandom *link, u32 kind);
static void WifiBattleMatchLink_DeleteRankCard(WifiBattleMatchLinkRandom *link);
static BOOL WifiBattleMatchLink_IsRankCardShown(WifiBattleMatchLinkRandom *link);
static BOOL WifiBattleMatchLink_IsRankCardHidden(WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_ShowRankCard(WifiBattleMatchLinkRandom *link, u32 kind);
static void WifiBattleMatchLink_HideRankCard(WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_CreatePartnerCard(WifiBattleMatchLinkRandom *link, WifiBattleMatchLinkPlayer *partner);
static void WifiBattleMatchLink_DeletePartnerCard(WifiBattleMatchLinkRandom *link);
static BOOL WifiBattleMatchLink_IsPartnerCardDone(WifiBattleMatchLinkRandom *link);
static BOOL WifiBattleMatchLink_UpdateRecord(void *results, u32 battleType, WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_AddMatchResult(void *results, u32 battleType, u32 matchResult);
static void WifiBattleMatchLink_CreateMenu(WifiBattleMatchLinkRandom *link, u32 kind);
static void WifiBattleMatchLink_DeleteMenu(WifiBattleMatchLinkRandom *link);
static s32 WifiBattleMatchLink_GetMenuSelection(WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_FillCondition(WifiBattleMatchNetKeys *condition, WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_FillOwnPlayer(WifiBattleMatchLinkPlayer *player, WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_UpdatePlayerRecord(WifiBattleMatchLinkPlayer *player, WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_StartSubTask(WifiBattleMatchLinkRandom *link, WifiBattleMatchTaskFunc func);
static u32 WifiBattleMatchLink_UpdateSubTask(WifiBattleMatchLinkRandom *link);
static BOOL WifiBattleMatchLink_CheckSignature(WifiBattleMatchLinkPlayer *player, HeapID heapId);
static u32 WifiBattleMatchLink_UpdateCancel(WifiBattleMatchLinkRandom *link, BOOL canCancel, BOOL canDisconnect);
static void WifiBattleMatchLink_ResetCancel(WifiBattleMatchLinkRandom *link);

const GameProcFunctions WIFIBATTLEMATCH_LINK_RANDOM_PROC_FUNCTIONS = {
    WifiBattleMatchLink_RandomInit,
    WifiBattleMatchLink_RandomMain,
    WifiBattleMatchLink_RandomExit,
};

static BOOL WifiBattleMatchLink_RandomInit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkParam *linkParam = param;
    WifiBattleMatchLinkRandom *link;

    GFL_OvlLoad(OVERLAY_ID(139));
    GFL_OvlLoad(OVERLAY_ID(189));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK, 0x35000);
    link = GFL_ProcInitSubsystem(proc, sizeof(WifiBattleMatchLinkRandom), HEAPID_WIFIBATTLEMATCH_LINK);
    sys_memset(link, 0, sizeof(WifiBattleMatchLinkRandom));
    link->param = linkParam;
    link->graphic = WifiBattleMatchGraphic_Create(1, HEAPID_WIFIBATTLEMATCH_LINK);
    link->res =
        WifiBattleMatchRes_Create(WifiBattleMatchGraphic_GetClActUnit(link->graphic), 2, HEAPID_WIFIBATTLEMATCH_LINK);
    WifiBattleMatchLink_FillOwnPlayer(linkParam->player, link);
    link->font = GFL_FontCreate(0x17, 0, 0, 0, HEAPID_WIFIBATTLEMATCH_LINK);
    link->printQueue = func_02021998(HEAPID_WIFIBATTLEMATCH_LINK);
    link->msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING, HEAPID_WIFIBATTLEMATCH_LINK);
    link->wordSet = GFL_WordSetSystemCreate(8, 0x40, HEAPID_WIFIBATTLEMATCH_LINK);
    link->net = WifiBattleMatchNet_Create(link->param->unk3C, linkParam->param->gameData, linkParam->unk1C,
                                    HEAPID_WIFIBATTLEMATCH_LINK);
    link->msgBox =
        WifiBattleMatchMsgBox_Create(0, 15, 14, 10, link->printQueue, link->font, HEAPID_WIFIBATTLEMATCH_LINK);
    link->task = WifiBattleMatchTask_Create(link, WifiBattleMatchLink_TaskStart, HEAPID_WIFIBATTLEMATCH_LINK);
    link->subTask = WifiBattleMatchTask_Create(link, NULL, HEAPID_WIFIBATTLEMATCH_LINK);
    link->linkIcon = WifiBattleMatchLinkIcon_Create(WifiBattleMatchGraphic_GetClActUnit(link->graphic), link->res,
                                                    HEAPID_WIFIBATTLEMATCH_LINK);
    WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
    if (linkParam->result != 0) {
        if (linkParam->mode == 0) {
            WifiBattleMatchLink_ShowRankCard(link, 0);
        } else if (linkParam->mode == 1) {
            if (linkParam->result == 1) {
                WifiBattleMatchLink_AddMatchResult(linkParam->record, linkParam->param->unk4,
                                                   linkParam->battleResult->result);
            }
            WifiBattleMatchLink_ShowRankCard(link, 1);
        }
    }
    WifiBattleMatchNet_ShowIcon(link->net, TRUE);
    if (linkParam->result == 1) {
        WifiBattleMatchNet_SetGuest(link->net, *link->param->unk40);
    }
    return TRUE;
}

static BOOL WifiBattleMatchLink_RandomExit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    WifiBattleMatchNet_ShowIcon(link->net, FALSE);
    WifiBattleMatchLinkIcon_Free(link->linkIcon);
    WifiBattleMatchTask_Free(link->subTask);
    WifiBattleMatchTask_Free(link->task);
    WifiBattleMatchMsgBox_Free(link->msgBox);
    if (link->net != NULL) {
        WifiBattleMatchNet_Delete(link->net);
    }
    WifiBattleMatchLink_DeletePartnerCard(link);
    WifiBattleMatchLink_DeleteRankCard(link);
    WifiBattleMatchLink_DeleteMenu(link);
    GFL_WordSetSystemFree(link->wordSet);
    GFL_MsgDataFree(link->msgData);
    func_02021a18(link->printQueue);
    GFL_FontFree(link->font);
    WifiBattleMatchRes_Free(link->res);
    WifiBattleMatchGraphic_Free(link->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFIBATTLEMATCH_LINK);
    GFL_OvlUnload(OVERLAY_ID(189));
    GFL_OvlUnload(OVERLAY_ID(139));
    return TRUE;
}

static BOOL WifiBattleMatchLink_RandomMain(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    switch (*state) {
    case 0:
        GFL_FadeSet(3, 16, 0, 0);
        *state = 1;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            *state = 2;
        }
        break;
    case 2:
        WifiBattleMatchTask_Run(link->task);
        if (WifiBattleMatchTask_IsEnded(link->task)) {
            *state = 3;
        }
        if (link->net != NULL) {
            WifiBattleMatchNet_Nop(link->net);
        }
        break;
    case 3:
        GFL_FadeSet(3, 0, 16, 0);
        *state = 4;
        break;
    case 4:
        if (!GFL_FadeIsRunning()) {
            return TRUE;
        }
        break;
    }
    WifiBattleMatchGraphic_Update(link->graphic);
    func_02021a3c(link->printQueue);
    WifiBattleMatchMsgBox_Update(link->msgBox);
    WifiBattleMatchLinkIcon_Update(link->linkIcon);
    WifiBattleMatchRes_Update(link->res);
    if (link->card != NULL) {
        WifiBattleMatchCard_Flush(link->card, link->printQueue);
    }
    if (link->partnerCard != NULL) {
        WifiBattleMatchPartnerCard_Flush(link->partnerCard, link->printQueue);
    }
    return FALSE;
}

static void WifiBattleMatchLink_TaskStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;

    switch (param->result) {
    case 0:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
        return;
    case 2:
        WifiBattleMatchNet_CancelMatch(link->net);
        // fall through
    case 1:
        if (param->mode == 0) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingResult);
            return;
        } else if (param->mode == 1) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeResult);
            return;
        }
        break;
    case 3:
        if (param->mode == 0) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingAfterBattle);
            return;
        } else if (param->mode == 1) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeAfterBattle);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskSelectMode(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    s32 selected;

    switch (*seq) {
    case 0:
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
        *seq = 1;
        break;
    case 1:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WhichModeWillChallenge,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 7;
        WifiBattleMatchTask_SetReturnSeq(task, 2);
        break;
    case 2:
        WifiBattleMatchLink_CreateMenu(link, MENU_MODE);
        *seq = 3;
        break;
    case 3:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case 0:
            param->mode = LINK_MODE_FREE;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeStart);
            break;
        case 1:
            param->mode = LINK_MODE_RATING;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingStart);
            break;
        case 2:
            param->mode = LINK_MODE_QUIT;
            *seq = 4;
            break;
        }
        break;
    case 4:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WillQuitRandomMatchup,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 7;
        WifiBattleMatchTask_SetReturnSeq(task, 5);
        break;
    case 5:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = 6;
        break;
    case 6:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case 0:
            param->action = WIFIBATTLEMATCH_LINK_ACTION_LOGOUT;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskEnd);
            break;
        case 1:
            *seq = 1;
            break;
        }
        break;
    case 7:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;

    switch (*seq) {
    case 0:
        if (func_020099f4(getDreamWorldStuffAddress(GameData_GetSaveControl(param->param->gameData)))) {
            *seq = 1;
        } else {
            *seq = 6;
        }
        break;
    case 1:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_RandomMatchupRatingMode_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 14;
        WifiBattleMatchTask_SetReturnSeq(task, 2);
        break;
    case 2:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataPleaseWait,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 14;
        WifiBattleMatchTask_SetReturnSeq(task, 3);
        break;
    case 3:
        sys_memset(param->account, 0, sizeof(WifiBattleMatchLinkAccount));
        WifiBattleMatchNet_StartAccountRequest(link->net, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = 4;
        break;
    case 4:
        switch (WifiBattleMatchNet_AccountRequestMain(link->net)) {
        case 0:
            break;
        case 1:
            WifiBattleMatchNet_GetAccountValues(link->net, link->param->account);
            *seq = 5;
            break;
        case 3:
            sys_memset(link->param->account, 0, sizeof(WifiBattleMatchLinkAccount));
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        case 2:
            switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
            case 2:
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
                break;
            case 1:
                WifiBattleMatchLink_HideRankCard(link);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
                break;
            }
            break;
        }
        break;
    case 5:
        if (param->account->registered != 0) {
            *seq = 9;
        } else {
            *seq = 6;
        }
        break;
    case 6:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_GameSyncIdHas,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 14;
        WifiBattleMatchTask_SetReturnSeq(task, 12);
        break;
    case 9:
        WifiBattleMatchLink_StartSubTask(link, WifiBattleMatchLink_TaskSignTeam);
        *seq = 10;
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case 10: {
        u32 signResult = WifiBattleMatchLink_UpdateSubTask(link);

        if (signResult == 0) {
            *seq = 13;
        } else if (signResult == 1) {
            *seq = 11;
        } else if (signResult == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
        }
        if (signResult == SUB_TASK_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    }
    case 11:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereProblemPokemonThey_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 14;
        WifiBattleMatchTask_SetReturnSeq(task, 12);
        break;
    case 12:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskEnd);
        break;
    case 13:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingConnect);
        break;
    case 14:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingConnect(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    BOOL ready;

    switch (*seq) {
    case 0:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataPleaseWait,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, 1);
        *seq = 11;
        break;
    case 1:
        if (*link->param->unk20 != 0) {
            break;
        }
        WifiBattleMatchNet_StartRead(link->net, -1, 0, param->ratings);
        *seq = 2;
        break;
    case 2:
        ready = WifiBattleMatchNet_ReadMain(link->net);
        if (ready == 1) {
            *link->param->unk20 = 1800;
            link->recordChanged = WifiBattleMatchLink_UpdateRecord(param->record, param->param->unk4, link);
            WifiBattleMatchLink_UpdatePlayerRecord(param->player, link);
            *link->param->unk28 = link->param->ratings->unk54;
            *seq = 3;
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            *link->param->unk20 = 1800;
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            *link->param->unk20 = 1800;
            break;
        }
        break;
    case 3:
        if (WifiBattleMatchNet_GetRecordId(link->net) == 0) {
            WifiBattleMatchNet_StartCreateRecord(link->net);
            *seq = 4;
        } else {
            *seq = 5;
        }
        break;
    case 4: {
        u32 done = WifiBattleMatchNet_CreateRecordMain(link->net);

        if (done == 1) {
            *seq = 1;
        }
        if (done == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            *link->param->unk20 = 1800;
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            *link->param->unk20 = 1800;
            break;
        }
        break;
    }
    case 5:
        if (link->card == NULL) {
            *seq = 8;
        } else if (link->recordChanged) {
            *seq = 6;
        } else {
            *seq = 10;
        }
        link->recordChanged = FALSE;
        break;
    case 6:
        *seq = 7;
        break;
    case 7:
        if (WifiBattleMatchLink_IsRankCardHidden(link)) {
            WifiBattleMatchLink_DeleteRankCard(link);
            *seq = 8;
        }
        break;
    case 8:
        WifiBattleMatchLink_CreateRankCard(link, 0);
        *seq = 9;
        break;
    case 9:
        if (WifiBattleMatchLink_IsRankCardShown(link)) {
            *seq = 10;
        }
        break;
    case 10:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingSearch);
        break;
    case 11:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static inline u32 WifiBattleMatchLinkPlayer_GetSize(void) {
    return PokeParty_GetSaveDataSize() + sizeof(WifiBattleMatchLinkPlayer);
}

static void WifiBattleMatchLink_TaskRatingSearch(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchLinkPlayer *otherPlayer;
    void *received;
    WifiBattleMatchNetKeys condition;
    BOOL countIdle = FALSE;
    BOOL canCancel = FALSE;
    BOOL canDisconnect;
    BOOL timedOut;
    BOOL noTerminator;
    BOOL matchFound;
    u32 cancel;
    u32 ret;
    u32 fixCount;
    s32 selected;
    int i;

    if ((s32)*seq >= 2 && (s32)*seq < 8) {
        canCancel = TRUE;
    }
    canDisconnect = (s32)*seq < 6 || (s32)*seq > 7;
    cancel = WifiBattleMatchLink_UpdateCancel(link, canCancel, canDisconnect);
    if (cancel == CANCEL_CONFIRMED) {
        *seq = 22;
        return;
    }
    switch (*seq) {
    case 0:
        link->cancelSeq = 0;
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SearchingOpponent,
                                    WIFIBATTLEMATCH_MSG_QUEUE);
        *seq = 1;
        break;
    case 1:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            return;
        case 2:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        default:
            WifiBattleMatchLink_FillCondition(&condition, link);
            WifiBattleMatchNet_StartMatch(link->net, 2, 1, param->param->unk4, &condition);
            *seq = 2;
            break;
        }
        break;
    case 2:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            WifiBattleMatchLink_ResetCancel(link);
            return;
        case 2:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        default:
            ret = WifiBattleMatchNet_MatchMain(link->net);
            if (ret == 1) {
                link->idleTimer = 0;
                *seq = 3;
            } else if (ret == 2) {
                *seq = 1;
            }
            break;
        }
        break;
    case 3:
        if (cancel != CANCEL_NONE) {
            break;
        }
        if (WifiBattleMatchNet_SendPlayerInfo(link->net, param->player)) {
            link->idleTimer = 0;
            *seq = 4;
        }
        countIdle = TRUE;
        break;
    case 4:
        if (WifiBattleMatchNet_TakePlayerInfo(link->net, &received)) {
            otherPlayer = param->otherPlayer;
            sys_memcpy(received, otherPlayer, WifiBattleMatchLinkPlayer_GetSize());
            fixCount = 0;
            noTerminator = TRUE;
            for (i = 0; i < 8; i++) {
                if (otherPlayer->info.name[i] == GFL_StrBufGetTerminator()) {
                    noTerminator = FALSE;
                }
            }
            if (noTerminator) {
                MsgData *msgData;
                StrBuf *name;

                fixCount++;
                msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING,
                                             HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK));
                name = GFL_MsgDataLoadStrbufNew(msgData, HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK));
                GFL_StrBufStoreString(name, otherPlayer->info.name, 8);
                GFL_StrBufFree(name);
                GFL_MsgDataFree(msgData);
            }
            if (otherPlayer->info.gender != GENDER_MALE && otherPlayer->info.gender != GENDER_FEMALE) {
                otherPlayer->info.gender = GENDER_MALE;
                fixCount++;
            }
            if (otherPlayer->info.unk1C >= 16) {
                otherPlayer->info.unk1C = otherPlayer->info.gender == GENDER_MALE ? 0 : 8;
                fixCount++;
            }
            if (!Country_IsValidPlace(otherPlayer->info.country, otherPlayer->info.province,
                                      otherPlayer->info.region)) {
                otherPlayer->info.country = 0;
                otherPlayer->info.province = 0;
                fixCount++;
            }
            if (!PMSData_IsValid(&otherPlayer->sentence, HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK))) {
                PMSData_Init(&otherPlayer->sentence, 1);
                fixCount++;
            }
            if (fixCount == 0) {
                getPalPadFriendListAddress(GameData_GetSaveControl(param->param->gameData));
            }
            link->idleTimer = 0;
            *seq = 5;
        }
        countIdle = TRUE;
        break;
    case 5:
        if (WifiBattleMatchLink_CheckSignature(param->otherPlayer, HEAPID_WIFIBATTLEMATCH_LINK)) {
            *seq = 6;
        } else {
            WifiBattleMatchNet_CancelMatch(link->net);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingSearch);
        }
        break;
    case 6:
        WifiBattleMatchNet_StartNameCheck(link->net, &param->otherPlayer->info, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = 7;
        break;
    case 7:
        if (WifiBattleMatchNet_NameCheckMain(link->net, &matchFound)) {
            *link->param->unk40 = matchFound;
            *seq = 8;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingContinue);
            WifiBattleMatchLink_ResetCancel(link);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case 8:
        if (cancel != CANCEL_NONE) {
            break;
        }
        WifiBattleMatchNet_StartSync(link->net, 21);
        link->idleTimer = 0;
        *seq = 9;
        break;
    case 9:
        if (!WifiBattleMatchNet_TakeCmd2402(link->net) && WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = 10;
        }
        countIdle = TRUE;
        break;
    case 10:
        WifiBattleMatchNet_StartSync(link->net, 22);
        link->idleTimer = 0;
        *seq = 11;
        break;
    case 11:
        if (WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = 12;
        }
        countIdle = TRUE;
        break;
    case 12:
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
        GFL_SndSEPlay(SEQ_SE_SYS_74);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_OpponentHasBeenFound,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 30;
        WifiBattleMatchTask_SetReturnSeq(task, 13);
        break;
    case 13:
        if (link->timer++ > 120) {
            link->timer = 0;
            WifiBattleMatchLink_CreatePartnerCard(link, param->otherPlayer);
            GFL_BGSysSetBGEnabled(0, FALSE);
            *seq = 14;
        }
        break;
    case 14:
        if (WifiBattleMatchLink_IsPartnerCardDone(link)) {
            *seq = 15;
        }
        break;
    case 15:
        if (link->timer++ != 0) {
            link->timer = 0;
            *seq = 16;
        }
        break;
    case 16:
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK_DWC, 0x60000);
        DWCRapCommon_SetSubHeapEx(HEAPID_DEVICE_ALLOC, 0x60000, 1000, HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        DWCRapCommon_SetSubHeapOwner(HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        WifiBattleMatchNet_StartReport(link->net, 0, 2, 0, 0, 0);
        link->exchangeDone = 0;
        link->exchangeResult = 0;
        *seq = 17;
        break;
    case 17:
        if (link->exchangeDone == 0) {
            link->exchangeDone = WifiBattleMatchNet_ReportMain(link->net, &link->exchangeResult);
        }
        if (link->exchangeDone == 1) {
            *seq = 18;
        }
        if (link->exchangeDone != 0) {
            switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
            case 1:
                WifiBattleMatchNet_CancelMatch(link->net);
                // fall through
            case 2:
            case 4:
                DWCRapCommon_EndSubHeap();
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
                break;
            }
        }
        break;
    case 18:
        DWCRapCommon_EndSubHeap();
        *seq = 19;
        break;
    case 19:
        GFL_BGSysSetBGEnabled(0, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PleaseChoosePokemon,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 30;
        WifiBattleMatchTask_SetReturnSeq(task, 20);
        break;
    case 20:
        if (link->timer++ > 60) {
            link->timer = 0;
            param->action = WIFIBATTLEMATCH_LINK_ACTION_TEAM_LIST;
            WifiBattleMatchTask_End(task);
        }
        break;
    case 21:
        WifiBattleMatchNet_CancelMatch(link->net);
        param->result = 2;
        if (link->exchangeResult != 0) {
            link->hadExchangeResult = TRUE;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingResult);
        } else {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingAfterBattle);
        }
        break;
    case 22:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WillQuitRandomMatchup_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 30;
        WifiBattleMatchTask_SetReturnSeq(task, 23);
        break;
    case 23:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = 24;
        break;
    case 24:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case 0:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingQuitSave);
            break;
        case 1:
            *seq = 25;
            break;
        }
        break;
    case 25:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WantContinueBattling,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 30;
        WifiBattleMatchTask_SetReturnSeq(task, 26);
        break;
    case 26:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = 27;
        break;
    case 27:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 0;
            break;
        case 1:
            *seq = 22;
            break;
        }
        break;
    case 28:
        if (WifiBattleMatchNet_SendCmd2402(link->net)) {
            *seq = 29;
        }
        break;
    case 29:
        WifiBattleMatchNet_CancelMatch(link->net);
        *seq = 0;
        break;
    case 30:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
    if (countIdle) {
        timedOut = FALSE;
        if (link->idleTimer++ > 2400) {
            link->idleTimer = 0;
            timedOut = TRUE;
        }
        if (WifiBattleMatchNet_TakeCmd2402(link->net)) {
            timedOut = TRUE;
        }
        if (timedOut) {
            *seq = 28;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingContinue);
            WifiBattleMatchLink_ResetCancel(link);
            return;
        case 2:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
    }
}

static void WifiBattleMatchLink_TaskRatingResult(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchLinkPlayer *otherPlayer;
    WifiBattleMatchLinkReportBuf reportBuf;
    RivalDataSave *rivalData;
    u32 ready;

    switch (*seq) {
    case 0:
        if (param->result == 2 && link->hadExchangeResult == 0) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingAfterBattle);
            return;
        }
        link->hadExchangeResult = 0;
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK_DWC, 0x60000);
        DWCRapCommon_SetSubHeapEx(HEAPID_DEVICE_ALLOC, 0x60000, 1000, HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        DWCRapCommon_SetSubHeapOwner(HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        *seq = 1;
        break;
    case 1:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 16;
        WifiBattleMatchTask_SetReturnSeq(task, 2);
        break;
    case 2:
        WifiBattleMatchNet_StartReport(link->net, 1, 2, param->param->unk4, &param->battleResult->result, param->result == 2);
        link->exchangeDone = 0;
        link->exchangeResult = 0;
        *seq = 3;
        break;
    case 3:
        if (link->exchangeDone == 0) {
            link->exchangeDone = WifiBattleMatchNet_ReportMain(link->net, &link->exchangeResult);
            if (link->exchangeDone != 0 && link->exchangeResult != 0) {
                func_ov262_021c5c94(param->param->gameData, param->battleResult->result);
            }
        }
        if (link->exchangeDone == 1) {
            param->battleResult->unk14 = WifiBattleMatchNet_GetReceivedValue(link->net);
            *seq = 4;
        }
        if (link->exchangeDone == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
            WifiBattleMatchNet_CancelMatch(link->net);
            // fall through
        case 2:
        case 4:
            DWCRapCommon_EndSubHeap();
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case 4:
        DWCRapCommon_EndSubHeap();
        if (param->result == 2) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingAfterBattle);
            return;
        }
        *seq = 5;
        break;
    case 5:
        otherPlayer = param->otherPlayer;
        if (func_02008bfc(&otherPlayer->info) == VERSION_WHITE2 ||
            func_02008bfc(&otherPlayer->info) == VERSION_BLACK2) {
            if (WifiBattleMatchNet_SendPerson(link->net)) {
                *seq = 6;
            }
        } else {
            *seq = 7;
        }
        break;
    case 6:
        if (WifiBattleMatchNet_TakePerson(link->net)) {
            *seq = 7;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
            WifiBattleMatchNet_CancelMatch(link->net);
            // fall through
        case 2:
        case 4:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case 7:
        rivalData = getHollow_RivalData(GameData_GetSaveControl(param->param->gameData));
        Misc_AddMetTrainer(rivalData, getIDAsUInt(&param->otherPlayer->info));
        *seq = 8;
        WifiBattleMatchNet_ResetEnd(link->net);
        break;
    case 8:
        if (WifiBattleMatchNet_EndMain(link->net)) {
            if (*link->param->unk20 == 0) {
                *seq = 10;
            } else {
                WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WaitingServerDontTurn,
                                            WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
                *seq = 16;
                WifiBattleMatchTask_SetReturnSeq(task, 9);
            }
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            *seq = 9;
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case 9:
        if (*link->param->unk20 == 0) {
            *seq = 10;
        }
        break;
    case 10:
        sys_memset(&reportBuf, 0, sizeof(reportBuf));
        reportBuf.report = *link->param->report;
        reportBuf.unk50 = *link->param->unk28;
        WifiBattleMatchNet_StartWrite(link->net, 3, &reportBuf);
        *seq = 11;
        break;
    case 11:
        ready = WifiBattleMatchNet_WriteMain(link->net);
        if (ready == 1) {
            *link->param->unk20 = 1800;
            *seq = 12;
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            *link->param->unk20 = 1800;
            return;
        case 1:
            *link->param->unk20 = 1800;
            *seq = 12;
            return;
        }
        break;
    case 12:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_BattleResultsWereSent,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 16;
        WifiBattleMatchTask_SetReturnSeq(task, 13);
        break;
    case 13:
        func_ov262_021c5cd4(param->param->gameData, param->battleResult->result);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 16;
        WifiBattleMatchTask_SetReturnSeq(task, 14);
        break;
    case 14:
        func_0201782c(param->param->gameData);
        *seq = 15;
        break;
    case 15:
        switch (func_02017850(param->param->gameData)) {
        case 0:
        case 1:
            break;
        case 2:
        case 3:
            param->action = WIFIBATTLEMATCH_LINK_ACTION_RESULT;
            WifiBattleMatchTask_End(task);
            break;
        }
        break;
    case 16:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    switch (*seq) {
    case 0:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 0:
            *seq = 1;
            break;
        case 1:
        case 4:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingContinue);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case 1:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingContinue);
        break;
    case 2:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingContinue(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    s32 selected;

    switch (*seq) {
    case 0:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 0:
        case 1:
        case 4:
            *seq = 1;
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case 1:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WantContinueBattling,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 4;
        WifiBattleMatchTask_SetReturnSeq(task, 2);
        break;
    case 2:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = 3;
        break;
    case 3:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case 0:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingConnect);
            break;
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingQuitConfirm);
            break;
        }
        break;
    case 4:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingQuitConfirm(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    s32 selected;

    switch (*seq) {
    case 0:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WillQuitRandomMatchup_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 3;
        WifiBattleMatchTask_SetReturnSeq(task, 1);
        break;
    case 1:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = 2;
        break;
    case 2:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case 0:
            WifiBattleMatchNet_CancelMatch(link->net);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingQuitSave);
            break;
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingContinue);
            break;
        }
        break;
    case 3:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingQuitSave(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;

    switch (*seq) {
    case 0:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, 1);
        *seq = 7;
        break;
    case 1:
        func_0201782c(param->param->gameData);
        *seq = 2;
        break;
    case 2:
        if (func_02017850(param->param->gameData) == 2) {
            *seq = 3;
        }
        break;
    case 3:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 7;
        WifiBattleMatchTask_SetReturnSeq(task, 4);
        break;
    case 4:
        if (*link->param->unk20 == 0) {
            WifiBattleMatchNet_StartWrite(link->net, 2, NULL);
            *seq = 5;
        }
        break;
    case 5:
        if (WifiBattleMatchNet_WriteMain(link->net)) {
            *link->param->unk20 = 1800;
            *seq = 6;
            WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE);
        }
        break;
    case 6:
        WifiBattleMatchNet_CancelMatch(link->net);
        param->action = WIFIBATTLEMATCH_LINK_ACTION_LOGOUT;
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskEnd);
        break;
    case 7:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskFreeStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    u32 signResult;

    switch (*seq) {
    case 0:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_RandomMatchupFreeMode_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchLink_CreateRankCard(link, 1);
        *seq = 1;
        break;
    case 1:
        if (WifiBattleMatchLink_IsRankCardShown(link)) {
            *seq = 2;
        }
        break;
    case 2:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            *seq = 3;
        }
        break;
    case 3:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataPleaseWait,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 4;
        break;
    case 4:
        WifiBattleMatchLink_StartSubTask(link, WifiBattleMatchLink_TaskSignTeam);
        *seq = 5;
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case 5:
        signResult = WifiBattleMatchLink_UpdateSubTask(link);
        if (signResult == 0) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeSearch);
        } else if (signResult == 1) {
            *seq = 6;
        } else if (signResult == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
        }
        if (signResult == SUB_TASK_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case 6:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereProblemPokemonThey_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 7;
        break;
    case 7:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskEnd);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskFreeSearch(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchLinkPlayer *otherPlayer;
    void *received;
    WifiBattleMatchNetKeys condition;
    BOOL countIdle = FALSE;
    BOOL canCancel = FALSE;
    BOOL canDisconnect;
    BOOL timedOut;
    BOOL noTerminator;
    BOOL matchFound;
    u32 cancel;
    u32 ret;
    int i;

    if ((s32)*seq >= 3 && (s32)*seq < 13) {
        canCancel = TRUE;
    }
    canDisconnect = (s32)*seq < 11 || (s32)*seq > 12;
    cancel = WifiBattleMatchLink_UpdateCancel(link, canCancel, canDisconnect);
    if (cancel == CANCEL_CONFIRMED) {
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeQuitConfirm);
        return;
    }
    switch (*seq) {
    case 0:
        WifiBattleMatchLink_UpdatePlayerRecord(param->player, link);
        *seq = 1;
        break;
    case 1:
        link->cancelSeq = 0;
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SearchingOpponent,
                                    WIFIBATTLEMATCH_MSG_QUEUE);
        *seq = 2;
        break;
    case 2:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            return;
        case 2:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        default:
            WifiBattleMatchLink_FillCondition(&condition, link);
            WifiBattleMatchNet_StartMatch(link->net, 3, 0, param->param->unk4, &condition);
            *seq = 3;
            break;
        }
        break;
    case 3:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            WifiBattleMatchLink_ResetCancel(link);
            return;
        case 2:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        default:
            ret = WifiBattleMatchNet_MatchMain(link->net);
            if (ret == 1) {
                link->idleTimer = 0;
                *seq = 6;
            } else if (ret == 2) {
                *seq = 2;
            }
            break;
        }
        break;
    case 6:
        WifiBattleMatchNet_StartSync(link->net, 20);
        *seq = 7;
        link->idleTimer = 0;
        break;
    case 7:
        if (WifiBattleMatchNet_CheckSync(link->net)) {
            link->idleTimer = 0;
            *seq = 8;
        }
        countIdle = TRUE;
        break;
    case 8:
        if (cancel != CANCEL_NONE) {
            break;
        }
        if (WifiBattleMatchNet_SendPlayerInfo(link->net, param->player)) {
            link->idleTimer = 0;
            *seq = 9;
        }
        countIdle = TRUE;
        break;
    case 9:
        if (WifiBattleMatchNet_TakePlayerInfo(link->net, &received)) {
            otherPlayer = param->otherPlayer;
            sys_memcpy(received, otherPlayer, WifiBattleMatchLinkPlayer_GetSize());
            noTerminator = TRUE;
            for (i = 0; i < 8; i++) {
                if (otherPlayer->info.name[i] == GFL_StrBufGetTerminator()) {
                    noTerminator = FALSE;
                }
            }
            if (noTerminator) {
                MsgData *msgData;
                StrBuf *name;

                msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING,
                                             HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK));
                name = GFL_MsgDataLoadStrbufNew(msgData, HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK));
                GFL_StrBufStoreString(name, otherPlayer->info.name, 8);
                GFL_StrBufFree(name);
                GFL_MsgDataFree(msgData);
            }
            if (otherPlayer->info.gender != GENDER_MALE && otherPlayer->info.gender != GENDER_FEMALE) {
                otherPlayer->info.gender = GENDER_MALE;
            }
            if (otherPlayer->info.unk1C >= 16) {
                otherPlayer->info.unk1C = otherPlayer->info.gender == GENDER_MALE ? 0 : 8;
            }
            if (!Country_IsValidPlace(otherPlayer->info.country, otherPlayer->info.province,
                                      otherPlayer->info.region)) {
                otherPlayer->info.country = 0;
                otherPlayer->info.province = 0;
            }
            if (!PMSData_IsValid(&otherPlayer->sentence, HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK))) {
                PMSData_Init(&otherPlayer->sentence, 1);
            }
            link->idleTimer = 0;
            *seq = 10;
        }
        countIdle = TRUE;
        break;
    case 10:
        if (WifiBattleMatchLink_CheckSignature(param->otherPlayer, HEAPID_WIFIBATTLEMATCH_LINK)) {
            *seq = 11;
        } else {
            WifiBattleMatchNet_CancelMatch(link->net);
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeSearch);
        }
        break;
    case 11:
        WifiBattleMatchNet_StartNameCheck(link->net, &param->otherPlayer->info, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = 12;
        break;
    case 12:
        if (WifiBattleMatchNet_NameCheckMain(link->net, &matchFound)) {
            *link->param->unk40 = matchFound;
            *seq = 13;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeContinue);
            WifiBattleMatchLink_ResetCancel(link);
            break;
        case 2:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case 13:
        if (cancel != CANCEL_NONE) {
            break;
        }
        WifiBattleMatchNet_StartSync(link->net, 21);
        link->idleTimer = 0;
        *seq = 14;
        break;
    case 14:
        if (!WifiBattleMatchNet_TakeCmd2402(link->net) && WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = 15;
        }
        countIdle = TRUE;
        break;
    case 15:
        WifiBattleMatchNet_StartSync(link->net, 22);
        link->idleTimer = 0;
        *seq = 16;
        break;
    case 16:
        countIdle = TRUE;
        if (WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = 17;
        }
        break;
    case 17:
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
        GFL_SndSEPlay(SEQ_SE_SYS_74);
        WifiBattleMatchLink_DeleteMenu(link);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_OpponentHasBeenFound,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 25;
        WifiBattleMatchTask_SetReturnSeq(task, 18);
        break;
    case 18:
        if (link->timer++ > 120) {
            link->timer = 0;
            WifiBattleMatchLink_CreatePartnerCard(link, param->otherPlayer);
            GFL_BGSysSetBGEnabled(0, FALSE);
            *seq = 19;
        }
        break;
    case 19:
        if (WifiBattleMatchLink_IsPartnerCardDone(link)) {
            *seq = 20;
        }
        break;
    case 20:
        if (link->timer++ > 300) {
            link->timer = 0;
            *seq = 21;
        }
        break;
    case 21:
        GFL_BGSysSetBGEnabled(0, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PleaseChoosePokemon,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 25;
        WifiBattleMatchTask_SetReturnSeq(task, 22);
        break;
    case 22:
        if (link->timer++ > 60) {
            link->timer = 0;
            param->action = WIFIBATTLEMATCH_LINK_ACTION_TEAM_LIST;
            WifiBattleMatchTask_End(task);
        }
        break;
    case 23:
        if (WifiBattleMatchNet_SendCmd2402(link->net)) {
            *seq = 24;
        }
        break;
    case 24:
        WifiBattleMatchNet_CancelMatch(link->net);
        *seq = 1;
        break;
    case 25:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
    if (countIdle) {
        timedOut = FALSE;
        if (link->idleTimer++ > 2400) {
            link->idleTimer = 0;
            timedOut = TRUE;
        }
        if (WifiBattleMatchNet_TakeCmd2402(link->net)) {
            timedOut = TRUE;
        }
        if (timedOut) {
            *seq = 23;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeContinue);
            WifiBattleMatchLink_ResetCancel(link);
            return;
        case 2:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
    }
}

static void WifiBattleMatchLink_TaskFreeResult(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchLinkPlayer *otherPlayer;
    RivalDataSave *rivalData;

    switch (*seq) {
    case 0:
        if (param->result == 2) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeAfterBattle);
            return;
        }
        *seq = 5;
        break;
    case 5:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_AwaitingCommunicationPleaseStand,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 13;
        WifiBattleMatchTask_SetReturnSeq(task, 1);
        link->timer = 0;
        break;
    case 1:
        if (WifiBattleMatchNet_SendReportValue(link->net, &param->battleResult->unk10)) {
            *seq = 2;
        }
        break;
    case 2:
        if (WifiBattleMatchNet_TakeReportValue(link->net, &param->battleResult->unk14)) {
            *seq = 3;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            link->timer = 0;
            *seq = 9;
            return;
        case 2:
            link->timer = 0;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        }
        break;
    case 3:
        otherPlayer = param->otherPlayer;
        if (func_02008bfc(&otherPlayer->info) == VERSION_WHITE2 ||
            func_02008bfc(&otherPlayer->info) == VERSION_BLACK2) {
            if (WifiBattleMatchNet_SendPerson(link->net)) {
                *seq = 4;
            }
        } else {
            *seq = 6;
        }
        break;
    case 4:
        if (WifiBattleMatchNet_TakePerson(link->net)) {
            *seq = 6;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            link->timer = 0;
            *seq = 9;
            return;
        case 2:
            link->timer = 0;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        }
        break;
    case 6:
        rivalData = getHollow_RivalData(GameData_GetSaveControl(param->param->gameData));
        Misc_AddMetTrainer(rivalData, getIDAsUInt(&param->otherPlayer->info));
        func_ov262_021c5c94(param->param->gameData, param->battleResult->result);
        WifiBattleMatchNet_ResetEnd(link->net);
        *seq = 7;
        break;
    case 7:
        link->timer++;
        if (WifiBattleMatchNet_EndMain(link->net)) {
            *seq = 8;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            link->timer = 0;
            *seq = 9;
            return;
        case 2:
            link->timer = 0;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        }
        break;
    case 8:
        if (link->timer++ > 30) {
            link->timer = 0;
            *seq = 9;
        }
        break;
    case 9:
        *seq = 10;
        break;
    case 10:
        func_ov262_021c5cd4(param->param->gameData, param->battleResult->result);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 13;
        WifiBattleMatchTask_SetReturnSeq(task, 11);
        break;
    case 11:
        func_0201782c(param->param->gameData);
        *seq = 12;
        break;
    case 12:
        switch (func_02017850(param->param->gameData)) {
        case 0:
        case 1:
            break;
        case 2:
        case 3:
            param->action = WIFIBATTLEMATCH_LINK_ACTION_RESULT;
            WifiBattleMatchTask_End(task);
            break;
        }
        break;
    case 13:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskFreeAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    switch (*seq) {
    case 0:
        *seq = 1;
        break;
    case 1:
        *seq = 2;
        break;
    case 2:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeContinue);
        break;
    case 3:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskFreeContinue(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    s32 selected;

    switch (*seq) {
    case 0:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WantContinueBattling,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 3;
        WifiBattleMatchTask_SetReturnSeq(task, 1);
        break;
    case 1:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = 2;
        break;
    case 2:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case 0:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeSearch);
            break;
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeQuitConfirm);
            break;
        }
        break;
    case 3:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskFreeQuitConfirm(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    s32 selected;

    switch (*seq) {
    case 0:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WillQuitRandomMatchup_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 5;
        WifiBattleMatchTask_SetReturnSeq(task, 1);
        break;
    case 1:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = 2;
        break;
    case 2:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case 0:
            param->action = WIFIBATTLEMATCH_LINK_ACTION_LOGOUT;
            *seq = 3;
            break;
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeContinue);
            break;
        }
        break;
    case 3:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_AwaitingCommunicationPleaseStand,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 5;
        WifiBattleMatchTask_SetReturnSeq(task, 4);
        break;
    case 4:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskEnd);
        break;
    case 5:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRelogin(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    link->param->action = WIFIBATTLEMATCH_LINK_ACTION_RELOGIN;
    if (link->net != NULL) {
        WifiBattleMatchNet_Delete(link->net);
        link->net = NULL;
    }
    func_02012154();
    WifiBattleMatchTask_End(task);
}

static void WifiBattleMatchLink_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    if (*seq == 0) {
        if (link->net != NULL) {
            WifiBattleMatchNet_Delete(link->net);
            link->net = NULL;
        }
        WifiBattleMatchTask_End(task);
    }
}

static void WifiBattleMatchLink_TaskSignTeam(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkPlayer *player = link->param->player;
    u32 ret;

    switch (*seq) {
    case 0:
        link->timer = 0;
        sys_memset(&link->signature, 0, sizeof(link->signature));
        (*seq)++;
        break;
    case 1:
        WifiBattleMatchNet_StartPokemonCheck(link->net, player->partyData, TRUE);
        (*seq)++;
        break;
    case 2:
        ret = WifiBattleMatchNet_PokemonCheckMain(link->net, &link->signature);
        if (ret == 1) {
            *seq = 3;
        } else if (ret == 2) {
            link->subTaskResult = 2;
            *seq = 4;
        } else if (ret == 3) {
            link->subTaskResult = 3;
            *seq = 4;
        }
        break;
    case 3:
        if (link->signature.status == 0) {
            sys_memcpy(link->signature.signature, player->signature, sizeof(player->signature));
            link->subTaskResult = 0;
            *seq = 4;
        } else {
            link->subTaskResult = 1;
            *seq = 4;
        }
        break;
    case 4:
        WifiBattleMatchTask_End(task);
        break;
    }
}

static void WifiBattleMatchLink_CreateRankCard(WifiBattleMatchLinkRandom *link, u32 kind) {
    GameData *gameData;
    PlayerInfo *playerInfo;
    ClActUnit *unit;
    WifiBattleMatchRankInfo info;
    u32 titleMsg;

    if (link->card != NULL) {
        return;
    }
    gameData = link->param->param->gameData;
    playerInfo = GetGameDataPlayerInfo(gameData);
    unit = WifiBattleMatchGraphic_GetClActUnit(link->graphic);
    sys_memset(&info, 0, sizeof(info));
    if (kind == 0) {
        titleMsg = 1;
        info.titleMsg = link->param->param->unk4;
        info.unk4 = RndMatchData_GetParam(link->param->record, link->param->param->unk4 + 5, 0);
        info.unk10 = RndMatchData_GetParam(link->param->record, link->param->param->unk4 + 5, 1);
        info.unk12 = RndMatchData_GetParam(link->param->record, link->param->param->unk4 + 5, 2);
        info.unkC = info.unk10 + info.unk12;
        if (info.unk4 == 0 && info.unkC == 0) {
            info.unk4 = 1500;
        }
    } else if (kind == 1) {
        titleMsg = 0;
        info.titleMsg = link->param->param->unk4;
        info.unk4 = 0;
        info.unk10 = RndMatchData_GetParam(link->param->record, link->param->param->unk4, 1);
        info.unk12 = RndMatchData_GetParam(link->param->record, link->param->param->unk4, 2);
        info.unkC = info.unk10 + info.unk12;
    }
    info.look = func_02008bf4(playerInfo);
    link->card =
        WifiBattleMatchRankCard_Create(&info, titleMsg, playerInfo, unit, link->res, link->font, link->printQueue,
                                       link->msgData, link->wordSet, HEAPID_WIFIBATTLEMATCH_LINK);
}

static void WifiBattleMatchLink_DeleteRankCard(WifiBattleMatchLinkRandom *link) {
    if (link->card != NULL) {
        WifiBattleMatchRankCard_Free(link->card);
        link->card = NULL;
    }
}

static BOOL WifiBattleMatchLink_IsRankCardShown(WifiBattleMatchLinkRandom *link) {
    return WifiBattleMatchCard_SlideIn(link->card);
}

static BOOL WifiBattleMatchLink_IsRankCardHidden(WifiBattleMatchLinkRandom *link) {
    return WifiBattleMatchCard_SlideOut(link->card);
}

static void WifiBattleMatchLink_ShowRankCard(WifiBattleMatchLinkRandom *link, u32 kind) {
    WifiBattleMatchLink_CreateRankCard(link, kind);
    while (!WifiBattleMatchLink_IsRankCardShown(link)) {
    }
}

static void WifiBattleMatchLink_HideRankCard(WifiBattleMatchLinkRandom *link) {
    if (link->card != NULL) {
        while (!WifiBattleMatchLink_IsRankCardHidden(link)) {
        }
        WifiBattleMatchLink_DeleteRankCard(link);
    }
}

static void WifiBattleMatchLink_CreatePartnerCard(WifiBattleMatchLinkRandom *link, WifiBattleMatchLinkPlayer *partner) {
    ClActUnit *unit;
    u32 kind;
    u32 shown;

    if (link->partnerCard == NULL) {
        unit = WifiBattleMatchGraphic_GetClActUnit(link->graphic);
        if (link->param->mode == 0) {
            shown = 1;
            kind = 2;
        } else if (link->param->mode == 1) {
            shown = 0;
            kind = 3;
        }
        link->partnerCard = WifiBattleMatchPartnerCard_Create(
            (const LiveBattleMatchPlayerData *)partner, unit, link->res, link->font, link->printQueue, link->msgData,
            link->wordSet, kind, shown, 0, HEAPID_WIFIBATTLEMATCH_LINK);
    }
}

static void WifiBattleMatchLink_DeletePartnerCard(WifiBattleMatchLinkRandom *link) {
    if (link->partnerCard != NULL) {
        WifiBattleMatchPartnerCard_Free(link->partnerCard);
        link->partnerCard = NULL;
    }
}

static BOOL WifiBattleMatchLink_IsPartnerCardDone(WifiBattleMatchLinkRandom *link) {
    return WifiBattleMatchPartnerCard_SlideIn(link->partnerCard);
}

static BOOL WifiBattleMatchLink_UpdateRecord(void *results, u32 battleType, WifiBattleMatchLinkRandom *link) {
    u32 rating;
    u32 first;
    u32 second;
    BOOL changed;

    switch (battleType) {
    case 0:
        rating = link->param->ratings->rating[3];
        first = link->param->ratings->results[3].first;
        second = link->param->ratings->results[3].second;
        break;
    case 1:
        rating = link->param->ratings->rating[0];
        first = link->param->ratings->results[0].first;
        second = link->param->ratings->results[0].second;
        break;
    case 2:
        rating = link->param->ratings->rating[4];
        first = link->param->ratings->results[4].first;
        second = link->param->ratings->results[4].second;
        break;
    case 3:
        rating = link->param->ratings->rating[1];
        first = link->param->ratings->results[1].first;
        second = link->param->ratings->results[1].second;
        break;
    case 4:
        rating = link->param->ratings->rating[2];
        first = link->param->ratings->results[2].first;
        second = link->param->ratings->results[2].second;
        break;
    }
    if (rating != RndMatchData_GetParam(results, battleType + 5, 0) || first != RndMatchData_GetParam(results, battleType + 5, 1) ||
        second != RndMatchData_GetParam(results, battleType + 5, 2)) {
        changed = TRUE;
    } else {
        changed = FALSE;
    }
    RndMatchData_SetParam(results, battleType + 5, 0, rating);
    RndMatchData_SetParam(results, battleType + 5, 1, first);
    RndMatchData_SetParam(results, battleType + 5, 2, second);
    return changed;
}

static void WifiBattleMatchLink_AddMatchResult(void *results, u32 battleType, u32 matchResult) {
    switch (matchResult) {
    case 1:
    case 4:
        RndMatchData_IncParam(results, battleType, 1);
        break;
    case 0:
    case 3:
        RndMatchData_IncParam(results, battleType, 2);
        break;
    }
}

static void WifiBattleMatchLink_CreateMenu(WifiBattleMatchLinkRandom *link, u32 kind) {
    WifiBattleMatchMenuParam menuParam;

    if (link->menu == NULL) {
        sys_memset(&menuParam, 0, sizeof(menuParam));
        menuParam.msgData = link->msgData;
        menuParam.font = link->font;
        menuParam.printQueue = link->printQueue;
        menuParam.bg = 0;
        menuParam.palette = 15;
        menuParam.framePalette = 13;
        menuParam.frameChar = 1;
        switch (kind) {
        case MENU_YES_NO:
            menuParam.msgIds[0] = EventFishing_Text_Yes;
            menuParam.msgIds[1] = EventFishing_Text_No;
            menuParam.count = 2;
            menuParam.canCancel = TRUE;
            menuParam.cancelValue = 1;
            break;
        case MENU_MODE:
            menuParam.msgIds[0] = EventFishing_Text_Free;
            menuParam.msgIds[1] = EventFishing_Text_Rating_2;
            menuParam.msgIds[2] = EventFishing_Text_Quit;
            menuParam.count = 3;
            menuParam.canCancel = TRUE;
            menuParam.cancelValue = 2;
            break;
        }
        link->menuKind = kind;
        link->menu = WifiBattleMatchMenu_CreateBottomRight(&menuParam, HEAPID_WIFIBATTLEMATCH_LINK);
    }
}

static void WifiBattleMatchLink_DeleteMenu(WifiBattleMatchLinkRandom *link) {
    if (link->menu != NULL) {
        WifiBattleMatchMenu_Free(link->menu);
        link->menu = NULL;
    }
}

static s32 WifiBattleMatchLink_GetMenuSelection(WifiBattleMatchLinkRandom *link) {
    if (link->menu != NULL) {
        return WifiBattleMatchMenu_Update(link->menu);
    }
    return -1;
}

static void WifiBattleMatchLink_FillCondition(WifiBattleMatchNetKeys *condition, WifiBattleMatchLinkRandom *link) {
    WifiBattleMatchLinkParam *param;

    sys_memset(condition, 0, sizeof(WifiBattleMatchNetKeys));
    param = link->param;
    condition->disconnects = param->ratings->unk20;
    condition->cup = 0;
    condition->matches = param->ratings->results[3].first + param->ratings->results[3].second +
                         param->ratings->results[0].first + param->ratings->results[0].second +
                         param->ratings->results[4].first + param->ratings->results[4].second +
                         param->ratings->results[1].first + param->ratings->results[1].second +
                         param->ratings->results[2].first + param->ratings->results[2].second;
    switch (param->param->unk4) {
    case 0:
        condition->rating = param->ratings->rating[3];
        break;
    case 1:
        condition->rating = param->ratings->rating[0];
        break;
    case 2:
        condition->rating = param->ratings->rating[4];
        break;
    case 3:
        condition->rating = param->ratings->rating[1];
        break;
    case 4:
        condition->rating = param->ratings->rating[2];
        break;
    }
}

static void WifiBattleMatchLink_FillOwnPlayer(WifiBattleMatchLinkPlayer *player, WifiBattleMatchLinkRandom *link) {
    GameData *gameData = link->param->param->gameData;
    SaveControl *save = GameData_GetSaveControl(gameData);
    PlayerInfo *info;

    player->unk44 = 100;
    info = GetGameDataPlayerInfo(gameData);
    sys_memcpy(info, player, PlayerInfo_GetSize());
    CGearPhrases_Get(CGearPhrases_GetBlock(save), 0, &player->sentence);
    if (link->param->mode == 0) {
        player->record1 = RndMatchData_GetParam(link->param->record, link->param->param->unk4 + 5, 1);
        player->record2 = RndMatchData_GetParam(link->param->record, link->param->param->unk4 + 5, 2);
    } else {
        player->record1 = RndMatchData_GetParam(link->param->record, link->param->param->unk4, 1);
        player->record2 = RndMatchData_GetParam(link->param->record, link->param->param->unk4, 2);
    }
    player->gsid = WifiList_GetMyGSID(GameData_GetWifiList(gameData));
}

static void WifiBattleMatchLink_UpdatePlayerRecord(WifiBattleMatchLinkPlayer *player, WifiBattleMatchLinkRandom *link) {
    if (link->param->mode == 0) {
        player->record1 = RndMatchData_GetParam(link->param->record, link->param->param->unk4 + 5, 1);
        player->record2 = RndMatchData_GetParam(link->param->record, link->param->param->unk4 + 5, 2);
        player->unk34 = RndMatchData_GetParam(link->param->record, link->param->param->unk4 + 5, 0);
    } else {
        player->record1 = RndMatchData_GetParam(link->param->record, link->param->param->unk4, 1);
        player->record2 = RndMatchData_GetParam(link->param->record, link->param->param->unk4, 2);
    }
}

static void WifiBattleMatchLink_StartSubTask(WifiBattleMatchLinkRandom *link, WifiBattleMatchTaskFunc func) {
    WifiBattleMatchTask_SetFunc(link->subTask, func);
    link->subTaskResult = SUB_TASK_RUNNING;
}

static u32 WifiBattleMatchLink_UpdateSubTask(WifiBattleMatchLinkRandom *link) {
    WifiBattleMatchTask_Run(link->subTask);
    if (WifiBattleMatchTask_IsEnded(link->subTask)) {
        return link->subTaskResult;
    }
    return SUB_TASK_RUNNING;
}

static BOOL WifiBattleMatchLink_CheckSignature(WifiBattleMatchLinkPlayer *player, HeapID heapId) {
    int i;
    PokeParty *party = (PokeParty *)player->partyData;
    void *array;
    BOOL result;

    array = NHttpRapEvilCheck_AllocArray(PokeParty_GetPkmRawSize(), PokeParty_GetPkmCount(party), HEAPID_TAIL(heapId));
    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        NHttpRapEvilCheck_SetElement(array, pkm, PokeParty_GetPkmRawSize(), i);
    }
    result = NHttpRapEvilCheck_Sign(array, PokeParty_GetPkmRawSize(), PokeParty_GetPkmCount(party), player->signature,
                                    HEAPID_TAIL(heapId));
    NHttpRapEvilCheck_FreeArray(array);
    return result;
}

static u32 WifiBattleMatchLink_UpdateCancel(WifiBattleMatchLinkRandom *link, BOOL canCancel, BOOL canDisconnect) {
    s32 selected;

    switch (link->cancelSeq) {
    case 0:
        if (canCancel && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            link->cancelSeq++;
            break;
        }
        return CANCEL_NONE;
    case 1:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_QuitSearchingSomeoneBattle,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        link->cancelSeq++;
        break;
    case 2:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            link->cancelSeq++;
        }
        break;
    case 3:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        link->cancelSeq++;
        break;
    case 4:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case 0:
            link->cancelSeq = 5;
            break;
        case 1:
            WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SearchingOpponent,
                                        WIFIBATTLEMATCH_MSG_QUEUE);
            link->cancelSeq = 0;
            return CANCEL_DECLINED;
            break;
        }
        break;
    case 5:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_AwaitingCommunicationPleaseStand,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        link->cancelSeq++;
        break;
    case 6:
        if (canDisconnect) {
            WifiBattleMatchNet_SendCmd2402(link->net);
            link->cancelSeq++;
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchNet_CancelMatch(link->net);
            if (!GFL_NetErrCheck() && func_02042788()) {
                func_ov011_02160170();
            }
        }
        break;
    case 7:
        link->cancelSeq = 0;
        return CANCEL_CONFIRMED;
    }
    return CANCEL_BUSY;
}

static void WifiBattleMatchLink_ResetCancel(WifiBattleMatchLinkRandom *link) {
    link->cancelSeq = 0;
    WifiBattleMatchLink_DeleteMenu(link);
}
