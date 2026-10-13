// The Battle Recorder's battle video list screen: downloads a list of battle videos from the Global Link and shows
// their players' names in a list on the sub screen, with the Pokémon of the five videos in view on the main screen.
// The record screen shows the chosen video. The name is a guess: the ROM gives none, and br_core.c runs it as
// BR_PROCID_BV_RANK

#include "types.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_bvrank_proc.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_net.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "save/battle_rec.h"
#include "system/bmp_menuwork.h"
#include "system/bmp_oam.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/wordset.h"

#define BR_BVRANK_PRINT_COLOR 0x3da0

// The videos of the list that the main screen shows the Pokémon of, and the Pokémon of a video
#define BR_BVRANK_ICON_ROW_MAX 5
#define BR_BVRANK_ICON_COL_MAX 6

// The species past the last one that has an icon
#define BR_BVRANK_SPECIES_END 686

// The frames the download takes at least
#define BR_BVRANK_DOWNLOAD_WAIT 96

// The Pokémon of a video, without the empty slots
typedef struct {
    u32 species[BR_BVRANK_ICON_COL_MAX];
    u32 form[BR_BVRANK_ICON_COL_MAX];
    u32 gender[BR_BVRANK_ICON_COL_MAX];
} BrBvRankPokeData;

// The list of the videos and the icons of their Pokémon
typedef struct {
    ListMenuOption *p_list_data;
    BrList *p_list;
    WordSet *p_word;
    BrBallEffect *ballMain;
    BrBallEffect *ballSub;
    u16 plt;
    u16 cel;
    // The icons' graphics, a ring of rows that starts at chrTop
    u16 chr[BR_BVRANK_ICON_ROW_MAX][BR_BVRANK_ICON_COL_MAX];
    int chrTop;
    ClActor *icon[BR_BVRANK_ICON_ROW_MAX][BR_BVRANK_ICON_COL_MAX];
    BrOutlineData *p_outline;
} BrBvRankList;

typedef struct {
    BmpOamSys *bmpoam;
    BrBtn *btn;
    BrMsgWin *captionWin;
    BrMsgWin *titleWin;
    BrSeq *seq;
    PrintQueue *que;
    HeapID heapId;
    BrBallEffect *ballEff[2];
    BrMsgWin *text;
    BrBvRankList list;
    u32 cnt;
    BOOL isListCreated;
    BOOL isResLoaded;
    BrBvRankProcParam *p_param;
} BrBvRankWork;

static BOOL BrBvRank_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBvRank_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrBvRank_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrBvRank_SeqCreate(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvRank_SeqDelete(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvRank_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvRank_SeqFadeOut(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvRank_SeqMain(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvRank_SeqDownload(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvRank_SeqConnect(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvRank_SeqDownloadEnd(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBvRankList_Init(BrBvRankList *p_wk, BrOutlineData *p_outline, BrRes *res, ClActUnit *unit,
                              BrBallEffect *ballMain, BrBallEffect *ballSub, BrListPos *pos, HeapID heapId);
static void BrBvRankList_Exit(BrBvRankList *p_wk);
static void BrBvRankList_Main(BrBvRankList *p_wk, HeapID heapId);
static u32 BrBvRankList_GetSelect(const BrBvRankList *cp_wk);
static void BrBvRankList_CreateIcons(BrBvRankList *p_wk, ClActUnit *unit, HeapID heapId);
static void BrBvRankList_DeleteIcons(BrBvRankList *p_wk);
static void BrBvRankList_ScrollIcons(BrBvRankList *p_wk, u32 list, s8 slide, HeapID heapId);
static void BrBvRank_GetPokeData(BrOutline *outline, BrBvRankPokeData *poke);
static BOOL BrBvRank_GetTrgList(u32 x, u32 y);

const GameProcFunctions BR_BVRANK_PROC_FUNCTIONS = {
    BrBvRank_ProcInit,
    BrBvRank_ProcMain,
    BrBvRank_ProcExit,
};

// The list of the videos, whose items are bitmaps of three characters each
static const BrListParam sBrBvRankListParam = {
    NULL, 0, 2, 8, 10, 15, 14, 1, 3, BR_LIST_TYPE_CURSOR, NULL, NULL, NULL, NULL, NULL,
};

static BOOL BrBvRank_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvRankProcParam *p_param = param;
    BrBvRankWork *p_wk;
    int i;

    p_wk = GFL_ProcInitSubsystem(proc, sizeof(BrBvRankWork), BrProcSys_GetHeapID(p_param->procSys));
    sys_memset(p_wk, 0, sizeof(BrBvRankWork));
    p_wk->p_param = p_param;
    p_wk->heapId = BrProcSys_GetHeapID(p_param->procSys);

    p_wk->bmpoam = BmpOam_Init(p_wk->heapId, p_param->unit);
    p_wk->que = func_02021998(p_wk->heapId);
    for (i = 0; i < 2; i++) {
        p_wk->ballEff[i] = BrBallEff_Init(p_param->unit, p_param->res, i, p_wk->heapId);
    }

    p_wk->seq = BrSeq_Init(p_wk, BrBvRank_SeqConnect, p_wk->heapId);
    if (p_wk->p_param->isReturn) {
        // The list is still there after a video of it was shown
        BrSeq_SetNext(p_wk->seq, BrBvRank_SeqCreate);
    } else {
        BrSeq_SetNext(p_wk->seq, BrBvRank_SeqConnect);
    }
    return TRUE;
}

static BOOL BrBvRank_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvRankProcParam *p_param = param;
    BrBvRankWork *p_wk = work;
    int i;

    if (p_wk->isListCreated) {
        BrBvRankList_Exit(&p_wk->list);
        p_wk->isListCreated = FALSE;
    }
    if (p_wk->titleWin != NULL) {
        BrMsgWin_Exit(p_wk->titleWin);
        p_wk->titleWin = NULL;
    }
    if (p_wk->captionWin != NULL) {
        BrMsgWin_Exit(p_wk->captionWin);
        p_wk->captionWin = NULL;
    }
    if (p_wk->btn != NULL) {
        BrBtn_Exit(p_wk->btn);
        p_wk->btn = NULL;
    }
    if (p_wk->isResLoaded) {
        BrRes_UnloadOBJ(p_wk->p_param->res, 6);
        BrRes_UnloadBG(p_wk->p_param->res, 0x18);
        p_wk->isResLoaded = FALSE;
    }
    BrSeq_Exit(p_wk->seq);
    if (p_wk->text != NULL) {
        BrText_Exit(p_wk->text, p_param->res);
        p_wk->text = NULL;
    }
    for (i = 0; i < 2; i++) {
        BrBallEff_Exit(p_wk->ballEff[i]);
    }
    BmpOam_Exit(p_wk->bmpoam);
    func_02021a18(p_wk->que);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BrBvRank_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BrBvRankWork *p_wk = work;
    int i;

    if (BrNet_CheckError(p_wk->p_param->net) == 2) {
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
    if (p_wk->captionWin != NULL) {
        BrMsgWin_Main(p_wk->captionWin);
    }
    if (p_wk->titleWin != NULL) {
        BrMsgWin_Main(p_wk->titleWin);
    }
    if (p_wk->text != NULL) {
        BrText_Main(p_wk->text);
    }
    return FALSE;
}

// Makes the list, the return button and the windows
static void BrBvRank_SeqCreate(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBvRankWork *p_wk = p_wk_adrs;
    Font *font;
    MsgData *msg;
    ClActorSetup setup;
    BrResObjData obj;
    BOOL ret;

    font = BrRes_GetFont(p_wk->p_param->res);
    msg = BrRes_GetMsgData(p_wk->p_param->res);
    if (p_wk->text != NULL) {
        BrText_Exit(p_wk->text, p_wk->p_param->res);
        p_wk->text = NULL;
    }
    BrRes_LoadBG(p_wk->p_param->res, 0x18, p_wk->heapId);
    BrRes_LoadOBJ(p_wk->p_param->res, 6, p_wk->heapId);
    p_wk->isResLoaded = TRUE;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 80;
    setup.y = 168;
    setup.sequence = 0;
    setup.priority = 1;
    ret = BrRes_GetObjData(p_wk->p_param->res, 6, &obj);
    GFL_ASSERT(ret);
    p_wk->btn = BrBtn_InitEx(&setup, 4, 96, CLACT_SURFACE_SUB, p_wk->p_param->unit, p_wk->bmpoam, font, msg, &obj,
                             p_wk->heapId);

    p_wk->captionWin = BrMsgWin_Init(4, 11, 6, 10, 4, 14, p_wk->que, p_wk->heapId);
    BrMsgWin_Print(p_wk->captionWin, msg, 0x55, font, BR_BVRANK_PRINT_COLOR);

    p_wk->titleWin = BrMsgWin_Init(1, 3, 3, 26, 4, 14, p_wk->que, p_wk->heapId);
    BrMsgWin_SetPos(p_wk->titleWin, 0, 1, BR_MSGWIN_POS_ABSOLUTE);
    BrMsgWin_Print(p_wk->titleWin, msg, 0x57 + p_wk->p_param->mode, font, BR_BVRANK_PRINT_COLOR);

    BrBvRankList_Init(&p_wk->list, p_wk->p_param->p_outline, p_wk->p_param->res, p_wk->p_param->unit,
                      p_wk->ballEff[CLACT_SURFACE_MAIN], p_wk->ballEff[CLACT_SURFACE_SUB], p_wk->p_param->listPos,
                      p_wk->heapId);
    p_wk->isListCreated = TRUE;

    BrSeq_SetNext(p_seqwk, BrBvRank_SeqFadeIn);
}

static void BrBvRank_SeqDelete(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBvRankWork *p_wk = p_wk_adrs;

    BrBvRankList_Exit(&p_wk->list);
    p_wk->isListCreated = FALSE;

    BrMsgWin_Exit(p_wk->titleWin);
    p_wk->titleWin = NULL;
    BrMsgWin_Exit(p_wk->captionWin);
    p_wk->captionWin = NULL;
    GFL_BGSysLoadScr(4);
    GFL_BGSysLoadScr(1);

    BrBtn_Exit(p_wk->btn);
    p_wk->btn = NULL;
    BrRes_UnloadOBJ(p_wk->p_param->res, 6);
    BrRes_UnloadBG(p_wk->p_param->res, 0x18);
    p_wk->isResLoaded = FALSE;

    BrSeq_End(p_seqwk);
}

static void BrBvRank_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_FADEIN_END,
    };
    BrBvRankWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEIN_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_FADEIN_END;
        }
        break;
    case SEQ_FADEIN_END:
        BrSeq_SetNext(p_seqwk, BrBvRank_SeqMain);
        break;
    }
}

static void BrBvRank_SeqFadeOut(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_FADEOUT_END,
    };
    BrBvRankWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_FADEOUT_END;
        }
        break;
    case SEQ_FADEOUT_END:
        BrSeq_SetNext(p_seqwk, BrBvRank_SeqDelete);
        break;
    }
}

static void BrBvRank_SeqMain(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBvRankWork *p_wk = p_wk_adrs;
    u32 x;
    u32 y;

    BrBvRankList_Main(&p_wk->list, p_wk->heapId);

    if (func_0203dac8(&x, &y)) {
        if (BrBtn_GetTrg(p_wk->btn, x, y)) {
            BrPoint pos;

            pos.x = x;
            pos.y = y;
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            BrProcSys_Pop(p_wk->p_param->procSys);
            BrSeq_SetNext(p_seqwk, BrBvRank_SeqFadeOut);
        }
        if (BrBvRank_GetTrgList(x, y)) {
            u32 select = BrBvRankList_GetSelect(&p_wk->list);
            BrPoint pos;

            GFL_ASSERT(select < p_wk->p_param->p_outline->data_num);
            p_wk->p_param->p_outline->select = select;
            BrProcSys_Push(p_wk->p_param->procSys, BR_PROCID_RECORD);
            BrSeq_SetNext(p_seqwk, BrBvRank_SeqFadeOut);

            pos.x = x;
            pos.y = y;
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        }
    }
}

// Downloads the list. Without one, a message stays until the screen is touched and the screen ends
static void BrBvRank_SeqDownload(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_DOWNLOAD_START,
        SEQ_DOWNLOAD_WAIT,
        SEQ_DOWNLOAD_END,
        SEQ_MSG_WAIT,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrBvRankWork *p_wk = p_wk_adrs;
    BOOL ret;
    int i;

    switch (*p_seq) {
    case SEQ_DOWNLOAD_START: {
        BrNetRequestParam req;
        int type;

        sys_memset(&req, 0, sizeof(BrNetRequestParam));
        switch (p_wk->p_param->mode) {
        case BR_BVRANK_MODE_LIST1:
            type = BR_NET_REQUEST_5;
            break;
        case BR_BVRANK_MODE_LIST3:
            type = BR_NET_REQUEST_6;
            break;
        case BR_BVRANK_MODE_LIST2:
            type = BR_NET_REQUEST_7;
            break;
        case BR_BVRANK_MODE_SEARCH:
            type = BR_NET_REQUEST_4;
            req.search = p_wk->p_param->search;
            break;
        default:
            GFL_ASSERT(0);
            break;
        }
        sys_memset(p_wk->p_param->p_outline, 0, sizeof(BrOutlineData));
        p_wk->cnt = 0;
        BrNet_StartRequest(p_wk->p_param->net, type, &req);
        *p_seq = SEQ_DOWNLOAD_WAIT;
        break;
    }
    case SEQ_DOWNLOAD_WAIT:
        p_wk->cnt++;
        if (BrNet_IsRequestEnd(p_wk->p_param->net) && p_wk->cnt > BR_BVRANK_DOWNLOAD_WAIT) {
            BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_NONE, NULL);
            *p_seq = SEQ_DOWNLOAD_END;
        }
        break;
    case SEQ_DOWNLOAD_END: {
        BrOutlineData *p_outline = p_wk->p_param->p_outline;
        int *p_num = &p_outline->data_num;
        u32 msgID;

        if (!BrNet_GetResultMsg(p_wk->p_param->net, &msgID)) {
            switch (p_wk->p_param->mode) {
            case BR_BVRANK_MODE_LIST1:
                ret = BrNet_GetDownloadVideoListB(p_wk->p_param->net, p_outline, BR_OUTLINE_RECV_MAX, p_num);
                break;
            case BR_BVRANK_MODE_LIST3:
                // The same call as for the first list
                ret = BrNet_GetDownloadVideoListB(p_wk->p_param->net, p_outline, BR_OUTLINE_RECV_MAX, p_num);
                break;
            case BR_BVRANK_MODE_LIST2:
                ret = BrNet_GetDownloadVideoListC(p_wk->p_param->net, p_outline, BR_OUTLINE_RECV_MAX, p_num);
                break;
            case BR_BVRANK_MODE_SEARCH:
                ret = BrNet_GetDownloadVideoListA(p_wk->p_param->net, p_outline, BR_OUTLINE_RECV_MAX, p_num);
                break;
            default:
                GFL_ASSERT(0);
                break;
            }
            if (ret) {
                for (i = 0; i < p_wk->p_param->p_outline->data_num; i++) {
                    p_wk->p_param->p_outline->isSecure[i] =
                        func_0200c124(&p_wk->p_param->p_outline->data[i].header, BATTLE_REC_HEADER_UNK2A, 0);
                }
                BrSeq_SetNext(p_seqwk, BrBvRank_SeqDownloadEnd);
            } else {
                // No videos
                if (p_wk->p_param->mode == BR_BVRANK_MODE_SEARCH) {
                    BrText_Print(p_wk->text, p_wk->p_param->res, 0x114);
                } else {
                    BrText_Print(p_wk->text, p_wk->p_param->res, 0x153);
                }
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
            BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
            *p_seq = SEQ_FADEOUT_WAIT;
        }
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

// Shows the message of the download
static void BrBvRank_SeqConnect(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_INIT,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_END,
    };
    BrBvRankWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_INIT: {
        BrPoint pos;

        if (p_wk->text == NULL) {
            p_wk->text = BrText_Init(p_wk->p_param->res, p_wk->que, p_wk->heapId);
        }
        if (p_wk->p_param->mode == BR_BVRANK_MODE_SEARCH) {
            BrText_Print(p_wk->text, p_wk->p_param->res, 0x14b);
        } else {
            BrText_Print(p_wk->text, p_wk->p_param->res, 0x13e);
        }
        pos.x = 128;
        pos.y = 96;
        BrBallEff_Start(p_wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_ROT, &pos);
        *p_seq = SEQ_FADEIN_START;
        break;
    }
    case SEQ_FADEIN_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_END;
        }
        break;
    case SEQ_END:
        BrSeq_SetNext(p_seqwk, BrBvRank_SeqDownload);
        break;
    }
}

// Fades the message of the download out to show the list
static void BrBvRank_SeqDownloadEnd(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_FADEOUT_END,
    };
    BrBvRankWork *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p_wk->p_param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p_wk->p_param->fade)) {
            *p_seq = SEQ_FADEOUT_END;
        }
        break;
    case SEQ_FADEOUT_END:
        BrSeq_SetNext(p_seqwk, BrBvRank_SeqCreate);
        break;
    }
}

// Makes the list, an item for each video with its place and its player's name, and the icons of the first videos
static void BrBvRankList_Init(BrBvRankList *p_wk, BrOutlineData *p_outline, BrRes *res, ClActUnit *unit,
                              BrBallEffect *ballMain, BrBallEffect *ballSub, BrListPos *pos, HeapID heapId) {
    Font *font;
    MsgData *msg;
    int i;

    font = BrRes_GetFont(res);
    msg = BrRes_GetMsgData(res);

    sys_memset(p_wk, 0, sizeof(BrBvRankList));
    p_wk->p_outline = p_outline;
    p_wk->p_word = GFL_WordSetSystemCreateDefault(heapId);
    p_wk->ballMain = ballMain;
    p_wk->ballSub = ballSub;

    p_wk->p_list_data = ListMenuCore_CreateOptionList(p_outline->data_num, heapId);
    for (i = 0; i < p_outline->data_num; i++) {
        p_wk->p_list_data[i].value = i;
    }

    {
        BrListParam param = sBrBvRankListParam;
        StrBuf *rank;
        StrBuf *src;
        StrBuf *name;

        param.cp_list = (const BrListItem *)p_wk->p_list_data;
        param.list_max = p_outline->data_num;
        param.res = res;
        param.unit = unit;
        param.ballCursor = p_wk->ballMain;
        param.ballDecide = p_wk->ballSub;
        param.pos = pos;
        p_wk->p_list = BrList_Init(&param, heapId);

        rank = GFL_StrBufCreate(0xff, heapId);
        src = GFL_MsgDataLoadStrbufNew(msg, 0x7a);
        name = GFL_StrBufCreate(0xff, heapId);
        for (i = 0; i < p_outline->data_num; i++) {
            GFLBitmap *bmp = GFL_BitmapCreate(param.w, param.str_line, 0x20, heapId);

            GFL_BitmapFill(bmp, 0);
            WordSetNumber(p_wk->p_word, 0, i + 1, 2, 0, TRUE);
            GFL_WordSetFormatStrbuf(p_wk->p_word, rank, src);
            GFL_StrBufLoadString(name, p_outline->data[i].profile.name);
            GFL_TextRendererDrawToBitmapEx(bmp, 16 - GFL_FontGetBlockWidth(rank, font, 0), 4, rank, font,
                                           BR_BVRANK_PRINT_COLOR);
            GFL_TextRendererDrawToBitmapEx(bmp, 20, 4, name, font, BR_BVRANK_PRINT_COLOR);
            BrList_SetBmp(p_wk->p_list, (u16)i, bmp);
        }
        GFL_StrBufFree(name);
        GFL_StrBufFree(rank);
        GFL_StrBufFree(src);
        BrList_Write(p_wk->p_list);
    }

    BrBvRankList_CreateIcons(p_wk, unit, heapId);
    // Scroll the icons to where the list was
    for (i = 0; i < pos->list; i++) {
        BrBvRankList_ScrollIcons(p_wk, i + 1, 1, heapId);
    }
}

static void BrBvRankList_Exit(BrBvRankList *p_wk) {
    int i;

    BrBvRankList_DeleteIcons(p_wk);
    for (i = 0; i < BrList_GetParam(p_wk->p_list, BR_LIST_PARAM_LISTMAX); i++) {
        GFL_BitmapFree(BrList_GetBmp(p_wk->p_list, (u16)i));
    }
    GFL_WordSetSystemFree(p_wk->p_word);
    BrList_Exit(p_wk->p_list);
    ListMenuCore_FreeOptionList(p_wk->p_list_data);
    sys_memset(p_wk, 0, sizeof(BrBvRankList));
}

// Runs the list, and scrolls the icons when it has scrolled
static void BrBvRankList_Main(BrBvRankList *p_wk, HeapID heapId) {
    u32 pre_list;
    u32 now_list;
    s8 slide;

    pre_list = BrList_GetParam(p_wk->p_list, BR_LIST_PARAM_LIST);
    BrList_Main(p_wk->p_list);
    now_list = BrList_GetParam(p_wk->p_list, BR_LIST_PARAM_LIST);
    slide = BrList_GetParam(p_wk->p_list, BR_LIST_PARAM_SLIDE);
    if (slide != 0 && pre_list != now_list) {
        BrBvRankList_ScrollIcons(p_wk, now_list, slide, heapId);
    }
}

static u32 BrBvRankList_GetSelect(const BrBvRankList *cp_wk) {
    return BrList_GetSelect(cp_wk->p_list);
}

// Makes the icons of the Pokémon of the first videos
static void BrBvRankList_CreateIcons(BrBvRankList *p_wk, ClActUnit *unit, HeapID heapId) {
    int i;
    ArcTool *handle;
    ClActorSetup setup;
    int j;
    int num;

    if (p_wk->p_outline->data_num < BR_BVRANK_ICON_ROW_MAX) {
        num = p_wk->p_outline->data_num;
    } else {
        num = BR_BVRANK_ICON_ROW_MAX;
    }

    handle = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_TAIL(heapId));
    p_wk->plt = func_0204bc48(handle, PokeIcon_GetPaletteFile(), CLACT_VRAM_MAIN, 0x80, heapId);
    p_wk->cel = func_0204bde0(handle, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), heapId);

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.sequence = 1;
    setup.priority = 0;
    setup.bgPriority = 0;

    for (i = 0; i < num; i++) {
        BrBvRankPokeData poke;

        BrBvRank_GetPokeData(&p_wk->p_outline->data[i], &poke);
        for (j = 0; j < BR_BVRANK_ICON_COL_MAX; j++) {
            u32 species = poke.species[j];

            // An empty slot gets a hidden icon
            if (species == 0) {
                species = 1;
            }
            p_wk->chr[i][j] =
                func_0204b81c(handle, PokeParty_GetIconIndex(species, poke.form[j], poke.gender[j], FALSE), FALSE,
                              CLACT_VRAM_MAIN, heapId);
            setup.x = j * 24 + 110;
            setup.y = i * 24 + 72;
            setup.priority = BR_BVRANK_ICON_COL_MAX - j;
            p_wk->icon[i][j] =
                func_0204c040(unit, p_wk->chr[i][j], p_wk->plt, p_wk->cel, &setup, CLACT_SURFACE_MAIN, heapId);
            func_0204c378(p_wk->icon[i][j], PokeIcon_GetPalette(species, poke.form[j], poke.gender[j], FALSE), 0);
            func_0204c318(p_wk->icon[i][j], 1);
            if (poke.species[j] == 0) {
                func_0204c124(p_wk->icon[i][j], FALSE);
            } else {
                func_0204c124(p_wk->icon[i][j], TRUE);
            }
        }
    }

    GFL_ArcToolFree(handle);
}

static void BrBvRankList_DeleteIcons(BrBvRankList *p_wk) {
    int i;
    int j;

    func_0204be64(p_wk->cel);
    func_0204bcd0(p_wk->plt);
    for (i = 0; i < BR_BVRANK_ICON_ROW_MAX; i++) {
        for (j = 0; j < BR_BVRANK_ICON_COL_MAX; j++) {
            if (p_wk->icon[i][j] != NULL) {
                func_0204b98c(p_wk->chr[i][j]);
                func_0204c108(p_wk->icon[i][j]);
                p_wk->icon[i][j] = NULL;
            }
        }
    }
}

// Scrolls the icons by a video, down if slide is 1 and up if -1, to show the five videos from list on: the icons
// take the graphics of the row after them, and the row that comes into view gets the graphics that went out of view
static void BrBvRankList_ScrollIcons(BrBvRankList *p_wk, u32 list, s8 slide, HeapID heapId) {
    ArcTool *handle;
    int row;
    int col;

    for (row = 0; row < BR_BVRANK_ICON_ROW_MAX; row++) {
        for (col = 0; col < BR_BVRANK_ICON_COL_MAX; col++) {
            if (slide == 1 && row != BR_BVRANK_ICON_ROW_MAX - 1) {
                ClActor *src = p_wk->icon[row + 1][col];
                ClActor *dst = p_wk->icon[row][col];

                if (func_0204c138(src)) {
                    NNSG2dImageProxy proxy;
                    u8 plt;

                    func_0204c40c(src, &proxy);
                    plt = func_0204c39c(src);
                    func_0204c3e4(dst, &proxy);
                    func_0204c378(dst, plt, 0);
                    func_0204c124(dst, TRUE);
                } else {
                    func_0204c124(dst, FALSE);
                }
            } else if (slide == -1 && row != BR_BVRANK_ICON_ROW_MAX - 1) {
                ClActor *src = p_wk->icon[BR_BVRANK_ICON_ROW_MAX - 2 - row][col];
                ClActor *dst = p_wk->icon[BR_BVRANK_ICON_ROW_MAX - 1 - row][col];

                if (func_0204c138(src)) {
                    NNSG2dImageProxy proxy;
                    u8 plt;

                    func_0204c40c(src, &proxy);
                    plt = func_0204c39c(src);
                    func_0204c3e4(dst, &proxy);
                    func_0204c378(dst, plt, 0);
                    func_0204c124(dst, TRUE);
                } else {
                    func_0204c124(dst, FALSE);
                }
            }
        }
    }

    handle = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_TAIL(heapId));

    if (slide == -1) {
        p_wk->chrTop--;
        if (p_wk->chrTop < 0) {
            p_wk->chrTop = BR_BVRANK_ICON_ROW_MAX - 1;
        }
    }

    for (col = 0; col < BR_BVRANK_ICON_COL_MAX; col++) {
        BrBvRankPokeData poke;

        if (slide == 1) {
            row = BR_BVRANK_ICON_ROW_MAX - 1;
        } else if (slide == -1) {
            row = 0;
        }
        BrBvRank_GetPokeData(&p_wk->p_outline->data[row + list], &poke);
        if (poke.species[col] != 0) {
            NNSG2dCharacterData *chr;
            NNSG2dImageProxy proxy;
            void *buf;

            buf = GFL_G2DIOReadOBJNCGRArc(
                handle, PokeParty_GetIconIndex(poke.species[col], poke.form[col], poke.gender[col], FALSE), FALSE, &chr,
                HEAPID_TAIL(heapId));
            func_0204ba40(p_wk->chr[p_wk->chrTop][col], chr);
            func_0204bb58(p_wk->chr[p_wk->chrTop][col], &proxy);
            func_0204c3e4(p_wk->icon[row][col], &proxy);
            func_0204c378(p_wk->icon[row][col],
                          PokeIcon_GetPalette(poke.species[col], poke.form[col], poke.gender[col], FALSE), 0);
            GFL_HeapFree(buf);
            func_0204c124(p_wk->icon[row][col], TRUE);
        } else {
            func_0204c124(p_wk->icon[row][col], FALSE);
        }
    }

    if (slide == 1) {
        p_wk->chrTop++;
        if (p_wk->chrTop > BR_BVRANK_ICON_ROW_MAX - 1) {
            p_wk->chrTop = 0;
        }
    }

    GFL_ArcToolFree(handle);
}

// The Pokémon of a video. A battle mode with teams of three has them in the header's first and third groups of three
static void BrBvRank_GetPokeData(BrOutline *outline, BrBvRankPokeData *poke) {
    BattleRecHeader *header;
    u32 form;
    BOOL isSmallTeam;
    u32 max;
    u32 mode;
    int idx = 0;
    int i;

    sys_memset(poke, 0, sizeof(BrBvRankPokeData));
    header = &outline->header;
    mode = func_0200c124(header, BATTLE_REC_HEADER_MODE, 0);
    switch (mode) {
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x16:
        isSmallTeam = TRUE;
        max = 9;
        break;
    default:
        isSmallTeam = FALSE;
        max = 6;
        break;
    }

    for (i = 0; i < max; i++) {
        int species;
        u32 gender;

        if (isSmallTeam && 3 <= i && i <= 5) {
            continue;
        }
        species = func_0200c124(header, BATTLE_REC_HEADER_SPECIES, i);
        form = func_0200c124(header, BATTLE_REC_HEADER_FORM, i);
        gender = func_0200c124(header, BATTLE_REC_HEADER_GENDER, i);
        if (species >= BR_BVRANK_SPECIES_END) {
            species = 0;
        }
        if (species != 0) {
            poke->species[idx] = species;
            poke->form[idx] = form;
            poke->gender[idx] = gender;
            idx++;
        }
    }
}

// Whether the touch is on the list's cursor, which chooses the video under it
static BOOL BrBvRank_GetTrgList(u32 x, u32 y) {
    BOOL ret = ((u32)(x - 80) <= 88) & ((u32)(y - 32) <= 56);

    if (ret) {
        GFL_SndSEPlay(SEQ_SE_BREC_06);
    }
    return ret;
}
