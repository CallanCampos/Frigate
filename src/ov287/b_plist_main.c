#include "battle/b_plist_main.h"
#include "battle/b_app_tool.h"
#include "battle/b_plist_anm.h"
#include "battle/b_plist_bmp.h"
#include "battle/b_plist_cursor.h"
#include "battle/b_plist_obj.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/hm_check.h"
#include "pml/item.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "pml/waza.h"
#include "system/app_menu_common.h"
#include "system/bgwinfrm.h"
#include "system/bmp_winframe.h"
#include "system/cursor_move.h"
#include "system/gf_font.h"
#include "system/hp_gauge.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/egg_demo.h"

// The battle party list's entry, task and states (b_plist_main.c, named by the ROM's embedded string): the list
// copies the party's data once, then runs one state function a frame, each returning the next state, until state
// 0x21 frees everything and sets the parameter's done. The pages are 0 the list, 1 the chosen Pokémon, 2 its
// summary, 3 its moves, 4 one move, 5 the move to restore PP to, 6 the move to forget, 7 the confirmation and 8 the
// position to fill.

static void BPlistMain_Task(TCB *tcb, void *data);
static int BPlistMain_StateInit(BPlistWork *work);
static int BPlistMain_StateStartWipe(BPlistWork *work);
static int BPlistMain_StateWaitWipe(BPlistWork *work);
static int BPlistMain_StatePartyPage(BPlistWork *work);
static int BPlistMain_SelectItemTarget(BPlistWork *work);
static int BPlistMain_StateSelectedPage(BPlistWork *work);
static int BPlistMain_StateSummaryPage(BPlistWork *work);
static int BPlistMain_StateMovesPage(BPlistWork *work);
static int BPlistMain_StateMoveInfoPage(BPlistWork *work);
static int BPlistMain_StateMoveInfoOnly(BPlistWork *work);
static int BPlistMain_StateForgetPage(BPlistWork *work);
static int BPlistMain_StateConfirmPage(BPlistWork *work);
static int BPlistMain_StateHMMessage(BPlistWork *work);
static int BPlistMain_StatePPRestorePage(BPlistWork *work);
static int BPlistMain_StateToPage0(BPlistWork *work);
static int BPlistMain_StateToPage1(BPlistWork *work);
static int BPlistMain_StateToPage2(BPlistWork *work);
static int BPlistMain_StateToPage3(BPlistWork *work);
static int BPlistMain_StateToPage4(BPlistWork *work);
static int BPlistMain_StateToPage6(BPlistWork *work);
static int BPlistMain_StateToPage7(BPlistWork *work);
static int BPlistMain_StateToPage5(BPlistWork *work);
static int BPlistMain_StateRedrawPage(BPlistWork *work);
static int BPlistMain_StateOpenSwitchMessage(BPlistWork *work);
static int BPlistMain_StateCloseMessage(BPlistWork *work);
static int BPlistMain_StatePrintMessage(BPlistWork *work);
static int BPlistMain_StateWaitMessageKey(BPlistWork *work);
static int BPlistMain_StateWaitButtonAnm(BPlistWork *work);
static int BPlistMain_StateStartExit(BPlistWork *work);
static int BPlistMain_StateWaitFadeOut(BPlistWork *work);
static BOOL BPlistMain_Exit(TCB *tcb, BPlistWork *work);
static void BPlistMain_InitBG(BPlistWork *work);
static void BPlistMain_ExitBG(void);
static void BPlistMain_LoadGraphics(BPlistWork *work);
static void BPlistMain_InitText(BPlistWork *work);
static void BPlistMain_ExitText(BPlistWork *work);
static void BPlistMain_SetPokemon(BPlistWork *work, PartyPkm *pkm, BPlistPokemon *row);
static void BPlistMain_InitPokemon(BPlistWork *work);
static BOOL BPlistMain_UpdatePartyCursor(BPlistWork *work);
static u8 BPlistMain_GetNextPos(BPlistWork *work, int pos, int dir);
static void BPlistMain_DrawExpBar(BPlistWork *work, u8 page);
static BOOL BPlistMain_ChangePage(BPlistWork *work, u8 page);
static void BPlistMain_LoadPageScrn(BPlistWork *work, u8 page);
static BOOL BPlistMain_CanSwitch(BPlistWork *work);
static u8 BPlistMain_IsEgg(BPlistWork *work);
static BOOL BPlistMain_IsMulti(BPlistWork *work);
static BOOL BPlistMain_IsHMMove(BPlistWork *work);
static BOOL BPlistMain_IsInBattle(BPlistWork *work, int pos);
static BOOL BPlistMain_IsBattlePos(BPlistWork *work, u8 pos);
static BOOL BPlistMain_IsChosen(BPlistWork *work, u8 pos);
static void BPlistMain_InitPageCursor(BPlistWork *work, u8 page);
static void BPlistMain_InitOrder(BPlistWork *work);
static void BPlistMain_SwapOrder(BPlistWork *work, u8 pos1, u8 pos2);
static BOOL BPlistMain_IsSlotChosen(BPlistWork *work, u8 slot);
static void BPlistMain_SetChosenSlot(BPlistWork *work, u8 idx, u8 pos);
static void BPlistMain_PushSwap(BPlistWork *work, u8 pos1, u8 pos2);
static int BPlistMain_StateToPage8(BPlistWork *work);
static BOOL BPlistMain_CanPlace(BPlistWork *work);
static int BPlistMain_StatePositionPage(BPlistWork *work);
static int BPlistMain_StateCloseSwapMessage(BPlistWork *work);
static void BPlistMain_GetPlatePos(u8 *x, u8 *y, u8 pos);
static void BPlistMain_CopyPlateScrn(u16 *buf, u8 bg, u8 pos);
static void BPlistMain_MovePlateIcons(BPlistWork *work, u8 pos, BOOL back);
static BOOL BPlistMain_SwapPlatesAnm(BPlistWork *work);
static int BPlistMain_StateSwapAnm(BPlistWork *work);
static BOOL BPlistMain_HasTwoFainted(BPlistWork *work);
static BOOL BPlistMain_HasReserve(BPlistWork *work);
static void BPlistMain_ChooseFaintedPos(BPlistWork *work);
static void BPlistMain_ClearChosen(BPlistWork *work);
static BOOL BPlistMain_CheckQuit(BPlistWork *work);
static void BPlistMain_PlaySE(BPlistWork *work, u32 se);
static void BPlistMain_PlayMessageSE(BPlistWork *work, u32 se);

// The NSCR files of BGs 6 and 7, by page
static const u32 data_ov287_021fb50c[9][2] = {
    { 1, 0 }, { 15, 14 }, { 5, 4 }, { 3, 2 }, { 7, 6 }, { 9, 8 }, { 11, 10 }, { 13, 12 }, { 1, 0 },
};

// The list position of each party slot in a multi battle, by the player's column
static const u8 data_ov287_021fb470[2][6] = {
    { 0, 3, 1, 4, 2, 5 },
    { 3, 0, 4, 1, 5, 2 },
};

static int (*const data_ov287_021fb554[])(BPlistWork *work) = {
    BPlistMain_StateInit,
    BPlistMain_StateStartWipe,
    BPlistMain_StateWaitWipe,
    BPlistMain_StatePartyPage,
    BPlistMain_StateSelectedPage,
    BPlistMain_StateSummaryPage,
    BPlistMain_StateMovesPage,
    BPlistMain_StateMoveInfoPage,
    BPlistMain_StatePositionPage,
    BPlistMain_StateSwapAnm,
    BPlistMain_StateCloseSwapMessage,
    BPlistMain_StateToPage0,
    BPlistMain_StateToPage1,
    BPlistMain_StateToPage2,
    BPlistMain_StateToPage3,
    BPlistMain_StateToPage4,
    BPlistMain_StateToPage6,
    BPlistMain_StateToPage7,
    BPlistMain_StateToPage5,
    BPlistMain_StateToPage8,
    BPlistMain_StateRedrawPage,
    BPlistMain_StateOpenSwitchMessage,
    BPlistMain_StateCloseMessage,
    BPlistMain_StatePrintMessage,
    BPlistMain_StateWaitMessageKey,
    BPlistMain_StateForgetPage,
    BPlistMain_StateConfirmPage,
    BPlistMain_StateHMMessage,
    BPlistMain_StatePPRestorePage,
    BPlistMain_StateWaitButtonAnm,
    BPlistMain_StateMoveInfoOnly,
    BPlistMain_StateStartExit,
    BPlistMain_StateWaitFadeOut,
};

void BPlistMain_Start(BPlistParam *param) {
    BPlistWork *work;

    if (param->partyIndex > 5) {
        param->partyIndex = 0;
    }
    work = GFL_HeapAllocate(param->heapId, sizeof(BPlistWork), TRUE, "b_plist_main.c", 304);
    GFL_TCBMgrAddTask(param->tcbManager, BPlistMain_Task, work, 100);
    work->param = param;
    if (param->unk1F != 5) {
        param->slot = 0;
    } else if (BPlistMain_IsMulti(work) == TRUE && param->unk1C == 1) {
        param->partyIndex++;
    }
    work->paletteFade = param->paletteFade;
    work->seq = 0;
    work->startPos = param->partyIndex;
    work->initialized = FALSE;
}

static void BPlistMain_Task(TCB *tcb, void *data) {
    BPlistWork *work = data;

    if (work->param->unk34 == 1) {
        BPlistMain_ClearChosen(work);
        work->seq = 0x21;
    }
    if (work->seq != 0x21) {
        work->seq = data_ov287_021fb554[work->seq](work);
    }
    if (work->seq == 0x21 && BPlistMain_Exit(tcb, work) == TRUE) {
        return;
    }
    GFL_TCBExMgrUpdate(work->tcbExMgr);
    BPlistObj_UpdatePokeIconAnims(work);
    BPlistAnm_MainButtonAnm(work);
    BPlistBmp_FlushWindows(work);
}

static int BPlistMain_StateInit(BPlistWork *work) {
    G2S_BlendNone();
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    work->tcbExMgr = GFL_TCBExMgrCreate(work->param->heapId, work->param->heapId, 1, 4);
    if (work->param->unk1F == 4) {
        work->page = 6;
    } else if (work->param->unk1F == 5) {
        work->page = 4;
    } else {
        work->page = 0;
    }
    work->cursor = BAppCursor_Create(work->param->heapId);
    BPlistMain_InitOrder(work);
    BPlistMain_InitPokemon(work);
    BPlistMain_InitBG(work);
    BPlistMain_LoadGraphics(work);
    BPlistMain_InitText(work);
    BPlistMain_LoadPageScrn(work, work->page);
    BPlistAnm_PutPageButtons(work, work->page);
    BPlistAnm_RestorePalette(work, work->page);
    BPlistObj_Init(work);
    if (work->param->unk1F == 5) {
        BPlistObj_ShowMoveTypes(work);
    }
    BPlistObj_SetPage(work, work->page);
    BPlistBmp_Init(work);
    BPlistBmp_DrawPage(work, work->page);
    BPlistBmp_TransferPage(work);
    if (*work->param->usingKeys == 1) {
        work->cursorVisible = TRUE;
    } else {
        work->cursorVisible = FALSE;
    }
    BAppCursor_SetVisible(work->cursor, work->cursorVisible);
    if (work->page == 0) {
        if (BPlistMain_IsPartnerSlot(work, BPlistMain_GetPartySlot(work, 0)) == TRUE) {
            work->param->partyIndex = 1;
        }
        BPlistCursor_Create(work, work->page, work->param->partyIndex);
    } else if (work->param->unk1F == 5) {
        BPlistCursor_Create(work, work->page, work->param->slot);
    } else {
        BPlistCursor_Create(work, work->page, 0);
    }
    BPlistMain_DrawExpBar(work, work->page);
    PaletteFade_StartFade(work->paletteFade, 0xa, 0xffff, 0, 16, 0, 0, work->param->tcbManager);
    work->initialized = TRUE;
    return 1;
}

static int BPlistMain_StateStartWipe(BPlistWork *work) {
    if (func_02021c0c(work->printQueue) == TRUE) {
        GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, work->param->heapId);
        return 2;
    }
    return 1;
}

static int BPlistMain_StateWaitWipe(BPlistWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        if (work->param->unk1F == 4) {
            return 0x19;
        }
        if (work->param->unk1F == 5) {
            return 0x1e;
        }
        return 3;
    }
    return 2;
}

static int BPlistMain_StatePartyPage(BPlistWork *work) {
    u8 pos1;
    u8 pos2;

    if (PaletteFade_GetActiveMask(work->paletteFade) != 0) {
        return 3;
    }
    if (BPlistMain_CheckQuit(work) == TRUE) {
        return 0x1f;
    }
    if (BPlistMain_UpdatePartyCursor(work) == TRUE) {
        if (work->param->partyIndex == 6) {
            if (work->param->unk1F == 2) {
                if (BPlistMain_PopSwap(work, &pos1, &pos2, TRUE) == TRUE) {
                    if (pos1 < pos2) {
                        work->param->unk48[pos1] = 0xff;
                    } else {
                        work->param->unk48[pos2] = 0xff;
                    }
                    work->swapAnimPos[0] = pos1;
                    work->swapAnimPos[1] = pos2;
                    work->animSeq = 0;
                    BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
                    return 9;
                }
            } else if (work->param->unk1F != 1) {
                BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
                BPlistAnm_StartButtonAnm(work, 6);
                BPlistMain_ClearChosen(work);
                return 0x1f;
            }
        } else {
            BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
            BPlistAnm_StartButtonAnm(work, work->param->partyIndex);
            if (work->param->unk1F == 3) {
                return BPlistMain_SelectItemTarget(work);
            }
            work->nextSeq = 0xc;
            return 0x1d;
        }
    }
    return 3;
}

static int BPlistMain_SelectItemTarget(BPlistWork *work) {
    BPlistParam *param = work->param;

    if (work->pokemon[BPlistMain_GetPartySlot(work, param->partyIndex)].isEgg) {
        GFL_MsgDataLoadStrbuf(work->msgData, EggDemo_Text_WontHaveAnyEffect, work->strBuf);
        BPlistBmp_OpenMessage(work);
        work->param->partyIndex = 6;
        work->nextSeq = 0x1f;
        BPlistMain_ClearChosen(work);
        return 0x17;
    }
    if (param->unk38[BPlistMain_GetPartySlot(work, param->partyIndex)] != 0) {
        BPlistBmp_SetEmbargoMessage(work);
        BPlistBmp_OpenMessage(work);
        work->param->partyIndex = 6;
        work->nextSeq = 0x1f;
        BPlistMain_ClearChosen(work);
        return 0x17;
    }
    if (GetItemParam(param->item, ITEM_PARAM_PP_RESTORE, param->heapId) != 0 &&
        GetItemParam(param->item, ITEM_PARAM_PP_RESTORE_ALL, param->heapId) == 0) {
        work->nextSeq = 0x12;
        return 0x1d;
    }
    param->unk48[0] = BPlistMain_GetPartySlot(work, param->partyIndex);
    return 0x1f;
}

static int BPlistMain_StateSelectedPage(BPlistWork *work) {
    BPlistParam *param;

    if (BPlistMain_CheckQuit(work) == TRUE) {
        return 0x1f;
    }
    switch (CursorMove_Update(work->cursorMove)) {
    case 0:
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, 7);
        if (BPlistMain_CanSwitch(work) == TRUE) {
            param = work->param;
            if (param->unk1F != 2) {
                param->unk48[param->unk21] = BPlistMain_GetPartySlot(work, param->partyIndex);
                return 0x1f;
            }
            if (BPlistMain_HasTwoFainted(work) == FALSE) {
                BPlistMain_ChooseFaintedPos(work);
                return 0x1f;
            }
            work->nextSeq = 0x13;
            return 0x1d;
        }
        work->nextSeq = 0x15;
        return 0x1d;
    case 1:
        if (BPlistMain_IsEgg(work) == TRUE) {
            break;
        }
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, 8);
        work->nextSeq = 0xd;
        return 0x1d;
    case 2:
        if (BPlistMain_IsEgg(work) == TRUE) {
            break;
        }
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, 10);
        work->nextSeq = 0xe;
        return 0x1d;
    case 3:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->nextSeq = 0xb;
        return 0x1d;
    case CURSOR_MOVE_CANCEL:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->nextSeq = 0xb;
        return 0x1d;
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        BPlistMain_PlaySE(work, SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_EDGE_RIGHT:
    case CURSOR_MOVE_EDGE_LEFT:
    case CURSOR_MOVE_EDGE_DOWN:
    case CURSOR_MOVE_EDGE_UP:
        break;
    }
    return 4;
}

static int BPlistMain_StateSummaryPage(BPlistWork *work) {
    u8 pos;

    if (BPlistMain_CheckQuit(work) == TRUE) {
        return 0x1f;
    }
    switch (CursorMove_Update(work->cursorMove)) {
    case 0:
        pos = BPlistMain_GetNextPos(work, work->param->partyIndex, -1);
        if (pos == 0xff) {
            break;
        }
        work->param->partyIndex = pos;
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, 12);
        work->nextSeq = 0x14;
        return 0x1d;
    case 1:
        pos = BPlistMain_GetNextPos(work, work->param->partyIndex, 1);
        if (pos == 0xff) {
            break;
        }
        work->param->partyIndex = pos;
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, 13);
        work->nextSeq = 0x14;
        return 0x1d;
    case 2:
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, 11);
        work->nextSeq = 0xe;
        return 0x1d;
    case 3:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->page1Pos = 1;
        work->nextSeq = 0xc;
        return 0x1d;
    case CURSOR_MOVE_CANCEL:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->page1Pos = 1;
        work->nextSeq = 0xc;
        return 0x1d;
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        BPlistMain_PlaySE(work, SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_EDGE_RIGHT:
    case CURSOR_MOVE_EDGE_LEFT:
    case CURSOR_MOVE_EDGE_DOWN:
    case CURSOR_MOVE_EDGE_UP:
        break;
    }
    return 5;
}

static int BPlistMain_StateMovesPage(BPlistWork *work) {
    u32 ret;
    u8 pos;

    if (BPlistMain_CheckQuit(work) == TRUE) {
        return 0x1f;
    }
    ret = CursorMove_Update(work->cursorMove);
    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
        if (work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].moves[ret].move == 0) {
            break;
        }
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, ret + 14);
        work->param->slot = ret;
        work->nextSeq = 0xf;
        return 0x1d;
    case 4:
        pos = BPlistMain_GetNextPos(work, work->param->partyIndex, -1);
        if (pos == 0xff) {
            break;
        }
        work->param->partyIndex = pos;
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, 12);
        work->nextSeq = 0x14;
        return 0x1d;
    case 5:
        pos = BPlistMain_GetNextPos(work, work->param->partyIndex, 1);
        if (pos == 0xff) {
            break;
        }
        work->param->partyIndex = pos;
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, 13);
        work->nextSeq = 0x14;
        return 0x1d;
    case 6:
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, 9);
        work->nextSeq = 0xd;
        return 0x1d;
    case 7:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->page1Pos = 2;
        work->nextSeq = 0xc;
        return 0x1d;
    case CURSOR_MOVE_CANCEL:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->page1Pos = 2;
        work->nextSeq = 0xc;
        return 0x1d;
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        BPlistMain_PlaySE(work, SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_EDGE_RIGHT:
    case CURSOR_MOVE_EDGE_LEFT:
    case CURSOR_MOVE_EDGE_DOWN:
    case CURSOR_MOVE_EDGE_UP:
        break;
    }
    return 6;
}

static int BPlistMain_StateMoveInfoPage(BPlistWork *work) {
    u32 ret;

    if (BPlistMain_CheckQuit(work) == TRUE) {
        return 0x1f;
    }
    ret = CursorMove_Update(work->cursorMove);
    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
        if (work->param->slot == ret) {
            break;
        }
        if (work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].moves[ret].move == 0) {
            break;
        }
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        work->param->slot = ret;
        return 0xf;
    case 4:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->nextSeq = 0xe;
        return 0x1d;
    case CURSOR_MOVE_CANCEL:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->nextSeq = 0xe;
        return 0x1d;
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        BPlistMain_PlaySE(work, SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_EDGE_RIGHT:
    case CURSOR_MOVE_EDGE_LEFT:
    case CURSOR_MOVE_EDGE_DOWN:
    case CURSOR_MOVE_EDGE_UP:
        break;
    }
    return 7;
}

static int BPlistMain_StateMoveInfoOnly(BPlistWork *work) {
    u32 ret;

    if (PaletteFade_GetActiveMask(work->paletteFade) != 0) {
        return 0x1e;
    }
    if (BPlistMain_CheckQuit(work) == TRUE) {
        return 0x1f;
    }
    ret = CursorMove_Update(work->cursorMove);
    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
        if (work->param->slot == ret) {
            break;
        }
        if (work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].moves[ret].move == 0) {
            break;
        }
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        work->param->slot = ret;
        return 0xf;
    case 4:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        return 0x1f;
    case CURSOR_MOVE_CANCEL:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        return 0x1f;
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        BPlistMain_PlaySE(work, SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_EDGE_RIGHT:
    case CURSOR_MOVE_EDGE_LEFT:
    case CURSOR_MOVE_EDGE_DOWN:
    case CURSOR_MOVE_EDGE_UP:
        break;
    }
    return 0x1e;
}

static int BPlistMain_StateForgetPage(BPlistWork *work) {
    u32 ret;

    if (PaletteFade_GetActiveMask(work->paletteFade) != 0) {
        return 0x19;
    }
    ret = CursorMove_Update(work->cursorMove);
    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        work->param->slot = ret;
        work->page6Pos = ret;
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, ret + 22);
        work->nextSeq = 0x11;
        return 0x1d;
    case 5:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        work->param->slot = 4;
        BPlistAnm_StartButtonAnm(work, 6);
        return 0x1f;
    case CURSOR_MOVE_CANCEL:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        work->param->slot = 4;
        BPlistAnm_StartButtonAnm(work, 6);
        return 0x1f;
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        BPlistMain_PlaySE(work, SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_EDGE_RIGHT:
    case CURSOR_MOVE_EDGE_LEFT:
    case CURSOR_MOVE_EDGE_DOWN:
    case CURSOR_MOVE_EDGE_UP:
        break;
    }
    return 0x19;
}

static int BPlistMain_StateConfirmPage(BPlistWork *work) {
    switch (CursorMove_Update(work->cursorMove)) {
    case 0:
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, 27);
        if (BPlistMain_IsHMMove(work) == FALSE) {
            work->nextSeq = 0x1f;
        } else {
            work->nextSeq = 0x1b;
        }
        return 0x1d;
    case 1:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->page7Pos = 0;
        work->nextSeq = 0x10;
        return 0x1d;
    case CURSOR_MOVE_CANCEL:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->page7Pos = 0;
        work->nextSeq = 0x10;
        return 0x1d;
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        BPlistMain_PlaySE(work, SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_EDGE_RIGHT:
    case CURSOR_MOVE_EDGE_LEFT:
    case CURSOR_MOVE_EDGE_DOWN:
    case CURSOR_MOVE_EDGE_UP:
        break;
    }
    return 0x1a;
}

static int BPlistMain_StateHMMessage(BPlistWork *work) {
    BPlistBmp_PrintMessage(work);
    work->msgWinOpen = TRUE;
    work->nextSeq = 0x1a;
    return 0x17;
}

static int BPlistMain_StatePPRestorePage(BPlistWork *work) {
    u32 ret;
    u8 slot;

    if (BPlistMain_CheckQuit(work) == TRUE) {
        return 0x1f;
    }
    ret = CursorMove_Update(work->cursorMove);
    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
        slot = BPlistMain_GetPartySlot(work, work->param->partyIndex);
        if (work->pokemon[slot].moves[ret].move == 0) {
            break;
        }
        work->param->slot = ret;
        work->param->unk48[0] = slot;
        BPlistMain_PlaySE(work, SEQ_SE_DECIDE2);
        BPlistAnm_StartButtonAnm(work, ret + 18);
        return 0x1f;
    case 4:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->nextSeq = 0xb;
        return 0x1d;
    case CURSOR_MOVE_CANCEL:
        BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
        BPlistAnm_StartButtonAnm(work, 6);
        work->nextSeq = 0xb;
        return 0x1d;
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        BPlistMain_PlaySE(work, SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_EDGE_RIGHT:
    case CURSOR_MOVE_EDGE_LEFT:
    case CURSOR_MOVE_EDGE_DOWN:
    case CURSOR_MOVE_EDGE_UP:
        break;
    }
    return 0x1c;
}

static int BPlistMain_StateToPage0(BPlistWork *work) {
    if (BPlistMain_ChangePage(work, 0) == FALSE) {
        return 0xb;
    }
    return 3;
}

static int BPlistMain_StateToPage1(BPlistWork *work) {
    if (BPlistMain_ChangePage(work, 1) == FALSE) {
        return 0xc;
    }
    return 4;
}

static int BPlistMain_StateToPage2(BPlistWork *work) {
    if (BPlistMain_ChangePage(work, 2) == FALSE) {
        return 0xd;
    }
    return 5;
}

static int BPlistMain_StateToPage3(BPlistWork *work) {
    if (BPlistMain_ChangePage(work, 3) == FALSE) {
        return 0xe;
    }
    return 6;
}

static int BPlistMain_StateToPage4(BPlistWork *work) {
    if (BPlistMain_ChangePage(work, 4) == FALSE) {
        return 0xf;
    }
    if (work->param->unk1F == 5) {
        return 0x1e;
    }
    return 7;
}

static int BPlistMain_StateToPage6(BPlistWork *work) {
    if (BPlistMain_ChangePage(work, 6) == FALSE) {
        return 0x10;
    }
    return 0x19;
}

static int BPlistMain_StateToPage7(BPlistWork *work) {
    if (BPlistMain_ChangePage(work, 7) == FALSE) {
        return 0x11;
    }
    return 0x1a;
}

static int BPlistMain_StateToPage5(BPlistWork *work) {
    if (BPlistMain_ChangePage(work, 5) == FALSE) {
        return 0x12;
    }
    return 0x1c;
}

static int BPlistMain_StateRedrawPage(BPlistWork *work) {
    BPlistObj_SetPage(work, work->page);
    BPlistBmp_DrawPage(work, work->page);
    BPlistAnm_PutPageButtons(work, work->page);
    BPlistMain_DrawExpBar(work, work->page);
    if (work->page == 2) {
        return 5;
    }
    return 6;
}

static int BPlistMain_StateOpenSwitchMessage(BPlistWork *work) {
    BPlistBmp_OpenMessage(work);
    work->nextSeq = 0x16;
    return 0x17;
}

static int BPlistMain_StateCloseMessage(BPlistWork *work) {
    BmpWin_ClearFrame(work->msgWins[1].window, 0);
    return 4;
}

static int BPlistMain_StatePrintMessage(BPlistWork *work) {
    switch (func_020223b4(work->printStream)) {
    case PRINT_STREAM_RUNNING:
        if (func_0203da48() == TRUE || (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            func_020223e0(work->printStream, 0);
        }
        work->streamAdvanced = FALSE;
        break;
    case PRINT_STREAM_PAUSED:
        if (work->streamAdvanced == FALSE &&
            (func_0203da48() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)))) {
            BPlistMain_PlayMessageSE(work, SEQ_SE_MESSAGE);
            func_020223bc(work->printStream);
            work->streamAdvanced = TRUE;
        }
        break;
    case PRINT_STREAM_DONE:
        func_020223cc(work->printStream);
        work->printStream = NULL;
        work->streamAdvanced = FALSE;
        return 0x18;
    }
    return 0x17;
}

static int BPlistMain_StateWaitMessageKey(BPlistWork *work) {
    if (BPlistMain_CheckQuit(work) == TRUE) {
        return 0x1f;
    }
    if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48() == TRUE) {
        if (work->msgWinOpen == TRUE) {
            BmpWin_ClearFrame(work->msgWins[1].window, 0);
            work->msgWinOpen = FALSE;
        }
        return work->nextSeq;
    }
    return 0x18;
}

static int BPlistMain_StateWaitButtonAnm(BPlistWork *work) {
    if (work->animActive == FALSE) {
        return work->nextSeq;
    }
    return 0x1d;
}

static int BPlistMain_StateStartExit(BPlistWork *work) {
    if (BPlistMain_IsMulti(work) == TRUE && work->param->partyIndex != 6) {
        work->param->partyIndex /= 2;
    }
    PaletteFade_StartFade(work->paletteFade, 0xa, 0xffff, 0, 0, 16, 0, work->param->tcbManager);
    return 0x20;
}

static int BPlistMain_StateWaitFadeOut(BPlistWork *work) {
    if (PaletteFade_GetActiveMask(work->paletteFade) == 0) {
        return 0x21;
    }
    return 0x20;
}

static BOOL BPlistMain_Exit(TCB *tcb, BPlistWork *work) {
    if (work->initialized == TRUE) {
        if (func_02021c0c(work->printQueue) == FALSE) {
            if (work->param->unk34 == 1) {
                func_02021c44(work->printQueue);
            } else {
                return FALSE;
            }
        }
        if (GFL_WipeIsFinished() == FALSE) {
            if (work->param->unk34 == 1) {
                GFL_WipeForceEnd();
            } else {
                return FALSE;
            }
        }
        if (work->printStream != NULL) {
            func_020223cc(work->printStream);
        }
        if (work->plateFrames != NULL) {
            BGWinFrame_Delete(work->plateFrames);
        }
        BPlistMain_ExitText(work);
        BPlistObj_Exit(work);
        BPlistBmp_Exit(work);
        BPlistMain_ExitBG();
        GFL_TCBExMgrFree(work->tcbExMgr);
        BPlistCursor_Delete(work);
        BAppCursor_Delete(work->cursor);
        if (work->cursorVisible == TRUE) {
            *work->param->usingKeys = TRUE;
        } else {
            *work->param->usingKeys = FALSE;
        }
    }
    work->param->done = TRUE;
    GFL_TCBRemove(tcb);
    GFL_HeapFree(work);
    return TRUE;
}

static void BPlistMain_InitBG(BPlistWork *work) {
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

        GFL_BGSysSetLCDConfigForEngine(&lcdConfig, BGSYS_ENGINE_SUB);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_23,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_23,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(5);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x18000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(4);
    }
    GFL_BGSysClearCharCore(5, BGSYS_TILE_SIZE_16, 0, work->param->heapId);
    GFL_BGSysClearCharCore(4, BGSYS_TILE_SIZE_16, 0, work->param->heapId);
    GFL_BGSysQueueScrLoad(5);
    GFL_BGSysQueueScrLoad(4);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
}

static void BPlistMain_ExitBG(void) {
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                           FALSE);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(7);
}

static void BPlistMain_LoadGraphics(BPlistWork *work) {
    ArcTool *arc;
    NNSG2dScreenData *scrn;
    void *buf;

    arc = GFL_ArcSysCreateFileHandle(99, HEAPID_TAIL(work->param->heapId));
    GFL_BGSysLoadArcNCGRStatic(arc, 18, 7, 0, 0, TRUE, work->param->heapId);
    buf = GFL_G2DIOReadNSCRArc(arc, 16, TRUE, &scrn, work->param->heapId);
    BPlistAnm_CutButtonScrn(work, scrn->rawData);
    GFL_HeapFree(buf);
    buf = GFL_G2DIOReadNSCRArc(arc, 17, TRUE, &scrn, work->param->heapId);
    BPlistAnm_CutPageScrn(work, scrn->rawData);
    GFL_HeapFree(buf);
#ifdef BLACK2
    PaletteFade_LoadNCLR(work->paletteFade, 99, 20, work->param->heapId, PALFADE_BUFFER_SUB_BG, 0x1e0, 0);
#else
    PaletteFade_LoadNCLR(work->paletteFade, 99, 19, work->param->heapId, PALFADE_BUFFER_SUB_BG, 0x1e0, 0);
#endif
    GFL_ArcToolFree(arc);
    sys_memcpy(PaletteFade_GetUnfadedBuffer(work->paletteFade, PALFADE_BUFFER_SUB_BG) + 0xc0, work->savedPalette,
               sizeof(work->savedPalette));
    LoadSysMsgBox(4, 1, 14, 0, work->param->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, PALFADE_VRAM_SUB_BG, 0xe0, 0x20);
    PaletteFade_LoadNCLR(work->paletteFade, ARCID_FONT, 5, work->param->heapId, PALFADE_BUFFER_SUB_BG, 0x20, 0xd0);
}

static void BPlistMain_InitText(BPlistWork *work) {
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EGG_DEMO, work->param->heapId);
    work->smallFont = GFL_FontCreate(ARCID_FONT, 3, 0, FALSE, work->param->heapId);
    work->wordSet = GFL_WordSetSystemCreateDefault(work->param->heapId);
    work->printQueue = func_020219a8(0x800, work->param->heapId);
    work->strBuf = GFL_StrBufCreate(0x200, work->param->heapId);
}

static void BPlistMain_ExitText(BPlistWork *work) {
    GFL_MsgDataFree(work->msgData);
    GFL_FontFree(work->smallFont);
    GFL_WordSetSystemFree(work->wordSet);
    func_02021a18(work->printQueue);
    GFL_StrBufFree(work->strBuf);
}

// NONMATCHING: narrows PokeParty_GetSex's result before the sex bit field, which the original doesn't (it
// matches once PokeParty_GetSex returns u8)
static void BPlistMain_SetPokemon(BPlistWork *work, PartyPkm *pkm, BPlistPokemon *row) {
    BPlistMove *move;
    u32 i;

    row->pkm = pkm;
    if (pkm == NULL) {
        row->species = 0;
        return;
    }
    row->species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    if (row->species == 0) {
        return;
    }
    row->attack = PokeParty_GetParam(row->pkm, PKM_PARAM_ATTACK, NULL);
    row->defense = PokeParty_GetParam(row->pkm, PKM_PARAM_DEFENSE, NULL);
    row->speed = PokeParty_GetParam(row->pkm, PKM_PARAM_SPEED, NULL);
    row->spAttack = PokeParty_GetParam(row->pkm, PKM_PARAM_SP_ATTACK, NULL);
    row->spDefense = PokeParty_GetParam(row->pkm, PKM_PARAM_SP_DEFENSE, NULL);
    row->hp = PokeParty_GetParam(row->pkm, PKM_PARAM_HP, NULL);
    row->maxHp = PokeParty_GetParam(row->pkm, PKM_PARAM_MAX_HP, NULL);
    row->type1 = PokeParty_GetParam(row->pkm, PKM_PARAM_TYPE1, NULL);
    row->type2 = PokeParty_GetParam(row->pkm, PKM_PARAM_TYPE2, NULL);
    row->level = PokeParty_GetParam(row->pkm, PKM_PARAM_LEVEL, NULL);
    if (PokeParty_GetParam(row->pkm, PKM_PARAM_SHOW_SEX, NULL) == TRUE) {
        row->hideSex = FALSE;
    } else {
        row->hideSex = TRUE;
    }
    row->sex = PokeParty_GetSex(row->pkm);
    row->status = (u8)AppMenuCommon_GetStatusIcon(row->pkm);
    row->isEgg = (u8)PokeParty_GetParam(row->pkm, PKM_PARAM_IS_EGG, NULL);
    row->ability = PokeParty_GetParam(row->pkm, PKM_PARAM_ABILITY, NULL);
    row->item = PokeParty_GetParam(row->pkm, PKM_PARAM_ITEM, NULL);
    row->form = PokeParty_GetParam(row->pkm, PKM_PARAM_FORM, NULL);
    row->exp = PokeParty_GetParam(row->pkm, PKM_PARAM_EXP, NULL);
    row->levelExp = getExpForPkm_Wrapper(row->pkm);
    if (row->level == 100) {
        row->nextLevelExp = row->levelExp;
    } else {
        row->nextLevelExp = PML_UtilGetPkmLvExp(row->species, row->form, row->level + 1);
    }
    for (i = 0; i < 4; i++) {
        move = &row->moves[i];
        move->move = PokeParty_GetParam(row->pkm, PKM_PARAM_MOVE1 + i, NULL);
        if (move->move == 0) {
            continue;
        }
        move->pp = PokeParty_GetParam(row->pkm, PKM_PARAM_MOVE1_PP + i, NULL);
        move->maxPp = PokeParty_GetParam(row->pkm, PKM_PARAM_MOVE1_PP_UP + i, NULL);
        move->maxPp = PML_MoveGetMaxPP(move->move, move->maxPp);
        move->type = PML_MoveGetParam(move->move, MOVE_PARAM_TYPE);
        move->category = PML_MoveGetParam(move->move, MOVE_PARAM_CATEGORY);
        if (PML_MoveIsAlwaysHit(move->move) == TRUE) {
            move->accuracy = 0;
        } else {
            move->accuracy = PML_MoveGetParam(move->move, MOVE_PARAM_ACCURACY);
        }
        move->power = PML_MoveGetParam(move->move, MOVE_PARAM_POWER);
    }
}

// NONMATCHING: in the multi battle loop the original keeps i * 0x4c in r6 across the calls and adds 0xe4 to it
// for the partner's row, where this computes (i + 3) * 0x4c
static void BPlistMain_InitPokemon(BPlistWork *work) {
    u32 i;

    if (work->param->unk18 == FALSE) {
        for (i = 0; i < 6; i++) {
            if (i < PokeParty_GetPkmCount(work->param->party)) {
                BPlistMain_SetPokemon(work, PokeParty_GetPkm(work->param->party, i), &work->pokemon[i]);
            } else {
                BPlistMain_SetPokemon(work, NULL, &work->pokemon[i]);
            }
        }
    } else {
        for (i = 0; i < 3; i++) {
            if (i < PokeParty_GetPkmCount(work->param->party)) {
                BPlistMain_SetPokemon(work, PokeParty_GetPkm(work->param->party, i), &work->pokemon[i]);
            } else {
                BPlistMain_SetPokemon(work, NULL, &work->pokemon[i]);
            }
            if (i < PokeParty_GetPkmCount(work->param->unk8)) {
                BPlistMain_SetPokemon(work, PokeParty_GetPkm(work->param->unk8, i), &work->pokemon[i + 3]);
            } else {
                BPlistMain_SetPokemon(work, NULL, &work->pokemon[i + 3]);
            }
        }
    }
}

static BOOL BPlistMain_UpdatePartyCursor(BPlistWork *work) {
    u32 ret = CursorMove_Update(work->cursorMove);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        if (BPlistMain_CheckPos(work, ret) == 0) {
            break;
        }
        work->param->partyIndex = ret;
        return TRUE;
    case 6:
        work->param->partyIndex = 6;
        return TRUE;
    case CURSOR_MOVE_CANCEL:
        work->param->partyIndex = 6;
        return TRUE;
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        BPlistMain_PlaySE(work, SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_EDGE_RIGHT:
    case CURSOR_MOVE_EDGE_LEFT:
    case CURSOR_MOVE_EDGE_DOWN:
    case CURSOR_MOVE_EDGE_UP:
        break;
    }
    return FALSE;
}

int BPlistMain_CheckPos(BPlistWork *work, int pos) {
    if (work->pokemon[BPlistMain_GetPartySlot(work, pos)].species == 0) {
        return 0;
    }
    if (BPlistMain_IsInBattle(work, pos) == TRUE) {
        return 1;
    }
    return 2;
}

static u8 BPlistMain_GetNextPos(BPlistWork *work, int pos, int dir) {
    int start = pos;
    u8 next;

    if (BPlistMain_IsMulti(work) == TRUE) {
        u8 order[6] = { 0, 2, 4, 1, 3, 5 };

        for (pos = 0; pos < 6; pos++) {
            if (start == order[pos]) {
                break;
            }
        }
        while (TRUE) {
            pos += dir;
            if (pos < 0) {
                pos = 5;
            } else if (pos >= 6) {
                pos = 0;
            }
            next = order[pos];
            if (start == next) {
                break;
            }
            if (BPlistMain_CheckPos(work, next) != 0 && !work->pokemon[BPlistMain_GetPartySlot(work, next)].isEgg) {
                return next;
            }
        }
    } else {
        while (TRUE) {
            pos += dir;
            if (pos < 0) {
                pos = 5;
            } else if (pos >= 6) {
                pos = 0;
            }
            if (start == pos) {
                break;
            }
            if (BPlistMain_CheckPos(work, pos) != 0 && !work->pokemon[BPlistMain_GetPartySlot(work, pos)].isEgg) {
                return pos;
            }
        }
    }
    return 0xff;
}

static void BPlistMain_DrawExpBar(BPlistWork *work, u8 page) {
    BPlistPokemon *row;
    u32 exp;
    u32 levelSpan;
    u8 fill;
    u16 tile;
    u8 i;

    if (page != 2) {
        return;
    }
    row = &work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)];
    if (row->level < 100) {
        levelSpan = row->nextLevelExp - row->levelExp;
        exp = row->exp - row->levelExp;
    } else {
        levelSpan = 0;
        exp = 0;
    }
    fill = HPGauge_GetFill(exp, levelSpan, 64);
    for (i = 0; i < 8; i++) {
        if (fill >= 8) {
            tile = 0x1e;
        } else {
            tile = fill + 0x16;
        }
        GFL_BGSysFillScrArea(7, tile, i + 10, 8, 1, 1, 16);
        if (fill < 8) {
            fill = 0;
        } else {
            fill -= 8;
        }
    }
    GFL_BGSysQueueScrLoad(7);
}

static BOOL BPlistMain_ChangePage(BPlistWork *work, u8 page) {
    switch (work->pageChangeSeq) {
    case 0:
        if (func_02021c0c(work->printQueue) == TRUE) {
            BPlistBmp_FreePageWindows(work);
            BPlistBmp_CreatePageWindows(work, page);
            BPlistBmp_DrawPage(work, page);
            work->pageChangeSeq++;
        }
        break;
    case 1:
        if (func_02021c0c(work->printQueue) == TRUE) {
            BPlistMain_LoadPageScrn(work, page);
            GFL_BGSysFillScrAsync(4, 0);
            GFL_BGSysFillScrAsync(5, 0);
            BPlistBmp_TransferPage(work);
            BPlistMain_DrawExpBar(work, page);
            BPlistObj_SetPage(work, page);
            BPlistMain_InitPageCursor(work, page);
            BPlistAnm_PutPageButtons(work, page);
            BPlistAnm_RestorePalette(work, page);
            work->page = page;
            work->pageChangeSeq = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void BPlistMain_LoadPageScrn(BPlistWork *work, u8 page) {
    NNSG2dScreenData *scrn;
    void *buf;
    u32 i;

    for (i = 0; i < 2; i++) {
        buf = GFL_G2DIOReadNSCR(99, data_ov287_021fb50c[page][i], TRUE, &scrn, work->param->heapId);
        GFL_BGSysLoadScrAreaAll(i + 6, scrn->rawData, 0, 0, 32, 24);
        GFL_BGSysQueueScrLoad(i + 6);
        GFL_HeapFree(buf);
    }
}

int BPlistMain_GetSwitchError(BPlistWork *work) {
    u8 slot = BPlistMain_GetPartySlot(work, work->param->partyIndex);
    BPlistPokemon *row = &work->pokemon[slot];

    if (BPlistMain_IsPartnerSlot(work, slot) == TRUE) {
        return 1;
    }
    if (row->hp == 0) {
        return 2;
    }
    if (BPlistMain_IsInBattle(work, work->param->partyIndex) == TRUE) {
        return 3;
    }
    if (BPlistMain_IsEgg(work) == TRUE) {
        return 5;
    }
    if (BPlistMain_IsChosen(work, work->param->partyIndex) == TRUE) {
        return 4;
    }
    if (work->param->move != 0) {
        return 6;
    }
    return 0;
}

static BOOL BPlistMain_CanSwitch(BPlistWork *work) {
    StrBuf *str;
    BPlistPokemon *row = &work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)];

    switch (BPlistMain_GetSwitchError(work)) {
    case 1:
        str = GFL_MsgDataLoadStrbufNew(work->msgData, EggDemo_Text_CannotDecidePartner);
        break;
    case 2:
        str = GFL_MsgDataLoadStrbufNew(work->msgData, EggDemo_Text_HasNoEnergyLeft);
        loadPokemonNicknameToStrbuf(work->wordSet, 0, row->pkm);
        break;
    case 3:
        str = GFL_MsgDataLoadStrbufNew(work->msgData, EggDemo_Text_AlreadyBattle);
        loadPokemonNicknameToStrbuf(work->wordSet, 0, row->pkm);
        break;
    case 4:
        str = GFL_MsgDataLoadStrbufNew(work->msgData, EggDemo_Text_HasAlreadyBeenSelected);
        loadPokemonNicknameToStrbuf(work->wordSet, 0, row->pkm);
        break;
    case 5:
        str = GFL_MsgDataLoadStrbufNew(work->msgData, EggDemo_Text_EggCantBattle);
        break;
    case 6:
        row = &work->pokemon[work->param->unk21];
        str = GFL_MsgDataLoadStrbufNew(work->msgData, EggDemo_Text_CantSwitchedOut);
        loadPokemonNicknameToStrbuf(work->wordSet, 0, row->pkm);
        break;
    case 0:
        return TRUE;
    }
    GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, str);
    GFL_StrBufFree(str);
    return FALSE;
}

static u8 BPlistMain_IsEgg(BPlistWork *work) {
    return work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].isEgg ? TRUE : FALSE;
}

static BOOL BPlistMain_IsMulti(BPlistWork *work) {
    return work->param->unk18;
}

BOOL BPlistMain_IsPartnerSlot(BPlistWork *work, u32 idx) {
    if (BPlistMain_IsMulti(work) == TRUE && idx >= 3) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BPlistMain_IsHMMove(BPlistWork *work) {
    BPlistParam *param = work->param;

    if (param->slot == 4) {
        return FALSE;
    }
    return isPkmMoveHmMove(param->gameData, work->pokemon[param->partyIndex].moves[param->slot].move, param->heapId);
}

static BOOL BPlistMain_IsInBattle(BPlistWork *work, int pos) {
    u8 slot = BPlistMain_GetPartySlot(work, pos);

    if (BPlistMain_IsMulti(work) == TRUE) {
        if (slot == 0 || slot == 3) {
            return TRUE;
        }
        return FALSE;
    }
    if (slot < work->param->unk22) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BPlistMain_IsBattlePos(BPlistWork *work, u8 pos) {
    if (pos < work->param->unk22) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BPlistMain_IsChosen(BPlistWork *work, u8 pos) {
    u8 slot = BPlistMain_GetPartySlot(work, pos);
    BPlistParam *param = work->param;

    if (param->unk22 != 1) {
        if (slot == param->unk1D[0] || slot == param->unk1D[1]) {
            return TRUE;
        }
        return BPlistMain_IsSlotChosen(work, slot);
    }
    return FALSE;
}

static void BPlistMain_InitPageCursor(BPlistWork *work, u8 page) {
    u8 pos;

    switch (page) {
    case 0:
    case 8:
        work->page1Pos = 0;
        work->param->slot = 0;
        pos = work->param->partyIndex;
        break;
    case 1:
        work->param->slot = 0;
        pos = work->page1Pos;
        break;
    case 3:
    case 4:
        pos = work->param->slot;
        break;
    case 6:
        pos = work->page6Pos;
        break;
    case 7:
        pos = work->page7Pos;
        break;
    case 2:
    case 5:
        pos = 0;
        break;
    }
    BPlistCursor_ChangePage(work, page, pos);
}

static void BPlistMain_InitOrder(BPlistWork *work) {
    u32 i;

    if (work->param->unk18 == TRUE) {
        for (i = 0; i < 6; i++) {
            work->partyOrder[i] = data_ov287_021fb470[work->param->unk1C][i];
        }
    } else {
        for (i = 0; i < 6; i++) {
            work->partyOrder[i] = i;
        }
    }
    for (i = 0; i < 3; i++) {
        work->param->unk48[i] = 0xff;
    }
    for (i = 0; i < 2; i++) {
        work->swaps[i][0] = 0xff;
        work->swaps[i][1] = 0xff;
    }
}

static void BPlistMain_SwapOrder(BPlistWork *work, u8 pos1, u8 pos2) {
    u8 tmp = work->partyOrder[pos1];

    work->partyOrder[pos1] = work->partyOrder[pos2];
    work->partyOrder[pos2] = tmp;
}

static BOOL BPlistMain_IsSlotChosen(BPlistWork *work, u8 slot) {
    u32 i;

    for (i = 0; i < 3; i++) {
        if (slot == work->param->unk48[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

u8 BPlistMain_GetPartySlot(BPlistWork *work, int pos) {
    return work->partyOrder[pos];
}

static void BPlistMain_SetChosenSlot(BPlistWork *work, u8 idx, u8 pos) {
    work->param->unk48[idx] = BPlistMain_GetPartySlot(work, pos);
}

static void BPlistMain_PushSwap(BPlistWork *work, u8 pos1, u8 pos2) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (work->swaps[i][0] == 0xff) {
            work->swaps[i][0] = pos1;
            work->swaps[i][1] = pos2;
            return;
        }
    }
}

BOOL BPlistMain_PopSwap(BPlistWork *work, u8 *pos1, u8 *pos2, BOOL clear) {
    int i;

    for (i = 1; i >= 0; i--) {
        if (work->swaps[i][0] != 0xff) {
            *pos1 = work->swaps[i][0];
            *pos2 = work->swaps[i][1];
            if (clear == TRUE) {
                work->swaps[i][0] = 0xff;
                work->swaps[i][1] = 0xff;
            }
            return TRUE;
        }
    }
    return FALSE;
}

static int BPlistMain_StateToPage8(BPlistWork *work) {
    if (BPlistMain_ChangePage(work, 8) == FALSE) {
        work->swapPos = work->param->partyIndex;
        return 0x13;
    }
    return 8;
}

static BOOL BPlistMain_CanPlace(BPlistWork *work) {
    StrBuf *str;
    u8 slot = BPlistMain_GetPartySlot(work, work->param->partyIndex);
    BPlistPokemon *row = &work->pokemon[slot];

    if (BPlistMain_IsPartnerSlot(work, slot) == TRUE) {
        str = GFL_MsgDataLoadStrbufNew(work->msgData, EggDemo_Text_CannotDecidePartner);
        GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, str);
        GFL_StrBufFree(str);
        return FALSE;
    }
    if (BPlistMain_IsBattlePos(work, work->param->partyIndex) == FALSE) {
        if (BPlistMain_IsBattlePos(work, slot) == TRUE) {
            str = GFL_MsgDataLoadStrbufNew(work->msgData, EggDemo_Text_AlreadySwitchedOut);
        } else {
            str = GFL_MsgDataLoadStrbufNew(work->msgData, EggDemo_Text_CantSwitchedOut_2);
        }
        loadPokemonNicknameToStrbuf(work->wordSet, 0, row->pkm);
        GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, str);
        GFL_StrBufFree(str);
        return FALSE;
    }
    if (BPlistMain_IsChosen(work, work->param->partyIndex) == TRUE) {
        str = GFL_MsgDataLoadStrbufNew(work->msgData, EggDemo_Text_AlreadyBattle);
        loadPokemonNicknameToStrbuf(work->wordSet, 0, row->pkm);
        GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, str);
        GFL_StrBufFree(str);
        return FALSE;
    }
    if (row->hp != 0) {
        str = GFL_MsgDataLoadStrbufNew(work->msgData, EggDemo_Text_AlreadyBattle);
        loadPokemonNicknameToStrbuf(work->wordSet, 0, row->pkm);
        GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, str);
        GFL_StrBufFree(str);
        return FALSE;
    }
    return TRUE;
}

static int BPlistMain_StatePositionPage(BPlistWork *work) {
    if (PaletteFade_GetActiveMask(work->paletteFade) != 0) {
        return 8;
    }
    if (BPlistMain_CheckQuit(work) == TRUE) {
        return 0x1f;
    }
    if (BPlistMain_UpdatePartyCursor(work) == TRUE) {
        if (work->param->partyIndex == 6) {
            BPlistMain_PlaySE(work, SEQ_SE_CANCEL2);
            BPlistAnm_StartButtonAnm(work, 6);
            work->param->partyIndex = work->swapPos;
            work->nextSeq = 0xc;
            return 0x1d;
        }
        if (work->param->partyIndex == work->swapPos) {
            return 0xc;
        }
        if (BPlistMain_CanPlace(work) == TRUE) {
            BPlistMain_PushSwap(work, work->param->partyIndex, work->swapPos);
            BPlistMain_SetChosenSlot(work, work->param->partyIndex, work->swapPos);
            work->swapAnimPos[0] = work->param->partyIndex;
            work->swapAnimPos[1] = work->swapPos;
            work->animSeq = 0;
            return 9;
        }
        BPlistBmp_OpenMessage(work);
        work->nextSeq = 0xa;
        return 0x17;
    }
    return 8;
}

static int BPlistMain_StateCloseSwapMessage(BPlistWork *work) {
    BmpWin_ClearFrame(work->msgWins[1].window, 0);
    BPlistBmp_PrintInfo10(work);
    BPlistBmp_DrawInfoFrame(work);
    return 8;
}

static void BPlistMain_GetPlatePos(u8 *x, u8 *y, u8 pos) {
    *x = (pos & 1) * 16;
    *y = (pos & 1) + pos / 2 * 6;
}

static void BPlistMain_CopyPlateScrn(u16 *buf, u8 bg, u8 pos) {
    u16 *scrn = GFL_BGSysIsScrHeapExists(bg);
    u8 x;
    u8 y;
    u8 i;

    BPlistMain_GetPlatePos(&x, &y, pos);
    for (i = 0; i < 6; i++) {
        sys_memcpy16(&scrn[(y + i) * 32 + x], &buf[i * 16], 16 * sizeof(u16));
    }
}

static void BPlistMain_MovePlateIcons(BPlistWork *work, u8 pos, BOOL back) {
    s16 dx;

    if (pos & 1) {
        if (back == FALSE) {
            dx = 8;
        } else {
            dx = -8;
        }
    } else {
        if (back == FALSE) {
            dx = -8;
        } else {
            dx = 8;
        }
    }
    BPlistObj_MovePlate(work, pos, dx);
}

static BOOL BPlistMain_SwapPlatesAnm(BPlistWork *work) {
    u16 *buf;
    u8 x;
    u8 y;
    s8 x1;
    s8 y1;
    s8 x2;
    s8 y2;
    s8 x3;
    s8 y3;
    s8 x4;
    s8 y4;

    switch (work->animSeq) {
    case 0:
        work->plateFrames = BGWinFrame_Create(BGWINFRAME_TRANSFER_VBLANK, 4, work->param->heapId);
        BGWinFrame_InitFrame(work->plateFrames, 0, 5, 16, 6);
        BGWinFrame_InitFrame(work->plateFrames, 1, 6, 16, 6);
        BGWinFrame_InitFrame(work->plateFrames, 2, 5, 16, 6);
        BGWinFrame_InitFrame(work->plateFrames, 3, 6, 16, 6);
        buf = GFL_HeapAllocate(HEAPID_TAIL(work->param->heapId), 16 * 6 * sizeof(u16), FALSE, "b_plist_main.c", 2971);
        BPlistMain_CopyPlateScrn(buf, 5, work->swapAnimPos[0]);
        BGWinFrame_SetScreen(work->plateFrames, 0, buf);
        BPlistMain_CopyPlateScrn(buf, 6, work->swapAnimPos[0]);
        BGWinFrame_SetScreen(work->plateFrames, 1, buf);
        BPlistMain_CopyPlateScrn(buf, 5, work->swapAnimPos[1]);
        BGWinFrame_SetScreen(work->plateFrames, 2, buf);
        BPlistMain_CopyPlateScrn(buf, 6, work->swapAnimPos[1]);
        BGWinFrame_SetScreen(work->plateFrames, 3, buf);
        GFL_HeapFree(buf);
        BPlistMain_GetPlatePos(&x, &y, work->swapAnimPos[0]);
        BGWinFrame_Put(work->plateFrames, 0, x, y);
        BGWinFrame_Put(work->plateFrames, 1, x, y);
        BPlistMain_GetPlatePos(&x, &y, work->swapAnimPos[1]);
        BGWinFrame_Put(work->plateFrames, 2, x, y);
        BGWinFrame_Put(work->plateFrames, 3, x, y);
        BGWinFrame_UpdateMoves(work->plateFrames);
        work->animSeq++;
        break;
    case 1:
        if (work->swapAnimPos[0] & 1) {
            BGWinFrame_StartMove(work->plateFrames, 0, 1, 0, 16);
            BGWinFrame_StartMove(work->plateFrames, 1, 1, 0, 16);
        } else {
            BGWinFrame_StartMove(work->plateFrames, 0, -1, 0, 16);
            BGWinFrame_StartMove(work->plateFrames, 1, -1, 0, 16);
        }
        if (work->swapAnimPos[1] & 1) {
            BGWinFrame_StartMove(work->plateFrames, 2, 1, 0, 16);
            BGWinFrame_StartMove(work->plateFrames, 3, 1, 0, 16);
        } else {
            BGWinFrame_StartMove(work->plateFrames, 2, -1, 0, 16);
            BGWinFrame_StartMove(work->plateFrames, 3, -1, 0, 16);
        }
        work->animSeq++;
        break;
    case 2:
        BGWinFrame_UpdateMoves(work->plateFrames);
        BPlistMain_MovePlateIcons(work, work->swapAnimPos[0], FALSE);
        BPlistMain_MovePlateIcons(work, work->swapAnimPos[1], FALSE);
        BGWinFrame_GetPos(work->plateFrames, 0, &x1, &y1);
        BGWinFrame_GetPos(work->plateFrames, 2, &x2, &y2);
        if (BGWinFrame_IsMoving(work->plateFrames, 0)) {
            break;
        }
        BPlistMain_SwapOrder(work, work->swapAnimPos[0], work->swapAnimPos[1]);
        BGWinFrame_Put(work->plateFrames, 0, x2, y2);
        BGWinFrame_Put(work->plateFrames, 1, x2, y2);
        BGWinFrame_Put(work->plateFrames, 2, x1, y1);
        BGWinFrame_Put(work->plateFrames, 3, x1, y1);
        BPlistObj_SwapPlates(work, work->swapAnimPos[0], work->swapAnimPos[1]);
        work->animSeq++;
        break;
    case 3:
        BGWinFrame_UpdateMoves(work->plateFrames);
        if (work->swapAnimPos[1] & 1) {
            BGWinFrame_StartMove(work->plateFrames, 0, -1, 0, 16);
            BGWinFrame_StartMove(work->plateFrames, 1, -1, 0, 16);
        } else {
            BGWinFrame_StartMove(work->plateFrames, 0, 1, 0, 16);
            BGWinFrame_StartMove(work->plateFrames, 1, 1, 0, 16);
        }
        if (work->swapAnimPos[0] & 1) {
            BGWinFrame_StartMove(work->plateFrames, 2, -1, 0, 16);
            BGWinFrame_StartMove(work->plateFrames, 3, -1, 0, 16);
        } else {
            BGWinFrame_StartMove(work->plateFrames, 2, 1, 0, 16);
            BGWinFrame_StartMove(work->plateFrames, 3, 1, 0, 16);
        }
        work->animSeq++;
        break;
    case 4:
        BGWinFrame_UpdateMoves(work->plateFrames);
        BPlistMain_MovePlateIcons(work, work->swapAnimPos[0], TRUE);
        BPlistMain_MovePlateIcons(work, work->swapAnimPos[1], TRUE);
        BGWinFrame_GetPos(work->plateFrames, 0, &x3, &y3);
        BGWinFrame_GetPos(work->plateFrames, 2, &x4, &y4);
        if (BGWinFrame_IsMoving(work->plateFrames, 0)) {
            break;
        }
        work->animSeq++;
        break;
    case 5:
        BGWinFrame_Delete(work->plateFrames);
        work->plateFrames = NULL;
        work->animSeq = 0;
        return FALSE;
    }
    return TRUE;
}

static int BPlistMain_StateSwapAnm(BPlistWork *work) {
    if (BPlistMain_SwapPlatesAnm(work) == FALSE) {
        if (BPlistMain_HasReserve(work) == FALSE) {
            return 0x1f;
        }
        BPlistAnm_PutReturnButton(work);
        BPlistBmp_PrintInfo9(work);
        BPlistBmp_DrawInfoFrame(work);
        return 3;
    }
    return 9;
}

static BOOL BPlistMain_HasTwoFainted(BPlistWork *work) {
    u8 count;
    u8 fainted;
    u8 i;

    if (BPlistMain_IsMulti(work) == TRUE) {
        return FALSE;
    }
    count = work->param->unk22;
    if (count == 1) {
        return FALSE;
    }
    fainted = 0;
    for (i = 0; i < count; i++) {
        if (work->pokemon[BPlistMain_GetPartySlot(work, i)].hp == 0) {
            fainted++;
            if (fainted == 2) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOL BPlistMain_HasReserve(BPlistWork *work) {
    u8 slot;
    u8 i;

    if (BPlistMain_IsMulti(work) == TRUE) {
        return FALSE;
    }
    i = work->param->unk22;
    if (i == 1) {
        return FALSE;
    }
    for (; i < 6; i++) {
        slot = BPlistMain_GetPartySlot(work, i);
        if (work->pokemon[slot].species != 0 && !work->pokemon[slot].isEgg && work->pokemon[slot].hp != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

static void BPlistMain_ChooseFaintedPos(BPlistWork *work) {
    u32 i;

    if (BPlistMain_IsMulti(work) == TRUE) {
        work->param->unk48[0] = BPlistMain_GetPartySlot(work, work->param->partyIndex);
        return;
    }
    for (i = 0; i < 3; i++) {
        if (work->pokemon[BPlistMain_GetPartySlot(work, i)].hp == 0) {
            work->param->unk48[i] = work->param->partyIndex;
            return;
        }
    }
}

static void BPlistMain_ClearChosen(BPlistWork *work) {
    u32 i;

    for (i = 0; i < 3; i++) {
        work->param->unk48[i] = 0xff;
    }
}

static BOOL BPlistMain_CheckQuit(BPlistWork *work) {
    if (work->param->unk30 == 1) {
        BPlistMain_ClearChosen(work);
        return TRUE;
    }
    return FALSE;
}

static void BPlistMain_PlaySE(BPlistWork *work, u32 se) {
    if (work->param->unk40 == 1) {
        GFL_SndSEPlay(se);
    }
}

static void BPlistMain_PlayMessageSE(BPlistWork *work, u32 se) {
    if (work->param->unk40 == 1) {
        GFL_SndSEPlay(se);
    }
}
