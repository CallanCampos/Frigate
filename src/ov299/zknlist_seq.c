#include "types.h"
#include "app/ui/frame_list.h"
#include "app/ui/ui_scene.h"
#include "app/zknlist_bmp.h"
#include "app/zknlist_main.h"
#include "app/zknlist_obj.h"
#include "app/zknlist_seq.h"
#include "app/zknlist_ui.h"
#include "app/zukan_common.h"
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
#include "save/pokedex.h"
#include "system/game_data.h"
#include "system/palanm.h"

// The sequences of the Pokédex's list. The ROM doesn't name this file; zknlist_seq.c is a guess after the search
// screen's zknsearch_seq.c

enum {
    SEQ_INIT,
    SEQ_EXIT,
    SEQ_WAIT_FADE,
    SEQ_START,
    SEQ_LIST,
    SEQ_LIST_SCROLL,
    SEQ_BUTTON_ANM,
    SEQ_DECIDE,
    SEQ_FINISH,
    SEQ_END,
};

// Which Pokédex counts, func_0200d1f8's values
#define DEX_REGIONAL 0
#define DEX_NATIONAL 1
#define DEX_HABITAT 2

// No button to wait for while the list jumps
#define BUTTON_NONE ZKNLIST_OBJ_COUNT

typedef int (*ZknListSeqFunc)(ZknListWork *wk);

static int ZknListSeq_Init(ZknListWork *wk);
static int ZknListSeq_Exit(ZknListWork *wk);
static int ZknListSeq_WaitFade(ZknListWork *wk);
static int ZknListSeq_Start(ZknListWork *wk);
static int ZknListSeq_List(ZknListWork *wk);
static int ZknListSeq_ListScroll(ZknListWork *wk);
static int ZknListSeq_ButtonAnm(ZknListWork *wk);
static int ZknListSeq_Decide(ZknListWork *wk);
static int ZknListSeq_Finish(ZknListWork *wk);
static void ZknListSeq_InitFade(ZknListWork *wk);
static int ZknListSeq_SetButtonAnm(ZknListWork *wk, u32 id, u32 anm, int next);
static BOOL ZknListSeq_IsItemKnown(ZknListWork *wk, u32 index);
static void ZknListSeq_SaveLastSpecies(ZknListWork *wk, u32 index);
static int ZknListSeq_SetDecide(ZknListWork *wk, u32 index);
static void ZknListSeq_ToggleShortcut(ZknListWork *wk);

static const ZknListSeqFunc sZknListSeqFuncs[] = {
    ZknListSeq_Init,       ZknListSeq_Exit,      ZknListSeq_WaitFade, ZknListSeq_Start,  ZknListSeq_List,
    ZknListSeq_ListScroll, ZknListSeq_ButtonAnm, ZknListSeq_Decide,   ZknListSeq_Finish,
};

BOOL ZknListSeq_Main(ZknListWork *wk) {
    wk->seq = sZknListSeqFuncs[wk->seq](wk);
    if (wk->seq == SEQ_END) {
        return FALSE;
    }
    ZknListObj_Update(wk);
    ZknListBmp_Update(wk);
    ZukanCommon_ScrollBGs(3, 7, &wk->scrollTimer);
    return TRUE;
}

static int ZknListSeq_Init(ZknListWork *wk) {
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, -16);
    GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR, -16);
    GFL_BGSysSetDisplayLayout(0);
    ZknListMain_InitPaletteFade(wk);
    ZknListMain_InitVRAM();
    ZknListMain_InitBG();
    ZknListMain_LoadGraphics();
    ZknListMain_InitMsg(wk);
    ZknListBmp_Init(wk);
    ZknListObj_Init(wk);
    ZknListMain_LoadRegionalNumbers(wk);
    ZknListMain_InitList(wk);
    if (wk->param->searched == FALSE) {
        ZknListBmp_PutCounts(wk);
    } else {
        ZknListBmp_PutSearchCount(wk);
    }
    func_02042ba8(TRUE, HEAPID_ZUKAN_LIST);
    ZknListMain_SetBlend();
    ZknListMain_InitWindow();
    ZknListMain_InitFramePos(wk);
    ZknListMain_InitVBlank(wk);
    ZknListMain_InitHBlank(wk);
    return SEQ_START;
}

static int ZknListSeq_Exit(ZknListWork *wk) {
    if (ZknListMain_UpdateFrameMove(wk) == TRUE || PaletteFade_GetActiveMask(wk->paletteFade) != 0) {
        return SEQ_EXIT;
    }
    ZknListMain_ExitHBlank(wk);
    ZknListMain_ExitVBlank(wk);
    ZknListMain_ExitWindow();
    ZknListMain_ExitList(wk);
    ZknListMain_FreeRegionalNumbers(wk);
    ZknListObj_Exit(wk);
    ZknListBmp_Exit(wk);
    ZknListMain_ExitMsg(wk);
    ZknListMain_ExitBG();
    ZknListMain_ExitPaletteFade(wk);
    GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, -16);
    GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR, -16);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return SEQ_END;
}

static int ZknListSeq_WaitFade(ZknListWork *wk) {
    if (PaletteFade_GetActiveMask(wk->paletteFade) == 0) {
        return wk->fadeNextSeq;
    }
    return SEQ_WAIT_FADE;
}

// Draws the list, then fades in while the touch bar slides in
static int ZknListSeq_Start(ZknListWork *wk) {
    switch (wk->initSeq) {
    case 0:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            ZknListObj_SetPageButtons(wk, FALSE);
            ZknListObj_UpdateScrollBar(wk);
            ZknListObj_SaveIcons(wk);
            ZknListSeq_InitFade(wk);
            wk->initSeq++;
        }
        break;
    case 1:
        if (PaletteFade_GetActiveMask(wk->paletteFade) == 0) {
            GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, 0);
            GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR, 0);
            ZknListMain_StartFade(wk, 16, 0);
            ZknListMain_StartFrameMove(wk, -8);
            wk->initSeq++;
        }
        break;
    case 2:
        if (ZknListMain_UpdateFrameMove(wk) == FALSE && PaletteFade_GetActiveMask(wk->paletteFade) == 0) {
            wk->initSeq = 0;
            return SEQ_LIST;
        }
        break;
    }
    return SEQ_START;
}

static int ZknListSeq_List(ZknListWork *wk) {
    int seq = SEQ_LIST;
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case -12:
        break;
    case -11:
    case -10:
    case -9:
    case -8:
        ZknListObj_SetPageButtons(wk, FALSE);
        break;
    case -7:
        ZknListObj_SetAnm(wk, ZKNLIST_OBJ_PAGE_UP, 12);
        wk->buttonId = ZKNLIST_OBJ_PAGE_UP;
        wk->subSeq = 0;
        seq = SEQ_LIST_SCROLL;
        break;
    case -6:
        ZknListObj_SetAnm(wk, ZKNLIST_OBJ_PAGE_DOWN, 13);
        wk->buttonId = ZKNLIST_OBJ_PAGE_DOWN;
        wk->subSeq = 0;
        seq = SEQ_LIST_SCROLL;
        break;
    case -5:
        return ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_PAGE_UP, 12, seq);
    case -4:
        return ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_PAGE_DOWN, 13, seq);
    case -3:
    case -2:
        ZknListObj_HideSavedIcons(wk);
        wk->buttonId = BUTTON_NONE;
        wk->subSeq = 1;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_NONE: {
        BOOL touch = func_0203d554();
        int ui = ZknListUI_Main(wk);

        switch (ui) {
        case ZKNLIST_UI_MON: {
            u32 index = FrameList_GetSelected(wk->frameList);

            if (ZknListSeq_IsItemKnown(wk, index) == TRUE) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                return ZknListSeq_SetDecide(wk, index);
            }
            func_0203d564(touch);
            break;
        }
        case ZKNLIST_UI_ROW:
        case ZKNLIST_UI_ROW + 1:
        case ZKNLIST_UI_ROW + 2:
        case ZKNLIST_UI_ROW + 3:
        case ZKNLIST_UI_ROW + 4:
        case ZKNLIST_UI_ROW + 5:
        case ZKNLIST_UI_ROW + 6: {
            u32 index = FrameList_GetScroll(wk->frameList) + ui - ZKNLIST_UI_ROW;

            if (ZknListSeq_IsItemKnown(wk, index) == TRUE) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                FrameList_SetCursor(wk->frameList, ui - ZKNLIST_UI_ROW);
                return ZknListSeq_SetDecide(wk, index);
            }
            break;
        }
        case ZKNLIST_UI_START:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            wk->param->result = ZUKAN_LIST_RESULT_SEARCH;
            ZknListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            return ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_START, 4, SEQ_FINISH);
        case ZKNLIST_UI_SELECT:
            // The next of the Pokédexes that the player has
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_UNOVA, 13, SEQ_FINISH);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_NATIONAL, 13, SEQ_FINISH);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_HABITAT, 13, SEQ_FINISH);
            if (func_0200d1f8(wk->param->pokedex) != DEX_NATIONAL) {
                if (PokeDex_IsNationalObtained(wk->param->pokedex) == TRUE) {
                    func_0200d1e4(wk->param->pokedex, DEX_NATIONAL);
                    wk->param->result = ZUKAN_LIST_RESULT_RELOAD;
                    ZknListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
                    ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_NATIONAL, 14, SEQ_FINISH);
                } else {
                    wk->param->result = ZUKAN_LIST_RESULT_HABITAT;
                    ZknListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
                    ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_HABITAT, 14, SEQ_FINISH);
                    func_0200d1e4(wk->param->pokedex, DEX_HABITAT);
                }
            } else {
                wk->param->result = ZUKAN_LIST_RESULT_HABITAT;
                ZknListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
                ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_HABITAT, 14, SEQ_FINISH);
                func_0200d1e4(wk->param->pokedex, DEX_HABITAT);
            }
            return ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_SELECT, 5, SEQ_FINISH);
        case ZKNLIST_UI_UNOVA:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_UNOVA, 13, SEQ_FINISH);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_NATIONAL, 13, SEQ_FINISH);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_HABITAT, 13, SEQ_FINISH);
            func_0200d1e4(wk->param->pokedex, DEX_REGIONAL);
            wk->param->result = ZUKAN_LIST_RESULT_RELOAD;
            ZknListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            return ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_UNOVA, 42, SEQ_FINISH);
        case ZKNLIST_UI_NATIONAL:
            // Without the National Pokédex its button is Unova's, and the next is the habitat list's
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_UNOVA, 13, SEQ_FINISH);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_NATIONAL, 13, SEQ_FINISH);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_HABITAT, 13, SEQ_FINISH);
            if (PokeDex_IsNationalObtained(wk->param->pokedex) == TRUE) {
                func_0200d1e4(wk->param->pokedex, DEX_NATIONAL);
                wk->param->result = ZUKAN_LIST_RESULT_RELOAD;
                ZknListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            } else {
                wk->param->result = ZUKAN_LIST_RESULT_HABITAT;
                ZknListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
                func_0200d1e4(wk->param->pokedex, DEX_HABITAT);
            }
            return ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_NATIONAL, 42, SEQ_FINISH);
        case ZKNLIST_UI_HABITAT:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_UNOVA, 13, SEQ_FINISH);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_NATIONAL, 13, SEQ_FINISH);
            ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_HABITAT, 13, SEQ_FINISH);
            wk->param->result = ZUKAN_LIST_RESULT_HABITAT;
            ZknListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            func_0200d1e4(wk->param->pokedex, DEX_HABITAT);
            return ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_HABITAT, 42, SEQ_FINISH);
        case ZKNLIST_UI_SHORTCUT:
            GFL_SndSEPlay(SEQ_SE_SYS_07);
            ZknListSeq_ToggleShortcut(wk);
            break;
        case ZKNLIST_UI_EXIT:
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            wk->param->result = ZUKAN_LIST_RESULT_CLOSE;
            ZknListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            return ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_EXIT, 8, SEQ_FINISH);
        case ZKNLIST_UI_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            wk->param->result = ZUKAN_LIST_RESULT_BACK;
            ZknListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            return ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_RETURN, 9, SEQ_FINISH);
        case ZKNLIST_UI_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            wk->param->result = ZUKAN_LIST_RESULT_BACK;
            ZknListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            return ZknListSeq_SetButtonAnm(wk, ZKNLIST_OBJ_RETURN, 9, SEQ_FINISH);
        }
        break;
    }
    default: {
        u32 index = FrameList_GetSelected(wk->frameList);

        ZknListObj_SetPageButtons(wk, FALSE);
        if (ZknListSeq_IsItemKnown(wk, index) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return ZknListSeq_SetDecide(wk, index);
        }
        break;
    }
    }
    ZknListObj_UpdateScrollBar(wk);
    ZknListObj_SaveIcons(wk);
    return seq;
}

// Waits for the list to stop, then for the page button's animation
static int ZknListSeq_ListScroll(ZknListWork *wk) {
    switch (wk->subSeq) {
    case 0:
        if (FrameList_Main(wk->frameList) == FRAMELIST_NONE) {
            wk->subSeq++;
        }
        break;
    case 1:
        if (wk->buttonId == BUTTON_NONE || ZknListObj_IsAnmPlaying(wk, wk->buttonId) == FALSE) {
            wk->subSeq = 0;
            ZknListObj_SetPageButtons(wk, FALSE);
            return SEQ_LIST;
        }
        break;
    }
    return SEQ_LIST_SCROLL;
}

static int ZknListSeq_ButtonAnm(ZknListWork *wk) {
    if (ZknListObj_IsAnmPlaying(wk, wk->buttonId) == FALSE) {
        ZknListObj_SetPageButtons(wk, FALSE);
        return wk->buttonNextSeq;
    }
    return SEQ_BUTTON_ANM;
}

// Flashes the cursor's row twice
static int ZknListSeq_Decide(ZknListWork *wk) {
    switch (wk->decideSeq) {
    case 0:
    case 2:
        if (wk->decideTimer == 0) {
            FrameList_SetRowPalette(wk->frameList, (u16)FrameList_GetCursor(wk->frameList), 1);
            wk->decideTimer = 4;
            wk->decideSeq++;
        } else {
            wk->decideTimer--;
        }
        break;
    case 1:
    case 3:
        if (wk->decideTimer == 0) {
            FrameList_SetRowPalette(wk->frameList, (u16)FrameList_GetCursor(wk->frameList), 2);
            GFL_BGSysQueueScrLoad(2);
            wk->decideTimer = 4;
            wk->decideSeq++;
        } else {
            wk->decideTimer--;
        }
        break;
    case 4:
        if (wk->decideTimer == 0) {
            wk->decideSeq = 0;
            return SEQ_FINISH;
        } else {
            wk->decideTimer--;
        }
        break;
    }
    return SEQ_DECIDE;
}

static int ZknListSeq_Finish(ZknListWork *wk) {
    ZknListObj_StopBlink(wk, TRUE);
    ZknListMain_StartFade(wk, 0, 16);
    ZknListMain_StartFrameMove(wk, 8);
    return SEQ_EXIT;
}

// Takes the colors that the fades start from, and blacks them out
static void ZknListSeq_InitFade(ZknListWork *wk) {
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_MAIN_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_SUB_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0, 0x200);
    ZknListMain_StartFade(wk, 16, 16);
}

static int ZknListSeq_SetButtonAnm(ZknListWork *wk, u32 id, u32 anm, int next) {
    ZknListObj_SetAnm(wk, id, anm);
    wk->buttonId = id;
    wk->buttonNextSeq = next;
    return SEQ_BUTTON_ANM;
}

// Whether a list item's species is seen
static BOOL ZknListSeq_IsItemKnown(ZknListWork *wk, u32 index) {
    if ((FrameList_GetValue(wk->frameList, index) & ZKNLIST_ITEM_STATE_MASK) >> ZKNLIST_ITEM_STATE_SHIFT != 0) {
        return TRUE;
    }
    return FALSE;
}

// The species that the list is on when the Pokédex opens again
static void ZknListSeq_SaveLastSpecies(ZknListWork *wk, u32 index) {
    if (ZknListSeq_IsItemKnown(wk, index) == TRUE) {
        func_0200d200(wk->param->pokedex, FrameList_GetValue(wk->frameList, index));
    }
}

static int ZknListSeq_SetDecide(ZknListWork *wk, u32 index) {
    ZknListSeq_SaveLastSpecies(wk, index);
    wk->param->result = ZUKAN_LIST_RESULT_DETAIL;
    return SEQ_DECIDE;
}

static void ZknListSeq_ToggleShortcut(ZknListWork *wk) {
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_LIST) == TRUE) {
        GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_POKEDEX_LIST, FALSE);
        ZknListObj_SetAnm(wk, ZKNLIST_OBJ_SHORTCUT, 6);
    } else {
        GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_POKEDEX_LIST, TRUE);
        ZknListObj_SetAnm(wk, ZKNLIST_OBJ_SHORTCUT, 7);
    }
}
