#include "types.h"
#include "app/pokedex_common.h"
#include "app/pokedex_search_bmp.h"
#include "app/pokedex_search_engine.h"
#include "app/pokedex_search_list.h"
#include "app/pokedex_search_main.h"
#include "app/pokedex_search_obj.h"
#include "app/pokedex_search_seq.h"
#include "app/pokedex_search_ui.h"
#include "app/ui/frame_list.h"
#include "app/ui/ui_scene.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/pokedex_savedata.h"
#include "system/blink_palanm.h"
#include "system/cursor_move.h"
#include "system/game_data.h"
#include "system/palanm.h"

// The sequences of the Pokédex's search screen: the main page with the criteria, a page with a list for each
// criterion, and the search. The ROM doesn't name this file; pokedex_search_seq.c is a guess after
// pokedex_search_main.c

enum {
    SEQ_INIT,
    SEQ_EXIT,
    SEQ_WAIT_FADE,
    SEQ_MAIN_INIT,
    SEQ_MAIN,
    SEQ_PAGE_END,
    SEQ_RESET,
    SEQ_SEARCH,
    SEQ_SEARCH_RESULT,
    SEQ_ORDER_INIT,
    SEQ_ORDER,
    SEQ_ORDER_EXIT,
    SEQ_INITIAL_INIT,
    SEQ_INITIAL,
    SEQ_INITIAL_EXIT,
    SEQ_TYPE_INIT,
    SEQ_TYPE,
    SEQ_TYPE_EXIT,
    SEQ_COLOR_INIT,
    SEQ_COLOR,
    SEQ_COLOR_EXIT,
    SEQ_SHAPE_INIT,
    SEQ_SHAPE,
    SEQ_SHAPE_EXIT,
    SEQ_LIST_SCROLL,
    SEQ_BUTTON_ANM,
    SEQ_FINISH,
    SEQ_END,
};

// How long the search takes at least, and when its sound stops
#define SEARCH_FRAMES 121
#define SEARCH_SOUND_FRAMES 100

// No button to wait for while the list jumps
#define BUTTON_NONE POKEDEX_SEARCH_OBJ_COUNT

typedef int (*PokedexSearchSeqFunc)(PokedexSearchWork *wk);

static int PokedexSearchSeq_Init(PokedexSearchWork *wk);
static int PokedexSearchSeq_Exit(PokedexSearchWork *wk);
static int PokedexSearchSeq_WaitFade(PokedexSearchWork *wk);
static int PokedexSearchSeq_MainInit(PokedexSearchWork *wk);
static int PokedexSearchSeq_MainPage(PokedexSearchWork *wk);
static int PokedexSearchSeq_PageEnd(PokedexSearchWork *wk);
static int PokedexSearchSeq_Reset(PokedexSearchWork *wk);
static int PokedexSearchSeq_Search(PokedexSearchWork *wk);
static int PokedexSearchSeq_SearchResult(PokedexSearchWork *wk);
static int PokedexSearchSeq_OrderInit(PokedexSearchWork *wk);
static int PokedexSearchSeq_Order(PokedexSearchWork *wk);
static int PokedexSearchSeq_OrderExit(PokedexSearchWork *wk);
static int PokedexSearchSeq_InitialInit(PokedexSearchWork *wk);
static int PokedexSearchSeq_Initial(PokedexSearchWork *wk);
static int PokedexSearchSeq_InitialExit(PokedexSearchWork *wk);
static int PokedexSearchSeq_TypeInit(PokedexSearchWork *wk);
static int PokedexSearchSeq_Type(PokedexSearchWork *wk);
static int PokedexSearchSeq_TypeExit(PokedexSearchWork *wk);
static int PokedexSearchSeq_ColorInit(PokedexSearchWork *wk);
static int PokedexSearchSeq_Color(PokedexSearchWork *wk);
static int PokedexSearchSeq_ColorExit(PokedexSearchWork *wk);
static int PokedexSearchSeq_ShapeInit(PokedexSearchWork *wk);
static int PokedexSearchSeq_Shape(PokedexSearchWork *wk);
static int PokedexSearchSeq_ShapeExit(PokedexSearchWork *wk);
static int PokedexSearchSeq_ListScroll(PokedexSearchWork *wk);
static int PokedexSearchSeq_ButtonAnm(PokedexSearchWork *wk);
static int PokedexSearchSeq_Finish(PokedexSearchWork *wk);
static void PokedexSearchSeq_SetOrder(PokedexSearchWork *wk, u32 row);
static void PokedexSearchSeq_ResetOrder(PokedexSearchWork *wk);
static void PokedexSearchSeq_SetInitial(PokedexSearchWork *wk, u32 row);
static void PokedexSearchSeq_ResetInitial(PokedexSearchWork *wk);
static void PokedexSearchSeq_SetType(PokedexSearchWork *wk, u32 row);
static void PokedexSearchSeq_ResetType(PokedexSearchWork *wk);
static void PokedexSearchSeq_SetColor(PokedexSearchWork *wk, u32 row);
static void PokedexSearchSeq_ResetColor(PokedexSearchWork *wk);
static void PokedexSearchSeq_SetShape(PokedexSearchWork *wk, u32 row);
static void PokedexSearchSeq_ResetShape(PokedexSearchWork *wk);
static int PokedexSearchSeq_SetButtonAnm(PokedexSearchWork *wk, u32 isActor, u32 id, int next);
static int PokedexSearchSeq_SetPageEnd(PokedexSearchWork *wk, int next);
static void PokedexSearchSeq_ToggleShortcut(PokedexSearchWork *wk);

static const PokedexSearchSeqFunc sPokedexSearchSeqFuncs[] = {
    PokedexSearchSeq_Init,         PokedexSearchSeq_Exit,      PokedexSearchSeq_WaitFade,    PokedexSearchSeq_MainInit,
    PokedexSearchSeq_MainPage,     PokedexSearchSeq_PageEnd,   PokedexSearchSeq_Reset,       PokedexSearchSeq_Search,
    PokedexSearchSeq_SearchResult, PokedexSearchSeq_OrderInit, PokedexSearchSeq_Order,       PokedexSearchSeq_OrderExit,
    PokedexSearchSeq_InitialInit,  PokedexSearchSeq_Initial,   PokedexSearchSeq_InitialExit, PokedexSearchSeq_TypeInit,
    PokedexSearchSeq_Type,         PokedexSearchSeq_TypeExit,  PokedexSearchSeq_ColorInit,   PokedexSearchSeq_Color,
    PokedexSearchSeq_ColorExit,    PokedexSearchSeq_ShapeInit, PokedexSearchSeq_Shape,       PokedexSearchSeq_ShapeExit,
    PokedexSearchSeq_ListScroll,   PokedexSearchSeq_ButtonAnm, PokedexSearchSeq_Finish,
};

BOOL PokedexSearchSeq_Main(PokedexSearchWork *wk) {
    wk->seq = sPokedexSearchSeqFuncs[wk->seq](wk);
    if (wk->seq == SEQ_END) {
        return FALSE;
    }
    PokedexSearchObj_Update(wk);
    PokedexSearchBmp_Update(wk);
    PokedexCommon_ScrollBGs(2, 6, &wk->scrollTimer);
    return TRUE;
}

static int PokedexSearchSeq_Init(PokedexSearchWork *wk) {
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetDisplayLayout(0);
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    if (PokeDex_GetMode(wk->param->pokedex) == 1) {
        wk->param->criteria->regional = 0;
    } else {
        wk->param->criteria->regional = 1;
    }
    PokedexSearchMain_InitVRAM();
    PokedexSearchMain_InitBG();
    PokedexSearchMain_LoadGraphics();
    PokedexSearchMain_InitMsg(wk);
    PokedexSearchBmp_Init(wk);
    PokedexSearchObj_Init(wk);
    PokedexSearchMain_InitBlinkAnm(wk);
    GFL_NetSetIconScreen(TRUE, HEAPID_POKEDEX_SEARCH);
    PokedexSearchMain_InitBGWinFrame(wk);
    PokedexSearchMain_InitFramePos(wk);
    PokedexSearchMain_InitPaletteFade(wk);
    PokedexSearchMain_StartFade(wk, 16, 16);
    PokedexSearchMain_LoadLoadingScreen(wk);
    PokedexSearchMain_InitWindow();
    PokedexSearchMain_InitVBlank(wk);
    PokedexSearchMain_InitHBlank(wk);
    wk->page = POKEDEX_SEARCH_NONE;
    PokedexSearchMain_ShapeToListPos(wk);
    return SEQ_MAIN_INIT;
}

static int PokedexSearchSeq_Exit(PokedexSearchWork *wk) {
    if (PokedexSearchMain_UpdateFrameMove(wk) == TRUE || PaletteFade_GetActiveMask(wk->paletteFade) != 0) {
        return SEQ_EXIT;
    }
    PokedexSearchMain_ExitHBlank(wk);
    PokedexSearchMain_ExitVBlank(wk);
    PokedexSearchMain_ExitWindow();
    PokedexSearchMain_FreeLoadingScreen(wk);
    PokedexSearchMain_ExitBGWinFrame(wk);
    PokedexSearchMain_ExitPaletteFade(wk);
    PokedexSearchMain_ExitBlinkAnm(wk);
    PokedexSearchObj_Exit(wk);
    PokedexSearchBmp_Exit(wk);
    PokedexSearchMain_ExitMsg(wk);
    PokedexSearchMain_ExitBG();
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return SEQ_END;
}

static int PokedexSearchSeq_WaitFade(PokedexSearchWork *wk) {
    if (PaletteFade_GetActiveMask(wk->paletteFade) == 0) {
        return wk->buttonNextSeq;
    }
    return SEQ_WAIT_FADE;
}

static int PokedexSearchSeq_MainInit(PokedexSearchWork *wk) {
    if (PaletteFade_GetActiveMask(wk->paletteFade) != 0) {
        return SEQ_MAIN_INIT;
    }
    PokedexSearchMain_SetBlend(TRUE);
    PokedexSearchMain_LoadMainScreen(wk);
    PokedexSearchBmp_PutMain(wk);
    PokedexSearchObj_SetMain(wk);
    PokedexSearchMain_SetMainBGPriority();
    PokedexSearchMain_RequestBGOn(wk);
    if (wk->page == POKEDEX_SEARCH_NONE) {
        PokedexSearchUI_InitCursor(wk, POKEDEX_SEARCH_UI_ORDER);
        GX_SetMasterBrightness(0);
        GXS_SetMasterBrightness(0);
        PokedexSearchMain_StartFrameMove(wk, -8);
        PokedexSearchMain_StartFade(wk, 16, 0);
        return SEQ_MAIN;
    }
    PokedexSearchUI_InitCursor(wk, wk->page);
    return SEQ_MAIN;
}

static int PokedexSearchSeq_MainPage(PokedexSearchWork *wk) {
    if (PokedexSearchMain_UpdateFrameMove(wk) == TRUE || PaletteFade_GetActiveMask(wk->paletteFade) != 0) {
        return SEQ_MAIN;
    }
    switch (PokedexSearchUI_Main(wk)) {
    case POKEDEX_SEARCH_UI_ORDER:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return PokedexSearchSeq_SetButtonAnm(wk, FALSE, POKEDEX_SEARCH_UI_ORDER,
                                             PokedexSearchSeq_SetPageEnd(wk, SEQ_ORDER_INIT));
    case POKEDEX_SEARCH_UI_INITIAL:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return PokedexSearchSeq_SetButtonAnm(wk, FALSE, POKEDEX_SEARCH_UI_INITIAL,
                                             PokedexSearchSeq_SetPageEnd(wk, SEQ_INITIAL_INIT));
    case POKEDEX_SEARCH_UI_TYPE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return PokedexSearchSeq_SetButtonAnm(wk, FALSE, POKEDEX_SEARCH_UI_TYPE,
                                             PokedexSearchSeq_SetPageEnd(wk, SEQ_TYPE_INIT));
    case POKEDEX_SEARCH_UI_COLOR:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return PokedexSearchSeq_SetButtonAnm(wk, FALSE, POKEDEX_SEARCH_UI_COLOR,
                                             PokedexSearchSeq_SetPageEnd(wk, SEQ_COLOR_INIT));
    case POKEDEX_SEARCH_UI_SHAPE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return PokedexSearchSeq_SetButtonAnm(wk, FALSE, POKEDEX_SEARCH_UI_SHAPE,
                                             PokedexSearchSeq_SetPageEnd(wk, SEQ_SHAPE_INIT));
    case POKEDEX_SEARCH_UI_START:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return PokedexSearchSeq_SetButtonAnm(wk, FALSE, POKEDEX_SEARCH_UI_START, SEQ_SEARCH);
    case POKEDEX_SEARCH_UI_RESET:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return PokedexSearchSeq_SetButtonAnm(wk, FALSE, POKEDEX_SEARCH_UI_RESET, SEQ_RESET);
    case POKEDEX_SEARCH_UI_SHORTCUT:
        GFL_SndSEPlay(SEQ_SE_SYS_07);
        PokedexSearchSeq_ToggleShortcut(wk);
        break;
    case POKEDEX_SEARCH_UI_EXIT:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        wk->param->result = POKEDEX_SEARCH_RESULT_CLOSE;
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_EXIT, 8);
        return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_EXIT, SEQ_FINISH);
    case POKEDEX_SEARCH_UI_RETURN:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        PokedexSearchMain_ListPosToShape(wk);
        wk->param->result = POKEDEX_SEARCH_RESULT_CANCEL;
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
        return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_FINISH);
    case CURSOR_MOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_MOVED:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        PokedexSearchMain_ListPosToShape(wk);
        wk->param->result = POKEDEX_SEARCH_RESULT_CANCEL;
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
        return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_FINISH);
    }
    BlinkPalAnm_Main(wk->blinkAnm);
    return SEQ_MAIN;
}

static int PokedexSearchSeq_PageEnd(PokedexSearchWork *wk) {
    PokedexSearchMain_ClearPage(wk);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE, FALSE);
    PokedexSearchUI_ExitCursor(wk);
    return wk->nextSeq;
}

static int PokedexSearchSeq_Reset(PokedexSearchWork *wk) {
    PokedexCommon_InitCriteria(wk->param->pokedex, wk->param->criteria);
    PokedexSearchBmp_PutMain(wk);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE, FALSE);
    return SEQ_MAIN;
}

// Searches a step a frame while the progress bar fills, which takes longer
static int PokedexSearchSeq_Search(PokedexSearchWork *wk) {
    u16 count;
    u16 *list;

    if (wk->timer == 0) {
        GFL_SndSEPlay(SEQ_SE_SYS_78);
        PokedexSearchMain_ShowLoading(wk);
        PokedexSearchBmp_PutSearching(wk);
        PokedexSearchBmp_ClearInfo(wk);
    }
    if (wk->timer == SEARCH_SOUND_FRAMES) {
        GFL_SndStop();
    }
    if (wk->timer == 1) {
        PokedexSearchMain_ListPosToShape(wk);
        wk->engine = PokedexSearchEngine_Create(wk->param->pokedex, wk->param->criteria,
                                                HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
        wk->searchDone = FALSE;
    }
    if (wk->timer >= 2 && wk->searchDone == FALSE) {
        wk->searchDone = PokedexSearchEngine_Step(wk->engine, HEAPID_POKEDEX, &count, &list);
        if (wk->searchDone == TRUE) {
            wk->param->count = count;
            wk->param->list = list;
            PokedexSearchEngine_Free(wk->engine);
        }
    }
    if (wk->timer == SEARCH_FRAMES) {
        if (wk->searchDone == TRUE) {
            if (wk->param->count != 0) {
                GFL_SndSEPlay(SEQ_SE_SYS_84);
                PokedexSearchBmp_PutFound(wk);
            } else {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                PokedexSearchBmp_PutNotFound(wk);
            }
            wk->timer = 0;
            return SEQ_SEARCH_RESULT;
        }
    } else {
        PokedexSearchObj_MoveLoadingBar(wk, wk->timer);
        wk->timer++;
    }
    return SEQ_SEARCH;
}

static int PokedexSearchSeq_SearchResult(PokedexSearchWork *wk) {
    if (PokedexSearchUI_WaitResult(wk) == TRUE) {
        if (wk->param->count != 0) {
            wk->param->result = POKEDEX_SEARCH_RESULT_FOUND;
            return SEQ_FINISH;
        }
        PokedexSearchMain_HideLoading(wk);
        PokedexSearchBmp_PutInfo(wk);
        if (GCTX_HIDIsUsingTouch() == TRUE) {
            CursorMove_SetCursorVisible(wk->cursorMove, FALSE);
        } else {
            CursorMove_SetCursorVisible(wk->cursorMove, TRUE);
        }
        PokedexSearchMain_ShapeToListPos(wk);
        return SEQ_MAIN;
    }
    return SEQ_SEARCH_RESULT;
}

static int PokedexSearchSeq_OrderInit(PokedexSearchWork *wk) {
    switch (wk->initSeq) {
    case 0:
        wk->page = POKEDEX_SEARCH_PAGE_ORDER;
        PokedexSearchMain_SetBlend(TRUE);
        PokedexSearchBmp_PutOrder(wk);
        PokedexSearchList_InitOrder(wk);
        wk->initSeq++;
    case 1:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            PokedexSearchObj_SetOrder(wk);
            PokedexSearchMain_LoadOrderScreen(wk);
            PokedexSearchMain_SetListBGPriority();
            PokedexSearchMain_RequestBGOn(wk);
            wk->initSeq = 0;
            return SEQ_ORDER;
        }
        break;
    }
    return SEQ_ORDER_INIT;
}

static int PokedexSearchSeq_Order(PokedexSearchWork *wk) {
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        if (GCTX_HIDIsUsingTouch() == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchSeq_SetOrder(wk, ret);
        }
        break;
    case FRAMELIST_NONE:
        ret = PokedexSearchUI_ListMain(wk);
        switch (ret) {
        case POKEDEX_SEARCH_UI_LIST_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
            return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_ORDER_EXIT);
        case POKEDEX_SEARCH_UI_LIST_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
            return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_ORDER_EXIT);
        case POKEDEX_SEARCH_UI_LIST_RESET:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchSeq_ResetOrder(wk);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchSeq_SetOrder(wk, ret - POKEDEX_SEARCH_UI_LIST_ROW);
            break;
        }
        break;
    }
    return SEQ_ORDER;
}

static int PokedexSearchSeq_OrderExit(PokedexSearchWork *wk) {
    PokedexSearchMain_ClearPage(wk);
    PokedexSearchObj_HideList(wk);
    PokedexSearchList_Exit(wk);
    return SEQ_MAIN_INIT;
}

static int PokedexSearchSeq_InitialInit(PokedexSearchWork *wk) {
    switch (wk->initSeq) {
    case 0:
        wk->page = POKEDEX_SEARCH_PAGE_INITIAL;
        PokedexSearchMain_SetBlend(TRUE);
        PokedexSearchBmp_PutInitial(wk);
        PokedexSearchList_InitInitial(wk);
        wk->initSeq++;
    case 1:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            PokedexSearchObj_SetPageButtons(wk, FALSE);
            PokedexSearchObj_SetInitial(wk);
            PokedexSearchMain_LoadInitialScreen(wk);
            PokedexSearchMain_SetListBGPriority();
            PokedexSearchMain_RequestBGOn(wk);
            wk->initSeq = 0;
            return SEQ_INITIAL;
        }
        break;
    }
    return SEQ_INITIAL_INIT;
}

static int PokedexSearchSeq_Initial(PokedexSearchWork *wk) {
    int seq = SEQ_INITIAL;
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        if (GCTX_HIDIsUsingTouch() == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchSeq_SetInitial(wk, ret);
        }
        break;
    case -12:
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        PokedexSearchObj_SetPageButtons(wk, FALSE);
        break;
    case -7:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, 12);
        wk->buttonId = POKEDEX_SEARCH_OBJ_PAGE_UP;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -6:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, 13);
        wk->buttonId = POKEDEX_SEARCH_OBJ_PAGE_DOWN;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -5:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, 12);
        return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_PAGE_UP, seq);
    case -4:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, 13);
        return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_PAGE_DOWN, seq);
    case -3:
    case -2:
        PokedexSearchObj_HideMarks(wk);
        PokedexSearchObj_PutItemMark(wk, 0, wk->param->criteria->initial);
        wk->buttonId = BUTTON_NONE;
        wk->subSeq = 1;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_NONE:
        ret = PokedexSearchUI_ListMain(wk);
        switch (ret) {
        case POKEDEX_SEARCH_UI_LIST_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
            return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_INITIAL_EXIT);
        case POKEDEX_SEARCH_UI_LIST_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
            return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_INITIAL_EXIT);
        case POKEDEX_SEARCH_UI_LIST_RESET:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchSeq_ResetInitial(wk);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchObj_SetPageButtons(wk, FALSE);
            PokedexSearchSeq_SetInitial(wk, ret - POKEDEX_SEARCH_UI_LIST_ROW);
            break;
        }
        break;
    }
    PokedexSearchObj_UpdateScrollBar(wk);
    return seq;
}

static int PokedexSearchSeq_InitialExit(PokedexSearchWork *wk) {
    PokedexSearchMain_ClearPage(wk);
    PokedexSearchObj_HideList(wk);
    PokedexSearchList_Exit(wk);
    return SEQ_MAIN_INIT;
}

static int PokedexSearchSeq_TypeInit(PokedexSearchWork *wk) {
    switch (wk->initSeq) {
    case 0:
        wk->page = POKEDEX_SEARCH_PAGE_TYPE;
        PokedexSearchMain_SetBlend(TRUE);
        PokedexSearchBmp_PutType(wk);
        PokedexSearchList_InitType(wk);
        wk->initSeq++;
    case 1:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            PokedexSearchObj_SetPageButtons(wk, FALSE);
            PokedexSearchObj_SetType(wk);
            PokedexSearchMain_LoadTypeScreen(wk);
            PokedexSearchMain_SetListBGPriority();
            PokedexSearchMain_RequestBGOn(wk);
            wk->initSeq = 0;
            return SEQ_TYPE;
        }
        break;
    }
    return SEQ_TYPE_INIT;
}

static int PokedexSearchSeq_Type(PokedexSearchWork *wk) {
    int seq = SEQ_TYPE;
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        if (GCTX_HIDIsUsingTouch() == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchSeq_SetType(wk, ret);
        }
        break;
    case -12:
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        PokedexSearchObj_SetPageButtons(wk, FALSE);
        break;
    case -7:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, 12);
        wk->buttonId = POKEDEX_SEARCH_OBJ_PAGE_UP;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -6:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, 13);
        wk->buttonId = POKEDEX_SEARCH_OBJ_PAGE_DOWN;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -5:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, 12);
        return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_PAGE_UP, seq);
    case -4:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, 13);
        return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_PAGE_DOWN, seq);
    case -3:
    case -2:
        PokedexSearchObj_HideMarks(wk);
        PokedexSearchObj_PutItemMark(wk, 0, PokedexSearchMain_TypeToListPos(wk, wk->param->criteria->type1));
        PokedexSearchObj_PutItemMark(wk, 1, PokedexSearchMain_TypeToListPos(wk, wk->param->criteria->type2));
        wk->buttonId = BUTTON_NONE;
        wk->subSeq = 1;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_NONE:
        ret = PokedexSearchUI_ListMain(wk);
        switch (ret) {
        case POKEDEX_SEARCH_UI_LIST_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
            return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_TYPE_EXIT);
        case POKEDEX_SEARCH_UI_LIST_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
            return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_TYPE_EXIT);
        case POKEDEX_SEARCH_UI_LIST_RESET:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchSeq_ResetType(wk);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchObj_SetPageButtons(wk, FALSE);
            PokedexSearchSeq_SetType(wk, ret - POKEDEX_SEARCH_UI_LIST_ROW);
            break;
        }
        break;
    }
    PokedexSearchObj_UpdateScrollBar(wk);
    return seq;
}

static int PokedexSearchSeq_TypeExit(PokedexSearchWork *wk) {
    PokedexSearchMain_ClearPage(wk);
    PokedexSearchObj_HideList(wk);
    PokedexSearchList_Exit(wk);
    return SEQ_MAIN_INIT;
}

static int PokedexSearchSeq_ColorInit(PokedexSearchWork *wk) {
    switch (wk->initSeq) {
    case 0:
        wk->page = POKEDEX_SEARCH_PAGE_COLOR;
        PokedexSearchMain_SetBlend(TRUE);
        PokedexSearchBmp_PutColor(wk);
        PokedexSearchList_InitColor(wk);
        wk->initSeq++;
    case 1:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            PokedexSearchObj_SetPageButtons(wk, FALSE);
            PokedexSearchObj_SetColor(wk);
            PokedexSearchMain_LoadColorScreen(wk);
            PokedexSearchMain_SetListBGPriority();
            PokedexSearchMain_RequestBGOn(wk);
            wk->initSeq = 0;
            return SEQ_COLOR;
        }
        break;
    }
    return SEQ_COLOR_INIT;
}

static int PokedexSearchSeq_Color(PokedexSearchWork *wk) {
    int seq = SEQ_COLOR;
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        if (GCTX_HIDIsUsingTouch() == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchSeq_SetColor(wk, ret);
        }
        break;
    case -12:
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        PokedexSearchObj_SetPageButtons(wk, FALSE);
        break;
    case -7:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, 12);
        wk->buttonId = POKEDEX_SEARCH_OBJ_PAGE_UP;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -6:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, 13);
        wk->buttonId = POKEDEX_SEARCH_OBJ_PAGE_DOWN;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -5:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, 12);
        return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_PAGE_UP, seq);
    case -4:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, 13);
        return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_PAGE_DOWN, seq);
    case -3:
    case -2:
        PokedexSearchObj_HideMarks(wk);
        PokedexSearchObj_PutItemMark(wk, 0, wk->param->criteria->color);
        wk->buttonId = BUTTON_NONE;
        wk->subSeq = 1;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_NONE:
        ret = PokedexSearchUI_ListMain(wk);
        switch (ret) {
        case POKEDEX_SEARCH_UI_LIST_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
            return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_COLOR_EXIT);
        case POKEDEX_SEARCH_UI_LIST_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
            return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_COLOR_EXIT);
        case POKEDEX_SEARCH_UI_LIST_RESET:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchSeq_ResetColor(wk);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchObj_SetPageButtons(wk, FALSE);
            PokedexSearchSeq_SetColor(wk, ret - POKEDEX_SEARCH_UI_LIST_ROW);
            break;
        }
        break;
    }
    PokedexSearchObj_UpdateScrollBar(wk);
    return seq;
}

static int PokedexSearchSeq_ColorExit(PokedexSearchWork *wk) {
    PokedexSearchMain_ClearPage(wk);
    PokedexSearchObj_HideList(wk);
    PokedexSearchList_Exit(wk);
    return SEQ_MAIN_INIT;
}

static int PokedexSearchSeq_ShapeInit(PokedexSearchWork *wk) {
    switch (wk->initSeq) {
    case 0:
        wk->page = POKEDEX_SEARCH_PAGE_SHAPE;
        PokedexSearchMain_SetBlend(TRUE);
        PokedexSearchBmp_PutShape(wk);
        PokedexSearchList_InitShape(wk);
        wk->initSeq++;
    case 1:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            PokedexSearchObj_SetPageButtons(wk, FALSE);
            PokedexSearchObj_SetShape(wk);
            PokedexSearchMain_LoadShapeScreen(wk);
            PokedexSearchMain_SetListBGPriority();
            PokedexSearchMain_RequestBGOn(wk);
            wk->initSeq = 0;
            return SEQ_SHAPE;
        }
        break;
    }
    return SEQ_SHAPE_INIT;
}

static int PokedexSearchSeq_Shape(PokedexSearchWork *wk) {
    int seq = SEQ_SHAPE;
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
        if (GCTX_HIDIsUsingTouch() == FALSE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchSeq_SetShape(wk, ret);
        }
        break;
    case -12:
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        PokedexSearchObj_SetPageButtons(wk, FALSE);
        break;
    case -7:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, 12);
        wk->buttonId = POKEDEX_SEARCH_OBJ_PAGE_UP;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -6:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, 13);
        wk->buttonId = POKEDEX_SEARCH_OBJ_PAGE_DOWN;
        wk->subSeq = 0;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case -5:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, 12);
        return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_PAGE_UP, seq);
    case -4:
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, 13);
        return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_PAGE_DOWN, seq);
    case -3:
    case -2:
        PokedexSearchObj_PutShapeIcons(wk);
        PokedexSearchObj_HideMarks(wk);
        PokedexSearchObj_PutShapeItemMark(wk);
        wk->buttonId = BUTTON_NONE;
        wk->subSeq = 1;
        wk->nextSeq = seq;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_NONE:
        ret = PokedexSearchUI_ShapeListMain(wk);
        switch (ret) {
        case POKEDEX_SEARCH_UI_LIST_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
            return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_SHAPE_EXIT);
        case POKEDEX_SEARCH_UI_LIST_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_RETURN, 9);
            return PokedexSearchSeq_SetButtonAnm(wk, TRUE, POKEDEX_SEARCH_OBJ_RETURN, SEQ_SHAPE_EXIT);
        case POKEDEX_SEARCH_UI_LIST_RESET:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchSeq_ResetShape(wk);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexSearchObj_SetPageButtons(wk, FALSE);
            PokedexSearchSeq_SetShape(wk, ret - POKEDEX_SEARCH_UI_LIST_ROW);
            break;
        }
        break;
    }
    PokedexSearchObj_UpdateScrollBar(wk);
    return seq;
}

static int PokedexSearchSeq_ShapeExit(PokedexSearchWork *wk) {
    PokedexSearchMain_ClearPage(wk);
    PokedexSearchObj_HideList(wk);
    PokedexSearchList_Exit(wk);
    return SEQ_MAIN_INIT;
}

// Waits for the list's jump or page scroll, and for the page button's animation
static int PokedexSearchSeq_ListScroll(PokedexSearchWork *wk) {
    switch (wk->subSeq) {
    case 0:
        if (FrameList_Main(wk->frameList) == FRAMELIST_NONE) {
            wk->subSeq++;
        }
        break;
    case 1:
        if (wk->buttonId == BUTTON_NONE || PokedexSearchObj_IsAnmPlaying(wk, wk->buttonId) == FALSE) {
            wk->subSeq = 0;
            PokedexSearchObj_SetPageButtons(wk, FALSE);
            return wk->nextSeq;
        }
        break;
    }
    return SEQ_LIST_SCROLL;
}

// Flashes the cursor's position twice, or waits for a button's animation
static int PokedexSearchSeq_ButtonAnm(PokedexSearchWork *wk) {
    if (wk->buttonIsActor == FALSE) {
        switch (wk->buttonSeq) {
        case 0:
        case 2:
            if (wk->buttonTimer == 0) {
                PokedexSearchUI_SetCursorPalette(wk, wk->buttonId, POKEDEX_SEARCH_UI_PALETTE_OFF);
                wk->buttonTimer = 4;
                wk->buttonSeq++;
            } else {
                wk->buttonTimer--;
            }
            break;
        case 1:
        case 3:
            if (wk->buttonTimer == 0) {
                PokedexSearchUI_SetCursorPalette(wk, wk->buttonId, POKEDEX_SEARCH_UI_PALETTE_PRESSED);
                wk->buttonTimer = 4;
                wk->buttonSeq++;
            } else {
                wk->buttonTimer--;
            }
            break;
        case 4:
            if (GCTX_HIDIsUsingTouch() == TRUE) {
                PokedexSearchUI_SetCursorPalette(wk, wk->buttonId, POKEDEX_SEARCH_UI_PALETTE_OFF);
            } else {
                PokedexSearchUI_SetCursorPalette(wk, wk->buttonId, POKEDEX_SEARCH_UI_PALETTE_ON);
            }
            return wk->buttonNextSeq;
        }
    } else {
        if (PokedexSearchObj_IsAnmPlaying(wk, wk->buttonId) == FALSE) {
            if (wk->frameList != NULL) {
                PokedexSearchObj_SetPageButtons(wk, FALSE);
            }
            return wk->buttonNextSeq;
        }
    }
    return SEQ_BUTTON_ANM;
}

static int PokedexSearchSeq_Finish(PokedexSearchWork *wk) {
    PokedexSearchUI_ExitCursor(wk);
    PokedexSearchMain_StartFrameMove(wk, 8);
    PokedexSearchMain_StartFade(wk, 0, 16);
    return SEQ_EXIT;
}

static void PokedexSearchSeq_SetOrder(PokedexSearchWork *wk, u32 row) {
    wk->param->criteria->order = row;
    FrameList_SetCursor(wk->frameList, row);
    PokedexSearchBmp_PutOrderValue(wk);
    PokedexSearchObj_PutRowMark(wk, row, TRUE);
}

static void PokedexSearchSeq_ResetOrder(PokedexSearchWork *wk) {
    wk->param->criteria->order = 0;
    PokedexSearchBmp_PutOrderValue(wk);
    PokedexSearchObj_PutRowMark(wk, 0, TRUE);
}

// Chooses the row's initial, or none if it was the one chosen
static void PokedexSearchSeq_SetInitial(PokedexSearchWork *wk, u32 row) {
    u32 item = row + FrameList_GetScroll(wk->frameList);

    if (wk->param->criteria->initial == item) {
        wk->param->criteria->initial = POKEDEX_SEARCH_NONE;
        PokedexSearchObj_PutRowMark(wk, row, FALSE);
    } else {
        wk->param->criteria->initial = item;
        PokedexSearchObj_PutRowMark(wk, row, TRUE);
    }
    FrameList_SetCursor(wk->frameList, row);
    PokedexSearchBmp_PutInitialValue(wk);
}

static void PokedexSearchSeq_ResetInitial(PokedexSearchWork *wk) {
    wk->param->criteria->initial = POKEDEX_SEARCH_NONE;
    PokedexSearchBmp_PutInitialValue(wk);
    PokedexSearchObj_HideMarks(wk);
}

// Chooses the row's type as the first or the second, dropping the first for a third, or drops it if it was chosen
static void PokedexSearchSeq_SetType(PokedexSearchWork *wk, u32 row) {
    u32 type = PokedexSearchMain_ListPosToType(wk, row + FrameList_GetScroll(wk->frameList));

    if (wk->param->criteria->type1 == POKEDEX_SEARCH_NONE) {
        wk->param->criteria->type1 = type;
    } else if (wk->param->criteria->type2 == POKEDEX_SEARCH_NONE) {
        if (wk->param->criteria->type1 == type) {
            wk->param->criteria->type1 = POKEDEX_SEARCH_NONE;
        } else {
            wk->param->criteria->type2 = type;
        }
    } else if (wk->param->criteria->type1 == type) {
        wk->param->criteria->type1 = wk->param->criteria->type2;
        wk->param->criteria->type2 = POKEDEX_SEARCH_NONE;
    } else if (wk->param->criteria->type2 == type) {
        wk->param->criteria->type2 = POKEDEX_SEARCH_NONE;
    } else {
        wk->param->criteria->type1 = wk->param->criteria->type2;
        wk->param->criteria->type2 = type;
    }
    FrameList_SetCursor(wk->frameList, row);
    PokedexSearchBmp_PutTypeValue(wk);
    PokedexSearchObj_PutTypeMarks(wk, wk->param->criteria->type1, wk->param->criteria->type2);
}

static void PokedexSearchSeq_ResetType(PokedexSearchWork *wk) {
    wk->param->criteria->type1 = POKEDEX_SEARCH_NONE;
    wk->param->criteria->type2 = POKEDEX_SEARCH_NONE;
    PokedexSearchBmp_PutTypeValue(wk);
    PokedexSearchObj_HideMarks(wk);
}

static void PokedexSearchSeq_SetColor(PokedexSearchWork *wk, u32 row) {
    u32 item = row + FrameList_GetScroll(wk->frameList);

    if (wk->param->criteria->color == item) {
        wk->param->criteria->color = POKEDEX_SEARCH_NONE;
        PokedexSearchObj_PutRowMark(wk, row, FALSE);
    } else {
        wk->param->criteria->color = item;
        PokedexSearchObj_PutRowMark(wk, row, TRUE);
    }
    FrameList_SetCursor(wk->frameList, row);
    PokedexSearchBmp_PutColorValue(wk);
}

static void PokedexSearchSeq_ResetColor(PokedexSearchWork *wk) {
    wk->param->criteria->color = POKEDEX_SEARCH_NONE;
    PokedexSearchBmp_PutColorValue(wk);
    PokedexSearchObj_HideMarks(wk);
}

static void PokedexSearchSeq_SetShape(PokedexSearchWork *wk, u32 row) {
    u32 item = row + FrameList_GetScroll(wk->frameList);

    if (wk->param->criteria->shape == item) {
        wk->param->criteria->shape = POKEDEX_SEARCH_NONE;
        PokedexSearchObj_PutShapeRowMark(wk, row, FALSE);
    } else {
        wk->param->criteria->shape = item;
        PokedexSearchObj_PutShapeRowMark(wk, row, TRUE);
    }
    FrameList_SetCursor(wk->frameList, row);
    PokedexSearchObj_PutShape(wk);
}

static void PokedexSearchSeq_ResetShape(PokedexSearchWork *wk) {
    wk->param->criteria->shape = POKEDEX_SEARCH_NONE;
    PokedexSearchObj_PutShape(wk);
    PokedexSearchObj_HideMarks(wk);
}

static int PokedexSearchSeq_SetButtonAnm(PokedexSearchWork *wk, u32 isActor, u32 id, int next) {
    wk->buttonIsActor = isActor;
    wk->buttonId = id;
    wk->buttonSeq = 0;
    wk->buttonTimer = 0;
    wk->buttonNextSeq = next;
    return SEQ_BUTTON_ANM;
}

static int PokedexSearchSeq_SetPageEnd(PokedexSearchWork *wk, int next) {
    wk->initSeq = 0;
    wk->nextSeq = next;
    return SEQ_PAGE_END;
}

static void PokedexSearchSeq_ToggleShortcut(PokedexSearchWork *wk) {
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH) == TRUE) {
        GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH, FALSE);
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, 6);
    } else {
        GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH, TRUE);
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, 7);
    }
}
