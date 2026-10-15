// The Battle Recorder's battle video search screen: the player chooses a battle mode, a Pokémon and whether the videos
// are of the player's own area, each on a page of the sub screen, with the choices on the main screen. The list
// screen then searches the Global Link with them. The name is a guess: the ROM gives none, and br_core.c runs it as
// BR_PROCID_BV_SEARCH

#include "types.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_bvsearch_proc.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_net.h"
#include "app/battle_recorder/br_pokesearch.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "constants/sound.h"
#include "field/unity_tower.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "pml/species_names.h"
#include "system/bmp_menuwork.h"
#include "system/bmp_oam.h"
#include "system/country_region.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The pages of the sub screen
enum {
    BR_BVSEARCH_PAGE_MENU,
    BR_BVSEARCH_PAGE_MODE,
    BR_BVSEARCH_PAGE_POKE,
    BR_BVSEARCH_PAGE_AREA,
    BR_BVSEARCH_PAGE_MAX,
};

// The next page when the screen ends instead
#define BR_BVSEARCH_PAGE_END (-1)

// The lines of the menu, which are the things to search by
enum {
    BR_BVSEARCH_MENU_MODE,
    BR_BVSEARCH_MENU_POKE,
    BR_BVSEARCH_MENU_AREA,
    BR_BVSEARCH_MENU_MAX,
};

// The menu's buttons
enum {
    BR_BVSEARCH_BTN_RETURN,
    BR_BVSEARCH_BTN_SEARCH,
    BR_BVSEARCH_BTN_MAX,
};

// The main screen's windows: a caption and the choice for each thing to search by
#define BR_BVSEARCH_MSGWIN_MAX (BR_BVSEARCH_MENU_MAX * 2)

#define BR_BVSEARCH_PRINT_COLOR 0x3da0

typedef struct {
    BrMsgWin *menuWin[BR_BVSEARCH_MENU_MAX];
    BrBtn *btn[BR_BVSEARCH_BTN_MAX];
    ListMenuOption *p_list_data;
    BrList *p_list;
    BrPokeSearch *p_search;
    BrMsgWin *msgWin[BR_BVSEARCH_MSGWIN_MAX];
    BrMsgWin *text;
    BrBallEffect *ballEff[2];
    BmpOamSys *bmpoam;
    PrintQueue *que;
    u32 page;
    int nextPage;
    HeapID heapId;
} BrBvSearchWork;

// A page of the sub screen. Each function returns TRUE once it is done
typedef BOOL (*BrBvSearchPageFunc)(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);

typedef struct {
    BrBvSearchPageFunc init;
    BrBvSearchPageFunc main;
    BrBvSearchPageFunc exit;
} BrBvSearchPageFuncs;

// Where a button is and its animation and message
typedef struct {
    u8 x;
    u8 y;
    u8 anmSeq;
    u16 msgID;
} BrBvSearchBtnData;

// Where a window is, in tiles, and its message
typedef struct {
    u8 x;
    u8 y;
    u8 w;
    u8 h;
    u32 msgID;
} BrBvSearchMsgWinData;

typedef struct {
    u32 left;
    u32 top;
    u32 right;
    u32 bottom;
} BrBvSearchRect;

static BOOL BrBvSearch_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBvSearch_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBvSearch_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBvSearch_MenuInit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static BOOL BrBvSearch_MenuExit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static BOOL BrBvSearch_MenuMain(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static BOOL BrBvSearch_ModeInit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static BOOL BrBvSearch_ModeExit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static BOOL BrBvSearch_ModeMain(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static BOOL BrBvSearch_PokeInit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static BOOL BrBvSearch_PokeExit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static BOOL BrBvSearch_PokeMain(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static BOOL BrBvSearch_AreaInit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static BOOL BrBvSearch_AreaExit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static BOOL BrBvSearch_AreaMain(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static void BrBvSearch_CreateMainDisplay(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static void BrBvSearch_DeleteMainDisplay(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param);
static void BrBvSearch_PrintMainDisplay(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param, u32 menu);
static u32 BrBvSearch_GetTrgMenu(void);

static const BrBvSearchBtnData sBrBvSearchModeBtns[] = {
    { 80, 168, 0, 4 },
};

static const BrBvSearchBtnData sBrBvSearchAreaBtns[] = {
    { 80, 168, 0, 4 },
};

static const BrBvSearchBtnData sBrBvSearchMenuBtns[BR_BVSEARCH_BTN_MAX] = {
    { 32, 168, 0, 4 },
    { 128, 168, 1, 0x67 },
};

static const BrBvSearchMsgWinData sBrBvSearchMenuWins[BR_BVSEARCH_MENU_MAX] = {
    { 7, 3, 17, 2, 0x64 },
    { 7, 8, 17, 2, 0x65 },
    { 7, 13, 17, 2, 0x66 },
};

static const BrListParam sBrBvSearchModeListParam = {
    NULL, 0, 5, 3, 22, 12, 14, 4, 4, BR_LIST_TYPE_TOUCH, NULL, NULL, NULL, NULL, NULL,
};

static const BrListParam sBrBvSearchAreaListParam = {
    NULL, 0, 5, 3, 22, 12, 14, 4, 4, BR_LIST_TYPE_TOUCH, NULL, NULL, NULL, NULL, NULL,
};

static const BrBvSearchMsgWinData sBrBvSearchMsgWins[BR_BVSEARCH_MSGWIN_MAX] = {
    { 4, 3, 16, 2, 0x24 },  { 5, 5, 22, 4, 0x75 },  { 4, 9, 16, 2, 0x25 },
    { 5, 11, 22, 4, 0x75 }, { 4, 13, 16, 2, 0x26 }, { 5, 15, 24, 4, 0x75 },
};

static const BrBvSearchPageFuncs sBrBvSearchPageFuncs[BR_BVSEARCH_PAGE_MAX] = {
    { BrBvSearch_MenuInit, BrBvSearch_MenuMain, BrBvSearch_MenuExit },
    { BrBvSearch_ModeInit, BrBvSearch_ModeMain, BrBvSearch_ModeExit },
    { BrBvSearch_PokeInit, BrBvSearch_PokeMain, BrBvSearch_PokeExit },
    { BrBvSearch_AreaInit, BrBvSearch_AreaMain, BrBvSearch_AreaExit },
};

static const BrBvSearchRect sBrBvSearchMenuRects[BR_BVSEARCH_MENU_MAX] = {
    { 24, 16, 232, 40 },
    { 24, 56, 232, 80 },
    { 24, 96, 232, 120 },
};

const GameProcFunctions BR_BVSEARCH_PROC_FUNCTIONS = {
    BrBvSearch_ProcInit,
    BrBvSearch_ProcMain,
    BrBvSearch_ProcExit,
};

static BOOL BrBvSearch_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvSearchProcParam *p_param = param;
    BrBvSearchWork *p_wk;
    int i;

    p_wk = GFL_ProcInitSubsystem(proc, sizeof(BrBvSearchWork), BrProcSys_GetHeapID(p_param->procSys));
    sys_memset(p_wk, 0, sizeof(BrBvSearchWork));
    p_wk->heapId = BrProcSys_GetHeapID(p_param->procSys);

    p_wk->que = func_02021998(p_wk->heapId);
    p_wk->bmpoam = BmpOam_Init(p_wk->heapId, p_param->unit);
    p_wk->text = BrText_Init(p_param->res, p_wk->que, p_wk->heapId);
    BrText_Print(p_wk->text, p_param->res, 0x6b);
    for (i = 0; i < 2; i++) {
        p_wk->ballEff[i] = BrBallEff_Init(p_param->unit, p_param->res, i, p_wk->heapId);
    }

    p_param->search.monsno = BR_NET_VIDEO_SEARCH_MONSNO_NONE;
    p_param->search.country = BR_NET_VIDEO_SEARCH_AREA_NONE;
    p_param->search.area = BR_NET_VIDEO_SEARCH_AREA_NONE;
    p_param->search.mode = BR_NET_VIDEO_SEARCH_MODE_NONE;

    BrRes_LoadOBJ(p_param->res, 6, p_wk->heapId);
    BrBvSearch_CreateMainDisplay(p_wk, p_param);
    return TRUE;
}

static BOOL BrBvSearch_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvSearchProcParam *p_param = param;
    BrBvSearchWork *p_wk = work;
    int i;

    BrBvSearch_DeleteMainDisplay(p_wk, p_param);
    BrRes_UnloadOBJ(p_param->res, 6);
    for (i = 0; i < 2; i++) {
        BrBallEff_Exit(p_wk->ballEff[i]);
    }
    BrText_Exit(p_wk->text, p_param->res);
    BmpOam_Exit(p_wk->bmpoam);
    func_02021a18(p_wk->que);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BrBvSearch_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    enum {
        SEQ_INIT,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_MAIN,
        SEQ_CHANGE_FADEOUT_START,
        SEQ_CHANGE_FADEOUT_WAIT,
        SEQ_CHANGE_EXIT,
        SEQ_CHANGE_INIT,
        SEQ_CHANGE_FADEIN_START,
        SEQ_CHANGE_FADEIN_WAIT,
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_EXIT,
    };
    BrBvSearchProcParam *p_param = param;
    BrBvSearchWork *p_wk = work;
    int i;

    switch (*state) {
    case SEQ_INIT:
        if (sBrBvSearchPageFuncs[p_wk->page].init(p_wk, p_param)) {
            (*state)++;
        }
        break;
    case SEQ_FADEIN_START:
        BrFade_StartFade(p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        (*state)++;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p_param->fade)) {
            *state = SEQ_MAIN;
        }
        break;
    case SEQ_MAIN:
        if (sBrBvSearchPageFuncs[p_wk->page].main(p_wk, p_param)) {
            if (p_wk->nextPage == BR_BVSEARCH_PAGE_END) {
                *state = SEQ_FADEOUT_START;
            } else {
                *state = SEQ_CHANGE_FADEOUT_START;
            }
        }
        break;
    case SEQ_CHANGE_FADEOUT_START:
        BrFade_StartFade(p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_SUB, BR_FADE_DIR_OUT);
        (*state)++;
        break;
    case SEQ_CHANGE_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_param->fade)) {
            (*state)++;
        }
        break;
    case SEQ_CHANGE_EXIT:
        if (sBrBvSearchPageFuncs[p_wk->page].exit(p_wk, p_param)) {
            p_wk->page = p_wk->nextPage;
            (*state)++;
        }
        break;
    case SEQ_CHANGE_INIT:
        if (sBrBvSearchPageFuncs[p_wk->page].init(p_wk, p_param)) {
            (*state)++;
        }
        break;
    case SEQ_CHANGE_FADEIN_START:
        BrFade_StartFade(p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_SUB, BR_FADE_DIR_IN);
        (*state)++;
        break;
    case SEQ_CHANGE_FADEIN_WAIT:
        if (BrFade_IsEnd(p_param->fade)) {
            *state = SEQ_MAIN;
        }
        break;
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        (*state)++;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_param->fade)) {
            *state = SEQ_EXIT;
        }
        break;
    case SEQ_EXIT:
        if (sBrBvSearchPageFuncs[p_wk->page].exit(p_wk, p_param)) {
            return TRUE;
        }
        break;
    }

    for (i = 0; i < BR_BVSEARCH_MENU_MAX; i++) {
        if (p_wk->menuWin[i] != NULL) {
            BrMsgWin_Main(p_wk->menuWin[i]);
        }
    }
    for (i = 0; i < BR_BVSEARCH_MSGWIN_MAX; i++) {
        if (p_wk->msgWin[i] != NULL) {
            BrMsgWin_Main(p_wk->msgWin[i]);
        }
    }
    if (p_wk->text != NULL) {
        BrText_Main(p_wk->text);
    }
    func_02021a3c(p_wk->que);
    for (i = 0; i < 2; i++) {
        BrBallEff_Main(p_wk->ballEff[i]);
    }
    return FALSE;
}

// The menu of the things to search by, with the return and search buttons
static BOOL BrBvSearch_MenuInit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    Font *font;
    MsgData *msg;
    ClActorSetup setup;
    BrResObjData obj;
    int i;

    font = BrRes_GetFont(p_param->res);
    msg = BrRes_GetMsgData(p_param->res);
    BrRes_LoadBG(p_param->res, 0x1a, p_wk->heapId);

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    BrRes_GetObjData(p_param->res, 6, &obj);
    for (i = 0; i < BR_BVSEARCH_BTN_MAX; i++) {
        setup.x = sBrBvSearchMenuBtns[i].x;
        setup.y = sBrBvSearchMenuBtns[i].y;
        setup.sequence = sBrBvSearchMenuBtns[i].anmSeq;
        setup.priority = 1;
        p_wk->btn[i] = BrBtn_InitEx(&setup, sBrBvSearchMenuBtns[i].msgID, 96, CLACT_SURFACE_SUB, p_param->unit,
                                    p_wk->bmpoam, font, msg, &obj, p_wk->heapId);
    }

    for (i = 0; i < BR_BVSEARCH_MENU_MAX; i++) {
        const BrBvSearchMsgWinData *data = &sBrBvSearchMenuWins[i];

        p_wk->menuWin[i] = BrMsgWin_Init(4, data->x, data->y, data->w, data->h, 14, p_wk->que, p_wk->heapId);
        BrMsgWin_Print(p_wk->menuWin[i], msg, data->msgID, font, BR_BVSEARCH_PRINT_COLOR);
    }

    BrText_Print(p_wk->text, p_param->res, 0x6b);
    return TRUE;
}

static BOOL BrBvSearch_MenuExit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    int i;

    for (i = 0; i < BR_BVSEARCH_MENU_MAX; i++) {
        if (p_wk->menuWin[i] != NULL) {
            BrMsgWin_Exit(p_wk->menuWin[i]);
            p_wk->menuWin[i] = NULL;
        }
    }
    for (i = 0; i < BR_BVSEARCH_BTN_MAX; i++) {
        if (p_wk->btn[i] != NULL) {
            BrBtn_Exit(p_wk->btn[i]);
            p_wk->btn[i] = NULL;
        }
    }
    GFL_BGSysLoadScr(4);
    BrRes_UnloadBG(p_param->res, 0x1a);
    return TRUE;
}

static BOOL BrBvSearch_MenuMain(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    u32 menu = BrBvSearch_GetTrgMenu();

    if (menu != BR_BVSEARCH_MENU_MAX) {
        switch (menu) {
        case BR_BVSEARCH_MENU_MODE: {
            u32 x;
            u32 y;
            BrPoint pos;

            func_0203dac8(&x, &y);
            pos.x = x;
            pos.y = y;
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            GFL_SndSEPlay(SEQ_SE_BREC_06);
            // Choosing a thing again clears it
            p_param->search.mode = BR_NET_VIDEO_SEARCH_MODE_NONE;
            BrBvSearch_PrintMainDisplay(p_wk, p_param, BR_BVSEARCH_MENU_MODE);
            BrText_Print(p_wk->text, p_param->res, 0x69);
            p_wk->nextPage = BR_BVSEARCH_PAGE_MODE;
            return TRUE;
        }
        case BR_BVSEARCH_MENU_POKE: {
            u32 x;
            u32 y;
            BrPoint pos;

            func_0203dac8(&x, &y);
            pos.x = x;
            pos.y = y;
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            GFL_SndSEPlay(SEQ_SE_BREC_06);
            p_param->search.monsno = BR_NET_VIDEO_SEARCH_MONSNO_NONE;
            BrBvSearch_PrintMainDisplay(p_wk, p_param, BR_BVSEARCH_MENU_POKE);
            BrText_Print(p_wk->text, p_param->res, 0x6a);
            p_wk->nextPage = BR_BVSEARCH_PAGE_POKE;
            return TRUE;
        }
        case BR_BVSEARCH_MENU_AREA: {
            PlayerInfo *player = GetGameDataPlayerInfo(p_param->gameData);
            u32 country = UnityTowerVisitor_GetCountry(player);
            u32 area = UnityTowerVisitor_GetProvince(player);

            if (country == 0 && area == 0) {
                // The player hasn't registered where they live
                BrText_Print(p_wk->text, p_param->res, 0x78);
                GFL_SndSEPlay(SEQ_SE_BREC_07);
            } else {
                u32 x;
                u32 y;
                BrPoint pos;

                func_0203dac8(&x, &y);
                pos.x = x;
                pos.y = y;
                BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
                GFL_SndSEPlay(SEQ_SE_BREC_06);
                // The area is set or cleared here, without its page
                if (p_param->search.country == BR_NET_VIDEO_SEARCH_AREA_NONE) {
                    p_param->search.country = country;
                    p_param->search.area = area;
                } else {
                    p_param->search.country = BR_NET_VIDEO_SEARCH_AREA_NONE;
                    p_param->search.area = BR_NET_VIDEO_SEARCH_AREA_NONE;
                }
                BrBvSearch_PrintMainDisplay(p_wk, p_param, BR_BVSEARCH_MENU_AREA);
                return FALSE;
            }
            break;
        }
        }
    }

    {
        u32 x;
        u32 y;

        if (func_0203dac8(&x, &y)) {
            if (BrBtn_GetTrg(p_wk->btn[BR_BVSEARCH_BTN_RETURN], x, y)) {
                BrPoint pos;

                pos.x = x;
                pos.y = y;
                BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
                p_wk->nextPage = BR_BVSEARCH_PAGE_END;
                BrProcSys_Pop(p_param->procSys);
                return TRUE;
            }
            if (BrBtn_GetHit(p_wk->btn[BR_BVSEARCH_BTN_SEARCH], x, y)) {
                if (p_param->search.monsno != BR_NET_VIDEO_SEARCH_MONSNO_NONE ||
                    p_param->search.area != BR_NET_VIDEO_SEARCH_AREA_NONE ||
                    p_param->search.mode != BR_NET_VIDEO_SEARCH_MODE_NONE) {
                    BrPoint pos;

                    pos.x = x;
                    pos.y = y;
                    GFL_SndSEPlay(SEQ_SE_BREC_06);
                    BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
                    p_wk->nextPage = BR_BVSEARCH_PAGE_END;
                    BrProcSys_Push(p_param->procSys, BR_PROCID_BV_RANK);
                    return TRUE;
                } else {
                    // Nothing to search by
                    GFL_SndSEPlay(SEQ_SE_BREC_07);
                    BrText_Print(p_wk->text, p_param->res, 0x6c);
                }
            }
        }
    }
    return FALSE;
}

// The list of the battle modes
static BOOL BrBvSearch_ModeInit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    Font *font;
    MsgData *msg;
    BrListParam param;
    ClActorSetup setup;
    BrResObjData obj;
    int i;

    font = BrRes_GetFont(p_param->res);
    msg = BrRes_GetMsgData(p_param->res);
    BrRes_LoadBG(p_param->res, 0x1b, p_wk->heapId);

    p_wk->p_list_data = ListMenuCore_CreateOptionList(BR_NET_VIDEO_SEARCH_MODE_NONE, p_wk->heapId);
    for (i = 0; i < BR_NET_VIDEO_SEARCH_MODE_NONE; i++) {
        ListMenuCore_AppendMsgOption(&p_wk->p_list_data[i], msg, 0xcd + i, i, p_wk->heapId);
    }

    param = sBrBvSearchModeListParam;
    param.cp_list = (const BrListItem *)p_wk->p_list_data;
    param.list_max = BR_NET_VIDEO_SEARCH_MODE_NONE;
    param.res = p_param->res;
    param.unit = p_param->unit;
    p_wk->p_list = BrList_Init(&param, p_wk->heapId);

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    BrRes_GetObjData(p_param->res, 6, &obj);
    for (i = 0; i < 1; i++) {
        setup.x = sBrBvSearchModeBtns[i].x;
        setup.y = sBrBvSearchModeBtns[i].y;
        setup.sequence = sBrBvSearchModeBtns[i].anmSeq;
        setup.priority = 1;
        p_wk->btn[i] = BrBtn_InitEx(&setup, sBrBvSearchModeBtns[i].msgID, 96, CLACT_SURFACE_SUB, p_param->unit,
                                    p_wk->bmpoam, font, msg, &obj, p_wk->heapId);
    }
    return TRUE;
}

static BOOL BrBvSearch_ModeExit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    if (p_wk->btn[BR_BVSEARCH_BTN_RETURN] != NULL) {
        BrBtn_Exit(p_wk->btn[BR_BVSEARCH_BTN_RETURN]);
        p_wk->btn[BR_BVSEARCH_BTN_RETURN] = NULL;
    }
    BrList_Exit(p_wk->p_list);
    p_wk->p_list = NULL;
    ListMenuCore_FreeOptionList(p_wk->p_list_data);
    p_wk->p_list_data = NULL;
    BrRes_UnloadBG(p_param->res, 0x1b);
    GFL_BGSysLoadScr(4);
    return TRUE;
}

static BOOL BrBvSearch_ModeMain(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    u32 select;
    u32 x;
    u32 y;
    BOOL isReturn;

    BrList_Main(p_wk->p_list);
    select = BrList_GetSelect(p_wk->p_list);
    isReturn = func_0203dac8(&x, &y);
    isReturn &= BrBtn_GetTrg(p_wk->btn[BR_BVSEARCH_BTN_RETURN], x, y);

    if (select != BR_LIST_SELECT_NONE) {
        u32 x;
        u32 y;
        BrPoint pos;

        func_0203dac8(&x, &y);
        pos.x = x;
        pos.y = y;
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        p_param->search.mode = select;
        BrBvSearch_PrintMainDisplay(p_wk, p_param, BR_BVSEARCH_MENU_MODE);
        p_wk->nextPage = BR_BVSEARCH_PAGE_MENU;
        return TRUE;
    } else if (isReturn) {
        u32 x;
        u32 y;
        BrPoint pos;

        func_0203dac8(&x, &y);
        pos.x = x;
        pos.y = y;
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        p_wk->nextPage = BR_BVSEARCH_PAGE_MENU;
        return TRUE;
    }
    return FALSE;
}

// The Pokémon search
static BOOL BrBvSearch_PokeInit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    p_wk->p_search = BrPokeSearch_Init(GameData_GetPokedex(p_param->gameData), p_param->res, p_param->unit,
                                       p_wk->bmpoam, p_param->fade, p_wk->ballEff[CLACT_SURFACE_MAIN],
                                       p_wk->ballEff[CLACT_SURFACE_SUB], p_wk->heapId);
    BrPokeSearch_StartUp(p_wk->p_search);
    return BrPokeSearch_PrintMain(p_wk->p_search);
}

static BOOL BrBvSearch_PokeExit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    BrPokeSearch_CleanUp(p_wk->p_search);
    BrPokeSearch_Exit(p_wk->p_search);
    GFL_BGSysLoadScr(4);
    return TRUE;
}

static BOOL BrBvSearch_PokeMain(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    u32 select;
    u16 monsno;

    BrPokeSearch_Main(p_wk->p_search);
    select = BrPokeSearch_GetSelect(p_wk->p_search, &monsno);
    if (select != BR_POKESEARCH_SELECT_NONE) {
        if (select == BR_POKESEARCH_SELECT_DECIDE) {
            p_param->search.monsno = monsno;
            BrBvSearch_PrintMainDisplay(p_wk, p_param, BR_BVSEARCH_MENU_POKE);
            p_wk->nextPage = BR_BVSEARCH_PAGE_MENU;
        } else {
            p_wk->nextPage = BR_BVSEARCH_PAGE_MENU;
        }
        return TRUE;
    }
    return FALSE;
}

// The list of the areas, which only has the player's own
static BOOL BrBvSearch_AreaInit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    Font *font;
    MsgData *msg;
    StrBuf *str;
    PlayerInfo *player;
    u32 country;
    u32 area;
    WordSet *wordSet;
    StrBuf *src;
    BrListParam param;
    ClActorSetup setup;
    BrResObjData obj;
    int i;

    font = BrRes_GetFont(p_param->res);
    msg = BrRes_GetMsgData(p_param->res);
    BrRes_LoadBG(p_param->res, 0x1c, p_wk->heapId);

    str = GFL_StrBufCreate(0x80, p_wk->heapId);
    player = GetGameDataPlayerInfo(p_param->gameData);
    country = UnityTowerVisitor_GetCountry(player);
    area = UnityTowerVisitor_GetProvince(player);
    wordSet = BrRes_GetWordSet(p_param->res);
    src = GFL_MsgDataLoadStrbufNew(msg, 0x7d);
    if (!CountryHasProvinces(country)) {
        loadCountryToStrbuf(wordSet, 0, country);
    } else {
        loadCountryAreaToStrbuf(wordSet, 0, country, area);
    }
    GFL_WordSetFormatStrbuf(wordSet, str, src);
    GFL_StrBufFree(src);

    p_wk->p_list_data = ListMenuCore_CreateOptionList(1, p_wk->heapId);
    ListMenuCore_AppendStrBufOption(p_wk->p_list_data, str, 0, p_wk->heapId);
    GFL_StrBufFree(str);

    param = sBrBvSearchAreaListParam;
    param.cp_list = (const BrListItem *)p_wk->p_list_data;
    param.list_max = 1;
    param.res = p_param->res;
    param.unit = p_param->unit;
    p_wk->p_list = BrList_Init(&param, p_wk->heapId);

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    BrRes_GetObjData(p_param->res, 6, &obj);
    for (i = 0; i < 1; i++) {
        setup.x = sBrBvSearchAreaBtns[i].x;
        setup.y = sBrBvSearchAreaBtns[i].y;
        setup.sequence = sBrBvSearchAreaBtns[i].anmSeq;
        setup.priority = 1;
        p_wk->btn[i] = BrBtn_InitEx(&setup, sBrBvSearchAreaBtns[i].msgID, 96, CLACT_SURFACE_SUB, p_param->unit,
                                    p_wk->bmpoam, font, msg, &obj, p_wk->heapId);
    }
    return TRUE;
}

static BOOL BrBvSearch_AreaExit(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    if (p_wk->btn[BR_BVSEARCH_BTN_RETURN] != NULL) {
        BrBtn_Exit(p_wk->btn[BR_BVSEARCH_BTN_RETURN]);
        p_wk->btn[BR_BVSEARCH_BTN_RETURN] = NULL;
    }
    BrList_Exit(p_wk->p_list);
    p_wk->p_list = NULL;
    ListMenuCore_FreeOptionList(p_wk->p_list_data);
    p_wk->p_list_data = NULL;
    BrRes_UnloadBG(p_param->res, 0x1c);
    GFL_BGSysLoadScr(4);
    return TRUE;
}

static BOOL BrBvSearch_AreaMain(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    u32 select;
    u32 x;
    u32 y;
    BOOL isReturn;

    BrList_Main(p_wk->p_list);
    select = BrList_GetSelect(p_wk->p_list);
    isReturn = func_0203dac8(&x, &y);
    isReturn &= BrBtn_GetTrg(p_wk->btn[BR_BVSEARCH_BTN_RETURN], x, y);

    if (select != BR_LIST_SELECT_NONE) {
        u32 x;
        u32 y;
        BrPoint pos;
        PlayerInfo *player;
        u32 country;

        func_0203dac8(&x, &y);
        pos.x = x;
        pos.y = y;
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        player = GetGameDataPlayerInfo(p_param->gameData);
        country = UnityTowerVisitor_GetCountry(player);
        p_param->search.area = UnityTowerVisitor_GetProvince(player);
        p_param->search.country = country;
        BrBvSearch_PrintMainDisplay(p_wk, p_param, BR_BVSEARCH_MENU_AREA);
        p_wk->nextPage = BR_BVSEARCH_PAGE_MENU;
        return TRUE;
    } else if (isReturn) {
        u32 x;
        u32 y;
        BrPoint pos;

        func_0203dac8(&x, &y);
        pos.x = x;
        pos.y = y;
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        p_wk->nextPage = BR_BVSEARCH_PAGE_MENU;
        return TRUE;
    }
    return FALSE;
}

// The main screen: the captions of the things to search by, and nothing chosen
static void BrBvSearch_CreateMainDisplay(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    Font *font;
    MsgData *msg;
    int i;

    font = BrRes_GetFont(p_param->res);
    msg = BrRes_GetMsgData(p_param->res);
    BrRes_LoadBG(p_param->res, 0x19, p_wk->heapId);

    for (i = 0; i < BR_BVSEARCH_MSGWIN_MAX; i++) {
        const BrBvSearchMsgWinData *data = &sBrBvSearchMsgWins[i];

        p_wk->msgWin[i] = BrMsgWin_Init(1, data->x, data->y, data->w, data->h, 14, p_wk->que, p_wk->heapId);
        BrMsgWin_SetPos(p_wk->msgWin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
        BrMsgWin_Print(p_wk->msgWin[i], msg, data->msgID, font, BR_BVSEARCH_PRINT_COLOR);
    }
}

static void BrBvSearch_DeleteMainDisplay(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param) {
    int i;

    for (i = 0; i < BR_BVSEARCH_MSGWIN_MAX; i++) {
        if (p_wk->msgWin[i] != NULL) {
            BrMsgWin_Exit(p_wk->msgWin[i]);
            p_wk->msgWin[i] = NULL;
        }
    }
    BrRes_UnloadBG(p_param->res, 0x19);
    GFL_BGSysLoadScr(1);
}

// Prints the choice of a thing to search by on the main screen
static void BrBvSearch_PrintMainDisplay(BrBvSearchWork *p_wk, BrBvSearchProcParam *p_param, u32 menu) {
    Font *font;
    MsgData *msg;

    font = BrRes_GetFont(p_param->res);
    msg = BrRes_GetMsgData(p_param->res);

    switch (menu) {
    case BR_BVSEARCH_MENU_MODE:
        if (p_param->search.mode != BR_NET_VIDEO_SEARCH_MODE_NONE) {
            BrMsgWin_Print(p_wk->msgWin[1], msg, 0xcd + p_param->search.mode, font, BR_BVSEARCH_PRINT_COLOR);
        } else {
            BrMsgWin_Print(p_wk->msgWin[1], msg, 0x75, font, BR_BVSEARCH_PRINT_COLOR);
        }
        break;
    case BR_BVSEARCH_MENU_POKE:
        if (p_param->search.monsno != BR_NET_VIDEO_SEARCH_MONSNO_NONE) {
            BrMsgWin_Print(p_wk->msgWin[3], g_PMLSpeciesNamesResident, p_param->search.monsno, font,
                           BR_BVSEARCH_PRINT_COLOR);
        } else {
            BrMsgWin_Print(p_wk->msgWin[3], msg, 0x75, font, BR_BVSEARCH_PRINT_COLOR);
        }
        break;
    case BR_BVSEARCH_MENU_AREA:
        if (p_param->search.country == BR_NET_VIDEO_SEARCH_AREA_NONE &&
            p_param->search.area == BR_NET_VIDEO_SEARCH_AREA_NONE) {
            BrMsgWin_Print(p_wk->msgWin[5], msg, 0x75, font, BR_BVSEARCH_PRINT_COLOR);
        } else {
            WordSet *wordSet = BrRes_GetWordSet(p_param->res);
            StrBuf *src = GFL_MsgDataLoadStrbufNew(msg, 0x18);
            StrBuf *str = GFL_StrBufCreate(0x80, p_wk->heapId);

            if (!CountryHasProvinces(p_param->search.country)) {
                loadCountryToStrbuf(wordSet, 0, p_param->search.country);
            } else {
                loadCountryAreaToStrbuf(wordSet, 0, p_param->search.country, p_param->search.area);
            }
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            BrMsgWin_PrintBuf(p_wk->msgWin[5], str, font, BR_BVSEARCH_PRINT_COLOR);
            GFL_StrBufFree(src);
            GFL_StrBufFree(str);
        }
        break;
    }
}

// The line of the menu that the touch is on, or BR_BVSEARCH_MENU_MAX
static u32 BrBvSearch_GetTrgMenu(void) {
    u32 x;
    u32 y;
    int i;

    if (func_0203dac8(&x, &y)) {
        for (i = 0; i < BR_BVSEARCH_MENU_MAX; i++) {
            const BrBvSearchRect *rect = &sBrBvSearchMenuRects[i];

            if ((u32)(x - rect->left) <= (u32)(rect->right - rect->left) &&
                (u32)(y - rect->top) <= (u32)(rect->bottom - rect->top)) {
                return i;
            }
        }
    }
    return BR_BVSEARCH_MENU_MAX;
}
