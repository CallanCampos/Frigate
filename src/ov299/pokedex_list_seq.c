#include "types.h"
#include "app/pokedex_common.h"
#include "app/pokedex_list_bmp.h"
#include "app/pokedex_list_main.h"
#include "app/pokedex_list_obj.h"
#include "app/pokedex_list_seq.h"
#include "app/pokedex_list_ui.h"
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
#include "system/game_data.h"
#include "system/palanm.h"

// The sequences of the Pokédex's list. The ROM doesn't name this file; pokedex_list_seq.c is a guess after the search
// screen's pokedex_search_seq.c

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

// No button to wait for while the list jumps
#define BUTTON_NONE POKEDEX_LIST_OBJ_COUNT

typedef int (*PokedexListSeqFunc)(PokedexListWork *wk);

static int PokedexListSeq_Init(PokedexListWork *wk);
static int PokedexListSeq_Exit(PokedexListWork *wk);
static int PokedexListSeq_WaitFade(PokedexListWork *wk);
static int PokedexListSeq_Start(PokedexListWork *wk);
static int PokedexListSeq_List(PokedexListWork *wk);
static int PokedexListSeq_ListScroll(PokedexListWork *wk);
static int PokedexListSeq_ButtonAnm(PokedexListWork *wk);
static int PokedexListSeq_Decide(PokedexListWork *wk);
static int PokedexListSeq_Finish(PokedexListWork *wk);
static void PokedexListSeq_InitFade(PokedexListWork *wk);
static int PokedexListSeq_SetButtonAnm(PokedexListWork *wk, u32 id, u32 anm, int next);
static BOOL PokedexListSeq_IsItemKnown(PokedexListWork *wk, u32 index);
static void PokedexListSeq_SaveLastSpecies(PokedexListWork *wk, u32 index);
static int PokedexListSeq_SetDecide(PokedexListWork *wk, u32 index);
static void PokedexListSeq_ToggleShortcut(PokedexListWork *wk);

static const PokedexListSeqFunc sPokedexListSeqFuncs[] = {
    PokedexListSeq_Init,      PokedexListSeq_Exit,   PokedexListSeq_WaitFade,
    PokedexListSeq_Start,     PokedexListSeq_List,   PokedexListSeq_ListScroll,
    PokedexListSeq_ButtonAnm, PokedexListSeq_Decide, PokedexListSeq_Finish,
};

BOOL PokedexListSeq_Main(PokedexListWork *wk) {
    wk->seq = sPokedexListSeqFuncs[wk->seq](wk);
    if (wk->seq == SEQ_END) {
        return FALSE;
    }
    PokedexListObj_Update(wk);
    PokedexListBmp_Update(wk);
    PokedexCommon_ScrollBGs(3, 7, &wk->scrollTimer);
    return TRUE;
}

static int PokedexListSeq_Init(PokedexListWork *wk) {
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    GFL_BGSysSetDisplayLayout(0);
    PokedexListMain_InitPaletteFade(wk);
    PokedexListMain_InitVRAM();
    PokedexListMain_InitBG();
    PokedexListMain_LoadGraphics();
    PokedexListMain_InitMsg(wk);
    PokedexListBmp_Init(wk);
    PokedexListObj_Init(wk);
    PokedexListMain_LoadRegionalNumbers(wk);
    PokedexListMain_InitList(wk);
    if (wk->param->searched == FALSE) {
        PokedexListBmp_PutCounts(wk);
    } else {
        PokedexListBmp_PutSearchCount(wk);
    }
    GFL_NetSetIconScreen(TRUE, HEAPID_POKEDEX_LIST);
    PokedexListMain_SetBlend();
    PokedexListMain_InitWindow();
    PokedexListMain_InitFramePos(wk);
    PokedexListMain_InitVBlank(wk);
    PokedexListMain_InitHBlank(wk);
    return SEQ_START;
}

static int PokedexListSeq_Exit(PokedexListWork *wk) {
    if (PokedexListMain_UpdateFrameMove(wk) == TRUE || PaletteFade_GetActiveMask(wk->paletteFade) != 0) {
        return SEQ_EXIT;
    }
    PokedexListMain_ExitHBlank(wk);
    PokedexListMain_ExitVBlank(wk);
    PokedexListMain_ExitWindow();
    PokedexListMain_ExitList(wk);
    PokedexListMain_FreeRegionalNumbers(wk);
    PokedexListObj_Exit(wk);
    PokedexListBmp_Exit(wk);
    PokedexListMain_ExitMsg(wk);
    PokedexListMain_ExitBG();
    PokedexListMain_ExitPaletteFade(wk);
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return SEQ_END;
}

static int PokedexListSeq_WaitFade(PokedexListWork *wk) {
    if (PaletteFade_GetActiveMask(wk->paletteFade) == 0) {
        return wk->fadeNextSeq;
    }
    return SEQ_WAIT_FADE;
}

// Draws the list, then fades in while the touch bar slides in
static int PokedexListSeq_Start(PokedexListWork *wk) {
    switch (wk->initSeq) {
    case 0:
        if (FrameList_Draw(wk->frameList) == FALSE) {
            PokedexListObj_SetPageButtons(wk, FALSE);
            PokedexListObj_UpdateScrollBar(wk);
            PokedexListObj_SaveIcons(wk);
            PokedexListSeq_InitFade(wk);
            wk->initSeq++;
        }
        break;
    case 1:
        if (PaletteFade_GetActiveMask(wk->paletteFade) == 0) {
            GX_SetMasterBrightness(0);
            GXS_SetMasterBrightness(0);
            PokedexListMain_StartFade(wk, 16, 0);
            PokedexListMain_StartFrameMove(wk, -8);
            wk->initSeq++;
        }
        break;
    case 2:
        if (PokedexListMain_UpdateFrameMove(wk) == FALSE && PaletteFade_GetActiveMask(wk->paletteFade) == 0) {
            wk->initSeq = 0;
            return SEQ_LIST;
        }
        break;
    }
    return SEQ_START;
}

static int PokedexListSeq_List(PokedexListWork *wk) {
    int seq = SEQ_LIST;
    u32 ret = FrameList_Main(wk->frameList);

    switch (ret) {
    case FRAMELIST_CURSOR_SHOWN:
        break;
    case FRAMELIST_CURSOR_MOVE:
    case FRAMELIST_SCROLL:
    case FRAMELIST_BAR:
    case FRAMELIST_DRAG:
        PokedexListObj_SetPageButtons(wk, FALSE);
        break;
    case FRAMELIST_PAGE_UP:
        PokedexListObj_SetAnm(wk, POKEDEX_LIST_OBJ_PAGE_UP, 12);
        wk->buttonId = POKEDEX_LIST_OBJ_PAGE_UP;
        wk->subSeq = 0;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_PAGE_DOWN:
        PokedexListObj_SetAnm(wk, POKEDEX_LIST_OBJ_PAGE_DOWN, 13);
        wk->buttonId = POKEDEX_LIST_OBJ_PAGE_DOWN;
        wk->subSeq = 0;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_CURSOR_TOP:
        return PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_PAGE_UP, 12, seq);
    case FRAMELIST_CURSOR_BOTTOM:
        return PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_PAGE_DOWN, 13, seq);
    case FRAMELIST_JUMP_TOP:
    case FRAMELIST_JUMP_BOTTOM:
        PokedexListObj_HideSavedIcons(wk);
        wk->buttonId = BUTTON_NONE;
        wk->subSeq = 1;
        seq = SEQ_LIST_SCROLL;
        break;
    case FRAMELIST_NONE: {
        BOOL touch = GCTX_HIDIsUsingTouch();
        int ui = PokedexListUI_Main(wk);

        switch (ui) {
        case POKEDEX_LIST_UI_MON: {
            u32 index = FrameList_GetSelected(wk->frameList);

            if (PokedexListSeq_IsItemKnown(wk, index) == TRUE) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                return PokedexListSeq_SetDecide(wk, index);
            }
            GCTX_HIDSetUsingTouch(touch);
            break;
        }
        case POKEDEX_LIST_UI_ROW:
        case POKEDEX_LIST_UI_ROW + 1:
        case POKEDEX_LIST_UI_ROW + 2:
        case POKEDEX_LIST_UI_ROW + 3:
        case POKEDEX_LIST_UI_ROW + 4:
        case POKEDEX_LIST_UI_ROW + 5:
        case POKEDEX_LIST_UI_ROW + 6: {
            u32 index = FrameList_GetScroll(wk->frameList) + ui - POKEDEX_LIST_UI_ROW;

            if (PokedexListSeq_IsItemKnown(wk, index) == TRUE) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                FrameList_SetCursor(wk->frameList, ui - POKEDEX_LIST_UI_ROW);
                return PokedexListSeq_SetDecide(wk, index);
            }
            break;
        }
        case POKEDEX_LIST_UI_START:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            wk->param->result = POKEDEX_LIST_RESULT_SEARCH;
            PokedexListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            return PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_START, 4, SEQ_FINISH);
        case POKEDEX_LIST_UI_SELECT:
            // The next of the Pokédexes that the player has
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_UNOVA, 13, SEQ_FINISH);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_NATIONAL, 13, SEQ_FINISH);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_HABITAT, 13, SEQ_FINISH);
            if (PokeDex_GetMode(wk->param->pokedex) != DEX_NATIONAL) {
                if (PokeDex_IsNationalObtained(wk->param->pokedex) == TRUE) {
                    PokeDex_SetMode(wk->param->pokedex, DEX_NATIONAL);
                    wk->param->result = POKEDEX_LIST_RESULT_RELOAD;
                    PokedexListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
                    PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_NATIONAL, 14, SEQ_FINISH);
                } else {
                    wk->param->result = POKEDEX_LIST_RESULT_HABITAT;
                    PokedexListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
                    PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_HABITAT, 14, SEQ_FINISH);
                    PokeDex_SetMode(wk->param->pokedex, DEX_HABITAT);
                }
            } else {
                wk->param->result = POKEDEX_LIST_RESULT_HABITAT;
                PokedexListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
                PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_HABITAT, 14, SEQ_FINISH);
                PokeDex_SetMode(wk->param->pokedex, DEX_HABITAT);
            }
            return PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_SELECT, 5, SEQ_FINISH);
        case POKEDEX_LIST_UI_UNOVA:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_UNOVA, 13, SEQ_FINISH);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_NATIONAL, 13, SEQ_FINISH);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_HABITAT, 13, SEQ_FINISH);
            PokeDex_SetMode(wk->param->pokedex, DEX_REGIONAL);
            wk->param->result = POKEDEX_LIST_RESULT_RELOAD;
            PokedexListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            return PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_UNOVA, 42, SEQ_FINISH);
        case POKEDEX_LIST_UI_NATIONAL:
            // Without the National Pokédex its button is Unova's, and the next is the habitat list's
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_UNOVA, 13, SEQ_FINISH);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_NATIONAL, 13, SEQ_FINISH);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_HABITAT, 13, SEQ_FINISH);
            if (PokeDex_IsNationalObtained(wk->param->pokedex) == TRUE) {
                PokeDex_SetMode(wk->param->pokedex, DEX_NATIONAL);
                wk->param->result = POKEDEX_LIST_RESULT_RELOAD;
                PokedexListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            } else {
                wk->param->result = POKEDEX_LIST_RESULT_HABITAT;
                PokedexListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
                PokeDex_SetMode(wk->param->pokedex, DEX_HABITAT);
            }
            return PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_NATIONAL, 42, SEQ_FINISH);
        case POKEDEX_LIST_UI_HABITAT:
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_UNOVA, 13, SEQ_FINISH);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_NATIONAL, 13, SEQ_FINISH);
            PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_HABITAT, 13, SEQ_FINISH);
            wk->param->result = POKEDEX_LIST_RESULT_HABITAT;
            PokedexListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            PokeDex_SetMode(wk->param->pokedex, DEX_HABITAT);
            return PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_HABITAT, 42, SEQ_FINISH);
        case POKEDEX_LIST_UI_SHORTCUT:
            GFL_SndSEPlay(SEQ_SE_SYS_07);
            PokedexListSeq_ToggleShortcut(wk);
            break;
        case POKEDEX_LIST_UI_EXIT:
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            wk->param->result = POKEDEX_LIST_RESULT_CLOSE;
            PokedexListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            return PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_EXIT, 8, SEQ_FINISH);
        case POKEDEX_LIST_UI_RETURN:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            wk->param->result = POKEDEX_LIST_RESULT_BACK;
            PokedexListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            return PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_RETURN, 9, SEQ_FINISH);
        case POKEDEX_LIST_UI_CANCEL:
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            wk->param->result = POKEDEX_LIST_RESULT_BACK;
            PokedexListSeq_SaveLastSpecies(wk, FrameList_GetSelected(wk->frameList));
            return PokedexListSeq_SetButtonAnm(wk, POKEDEX_LIST_OBJ_RETURN, 9, SEQ_FINISH);
        }
        break;
    }
    default: {
        u32 index = FrameList_GetSelected(wk->frameList);

        PokedexListObj_SetPageButtons(wk, FALSE);
        if (PokedexListSeq_IsItemKnown(wk, index) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return PokedexListSeq_SetDecide(wk, index);
        }
        break;
    }
    }
    PokedexListObj_UpdateScrollBar(wk);
    PokedexListObj_SaveIcons(wk);
    return seq;
}

// Waits for the list to stop, then for the page button's animation
static int PokedexListSeq_ListScroll(PokedexListWork *wk) {
    switch (wk->subSeq) {
    case 0:
        if (FrameList_Main(wk->frameList) == FRAMELIST_NONE) {
            wk->subSeq++;
        }
        break;
    case 1:
        if (wk->buttonId == BUTTON_NONE || PokedexListObj_IsAnmPlaying(wk, wk->buttonId) == FALSE) {
            wk->subSeq = 0;
            PokedexListObj_SetPageButtons(wk, FALSE);
            return SEQ_LIST;
        }
        break;
    }
    return SEQ_LIST_SCROLL;
}

static int PokedexListSeq_ButtonAnm(PokedexListWork *wk) {
    if (PokedexListObj_IsAnmPlaying(wk, wk->buttonId) == FALSE) {
        PokedexListObj_SetPageButtons(wk, FALSE);
        return wk->buttonNextSeq;
    }
    return SEQ_BUTTON_ANM;
}

// Flashes the cursor's row twice
static int PokedexListSeq_Decide(PokedexListWork *wk) {
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

static int PokedexListSeq_Finish(PokedexListWork *wk) {
    PokedexListObj_StopBlink(wk, TRUE);
    PokedexListMain_StartFade(wk, 0, 16);
    PokedexListMain_StartFrameMove(wk, 8);
    return SEQ_EXIT;
}

// Takes the colors that the fades start from, and blacks them out
static void PokedexListSeq_InitFade(PokedexListWork *wk) {
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_MAIN_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_SUB_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0, 0x200);
    PaletteFade_LoadFromVRAM(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0, 0x200);
    PokedexListMain_StartFade(wk, 16, 16);
}

static int PokedexListSeq_SetButtonAnm(PokedexListWork *wk, u32 id, u32 anm, int next) {
    PokedexListObj_SetAnm(wk, id, anm);
    wk->buttonId = id;
    wk->buttonNextSeq = next;
    return SEQ_BUTTON_ANM;
}

// Whether a list item's species is seen
static BOOL PokedexListSeq_IsItemKnown(PokedexListWork *wk, u32 index) {
    if ((FrameList_GetValue(wk->frameList, index) & POKEDEX_LIST_ITEM_STATE_MASK) >> POKEDEX_LIST_ITEM_STATE_SHIFT !=
        0) {
        return TRUE;
    }
    return FALSE;
}

// The species that the list is on when the Pokédex opens again
static void PokedexListSeq_SaveLastSpecies(PokedexListWork *wk, u32 index) {
    if (PokedexListSeq_IsItemKnown(wk, index) == TRUE) {
        PokeDex_SetLastSpecies(wk->param->pokedex, FrameList_GetValue(wk->frameList, index));
    }
}

static int PokedexListSeq_SetDecide(PokedexListWork *wk, u32 index) {
    PokedexListSeq_SaveLastSpecies(wk, index);
    wk->param->result = POKEDEX_LIST_RESULT_DETAIL;
    return SEQ_DECIDE;
}

static void PokedexListSeq_ToggleShortcut(PokedexListWork *wk) {
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_LIST) == TRUE) {
        GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_POKEDEX_LIST, FALSE);
        PokedexListObj_SetAnm(wk, POKEDEX_LIST_OBJ_SHORTCUT, 6);
    } else {
        GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_POKEDEX_LIST, TRUE);
        PokedexListObj_SetAnm(wk, POKEDEX_LIST_OBJ_SHORTCUT, 7);
    }
}
