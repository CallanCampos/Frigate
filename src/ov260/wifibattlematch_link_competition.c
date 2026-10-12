// wifibattlematch_link_competition.c, in overlay 260 (a descriptive name: the ROM names only the next file,
// wifibattlematch_net.c): the Wi-Fi competition of the title screen's menu. The link checks the account on the
// Global Link server and the competition's period, updates the Digital Player ID, registers the Battle Box, finds an
// opponent, checks the team and the player's data that the other sends, and leaves overlay 290 to pick the teams
// and battle. Each step is a task of overlay 262 (wifibattlematch_util.c) that goes through its states with a
// message box and menus

#include "types.h"
#include "app/livebattlematch_flow.h"
#include "app/wifibattlematch_graphic.h"
#include "app/wifibattlematch_link.h"
#include "app/wifibattlematch_net.h"
#include "app/wifibattlematch_util.h"
#include "app/wifibattlematch_view.h"
#include "battle/regulation.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
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
#include "gfl/str_sjis.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/dream_world.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/tournament_block.h"
#include "save/wifi_list.h"
#include "system/country_region.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/pms.h"
#include "system/pms_data.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/event_fishing.h"

#define HEAPID_WIFIBATTLEMATCH 0x56

// The menus
enum {
    COMPETITION_MENU_YES_NO,
    COMPETITION_MENU_JOIN_INFO_CANCEL,
    COMPETITION_MENU_REGISTER,
    COMPETITION_MENU_BATTLE,
    COMPETITION_MENU_BATTLE_AGAIN,
    COMPETITION_MENU_YES_NO_NO_FIRST,
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

// What a request to the server's account takes
typedef struct {
    u8 unk0;
    u8 unk1;
    u32 unk4;
} WifiBattleMatchLinkRequest;

// The work of the Wi-Fi competition
typedef struct {
    WifiBattleMatchGraphic *graphic;
    WifiBattleMatchRes *res;
    Font *font;
    Font *smallFont;
    PrintQueue *printQueue;
    MsgData *msgData;
    WordSet *wordSet;
    WifiBattleMatchCard *card;
    WifiBattleMatchPartnerCard *partnerCard;
    WifiBattleMatchTeamList *teamList;
    WifiBattleMatchLinkIcon *linkIcon;
    WifiBattleMatchMsgBox *msgBox;
    WifiBattleMatchMenu *menu;
    WifiBattleMatchNet *net;
    WifiBattleMatchTask *task;
    WifiBattleMatchTask *subTask;
    u32 subTaskResult;
    WifiBattleMatchNetCheck signature;
    u32 timer;
    u32 unkE4;
    u32 unkE8;
    u32 cancelSeq;
    u32 idleTimer;
    u32 exchangeResult;
    u32 exchangeDone;
    PokeParty *party;
    BOOL hadExchangeResult;
    WifiBattleMatchLinkParam *param;
    void *entry;
    u8 serverEntry[0x160];
    BOOL hadBattleBox;
} WifiBattleMatchLinkCompetition;

// The three bytes of a date in the tournament block, the first in the high byte
static inline u32 WifiBattleMatchLink_PackBytes(u32 high, u32 middle, u32 low) {
    return (high << 24) | ((middle & 0xff) << 16) | ((low & 0xff) << 8);
}

static BOOL WifiBattleMatchLink_CompetitionInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchLink_CompetitionExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchLink_CompetitionMain(GameProc *proc, u32 *state, void *param, void *work);
static void WifiBattleMatchLink_CompetitionTaskStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskAccount(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskEntry(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskRegister(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskMenu(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskMatch(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskResult(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskContinue(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskCancel(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskLogout(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskQuit(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskRelogin(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskCheckPeriod(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskUnregister(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskSignTeam(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionCreateCard(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionDeleteCard(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionIsCardShown(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionIsCardHidden(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionShowCard(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionSetCardPage(WifiBattleMatchLinkCompetition *link, u32 page);
static void WifiBattleMatchLink_CompetitionCreatePartnerCard(WifiBattleMatchLinkCompetition *link,
                                                             WifiBattleMatchLinkPlayer *partner);
static void WifiBattleMatchLink_CompetitionDeletePartnerCard(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionIsPartnerCardDone(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionCreateTeamList(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionDeleteTeamList(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionIsTeamListShown(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionIsTeamListHidden(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionCreateMenu(WifiBattleMatchLinkCompetition *link, u32 kind);
static void WifiBattleMatchLink_CompetitionDeleteMenu(WifiBattleMatchLinkCompetition *link);
static s32 WifiBattleMatchLink_CompetitionGetMenuSelection(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionStartSubTask(WifiBattleMatchLinkCompetition *link,
                                                        WifiBattleMatchTaskFunc func);
static u32 WifiBattleMatchLink_CompetitionUpdateSubTask(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionFillBattleBoxParty(WifiBattleMatchLinkPlayer *player,
                                                              WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionFillPlayer(WifiBattleMatchLinkPlayer *player,
                                                      WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionCopySignature(WifiBattleMatchLinkPlayer *player,
                                                         WifiBattleMatchNetCheck *signature);
static BOOL WifiBattleMatchLink_CompetitionCheckSignature(WifiBattleMatchLinkPlayer *player, PokeParty *party,
                                                          HeapID heapId);
static BOOL WifiBattleMatchLink_CompetitionCompareParties(PokeParty *party, PokeParty *otherParty);
static u32 WifiBattleMatchLink_CompetitionUpdateCancel(WifiBattleMatchLinkCompetition *link, BOOL canCancel,
                                                       BOOL canDisconnect);
static void WifiBattleMatchLink_CompetitionResetCancel(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionUpdateRecord(WifiBattleMatchLinkCompetition *link,
                                                        WifiBattleMatchLinkRanking *ranking);

const GameProcFunctions WIFIBATTLEMATCH_LINK_COMPETITION_PROC_FUNCTIONS = {
    WifiBattleMatchLink_CompetitionInit,
    WifiBattleMatchLink_CompetitionMain,
    WifiBattleMatchLink_CompetitionExit,
};

static BOOL WifiBattleMatchLink_CompetitionInit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkParam *linkParam = param;
    WifiBattleMatchLinkCompetition *link;

    GFL_OvlLoad(OVERLAY_ID(139));
    GFL_OvlLoad(OVERLAY_ID(189));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK, 0x30000);
    link = GFL_ProcInitSubsystem(proc, sizeof(WifiBattleMatchLinkCompetition), HEAPID_WIFIBATTLEMATCH_LINK);
    sys_memset(link, 0, sizeof(WifiBattleMatchLinkCompetition));
    link->param = linkParam;
    link->entry = TournamentBlock_GetEntry(getTournamentBlockAddress(GameData_GetSaveControl(linkParam->param->gameData)), 0);
    link->graphic = WifiBattleMatchGraphic_Create(1, HEAPID_WIFIBATTLEMATCH_LINK);
    link->res =
        WifiBattleMatchRes_Create(WifiBattleMatchGraphic_GetClActUnit(link->graphic), 0, HEAPID_WIFIBATTLEMATCH_LINK);
    link->font = GFL_FontCreate(0x17, 0, 0, 0, HEAPID_WIFIBATTLEMATCH_LINK);
    link->smallFont = GFL_FontCreate(0x17, 3, 0, 0, HEAPID_WIFIBATTLEMATCH_LINK);
    link->printQueue = func_02021998(HEAPID_WIFIBATTLEMATCH_LINK);
    link->msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING, HEAPID_WIFIBATTLEMATCH_LINK);
    link->wordSet = GFL_WordSetSystemCreate(8, 0x40, HEAPID_WIFIBATTLEMATCH_LINK);
    link->net = WifiBattleMatchNet_Create(link->param->unk3C, linkParam->param->gameData, linkParam->unk1C,
                                    HEAPID_WIFIBATTLEMATCH_LINK);
    link->msgBox =
        WifiBattleMatchMsgBox_Create(0, 15, 14, 10, link->printQueue, link->font, HEAPID_WIFIBATTLEMATCH_LINK);
    link->task =
        WifiBattleMatchTask_Create(link, WifiBattleMatchLink_CompetitionTaskStart, HEAPID_WIFIBATTLEMATCH_LINK);
    link->subTask = WifiBattleMatchTask_Create(link, NULL, HEAPID_WIFIBATTLEMATCH_LINK);
    link->linkIcon = WifiBattleMatchLinkIcon_Create(WifiBattleMatchGraphic_GetClActUnit(link->graphic), link->res,
                                                    HEAPID_WIFIBATTLEMATCH_LINK);
    WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
    link->party = PokeParty_Create(HEAPID_WIFIBATTLEMATCH_LINK);
    WifiBattleMatchNet_ShowIcon(link->net, TRUE);
    return TRUE;
}

static BOOL WifiBattleMatchLink_CompetitionExit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkCompetition *link = work;

    GFL_HeapFree(link->party);
    WifiBattleMatchNet_ShowIcon(link->net, FALSE);
    WifiBattleMatchLinkIcon_Free(link->linkIcon);
    WifiBattleMatchTask_Free(link->subTask);
    WifiBattleMatchTask_Free(link->task);
    WifiBattleMatchMsgBox_Free(link->msgBox);
    if (link->net != NULL) {
        WifiBattleMatchNet_Delete(link->net);
    }
    WifiBattleMatchLink_CompetitionDeleteTeamList(link);
    WifiBattleMatchLink_CompetitionDeletePartnerCard(link);
    WifiBattleMatchLink_CompetitionDeleteCard(link);
    WifiBattleMatchLink_CompetitionDeleteMenu(link);
    GFL_WordSetSystemFree(link->wordSet);
    GFL_MsgDataFree(link->msgData);
    func_02021a18(link->printQueue);
    GFL_FontFree(link->smallFont);
    GFL_FontFree(link->font);
    WifiBattleMatchRes_Free(link->res);
    WifiBattleMatchGraphic_Free(link->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFIBATTLEMATCH_LINK);
    GFL_OvlUnload(OVERLAY_ID(189));
    GFL_OvlUnload(OVERLAY_ID(139));
    return TRUE;
}

static BOOL WifiBattleMatchLink_CompetitionMain(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkCompetition *link = work;

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
    WifiBattleMatchNet_Nop(link->net);
    if (link->card != NULL) {
        WifiBattleMatchCard_Flush(link->card, link->printQueue);
    }
    if (link->partnerCard != NULL) {
        WifiBattleMatchPartnerCard_Flush(link->partnerCard, link->printQueue);
    }
    if (link->teamList != NULL) {
        WifiBattleMatchTeamList_Flush(link->teamList, link->printQueue);
    }
    return FALSE;
}

static void WifiBattleMatchLink_CompetitionTaskStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;

    switch (link->param->result) {
    case 0:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskAccount);
        return;
    case 2:
        WifiBattleMatchNet_CancelMatch(link->net);
        // fall through
    case 1:
        WifiBattleMatchLink_CompetitionShowCard(link);
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskResult);
        return;
    case 3:
        WifiBattleMatchLink_CompetitionShowCard(link);
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskAfterBattle);
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskAccount(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    u32 ready;

    switch (*seq) {
    case 0:
        if (func_020099f4(getDreamWorldStuffAddress(GameData_GetSaveControl(param->param->gameData)))) {
            *seq = 1;
        } else {
            *seq = 5;
        }
        break;
    case 1:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SearchingCompetitionData,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 2;
        break;
    case 2:
        sys_memset(param->account, 0, sizeof(WifiBattleMatchLinkAccount));
        WifiBattleMatchNet_StartAccountRequest(link->net, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = 3;
        break;
    case 3:
        ready = WifiBattleMatchNet_AccountRequestMain(link->net);
        if (ready == 1) {
            WifiBattleMatchNet_GetAccountValues(link->net, link->param->account);
            *seq = 4;
            break;
        }
        if (ready == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 4:
        if (param->account->registered != 0) {
            *seq = 7;
        } else {
            *seq = 5;
        }
        break;
    case 5:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_GameSyncIdHas,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 6;
        break;
    case 6:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskEnd);
        }
        break;
    case 7:
        if (*link->param->unk20 == 0) {
            WifiBattleMatchNet_ResetLogin(link->net);
            *seq = 8;
        }
        break;
    case 8:
        ready = WifiBattleMatchNet_LoginMain(link->net);
        if (ready == 1) {
            *link->param->unk20 = 1800;
            *seq = 9;
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 9:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskEntry);
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskEntry(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    void *entry = link->entry;
    u32 ready;
    s32 selected;
    u32 state;
    u8 accountState;

    switch (*seq) {
    case 0:
        if (param->account->unk4 == 1) {
            *seq = 1;
        } else {
            *seq = 3;
        }
        break;
    case 1: {
        WifiBattleMatchLinkRequest request;

        sys_memset(&request, 0, sizeof(request));
        request.unk1 = 0;
        request.unk0 = link->param->account->unk4;
        request.unk4 = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->unk5 = 0;
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = 2;
        break;
    }
    case 2:
        ready = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ready == 1) {
            *seq = 11;
        } else if (ready == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 3:
        if (link->param->account->unk5 == TournamentEntry_GetField(entry, 10)) {
            *seq = 4;
        } else if (TournamentEntry_GetField(entry, 10) == 4 || TournamentEntry_GetField(entry, 10) == 5) {
            *seq = 7;
        } else {
            *seq = 4;
        }
        break;
    case 4:
        accountState = param->account->unk5;
        if (accountState == 3) {
            *seq = 5;
        } else if (accountState == 4 || accountState == 5) {
            *seq = 6;
        } else {
            *seq = 11;
        }
        break;
    case 5:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_NoCompetitionsHaveBeen,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 46);
        *seq = 56;
        break;
    case 6:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_NoCompetitionsHaveBeen,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 46);
        *seq = 56;
        break;
    case 7: {
        WifiBattleMatchLinkRequest request;

        sys_memset(&request, 0, sizeof(request));
        request.unk1 = TournamentEntry_GetField(entry, 10);
        request.unk4 = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->unk5 = TournamentEntry_GetField(entry, 10);
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = 8;
        break;
    }
    case 8:
        ready = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ready == 1) {
            *seq = 9;
        } else if (ready == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 9:
        *seq = 6;
        break;
    case 10:
        ready = WifiBattleMatchNet_WriteMain(link->net);
        if (ready == 1) {
            *link->param->unk20 = 1800;
            *seq = 6;
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 11:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CheckingCompetitionDetails,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, 12);
        *seq = 56;
        break;
    case 12:
        WifiBattleMatchNet_InitDownload(link->net, param->account->profileId);
        *seq = 13;
        break;
    case 13:
        ready = WifiBattleMatchNet_DownloadMain(link->net);
        if (ready == 1) {
            WifiBattleMatchNet_GetDownload(link->net, &link->serverEntry);
            if (TournamentEntry_CheckCRC(&link->serverEntry)) {
                *seq = 14;
            } else {
                *seq = 15;
            }
        } else if (ready == 2) {
            *seq = 15;
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 15:
        if (TournamentEntry_GetField(entry, 2) == 0) {
            *seq = 18;
        } else {
            *seq = 16;
        }
        break;
    case 16:
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        case 0:
            WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskCheckPeriod);
            *seq = 17;
            break;
        }
        break;
    case 17:
        ready = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ready != SUB_TASK_RUNNING) {
            *seq = 46;
        }
        if (ready == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 18:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereNoCurrentCompetitions,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 55);
        *seq = 56;
        break;
    case 14:
        if (param->account->unk5 == 0) {
            *seq = 19;
        } else if (param->account->unk5 == 1) {
            *seq = 53;
        } else if (param->account->unk5 == 2) {
            *seq = 54;
        } else {
            *seq = 55;
        }
        break;
    case 19:
        if (TournamentEntry_GetField(entry, 2) == 0) {
            *seq = 42;
        } else {
            *seq = 20;
        }
        break;
    case 20:
        WifiBattleMatchLink_CompetitionCreateCard(link);
        if (BattleBoxSave_CheckFlags(getBattleBox(GameData_GetSaveControl(param->param->gameData)), 1)) {
            WifiBattleMatchLink_CompetitionSetCardPage(link, 0);
        } else {
            WifiBattleMatchLink_CompetitionSetCardPage(link, 1);
        }
        *seq = 21;
        break;
    case 21:
        if (WifiBattleMatchLink_CompetitionIsCardShown(link)) {
            *seq = 22;
        }
        break;
    case 22:
        if (BattleBoxSave_CheckFlags(getBattleBox(GameData_GetSaveControl(param->param->gameData)), 1)) {
            *seq = 23;
        } else {
            *seq = 29;
        }
        break;
    case 23:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ParticipateWiFiCompetition_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 24);
        *seq = 56;
        break;
    case 24:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 0);
        *seq = 25;
        break;
    case 25:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 35;
            break;
        case 1:
            *seq = 26;
            break;
        }
        break;
    case 26:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CannotParticipateWithoutUpdating,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 27);
        *seq = 56;
        break;
    case 27:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 0);
        *seq = 28;
        break;
    case 28:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 55;
            break;
        case 1:
            *seq = 23;
            break;
        }
        break;
    case 29:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ParticipateWiFiCompetition,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 30);
        *seq = 56;
        break;
    case 30:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 0);
        *seq = 31;
        break;
    case 31:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 35;
            break;
        case 1:
            *seq = 32;
            break;
        }
        break;
    case 32:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CannotParticipateWithoutUpdating,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 33);
        *seq = 56;
        break;
    case 33:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 0);
        *seq = 34;
        break;
    case 34:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 55;
            break;
        case 1:
            *seq = 29;
            break;
        }
        break;
    case 35:
        if (WifiBattleMatchLink_CompetitionIsCardHidden(link)) {
            *seq = 36;
        }
        break;
    case 36:
        WifiBattleMatchLink_CompetitionDeleteCard(link);
        *seq = 42;
        break;
    case 37:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, 38);
        *seq = 56;
        break;
    case 38:
        if (*link->param->unk20 == 0) {
            sys_memset(&param->unk34->body, 0, sizeof(param->unk34->body));
            WifiBattleMatchNet_StartWrite(link->net, 0, &link->param->unk34->body);
            *seq = 39;
        }
        break;
    case 39:
        ready = WifiBattleMatchNet_WriteMain(link->net);
        if (ready == 1) {
            *link->param->unk20 = 1800;
            *seq = 40;
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 40: {
        WifiBattleMatchLinkRequest request;

        sys_memset(&request, 0, sizeof(request));
        request.unk0 = 0;
        request.unk1 = 1;
        request.unk4 = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->unk5 = 1;
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = 41;
        break;
    }
    case 41:
        ready = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ready == 1) {
            *seq = 53;
        } else if (ready == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 42:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PlayerIdWillUpdated,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, 43);
        *seq = 56;
        break;
    case 43: {
        SaveControl *save = GameData_GetSaveControl(param->param->gameData);
        BattleBoxSave *battleBox = getBattleBox(save);
        void *previous = TournamentBlock_GetTeam(getTournamentBlockAddress(save), 0);

        sys_memcpy(&link->serverEntry, link->entry, sizeof(link->serverEntry));
        BattleBoxSave_ClearFlags(battleBox, 1);
        TournamentEntry_SetField(link->entry, 10, 1);
        RndMatchData_SetParam(param->record, 10, 0, 0);
        RndMatchData_SetParam(param->record, 10, 1, 0);
        RndMatchData_SetParam(param->record, 10, 2, 0);
        TournamentTeam_Clear(previous);
        func_0201782c(param->param->gameData);
        *seq = 44;
        break;
    }
    case 44:
        state = func_02017850(param->param->gameData);
        if (state == 2) {
            *seq = 37;
        } else if (state == 3) {
            *seq = 45;
        }
        break;
    case 45:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_UnableUpdateDigitalPlayer,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 55);
        *seq = 56;
        break;
    case 46:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataDontTurn,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, 47);
        *seq = 56;
        break;
    case 47:
        if (*link->param->unk20 == 0) {
            WifiBattleMatchNet_StartRead(link->net, -1, 1, param->unk34);
            *seq = 48;
        }
        break;
    case 48:
        ready = WifiBattleMatchNet_ReadMain(link->net);
        if (ready == 1) {
            *link->param->unk20 = 1800;
            *seq = 49;
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 49:
        if (link->param->unk34->unk10 != RndMatchData_GetParam(param->record, 10, 0)) {
            *seq = 50;
        } else {
            *seq = 55;
        }
        break;
    case 50:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PlayerIdWillUpdated,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, 51);
        *seq = 56;
        break;
    case 51:
        WifiBattleMatchLink_CompetitionUpdateRecord(link, param->unk34);
        func_0201782c(param->param->gameData);
        *seq = 52;
        break;
    case 52:
        if (func_02017850(param->param->gameData) == 2) {
            *seq = 55;
        }
        break;
    case 53:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRegister);
        break;
    case 54:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
        break;
    case 55:
        param->action = WIFIBATTLEMATCH_LINK_ACTION_LOGOUT;
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskEnd);
        break;
    case 56:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskRegister(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchLinkPlayer *player;
    Regulation *regulation;
    PokeParty *party;
    SaveControl *save;
    BattleBoxSave *battleBox;
    void *previous;
    BOOL ready;
    u32 invalid;
    BOOL otherInvalid;
    u32 invalidFlags;
    s32 selected;
    u32 ret;
    int count;
    int i;
    int zeroCount;

    switch (*seq) {
    case 0:
        WifiBattleMatchLink_CompetitionCreateCard(link);
        WifiBattleMatchLink_CompetitionSetCardPage(link, 2);
        WifiBattleMatchLink_CompetitionCreateTeamList(link);
        *seq = 1;
        break;
    case 1:
        ready = TRUE;
        ready &= WifiBattleMatchLink_CompetitionIsCardShown(link);
        if (WifiBattleMatchLink_CompetitionIsTeamListShown(link) & ready) {
            *seq = 2;
        }
        break;
    case 2:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_BattleBoxWillRegistered,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 3);
        *seq = 30;
        break;
    case 3:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 2);
        *seq = 4;
        break;
    case 4:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 12;
            break;
        case 1:
            *seq = 5;
            break;
        }
        break;
    case 5:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_QuitRegistration,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 6);
        *seq = 30;
        break;
    case 6:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 0);
        *seq = 7;
        break;
    case 7:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 8;
            break;
        case 1:
            *seq = 2;
            break;
        }
        break;
    case 8:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_QuitCompetitionRegistration,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 9);
        *seq = 30;
        break;
    case 9:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 0);
        *seq = 10;
        break;
    case 10:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 11;
            break;
        case 1:
            *seq = 2;
            break;
        }
        break;
    case 11:
        if (WifiBattleMatchLink_CompetitionIsTeamListHidden(link)) {
            WifiBattleMatchLink_CompetitionDeleteTeamList(link);
            *seq = 29;
        }
        break;
    case 12:
        player = param->player;
        WifiBattleMatchLink_CompetitionFillBattleBoxParty(player, link);
        if (howManyPokesAreAbleToFight((PokeParty *)player->partyData) == 0) {
            *seq = 14;
            break;
        }
        invalidFlags = 0;
        regulation = func_0200b530(link->entry);
        invalid = PokeRegulation_CheckParty(regulation, (PokeParty *)player->partyData, &invalidFlags);
        otherInvalid = PokeRegulation_PartyHasEgg(regulation, (PokeParty *)player->partyData);
        if (invalid == 0 && otherInvalid == 0) {
            *seq = 15;
        } else {
            *seq = 13;
        }
        break;
    case 13:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ConditionsTournamentHaveNot,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 29);
        *seq = 30;
        break;
    case 14:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereNoPokemonBattle,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 29);
        *seq = 30;
        break;
    case 15:
        if (WifiBattleMatchLink_CompetitionIsTeamListHidden(link)) {
            WifiBattleMatchLink_CompetitionDeleteTeamList(link);
            *seq = 16;
        }
        break;
    case 16:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, 17);
        *seq = 30;
        break;
    case 17:
        WifiBattleMatchNet_StartPokemonCheck(link->net, param->player->partyData, TRUE);
        sys_memset(&link->signature, 0, sizeof(link->signature));
        *seq = 18;
        break;
    case 18:
        ret = WifiBattleMatchNet_PokemonCheckMain(link->net, &link->signature);
        if (ret == 1) {
            count = PokeParty_GetPkmCount((PokeParty *)param->player->partyData);
            zeroCount = 0;
            for (i = 0; i < count; i++) {
                if (link->signature.result[i] == 0) {
                    zeroCount++;
                }
            }
            if (link->signature.status == 0 && zeroCount == count) {
                *seq = 20;
            } else {
                *seq = 19;
            }
        } else if (ret == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            *seq = 0;
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 19:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereProblemPokemonThey,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 29);
        *seq = 30;
        break;
    case 20:
        if (*link->param->unk20 == 0) {
            WifiBattleMatchNet_StartWrite(link->net, 0, param->player->partyData);
            *seq = 21;
        }
        break;
    case 21:
        ready = WifiBattleMatchNet_WriteMain(link->net);
        if (ready == 1) {
            *link->param->unk20 = 1800;
            *seq = 22;
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            *seq = 0;
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 22:
        save = GameData_GetSaveControl(param->param->gameData);
        battleBox = getBattleBox(save);
        previous = TournamentBlock_GetTeam(getTournamentBlockAddress(save), 0);
        party = convertBoxedPokeSetToParty(battleBox, HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH));
        BattleBoxSave_SetFlags(battleBox, 1);
        TournamentEntry_SetField(link->entry, 10, 2);
        TournamentTeam_SetFromParty(previous, party);
        GFL_HeapFree(party);
        *seq = 23;
        break;
    case 23:
        func_0201782c(param->param->gameData);
        *seq = 24;
        break;
    case 24:
        ret = func_02017850(param->param->gameData);
        if (ret == 2) {
            *seq = 25;
        } else if (ret == 3) {
            *seq = 29;
        }
        break;
    case 25: {
        WifiBattleMatchLinkRequest request;

        sys_memset(&request, 0, sizeof(request));
        request.unk1 = 2;
        request.unk4 = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->unk5 = 2;
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = 26;
        break;
    }
    case 26:
        ret = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ret == 1) {
            *seq = 27;
        } else if (ret == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            *seq = 0;
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 27:
        WifiBattleMatchLink_CompetitionSetCardPage(link, 0);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CompetitionRegistrationComplete,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 28);
        *seq = 30;
        break;
    case 28:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
        break;
    case 29:
        param->action = WIFIBATTLEMATCH_LINK_ACTION_LOGOUT;
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case 30:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskMenu(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    u32 ret;
    s32 selected;

    switch (*seq) {
    case 0:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 0:
            *seq = 1;
            break;
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 1:
        if (link->card == NULL) {
            WifiBattleMatchLink_CompetitionCreateCard(link);
            WifiBattleMatchLink_CompetitionSetCardPage(link, 0);
            *seq = 2;
        } else {
            *seq = 3;
        }
        break;
    case 2:
        if (WifiBattleMatchLink_CompetitionIsCardShown(link)) {
            *seq = 3;
        }
        break;
    case 3:
        GFL_BGSysSetBGEnabled(0, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PokemonCompetitionOverNintendo,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, 4);
        *seq = 16;
        break;
    case 4:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 3);
        *seq = 5;
        break;
    case 5:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 10;
            break;
        case 1:
            *seq = 7;
            break;
        case 2:
            *seq = 6;
            break;
        }
        break;
    case 6:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskCancel);
        break;
    case 7:
        WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskUnregister);
        *seq = 8;
        break;
    case 8:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == 1 || ret == 2) {
            *seq = 1;
        } else if (ret == 0) {
            *seq = 9;
        } else if (ret == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 9:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case 10:
        WifiBattleMatchLink_CompetitionFillBattleBoxParty(param->player, link);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataPleaseWait,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, 11);
        *seq = 16;
        break;
    case 11:
        WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskSignTeam);
        *seq = 12;
        break;
    case 12:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == 0) {
            *seq = 13;
        } else if (ret == 1) {
            *seq = 13;
        } else if (ret == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 13:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMatch);
        break;
    case 14:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereProblemPokemonThey_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 16;
        WifiBattleMatchTask_SetReturnSeq(task, 15);
        break;
    case 15:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case 16:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskMatch(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchNetKeys condition;
    void *received;
    u32 flags;
    BOOL matchFound;
    char ownName[8];
    char otherName[8];
    WifiBattleMatchLinkPlayer *player;
    WifiBattleMatchLinkPlayer *otherPlayer;
    PokeParty *party;
    void *entry;
    Regulation *regulation;
    BOOL countIdle = FALSE;
    BOOL canCancel = FALSE;
    BOOL canDisconnect;
    BOOL timedOut;
    BOOL noTerminator;
    u32 cancel;
    u32 fixCount;
    u32 ret;
    s32 selected;
    int i;

    if ((s32)*seq >= 12 && (s32)*seq < 27) {
        canCancel = TRUE;
    }
    canDisconnect = ((s32)*seq < 25 || (s32)*seq > 26) && ((s32)*seq < 17 || (s32)*seq > 18);
    cancel = WifiBattleMatchLink_CompetitionUpdateCancel(link, canCancel, canDisconnect);
    if (cancel == CANCEL_CONFIRMED) {
        *seq = 40;
        return;
    }
    switch (*seq) {
    case 0:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataPleaseWait,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, 1);
        *seq = 53;
        break;
    case 1:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        case 0:
            WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskCheckPeriod);
            *seq = 2;
            break;
        }
        break;
    case 2:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == 0) {
            *seq = 4;
        } else if (ret == 1 || ret == 2) {
            *seq = 3;
        } else if (ret == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 3:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case 4:
        if (*link->param->unk20 != 0) {
            break;
        }
        WifiBattleMatchNet_StartRead(link->net, -1, 1, link->param->unk34);
        *seq = 5;
        break;
    case 5:
        ret = WifiBattleMatchNet_ReadMain(link->net);
        if (ret == 1) {
            player = param->player;
            *link->param->unk20 = 0;
            WifiBattleMatchLink_CompetitionFillPlayer(player, link);
            *link->param->unk28 = link->param->unk34->unk14;
            if (WifiBattleMatchLink_CompetitionUpdateRecord(link, link->param->unk34)) {
                *seq = 6;
            } else {
                *seq = 10;
            }
        }
        if (ret == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 6:
        *seq = 7;
        break;
    case 7:
        if (WifiBattleMatchLink_CompetitionIsCardHidden(link)) {
            WifiBattleMatchLink_CompetitionDeleteCard(link);
            *seq = 8;
        }
        break;
    case 8:
        WifiBattleMatchLink_CompetitionCreateCard(link);
        *seq = 9;
        break;
    case 9:
        if (WifiBattleMatchLink_CompetitionIsCardShown(link)) {
            *seq = 10;
        }
        break;
    case 10:
        link->cancelSeq = 0;
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SearchingOpponent_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE);
        *seq = 53;
        WifiBattleMatchTask_SetReturnSeq(task, 11);
        break;
    case 11:
        entry = link->entry;
        sys_memset(&condition, 0, sizeof(condition));
        condition.rating = link->param->unk34->unk10;
        condition.disconnects = link->param->unk34->unk4;
        condition.cup = TournamentEntry_GetField(entry, 2);
        condition.matches = link->param->unk34->unk8 + link->param->unk34->unkC;
        WifiBattleMatchNet_StartMatch(link->net, 0, 0, param->param->unk4, &condition);
        link->unkE4 = 0;
        link->unkE8 = 0;
        *seq = 12;
        break;
    case 12:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case 2:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        ret = WifiBattleMatchNet_MatchMain(link->net);
        if (ret == 1) {
            *seq = 13;
        } else if (ret == 2) {
            *seq = 11;
        }
        break;
    case 13:
        if (cancel == CANCEL_NONE) {
            if (WifiBattleMatchNet_SendPlayerInfo(link->net, param->player)) {
                *seq = 14;
                link->idleTimer = 0;
            }
        } else {
            switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
            case 1:
            case 4:
                link->cancelSeq = 0;
                WifiBattleMatchLink_CompetitionDeleteMenu(link);
                WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
                WifiBattleMatchLink_CompetitionResetCancel(link);
                break;
            case 2:
                WifiBattleMatchLink_CompetitionDeleteMenu(link);
                WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
                break;
            }
        }
        break;
    case 14:
        if (WifiBattleMatchNet_TakePlayerInfo(link->net, &received)) {
            sys_memcpy(received, param->otherPlayer, PokeParty_GetSaveDataSize() + sizeof(WifiBattleMatchLinkPlayer));
            otherPlayer = param->otherPlayer;
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
            *seq = 15;
        }
        countIdle = TRUE;
        break;
    case 15:
        player = param->player;
        StrUnicodeToSjis(player->info.name, ownName, 8);
        party = (PokeParty *)player->partyData;
        for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
            PokeParty_GetPkm(party, i);
        }
        otherPlayer = param->otherPlayer;
        StrUnicodeToSjis(otherPlayer->info.name, otherName, 8);
        party = (PokeParty *)otherPlayer->partyData;
        for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
            PokeParty_GetPkm(party, i);
        }
        if (param->player->entryField2 == param->otherPlayer->entryField2 &&
            param->player->checksum == param->otherPlayer->checksum) {
            *seq = 16;
        } else {
            *seq = 51;
        }
        break;
    case 16:
        flags = 0;
        regulation = func_0200b530(link->entry);
        if (PokeRegulation_CheckParty(regulation, (PokeParty *)param->otherPlayer->partyData, &flags) == 0) {
            *seq = 17;
        } else {
            link->unkE4++;
            *seq = 22;
        }
        break;
    case 17:
        if (*link->param->unk20 == 0) {
            WifiBattleMatchNet_StartRead(link->net, link->param->otherPlayer->unk4C, 3, link->party);
            *seq = 18;
        }
        break;
    case 18:
        ret = WifiBattleMatchNet_ReadMain(link->net);
        if (ret == 1) {
            *link->param->unk20 = 3600;
            *seq = 19;
        }
        if (ret == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case 2:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 19:
        WifiBattleMatchNet_StartSync(link->net, 23);
        *seq = 20;
        break;
    case 20:
        if (WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = 21;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case 2:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 21:
        if (!WifiBattleMatchLink_CompetitionCompareParties(link->party,
                                                           (PokeParty *)link->param->otherPlayer->partyData)) {
            link->unkE4++;
        }
        if (!WifiBattleMatchLink_CompetitionCheckSignature(link->param->otherPlayer,
                                                           (PokeParty *)link->param->otherPlayer->partyData,
                                                           HEAPID_WIFIBATTLEMATCH_LINK)) {
            link->unkE4++;
        }
        *seq = 22;
        break;
    case 22:
        if (cancel == CANCEL_NONE) {
            if (WifiBattleMatchNet_SendValue4(link->net, &link->unkE4)) {
                *seq = 23;
                link->idleTimer = 0;
            }
        } else {
            switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
            case 1:
            case 4:
                link->cancelSeq = 0;
                WifiBattleMatchLink_CompetitionDeleteMenu(link);
                WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
                WifiBattleMatchLink_CompetitionResetCancel(link);
                break;
            case 2:
                WifiBattleMatchLink_CompetitionDeleteMenu(link);
                WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
                break;
            }
        }
        break;
    case 23:
        if (WifiBattleMatchNet_TakeValue4(link->net, &link->unkE8)) {
            *seq = 24;
        }
        countIdle = TRUE;
        break;
    case 24:
        if (link->unkE8 == 0 && link->unkE4 == 0) {
            *seq = 25;
        } else {
            *seq = 51;
        }
        break;
    case 25:
        WifiBattleMatchNet_StartNameCheck(link->net, &param->otherPlayer->info, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = 26;
        break;
    case 26:
        if (WifiBattleMatchNet_NameCheckMain(link->net, &matchFound)) {
            *link->param->unk40 = matchFound;
            *seq = 27;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            link->cancelSeq = 0;
            WifiBattleMatchLink_CompetitionDeleteMenu(link);
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case 2:
            WifiBattleMatchLink_CompetitionDeleteMenu(link);
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 27:
        if (cancel == CANCEL_NONE) {
            WifiBattleMatchNet_StartSync(link->net, 21);
            link->idleTimer = 0;
            *seq = 28;
        }
        break;
    case 28:
        if (!WifiBattleMatchNet_TakeCmd2402(link->net) && WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = 29;
        }
        countIdle = TRUE;
        break;
    case 29:
        WifiBattleMatchNet_StartSync(link->net, 22);
        *seq = 30;
        link->idleTimer = 0;
        break;
    case 30:
        if (WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = 31;
        }
        countIdle = TRUE;
        break;
    case 31:
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
        GFL_SndSEPlay(SEQ_SE_SYS_74);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_OpponentHasBeenFound_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 53;
        WifiBattleMatchTask_SetReturnSeq(task, 32);
        link->timer = 0;
        break;
    case 32:
        if (link->timer++ > 120) {
            link->timer = 0;
            GFL_BGSysSetBGEnabled(0, FALSE);
            WifiBattleMatchLink_CompetitionCreatePartnerCard(link, param->otherPlayer);
            *seq = 33;
        }
        break;
    case 33:
        if (WifiBattleMatchLink_CompetitionIsPartnerCardDone(link)) {
            *seq = 34;
        }
        break;
    case 34:
        if (link->timer++ != 0) {
            link->timer = 0;
            *seq = 35;
        }
        break;
    case 35:
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK_DWC, 0x60000);
        DWCRapCommon_SetSubHeapEx(HEAPID_DEVICE_ALLOC, 0x60000, 1000, HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        DWCRapCommon_SetSubHeapOwner(HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        WifiBattleMatchNet_StartReport(link->net, 0, 0, 0, 0, 0);
        link->exchangeResult = 0;
        *seq = 36;
        link->timer = 0;
        link->exchangeDone = 0;
        break;
    case 36:
        if (link->exchangeDone == 0) {
            link->exchangeDone = WifiBattleMatchNet_ReportMain(link->net, &link->exchangeResult);
        }
        if (link->exchangeDone == 1) {
            *seq = 37;
        }
        if (link->exchangeDone != 0) {
            switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
            case 1:
                WifiBattleMatchNet_CancelMatch(link->net);
                // fall through
            case 2:
            case 4:
                DWCRapCommon_EndSubHeap();
                WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
                break;
            case 0:
                if (link->exchangeDone == 2) {
                    if (link->timer++ > 3600) {
                        GFL_ASSERT(0);
                    }
                }
                break;
            }
        }
        break;
    case 37:
        DWCRapCommon_EndSubHeap();
        GFL_BGSysSetBGEnabled(0, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PleaseChoosePokemon,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 53;
        WifiBattleMatchTask_SetReturnSeq(task, 38);
        break;
    case 38:
        if (link->timer++ > 60) {
            link->timer = 0;
            param->action = WIFIBATTLEMATCH_LINK_ACTION_TEAM_LIST;
            WifiBattleMatchTask_End(task);
        }
        break;
    case 39:
        WifiBattleMatchNet_CancelMatch(link->net);
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
        param->result = 2;
        link->hadExchangeResult = TRUE;
        if (link->exchangeResult != 0) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskResult);
        } else {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
        }
        break;
    case 40:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_QuitCompetition,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 53;
        WifiBattleMatchTask_SetReturnSeq(task, 41);
        break;
    case 41:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 0);
        *seq = 42;
        break;
    case 42:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskQuit);
            break;
        case 1:
            *seq = 43;
            break;
        }
        break;
    case 43:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WantContinueBattling,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 53;
        WifiBattleMatchTask_SetReturnSeq(task, 44);
        break;
    case 44:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 0);
        *seq = 45;
        break;
    case 45:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 46;
            break;
        case 1:
            *seq = 40;
            break;
        }
        break;
    case 46:
        *seq = 47;
        break;
    case 47:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 0:
            WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskCheckPeriod);
            *seq = 48;
            break;
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 48:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == 0) {
            *seq = 10;
        } else if (ret == 1 || ret == 2) {
            *seq = 3;
        } else if (ret == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 49:
        if (WifiBattleMatchNet_SendCmd2402(link->net)) {
            *seq = 50;
        }
        break;
    case 50:
        WifiBattleMatchNet_CancelMatch(link->net);
        *seq = 10;
        break;
    case 51:
        WifiBattleMatchNet_ResetEnd(link->net);
        *seq = 52;
        link->idleTimer = 0;
        break;
    case 52:
        countIdle = TRUE;
        if (WifiBattleMatchNet_EndMain(link->net)) {
            *seq = 11;
        }
        break;
    case 53:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
    if (countIdle) {
        timedOut = FALSE;
        if (link->idleTimer++ > 1200) {
            link->idleTimer = 0;
            timedOut = TRUE;
        }
        if (WifiBattleMatchNet_TakeCmd2402(link->net)) {
            timedOut = TRUE;
        }
        if (timedOut) {
            WifiBattleMatchNet_CancelMatch(link->net);
            if (!GFL_NetErrCheck() && func_02042788()) {
                func_ov011_02160170();
            }
            *seq = 10;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            return;
        case 2:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
    }
}

static void WifiBattleMatchLink_CompetitionTaskResult(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchLinkReportBuf reportBuf;
    RivalDataSave *rivalData;
    u32 ready;

    switch (*seq) {
    case 0:
        if (param->result == 2 && link->hadExchangeResult == 0) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskAfterBattle);
            return;
        }
        link->hadExchangeResult = 0;
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK_DWC, 0x60000);
        DWCRapCommon_SetSubHeapEx(HEAPID_DEVICE_ALLOC, 0x60000, 1000, HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        DWCRapCommon_SetSubHeapOwner(HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        *seq = 1;
        break;
    case 1:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 13;
        WifiBattleMatchTask_SetReturnSeq(task, 2);
        break;
    case 2:
        WifiBattleMatchNet_StartReport(link->net, 1, 0, param->param->unk4, &param->battleResult->result, param->result == 2);
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
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            DWCRapCommon_EndSubHeap();
            return;
        }
        break;
    case 4:
        DWCRapCommon_EndSubHeap();
        if (param->result == 2) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskAfterBattle);
            return;
        }
        *seq = 5;
        break;
    case 5:
        rivalData = getHollow_RivalData(GameData_GetSaveControl(param->param->gameData));
        Misc_AddMetTrainer(rivalData, getIDAsUInt(&param->otherPlayer->info));
        WifiBattleMatchNet_ResetEnd(link->net);
        *seq = 6;
        break;
    case 6:
        if (WifiBattleMatchNet_EndMain(link->net)) {
            *seq = 7;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case 1:
        case 4:
            *seq = 7;
            return;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            return;
        }
        break;
    case 7:
        if (*link->param->unk20 != 0) {
            break;
        }
        sys_memset(&reportBuf, 0, sizeof(reportBuf));
        reportBuf.report = *link->param->report;
        reportBuf.unk50 = *link->param->unk28;
        WifiBattleMatchNet_StartWrite(link->net, 3, &reportBuf);
        *seq = 8;
        break;
    case 8:
        ready = WifiBattleMatchNet_WriteMain(link->net);
        if (ready == 1) {
            *link->param->unk20 = 1800;
            *seq = 9;
        }
        if (ready == 0) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            return;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            return;
        }
        break;
    case 9:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_BattleResultsWereSent,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 13;
        WifiBattleMatchTask_SetReturnSeq(task, 10);
        break;
    case 10:
        param->action = WIFIBATTLEMATCH_LINK_ACTION_RESULT;
        WifiBattleMatchTask_End(task);
        break;
    case 13:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    u32 ret;

    switch (*seq) {
    case 0:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 5;
        WifiBattleMatchTask_SetReturnSeq(task, 1);
        break;
    case 1:
        if (param->result != 2) {
            func_ov262_021c5cd4(param->param->gameData, param->battleResult->result);
        }
        WifiBattleMatchLink_CompetitionUpdateRecord(link, link->param->unk34);
        func_0201782c(param->param->gameData);
        *seq = 2;
        break;
    case 2:
        ret = func_02017850(param->param->gameData);
        if (ret == 2) {
            *seq = 3;
        } else if (ret == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        }
        break;
    case 3:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        case 0:
            WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskCheckPeriod);
            *seq = 4;
            break;
        }
        break;
    case 4:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == 0) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
        } else if (ret == 1 || ret == 2) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        } else if (ret == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 5:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskContinue(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    u32 ret;
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
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 1:
        GFL_BGSysSetBGEnabled(0, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PokemonWiFiCompetition,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 7;
        WifiBattleMatchTask_SetReturnSeq(task, 2);
        break;
    case 2:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 4);
        *seq = 3;
        break;
    case 3:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMatch);
            break;
        case 1:
            *seq = 4;
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskCancel);
            break;
        }
        break;
    case 4:
        WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskUnregister);
        *seq = 5;
        break;
    case 5:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == 1 || ret == 2) {
            *seq = 1;
        } else if (ret == 0) {
            *seq = 6;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 6:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case 7:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskCancel(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    s32 selected;

    switch (*seq) {
    case 0:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_QuitCompetition,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 3;
        WifiBattleMatchTask_SetReturnSeq(task, 1);
        break;
    case 1:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 0);
        *seq = 2;
        break;
    case 2:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskQuit);
            break;
        case 1:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
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

static void WifiBattleMatchLink_CompetitionTaskLogout(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
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
        WifiBattleMatchTask_SetReturnSeq(task, 4);
        *seq = 7;
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
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskEnd);
        break;
    case 7:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskQuit(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    u32 ret;

    switch (*seq) {
    case 0:
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        case 0:
            WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskCheckPeriod);
            *seq = 1;
            break;
        }
        break;
    case 1:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret != SUB_TASK_RUNNING) {
            *seq = 2;
        }
        if (ret == 3) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case 1:
        case 2:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case 2:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case 3:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchTask_End(task);
}

static void WifiBattleMatchLink_CompetitionTaskRelogin(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;

    link->param->action = WIFIBATTLEMATCH_LINK_ACTION_RELOGIN;
    if (link->net != NULL) {
        WifiBattleMatchNet_Delete(link->net);
        link->net = NULL;
    }
    func_02012154();
    WifiBattleMatchTask_End(task);
}

static void WifiBattleMatchLink_CompetitionTaskCheckPeriod(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    void *entry;
    RTCDate startDate;
    RTCDate endDate;
    RTCDate date;
    RTCTime time;
    BOOL valid;
    s32 startDay;
    s32 endDay;
    s32 today;
    u8 accountState;
    u32 ret;
    WifiBattleMatchLinkRequest request;

    switch (*seq) {
    case 0:
        *seq = 1;
        break;
    case 1:
        entry = link->entry;
        if (!func_02042788()) {
            GFL_NetErrAbort();
        }
        valid = func_ov011_0215dda8(&date, &time);
        if (valid) {
            func_0207d244(&date, &time, func_0207d12c(&date, &time));
        }
        startDate.year = TournamentEntry_GetField(entry, 4);
        startDate.month = TournamentEntry_GetField(entry, 5);
        startDate.day = TournamentEntry_GetField(entry, 6);
        startDate.week = RTC_WEEK_SUNDAY;
        endDate.year = TournamentEntry_GetField(entry, 7);
        endDate.month = TournamentEntry_GetField(entry, 8);
        endDate.day = TournamentEntry_GetField(entry, 9);
        endDate.week = RTC_WEEK_SUNDAY;
        startDay = func_0207d0b4(&startDate);
        endDay = func_0207d0b4(&endDate);
        today = func_0207d0b4(&date);
        if (valid && today < startDay) {
            link->subTaskResult = 2;
            *seq = 2;
            return;
        }
        if (valid && startDay <= today && today <= endDay) {
            link->subTaskResult = 0;
            *seq = 13;
            return;
        }
        link->subTaskResult = 1;
        *seq = 3;
        break;
    case 2:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CompetitionYouveRegisteredNot,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 14;
        WifiBattleMatchTask_SetReturnSeq(task, 13);
        break;
    case 3:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CompetitionTimeHasRun,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 14;
        WifiBattleMatchTask_SetReturnSeq(task, 4);
        break;
    case 4:
        accountState = param->account->unk5;
        if (accountState == 1 || accountState == 2) {
            *seq = 5;
        } else {
            *seq = 13;
        }
        break;
    case 5:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 14;
        WifiBattleMatchTask_SetReturnSeq(task, 6);
        break;
    case 6: {
        BattleBoxSave *battleBox = getBattleBox(GameData_GetSaveControl(param->param->gameData));

        link->hadBattleBox = BattleBoxSave_CheckFlags(battleBox, 1);
        BattleBoxSave_ClearFlags(battleBox, 1);
        TournamentEntry_SetField(link->entry, 10, 3);
        func_0201782c(param->param->gameData);
        *seq = 7;
        break;
    }
    case 7:
        if (func_02017850(param->param->gameData) == 2) {
            *seq = 8;
        }
        break;
    case 8:
        getBattleBox(GameData_GetSaveControl(param->param->gameData));
        if (link->hadBattleBox != 0) {
            *seq = 9;
        } else {
            *seq = 10;
        }
        break;
    case 9:
        WifiBattleMatchLink_CompetitionSetCardPage(link, 1);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_BattleBoxHasBeen,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 14;
        WifiBattleMatchTask_SetReturnSeq(task, 10);
        break;
    case 10:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 14;
        WifiBattleMatchTask_SetReturnSeq(task, 11);
        break;
    case 11:
        sys_memset(&request, 0, sizeof(request));
        request.unk1 = 3;
        request.unk4 = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->unk5 = 3;
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = 12;
        break;
    case 12:
        ret = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ret == 1) {
            *seq = 13;
        } else if (ret == 3) {
            link->subTaskResult = 3;
            *seq = 13;
        }
        break;
    case 13:
        WifiBattleMatchTask_End(task);
        break;
    case 14:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskUnregister(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchLinkRequest request;
    u8 accountState;
    u32 ret;
    s32 selected;

    switch (*seq) {
    case 0:
        accountState = param->account->unk5;
        if (accountState == 1 || accountState == 2) {
            link->subTaskResult = 1;
            *seq = 2;
        } else {
            link->subTaskResult = 2;
            *seq = 1;
        }
        break;
    case 1:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CurrentlyHaveRegisteredNo,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 16;
        WifiBattleMatchTask_SetReturnSeq(task, 15);
        break;
    case 2:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_OnceUnregisteredWillNot,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 16;
        WifiBattleMatchTask_SetReturnSeq(task, 3);
        break;
    case 3:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 5);
        *seq = 4;
        break;
    case 4:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 5;
            break;
        case 1:
            *seq = 15;
            break;
        }
        break;
    case 5:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WillUnregisteredSureAbout,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 16;
        WifiBattleMatchTask_SetReturnSeq(task, 6);
        break;
    case 6:
        WifiBattleMatchLink_CompetitionCreateMenu(link, 5);
        *seq = 7;
        break;
    case 7:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case 0:
            *seq = 8;
            break;
        case 1:
            *seq = 15;
            break;
        }
        break;
    case 8:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 16;
        WifiBattleMatchTask_SetReturnSeq(task, 9);
        break;
    case 9:
        BattleBoxSave_ClearFlags(getBattleBox(GameData_GetSaveControl(param->param->gameData)), 1);
        TournamentEntry_SetField(link->entry, 10, 4);
        func_0201782c(param->param->gameData);
        *seq = 10;
        break;
    case 10:
        ret = func_02017850(param->param->gameData);
        if (ret == 2) {
            link->subTaskResult = 0;
            *seq = 11;
        } else if (ret == 3) {
            *seq = 15;
        }
        break;
    case 11:
        WifiBattleMatchLink_CompetitionSetCardPage(link, 1);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_BattleBoxHasBeen,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = 16;
        WifiBattleMatchTask_SetReturnSeq(task, 12);
        break;
    case 12:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = 16;
        WifiBattleMatchTask_SetReturnSeq(task, 13);
        break;
    case 13:
        sys_memset(&request, 0, sizeof(request));
        request.unk1 = 4;
        request.unk4 = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->unk5 = 4;
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = 14;
        break;
    case 14:
        ret = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ret == 1) {
            *seq = 15;
        } else if (ret == 3) {
            link->subTaskResult = 3;
            *seq = 15;
        }
        break;
    case 15:
        WifiBattleMatchTask_End(task);
        break;
    case 16:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskSignTeam(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkPlayer *player = link->param->player;
    u32 ret;

    switch (*seq) {
    case 0:
        *seq = 1;
        break;
    case 1:
        sys_memset(&link->signature, 0, sizeof(link->signature));
        WifiBattleMatchNet_StartPokemonCheck(link->net, player->partyData, TRUE);
        *seq = 2;
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
            WifiBattleMatchLink_CompetitionCopySignature(player, &link->signature);
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

static void WifiBattleMatchLink_CompetitionCreateCard(WifiBattleMatchLinkCompetition *link) {
    WifiBattleMatchWifiCardInfo info;
    PlayerInfo *playerInfo;
    ClActUnit *unit;
    void *previous;
    void *entry;
    u32 first;

    if (link->card != NULL) {
        return;
    }
    entry = link->entry;
    playerInfo = GetGameDataPlayerInfo(link->param->param->gameData);
    unit = WifiBattleMatchGraphic_GetClActUnit(link->graphic);
    previous = TournamentBlock_GetTeam(getTournamentBlockAddress(GameData_GetSaveControl(link->param->param->gameData)), 0);
    sys_memset(&info, 0, sizeof(info));
    sys_memcpy(func_0200b794(entry), &info, 0x92);
    info.startDate =
        WifiBattleMatchLink_PackBytes(TournamentEntry_GetField(entry, 4), TournamentEntry_GetField(entry, 5), TournamentEntry_GetField(entry, 6));
    info.endDate =
        WifiBattleMatchLink_PackBytes(TournamentEntry_GetField(entry, 7), TournamentEntry_GetField(entry, 8), TournamentEntry_GetField(entry, 9));
    info.look = func_02008bf4(playerInfo);
    info.unkA0 = RndMatchData_GetParam(link->param->record, 10, 0);
    first = RndMatchData_GetParam(link->param->record, 10, 1);
    info.unkA4 = first + RndMatchData_GetParam(link->param->record, 10, 2);
    info.variant = TournamentEntry_GetField(entry, 11);
    if (info.unkA4 == 0 && info.unkA0 == 0) {
        info.unkA0 = 1500;
    }
    link->card =
        WifiBattleMatchWifiCard_Create(&info, 0, playerInfo, unit, link->res, link->font, link->printQueue,
                                       link->msgData, link->wordSet, previous, FALSE, HEAPID_WIFIBATTLEMATCH_LINK);
}

static void WifiBattleMatchLink_CompetitionDeleteCard(WifiBattleMatchLinkCompetition *link) {
    if (link->card != NULL) {
        WifiBattleMatchWifiCard_Free(link->card);
        link->card = NULL;
    }
}

static BOOL WifiBattleMatchLink_CompetitionIsCardShown(WifiBattleMatchLinkCompetition *link) {
    return WifiBattleMatchCard_SlideIn(link->card);
}

static BOOL WifiBattleMatchLink_CompetitionIsCardHidden(WifiBattleMatchLinkCompetition *link) {
    return WifiBattleMatchCard_SlideOut(link->card);
}

static void WifiBattleMatchLink_CompetitionShowCard(WifiBattleMatchLinkCompetition *link) {
    WifiBattleMatchLink_CompetitionCreateCard(link);
    while (!WifiBattleMatchLink_CompetitionIsCardShown(link)) {
    }
}

static void WifiBattleMatchLink_CompetitionSetCardPage(WifiBattleMatchLinkCompetition *link, u32 page) {
    if (link->card != NULL) {
        WifiBattleMatchWifiCard_ShowPage(link->card, page, link->msgData, link->printQueue, link->font,
                                         HEAPID_WIFIBATTLEMATCH_LINK);
    }
}

static void WifiBattleMatchLink_CompetitionCreatePartnerCard(WifiBattleMatchLinkCompetition *link,
                                                             WifiBattleMatchLinkPlayer *partner) {
    ClActUnit *unit;

    if (link->partnerCard == NULL) {
        unit = WifiBattleMatchGraphic_GetClActUnit(link->graphic);
        link->partnerCard = WifiBattleMatchPartnerCard_Create(
            (const LiveBattleMatchPlayerData *)partner, unit, link->res, link->font, link->printQueue, link->msgData,
            link->wordSet, 0, 1, TournamentEntry_GetField(link->entry, 11), HEAPID_WIFIBATTLEMATCH_LINK);
    }
}

static void WifiBattleMatchLink_CompetitionDeletePartnerCard(WifiBattleMatchLinkCompetition *link) {
    if (link->partnerCard != NULL) {
        WifiBattleMatchPartnerCard_Free(link->partnerCard);
        link->partnerCard = NULL;
    }
}

static BOOL WifiBattleMatchLink_CompetitionIsPartnerCardDone(WifiBattleMatchLinkCompetition *link) {
    return WifiBattleMatchPartnerCard_SlideIn(link->partnerCard);
}

static void WifiBattleMatchLink_CompetitionCreateTeamList(WifiBattleMatchLinkCompetition *link) {
    ClActUnit *unit;

    if (link->teamList == NULL) {
        unit = WifiBattleMatchGraphic_GetClActUnit(link->graphic);
        link->teamList = WifiBattleMatchTeamList_Create(
            getBattleBox(GameData_GetSaveControl(link->param->param->gameData)), unit, link->res, link->smallFont,
            link->printQueue, link->msgData, HEAPID_WIFIBATTLEMATCH_LINK);
    }
}

static void WifiBattleMatchLink_CompetitionDeleteTeamList(WifiBattleMatchLinkCompetition *link) {
    if (link->teamList != NULL) {
        WifiBattleMatchTeamList_Free(link->teamList);
        link->teamList = NULL;
    }
}

static BOOL WifiBattleMatchLink_CompetitionIsTeamListShown(WifiBattleMatchLinkCompetition *link) {
    return WifiBattleMatchTeamList_SlideIn(link->teamList);
}

static BOOL WifiBattleMatchLink_CompetitionIsTeamListHidden(WifiBattleMatchLinkCompetition *link) {
    return WifiBattleMatchTeamList_SlideOut(link->teamList);
}

static void WifiBattleMatchLink_CompetitionCreateMenu(WifiBattleMatchLinkCompetition *link, u32 kind) {
    WifiBattleMatchMenuParam menuParam;

    if (link->menu != NULL) {
        return;
    }
    sys_memset(&menuParam, 0, sizeof(menuParam));
    menuParam.msgData = link->msgData;
    menuParam.font = link->font;
    menuParam.printQueue = link->printQueue;
    menuParam.bg = 0;
    menuParam.palette = 15;
    menuParam.framePalette = 13;
    menuParam.frameChar = 1;
    switch (kind) {
    case 5:
        menuParam.cursorRow = 1;
        // fall through
    case 0:
        menuParam.msgIds[0] = EventFishing_Text_Yes_2;
        menuParam.msgIds[1] = EventFishing_Text_No_2;
        menuParam.count = 2;
        menuParam.canCancel = TRUE;
        menuParam.cancelValue = 1;
        link->menu = WifiBattleMatchMenu_CreateBottomRight(&menuParam, HEAPID_WIFIBATTLEMATCH_LINK);
        break;
    case 1:
        menuParam.msgIds[0] = EventFishing_Text_Join;
        menuParam.msgIds[1] = EventFishing_Text_Info;
        menuParam.msgIds[2] = EventFishing_Text_Cancel;
        menuParam.count = 3;
        menuParam.canCancel = TRUE;
        menuParam.cancelValue = 2;
        link->menu = WifiBattleMatchMenu_CreateBottomRight(&menuParam, HEAPID_WIFIBATTLEMATCH_LINK);
        break;
    case 2:
        menuParam.msgIds[0] = EventFishing_Text_Yes_3;
        menuParam.msgIds[1] = EventFishing_Text_No_3;
        menuParam.count = 2;
        menuParam.canCancel = TRUE;
        menuParam.cancelValue = 1;
        link->menu = WifiBattleMatchMenu_CreateBottomRight(&menuParam, HEAPID_WIFIBATTLEMATCH_LINK);
        break;
    case 3:
        menuParam.msgIds[0] = EventFishing_Text_Battle;
        menuParam.msgIds[1] = EventFishing_Text_CancelParticipation;
        menuParam.msgIds[2] = EventFishing_Text_Cancel_2;
        menuParam.count = 3;
        menuParam.canCancel = TRUE;
        menuParam.cancelValue = 2;
        link->menu = WifiBattleMatchMenu_Create(&menuParam, 3, 6, 26, 6, HEAPID_WIFIBATTLEMATCH_LINK);
        break;
    case 4:
        menuParam.msgIds[0] = EventFishing_Text_Battle_2;
        menuParam.msgIds[1] = EventFishing_Text_CancelParticipation_2;
        menuParam.msgIds[2] = EventFishing_Text_Cancel_3;
        menuParam.count = 3;
        menuParam.canCancel = TRUE;
        menuParam.cancelValue = 2;
        link->menu = WifiBattleMatchMenu_Create(&menuParam, 3, 6, 26, 6, HEAPID_WIFIBATTLEMATCH_LINK);
        break;
    }
}

static void WifiBattleMatchLink_CompetitionDeleteMenu(WifiBattleMatchLinkCompetition *link) {
    if (link->menu != NULL) {
        WifiBattleMatchMenu_Free(link->menu);
        link->menu = NULL;
    }
}

static s32 WifiBattleMatchLink_CompetitionGetMenuSelection(WifiBattleMatchLinkCompetition *link) {
    if (link->menu != NULL) {
        return WifiBattleMatchMenu_Update(link->menu);
    }
    return -1;
}

static void WifiBattleMatchLink_CompetitionStartSubTask(WifiBattleMatchLinkCompetition *link,
                                                        WifiBattleMatchTaskFunc func) {
    WifiBattleMatchTask_SetFunc(link->subTask, func);
    link->subTaskResult = SUB_TASK_RUNNING;
}

static u32 WifiBattleMatchLink_CompetitionUpdateSubTask(WifiBattleMatchLinkCompetition *link) {
    WifiBattleMatchTask_Run(link->subTask);
    if (WifiBattleMatchTask_IsEnded(link->subTask)) {
        return link->subTaskResult;
    }
    return SUB_TASK_RUNNING;
}

static void WifiBattleMatchLink_CompetitionFillBattleBoxParty(WifiBattleMatchLinkPlayer *player,
                                                              WifiBattleMatchLinkCompetition *link) {
    PokeParty *party;

    sys_memset(player, 0, sizeof(WifiBattleMatchLinkPlayer));
    party = convertBoxedPokeSetToParty(getBattleBox(GameData_GetSaveControl(link->param->param->gameData)),
                                       HEAPID_WIFIBATTLEMATCH_LINK);
    PokeParty_Init((PokeParty *)player->partyData);
    sys_memcpy(party, player->partyData, PokeParty_GetSaveDataSize());
    GFL_HeapFree(party);
    player->unk44 = 100;
}

static void WifiBattleMatchLink_CompetitionFillPlayer(WifiBattleMatchLinkPlayer *player,
                                                      WifiBattleMatchLinkCompetition *link) {
    SaveControl *save = GameData_GetSaveControl(link->param->param->gameData);
    WifiBattleMatchLinkParam *param;
    void *entry;

    func_02008b34(GetGameDataPlayerInfo(link->param->param->gameData), &player->info);
    CGearPhrases_Get(CGearPhrases_GetBlock(save), 0, &player->sentence);
    param = link->param;
    player->record1 = param->unk34->unk8;
    player->record2 = param->unk34->unkC;
    player->unk34 = param->unk34->unk10;
    player->unk4C = WifiBattleMatchNet_GetRecordId(link->net);
    player->gsid = WifiList_GetMyGSID(GameData_GetWifiList(link->param->param->gameData));
    entry = link->entry;
    player->entryField2 = TournamentEntry_GetField(entry, 2);
    player->checksum = CRC16_350bytes(entry);
}

static void WifiBattleMatchLink_CompetitionCopySignature(WifiBattleMatchLinkPlayer *player,
                                                         WifiBattleMatchNetCheck *signature) {
    sys_memcpy(signature->signature, player->signature, sizeof(player->signature));
}

static BOOL WifiBattleMatchLink_CompetitionCheckSignature(WifiBattleMatchLinkPlayer *player, PokeParty *party,
                                                          HeapID heapId) {
    void *array;
    int i;
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

static BOOL WifiBattleMatchLink_CompetitionCompareParties(PokeParty *party, PokeParty *otherParty) {
    u32 count = PokeParty_GetPkmCount(party);
    u32 i;

    if (count == PokeParty_GetPkmCount(otherParty)) {
        for (i = 0; i < count; i++) {
            PartyPkm *pkm = PokeParty_GetPkm(party, i);
            PartyPkm *otherPkm = PokeParty_GetPkm(otherParty, i);

            if (GFL_STD_MemCmp(func_0201d620(pkm), func_0201d620(otherPkm), PML_GetPkmRawSize()) != 0) {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

static u32 WifiBattleMatchLink_CompetitionUpdateCancel(WifiBattleMatchLinkCompetition *link, BOOL canCancel,
                                                       BOOL canDisconnect) {
    s32 selected;

    switch (link->cancelSeq) {
    case 0:
        if (canCancel && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            link->cancelSeq++;
        }
        break;
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
        WifiBattleMatchLink_CompetitionCreateMenu(link, 0);
        link->cancelSeq++;
        break;
    case 4:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == -1) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
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
    return link->cancelSeq == 0 ? CANCEL_NONE : CANCEL_BUSY;
}

static void WifiBattleMatchLink_CompetitionResetCancel(WifiBattleMatchLinkCompetition *link) {
    link->cancelSeq = 0;
    WifiBattleMatchLink_CompetitionDeleteMenu(link);
}

static BOOL WifiBattleMatchLink_CompetitionUpdateRecord(WifiBattleMatchLinkCompetition *link,
                                                        WifiBattleMatchLinkRanking *ranking) {
    BOOL changed = FALSE;

    if (ranking->unk10 != RndMatchData_GetParam(link->param->record, 10, 0) ||
        ranking->unk8 != RndMatchData_GetParam(link->param->record, 10, 1) ||
        ranking->unkC != RndMatchData_GetParam(link->param->record, 10, 2)) {
        changed = TRUE;
    }
    RndMatchData_SetParam(link->param->record, 10, 0, ranking->unk10);
    RndMatchData_SetParam(link->param->record, 10, 1, ranking->unk8);
    RndMatchData_SetParam(link->param->record, 10, 2, ranking->unkC);
    return changed;
}
