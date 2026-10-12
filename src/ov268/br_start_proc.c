// The Battle Recorder's opening and closing screen: it shows the start-up message and opens the sidebars, closes them
// when the player leaves, or opens the menu at once after a battle video. The name is a guess: the ROM gives none, and
// br_core.c runs it as BR_PROCID_START

#include "types.h"
#include "app/battle_recorder/br_start_proc.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_sidebar.h"
#include "app/battle_recorder/br_util.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "system/printsys.h"

typedef struct {
    BrSeq *seq;
    HeapID heapId;
    BrMsgWin *msgWin;
    PrintQueue *que;
    u32 cnt;
    // For the main and the sub screen
    BrBallEffect *ballEff[2];
    BrStartProcParam *param;
} BrStartWork;

static BOOL BrStart_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrStart_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrStart_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrStart_SeqOpen(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrStart_SeqClose(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrStart_SeqReturn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrStart_SeqEnd(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);

const GameProcFunctions BR_START_PROC_FUNCTIONS = {
    BrStart_ProcInit,
    BrStart_ProcMain,
    BrStart_ProcExit,
};

static BOOL BrStart_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrStartProcParam *p = param;
    BrStartWork *wk;
    BrSeqFunc seqFunc;

    wk = GFL_ProcInitSubsystem(proc, sizeof(BrStartWork), BrProcSys_GetHeapID(p->procSys));
    sys_memset(wk, 0, sizeof(BrStartWork));
    wk->param = p;
    wk->heapId = BrProcSys_GetHeapID(p->procSys);
    wk->que = func_02021998(wk->heapId);
    wk->ballEff[CLACT_SURFACE_MAIN] = BrBallEff_Init(p->unit, p->res, CLACT_SURFACE_MAIN, wk->heapId);
    wk->ballEff[CLACT_SURFACE_SUB] = BrBallEff_Init(p->unit, p->res, CLACT_SURFACE_SUB, wk->heapId);
    BrBallEff_SetAnmSeq(wk->ballEff[CLACT_SURFACE_MAIN], 1);
    BrBallEff_SetAnmSeq(wk->ballEff[CLACT_SURFACE_SUB], 1);

    if (p->mode == BR_START_MODE_OPEN) {
        seqFunc = BrStart_SeqOpen;
    } else if (p->mode == BR_START_MODE_CLOSE) {
        seqFunc = BrStart_SeqClose;
    } else {
        seqFunc = BrStart_SeqReturn;
    }
    wk->seq = BrSeq_Init(wk, seqFunc, wk->heapId);
    return TRUE;
}

static BOOL BrStart_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrStartWork *wk = work;

    BrBallEff_Exit(wk->ballEff[CLACT_SURFACE_MAIN]);
    BrBallEff_Exit(wk->ballEff[CLACT_SURFACE_SUB]);
    func_02021a18(wk->que);
    BrSeq_Exit(wk->seq);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BrStart_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BrStartWork *wk = work;

    BrSeq_Main(wk->seq);
    func_02021a3c(wk->que);
    BrBallEff_Main(wk->ballEff[CLACT_SURFACE_MAIN]);
    BrBallEff_Main(wk->ballEff[CLACT_SURFACE_SUB]);
    if (BrSeq_IsEnd(wk->seq)) {
        return TRUE;
    }
    return FALSE;
}

// Starts the Battle Recorder up: the balls gather, the start-up message waits for a touch, then the sidebars open and
// the menu follows
static void BrStart_SeqOpen(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_BALL_MAIN_START,
        SEQ_BALL_MAIN_WAIT,
        SEQ_BALL_SUB_START,
        SEQ_BALL_SUB_WAIT,
        SEQ_MSG_FADEIN_START,
        SEQ_MSG_FADEIN_WAIT,
        SEQ_MSG_FADEIN2_START,
        SEQ_MSG_FADEIN2_WAIT,
        SEQ_TOUCH_WAIT,
        SEQ_OPEN_START,
        SEQ_OPEN_WAIT,
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrStartWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_BALL_MAIN_START: {
        BrPoint pos;

        pos.x = 0x80;
        pos.y = 0x60;
        BrBallEff_Start(wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_LINE, &pos);
        GFL_SndSEPlay(SEQ_SE_BREC_03);
        *p_seq = SEQ_BALL_MAIN_WAIT;
        break;
    }
    case SEQ_BALL_MAIN_WAIT:
        if (BrBallEff_IsEnd(wk->ballEff[CLACT_SURFACE_MAIN])) {
            *p_seq = SEQ_BALL_SUB_START;
        }
        break;
    case SEQ_BALL_SUB_START: {
        BrPoint pos;

        pos.x = 0x80;
        pos.y = -8;
        BrBallEff_Start(wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_LINE_ROT, &pos);
        *p_seq = SEQ_BALL_SUB_WAIT;
        break;
    }
    case SEQ_BALL_SUB_WAIT:
        if (BrBallEff_IsEnd(wk->ballEff[CLACT_SURFACE_SUB])) {
            *p_seq = SEQ_MSG_FADEIN_START;
        }
        break;
    case SEQ_MSG_FADEIN_START: {
        Font *font;
        MsgData *msg;

        wk->msgWin = BrMsgWin_Init(4, 8, 16, 16, 2, 14, wk->que, wk->heapId);
        font = BrRes_GetFont(wk->param->res);
        msg = BrRes_GetMsgData(wk->param->res);
        BrMsgWin_Print(wk->msgWin, msg, 0xe4, font, 0x440);
        BrFade_StartFadeEx(wk->param->fade, BR_FADE_TYPE_PLTT, 0xf, BR_FADE_DIR_OUT, 4);
        *p_seq = SEQ_MSG_FADEIN_WAIT;
        break;
    }
    case SEQ_MSG_FADEIN_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_MSG_FADEIN2_START;
        }
        break;
    case SEQ_MSG_FADEIN2_START:
        BrFade_StartFadeEx(wk->param->fade, BR_FADE_TYPE_PLTT, 0xf, BR_FADE_DIR_IN, 4);
        *p_seq = SEQ_MSG_FADEIN2_WAIT;
        break;
    case SEQ_MSG_FADEIN2_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_TOUCH_WAIT;
        }
        break;
    case SEQ_TOUCH_WAIT:
        if (func_0203da48()) {
            GFL_SndSEPlay(SEQ_SE_BREC_02);
            BrMsgWin_Exit(wk->msgWin);
            wk->msgWin = NULL;
            GFL_BGSysLoadScr(4);
            *p_seq = SEQ_OPEN_START;
        }
        break;
    case SEQ_OPEN_START:
        BrFade_StartFadeEx(wk->param->fade, BR_FADE_TYPE_PLTT, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN, 48);
        BrSidebar_StartOpen(wk->param->sidebar);
        *p_seq = SEQ_OPEN_WAIT;
        break;
    case SEQ_OPEN_WAIT: {
        BOOL isEnd = TRUE;

        isEnd &= BrFade_IsEnd(wk->param->fade);
        if (isEnd) {
            BrBallEff_Start(wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_RING, NULL);
            *p_seq = SEQ_FADEOUT_START;
        }
        break;
    }
    case SEQ_FADEOUT_START:
        BrFade_StartFadeEx(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT, 1);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_END;
        }
        break;
    case SEQ_END:
        if (BrBallEff_IsEnd(wk->ballEff[CLACT_SURFACE_SUB])) {
            BrProcSys_Push(wk->param->procSys, BR_PROCID_MENU);
            BrSeq_SetNext(p_seqwk, BrStart_SeqEnd);
        }
        break;
    }

    if (wk->msgWin != NULL) {
        BrMsgWin_Main(wk->msgWin);
    }
}

// Closes the sidebars and leaves the Battle Recorder
static void BrStart_SeqClose(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_INIT,
        SEQ_CLOSE_START,
        SEQ_WAIT,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrStartWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_INIT:
        *p_seq = SEQ_CLOSE_START;
        break;
    case SEQ_CLOSE_START:
        BrSidebar_StartClose(wk->param->sidebar);
        *p_seq = SEQ_WAIT;
        break;
    case SEQ_WAIT:
        if (wk->cnt++ > 0) {
            wk->cnt = 0;
            BrFade_StartFadeEx(wk->param->fade, BR_FADE_TYPE_PLTT, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT, 26);
            *p_seq = SEQ_FADEOUT_WAIT;
        }
        break;
    case SEQ_FADEOUT_WAIT: {
        BOOL isEnd = TRUE;

        isEnd &= BrFade_IsEnd(wk->param->fade);
        if (isEnd) {
            *p_seq = SEQ_END;
        }
        break;
    }
    case SEQ_END:
        BrProcSys_Pop(wk->param->procSys);
        BrSeq_SetNext(p_seqwk, BrStart_SeqEnd);
        break;
    }
}

// Opens the menu at once, with the sidebars already open
static void BrStart_SeqReturn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrStartWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFadeEx(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT, 1);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_END;
        }
        break;
    case SEQ_END:
        BrSidebar_SetEndPos(wk->param->sidebar);
        BrSidebar_StartBound(wk->param->sidebar);
        BrProcSys_Push(wk->param->procSys, BR_PROCID_MENU);
        BrSeq_SetNext(p_seqwk, BrStart_SeqEnd);
        break;
    }
}

static void BrStart_SeqEnd(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrSeq_End(p_seqwk);
}
