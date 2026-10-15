#include "app/battle_championship_flow.h"
#include "types.h"
#include "app/digitalcardcheck.h"
#include "app/livebattlematch_flow.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "save/box.h"
#include "save/bsubway_save.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/tournament_block.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/script/global_10520.h"
#include "text/system/entree_forest.h"

#define HEAPID_BATTLE_CHAMPIONSHIP_FLOW 0x4e

// What the menu's list holds
enum {
    LIST_TOP,
    LIST_WIFI,
    LIST_LIVE_MENU,
    LIST_LIVE,
};

// What the menu is for
enum {
    MENU_MODE_TOP,
    MENU_MODE_REWARD,
    MENU_MODE_LIVE,
};

// What the menu ends with
enum {
    MENU_RESULT_NONE,
    MENU_RESULT_EXIT,
    MENU_RESULT_WIFI,
    MENU_RESULT_LIVE,
};

// What the live competition's flow ends with
enum {
    LIVE_RESULT_NONE,
    LIVE_RESULT_MENU,
};

typedef struct {
    u16 heapId;
    WifiBattleMatchTask *task;
    WifiBattleMatchMsgBox *msgBox;
    Font *font;
    // The Entree Forest's messages
    MsgData *msgData;
    PrintQueue *printQueue;
    u32 unk18;
    u32 unk1C;
    WifiBattleMatchRes *seq;
    WifiBattleMatchGraphic *net;
    BattleChampionshipFlowParam *param;
    // The menu or the live competition's flow, whichever is running
    void *flow;
    // Set when the live competition sends the player back to the menu
    u32 backToMenu;
} BattleChampionshipFlow;

// What the menu is given
typedef struct {
    u32 mode;
    WifiBattleMatchMsgBox *msgBox;
    Font *font;
    MsgData *msgData;
    PrintQueue *printQueue;
    WifiBattleMatchRes *seq;
    WifiBattleMatchGraphic *net;
    GameData *gameData;
} BattleChampionshipMenuParam;

typedef struct {
    WifiBattleMatchTask *task;
    WifiBattleMatchMenu *list;
    u32 listKind;
    // Global script messages
    MsgData *scriptMsgData;
    u32 result;
    BattleChampionshipMenuParam param;
    u16 heapId;
    WordSet *wordSet;
    StrBuf *strBuf;
    DigitalCardCheck *cardCheck;
} BattleChampionshipMenu;

static BOOL BattleChampionshipFlow_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BattleChampionshipFlow_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BattleChampionshipFlow_Main(GameProc *proc, u32 *state, void *param, void *work);
static void BattleChampionshipFlow_CreateNet(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_DeleteNet(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_UpdateNet(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_CreateSeq(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_DeleteSeq(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_CreateUI(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_DeleteUI(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_UpdateUI(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_TaskDispatch(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_TaskFadeIn(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_TaskFadeOut(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_TaskMenu(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_TaskLive(WifiBattleMatchTask *task, u32 *seq, void *work);
static BattleChampionshipMenu *BattleChampionshipFlow_MenuCreate(const BattleChampionshipMenuParam *param,
                                                                 HeapID heapId);
static void BattleChampionshipFlow_MenuDelete(BattleChampionshipMenu *menu);
static void BattleChampionshipFlow_MenuUpdate(BattleChampionshipMenu *menu);
static u32 BattleChampionshipFlow_MenuGetResult(BattleChampionshipMenu *menu);
static void BattleChampionshipFlow_MenuTaskStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskTop(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskIntro(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskWifi(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskWifiInfo(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskWifiCard(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskLiveCard(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskLive(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskLiveInfo(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskNoWireless(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskWifiEntry(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskReward(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuOpenList(BattleChampionshipMenu *menu, u32 kind);
static void BattleChampionshipFlow_MenuCloseList(BattleChampionshipMenu *menu);
static s32 BattleChampionshipFlow_MenuGetListChoice(BattleChampionshipMenu *menu);
static void BattleChampionshipFlow_MenuPrint(BattleChampionshipMenu *menu, u32 msgId);
static void BattleChampionshipFlow_MenuPrintScript(BattleChampionshipMenu *menu, u32 msgId);
static BOOL BattleChampionshipFlow_MenuIsPrinted(BattleChampionshipMenu *menu);
static void BattleChampionshipFlow_MenuFinish(BattleChampionshipMenu *menu, u32 result);

const GameProcFunctions BATTLE_CHAMPIONSHIP_FLOW_PROC_FUNCTIONS = {
    BattleChampionshipFlow_Init,
    BattleChampionshipFlow_Main,
    BattleChampionshipFlow_Exit,
};

static BOOL BattleChampionshipFlow_Init(GameProc *proc, u32 *state, void *param, void *work) {
    BattleChampionshipFlow *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_BATTLE_CHAMPIONSHIP_FLOW, 0x30200);
    wk = GFL_ProcInitSubsystem(proc, sizeof(BattleChampionshipFlow), HEAPID_BATTLE_CHAMPIONSHIP_FLOW);
    sys_memset(wk, 0, sizeof(BattleChampionshipFlow));
    wk->param = param;
    wk->heapId = HEAPID_BATTLE_CHAMPIONSHIP_FLOW;
    GFL_OvlLoad(OVERLAY_ID(139));
    GFL_OvlLoad(OVERLAY_ID(262));
    BattleChampionshipFlow_CreateNet(wk);
    BattleChampionshipFlow_CreateSeq(wk);
    BattleChampionshipFlow_CreateUI(wk);
    wk->task = WifiBattleMatchTask_Create(wk, BattleChampionshipFlow_TaskFadeIn, HEAPID_BATTLE_CHAMPIONSHIP_FLOW);
    return TRUE;
}

static BOOL BattleChampionshipFlow_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    BattleChampionshipFlow *wk = work;

    WifiBattleMatchTask_Free(wk->task);
    BattleChampionshipFlow_DeleteUI(wk);
    BattleChampionshipFlow_DeleteSeq(wk);
    BattleChampionshipFlow_DeleteNet(wk);
    GFL_OvlUnload(OVERLAY_ID(262));
    GFL_OvlUnload(OVERLAY_ID(139));
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_BATTLE_CHAMPIONSHIP_FLOW);
    return TRUE;
}

static BOOL BattleChampionshipFlow_Main(GameProc *proc, u32 *state, void *param, void *work) {
    BattleChampionshipFlow *wk = work;

    BattleChampionshipFlow_UpdateNet(wk);
    BattleChampionshipFlow_UpdateUI(wk);
    WifiBattleMatchTask_Run(wk->task);
    if (WifiBattleMatchTask_IsEnded(wk->task)) {
        return TRUE;
    }
    return FALSE;
}

static void BattleChampionshipFlow_CreateNet(BattleChampionshipFlow *wk) {
    wk->net = WifiBattleMatchGraphic_Create(TRUE, wk->heapId);
}

static void BattleChampionshipFlow_DeleteNet(BattleChampionshipFlow *wk) {
    WifiBattleMatchGraphic_Free(wk->net);
    wk->net = NULL;
}

static void BattleChampionshipFlow_UpdateNet(BattleChampionshipFlow *wk) {
    if (wk->net != NULL) {
        WifiBattleMatchGraphic_Update(wk->net);
        WifiBattleMatchRes_Update(wk->seq);
    }
}

static void BattleChampionshipFlow_CreateSeq(BattleChampionshipFlow *wk) {
    wk->seq = WifiBattleMatchRes_Create(WifiBattleMatchGraphic_GetClActUnit(wk->net), TRUE, wk->heapId);
}

static void BattleChampionshipFlow_DeleteSeq(BattleChampionshipFlow *wk) {
    WifiBattleMatchRes_Free(wk->seq);
}

static void BattleChampionshipFlow_CreateUI(BattleChampionshipFlow *wk) {
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ENTREE_FOREST, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    wk->msgBox = WifiBattleMatchMsgBox_Create(0, 15, 14, 10, wk->printQueue, wk->font, wk->heapId);
}

static void BattleChampionshipFlow_DeleteUI(BattleChampionshipFlow *wk) {
    WifiBattleMatchMsgBox_Free(wk->msgBox);
    func_02021a18(wk->printQueue);
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->font);
}

static void BattleChampionshipFlow_UpdateUI(BattleChampionshipFlow *wk) {
    WifiBattleMatchMsgBox_Update(wk->msgBox);
    func_02021a3c(wk->printQueue);
}

static void BattleChampionshipFlow_TaskDispatch(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipFlow *wk = work;

    switch (wk->param->mode) {
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_MENU:
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_REWARD:
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_MENU_2:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskMenu);
        break;
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_JOIN:
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_AFTER_MATCH:
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_QUIT:
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_MENU:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskLive);
        break;
    }
}

static void BattleChampionshipFlow_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchTask_End(task);
}

static void BattleChampionshipFlow_TaskFadeIn(WifiBattleMatchTask *task, u32 *seq, void *work) {
    switch (*seq) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        *seq = 1;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            *seq = 2;
        }
        break;
    case 2:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskDispatch);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
}

static void BattleChampionshipFlow_TaskFadeOut(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipFlow *wk = work;
    u32 mode;

    switch (*seq) {
    case 0:
        if (wk->param->result == BATTLE_CHAMPIONSHIP_FLOW_RESULT_EXIT) {
            GFL_SndBGMFadeOut(6);
            mode = FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE;
        } else {
            mode = FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK;
        }
        GFL_FadeSet(mode, 0, 16, 0);
        *seq = 1;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            *seq = 2;
        }
        break;
    case 2:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskEnd);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
}

static void BattleChampionshipFlow_TaskMenu(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipFlow *wk = work;
    BattleChampionshipMenuParam param;

    switch (*seq) {
    case 0:
        sys_memset(&param, 0, sizeof(BattleChampionshipMenuParam));
        param.msgBox = wk->msgBox;
        param.font = wk->font;
        param.msgData = wk->msgData;
        param.printQueue = wk->printQueue;
        param.seq = wk->seq;
        param.net = wk->net;
        param.gameData = wk->param->gameData;
        if (wk->backToMenu != 0) {
            wk->backToMenu = 0;
            param.mode = MENU_MODE_LIVE;
        } else {
            switch (wk->param->mode) {
            case BATTLE_CHAMPIONSHIP_FLOW_MODE_MENU:
                param.mode = MENU_MODE_TOP;
                break;
            case BATTLE_CHAMPIONSHIP_FLOW_MODE_REWARD:
                param.mode = MENU_MODE_REWARD;
                break;
            }
        }
        wk->flow = BattleChampionshipFlow_MenuCreate(&param, HEAPID_BATTLE_CHAMPIONSHIP_FLOW);
        (*seq)++;
        break;
    case 1:
        BattleChampionshipFlow_MenuUpdate(wk->flow);
        switch (BattleChampionshipFlow_MenuGetResult(wk->flow)) {
        case MENU_RESULT_EXIT:
            wk->param->result = BATTLE_CHAMPIONSHIP_FLOW_RESULT_EXIT;
            *seq = 2;
            break;
        case MENU_RESULT_WIFI:
            wk->param->result = BATTLE_CHAMPIONSHIP_FLOW_RESULT_WIFI;
            *seq = 2;
            break;
        case MENU_RESULT_LIVE:
            wk->param->mode = BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_JOIN;
            *seq = 3;
            break;
        }
        break;
    case 2:
        BattleChampionshipFlow_MenuDelete(wk->flow);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskFadeOut);
        break;
    case 3:
        BattleChampionshipFlow_MenuDelete(wk->flow);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskLive);
        break;
    }
}

static void BattleChampionshipFlow_TaskLive(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipFlow *wk = work;
    LiveBattleMatchFlowParam param;

    switch (*seq) {
    case 0:
        sys_memset(&param, 0, sizeof(LiveBattleMatchFlowParam));
        param.msgBox = wk->msgBox;
        param.font = wk->font;
        param.printQueue = wk->printQueue;
        param.seq = wk->seq;
        param.net = wk->net;
        param.gameData = wk->param->gameData;
        param.playerData = wk->param->playerData;
        param.partnerData = wk->param->partnerData;
        param.matchResult = wk->param->matchResult;
        switch (wk->param->mode) {
        case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_JOIN:
            param.mode = 0;
            break;
        case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_AFTER_MATCH:
            param.mode = 2;
            break;
        case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_QUIT:
            param.mode = 3;
            break;
        case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_MENU:
            param.mode = 1;
            break;
        default:
            param.mode = 0;
            break;
        }
        wk->flow = LiveBattleMatchFlow_Create(&param, HEAPID_BATTLE_CHAMPIONSHIP_FLOW);
        (*seq)++;
        break;
    case 1:
        LiveBattleMatchFlow_Main(wk->flow);
        switch (LiveBattleMatchFlow_GetResult(wk->flow)) {
        case LIVE_RESULT_MENU:
            *seq = 3;
            break;
        case 2:
            wk->param->result = 2;
            *seq = 2;
            break;
        case 3:
            wk->param->result = 3;
            *seq = 2;
            break;
        case 4:
            wk->param->result = 4;
            *seq = 2;
            break;
        }
        break;
    case 2:
        LiveBattleMatchFlow_Delete(wk->flow);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskFadeOut);
        break;
    case 3:
        wk->backToMenu = 1;
        LiveBattleMatchFlow_Delete(wk->flow);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskMenu);
        break;
    }
}

static BattleChampionshipMenu *BattleChampionshipFlow_MenuCreate(const BattleChampionshipMenuParam *param,
                                                                 HeapID heapId) {
    BattleChampionshipMenu *menu =
        GFL_HeapAllocate(heapId, sizeof(BattleChampionshipMenu), FALSE, "battle_championship_flow.c", 185);

    sys_memset(menu, 0, sizeof(BattleChampionshipMenu));
    menu->param = *param;
    menu->heapId = heapId;
    menu->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    menu->strBuf = GFL_StrBufCreate(128, heapId);
    menu->scriptMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520, heapId);
    menu->task = WifiBattleMatchTask_Create(menu, BattleChampionshipFlow_MenuTaskStart, heapId);
    return menu;
}

static void BattleChampionshipFlow_MenuDelete(BattleChampionshipMenu *menu) {
    WifiBattleMatchTask_Free(menu->task);
    GFL_MsgDataFree(menu->scriptMsgData);
    GFL_WordSetSystemFree(menu->wordSet);
    GFL_StrBufFree(menu->strBuf);
    GFL_HeapFree(menu);
}

static void BattleChampionshipFlow_MenuUpdate(BattleChampionshipMenu *menu) {
    WifiBattleMatchTask_Run(menu->task);
}

static u32 BattleChampionshipFlow_MenuGetResult(BattleChampionshipMenu *menu) {
    if (WifiBattleMatchTask_IsEnded(menu->task)) {
        return menu->result;
    }
    return MENU_RESULT_NONE;
}

static void BattleChampionshipFlow_MenuTaskStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;

    switch (menu->param.mode) {
    case MENU_MODE_TOP:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
        break;
    case MENU_MODE_REWARD:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskReward);
        break;
    case MENU_MODE_LIVE:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLive);
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchTask_End(task);
}

static void BattleChampionshipFlow_MenuTaskTop(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    s32 choice;

    switch (*seq) {
    case 0:
        BattleChampionshipFlow_MenuPrint(menu, EntreeForest_Text_WelcomeBattleCompetition);
        *seq = 1;
        break;
    case 1:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            *seq = 2;
        }
        break;
    case 2:
        BattleChampionshipFlow_MenuOpenList(menu, LIST_TOP);
        *seq = 3;
        break;
    case 3:
        choice = BattleChampionshipFlow_MenuGetListChoice(menu);
        if (choice != -1) {
            switch (choice) {
            case 0:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifi);
                break;
            case 1:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLive);
                break;
            case 2:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskIntro);
                break;
            case 3:
                BattleChampionshipFlow_MenuFinish(menu, MENU_RESULT_EXIT);
                break;
            }
            BattleChampionshipFlow_MenuCloseList(menu);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskIntro(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;

    switch (*seq) {
    case 0:
        BattleChampionshipFlow_MenuPrint(menu, EntreeForest_Text_ThereTwoKindsBattle);
        *seq = 1;
        break;
    case 1:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskWifi(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    s32 choice;

    switch (*seq) {
    case 0:
        BattleChampionshipFlow_MenuPrint(menu, EntreeForest_Text_WillParticipateWiFi);
        *seq = 1;
        break;
    case 1:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            *seq = 2;
        }
        break;
    case 2:
        BattleChampionshipFlow_MenuOpenList(menu, LIST_WIFI);
        *seq = 3;
        break;
    case 3:
        choice = BattleChampionshipFlow_MenuGetListChoice(menu);
        if (choice != -1) {
            switch (choice) {
            case 0:
                if (!isWirelessEnabled()) {
                    WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskNoWireless);
                } else {
                    WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifiEntry);
                }
                break;
            case 1:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifiInfo);
                break;
            case 2:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifiCard);
                break;
            case 3:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
                break;
            }
            BattleChampionshipFlow_MenuCloseList(menu);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskWifiInfo(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;

    switch (*seq) {
    case 0:
        BattleChampionshipFlow_MenuPrint(menu, EntreeForest_Text_ParticipateWiFiCompetition);
        *seq = 1;
        break;
    case 1:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifi);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskWifiCard(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    DigitalCardCheckParam param;

    switch (*seq) {
    case 0:
        sys_memset(&param, 0, sizeof(DigitalCardCheckParam));
        param.mode = DIGITAL_CARD_CHECK_WIFI;
        param.msgBox = menu->param.msgBox;
        param.font = menu->param.font;
        param.msgData = menu->param.msgData;
        param.printQueue = menu->param.printQueue;
        param.seq = menu->param.seq;
        param.net = menu->param.net;
        param.gameData = menu->param.gameData;
        menu->cardCheck = DigitalCardCheck_Create(&param, menu->heapId);
        (*seq)++;
        break;
    case 1:
        DigitalCardCheck_Update(menu->cardCheck);
        if (DigitalCardCheck_IsFinished(menu->cardCheck)) {
            (*seq)++;
        }
        break;
    case 2:
        DigitalCardCheck_Delete(menu->cardCheck);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifi);
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskLiveCard(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    DigitalCardCheckParam param;

    switch (*seq) {
    case 0:
        sys_memset(&param, 0, sizeof(DigitalCardCheckParam));
        param.mode = DIGITAL_CARD_CHECK_LIVE;
        param.msgBox = menu->param.msgBox;
        param.font = menu->param.font;
        param.msgData = menu->param.msgData;
        param.printQueue = menu->param.printQueue;
        param.seq = menu->param.seq;
        param.net = menu->param.net;
        param.gameData = menu->param.gameData;
        menu->cardCheck = DigitalCardCheck_Create(&param, menu->heapId);
        (*seq)++;
        break;
    case 1:
        DigitalCardCheck_Update(menu->cardCheck);
        if (DigitalCardCheck_IsFinished(menu->cardCheck)) {
            (*seq)++;
        }
        break;
    case 2:
        DigitalCardCheck_Delete(menu->cardCheck);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLive);
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskLive(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    s32 choice;

    switch (*seq) {
    case 0:
        BattleChampionshipFlow_MenuPrint(menu, EntreeForest_Text_WillParticipateLiveCompetition);
        *seq = 1;
        break;
    case 1:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            *seq = 2;
        }
        break;
    case 2:
        BattleChampionshipFlow_MenuOpenList(menu, LIST_LIVE_MENU);
        *seq = 3;
        break;
    case 3:
        choice = BattleChampionshipFlow_MenuGetListChoice(menu);
        if (choice != -1) {
            switch (choice) {
            case 0:
                if (!isWirelessEnabled()) {
                    WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskNoWireless);
                } else {
                    clearVSPlayer();
                    BattleChampionshipFlow_MenuFinish(menu, MENU_RESULT_LIVE);
                }
                break;
            case 1:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLiveInfo);
                break;
            case 2:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLiveCard);
                break;
            case 3:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
                break;
            }
            BattleChampionshipFlow_MenuCloseList(menu);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskLiveInfo(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;

    switch (*seq) {
    case 0:
        BattleChampionshipFlow_MenuPrint(menu, EntreeForest_Text_CanCheckInformationRegarding);
        *seq = 1;
        break;
    case 1:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLive);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskNoWireless(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;

    switch (*seq) {
    case 0:
        BattleChampionshipFlow_MenuPrintScript(menu, Global10520_Text_WirelessCommunicationsTurnedOff);
        *seq = 1;
        break;
    case 1:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskWifiEntry(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    SaveControl *save = GameData_GetSaveControl(menu->param.gameData);
    RecordSave *record = getRecordBlkAddress(save);

    switch (*seq) {
    case 0:
        if (func_0200f300(record)) {
            if (func_0200f308(record)) {
                if (func_0200f334(record)) {
                    RecordAddOne(getTrainerCardInfoBlkAddress(save), 0x35);
                    *seq = 1;
                } else {
                    *seq = 4;
                }
            } else {
                func_0200f2dc(record);
                *seq = 2;
            }
        } else {
            *seq = 1;
        }
        break;
    case 1:
        func_0200f2dc(record);
        WifiBattleMatchTask_SetReturnSeq(task, 9);
        *seq = 6;
        break;
    case 2:
        BattleChampionshipFlow_MenuPrint(menu, EntreeForest_Text_SinceSystemTurnedOff);
        *seq = 3;
        break;
    case 3:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            WifiBattleMatchTask_SetReturnSeq(task, 10);
            *seq = 6;
        }
        break;
    case 4:
        BattleChampionshipFlow_MenuPrint(menu, EntreeForest_Text_SinceSystemTurnedOff);
        *seq = 5;
        break;
    case 5:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            *seq = 10;
        }
        break;
    case 6:
        WifiBattleMatchMsgBox_Print(menu->param.msgBox, menu->param.msgData, EntreeForest_Text_SavingDontTurnOff, 2);
        *seq = 7;
        break;
    case 7:
        GCTX_HIDBlockSoftReset(0x40);
        func_0201782c(menu->param.gameData);
        *seq = 8;
        break;
    case 8:
        switch (func_02017850(menu->param.gameData)) {
        case 2:
        case 3:
            WifiBattleMatchTask_ReturnToSeq(task);
            break;
        }
        break;
    case 9:
        clearVSPlayer();
        BattleChampionshipFlow_MenuFinish(menu, MENU_RESULT_WIFI);
        break;
    case 10:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskReward(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    RecordSave *record = getRecordBlkAddress(GameData_GetSaveControl(menu->param.gameData));
    StrBuf *text;
    BSubwayScoreData *score;
    u8 points;

    switch (*seq) {
    case 0:
        RecordSave_ClearMatchInProgress(record);
        if (func_02017b8c(menu->param.gameData) == 0) {
            *seq = 6;
        } else {
            *seq = 1;
        }
        break;
    case 1:
        WifiBattleMatchMsgBox_Print(menu->param.msgBox, menu->param.msgData,
                            EntreeForest_Text_RecognizeAchievementBattlePresent, 1);
        *seq = 2;
        break;
    case 2:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            *seq = 3;
        }
        break;
    case 3:
        GFL_SndBGMPush();
        GFL_SndBGMPlay(SEQ_ME_BPGET, SND_CHANNEL_MASK_ALL);
        text = GFL_MsgDataLoadStrbufNew(menu->param.msgData, EntreeForest_Text_ReceivedBp);
        copyVarForText(menu->wordSet, 0, GetGameDataPlayerInfo(menu->param.gameData));
        WordSetNumber(menu->wordSet, 1, func_02017b8c(menu->param.gameData), 2, 0, 1);
        GFL_WordSetFormatStrbuf(menu->wordSet, menu->strBuf, text);
        GFL_StrBufFree(text);
        WifiBattleMatchMsgBox_PrintStr(menu->param.msgBox, menu->strBuf, 3);
        *seq = 4;
        break;
    case 4:
        if (!GFL_SndBGMIsPlaying()) {
            GFL_SndBGMPop();
            WifiBattleMatchMsgBox_EnableKeyCursor(menu->param.msgBox);
            *seq = 5;
        }
        break;
    case 5:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            score = func_0201795c(menu->param.gameData);
            points = func_02017b8c(menu->param.gameData);
            BSubwayScoreData_AddBattlePoints(score, points);
            func_02017bb4(menu->param.gameData);
            if (points != 0) {
                RecordAdd(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(menu->param.gameData)), 0x21, points);
            }
            *seq = 6;
        }
        break;
    case 6:
        WifiBattleMatchMsgBox_Print(menu->param.msgBox, menu->param.msgData, EntreeForest_Text_SavingDontTurnOff, 2);
        *seq = 7;
        break;
    case 7:
        func_0201782c(menu->param.gameData);
        *seq = 8;
        break;
    case 8:
        switch (func_02017850(menu->param.gameData)) {
        case 2:
        case 3:
            GCTX_HIDUnblockSoftReset(0x40);
            *seq = 9;
            break;
        }
        break;
    case 9:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifi);
        break;
    }
}

static void BattleChampionshipFlow_MenuOpenList(BattleChampionshipMenu *menu, u32 kind) {
    u32 layout;
    void *tournament = getTournamentBlockAddress(GameData_GetSaveControl(menu->param.gameData));
    WifiBattleMatchMenuParam param;
    TournamentEntry *entry;
    u8 x, y, width, height;

    sys_memset(&param, 0, sizeof(WifiBattleMatchMenuParam));
    param.msgData = menu->param.msgData;
    param.font = menu->param.font;
    param.printQueue = menu->param.printQueue;
    switch (kind) {
    case LIST_TOP:
        param.msgIds[0] = 2;
        param.msgIds[1] = 3;
        param.msgIds[2] = 5;
        param.msgIds[3] = 6;
        param.count = 4;
        param.canCancel = 1;
        param.cancelValue = 3;
        layout = 0;
        break;
    case LIST_WIFI:
        entry = TournamentBlock_GetEntry(tournament, 0);
        param.msgIds[0] = 7;
        param.msgIds[1] = 5;
        if (entry->unk15B == 1 || entry->unk15B == 2) {
            param.msgIds[2] = 13;
        } else {
            param.msgIds[2] = 4;
        }
        param.msgIds[3] = 6;
        param.count = 4;
        param.canCancel = 1;
        param.cancelValue = 3;
        layout = 0;
        break;
    case LIST_LIVE_MENU:
        entry = TournamentBlock_GetEntry(tournament, 1);
        param.msgIds[0] = 7;
        param.msgIds[1] = 5;
        if (entry->unk15B == 1 || entry->unk15B == 2) {
            param.msgIds[2] = 13;
        } else {
            param.msgIds[2] = 4;
        }
        param.msgIds[3] = 6;
        param.count = 4;
        param.canCancel = 1;
        param.cancelValue = 3;
        layout = 0;
        break;
    case LIST_LIVE:
        param.msgIds[0] = 11;
        param.msgIds[1] = 12;
        param.count = 2;
        param.canCancel = 1;
        param.cancelValue = 1;
        layout = 1;
        break;
    }
    param.bg = 0;
    param.palette = 15;
    param.framePalette = 13;
    param.frameChar = 1;
    switch (layout) {
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
    menu->list = WifiBattleMatchMenu_Create(&param, x, y, width, height, menu->heapId);
    menu->listKind = kind;
}

static void BattleChampionshipFlow_MenuCloseList(BattleChampionshipMenu *menu) {
    WifiBattleMatchMenu_Free(menu->list);
}

static s32 BattleChampionshipFlow_MenuGetListChoice(BattleChampionshipMenu *menu) {
    return WifiBattleMatchMenu_Update(menu->list);
}

static void BattleChampionshipFlow_MenuPrint(BattleChampionshipMenu *menu, u32 msgId) {
    WifiBattleMatchMsgBox_Print(menu->param.msgBox, menu->param.msgData, msgId, 1);
}

static void BattleChampionshipFlow_MenuPrintScript(BattleChampionshipMenu *menu, u32 msgId) {
    WifiBattleMatchMsgBox_Print(menu->param.msgBox, menu->scriptMsgData, msgId, 1);
}

static BOOL BattleChampionshipFlow_MenuIsPrinted(BattleChampionshipMenu *menu) {
    return WifiBattleMatchMsgBox_IsDone(menu->param.msgBox);
}

static void BattleChampionshipFlow_MenuFinish(BattleChampionshipMenu *menu, u32 result) {
    menu->result = result;
    WifiBattleMatchTask_SetFunc(menu->task, BattleChampionshipFlow_MenuTaskEnd);
}
