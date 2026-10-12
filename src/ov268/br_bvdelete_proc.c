// The Battle Recorder's battle video deletion screen: it asks twice whether to delete the saved battle video of a
// slot, then deletes it and says so. The name is a guess: the ROM gives none, and br_core.c runs it as
// BR_PROCID_BV_DELETE

#include "types.h"
#include "app/battle_recorder/br_bvdelete_proc.h"
#include "app/battle_rec_ov.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The answers of the question
enum {
    MSGWIN_YES,
    MSGWIN_NO,
    MSGWIN_MAX,
};

#define BR_BVDELETE_PRINT_COLOR 0x3da0

typedef struct {
    PrintQueue *que;
    BrMsgWin *text;
    BrSeq *seq;
    BrMsgWin *msgWin[MSGWIN_MAX];
    // For the main and the sub screen
    BrBallEffect *ballEff[2];
    u16 heapId;
    BrBvDeleteProcParam *p_param;
    u32 cnt;
} BrBvDeleteWork;

// Where a window is, in tiles, and its message
typedef struct {
    u8 x;
    u8 y;
    u8 w;
    u8 h;
    u32 msgID;
} BrBvDeleteMsgWinData;

static BOOL BrBvDelete_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBvDelete_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBvDelete_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrBvDelete_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvDelete_SeqFadeOut(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvDelete_SeqMain(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvDelete_CreateConfirm(BrBvDeleteWork *p_wk, BrBvDeleteProcParam *p_param);
static void BrBvDelete_DeleteText(BrBvDeleteWork *p_wk, BrBvDeleteProcParam *p_param);
static void BrBvDelete_DeleteConfirm(BrBvDeleteWork *p_wk, BrBvDeleteProcParam *p_param);
static BOOL BrBvDelete_GetTrgYes(BrBvDeleteWork *p_wk, u32 x, u32 y);
static BOOL BrBvDelete_GetTrgNo(BrBvDeleteWork *p_wk, u32 x, u32 y);

const GameProcFunctions BR_BVDELETE_PROC_FUNCTIONS = {
    BrBvDelete_ProcInit,
    BrBvDelete_ProcMain,
    BrBvDelete_ProcExit,
};

static const BrBvDeleteMsgWinData sBrBvDeleteConfirmWins[MSGWIN_MAX] = {
    [MSGWIN_YES] = { 4, 6, 9, 2, 0x9f },
    [MSGWIN_NO] = { 18, 6, 9, 2, 0xa0 },
};

static BOOL BrBvDelete_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvDeleteProcParam *p_param = param;
    BrBvDeleteWork *p_wk;
    int i;

    GFL_ASSERT(p_param->mode < BR_BVDELETE_MODE_MAX);

    p_wk = GFL_ProcInitSubsystem(proc, sizeof(BrBvDeleteWork), BrProcSys_GetHeapID(p_param->procSys));
    sys_memset(p_wk, 0, sizeof(BrBvDeleteWork));
    p_wk->p_param = p_param;
    p_wk->heapId = BrProcSys_GetHeapID(p_param->procSys);

    p_wk->seq = BrSeq_Init(p_wk, BrBvDelete_SeqFadeIn, p_wk->heapId);
    p_wk->que = func_02021998(p_wk->heapId);
    BrBvDelete_CreateConfirm(p_wk, p_param);
    for (i = 0; i < 2; i++) {
        p_wk->ballEff[i] = BrBallEff_Init(p_param->unit, p_param->res, i, p_wk->heapId);
    }
    return TRUE;
}

static BOOL BrBvDelete_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvDeleteProcParam *p_param = param;
    BrBvDeleteWork *p_wk = work;
    int i;

    for (i = 0; i < 2; i++) {
        BrBallEff_Exit(p_wk->ballEff[i]);
    }
    if (p_wk->p_param->isDelete) {
        BrBvDelete_DeleteText(p_wk, p_param);
    } else {
        BrBvDelete_DeleteText(p_wk, p_param);
        BrBvDelete_DeleteConfirm(p_wk, p_param);
    }
    func_02021a18(p_wk->que);
    BrSeq_Exit(p_wk->seq);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BrBvDelete_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvDeleteWork *p_wk = work;
    int i;

    BrSeq_Main(p_wk->seq);
    if (BrSeq_IsEnd(p_wk->seq)) {
        return TRUE;
    }

    for (i = 0; i < 2; i++) {
        BrBallEff_Main(p_wk->ballEff[i]);
    }
    func_02021a3c(p_wk->que);
    if (p_wk->text != NULL) {
        BrText_Main(p_wk->text);
    }
    for (i = 0; i < MSGWIN_MAX; i++) {
        if (p_wk->msgWin[i] != NULL) {
            BrMsgWin_Main(p_wk->msgWin[i]);
        }
    }
    return FALSE;
}

static void BrBvDelete_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_NEXT,
    };
    BrBvDeleteWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEIN_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_NEXT;
        }
        break;
    case SEQ_NEXT:
        BrSeq_SetNext(p_seqwk, BrBvDelete_SeqMain);
        break;
    }
}

static void BrBvDelete_SeqFadeOut(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrBvDeleteWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_END;
        }
        break;
    case SEQ_END:
        BrProcSys_Pop(p_wk->p_param->procSys);
        BrSeq_End(p_seqwk);
        break;
    }
}

// Asks twice, then deletes the video and shows the result until the screen is touched
static void BrBvDelete_SeqMain(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_INPUT,
        SEQ_INPUT_AGAIN,
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_DELETE_START,
        SEQ_DELETE_MAIN,
        SEQ_DELETE_WAIT,
        SEQ_MSG_WAIT,
        SEQ_END,
    };
    BrBvDeleteWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_INPUT: {
        u32 x;
        u32 y;

        if (func_0203dac8(&x, &y)) {
            if (BrBvDelete_GetTrgYes(p_wk, x, y)) {
                BrText_Print(p_wk->text, p_wk->p_param->res, 0x142);
                *p_seq = SEQ_INPUT_AGAIN;
            } else if (BrBvDelete_GetTrgNo(p_wk, x, y)) {
                *p_seq = SEQ_END;
            }
        }
        break;
    }
    case SEQ_INPUT_AGAIN: {
        u32 x;
        u32 y;

        if (func_0203dac8(&x, &y)) {
            if (BrBvDelete_GetTrgYes(p_wk, x, y)) {
                *p_seq = SEQ_FADEOUT_START;
            } else if (BrBvDelete_GetTrgNo(p_wk, x, y)) {
                *p_seq = SEQ_END;
            }
        }
        break;
    }
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_SUB, BR_FADE_DIR_OUT);
        (*p_seq)++;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            BrPoint pos;

            BrBvDelete_DeleteConfirm(p_wk, p_wk->p_param);
            BrText_Print(p_wk->text, p_wk->p_param->res, 0x143);
            pos.x = 0x80;
            pos.y = 0x60;
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_ROT, &pos);
            p_wk->cnt = 0;
            (*p_seq)++;
        }
        break;
    case SEQ_FADEIN_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_SUB, BR_FADE_DIR_IN);
        (*p_seq)++;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            (*p_seq)++;
        }
        break;
    case SEQ_DELETE_START:
        BattleRecOv_StartDeleteVideo(p_wk->p_param->gameData, p_wk->heapId, p_wk->p_param->mode);
        p_wk->cnt++;
        (*p_seq)++;
        break;
    case SEQ_DELETE_MAIN: {
        u32 ret = BattleRecOv_DeleteVideoMain(p_wk->p_param->gameData, p_wk->p_param->mode);

        p_wk->cnt++;
        if (ret == 2 || ret == 3) {
            p_wk->p_param->isDelete = TRUE;
            *p_seq = SEQ_DELETE_WAIT;
        }
        break;
    }
    case SEQ_DELETE_WAIT:
        // The balls turn for at least 96 frames
        if (p_wk->cnt++ > 96) {
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_NONE, NULL);
            BrText_Print(p_wk->text, p_wk->p_param->res, 0x144);
            *p_seq = SEQ_MSG_WAIT;
        }
        break;
    case SEQ_MSG_WAIT:
        if (func_0203da48()) {
            u32 x;
            u32 y;
            BrPoint pos;

            func_0203dac8(&x, &y);
            pos.x = x;
            pos.y = y;
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            GFL_SndSEPlay(SEQ_SE_BREC_06);
            *p_seq = SEQ_END;
        }
        break;
    case SEQ_END:
        BrSeq_SetNext(p_seqwk, BrBvDelete_SeqFadeOut);
        break;
    }
}

// Makes the question, which names the video's player, with its two answers
static void BrBvDelete_CreateConfirm(BrBvDeleteWork *p_wk, BrBvDeleteProcParam *p_param) {
    WordSet *wordSet;
    StrBuf *src;
    StrBuf *str;
    Font *font;
    MsgData *msg;
    int i;

    if (p_wk->text == NULL) {
        p_wk->text = BrText_Init(p_param->res, p_wk->que, p_wk->heapId);
    }
    wordSet = BrRes_GetWordSet(p_param->res);
    src = GFL_MsgDataLoadStrbufNew(BrRes_GetMsgData(p_param->res), 0x141);
    str = GFL_StrBufCreate(0x80, p_wk->heapId);
    func_0202437c(wordSet, 0, p_param->recordInfo->name[p_param->mode], p_param->recordInfo->sex[p_param->mode], 1, 2);
    GFL_WordSetFormatStrbuf(wordSet, str, src);
    BrText_PrintBuf(p_wk->text, p_param->res, str);
    GFL_StrBufFree(str);
    GFL_StrBufFree(src);

    BrRes_LoadBG(p_param->res, 0x1e, p_wk->heapId);
    font = BrRes_GetFont(p_param->res);
    msg = BrRes_GetMsgData(p_param->res);
    for (i = 0; i < MSGWIN_MAX; i++) {
        const BrBvDeleteMsgWinData *data = &sBrBvDeleteConfirmWins[i];

        p_wk->msgWin[i] = BrMsgWin_Init(4, data->x, data->y, data->w, data->h, 14, p_wk->que, p_wk->heapId);
        BrMsgWin_Print(p_wk->msgWin[i], msg, data->msgID, font, BR_BVDELETE_PRINT_COLOR);
    }
}

static void BrBvDelete_DeleteText(BrBvDeleteWork *p_wk, BrBvDeleteProcParam *p_param) {
    if (p_wk->text != NULL) {
        BrText_Exit(p_wk->text, p_param->res);
        p_wk->text = NULL;
    }
}

static void BrBvDelete_DeleteConfirm(BrBvDeleteWork *p_wk, BrBvDeleteProcParam *p_param) {
    int i;

    for (i = 0; i < MSGWIN_MAX; i++) {
        if (p_wk->msgWin[i] != NULL) {
            BrMsgWin_Exit(p_wk->msgWin[i]);
            p_wk->msgWin[i] = NULL;
        }
    }
    GFL_BGSysLoadScr(4);
    BrRes_UnloadBG(p_param->res, 0x1e);
}

// The answers of the question
static BOOL BrBvDelete_GetTrgYes(BrBvDeleteWork *p_wk, u32 x, u32 y) {
    BOOL ret = ((u32)(x - 32) <= (u32)(104 - 32)) & ((u32)(y - 48) <= (u32)(64 - 48));

    if (ret) {
        BrPoint pos;

        pos.x = x;
        pos.y = y;
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        GFL_SndSEPlay(SEQ_SE_BREC_06);
    }
    return ret;
}

static BOOL BrBvDelete_GetTrgNo(BrBvDeleteWork *p_wk, u32 x, u32 y) {
    BOOL ret = ((u32)(x - 144) <= (u32)(216 - 144)) & ((u32)(y - 48) <= (u32)(64 - 48));

    if (ret) {
        BrPoint pos;

        pos.x = x;
        pos.y = y;
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        GFL_SndSEPlay(SEQ_SE_BREC_06);
    }
    return ret;
}
