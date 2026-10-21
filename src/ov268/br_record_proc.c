// The Battle Recorder's battle video screen: it shows a battle video's summary (the player, the battle mode, the
// video's number and the Pokémon) or its player's profile, with the buttons to play it, to go back and, for a video of
// the Global Link, to save it. It downloads a video found by its number first, and downloads the whole video when one
// of the Global Link is played. The name is a guess: the ROM gives none, and br_core.c runs it as BR_PROCID_RECORD

#include "types.h"
#include "app/battle_rec_ov.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_net.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_record_proc.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "save/battle_rec.h"
#include "save/gds_profile.h"
#include "system/bmp_oam.h"
#include "system/game_data.h"
#include "system/pms.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The buttons on the sub screen
enum {
    BTN_RETURN,
    BTN_SAVE,
    BTN_MAX,
};

// The windows of the summary
enum {
    MSGWIN_NAME,
    MSGWIN_MODE,
    MSGWIN_NUMBER,
    MSGWIN_COUNTER,
    MSGWIN_COUNTER_UNIT,
    MSGWIN_MAX,
};

// The windows on the sub screen: the title, or the two answers of the question before an unwatched video plays
#define BR_RECORD_TITLE_MAX 2

#define BR_RECORD_PRINT_COLOR 0x3da0

typedef struct {
    BmpOamSys *bmpoam;
    PrintQueue *que;
    BrSeq *seq;
    // For the main and the sub screen
    BrBallEffect *ballEff[2];
    u32 unk14;
    BrMsgWin *text;
    BrBtn *btn[BTN_MAX];
    u32 unk24;
    BrMsgWin *title[BR_RECORD_TITLE_MAX];
    BrMsgWin *msgWin[MSGWIN_MAX];
    BrProfile *profileWin;
    u32 iconPlt;
    u32 iconCell;
    u32 iconChr[BATTLE_REC_HEADER_PKM_MAX];
    ClActor *icon[BATTLE_REC_HEADER_PKM_MAX];
    // Whether the main screen shows the player's profile instead of the summary
    BOOL isProfile;
    HeapID heapId;
    u16 saveSeq;
    u16 saveSeq2;
    u32 cnt;
    GdsProfile *profile;
    BattleRecHeader *header;
    // Whether the video was watched to its end, without which a saved video has to be marked before it plays
    BOOL isSecure;
    BOOL canSave;
    BrRecordProcParam *p_param;
    BOOL isInfoCreated;
    BOOL isButtonsCreated;
    BOOL isProfileCreated;
    BOOL isTextCreated;
    BOOL isConfirmCreated;
} BrRecordWork;

// Where a window is, in tiles, and its message
typedef struct {
    u8 x;
    u8 y;
    u8 w;
    u8 h;
    u32 msgID;
} BrRecordMsgWinData;

static BOOL BrRecord_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrRecord_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrRecord_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrRecord_SeqStart(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrRecord_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrRecord_SeqFadeOut(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrRecord_SeqEnd(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrRecord_SeqMain(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrRecord_SeqChangePage(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrRecord_SeqDownloadOutline(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrRecord_SeqDownloadPlay(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrRecord_SeqSecureSave(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrRecord_SeqConfirmPlay(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrRecord_CreateInfo(BrRecordWork *p_wk, BrRecordProcParam *p_param);
static void BrRecord_CreateIcons(BrRecordWork *p_wk, ClActUnit *unit, BattleRecHeader *header, HeapID heapId);
static void BrRecord_CreateProfile(BrRecordWork *p_wk, BrRecordProcParam *p_param);
static void BrRecord_DeleteProfile(BrRecordWork *p_wk, BrRecordProcParam *p_param);
static void BrRecord_DeleteInfo(BrRecordWork *p_wk, BrRecordProcParam *p_param);
static void BrRecord_CreateButtons(BrRecordWork *p_wk, BrRecordProcParam *p_param);
static void BrRecord_DeleteButtons(BrRecordWork *p_wk, BrRecordProcParam *p_param);
static void BrRecord_CreateText(BrRecordWork *p_wk, BrRecordProcParam *p_param);
static void BrRecord_DeleteText(BrRecordWork *p_wk, BrRecordProcParam *p_param);
static void BrRecord_CreateConfirm(BrRecordWork *p_wk, BrRecordProcParam *p_param);
static void BrRecord_DeleteConfirm(BrRecordWork *p_wk, BrRecordProcParam *p_param);
static BOOL BrRecord_IsSmallTeamMode(BattleRecHeader *header);
static BOOL BrRecord_IsCounterMode(BattleRecHeader *header);
static BOOL BrRecord_GetTrgChange(BrRecordWork *p_wk, u32 x, u32 y);
static BOOL BrRecord_GetTrgPlay(BrRecordWork *p_wk, u32 x, u32 y);
static BOOL BrRecord_GetTrgYes(BrRecordWork *p_wk, u32 x, u32 y);
static BOOL BrRecord_GetTrgNo(BrRecordWork *p_wk, u32 x, u32 y);

const GameProcFunctions BR_RECORD_PROC_FUNCTIONS = {
    BrRecord_ProcInit,
    BrRecord_ProcMain,
    BrRecord_ProcExit,
};

// The answers of the question before an unwatched video plays. The last two entries are not read: the third is where
// BrRecord_CreateButtons puts the title
static const BrRecordMsgWinData sBrRecordConfirmWins[] = {
    { 4, 6, 9, 2, 0x9f },
    { 18, 6, 9, 2, 0xa0 },
    { 8, 3, 16, 2, 0x71 },
    { 0, 0, 0, 0, 0 },
};

// clang-format off
static const BrRecordMsgWinData sBrRecordInfoWins[MSGWIN_MAX] = {
    [MSGWIN_NAME] = { 4, 4, 24, 2, 8 },
    [MSGWIN_MODE] = { 4, 6, 24, 4, 0xa4 },
    [MSGWIN_NUMBER] = { 2, 21, 28, 2, 0xc },
    [MSGWIN_COUNTER] = { 3, 11, 7, 2, 0xa },
    [MSGWIN_COUNTER_UNIT] = { 3, 13, 7, 2, 0xb },
};

// clang-format on

// Where the Pokémon icons go: in two rows of six, or in four groups of three
static const struct {
    s16 x;
    s16 y;
} sBrRecordIconPos[2][BATTLE_REC_HEADER_PKM_MAX] = {
    {
        { 24, 152 },
        { 48, 152 },
        { 72, 152 },
        { 96, 152 },
        { 120, 152 },
        { 144, 152 },
        { 88, 104 },
        { 112, 104 },
        { 136, 104 },
        { 160, 104 },
        { 184, 104 },
        { 208, 104 },
    },
    {
        { 24, 136 },
        { 48, 136 },
        { 72, 136 },
        { 144, 136 },
        { 168, 136 },
        { 192, 136 },
        { 40, 160 },
        { 64, 160 },
        { 88, 160 },
        { 160, 160 },
        { 184, 160 },
        { 208, 160 },
    },
};

static BOOL BrRecord_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrRecordProcParam *p_param = param;
    BrRecordWork *p_wk;
    int i;

    p_wk = GFL_ProcInitSubsystem(proc, sizeof(BrRecordWork), BrProcSys_GetHeapID(p_param->procSys));
    MI_CpuFill8(p_wk, 0, sizeof(BrRecordWork));
    p_wk->heapId = BrProcSys_GetHeapID(p_param->procSys);
    p_wk->isProfile = FALSE;
    p_wk->p_param = p_param;

    p_wk->bmpoam = BmpOam_Init(p_wk->heapId, p_param->unit);
    p_wk->que = PrintQueue_CreateDefault(p_wk->heapId);
    p_wk->seq = BrSeq_Init(p_wk, NULL, p_wk->heapId);
    for (i = 0; i < 2; i++) {
        p_wk->ballEff[i] = BrBallEff_Init(p_param->unit, p_param->res, i, p_wk->heapId);
    }

    switch (p_wk->p_param->mode) {
    case BR_RECORD_MODE_BROWSE: {
        BrOutlineData *outline = p_wk->p_param->outline;

        p_wk->header = &outline->data[outline->select].header;
        p_wk->profile = &p_wk->p_param->outline->data[p_wk->p_param->outline->select].profile;
        p_wk->canSave = TRUE;
        if (*p_wk->p_param->unk3C && p_wk->p_param->isRecovery) {
            p_wk->p_param->outline->isSecure[p_wk->p_param->outline->select] = TRUE;
        }
        p_wk->isSecure = p_wk->p_param->outline->isSecure[p_wk->p_param->outline->select];
        break;
    }
    case BR_RECORD_MODE_MINE:
        p_wk->header = BattleRec_GetHeader();
        p_wk->profile = getVSPlayerAllocation();
        p_wk->isSecure = TRUE;
        p_wk->canSave = FALSE;
        break;
    case BR_RECORD_MODE_OTHER1:
    case BR_RECORD_MODE_OTHER2:
    case BR_RECORD_MODE_OTHER3:
        p_wk->header = BattleRec_GetHeader();
        p_wk->profile = getVSPlayerAllocation();
        p_wk->isSecure = BattleRec_GetHeaderValue(p_wk->header, BATTLE_REC_HEADER_SECURE, 0);
        p_wk->canSave = FALSE;
        break;
    case BR_RECORD_MODE_CODEIN:
        if (!p_wk->p_param->isRecovery) {
            // The video is downloaded first
            p_wk->header = NULL;
            p_wk->profile = NULL;
        } else {
            p_wk->header = BattleRec_GetHeader();
            p_wk->profile = getVSPlayerAllocation();
            p_wk->isSecure = *p_wk->p_param->unk3C;
        }
        p_wk->canSave = TRUE;
        break;
    }

    if (p_wk->p_param->mode == BR_RECORD_MODE_CODEIN) {
        if (!p_wk->p_param->isRecovery) {
            BrSeq_SetNext(p_wk->seq, BrRecord_SeqDownloadOutline);
        } else {
            BrSeq_SetNext(p_wk->seq, BrRecord_SeqStart);
        }
    } else if ((p_wk->p_param->mode == BR_RECORD_MODE_OTHER1 || p_wk->p_param->mode == BR_RECORD_MODE_OTHER2 ||
                p_wk->p_param->mode == BR_RECORD_MODE_OTHER3) &&
               p_wk->p_param->isRecovery && *p_wk->p_param->unk3C && !p_wk->isSecure) {
        // A saved video that has just been watched to its end for the first time is marked
        BrSeq_SetNext(p_wk->seq, BrRecord_SeqSecureSave);
    } else {
        BrSeq_SetNext(p_wk->seq, BrRecord_SeqStart);
    }
    return TRUE;
}

static BOOL BrRecord_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrRecordProcParam *p_param = param;
    BrRecordWork *p_wk = work;
    int i;

    BrRecord_DeleteProfile(p_wk, p_param);
    BrRecord_DeleteInfo(p_wk, p_param);
    BrRecord_DeleteButtons(p_wk, p_param);
    BrRecord_DeleteText(p_wk, p_param);
    BrRecord_DeleteConfirm(p_wk, p_param);
    p_param->unk4 = p_wk->isSecure;

    GFL_BGSysLoadScr(4);
    GFL_BGSysLoadScr(1);
    if (p_wk->text != NULL) {
        BrText_Exit(p_wk->text, p_wk->p_param->res);
        p_wk->text = NULL;
    }
    for (i = 0; i < 2; i++) {
        BrBallEff_Exit(p_wk->ballEff[i]);
    }
    BrSeq_Exit(p_wk->seq);
    PrintQueue_Free(p_wk->que);
    BmpOam_Exit(p_wk->bmpoam);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BrRecord_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BrRecordWork *p_wk = work;
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
    PrintQueue_Main(p_wk->que);
    for (i = 0; i < BR_RECORD_TITLE_MAX; i++) {
        if (p_wk->title[i] != NULL) {
            BrMsgWin_Main(p_wk->title[i]);
        }
    }
    for (i = 0; i < MSGWIN_MAX; i++) {
        if (p_wk->msgWin[i] != NULL) {
            BrMsgWin_Main(p_wk->msgWin[i]);
        }
    }
    if (p_wk->profileWin != NULL) {
        BrProfile_Main(p_wk->profileWin);
    }
    if (p_wk->text != NULL) {
        BrText_Main(p_wk->text);
    }
    return FALSE;
}

// Makes the screen and fades it in
static void BrRecord_SeqStart(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrRecordWork *p_wk = p_wk_adrs;

    BrRecord_CreateButtons(p_wk, p_wk->p_param);
    if (p_wk->isProfile) {
        BrRecord_CreateProfile(p_wk, p_wk->p_param);
    } else {
        BrRecord_CreateInfo(p_wk, p_wk->p_param);
    }
    BrSeq_SetNext(p_seqwk, BrRecord_SeqFadeIn);
}

static void BrRecord_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_NEXT,
    };
    BrRecordWork *p_wk = p_wk_adrs;

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
        BrSeq_SetNext(p_seqwk, BrRecord_SeqMain);
        break;
    }
}

static void BrRecord_SeqFadeOut(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_NEXT,
    };
    BrRecordWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
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
        BrSeq_SetNext(p_seqwk, BrRecord_SeqEnd);
        break;
    }
}

static void BrRecord_SeqEnd(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrRecordWork *p_wk = p_wk_adrs;

    if (p_wk->isProfile) {
        BrRecord_DeleteProfile(p_wk, p_wk->p_param);
    } else {
        BrRecord_DeleteInfo(p_wk, p_wk->p_param);
    }
    BrRecord_DeleteButtons(p_wk, p_wk->p_param);
    BrSeq_End(p_seqwk);
}

// Waits for the player to touch a button
static void BrRecord_SeqMain(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrRecordWork *p_wk = p_wk_adrs;
    u32 x;
    u32 y;

    if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
        if (BrRecord_GetTrgChange(p_wk, x, y)) {
            p_wk->isProfile ^= 1;
            BrSeq_SetNext(p_seqwk, BrRecord_SeqChangePage);
        }
        if (BrBtn_GetTrg(p_wk->btn[BTN_RETURN], x, y)) {
            BrPoint pos;

            pos.x = x;
            pos.y = y;
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            p_wk->p_param->result = 0;
            BrProcSys_Pop(p_wk->p_param->procSys);
            BrSeq_SetNext(p_seqwk, BrRecord_SeqFadeOut);
        }
        if (p_wk->btn[BTN_SAVE] != NULL && BrBtn_GetTrg(p_wk->btn[BTN_SAVE], x, y)) {
            BrPoint pos;

            pos.x = x;
            pos.y = y;
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            BrProcSys_Push(p_wk->p_param->procSys, BR_PROCID_BV_SAVE);
            p_wk->p_param->videoNumber = BattleRec_GetHeaderValue(p_wk->header, BATTLE_REC_HEADER_DATA_NUMBER, 0);
            BrSeq_SetNext(p_seqwk, BrRecord_SeqFadeOut);
        }
        if (BrRecord_GetTrgPlay(p_wk, x, y)) {
            if (p_wk->p_param->mode == BR_RECORD_MODE_BROWSE || p_wk->p_param->mode == BR_RECORD_MODE_CODEIN) {
                BrSeq_SetNext(p_seqwk, BrRecord_SeqDownloadPlay);
            } else if (!p_wk->isSecure) {
                BrSeq_SetNext(p_seqwk, BrRecord_SeqConfirmPlay);
            } else {
                p_wk->p_param->result = 1;
                BrProcSys_Interrupt(p_wk->p_param->procSys);
                BrSeq_SetNext(p_seqwk, BrRecord_SeqFadeOut);
            }
        }
    }
}

// Swaps the summary and the profile on the main screen
static void BrRecord_SeqChangePage(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_CHANGE,
        SEQ_FADEIN_WAIT,
    };
    BrRecordWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_MAIN, BR_FADE_DIR_OUT);
        (*p_seq)++;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            (*p_seq)++;
        }
        break;
    case SEQ_CHANGE: {
        Font *font = BrRes_GetFont(p_wk->p_param->res);
        MsgData *msg = BrRes_GetMsgData(p_wk->p_param->res);

        if (p_wk->isProfile) {
            BrRecord_DeleteInfo(p_wk, p_wk->p_param);
            BrMsgWin_Print(p_wk->title[0], msg, 0x72, font, BR_RECORD_PRINT_COLOR);
            BrRecord_CreateProfile(p_wk, p_wk->p_param);
        } else {
            BrRecord_DeleteProfile(p_wk, p_wk->p_param);
            BrMsgWin_Print(p_wk->title[0], msg, 0x71, font, BR_RECORD_PRINT_COLOR);
            BrRecord_CreateInfo(p_wk, p_wk->p_param);
        }
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_MAIN, BR_FADE_DIR_IN);
        (*p_seq)++;
        break;
    }
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            BrSeq_SetNext(p_seqwk, BrRecord_SeqMain);
        }
        break;
    }
}

// Downloads the video of the number the player entered and shows it, or shows the error and goes back
static void BrRecord_SeqDownloadOutline(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_INIT,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_DOWNLOAD_START,
        SEQ_DOWNLOAD_WAIT,
        SEQ_DOWNLOAD_END,
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_INFO_FADEIN_START,
        SEQ_INFO_FADEIN_WAIT,
        SEQ_NEXT,
        SEQ_MSG_WAIT,
        SEQ_ERROR_FADEOUT_START,
        SEQ_ERROR_FADEOUT_WAIT,
        SEQ_ERROR_END,
    };
    BrRecordWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_INIT:
        BrRecord_CreateText(p_wk, p_wk->p_param);
        BrText_Print(p_wk->text, p_wk->p_param->res, 0x14b);
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

        MI_CpuFill8(&req, 0, sizeof(BrNetRequestParam));
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
            BattleRecVideo *video;
            u32 value;

            if (BrNet_GetDownloadVideo(p_wk->p_param->net, (void **)&video, &value)) {
                p_wk->profile = (GdsProfile *)video->profile;
                p_wk->header = &video->header;
                *p_seq = SEQ_FADEOUT_START;
            } else {
                BrText_Print(p_wk->text, p_wk->p_param->res, 0x114);
                *p_seq = SEQ_MSG_WAIT;
            }
        } else {
            BrText_Print(p_wk->text, p_wk->p_param->res, msgID);
            *p_seq = SEQ_MSG_WAIT;
        }
        break;
    }
    case SEQ_MSG_WAIT:
        if (GFL_HIDIsTouchPressed()) {
            u32 x;
            u32 y;
            BrPoint pos;

            GFL_HIDGetTouchPressedPoint(&x, &y);
            pos.x = x;
            pos.y = y;
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            GFL_SndSEPlay(SEQ_SE_BREC_06);
            BrProcSys_Pop(p_wk->p_param->procSys);
            *p_seq = SEQ_ERROR_FADEOUT_START;
        }
        break;
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            BrRecord_DeleteText(p_wk, p_wk->p_param);
            BrRecord_CreateButtons(p_wk, p_wk->p_param);
            BrRecord_CreateInfo(p_wk, p_wk->p_param);
            *p_seq = SEQ_INFO_FADEIN_START;
        }
        break;
    case SEQ_INFO_FADEIN_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *p_seq = SEQ_INFO_FADEIN_WAIT;
        break;
    case SEQ_INFO_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_NEXT;
        }
        break;
    case SEQ_NEXT:
        BrSeq_SetNext(p_seqwk, BrRecord_SeqMain);
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
        BrRecord_DeleteText(p_wk, p_wk->p_param);
        BrSeq_End(p_seqwk);
        break;
    }
}

// Downloads the whole video of the Global Link and leaves to play it, or shows the error and the screen again
static void BrRecord_SeqDownloadPlay(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_CHANGE,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_DOWNLOAD_START,
        SEQ_DOWNLOAD_WAIT,
        SEQ_DOWNLOAD_END,
        SEQ_PLAY_FADEOUT_START,
        SEQ_PLAY_FADEOUT_WAIT,
        SEQ_PLAY_END,
        SEQ_MSG_WAIT,
        SEQ_ERROR_FADEOUT_START,
        SEQ_ERROR_FADEOUT_WAIT,
        SEQ_ERROR_END,
    };
    BrRecordWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_CHANGE;
        }
        break;
    case SEQ_CHANGE:
        if (p_wk->isProfile) {
            BrRecord_DeleteProfile(p_wk, p_wk->p_param);
        } else {
            BrRecord_DeleteInfo(p_wk, p_wk->p_param);
        }
        BrRecord_DeleteButtons(p_wk, p_wk->p_param);
        BrRecord_CreateText(p_wk, p_wk->p_param);
        BrText_Print(p_wk->text, p_wk->p_param->res, 0x14b);
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

        MI_CpuFill8(&req, 0, sizeof(BrNetRequestParam));
        req.dataNumber = BattleRec_GetHeaderValue(p_wk->header, BATTLE_REC_HEADER_DATA_NUMBER, 0);
        BrNet_StartRequest(p_wk->p_param->net, BR_NET_REQUEST_3, &req);
        p_wk->cnt = 0;
        *p_seq = SEQ_DOWNLOAD_WAIT;
        break;
    }
    case SEQ_DOWNLOAD_WAIT:
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
                BattleRec_StoreDownloaded((GdsProfile *)video->profile, &video->header, &video->body,
                                          GameData_GetSaveControl(p_wk->p_param->gameData));
                *p_wk->p_param->unk2C = (s32)value;
                p_wk->p_param->result = 1;
                BrProcSys_Interrupt(p_wk->p_param->procSys);
                *p_seq = SEQ_PLAY_FADEOUT_START;
            } else {
                BrText_Print(p_wk->text, p_wk->p_param->res, 0x114);
                *p_seq = SEQ_MSG_WAIT;
            }
        } else {
            BrText_Print(p_wk->text, p_wk->p_param->res, msgID);
            *p_seq = SEQ_MSG_WAIT;
        }
        break;
    }
    case SEQ_MSG_WAIT:
        if (GFL_HIDIsTouchPressed()) {
            u32 x;
            u32 y;
            BrPoint pos;

            GFL_HIDGetTouchPressedPoint(&x, &y);
            pos.x = x;
            pos.y = y;
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            GFL_SndSEPlay(SEQ_SE_BREC_06);
            *p_seq = SEQ_ERROR_FADEOUT_START;
        }
        break;
    case SEQ_PLAY_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_PLAY_FADEOUT_WAIT;
        break;
    case SEQ_PLAY_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_PLAY_END;
        }
        break;
    case SEQ_PLAY_END:
        if (!GFL_SndIsPlaying(SEQ_SE_BREC_04)) {
            BrRecord_DeleteText(p_wk, p_wk->p_param);
            BrSeq_End(p_wk->seq);
        }
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
        BrRecord_DeleteText(p_wk, p_wk->p_param);
        BrSeq_SetNext(p_seqwk, BrRecord_SeqStart);
        break;
    }
}

// Marks the saved video as watched to its end, then shows the screen
static void BrRecord_SeqSecureSave(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_WAIT,
        SEQ_SAVE_INIT,
        SEQ_SAVE_MAIN,
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrRecordWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEIN_START:
        BrRecord_CreateText(p_wk, p_wk->p_param);
        BrText_Print(p_wk->text, p_wk->p_param->res, 0x152);
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_WAIT;
        }
        break;
    case SEQ_WAIT:
        *p_seq = SEQ_SAVE_INIT;
        break;
    case SEQ_SAVE_INIT:
        p_wk->saveSeq = 0;
        p_wk->saveSeq2 = 0;
        *p_seq = SEQ_SAVE_MAIN;
        break;
    case SEQ_SAVE_MAIN:
        GFL_ASSERT(p_wk->p_param->mode <= BR_RECODE_PROC_OTHER_02);
        if (BattleRecOv_SaveSecureVideo(p_wk->p_param->gameData, p_wk->p_param->mode, &p_wk->saveSeq, &p_wk->saveSeq2,
                                        p_wk->heapId) == 2) {
            BattleRec_DecryptBody();
            p_wk->isSecure = BattleRec_GetHeaderValue(p_wk->header, BATTLE_REC_HEADER_SECURE, 0);
            *p_seq = SEQ_FADEOUT_START;
        }
        break;
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
        BrRecord_DeleteText(p_wk, p_wk->p_param);
        BrSeq_SetNext(p_seqwk, BrRecord_SeqStart);
        break;
    }
}

// Asks before a saved video that was never watched to its end plays, since the game is saved first
static void BrRecord_SeqConfirmPlay(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_CHANGE,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_WAIT,
        SEQ_INPUT,
        SEQ_YES_FADEOUT_START,
        SEQ_YES_FADEOUT_WAIT,
        SEQ_YES_END,
        SEQ_NO_FADEOUT_START,
        SEQ_NO_FADEOUT_WAIT,
        SEQ_NO_END,
        SEQ_SAVE_INIT,
        SEQ_SAVE_FADEIN_START,
        SEQ_SAVE_FADEIN_WAIT,
        SEQ_SAVE_WAIT,
        SEQ_SAVE_START,
        SEQ_SAVE_MAIN,
        SEQ_PLAY_FADEOUT_START,
        SEQ_PLAY_FADEOUT_WAIT,
        SEQ_PLAY_END,
    };
    BrRecordWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_CHANGE;
        }
        break;
    case SEQ_CHANGE:
        if (p_wk->isProfile) {
            BrRecord_DeleteProfile(p_wk, p_wk->p_param);
        } else {
            BrRecord_DeleteInfo(p_wk, p_wk->p_param);
        }
        BrRecord_DeleteButtons(p_wk, p_wk->p_param);
        BrRecord_CreateConfirm(p_wk, p_wk->p_param);
        *p_seq = SEQ_FADEIN_START;
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
    case SEQ_WAIT:
        (*p_seq)++;
        break;
    case SEQ_INPUT: {
        u32 x;
        u32 y;

        if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
            if (BrRecord_GetTrgYes(p_wk, x, y)) {
                *p_seq = SEQ_YES_FADEOUT_START;
            } else if (BrRecord_GetTrgNo(p_wk, x, y)) {
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
    case SEQ_YES_END:
        BrRecord_DeleteConfirm(p_wk, p_wk->p_param);
        *p_seq = SEQ_SAVE_INIT;
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
    case SEQ_NO_END:
        BrRecord_DeleteConfirm(p_wk, p_wk->p_param);
        BrSeq_SetNext(p_seqwk, BrRecord_SeqStart);
        break;
    case SEQ_SAVE_INIT:
        BrRecord_CreateText(p_wk, p_wk->p_param);
        *p_seq = SEQ_SAVE_FADEIN_START;
        break;
    case SEQ_SAVE_FADEIN_START:
        BrText_Print(p_wk->text, p_wk->p_param->res, 0x152);
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *p_seq = SEQ_SAVE_FADEIN_WAIT;
        break;
    case SEQ_SAVE_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_SAVE_WAIT;
        }
        break;
    case SEQ_SAVE_WAIT:
        *p_seq = SEQ_SAVE_START;
        break;
    case SEQ_SAVE_START:
        GameData_StartSave(p_wk->p_param->gameData);
        *p_seq = SEQ_SAVE_MAIN;
        break;
    case SEQ_SAVE_MAIN:
        if (GameData_UpdateSave(p_wk->p_param->gameData) == 2) {
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_NONE, NULL);
            *p_seq = SEQ_PLAY_FADEOUT_START;
        }
        break;
    case SEQ_PLAY_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_PLAY_FADEOUT_WAIT;
        break;
    case SEQ_PLAY_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_PLAY_END;
        }
        break;
    case SEQ_PLAY_END:
        if (!GFL_SndIsPlaying(SEQ_SE_BREC_04)) {
            BrRecord_DeleteText(p_wk, p_wk->p_param);
            p_wk->p_param->result = 1;
            BrProcSys_Interrupt(p_wk->p_param->procSys);
            BrSeq_End(p_seqwk);
        }
        break;
    }
}

// Makes the summary of the video on the main screen
static void BrRecord_CreateInfo(BrRecordWork *p_wk, BrRecordProcParam *p_param) {
    int i;
    Font *font;
    MsgData *msg;
    WordSet *wordSet;
    BOOL isPrint;
    StrBuf *str;

    if (p_wk->isInfoCreated) {
        return;
    }
    p_wk->isInfoCreated = TRUE;

    if (BrRecord_IsSmallTeamMode(p_wk->header)) {
        BrRes_LoadBG(p_param->res, 7, p_wk->heapId);
    } else {
        BrRes_LoadBG(p_param->res, 6, p_wk->heapId);
    }
    font = BrRes_GetFont(p_param->res);
    msg = BrRes_GetMsgData(p_param->res);
    wordSet = BrRes_GetWordSet(p_param->res);

    for (i = 0; i < MSGWIN_MAX; i++) {
        const BrRecordMsgWinData *data = &sBrRecordInfoWins[i];

        p_wk->msgWin[i] = BrMsgWin_Init(1, data->x, data->y, data->w, data->h, 14, p_wk->que, p_wk->heapId);
        switch (i) {
        case MSGWIN_NAME: {
            StrBuf *name;
            StrBuf *src;

            str = GFL_StrBufCreate(0x80, p_wk->heapId);
            name = GdsProfile_CreateNameStrBuf(p_wk->profile, p_wk->heapId);
            src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            GFL_WordSetSetStrbuf(wordSet, 0, name, GdsProfile_GetSex(p_wk->profile), 1, 2);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            GFL_StrBufFree(name);
            GFL_StrBufFree(src);
            BrMsgWin_SetPos(p_wk->msgWin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
            isPrint = TRUE;
            break;
        }
        case MSGWIN_MODE: {
            u32 mode = BattleRec_GetHeaderValue(p_wk->header, BATTLE_REC_HEADER_MODE, 0);

            str = GFL_MsgDataLoadStrbufNew(msg, mode + data->msgID);
            BrMsgWin_SetPos(p_wk->msgWin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
            isPrint = TRUE;
            break;
        }
        case MSGWIN_NUMBER: {
            // The video's number is shown as 12-34567-89012
            u64 number = BattleRec_GetHeaderValue(p_wk->header, BATTLE_REC_HEADER_DATA_NUMBER, 0);
            u32 low;
            u32 middle;
            u32 high;
            PMSData pms;
            StrBuf *intro;
            StrBuf *src;

            low = number % 100000;
            number /= 100000;
            middle = number % 100000;
            number /= 100000;
            high = number;

            intro = GdsProfile_GetIntroduction(p_wk->profile, &pms, p_wk->heapId);
            if (intro != NULL) {
                src = GFL_MsgDataLoadStrbufNew(msg, 0xd);
                GFL_StrBufFree(intro);
            } else {
                src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
            }
            str = GFL_StrBufCreate(0x80, p_wk->heapId);
            WordSetNumber(wordSet, 2, low, 5, 2, TRUE);
            WordSetNumber(wordSet, 1, middle, 5, 2, TRUE);
            WordSetNumber(wordSet, 0, high, 2, 2, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, src);
            BrMsgWin_SetPos(p_wk->msgWin[i], 0, 1, BR_MSGWIN_POS_ABSOLUTE);
            GFL_StrBufFree(src);
            isPrint = TRUE;
            break;
        }
        case MSGWIN_COUNTER:
            if (BrRecord_IsCounterMode(p_wk->header)) {
                u32 counter = BattleRec_GetHeaderValue(p_wk->header, BATTLE_REC_HEADER_COUNTER, 0);
                StrBuf *src;

                str = GFL_StrBufCreate(0x80, p_wk->heapId);
                src = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
                WordSetNumber(wordSet, 0, counter, 4, 0, TRUE);
                GFL_WordSetFormatStrbuf(wordSet, str, src);
                GFL_StrBufFree(src);
                BrMsgWin_SetPos(p_wk->msgWin[i], 0, 1, BR_MSGWIN_POS_WH_CENTER);
                isPrint = TRUE;
            } else {
                isPrint = FALSE;
            }
            break;
        case MSGWIN_COUNTER_UNIT:
            if (BrRecord_IsCounterMode(p_wk->header)) {
                str = GFL_MsgDataLoadStrbufNew(msg, data->msgID);
                BrMsgWin_SetPos(p_wk->msgWin[i], 0, 1, BR_MSGWIN_POS_WH_CENTER);
                isPrint = TRUE;
            } else {
                isPrint = FALSE;
            }
            break;
        }
        if (isPrint) {
            BrMsgWin_PrintBuf(p_wk->msgWin[i], str, font, BR_RECORD_PRINT_COLOR);
            GFL_StrBufFree(str);
        }
    }

    BrRecord_CreateIcons(p_wk, p_param->unit, p_wk->header, p_wk->heapId);
}

// Makes the icons of the video's Pokémon, each side's closed up to its first places
static void BrRecord_CreateIcons(BrRecordWork *p_wk, ClActUnit *unit, BattleRecHeader *header, HeapID heapId) {
    u32 species[BATTLE_REC_HEADER_PKM_MAX];
    u32 form[BATTLE_REC_HEADER_PKM_MAX];
    u32 gender[BATTLE_REC_HEADER_PKM_MAX];
    int cnt = 0;
    int layout;
    ArcTool *handle;
    int step;
    int start;
    int end;
    int i;
    int idx;

    handle = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_TAIL(heapId));
    p_wk->iconPlt = ClActSys_LoadPlttEx(handle, PokeIcon_GetPaletteFile(), CLACT_SURFACE_MAIN, 0x80, heapId);
    p_wk->iconCell = ClActSys_LoadCellAnims(handle, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), heapId);

    idx = 0;
    step = 6;
    start = 0;
    end = 6;
    layout = 0;
    if (BrRecord_IsSmallTeamMode(header) == TRUE) {
        step = 3;
        end = 3;
        layout = 1;
    }
    do {
        for (i = start; i < end; i++) {
            u32 monsno;
            u32 formno;
            u32 sex;

            species[idx] = 0xff;
            monsno = BattleRec_GetHeaderValue(header, BATTLE_REC_HEADER_SPECIES, i);
            formno = BattleRec_GetHeaderValue(header, BATTLE_REC_HEADER_FORM, i);
            sex = BattleRec_GetHeaderValue(header, BATTLE_REC_HEADER_GENDER, i);
            if (monsno != 0) {
                species[idx] = monsno;
                form[idx] = formno;
                gender[idx] = sex;
                idx++;
            }
        }
        for (i = idx; i < end; i++) {
            species[idx] = 0;
            form[idx] = 0;
            gender[idx] = 0;
            idx++;
        }
        start += step;
        end += step;
    } while (end <= BATTLE_REC_HEADER_PKM_MAX);

    for (i = 0; i < BATTLE_REC_HEADER_PKM_MAX; i++) {
        p_wk->icon[i] = NULL;
        p_wk->iconChr[i] = -1;
        if (species[i] != 0) {
            ClActorSetup setup;

            p_wk->iconChr[i] = ClActSys_LoadChars(handle, PokeParty_GetIconIndex(species[i], form[i], gender[i], FALSE),
                                                  FALSE, CLACT_SURFACE_MAIN, heapId);
            MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
            setup.x = sBrRecordIconPos[layout][i].x + 8;
            setup.y = sBrRecordIconPos[layout][i].y - 12;
            setup.sequence = 1;
            setup.priority = BATTLE_REC_HEADER_PKM_MAX - cnt;
            p_wk->icon[cnt] = ClActor_Create(unit, p_wk->iconChr[i], p_wk->iconPlt, p_wk->iconCell, &setup, 0, heapId);
            ClActor_SetObjMode(p_wk->icon[cnt], 1);
            ClActor_SetPalette(p_wk->icon[cnt], PokeIcon_GetPalette(species[i], form[i], gender[i], FALSE), 0);
            cnt++;
        }
    }

    GFL_ArcToolFree(handle);
}

static void BrRecord_CreateProfile(BrRecordWork *p_wk, BrRecordProcParam *p_param) {
    if (p_wk->profileWin == NULL && !p_wk->isProfileCreated) {
        u32 type;

        p_wk->isProfileCreated = TRUE;
        if (p_wk->p_param->mode == BR_RECORD_MODE_MINE) {
            type = BR_PROFILE_TYPE_TRAINER;
        } else {
            type = BR_PROFILE_TYPE_UNION;
        }
        p_wk->profileWin = BrProfile_Init(p_wk->profile, p_param->res, p_param->unit, p_wk->que, type, p_wk->heapId);
    }
}

static void BrRecord_DeleteProfile(BrRecordWork *p_wk, BrRecordProcParam *p_param) {
    if (p_wk->isProfileCreated && p_wk->profileWin != NULL) {
        BrProfile_Exit(p_wk->profileWin);
        p_wk->profileWin = NULL;
        p_wk->isProfileCreated = FALSE;
    }
}

static void BrRecord_DeleteInfo(BrRecordWork *p_wk, BrRecordProcParam *p_param) {
    int i;

    if (p_wk->isInfoCreated) {
        for (i = 0; i < BATTLE_REC_HEADER_PKM_MAX; i++) {
            if (p_wk->icon[i] != NULL) {
                ClActor_SetVisible(p_wk->icon[i], FALSE);
                ClActor_Delete(p_wk->icon[i]);
                p_wk->icon[i] = NULL;
            }
            if (p_wk->iconChr[i] != -1) {
                ClActSys_FreeChars(p_wk->iconChr[i]);
                p_wk->iconChr[i] = -1;
            }
        }
        if (p_wk->iconPlt != -1) {
            ClActSys_FreePltt(p_wk->iconPlt);
            p_wk->iconPlt = -1;
        }
        if (p_wk->iconCell != -1) {
            ClActSys_FreeCellAnims(p_wk->iconCell);
            p_wk->iconCell = -1;
        }
        for (i = 0; i < MSGWIN_MAX; i++) {
            if (p_wk->msgWin[i] != NULL) {
                BrMsgWin_Exit(p_wk->msgWin[i]);
                p_wk->msgWin[i] = NULL;
            }
        }
        BrRes_UnloadBG(p_param->res, 6);
        GFL_BGSysLoadScr(1);
        p_wk->isInfoCreated = FALSE;
    }
}

// Makes the buttons and the title on the sub screen
static void BrRecord_CreateButtons(BrRecordWork *p_wk, BrRecordProcParam *p_param) {
    Font *font;
    MsgData *msg;

    if (p_wk->isButtonsCreated) {
        return;
    }
    p_wk->isButtonsCreated = TRUE;

    BrRes_LoadBG(p_param->res, 5, p_wk->heapId);
    BrRes_LoadOBJ(p_param->res, 6, p_wk->heapId);
    font = BrRes_GetFont(p_param->res);
    msg = BrRes_GetMsgData(p_param->res);

    if (p_wk->canSave) {
        ClActorSetup setup;
        BrResObjData obj;

        MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
        setup.x = 24;
        setup.y = 168;
        setup.sequence = 0;
        setup.priority = 1;
        BrRes_GetObjData(p_param->res, 6, &obj);
        p_wk->btn[BTN_RETURN] =
            BrBtn_InitEx(&setup, 4, 96, CLACT_SURFACE_SUB, p_param->unit, p_wk->bmpoam, font, msg, &obj, p_wk->heapId);
        setup.x = 136;
        setup.sequence = 3;
        p_wk->btn[BTN_SAVE] = BrBtn_InitEx(&setup, 0x54, 96, CLACT_SURFACE_SUB, p_param->unit, p_wk->bmpoam, font, msg,
                                           &obj, p_wk->heapId);
    } else {
        ClActorSetup setup;
        BrResObjData obj;

        MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
        setup.x = 80;
        setup.y = 168;
        setup.sequence = 0;
        setup.priority = 1;
        BrRes_GetObjData(p_param->res, 6, &obj);
        p_wk->btn[BTN_RETURN] =
            BrBtn_InitEx(&setup, 4, 96, CLACT_SURFACE_SUB, p_param->unit, p_wk->bmpoam, font, msg, &obj, p_wk->heapId);
    }

    p_wk->title[0] = BrMsgWin_Init(4, 8, 3, 16, 2, 14, p_wk->que, p_wk->heapId);
    if (p_wk->isProfile) {
        BrMsgWin_Print(p_wk->title[0], msg, 0x72, font, BR_RECORD_PRINT_COLOR);
    } else {
        BrMsgWin_Print(p_wk->title[0], msg, 0x71, font, BR_RECORD_PRINT_COLOR);
    }
}

static void BrRecord_DeleteButtons(BrRecordWork *p_wk, BrRecordProcParam *p_param) {
    int i;

    if (p_wk->isButtonsCreated) {
        if (p_wk->title[0] != NULL) {
            BrMsgWin_Exit(p_wk->title[0]);
            p_wk->title[0] = NULL;
        }
        for (i = 0; i < BTN_MAX; i++) {
            if (p_wk->btn[i] != NULL) {
                BrBtn_Exit(p_wk->btn[i]);
                p_wk->btn[i] = NULL;
            }
        }
        GFL_BGSysLoadScr(4);
        BrRes_UnloadOBJ(p_param->res, 6);
        BrRes_UnloadBG(p_param->res, 5);
        p_wk->isButtonsCreated = FALSE;
    }
}

// Makes the text box and starts the balls turning, for a download or a save
static void BrRecord_CreateText(BrRecordWork *p_wk, BrRecordProcParam *p_param) {
    if (!p_wk->isTextCreated) {
        BrPoint pos;

        p_wk->isTextCreated = TRUE;
        if (p_wk->text == NULL) {
            p_wk->text = BrText_Init(p_wk->p_param->res, p_wk->que, p_wk->heapId);
        }
        pos.x = 0x80;
        pos.y = 0x60;
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_ROT, &pos);
    }
}

static void BrRecord_DeleteText(BrRecordWork *p_wk, BrRecordProcParam *p_param) {
    if (p_wk->isTextCreated) {
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_NONE, NULL);
        if (p_wk->text != NULL) {
            BrText_Exit(p_wk->text, p_wk->p_param->res);
            p_wk->text = NULL;
        }
        p_wk->isTextCreated = FALSE;
    }
}

// Makes the question before an unwatched video plays, with its two answers
static void BrRecord_CreateConfirm(BrRecordWork *p_wk, BrRecordProcParam *p_param) {
    Font *font;
    MsgData *msg;
    int i;

    if (!p_wk->isConfirmCreated) {
        p_wk->isConfirmCreated = TRUE;
        if (p_wk->text == NULL) {
            p_wk->text = BrText_Init(p_param->res, p_wk->que, p_wk->heapId);
        }
        BrText_Print(p_wk->text, p_param->res, 0x14f);
        BrRes_LoadBG(p_param->res, 0x1e, p_wk->heapId);
        font = BrRes_GetFont(p_param->res);
        msg = BrRes_GetMsgData(p_param->res);
        for (i = 0; i < BR_RECORD_TITLE_MAX; i++) {
            const BrRecordMsgWinData *data = &sBrRecordConfirmWins[i];

            p_wk->title[i] = BrMsgWin_Init(4, data->x, data->y, data->w, data->h, 14, p_wk->que, p_wk->heapId);
            BrMsgWin_Print(p_wk->title[i], msg, data->msgID, font, BR_RECORD_PRINT_COLOR);
        }
    }
}

static void BrRecord_DeleteConfirm(BrRecordWork *p_wk, BrRecordProcParam *p_param) {
    int i;

    if (p_wk->isConfirmCreated) {
        for (i = 0; i < BR_RECORD_TITLE_MAX; i++) {
            if (p_wk->title[i] != NULL) {
                BrMsgWin_Exit(p_wk->title[i]);
                p_wk->title[i] = NULL;
            }
        }
        GFL_BGSysLoadScr(4);
        BrRes_UnloadBG(p_param->res, 0x1e);
        if (p_wk->text != NULL) {
            BrText_Exit(p_wk->text, p_param->res);
            p_wk->text = NULL;
        }
        p_wk->isConfirmCreated = FALSE;
    }
}

// Whether the video's battle mode has teams of three Pokémon, whose icons are laid out in groups of three
static BOOL BrRecord_IsSmallTeamMode(BattleRecHeader *header) {
    u32 mode = BattleRec_GetHeaderValue(header, BATTLE_REC_HEADER_MODE, 0);

    switch (mode) {
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x16:
        return TRUE;
    default:
        return FALSE;
    }
}

// Whether the video's battle mode has a counter to show, such as a Battle Subway streak
static BOOL BrRecord_IsCounterMode(BattleRecHeader *header) {
    BOOL ret = FALSE;
    u32 mode = BattleRec_GetHeaderValue(header, BATTLE_REC_HEADER_MODE, 0);

    switch (mode) {
    case 0x14:
    case 0x15:
    case 0x16:
        return TRUE;
    }
    return ret;
}

// The title, which swaps the summary and the profile
static BOOL BrRecord_GetTrgChange(BrRecordWork *p_wk, u32 x, u32 y) {
    BOOL ret = ((u32)(x - 64) <= (u32)(200 - 64)) & ((u32)(y - 16) <= (u32)(48 - 16));

    if (ret) {
        BrPoint pos;

        pos.x = x;
        pos.y = y;
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        GFL_SndSEPlay(SEQ_SE_BREC_06);
    }
    return ret;
}

// The play button
static BOOL BrRecord_GetTrgPlay(BrRecordWork *p_wk, u32 x, u32 y) {
    BOOL ret = ((u32)(x - 80) <= (u32)(176 - 80)) & ((u32)(y - 56) <= (u32)(120 - 56));

    if (ret) {
        BrPoint pos;

        pos.x = x;
        pos.y = y;
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        GFL_SndSEPlay(SEQ_SE_BREC_06);
    }
    return ret;
}

// The answers of the question before an unwatched video plays
static BOOL BrRecord_GetTrgYes(BrRecordWork *p_wk, u32 x, u32 y) {
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

static BOOL BrRecord_GetTrgNo(BrRecordWork *p_wk, u32 x, u32 y) {
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
