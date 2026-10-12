// The Battle Recorder's battle video upload screen: it loads the player's own battle video, sends it to the Global
// Link and shows the number the video was given, or the error. The name is a guess: the ROM gives none, and br_core.c
// runs it as BR_PROCID_BV_SEND

#include "types.h"
#include "app/battle_recorder/br_bvsend_proc.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_net.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "constants/sound.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "save/battle_rec.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"

typedef struct {
    PrintQueue *que;
    BrSeq *seq;
    u16 heapId;
    BrMsgWin *text;
    // For the main and the sub screen
    BrBallEffect *ballEff[2];
    BrBvSendProcParam *p_param;
    u32 cnt;
} BrBvSendWork;

static BOOL BrBvSend_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBvSend_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBvSend_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrBvSend_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvSend_SeqFadeOut(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvSend_SeqMain(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);

const GameProcFunctions BR_BVSEND_PROC_FUNCTIONS = {
    BrBvSend_ProcInit,
    BrBvSend_ProcMain,
    BrBvSend_ProcExit,
};

static BOOL BrBvSend_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvSendProcParam *p_param = param;
    BrBvSendWork *p_wk;
    int i;

    p_wk = GFL_ProcInitSubsystem(proc, sizeof(BrBvSendWork), BrProcSys_GetHeapID(p_param->procSys));
    sys_memset(p_wk, 0, sizeof(BrBvSendWork));
    p_wk->p_param = p_param;
    p_wk->heapId = BrProcSys_GetHeapID(p_param->procSys);

    p_wk->seq = BrSeq_Init(p_wk, BrBvSend_SeqFadeIn, p_wk->heapId);
    p_wk->que = func_02021998(p_wk->heapId);
    p_wk->text = BrText_Init(p_param->res, p_wk->que, p_wk->heapId);
    BrText_Print(p_wk->text, p_param->res, 0x6f);
    for (i = 0; i < 2; i++) {
        p_wk->ballEff[i] = BrBallEff_Init(p_param->unit, p_param->res, i, p_wk->heapId);
    }
    {
        BrPoint pos = { 0x80, 0x60 };

        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_ROT, &pos);
    }
    return TRUE;
}

static BOOL BrBvSend_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvSendProcParam *p_param = param;
    BrBvSendWork *p_wk = work;
    int i;

    for (i = 0; i < 2; i++) {
        BrBallEff_Exit(p_wk->ballEff[i]);
    }
    BrText_Exit(p_wk->text, p_param->res);
    func_02021a18(p_wk->que);
    BrSeq_Exit(p_wk->seq);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BrBvSend_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvSendWork *p_wk = work;
    int i;

    if (!GameData_CheckEventsPaused(p_wk->p_param->gameData) && BrNet_CheckError(p_wk->p_param->net) == 2) {
        BrProcSys_Abort(p_wk->p_param->procSys);
        return TRUE;
    }

    BrSeq_Main(p_wk->seq);
    if (BrSeq_IsEnd(p_wk->seq)) {
        return TRUE;
    }

    for (i = 0; i < 2; i++) {
        BrBallEff_Main(p_wk->ballEff[i]);
    }
    func_02021a3c(p_wk->que);
    BrText_Main(p_wk->text);
    return FALSE;
}

static void BrBvSend_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_NEXT,
    };
    BrBvSendWork *p_wk = p_wk_adrs;

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
        BrSeq_SetNext(p_seqwk, BrBvSend_SeqMain);
        break;
    }
}

static void BrBvSend_SeqFadeOut(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrBvSendWork *p_wk = p_wk_adrs;

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

// Loads the player's video, uploads it and shows the result until the screen is touched
static void BrBvSend_SeqMain(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_LOAD,
        SEQ_UPLOAD_START,
        SEQ_UPLOAD_WAIT,
        SEQ_END,
        SEQ_MSG_WAIT,
    };
    BrBvSendWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_LOAD: {
        u32 result;

        func_0200bc9c(GameData_GetSaveControl(p_wk->p_param->gameData), HEAPID_BATTLE_RECORDER_SYS, &result, 0);
        if (result == 1) {
            *p_seq = SEQ_UPLOAD_START;
        } else {
            BrSeq_SetNext(p_seqwk, BrBvSend_SeqFadeOut);
        }
        break;
    }
    case SEQ_UPLOAD_START:
        p_wk->cnt = 0;
        BrNet_StartRequest(p_wk->p_param->net, BR_NET_REQUEST_2, NULL);
        *p_seq = SEQ_UPLOAD_WAIT;
        break;
    case SEQ_UPLOAD_WAIT:
        // The balls turn for at least 96 frames
        p_wk->cnt++;
        if (BrNet_IsRequestEnd(p_wk->p_param->net)) {
            u64 number = 0;

            if (p_wk->cnt >= 96) {
                u32 msgID;
                u32 ret = BrNet_GetResultMsg(p_wk->p_param->net, &msgID);

                if (ret == 0) {
                    if (BrNet_GetUploadVideoNumber(p_wk->p_param->net, &number)) {
                        MsgData *msg = BrRes_GetMsgData(p_wk->p_param->res);
                        WordSet *wordSet = BrRes_GetWordSet(p_wk->p_param->res);
                        StrBuf *src = GFL_MsgDataLoadStrbufNew(msg, 0x14e);
                        StrBuf *str = GFL_StrBufCreate(0x80, HEAPID_TAIL(HEAPID_BATTLE_RECORDER_SYS));
                        // The video's number, in blocks of five digits
                        u64 high = number / 100000;

                        WordSetNumber(wordSet, 2, number % 100000, 5, 2, TRUE);
                        WordSetNumber(wordSet, 1, high % 100000, 5, 2, TRUE);
                        WordSetNumber(wordSet, 0, high / 100000, 2, 2, TRUE);
                        GFL_WordSetFormatStrbuf(wordSet, str, src);
                        BrText_PrintBuf(p_wk->text, p_wk->p_param->res, str);
                        GFL_StrBufFree(str);
                        GFL_StrBufFree(src);
                    }
                } else if (ret == 3) {
                    BrProcSys_Abort(p_wk->p_param->procSys);
                    BrSeq_End(p_seqwk);
                    return;
                } else {
                    BrText_Print(p_wk->text, p_wk->p_param->res, msgID);
                }
                BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_NONE, NULL);
                *p_seq = SEQ_MSG_WAIT;
            }
        }
        break;
    case SEQ_END:
        BrSeq_SetNext(p_seqwk, BrBvSend_SeqFadeOut);
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
    }
}
