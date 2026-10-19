// The Battle Recorder's musical photo viewing screen: the player picks a Pokémon, downloads the musical photos with it
// from the Global Link, and looks through them, each with the profile of the player who sent it. The name is a guess:
// the ROM gives none, and br_core.c runs it as BR_PROCID_MUSICAL_LOOK

#include "types.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_graphic.h"
#include "app/battle_recorder/br_musical_look_proc.h"
#include "app/battle_recorder/br_net.h"
#include "app/battle_recorder/br_pokesearch.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_sidebar.h"
#include "app/battle_recorder/br_util.h"
#include "app/musical/mus_shot_photo.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "system/bmp_oam.h"
#include "system/game_data.h"
#include "system/printsys.h"

// The most photos downloaded at once
#define RECV_MAX 5

// The button graphics and the sub screen's BG, from br_res.c
#define BR_RES_OBJ_MUSICAL_LOOK_BTN 6
#define BR_RES_BG_MUSICAL_LOOK 0x16

typedef struct {
    BrPokeSearch *search;
    BrMsgWin *text;
    // The species picked in the search
    u16 species;
    u16 cnt;
    // The photo shown, in recv
    u16 idx;
    BrProfile *profile;
    MusShotPhoto *photo;
    BrBtn *btn;
    // The sub screen's text, which says whether the button shows the photo or the profile
    BrMsgWin *msgWin;
    // TRUE while the photo is shown, FALSE while its sender's profile is
    BOOL isPhoto;
    BrSeq *seq;
    BmpOamSys *bmpoam;
    PrintQueue *que;
    HeapID heapId;
    BrMusicalLookProcParam *param;
    // For the main and the sub screen
    BrBallEffect *ballEff[2];
    BrMusicalShotRecv *recv[RECV_MAX];
    int recvNum;
} BrMusicalLookWork;

static BOOL BrMusicalLook_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrMusicalLook_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrMusicalLook_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrMusicalLook_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalLook_SeqReturn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalLook_SeqSearch(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalLook_SeqTouch(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalLook_SeqDownload(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalLook_SeqShowPhoto(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalLook_SeqReturnSearch(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalLook_SeqDownloadEnd(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalLook_SeqStartDownload(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalLook_SeqChangePhoto(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalLook_SeqChangeView(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalLook_CreatePhoto(BrMusicalLookWork *wk, BrMusicalLookProcParam *param);
static void BrMusicalLook_DeletePhoto(BrMusicalLookWork *wk, BrMusicalLookProcParam *param);
static void BrMusicalLook_CreateButton(BrMusicalLookWork *wk, BrMusicalLookProcParam *param);
static void BrMusicalLook_DeleteButton(BrMusicalLookWork *wk, BrMusicalLookProcParam *param);
static void BrMusicalLook_CreateProfile(BrMusicalLookWork *wk, BrMusicalLookProcParam *param);
static void BrMusicalLook_DeleteProfile(BrMusicalLookWork *wk, BrMusicalLookProcParam *param);
static void BrMusicalLook_CreateSearch(BrMusicalLookWork *wk, BrMusicalLookProcParam *param);
static void BrMusicalLook_DeleteSearch(BrMusicalLookWork *wk, BrMusicalLookProcParam *param);
static void BrMusicalLook_CreateText(BrMusicalLookWork *wk, BrMusicalLookProcParam *param);
static void BrMusicalLook_DeleteText(BrMusicalLookWork *wk, BrMusicalLookProcParam *param);
static BOOL BrMusicalLook_GetTrgChange(BrMusicalLookWork *wk, u32 x, u32 y);
static BOOL BrMusicalLook_GetTrgLeft(BrMusicalLookWork *wk, u32 x, u32 y);
static BOOL BrMusicalLook_GetTrgRight(BrMusicalLookWork *wk, u32 x, u32 y);

const GameProcFunctions BR_MUSICAL_LOOK_PROC_FUNCTIONS = {
    BrMusicalLook_ProcInit,
    BrMusicalLook_ProcMain,
    BrMusicalLook_ProcExit,
};

static BOOL BrMusicalLook_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrMusicalLookProcParam *p = param;
    BrMusicalLookWork *wk;
    ClActUnit *unit;
    int i;

    GFL_OvlLoad(OVERLAY_ID(209));
    GFL_OvlLoad(OVERLAY_ID(210));

    wk = GFL_ProcInitSubsystem(proc, sizeof(BrMusicalLookWork), BrProcSys_GetHeapID(p->procSys));
    MI_CpuFill8(wk, 0, sizeof(BrMusicalLookWork));
    wk->param = p;
    wk->heapId = BrProcSys_GetHeapID(p->procSys);

    wk->que = PrintQueue_CreateDefault(wk->heapId);
    unit = BrGraphic_GetClunit(p->graphic);
    wk->bmpoam = BmpOam_Init(wk->heapId, unit);
    for (i = 0; i < 2; i++) {
        wk->ballEff[i] = BrBallEff_Init(unit, p->res, i, wk->heapId);
    }
    BrRes_LoadOBJ(p->res, BR_RES_OBJ_MUSICAL_LOOK_BTN, wk->heapId);
    GFL_BGSysSetBGEnabled(0, TRUE);

    BrMusicalLook_CreateSearch(wk, p);

    wk->seq = BrSeq_Init(wk, BrMusicalLook_SeqFadeIn, wk->heapId);
    return TRUE;
}

static BOOL BrMusicalLook_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrMusicalLookProcParam *p = param;
    BrMusicalLookWork *wk = work;
    int i;

    BrMusicalLook_DeletePhoto(wk, p);
    BrMusicalLook_DeleteButton(wk, p);
    BrMusicalLook_DeleteProfile(wk, p);
    BrMusicalLook_DeleteSearch(wk, p);
    BrMusicalLook_DeleteText(wk, p);
    BrSeq_Exit(wk->seq);
    BrMusicalLook_DeleteSearch(wk, p);
    BrRes_UnloadOBJ(p->res, BR_RES_OBJ_MUSICAL_LOOK_BTN);
    for (i = 0; i < 2; i++) {
        BrBallEff_Exit(wk->ballEff[i]);
    }
    BmpOam_Exit(wk->bmpoam);
    PrintQueue_Free(wk->que);

    GFL_ProcReleaseSubsystem(proc);
    GFL_OvlUnload(OVERLAY_ID(209));
    GFL_OvlUnload(OVERLAY_ID(210));
    return TRUE;
}

static BOOL BrMusicalLook_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BrMusicalLookWork *wk = work;
    int i;

    if (BrNet_CheckError(wk->param->net) == 2) {
        BrProcSys_Abort(wk->param->procSys);
        return TRUE;
    }

    BrSeq_Main(wk->seq);
    if (BrSeq_IsEnd(wk->seq)) {
        return TRUE;
    }

    for (i = 0; i < 2; i++) {
        BrBallEff_Main(wk->ballEff[i]);
    }
    PrintQueue_Main(wk->que);
    if (wk->msgWin != NULL) {
        BrMsgWin_Main(wk->msgWin);
    }
    if (wk->profile != NULL) {
        BrProfile_Main(wk->profile);
    }
    if (wk->text != NULL) {
        BrText_Main(wk->text);
    }
    if (wk->photo != NULL) {
        MusShotPhoto_Main(wk->photo);
    }
    return FALSE;
}

// Waits for the search's text, fades in, then lets the player search
static void BrMusicalLook_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_PRINT_WAIT,
        SEQ_FADEIN_WAIT,
        SEQ_NEXT,
    };
    BrMusicalLookWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_PRINT_WAIT:
        if (BrPokeSearch_PrintMain(wk->search)) {
            BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
            *p_seq = SEQ_FADEIN_WAIT;
        }
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_NEXT;
        }
        break;
    case SEQ_NEXT:
        BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqSearch);
        break;
    }
}

// Fades out and goes back to the menu
static void BrMusicalLook_SeqReturn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrMusicalLookWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_END;
        }
        break;
    case SEQ_END:
        BrSeq_End(wk->seq);
        BrProcSys_Pop(wk->param->procSys);
        break;
    }
}

// Runs the search until the player picks a Pokémon or goes back
static void BrMusicalLook_SeqSearch(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrMusicalLookWork *wk = p_wk_adrs;
    u16 species;
    u32 select;

    BrPokeSearch_Main(wk->search);
    select = BrPokeSearch_GetSelect(wk->search, &species);
    switch (select) {
    case BR_POKESEARCH_SELECT_NONE:
        break;
    case BR_POKESEARCH_SELECT_DECIDE:
        wk->species = species;
        BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqStartDownload);
        break;
    default:
        BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqReturn);
        break;
    }
}

// Waits for the player to touch the return button, the button that swaps the photo and the profile, or the arrows
static void BrMusicalLook_SeqTouch(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrMusicalLookWork *wk = p_wk_adrs;
    u32 x;
    u32 y;

    if (func_0203dac8(&x, &y)) {
        if (BrBtn_GetTrg(wk->btn, x, y)) {
            BrPoint pos;

            pos.x = x;
            pos.y = y;
            BrBallEff_Start(wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqReturnSearch);
        }
        if (BrMusicalLook_GetTrgChange(wk, x, y)) {
            BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqChangeView);
        }
        if (BrMusicalLook_GetTrgLeft(wk, x, y)) {
            if (wk->idx - 1 < 0) {
                wk->idx = wk->recvNum - 1;
            } else {
                wk->idx--;
            }
            BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqChangePhoto);
        }
        if (BrMusicalLook_GetTrgRight(wk, x, y)) {
            wk->idx++;
            wk->idx %= wk->recvNum;
            BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqChangePhoto);
        }
    }
}

// Downloads the photos with the Pokémon, then shows them or the result
static void BrMusicalLook_SeqDownload(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_DOWNLOAD_START,
        SEQ_DOWNLOAD_WAIT,
        SEQ_RESULT,
        SEQ_TOUCH_WAIT,
    };
    BrMusicalLookWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_DOWNLOAD_START: {
        BrNetRequestParam req;

        MI_CpuFill8(&req, 0, sizeof(BrNetRequestParam));
        req.species = wk->species;
        wk->cnt = 0;
        BrNet_StartRequest(wk->param->net, 1, &req);
        *p_seq = SEQ_DOWNLOAD_WAIT;
        break;
    }
    case SEQ_DOWNLOAD_WAIT:
        // The balls turn for at least 96 frames
        wk->cnt++;
        if (BrNet_IsRequestEnd(wk->param->net) && wk->cnt > 96) {
            BrBallEff_Start(wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_NONE, NULL);
            *p_seq = SEQ_RESULT;
        }
        break;
    case SEQ_RESULT: {
        u32 msgID;

        if (!BrNet_GetResultMsg(wk->param->net, &msgID)) {
            if (BrNet_GetDownloadMusicalShot(wk->param->net, wk->recv, RECV_MAX, &wk->recvNum) && wk->recvNum != 0) {
                wk->idx = 0;
                BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqShowPhoto);
            } else {
                // No photo has the Pokémon
                *p_seq = SEQ_TOUCH_WAIT;
                BrText_Print(wk->text, wk->param->res, 0x14d);
            }
        } else {
            *p_seq = SEQ_TOUCH_WAIT;
            BrText_Print(wk->text, wk->param->res, msgID);
        }
        break;
    }
    case SEQ_TOUCH_WAIT:
        if (func_0203da48()) {
            u32 x;
            u32 y;
            BrPoint pos;

            func_0203dac8(&x, &y);
            pos.x = x;
            pos.y = y;
            BrBallEff_Start(wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            GFL_SndSEPlay(SEQ_SE_BREC_06);
            BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqDownloadEnd);
        }
        break;
    }
}

// Shows the first photo and the button
static void BrMusicalLook_SeqShowPhoto(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_CHANGE_SCREEN,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_NEXT,
    };
    BrMusicalLookWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_MASTER_BRIGHT_AND_ALPHA, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_CHANGE_SCREEN;
        }
        break;
    case SEQ_CHANGE_SCREEN:
        BrMusicalLook_DeleteText(wk, wk->param);
        BrMusicalLook_CreatePhoto(wk, wk->param);
        BrMusicalLook_CreateButton(wk, wk->param);
        *p_seq = SEQ_FADEIN_START;
        break;
    case SEQ_FADEIN_START:
        BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_MASTER_BRIGHT_AND_ALPHA, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_NEXT;
        }
        break;
    case SEQ_NEXT:
        wk->isPhoto = TRUE;
        BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqTouch);
        break;
    }
}

// Leaves the photos for the search again
static void BrMusicalLook_SeqReturnSearch(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_CHANGE_SCREEN,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_NEXT,
    };
    BrMusicalLookWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        if (wk->isPhoto) {
            BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_MASTER_BRIGHT_AND_ALPHA, BR_FADE_DISPLAY_BOTH,
                             BR_FADE_DIR_OUT);
        } else {
            BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        }
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_CHANGE_SCREEN;
        }
        break;
    case SEQ_CHANGE_SCREEN:
        BrMusicalLook_DeleteProfile(wk, wk->param);
        BrMusicalLook_DeletePhoto(wk, wk->param);
        BrMusicalLook_DeleteButton(wk, wk->param);
        BrMusicalLook_CreateSearch(wk, wk->param);
        *p_seq = SEQ_FADEIN_START;
        break;
    case SEQ_FADEIN_START:
        if (wk->isPhoto) {
            BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_MASTER_BRIGHT_AND_ALPHA, BR_FADE_DISPLAY_BOTH,
                             BR_FADE_DIR_IN);
        } else {
            BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        }
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_NEXT;
        }
        break;
    case SEQ_NEXT:
        BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqSearch);
        break;
    }
}

// Goes back to the search after the download's result
static void BrMusicalLook_SeqDownloadEnd(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_CHANGE_SCREEN,
        SEQ_PRINT_WAIT,
        SEQ_FADEIN_WAIT,
        SEQ_NEXT,
    };
    BrMusicalLookWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_CHANGE_SCREEN;
        }
        break;
    case SEQ_CHANGE_SCREEN:
        BrMusicalLook_DeleteText(wk, wk->param);
        BrMusicalLook_CreateSearch(wk, wk->param);
        *p_seq = SEQ_PRINT_WAIT;
        break;
    case SEQ_PRINT_WAIT:
        if (BrPokeSearch_PrintMain(wk->search)) {
            BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
            *p_seq = SEQ_FADEIN_WAIT;
        }
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_NEXT;
        }
        break;
    case SEQ_NEXT:
        BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqSearch);
        break;
    }
}

// Leaves the search for the download's screen
static void BrMusicalLook_SeqStartDownload(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_CHANGE_SCREEN,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_NEXT,
    };
    BrMusicalLookWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_CHANGE_SCREEN;
        }
        break;
    case SEQ_CHANGE_SCREEN: {
        BrPoint pos;

        BrMusicalLook_DeleteSearch(wk, wk->param);
        BrMusicalLook_CreateText(wk, wk->param);
        pos.x = 0x80;
        pos.y = 0x60;
        BrBallEff_Start(wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_ROT, &pos);
        *p_seq = SEQ_FADEIN_START;
        break;
    }
    case SEQ_FADEIN_START:
        BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_NEXT;
        }
        break;
    case SEQ_NEXT:
        BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqDownload);
        break;
    }
}

// Shows the next or the previous photo, or its sender's profile
static void BrMusicalLook_SeqChangePhoto(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_CHANGE_SCREEN,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_NEXT,
    };
    BrMusicalLookWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        if (wk->isPhoto) {
            BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_MASTER_BRIGHT_WHITE, BR_FADE_DISPLAY_MAIN, BR_FADE_DIR_OUT);
        } else {
            BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_MAIN, BR_FADE_DIR_OUT);
        }
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_CHANGE_SCREEN;
        }
        break;
    case SEQ_CHANGE_SCREEN:
        if (wk->isPhoto) {
            BrMusicalLook_DeleteProfile(wk, wk->param);
            BrMusicalLook_DeletePhoto(wk, wk->param);
            BrMusicalLook_CreatePhoto(wk, wk->param);
        } else {
            BrMusicalLook_DeleteProfile(wk, wk->param);
            BrMusicalLook_DeletePhoto(wk, wk->param);
            BrMusicalLook_CreateProfile(wk, wk->param);
        }
        *p_seq = SEQ_FADEIN_START;
        break;
    case SEQ_FADEIN_START:
        if (wk->isPhoto) {
            BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_MASTER_BRIGHT_WHITE, BR_FADE_DISPLAY_MAIN, BR_FADE_DIR_IN);
        } else {
            BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_MAIN, BR_FADE_DIR_IN);
        }
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_NEXT;
        }
        break;
    case SEQ_NEXT:
        BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqTouch);
        break;
    }
}

// Swaps the photo and its sender's profile
static void BrMusicalLook_SeqChangeView(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_CHANGE_SCREEN,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_NEXT,
    };
    BrMusicalLookWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_MASTER_BRIGHT_WHITE, BR_FADE_DISPLAY_MAIN, BR_FADE_DIR_OUT);
        *p_seq = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_CHANGE_SCREEN;
        }
        break;
    case SEQ_CHANGE_SCREEN: {
        BOOL isPhoto = wk->photo != NULL;
        Font *font;
        MsgData *msg;
        u32 msgID;

        if (isPhoto) {
            BrMusicalLook_DeletePhoto(wk, wk->param);
            BrMusicalLook_CreateProfile(wk, wk->param);
        } else {
            BrMusicalLook_DeleteProfile(wk, wk->param);
            BrMusicalLook_CreatePhoto(wk, wk->param);
        }

        font = BrRes_GetFont(wk->param->res);
        msg = BrRes_GetMsgData(wk->param->res);
        if (isPhoto) {
            wk->isPhoto = FALSE;
            msgID = 0x72;
        } else {
            wk->isPhoto = TRUE;
            msgID = 0x71;
        }
        BrMsgWin_Print(wk->msgWin, msg, msgID, font, 0x3da0);
        *p_seq = SEQ_FADEIN_START;
        break;
    }
    case SEQ_FADEIN_START:
        BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_MASTER_BRIGHT_WHITE, BR_FADE_DISPLAY_MAIN, BR_FADE_DIR_IN);
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_NEXT;
        }
        break;
    case SEQ_NEXT:
        BrSeq_SetNext(p_seqwk, BrMusicalLook_SeqTouch);
        break;
    }
}

// Shows the photo on the main screen in place of its BG and sidebars
static void BrMusicalLook_CreatePhoto(BrMusicalLookWork *wk, BrMusicalLookProcParam *param) {
    if (wk->photo == NULL) {
        BrSidebar_SetVisible(param->sidebar, CLACT_SURFACE_MAIN, FALSE);
        BrRes_UnloadBG(param->res, 0);
        BrRes_UnloadCommonPltt(param->res, CLACT_VRAM_MAIN);
        BrGraphic_StartMain3D(param->graphic, wk->heapId);
        wk->photo = MusShotPhoto_Create(&wk->recv[wk->idx]->shot, wk->heapId);
    }
}

static void BrMusicalLook_DeletePhoto(BrMusicalLookWork *wk, BrMusicalLookProcParam *param) {
    if (wk->photo != NULL) {
        MusShotPhoto_Delete(wk->photo);
        wk->photo = NULL;
        BrGraphic_EndMain3D(param->graphic, wk->heapId);
        BrRes_LoadCommonPltt(param->res, CLACT_VRAM_MAIN, wk->heapId);
        BrRes_LoadBG(param->res, 0, wk->heapId);
        BrSidebar_SetVisible(param->sidebar, CLACT_SURFACE_MAIN, TRUE);
    }
}

// The return button and the text over the button that swaps the photo and the profile, on the sub screen
static void BrMusicalLook_CreateButton(BrMusicalLookWork *wk, BrMusicalLookProcParam *param) {
    ClActUnit *unit;
    Font *font;
    MsgData *msg;
    BOOL ret;
    ClActorSetup setup;
    BrResObjData obj;

    if (wk->msgWin == NULL) {
        font = BrRes_GetFont(param->res);
        msg = BrRes_GetMsgData(param->res);
        unit = BrGraphic_GetClunit(param->graphic);

        MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
        setup.x = 80;
        setup.y = 168;
        setup.sequence = 0;
        setup.priority = 1;
        unit = BrGraphic_GetClunit(param->graphic);
        ret = BrRes_GetObjData(param->res, BR_RES_OBJ_MUSICAL_LOOK_BTN, &obj);
        GFL_ASSERT(ret);
        wk->btn = BrBtn_InitEx(&setup, 4, 96, CLACT_SURFACE_SUB, unit, wk->bmpoam, font, msg, &obj, wk->heapId);

        wk->msgWin = BrMsgWin_Init(4, 8, 3, 16, 2, 14, wk->que, wk->heapId);
        BrMsgWin_Print(wk->msgWin, msg, 0x71, font, 0x3da0);
        BrRes_LoadBG(param->res, BR_RES_BG_MUSICAL_LOOK, wk->heapId);
    }
}

static void BrMusicalLook_DeleteButton(BrMusicalLookWork *wk, BrMusicalLookProcParam *param) {
    if (wk->msgWin != NULL) {
        BrRes_UnloadBG(param->res, BR_RES_BG_MUSICAL_LOOK);
        BrBtn_Exit(wk->btn);
        wk->btn = NULL;
        BrMsgWin_Exit(wk->msgWin);
        wk->msgWin = NULL;
        // The BG is unloaded a second time
        BrRes_UnloadBG(param->res, BR_RES_BG_MUSICAL_LOOK);
    }
}

// The profile of the photo's sender, on the main screen
static void BrMusicalLook_CreateProfile(BrMusicalLookWork *wk, BrMusicalLookProcParam *param) {
    Font *font;
    MsgData *msg;

    if (wk->profile == NULL) {
        font = BrRes_GetFont(param->res);
        msg = BrRes_GetMsgData(param->res);
        wk->profile = BrProfile_Init((GdsProfile *)wk->recv[wk->idx]->profile, param->res,
                                     BrGraphic_GetClunit(param->graphic), wk->que, BR_PROFILE_TYPE_UNION, wk->heapId);
        BrMsgWin_Print(wk->msgWin, msg, 0x72, font, 0x3da0);
    }
}

static void BrMusicalLook_DeleteProfile(BrMusicalLookWork *wk, BrMusicalLookProcParam *param) {
    if (wk->profile != NULL) {
        BrProfile_Exit(wk->profile);
        wk->profile = NULL;
    }
}

static void BrMusicalLook_CreateSearch(BrMusicalLookWork *wk, BrMusicalLookProcParam *param) {
    ClActUnit *unit;

    if (wk->search == NULL) {
        unit = BrGraphic_GetClunit(param->graphic);
        if (wk->text == NULL) {
            wk->text = BrText_Init(param->res, wk->que, wk->heapId);
        }
        BrText_Print(wk->text, param->res, 0x2c);
        wk->search = BrPokeSearch_Init(GameData_GetPokedex(param->gameData), param->res, unit, wk->bmpoam, param->fade,
                                       wk->ballEff[CLACT_SURFACE_MAIN], wk->ballEff[CLACT_SURFACE_SUB], wk->heapId);
        BrPokeSearch_StartUp(wk->search);
    }
}

static void BrMusicalLook_DeleteSearch(BrMusicalLookWork *wk, BrMusicalLookProcParam *param) {
    if (wk->search != NULL) {
        BrPokeSearch_CleanUp(wk->search);
        BrPokeSearch_Exit(wk->search);
        wk->search = NULL;
        GFL_BGSysLoadScr(4);
        if (wk->text != NULL) {
            BrText_Exit(wk->text, param->res);
            wk->text = NULL;
        }
    }
}

// The text that the photos are being downloaded
static void BrMusicalLook_CreateText(BrMusicalLookWork *wk, BrMusicalLookProcParam *param) {
    if (wk->text == NULL) {
        wk->text = BrText_Init(param->res, wk->que, wk->heapId);
    }
    BrText_Print(wk->text, param->res, 0x146);
}

static void BrMusicalLook_DeleteText(BrMusicalLookWork *wk, BrMusicalLookProcParam *param) {
    if (wk->text != NULL) {
        BrText_Exit(wk->text, param->res);
        wk->text = NULL;
    }
}

// The button over the photo that swaps it and the profile
static BOOL BrMusicalLook_GetTrgChange(BrMusicalLookWork *wk, u32 x, u32 y) {
    BOOL ret = ((u32)(x - 64) <= (u32)(184 - 64)) & ((u32)(y - 16) <= (u32)(48 - 16));

    if (ret) {
        BrPoint pos;

        pos.x = x;
        pos.y = y;
        BrBallEff_Start(wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        GFL_SndSEPlay(SEQ_SE_BREC_06);
    }
    return ret;
}

static BOOL BrMusicalLook_GetTrgLeft(BrMusicalLookWork *wk, u32 x, u32 y) {
    BOOL ret = ((u32)(x - 24) <= (u32)(64 - 24)) & ((u32)(y - 64) <= (u32)(104 - 64));

    if (ret) {
        BrPoint pos;

        pos.x = x;
        pos.y = y;
        BrBallEff_Start(wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        GFL_SndSEPlay(SEQ_SE_BREC_06);
    }
    return ret;
}

static BOOL BrMusicalLook_GetTrgRight(BrMusicalLookWork *wk, u32 x, u32 y) {
    BOOL ret = ((u32)(x - 184) <= (u32)(224 - 184)) & ((u32)(y - 64) <= (u32)(104 - 64));

    if (ret) {
        BrPoint pos;

        pos.x = x;
        pos.y = y;
        BrBallEff_Start(wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
        GFL_SndSEPlay(SEQ_SE_BREC_06);
    }
    return ret;
}
