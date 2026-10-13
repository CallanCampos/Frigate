#include "types.h"
#include "app/mailbox_local.h"
#include "app/pokelist.h"
#include "constants/sound.h"
#include "field/mailsys.h"
#include "field/mailtool.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "pml/item.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/bgwinfrm.h"
#include "system/bmp_winframe.h"
#include "system/cursor_move.h"
#include "system/game_data.h"
#include "system/pms_data.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wipe.h"

// The mailbox screen's state machine, one function per state, with the party list and the mail writing screen run as
// child processes

// What a yes/no question does for each answer
typedef struct {
    MailBoxStateFunc yes;
    MailBoxStateFunc no;
} MailBoxYesNo;

typedef struct {
    MailBoxStateFunc start;
    MailBoxStateFunc end;
    u32 nextState;
} MailBoxChildProc;

static u32 MailBox_StateInit(MailBoxWork *work);
static u32 MailBox_StateExit(MailBoxWork *work);
static u32 MailBox_StateWaitWipe(MailBoxWork *work);
static u32 MailBox_StateWaitMessage(MailBoxWork *work);
static u32 MailBox_StateWaitYesNo(MailBoxWork *work);
static u32 MailBox_StateWaitPaletteFx(MailBoxWork *work);
static u32 MailBox_StateStartChild(MailBoxWork *work);
static u32 MailBox_StateWaitChild(MailBoxWork *work);
static u32 MailBox_StateWipeIn(MailBoxWork *work);
static u32 MailBox_StateSelectMail(MailBoxWork *work);
static u32 MailBox_StateWipeOut(MailBoxWork *work);
static u32 MailBox_StateChangePage(MailBoxWork *work);
static u32 MailBox_StateOpenMenu(MailBoxWork *work);
static u32 MailBox_StateCreateMenu(MailBoxWork *work);
static u32 MailBox_StateWaitMenu(MailBoxWork *work);
static u32 MailBox_StateReadMail(MailBoxWork *work);
static u32 MailBox_StateCloseMail(MailBoxWork *work);
static u32 MailBox_StateAskErase(MailBoxWork *work);
static u32 MailBox_StateEraseNotice(MailBoxWork *work);
static u32 MailBox_StateAskGive(MailBoxWork *work);
static u32 MailBox_StatePartyReturn(MailBoxWork *work);
static u32 MailBox_StateGiveFadeIn(MailBoxWork *work);
static u32 MailBox_StateGiveResult(MailBoxWork *work);
static u32 MailBox_StateMailWritten(MailBoxWork *work);
static u32 MailBox_StatePressReturn(MailBoxWork *work);
static u32 MailBox_StatePressClose(MailBoxWork *work);
static u32 MailBox_StateBackToList(MailBoxWork *work);
static u32 MailBox_StartWipeIn(MailBoxWork *work, u32 nextState);
static u32 MailBox_StartWipeOut(MailBoxWork *work, u32 nextState);
static u32 MailBox_StartMessage(MailBoxWork *work, u8 waitForInput, u32 nextState);
static u32 MailBox_StartYesNo(MailBoxWork *work, u32 index);
static u32 MailBox_StartFrameFx(MailBoxWork *work, u32 frame, u32 nextState);
static u32 MailBox_StartPageFx(MailBoxWork *work, u32 direction, u32 nextState);
static void MailBox_SetDimmed(MailBoxView *view, BOOL dimmed);
static void MailBox_SetDimmedForReading(MailBoxView *view);
static void MailBox_ShowMail(MailBoxWork *work);
static void MailBox_ClearMail(MailBoxWork *work);
static void MailBox_TakeMailItem(MailBoxWork *work);
static void MailBox_EraseMail(MailBoxWork *work);
static void MailBox_CreatePartyParam(MailBoxWork *work, u32 mode, u16 item, u16 mailIndex);
static u32 MailBox_StartPartyList(MailBoxWork *work);
static u32 MailBox_EndPartyListCancel(MailBoxWork *work);
static u32 MailBox_EndPartyListPick(MailBoxWork *work);
static u32 MailBox_EndPartyListCancelAsk(MailBoxWork *work);
static u32 MailBox_StartMailSys(MailBoxWork *work);
static u32 MailBox_EndMailSys(MailBoxWork *work);
static u32 MailBox_EraseYes(MailBoxWork *work);
static u32 MailBox_GiveYes(MailBoxWork *work);
static u32 MailBox_GiveNo(MailBoxWork *work);

static const GameProcFunctions sMailSysProc = { MailSys_ProcInit, MailSys_ProcMain, MailSys_ProcExit };

static const MailBoxYesNo sYesNoFuncs[2] = {
    { MailBox_EraseYes, MailBox_StateBackToList },
    { MailBox_GiveYes, MailBox_GiveNo },
};

static const MailBoxChildProc sChildProcs[MAILBOX_CHILD_COUNT] = {
    { MailBox_StartPartyList, MailBox_EndPartyListCancel, MAILBOX_STATE_INIT },
    { MailBox_StartPartyList, MailBox_EndPartyListPick, MAILBOX_STATE_PARTY_RETURN },
    { MailBox_StartMailSys, MailBox_EndMailSys, MAILBOX_STATE_MAIL_WRITTEN },
    { MailBox_StartPartyList, MailBox_EndPartyListCancelAsk, MAILBOX_STATE_INIT },
};

static const MailBoxStateFunc sStateFuncs[] = {
    MailBox_StateInit,       MailBox_StateExit,         MailBox_StateWaitWipe,    MailBox_StateWaitMessage,
    MailBox_StateWaitYesNo,  MailBox_StateWaitPaletteFx, MailBox_StateStartChild, MailBox_StateWaitChild,
    MailBox_StateWipeIn,     MailBox_StateSelectMail,   MailBox_StateWipeOut,     MailBox_StateChangePage,
    MailBox_StateOpenMenu,   MailBox_StateCreateMenu,   MailBox_StateWaitMenu,    MailBox_StateReadMail,
    MailBox_StateCloseMail,  MailBox_StateAskErase,     MailBox_StateEraseNotice, MailBox_StateAskGive,
    MailBox_StatePartyReturn, MailBox_StateGiveFadeIn,  MailBox_StateGiveResult,  MailBox_StateMailWritten,
    MailBox_StatePressReturn, MailBox_StatePressClose,  MailBox_StateBackToList,
};

BOOL MailBox_Update(MailBoxWork *work, u32 *state) {
    *state = sStateFuncs[*state](work);
    if (*state == MAILBOX_STATE_DONE) {
        return FALSE;
    }
    if (work->view != NULL) {
        BGWinFrame_UpdateMoves(work->view->frames);
        MailBox_UpdateClActUnit(work->view);
        if (work->view->printQueue != NULL) {
            int i;

            func_02021a3c(work->view->printQueue);
            for (i = 0; i < MAILBOX_WIN_SENTENCE_1; i++) {
                PrintQueue *queue = work->view->printQueue;
                MailBoxPrintState *printState = &work->view->printStates[i];

                if (printState->printing) {
                    if (func_02021c1c(queue, BmpWin_GetBitmap(printState->window)) == FALSE) {
                        BmpWin_FlushChar(printState->window);
                        printState->printing = FALSE;
                    }
                }
            }
        }
        if (work->view->tcbManager != NULL) {
            GFL_TCBExMgrUpdate(work->view->tcbManager);
        }
        if (work->view->pmsPrintQueue != NULL) {
            int i;

            PMSDraw_Main(work->view->pmsDraw);
            func_02021a3c(work->view->pmsPrintQueue);
            for (i = MAILBOX_WIN_SENTENCE_1; i < MAILBOX_WINDOW_COUNT; i++) {
                PrintQueue *queue = work->view->printQueue;
                MailBoxPrintState *printState = &work->view->printStates[i];

                if (printState->printing) {
                    if (func_02021c1c(queue, BmpWin_GetBitmap(printState->window)) == FALSE) {
                        BmpWin_FlushChar(printState->window);
                        printState->printing = FALSE;
                    }
                }
            }
        }
    }
    return TRUE;
}

static u32 MailBox_StateInit(MailBoxWork *work) {
    if (work->vblankTask != NULL) {
        GFL_TCBRemove(work->vblankTask);
        work->vblankTask = NULL;
    }
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetDisplayLayout(0);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MAILBOX_VIEW, 0x60000);
    work->view = GFL_HeapAllocate(HEAPID_MAILBOX_VIEW, sizeof(MailBoxView), FALSE, "mb_seq.c", 268);
    sys_memset32_fast(0, work->view, sizeof(MailBoxView));
    MailBox_LoadMails(work);
    MailBox_SetVRAMBanks();
    MailBox_InitBGs(work);
    MailBox_LoadBGGraphics(work);
    MailBox_LoadSystemPalettes(work);
    MailBox_CreateWindows(work);
    MailBox_CreateFrames(work);
    MailBox_CreateOBJs(work);
    MailBox_CreatePMSDraw(work);
    MailBox_CreateCursor(work);
    MailBox_LoadMessages(work->view);
    MailBox_PrintTitle(work);
    MailBox_UpdateMailList(work);
    MailBox_CreateMenuResources(work);
    work->vblankTask = GFL_VBlankTCBAdd(MailBox_VBlank, work, 1);
    func_02042ba8(FALSE, HEAPID_MAILBOX_VIEW);
    return work->nextState;
}

static u32 MailBox_StateExit(MailBoxWork *work) {
    if (work->vblankTask != NULL) {
        GFL_TCBRemove(work->vblankTask);
        work->vblankTask = NULL;
    }
    MailBox_FreeMenuResources(work);
    MailBox_FreeMessages(work->view);
    MailBox_FreeCursor(work);
    MailBox_FreePMSDraw(work);
    MailBox_FreeOBJs(work);
    MailBox_FreeFrames(work);
    MailBox_FreeWindows(work);
    MailBox_FreeBGs(work);
    MailBox_FreeMails(work);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_HeapFree(work->view);
    work->view = NULL;
    GFL_HeapDelete(HEAPID_MAILBOX_VIEW);
    return work->nextState;
}

static u32 MailBox_StateWaitWipe(MailBoxWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        return work->view->wipeNextState;
    }
    return MAILBOX_STATE_WAIT_WIPE;
}

static u32 MailBox_StateWaitMessage(MailBoxWork *work) {
    if (func_020223b4(work->view->stream) == PRINT_STREAM_DONE) {
        if (!work->view->waitForInput) {
            func_020223cc(work->view->stream);
            return work->nextState;
        }
        if (func_0203da48() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            func_020223cc(work->view->stream);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return work->nextState;
        }
    }
    return MAILBOX_STATE_WAIT_MESSAGE;
}

static u32 MailBox_StateWaitYesNo(MailBoxWork *work) {
    AppTaskMenu_Update(work->view->menu);
    if (!AppTaskMenu_IsFlashFinished(work->view->menu)) {
        return MAILBOX_STATE_WAIT_YES_NO;
    }
    switch (AppTaskMenu_GetCursorPos(work->view->menu)) {
    case 0:
        AppTaskMenu_Free(work->view->menu);
        return sYesNoFuncs[work->view->yesNoIndex].yes(work);
    case 1:
        AppTaskMenu_Free(work->view->menu);
        return sYesNoFuncs[work->view->yesNoIndex].no(work);
    default:
        return MAILBOX_STATE_WAIT_YES_NO;
    }
}

static u32 MailBox_StateWaitPaletteFx(MailBoxWork *work) {
    if (MailBox_UpdatePaletteFx(work) == FALSE) {
        return work->nextState;
    }
    return MAILBOX_STATE_WAIT_PALETTE_FX;
}

static u32 MailBox_StateStartChild(MailBoxWork *work) {
    sChildProcs[work->childProc].start(work);
    return MAILBOX_STATE_WAIT_CHILD;
}

static u32 MailBox_StateWaitChild(MailBoxWork *work) {
    if (GFL_ProcMgrUpdate(work->procManager) == TRUE) {
        return MAILBOX_STATE_WAIT_CHILD;
    }
    work->nextState = sChildProcs[work->childProc].end(work);
    return sChildProcs[work->childProc].nextState;
}

static u32 MailBox_StateWipeIn(MailBoxWork *work) {
    return MailBox_StartWipeIn(work, MAILBOX_STATE_SELECT_MAIL);
}

static u32 MailBox_StateSelectMail(MailBoxWork *work) {
    u32 result;

    if (func_02021c0c(work->view->printQueue) == FALSE) {
        return MAILBOX_STATE_SELECT_MAIL;
    }
    switch (MailBox_GetTouch(work)) {
    case 0:
        if (work->page != 0) {
            work->page--;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return MailBox_StartPageFx(work, 0, MAILBOX_STATE_CHANGE_PAGE);
        }
        return MAILBOX_STATE_SELECT_MAIL;
    case 1:
        if (work->page != work->view->maxPage) {
            work->page++;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return MailBox_StartPageFx(work, 1, MAILBOX_STATE_CHANGE_PAGE);
        }
        return MAILBOX_STATE_SELECT_MAIL;
    case 2:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_RETURN_BUTTON, 9);
        work->nextState = MAILBOX_STATE_WIPE_OUT;
        return MAILBOX_STATE_PRESS_RETURN;
    case 3:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CLOSE_BUTTON, 8);
        work->nextState = MAILBOX_STATE_WIPE_OUT;
        return MAILBOX_STATE_PRESS_CLOSE;
    }
    result = CursorMove_Update(work->view->cursor);
    switch (result) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        if (*(work->view->pageMails + result + MAILBOX_PAGE_SIZE * work->page) == 0xff) {
            break;
        }
        work->cursor = result;
        work->mailIndex = *(work->view->pageMails + result + MAILBOX_PAGE_SIZE * work->page);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return MailBox_StartFrameFx(work, result, MAILBOX_STATE_OPEN_MENU);
    case CURSOR_MOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_RETURN_BUTTON, 9);
        work->nextState = MAILBOX_STATE_WIPE_OUT;
        return MAILBOX_STATE_PRESS_RETURN;
    case CURSOR_MOVE_EDGE_RIGHT:
        CursorMove_GetPos(work->view->cursor);
        if (work->page != work->view->maxPage) {
            work->page++;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return MailBox_StartPageFx(work, 1, MAILBOX_STATE_CHANGE_PAGE);
        }
        break;
    case CURSOR_MOVE_EDGE_LEFT:
        CursorMove_GetPos(work->view->cursor);
        if (work->page != 0) {
            work->page--;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return MailBox_StartPageFx(work, 0, MAILBOX_STATE_CHANGE_PAGE);
        }
        break;
    case CURSOR_MOVE_MOVED:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_CURSOR_ON:
        break;
    default:
        if (MailBox_IsCloseKeyPressed()) {
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CLOSE_BUTTON, 8);
            work->nextState = MAILBOX_STATE_WIPE_OUT;
            return MAILBOX_STATE_PRESS_CLOSE;
        }
        break;
    }
    return MAILBOX_STATE_SELECT_MAIL;
}

static u32 MailBox_StateWipeOut(MailBoxWork *work) {
    work->nextState = MAILBOX_STATE_DONE;
    return MailBox_StartWipeOut(work, MAILBOX_STATE_EXIT);
}

static u32 MailBox_StateChangePage(MailBoxWork *work) {
    if (work->page == 0) {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_PREV_PAGE, FALSE);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_NEXT_PAGE, TRUE);
    } else {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_PREV_PAGE, TRUE);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_NEXT_PAGE, FALSE);
    }
    MailBox_UpdateMailList(work);
    return MAILBOX_STATE_SELECT_MAIL;
}

static u32 MailBox_StateOpenMenu(MailBoxWork *work) {
    MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, FALSE);
    MailBox_SetDimmed(work->view, TRUE);
    MailBox_ShowSelectedFrame(work);
    MailBox_ShowMessageWindow(work);
    MailBox_PrintWhatWouldYouLike(work);
    return MailBox_StartMessage(work, FALSE, MAILBOX_STATE_CREATE_MENU);
}

static u32 MailBox_StateCreateMenu(MailBoxWork *work) {
    MailBox_CreateMainMenu(work);
    return MAILBOX_STATE_WAIT_MENU;
}

static u32 MailBox_StateWaitMenu(MailBoxWork *work) {
    AppTaskMenu_Update(work->view->menu);
    if (!AppTaskMenu_IsFlashFinished(work->view->menu)) {
        return MAILBOX_STATE_WAIT_MENU;
    }
    switch (AppTaskMenu_GetCursorPos(work->view->menu) + 2) {
    case 2:
        MailBox_FreeMenu(work);
        MailBox_HideMessageWindow(work);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, FALSE);
        MailBox_Nop1(work->view);
        MailBox_ShowReadCursor(work->view);
        MailBox_ShowMail(work);
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CLOSE_BUTTON, 14);
        MailBox_SetDimmedForReading(work->view);
        return MAILBOX_STATE_READ_MAIL;
    case 3:
        MailBox_FreeMenu(work);
        MailBox_SetMessage(work, MAILBOX_MESSAGE_ERASE);
        return MailBox_StartMessage(work, FALSE, MAILBOX_STATE_ASK_ERASE);
    case 4:
        MailBox_FreeMenu(work);
        MailBox_CreatePartyParam(work, 14, PML_ItemGetMailItemID(func_02009750(work->view->mails[work->mailIndex])),
                                 work->mailIndex);
        work->childProc = MAILBOX_CHILD_PARTY_GIVE;
        work->nextState = MAILBOX_STATE_START_CHILD;
        return MailBox_StartWipeOut(work, 1);
    case 0:
    case 5:
        MailBox_FreeMenu(work);
        MailBox_HideMessageWindow(work);
        BGWinFrame_Hide(work->view->frames, MAILBOX_FRAME_SELECTED);
        MailBox_SetDimmed(work->view, FALSE);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, TRUE);
        return MAILBOX_STATE_SELECT_MAIL;
    default:
        return MAILBOX_STATE_WAIT_MENU;
    }
}

static u32 MailBox_StateReadMail(MailBoxWork *work) {
    if (MailBox_IsReturnPressed() == TRUE) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_RETURN_BUTTON, 9);
        work->nextState = MAILBOX_STATE_CLOSE_MAIL;
        return MAILBOX_STATE_PRESS_RETURN;
    }
    return MAILBOX_STATE_READ_MAIL;
}

static u32 MailBox_StateCloseMail(MailBoxWork *work) {
    MailBox_ClearMail(work);
    MailBox_UpdateCursor(work, CursorMove_GetPos(work->view->cursor));
    MailBox_Nop2(work->view);
    BGWinFrame_Hide(work->view->frames, MAILBOX_FRAME_SELECTED);
    MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, TRUE);
    MailBox_SetDimmed(work->view, FALSE);
    return MAILBOX_STATE_SELECT_MAIL;
}

static u32 MailBox_StateAskErase(MailBoxWork *work) {
    return MailBox_StartYesNo(work, 0);
}

static u32 MailBox_StateEraseNotice(MailBoxWork *work) {
    MailBox_SetMessage(work, MAILBOX_MESSAGE_ASK_HOLD);
    return MailBox_StartMessage(work, FALSE, MAILBOX_STATE_ASK_GIVE);
}

static u32 MailBox_StateAskGive(MailBoxWork *work) {
    return MailBox_StartYesNo(work, 1);
}

static u32 MailBox_StatePartyReturn(MailBoxWork *work) {
    if (work->partyResult != 6) {
        work->nextState = MAILBOX_STATE_GIVE_FADE_IN;
        return MAILBOX_STATE_INIT;
    }
    work->childProc = MAILBOX_CHILD_MAIL_SYS;
    return MAILBOX_STATE_START_CHILD;
}

static u32 MailBox_StateGiveFadeIn(MailBoxWork *work) {
    MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, FALSE);
    MailBox_SetDimmed(work->view, TRUE);
    return MailBox_StartWipeIn(work, MAILBOX_STATE_GIVE_RESULT);
}

static u32 MailBox_StateGiveResult(MailBoxWork *work) {
    MailBox_ShowMessageWindow(work);
    return MailBox_GiveNo(work);
}

static u32 MailBox_StateMailWritten(MailBoxWork *work) {
    if (!work->mailGiven) {
        work->nextState = MAILBOX_STATE_GIVE_FADE_IN;
        return MAILBOX_STATE_INIT;
    }
    MailBox_CreatePartyParam(work, 11, work->item, work->partyPos);
    work->childProc = MAILBOX_CHILD_PARTY_PICK;
    return MAILBOX_STATE_START_CHILD;
}

static u32 MailBox_StatePressReturn(MailBoxWork *work) {
    if (!func_0204c560(work->view->actors[MAILBOX_ACTOR_RETURN_BUTTON])) {
        work->param->result = 0;
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CLOSE_BUTTON, 0);
        return work->nextState;
    }
    return MAILBOX_STATE_PRESS_RETURN;
}

static u32 MailBox_StatePressClose(MailBoxWork *work) {
    if (!func_0204c560(work->view->actors[MAILBOX_ACTOR_CLOSE_BUTTON])) {
        work->param->result = 1;
        return work->nextState;
    }
    return MAILBOX_STATE_PRESS_CLOSE;
}

static u32 MailBox_StateBackToList(MailBoxWork *work) {
    MailBox_HideMessageWindow(work);
    BGWinFrame_Hide(work->view->frames, MAILBOX_FRAME_SELECTED);
    MailBox_UpdateCursor(work, CursorMove_GetPos(work->view->cursor));
    MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, TRUE);
    MailBox_SetDimmed(work->view, FALSE);
    return MAILBOX_STATE_SELECT_MAIL;
}

static u32 MailBox_StartWipeIn(MailBoxWork *work, u32 nextState) {
    GFL_WipeSet(0, 1, 1, 0, 6, 1, HEAPID_MAILBOX_VIEW);
    work->view->wipeNextState = nextState;
    return MAILBOX_STATE_WAIT_WIPE;
}

static u32 MailBox_StartWipeOut(MailBoxWork *work, u32 nextState) {
    GFL_WipeSet(0, 0, 0, 0, 6, 1, HEAPID_MAILBOX_VIEW);
    work->view->wipeNextState = nextState;
    return MAILBOX_STATE_WAIT_WIPE;
}

static u32 MailBox_StartMessage(MailBoxWork *work, u8 waitForInput, u32 nextState) {
    work->view->waitForInput = waitForInput;
    work->nextState = nextState;
    return MAILBOX_STATE_WAIT_MESSAGE;
}

static u32 MailBox_StartYesNo(MailBoxWork *work, u32 index) {
    work->view->yesNoIndex = index;
    MailBox_CreateYesNoMenu(work);
    return MAILBOX_STATE_WAIT_YES_NO;
}

static u32 MailBox_StartFrameFx(MailBoxWork *work, u32 frame, u32 nextState) {
    MailBoxPaletteFx *fx;
    s8 x;
    s8 y;
    u16 width;
    u16 height;

    BGWinFrame_GetPos(work->view->frames, frame, &x, &y);
    BGWinFrame_GetSize(work->view->frames, frame, &width, &height);
    fx = &work->view->paletteFx;
    fx->onScreen = TRUE;
    fx->target = 2;
    fx->firstPalette = 2;
    fx->secondPalette = 1;
    fx->phase = 0;
    fx->timer = 0;
    fx->x = x;
    fx->y = y;
    fx->width = width;
    fx->height = height;
    work->nextState = nextState;
    return MAILBOX_STATE_WAIT_PALETTE_FX;
}

static u32 MailBox_StartPageFx(MailBoxWork *work, u32 direction, u32 nextState) {
    MailBoxPaletteFx *fx = &work->view->paletteFx;

    fx->onScreen = FALSE;
    fx->target = direction;
    fx->firstPalette = 1;
    fx->secondPalette = 0;
    fx->phase = 0;
    fx->timer = 0;
    work->nextState = nextState;
    return MAILBOX_STATE_WAIT_PALETTE_FX;
}

static void MailBox_SetDimmed(MailBoxView *view, BOOL dimmed) {
    if (dimmed == TRUE) {
        MailBox_SetActorBlend(view, 0, TRUE);
        MailBox_SetActorBlend(view, 1, TRUE);
        MailBox_SetActorBlend(view, 3, TRUE);
        MailBox_SetActorBlend(view, 7, TRUE);
        gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 4, 8, 4, 12);
    } else {
        MailBox_SetActorBlend(view, 0, FALSE);
        MailBox_SetActorBlend(view, 1, FALSE);
        MailBox_SetActorBlend(view, 3, FALSE);
        MailBox_SetActorBlend(view, 7, FALSE);
        G2_BlendNone();
    }
}

static void MailBox_SetDimmedForReading(MailBoxView *view) {
    MailBox_SetActorBlend(view, 0, TRUE);
    MailBox_SetActorBlend(view, 1, TRUE);
    MailBox_SetActorBlend(view, 3, FALSE);
    MailBox_SetActorBlend(view, 7, FALSE);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 4, 8, 4, 12);
}

static void MailBox_ShowMail(MailBoxWork *work) {
    MailBox_LoadMailGraphics(work);
    MailBox_PrintMail(work);
}

static void MailBox_ClearMail(MailBoxWork *work) {
    int i;

    for (i = 0; i < 3; i++) {
        if (PMSData_IsNotEmpty(func_0200976c(work->view->mails[work->mailIndex], i))) {
            PMSDraw_Clear(work->view->pmsDraw, i, FALSE);
        }
    }
    if (work->view->stampSentence.words[0] != 0) {
        PMSDraw_Clear(work->view->pmsDraw, i, FALSE);
    }
    GFL_BGSysClearScr(4);
    GFL_BGSysClearScr(5);
}

static void MailBox_TakeMailItem(MailBoxWork *work) {
    BagSave *bag = GameData_GetBag(work->param->gameData);

    work->item = PML_ItemGetMailItemID(func_02009750(work->view->mails[work->mailIndex]));
    if (BagSave_AddItem(bag, work->item, 1, HEAPID_MAILBOX_VIEW) == TRUE) {
        work->bagFull = FALSE;
    } else {
        work->bagFull = TRUE;
    }
}

static void MailBox_EraseMail(MailBoxWork *work) {
    func_020097d0(work->mails, 0, work->mailIndex);
}

static void MailBox_CreatePartyParam(MailBoxWork *work, u32 mode, u16 item, u16 mailIndex) {
    PokeListParam *param = PokeListParam_Create(work->param->gameData, mode, GameData_GetParty(work->param->gameData),
                                                HEAPID_MAILBOX_PROC);

    param->item = item;
    param->unk6C = mailIndex;
    work->childParam = param;
}

static u32 MailBox_StartPartyList(MailBoxWork *work) {
    PokeListParam *param = work->childParam;

    param->index = work->partyPos;
    QueueGameProc(work->procManager, OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, work->childParam);
    return 0;
}

static u32 MailBox_EndPartyListCancel(MailBoxWork *work) {
    GFL_HeapFree(work->childParam);
    return MAILBOX_STATE_WIPE_IN;
}

static u32 MailBox_EndPartyListPick(MailBoxWork *work) {
    PokeListParam *param = work->childParam;

    work->partyPos = param->index;
    work->partyResult = param->result;
    GFL_HeapFree(param);
    return 0;
}

static u32 MailBox_EndPartyListCancelAsk(MailBoxWork *work) {
    GFL_HeapFree(work->childParam);
    return MAILBOX_STATE_WIPE_IN;
}

static u32 MailBox_StartMailSys(MailBoxWork *work) {
    work->childParam = MailTool_CreateWriteParam(work->param->gameData, 2, work->partyPos, PML_ItemGetMailID(work->item),
                                           HEAPID_MAILBOX_PROC);
    QueueGameProc(work->procManager, OVERLAY_NONE, &sMailSysProc, work->childParam);
    return 0;
}

static u32 MailBox_EndMailSys(MailBoxWork *work) {
    work->mailGiven = MailTool_IsWritten(work->childParam);
    if (work->mailGiven == TRUE) {
        MailTool_GiveMail(work->childParam,
                            PokeParty_GetPkm(GameData_GetParty(work->param->gameData), work->partyPos));
        BagSave_SubItem(GameData_GetBag(work->param->gameData), work->item, 1, HEAPID_MAILBOX_PROC);
    }
    MailTool_FreeParam(work->childParam);
    return 0;
}

static u32 MailBox_EraseYes(MailBoxWork *work) {
    MailBox_TakeMailItem(work);
    MailBox_EraseMail(work);
    MailBox_FreeMails(work);
    MailBox_LoadMails(work);
    if (work->view->mailCount <= MAILBOX_PAGE_SIZE) {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_PREV_PAGE, FALSE);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_NEXT_PAGE, FALSE);
    }
    if (work->page > work->view->maxPage) {
        work->page = work->view->maxPage;
        work->cursor = 0;
    }
    MailBox_UpdateMailList(work);
    BGWinFrame_Hide(work->view->frames, MAILBOX_FRAME_SELECTED);
    MailBox_SetMessage(work, MAILBOX_MESSAGE_ERASED);
    return MailBox_StartMessage(work, TRUE, MAILBOX_STATE_ERASE_NOTICE);
}

static u32 MailBox_GiveYes(MailBoxWork *work) {
    MailBox_CreatePartyParam(work, 9, work->item, 0);
    work->childProc = MAILBOX_CHILD_PARTY_ASK;
    work->nextState = MAILBOX_STATE_START_CHILD;
    return MailBox_StartWipeOut(work, 1);
}

static u32 MailBox_GiveNo(MailBoxWork *work) {
    if (!work->bagFull) {
        MailBox_SetMessage(work, MAILBOX_MESSAGE_HOLD_DONE);
    } else {
        MailBox_SetMessage(work, MAILBOX_MESSAGE_BAG_FULL);
    }
    return MailBox_StartMessage(work, TRUE, MAILBOX_STATE_BACK_TO_LIST);
}
