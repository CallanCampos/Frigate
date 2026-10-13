// The Battle Recorder's battle video saving screen: it downloads the battle video of the Global Link that the record
// screen showed, lets the player choose one of the three slots for downloaded videos, asks before it overwrites one,
// saves the video and tells the Global Link that it was saved. The name is a guess: the ROM gives none, and br_core.c
// runs it as BR_PROCID_BV_SAVE

#include "types.h"
#include "app/battle_recorder/br_bvsave_proc.h"
#include "app/battle_rec_ov.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_net.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "save/battle_rec.h"
#include "save/gds_profile.h"
#include "system/bmp_oam.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The buttons of the slot page: the three slots for downloaded videos, then the one to go back
enum {
    BTN_SLOT1,
    BTN_SLOT2,
    BTN_SLOT3,
    BTN_RETURN,
    BTN_MAX,
};

// The answers of the question before a slot is overwritten
enum {
    MSGWIN_YES,
    MSGWIN_NO,
    MSGWIN_MAX,
};

#define BR_BVSAVE_PRINT_COLOR 0x3da0

typedef struct BrBvSaveWork BrBvSaveWork;

// What makes and removes a page. Each is called once and then cleared
typedef struct {
    void (*create)(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param);
    void (*delete)(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param);
} BrBvSavePage;

struct BrBvSaveWork {
    BmpOamSys *bmpoam;
    PrintQueue *que;
    BrMsgWin *text;
    BrSeq *seq;
    // The slot button chosen. Its slot is one more, since the first slot is for the player's own video
    unsigned long select;
    BrBtn *btn[BTN_MAX];
    u32 unk24;
    BrMsgWin *msgWin[MSGWIN_MAX];
    // For the main and the sub screen
    BrBallEffect *ballEff[2];
    u16 heapId;
    BrBvSaveProcParam *p_param;
    u16 saveSeq;
    u16 saveSeq2;
    u32 cnt;
    u32 netCnt;
    BrBvSavePage page;
};

// Where a window is, in tiles, and its message
typedef struct {
    u8 x;
    u8 y;
    u8 w;
    u8 h;
    u32 msgID;
} BrBvSaveMsgWinData;

static BOOL BrBvSave_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBvSave_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBvSave_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrBvSave_SeqDownload(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvSave_SeqSelect(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvSave_SeqConfirm(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvSave_SeqSave(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvSave_CreatePage(BrBvSavePage *page, BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param);
static void BrBvSave_DeletePage(BrBvSavePage *page, BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param);
static BrBvSavePage BrBvSave_GetConfirmPage(void);
static void BrBvSave_CreateConfirm(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param);
static void BrBvSave_DeleteConfirm(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param);
static BrBvSavePage BrBvSave_GetDownloadPage(void);
static void BrBvSave_CreateDownload(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param);
static void BrBvSave_DeleteDownload(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param);
static BrBvSavePage BrBvSave_GetSelectPage(void);
static void BrBvSave_CreateSelect(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param);
static void BrBvSave_DeleteSelect(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param);
static BOOL BrBvSave_GetTrgYes(BrBvSaveWork *p_wk, u32 x, u32 y);
static BOOL BrBvSave_GetTrgNo(BrBvSaveWork *p_wk, u32 x, u32 y);

const GameProcFunctions BR_BVSAVE_PROC_FUNCTIONS = {
    BrBvSave_ProcInit,
    BrBvSave_ProcMain,
    BrBvSave_ProcExit,
};

static BOOL BrBvSave_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvSaveProcParam *p_param = param;
    BrBvSaveWork *p_wk;
    int i;

    p_wk = GFL_ProcInitSubsystem(proc, sizeof(BrBvSaveWork), BrProcSys_GetHeapID(p_param->procSys));
    sys_memset(p_wk, 0, sizeof(BrBvSaveWork));
    p_wk->p_param = p_param;
    p_wk->heapId = BrProcSys_GetHeapID(p_param->procSys);

    p_wk->bmpoam = BmpOam_Init(p_wk->heapId, p_param->unit);
    p_wk->seq = BrSeq_Init(p_wk, BrBvSave_SeqDownload, p_wk->heapId);
    p_wk->que = func_02021998(p_wk->heapId);
    for (i = 0; i < 2; i++) {
        p_wk->ballEff[i] = BrBallEff_Init(p_param->unit, p_param->res, i, p_wk->heapId);
    }
    return TRUE;
}

static BOOL BrBvSave_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvSaveWork *p_wk = work;
    int i;

    BrBvSave_DeletePage(&p_wk->page, p_wk, p_wk->p_param);
    for (i = 0; i < 2; i++) {
        BrBallEff_Exit(p_wk->ballEff[i]);
    }
    if (p_wk->text != NULL) {
        BrText_Exit(p_wk->text, p_wk->p_param->res);
        p_wk->text = NULL;
    }
    func_02021a18(p_wk->que);
    BrSeq_Exit(p_wk->seq);
    BmpOam_Exit(p_wk->bmpoam);
    BrProcSys_Pop(p_wk->p_param->procSys);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BrBvSave_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvSaveWork *p_wk = work;
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

// Downloads the whole video, and goes on to the slots unless the video is saved already
static void BrBvSave_SeqDownload(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_INIT,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_DOWNLOAD_START,
        SEQ_DOWNLOAD_WAIT,
        SEQ_DOWNLOAD_END,
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_NEXT,
        SEQ_MSG_WAIT,
        SEQ_ERROR_FADEOUT_START,
        SEQ_ERROR_FADEOUT_WAIT,
        SEQ_ERROR_END,
    };
    BrBvSaveWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_INIT:
        p_wk->page = BrBvSave_GetDownloadPage();
        BrBvSave_CreatePage(&p_wk->page, p_wk, p_wk->p_param);
        *p_seq = SEQ_FADEIN_START;
        break;
    case SEQ_FADEIN_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_DOWNLOAD_START;
        }
        break;
    case SEQ_DOWNLOAD_START: {
        BrNetRequestParam req;

        sys_memset(&req, 0, sizeof(BrNetRequestParam));
        req.dataNumber = p_wk->p_param->videoNumber;
        BrNet_StartRequest(p_wk->p_param->net, BR_NET_REQUEST_3, &req);
        p_wk->cnt = 0;
        *p_seq = SEQ_DOWNLOAD_WAIT;
        break;
    }
    case SEQ_DOWNLOAD_WAIT:
        // The balls turn for at least 96 frames
        p_wk->cnt++;
        if (BrNet_IsRequestEnd(p_wk->p_param->net) && p_wk->cnt > 96) {
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_NONE, NULL);
            *p_seq = SEQ_DOWNLOAD_END;
        }
        break;
    case SEQ_DOWNLOAD_END: {
        u32 msgID;

        if (!BrNet_GetResultMsg(p_wk->p_param->net, &msgID)) {
            u32 value;
            BattleRecVideo *video;

            if (BrNet_GetDownloadVideo(p_wk->p_param->net, (void **)&video, &value)) {
                BOOL isSaved;
                u64 number;
                int i;

                func_0200c0d0((GdsProfile *)video->profile, &video->header, &video->body,
                              GameData_GetSaveControl(p_wk->p_param->gameData));
                isSaved = FALSE;
                number = func_0200c124(func_0200c0c0(), BATTLE_REC_HEADER_DATA_NUMBER, 0);
                for (i = BR_RECORD_MODE_OTHER1; i <= BR_RECORD_MODE_OTHER3; i++) {
                    if (p_wk->p_param->recordInfo->videoNumber[i] == number) {
                        isSaved = TRUE;
                        break;
                    }
                }
                if (isSaved == TRUE) {
                    BrText_Print(p_wk->text, p_wk->p_param->res, 0x13c);
                    *p_seq = SEQ_MSG_WAIT;
                } else {
                    *p_seq = SEQ_FADEOUT_START;
                }
            } else {
                BrText_Print(p_wk->text, p_wk->p_param->res, 0x153);
                *p_seq = SEQ_MSG_WAIT;
            }
        } else {
            BrText_Print(p_wk->text, p_wk->p_param->res, msgID);
            *p_seq = SEQ_MSG_WAIT;
        }
        break;
    }
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
            *p_seq = SEQ_ERROR_FADEOUT_START;
        }
        break;
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_NEXT;
        }
        break;
    case SEQ_NEXT:
        BrBvSave_DeletePage(&p_wk->page, p_wk, p_wk->p_param);
        BrSeq_SetNext(p_seqwk, BrBvSave_SeqSelect);
        break;
    case SEQ_ERROR_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_ERROR_FADEOUT_WAIT;
        break;
    case SEQ_ERROR_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_ERROR_END;
        }
        break;
    case SEQ_ERROR_END:
        BrBvSave_DeletePage(&p_wk->page, p_wk, p_wk->p_param);
        BrSeq_End(p_seqwk);
        break;
    }
}

// The player chooses a slot, or goes back
static void BrBvSave_SeqSelect(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_INIT,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_INPUT,
        SEQ_SAVE_FADEOUT_START,
        SEQ_SAVE_FADEOUT_WAIT,
        SEQ_SAVE_NEXT,
        SEQ_CONFIRM_FADEOUT_START,
        SEQ_CONFIRM_FADEOUT_WAIT,
        SEQ_CONFIRM_NEXT,
        SEQ_RETURN_FADEOUT_START,
        SEQ_RETURN_FADEOUT_WAIT,
        SEQ_RETURN_END,
    };
    BrBvSaveWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_INIT:
        p_wk->page = BrBvSave_GetSelectPage();
        BrBvSave_CreatePage(&p_wk->page, p_wk, p_wk->p_param);
        *p_seq = SEQ_FADEIN_START;
        break;
    case SEQ_FADEIN_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_INPUT;
        }
        break;
    case SEQ_INPUT: {
        u32 x;
        u32 y;

        if (func_0203dac8(&x, &y)) {
            int i;

            for (i = BTN_SLOT1; i <= BTN_SLOT3; i++) {
                if (BrBtn_GetTrg(p_wk->btn[i], x, y)) {
                    BrPoint pos;

                    pos.x = x;
                    pos.y = y;
                    BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
                    p_wk->select = i;
                    // A slot that holds a video is asked about first
                    if (p_wk->p_param->recordInfo->isValid[p_wk->select + 1]) {
                        *p_seq = SEQ_CONFIRM_FADEOUT_START;
                    } else {
                        *p_seq = SEQ_SAVE_FADEOUT_START;
                    }
                }
            }
            if (BrBtn_GetTrg(p_wk->btn[BTN_RETURN], x, y)) {
                BrPoint pos;

                pos.x = x;
                pos.y = y;
                BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
                *p_seq = SEQ_RETURN_FADEOUT_START;
            }
        }
        break;
    }
    case SEQ_SAVE_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_SAVE_FADEOUT_WAIT;
        break;
    case SEQ_SAVE_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_SAVE_NEXT;
        }
        break;
    case SEQ_SAVE_NEXT:
        BrBvSave_DeletePage(&p_wk->page, p_wk, p_wk->p_param);
        BrSeq_SetNext(p_wk->seq, BrBvSave_SeqSave);
        break;
    case SEQ_CONFIRM_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        (*p_seq)++;
        break;
    case SEQ_CONFIRM_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            (*p_seq)++;
        }
        break;
    case SEQ_CONFIRM_NEXT:
        BrBvSave_DeletePage(&p_wk->page, p_wk, p_wk->p_param);
        BrSeq_SetNext(p_wk->seq, BrBvSave_SeqConfirm);
        break;
    case SEQ_RETURN_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_RETURN_FADEOUT_WAIT;
        break;
    case SEQ_RETURN_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_RETURN_END;
        }
        break;
    case SEQ_RETURN_END:
        BrBvSave_DeletePage(&p_wk->page, p_wk, p_wk->p_param);
        BrSeq_End(p_seqwk);
        break;
    }
}

// Asks whether to overwrite the video of the chosen slot
static void BrBvSave_SeqConfirm(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_INIT,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_INPUT,
        SEQ_YES_FADEOUT_START,
        SEQ_YES_FADEOUT_WAIT,
        SEQ_YES_NEXT,
        SEQ_NO_FADEOUT_START,
        SEQ_NO_FADEOUT_WAIT,
        SEQ_NO_NEXT,
    };
    BrBvSaveWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_INIT:
        p_wk->page = BrBvSave_GetConfirmPage();
        BrBvSave_CreatePage(&p_wk->page, p_wk, p_wk->p_param);
        (*p_seq)++;
        break;
    case SEQ_FADEIN_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        (*p_seq)++;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            (*p_seq)++;
        }
        break;
    case SEQ_INPUT: {
        u32 x;
        u32 y;

        if (func_0203dac8(&x, &y)) {
            if (BrBvSave_GetTrgYes(p_wk, x, y)) {
                *p_seq = SEQ_YES_FADEOUT_START;
            }
            if (BrBvSave_GetTrgNo(p_wk, x, y)) {
                *p_seq = SEQ_NO_FADEOUT_START;
            }
        }
        break;
    }
    case SEQ_YES_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        (*p_seq)++;
        break;
    case SEQ_YES_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            (*p_seq)++;
        }
        break;
    case SEQ_YES_NEXT:
        BrBvSave_DeletePage(&p_wk->page, p_wk, p_wk->p_param);
        BrSeq_SetNext(p_seqwk, BrBvSave_SeqSave);
        break;
    case SEQ_NO_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        (*p_seq)++;
        break;
    case SEQ_NO_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            (*p_seq)++;
        }
        break;
    case SEQ_NO_NEXT:
        BrBvSave_DeletePage(&p_wk->page, p_wk, p_wk->p_param);
        BrSeq_SetNext(p_seqwk, BrBvSave_SeqSelect);
        break;
    }
}

// Saves the video to the chosen slot, tells the Global Link and shows the result until the screen is touched
static void BrBvSave_SeqSave(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_SAVE_INIT,
        SEQ_SAVE_MAIN,
        SEQ_NOTIFY_START,
        SEQ_NOTIFY_WAIT,
        SEQ_MSG_WAIT,
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrBvSaveWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEIN_START: {
        BrPoint pos;

        if (p_wk->text == NULL) {
            p_wk->text = BrText_Init(p_wk->p_param->res, p_wk->que, p_wk->heapId);
        }
        BrText_Print(p_wk->text, p_wk->p_param->res, 0x13d);
        pos.x = 0x80;
        pos.y = 0x60;
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_ROT, &pos);
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        (*p_seq)++;
        break;
    }
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            (*p_seq)++;
        }
        break;
    case SEQ_SAVE_INIT:
        p_wk->cnt = 0;
        p_wk->saveSeq = 0;
        p_wk->saveSeq2 = 0;
        (*p_seq)++;
        break;
    case SEQ_SAVE_MAIN: {
        u32 ret = BattleRecOv_SaveVideo(p_wk->p_param->gameData, p_wk->select + 1,
                                        (u8)p_wk->p_param->unk24, &p_wk->saveSeq, &p_wk->saveSeq2, p_wk->heapId);

        if (ret == 2 || ret == 3) {
            p_wk->p_param->isSave = TRUE;
            p_wk->p_param->saveSlot = p_wk->select + 1;
            (*p_seq)++;
        }
        p_wk->cnt++;
        break;
    }
    case SEQ_NOTIFY_START: {
        BrNetRequestParam req;

        sys_memset(&req, 0, sizeof(BrNetRequestParam));
        req.dataNumber = func_0200c124(func_0200c0c0(), BATTLE_REC_HEADER_DATA_NUMBER, 0);
        BrNet_StartRequest(p_wk->p_param->net, BR_NET_REQUEST_8, &req);
        BrText_Print(p_wk->text, p_wk->p_param->res, 0x13f);
        p_wk->netCnt = 0;
        p_wk->cnt++;
        (*p_seq)++;
        break;
    }
    case SEQ_NOTIFY_WAIT:
        // The balls turn for at least 96 frames, and the second message shows for at least 60
        p_wk->cnt++;
        p_wk->netCnt++;
        if (BrNet_IsRequestEnd(p_wk->p_param->net) && p_wk->cnt > 96 && p_wk->netCnt > 60) {
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_NONE, NULL);
            BrText_Print(p_wk->text, p_wk->p_param->res, 0x140);
            (*p_seq)++;
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
            (*p_seq)++;
        }
        break;
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        (*p_seq)++;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            (*p_seq)++;
        }
        break;
    case SEQ_END:
        if (p_wk->text != NULL) {
            BrText_Exit(p_wk->text, p_wk->p_param->res);
            p_wk->text = NULL;
        }
        BrSeq_End(p_seqwk);
        break;
    }
}

static void BrBvSave_CreatePage(BrBvSavePage *page, BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param) {
    if (page->create != NULL) {
        page->create(p_wk, p_param);
        page->create = NULL;
    }
}

static void BrBvSave_DeletePage(BrBvSavePage *page, BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param) {
    if (page->delete != NULL) {
        page->delete(p_wk, p_param);
        page->delete = NULL;
    }
}

// The question before a slot is overwritten, which names the player of the video there, with its two answers
static const BrBvSavePage sBrBvSaveConfirmPage = { BrBvSave_CreateConfirm, BrBvSave_DeleteConfirm };

static BrBvSavePage BrBvSave_GetConfirmPage(void) {
    return sBrBvSaveConfirmPage;
}

static const BrBvSaveMsgWinData sBrBvSaveConfirmWins[MSGWIN_MAX] = {
    [MSGWIN_YES] = { 4, 6, 9, 2, 0x9f },
    [MSGWIN_NO] = { 18, 6, 9, 2, 0xa0 },
};

static void BrBvSave_CreateConfirm(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param) {
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
    func_0202437c(wordSet, 0, p_param->recordInfo->name[p_wk->select + 1],
                  p_param->recordInfo->sex[p_wk->select + 1], 1, 2);
    GFL_WordSetFormatStrbuf(wordSet, str, src);
    BrText_PrintBuf(p_wk->text, p_param->res, str);
    GFL_StrBufFree(str);
    GFL_StrBufFree(src);

    BrRes_LoadBG(p_param->res, 0x1e, p_wk->heapId);
    font = BrRes_GetFont(p_param->res);
    msg = BrRes_GetMsgData(p_param->res);
    for (i = 0; i < MSGWIN_MAX; i++) {
        const BrBvSaveMsgWinData *data = &sBrBvSaveConfirmWins[i];

        p_wk->msgWin[i] = BrMsgWin_Init(4, data->x, data->y, data->w, data->h, 14, p_wk->que, p_wk->heapId);
        BrMsgWin_Print(p_wk->msgWin[i], msg, data->msgID, font, BR_BVSAVE_PRINT_COLOR);
    }
}

static void BrBvSave_DeleteConfirm(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param) {
    int i;

    for (i = 0; i < MSGWIN_MAX; i++) {
        if (p_wk->msgWin[i] != NULL) {
            BrMsgWin_Exit(p_wk->msgWin[i]);
            p_wk->msgWin[i] = NULL;
        }
    }
    GFL_BGSysLoadScr(4);
    BrRes_UnloadBG(p_param->res, 0x1e);
    if (p_wk->text != NULL) {
        BrText_Exit(p_wk->text, p_param->res);
        p_wk->text = NULL;
    }
}

// The text box with the balls turning, while the video downloads
static const BrBvSavePage sBrBvSaveDownloadPage = { BrBvSave_CreateDownload, BrBvSave_DeleteDownload };

static BrBvSavePage BrBvSave_GetDownloadPage(void) {
    return sBrBvSaveDownloadPage;
}

static void BrBvSave_CreateDownload(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param) {
    BrPoint pos;

    if (p_wk->text == NULL) {
        p_wk->text = BrText_Init(p_wk->p_param->res, p_wk->que, p_wk->heapId);
    }
    BrText_Print(p_wk->text, p_wk->p_param->res, 0x14b);
    pos.x = 0x80;
    pos.y = 0x60;
    BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_ROT, &pos);
}

static void BrBvSave_DeleteDownload(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param) {
    BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_NONE, NULL);
    if (p_wk->text != NULL) {
        BrText_Exit(p_wk->text, p_wk->p_param->res);
        p_wk->text = NULL;
    }
}

// The buttons of the three slots, which name the players of the videos they hold, and the one to go back
static const BrBvSavePage sBrBvSaveSelectPage = { BrBvSave_CreateSelect, BrBvSave_DeleteSelect };

static BrBvSavePage BrBvSave_GetSelectPage(void) {
    return sBrBvSaveSelectPage;
}

static void BrBvSave_CreateSelect(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param) {
    Font *font;
    MsgData *msg;
    WordSet *wordSet;
    StrBuf *str;
    StrBuf *src;
    ClActorSetup setup;
    BrResObjData obj;
    GdsProfile *profile;
    StrBuf *name;
    int i;

    font = BrRes_GetFont(p_param->res);
    msg = BrRes_GetMsgData(p_param->res);
    wordSet = BrRes_GetWordSet(p_param->res);
    str = GFL_StrBufCreate(0x80, p_wk->heapId);
    src = GFL_StrBufCreate(0x80, p_wk->heapId);

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 48;
    setup.y = 40;
    setup.sequence = 11;
    setup.priority = 1;
    BrRes_LoadOBJ(p_param->res, BR_RES_OBJ_BROWSE_BTN_S, p_wk->heapId);
    BrRes_GetObjData(p_param->res, BR_RES_OBJ_BROWSE_BTN_S, &obj);
    for (i = BTN_SLOT1; i < BTN_RETURN; i++) {
        // The slot of the button
        int slot = BR_RECORD_MODE_OTHER1 + i;

        if (p_param->recordInfo->isValid[slot]) {
            GFL_MsgDataLoadStrbuf(msg, 0x1a, src);
            func_0202437c(wordSet, 0, p_param->recordInfo->name[slot], p_param->recordInfo->sex[slot], 1, 2);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
        } else {
            GFL_MsgDataLoadStrbuf(msg, 9, str);
        }
        p_wk->btn[i] = BrBtn_Init(&setup, str, 160, CLACT_SURFACE_SUB, p_param->unit, p_wk->bmpoam, font, &obj,
                                  p_wk->heapId);
        setup.y += 40;
    }
    setup.sequence = 1;
    p_wk->btn[BTN_RETURN] = BrBtn_InitEx(&setup, 4, 160, CLACT_SURFACE_SUB, p_param->unit, p_wk->bmpoam, font, msg,
                                         &obj, p_wk->heapId);

    // The text asks where to save the video of its player
    profile = getVSPlayerAllocation();
    name = GdsProfile_CreateNameStrBuf(profile, HEAPID_TAIL(p_wk->heapId));
    if (p_wk->text == NULL) {
        p_wk->text = BrText_Init(p_param->res, p_wk->que, p_wk->heapId);
    }
    GFL_MsgDataLoadStrbuf(msg, 0x5c, src);
    func_0202437c(wordSet, 0, name, GdsProfile_GetSex(profile), 1, 2);
    GFL_WordSetFormatStrbuf(wordSet, str, src);
    BrText_PrintBuf(p_wk->text, p_param->res, str);
    GFL_StrBufFree(name);
    GFL_StrBufFree(src);
    GFL_StrBufFree(str);
}

static void BrBvSave_DeleteSelect(BrBvSaveWork *p_wk, BrBvSaveProcParam *p_param) {
    int i;

    if (p_wk->text != NULL) {
        BrText_Exit(p_wk->text, p_param->res);
        p_wk->text = NULL;
    }
    for (i = 0; i < BTN_MAX; i++) {
        if (p_wk->btn[i] != NULL) {
            BrBtn_Exit(p_wk->btn[i]);
            p_wk->btn[i] = NULL;
        }
    }
    BrRes_UnloadOBJ(p_param->res, BR_RES_OBJ_BROWSE_BTN_S);
}

// The answers of the question
static BOOL BrBvSave_GetTrgYes(BrBvSaveWork *p_wk, u32 x, u32 y) {
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

static BOOL BrBvSave_GetTrgNo(BrBvSaveWork *p_wk, u32 x, u32 y) {
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
