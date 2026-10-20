// The Battle Recorder's musical photo sending screen: it shows the photo of the player's last musical with the buttons
// to go back or to send it to the Global Link, and sends it. The name is a guess: the ROM gives none, and br_core.c
// runs it as BR_PROCID_MUSICAL_SEND

#include "types.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_graphic.h"
#include "app/battle_recorder/br_musical_send_proc.h"
#include "app/battle_recorder/br_net.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_sidebar.h"
#include "app/battle_recorder/br_util.h"
#include "app/musical/mus_shot_photo.h"
#include "constants/sound.h"
#include "gfl/clact.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "save/musical_savedata.h"
#include "save/save_control.h"
#include "system/bmp_oam.h"
#include "system/game_data.h"
#include "system/printsys.h"

// The buttons on the sub screen
enum {
    BTN_RETURN,
    BTN_SEND,
    BTN_MAX,
};

// The button graphics, from br_res.c
#define BR_RES_OBJ_MUSICAL_SEND_BTN 6
#define BR_RES_BG_MUSICAL_SEND 0x17

typedef struct {
    MusShotPhoto *photo;
    BrBtn *btn[BTN_MAX];
    BrSeq *seq;
    BrMsgWin *text;
    // For the main and the sub screen
    BrBallEffect *ballEff[2];
    BmpOamSys *bmpoam;
    PrintQueue *que;
    HeapID heapId;
    MusicalShot *shot;
    u32 cnt;
    BrMusicalSendProcParam *param;
} BrMusicalSendWork;

static BOOL BrMusicalSend_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrMusicalSend_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrMusicalSend_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static void BrMusicalSend_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalSend_SeqReturn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalSend_SeqTouch(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalSend_SeqSend(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrMusicalSend_CreatePhoto(BrMusicalSendWork *wk, BrMusicalSendProcParam *param);
static void BrMusicalSend_DeletePhoto(BrMusicalSendWork *wk, BrMusicalSendProcParam *param);
static void BrMusicalSend_CreateButtons(BrMusicalSendWork *wk, BrMusicalSendProcParam *param);
static void BrMusicalSend_DeleteButtons(BrMusicalSendWork *wk, BrMusicalSendProcParam *param);

const GameProcFunctions BR_MUSICAL_SEND_PROC_FUNCTIONS = {
    BrMusicalSend_ProcInit,
    BrMusicalSend_ProcMain,
    BrMusicalSend_ProcExit,
};

static BOOL BrMusicalSend_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrMusicalSendProcParam *p = param;
    BrMusicalSendWork *wk;
    ClActUnit *unit;
    int i;

    GFL_OvlLoad(OVERLAY_ID(209));
    GFL_OvlLoad(OVERLAY_ID(210));

    wk = GFL_ProcInitSubsystem(proc, sizeof(BrMusicalSendWork), BrProcSys_GetHeapID(p->procSys));
    MI_CpuFill8(wk, 0, sizeof(BrMusicalSendWork));
    wk->heapId = BrProcSys_GetHeapID(p->procSys);
    wk->param = p;

    wk->que = PrintQueue_CreateDefault(wk->heapId);
    unit = BrGraphic_GetClunit(p->graphic);
    wk->bmpoam = BmpOam_Init(wk->heapId, unit);
    for (i = 0; i < 2; i++) {
        wk->ballEff[i] = BrBallEff_Init(unit, p->res, i, wk->heapId);
    }
    wk->shot = MusicalSave_GetShot(getAddressOfMusicalDataInfo(GameData_GetSaveControl(p->gameData)));

    BrMusicalSend_CreatePhoto(wk, p);
    BrMusicalSend_CreateButtons(wk, p);

    wk->seq = BrSeq_Init(wk, BrMusicalSend_SeqFadeIn, wk->heapId);
    return TRUE;
}

static BOOL BrMusicalSend_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrMusicalSendProcParam *p = param;
    BrMusicalSendWork *wk = work;
    int i;

    BrSeq_Exit(wk->seq);
    BrMusicalSend_DeleteButtons(wk, p);
    BrMusicalSend_DeletePhoto(wk, p);
    for (i = 0; i < 2; i++) {
        BrBallEff_Exit(wk->ballEff[i]);
    }
    if (wk->text != NULL) {
        BrText_Exit(wk->text, p->res);
        wk->text = NULL;
    }
    BmpOam_Exit(wk->bmpoam);
    PrintQueue_Free(wk->que);

    GFL_ProcReleaseSubsystem(proc);
    GFL_OvlUnload(OVERLAY_ID(209));
    GFL_OvlUnload(OVERLAY_ID(210));
    return TRUE;
}

static BOOL BrMusicalSend_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BrMusicalSendWork *wk = work;
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
    if (wk->text != NULL) {
        BrText_Main(wk->text);
    }
    if (wk->photo != NULL) {
        MusShotPhoto_Main(wk->photo);
    }
    return FALSE;
}

// Fades in, then waits for a button
static void BrMusicalSend_SeqFadeIn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_NEXT,
    };
    BrMusicalSendWork *wk = p_wk_adrs;

    switch (*p_seq) {
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
        BrSeq_SetNext(p_seqwk, BrMusicalSend_SeqTouch);
        break;
    }
}

// Fades out and goes back to the menu
static void BrMusicalSend_SeqReturn(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrMusicalSendWork *wk = p_wk_adrs;

    switch (*p_seq) {
    case SEQ_FADEOUT_START:
        BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_MASTER_BRIGHT_AND_ALPHA, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
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

// Waits for the player to touch a button
static void BrMusicalSend_SeqTouch(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrMusicalSendWork *wk = p_wk_adrs;
    u32 x;
    u32 y;

    if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
        if (BrBtn_GetTrg(wk->btn[BTN_RETURN], x, y)) {
            BrPoint pos;

            pos.x = x;
            pos.y = y;
            BrBallEff_Start(wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            wk->param->isSend = FALSE;
            BrSeq_SetNext(p_seqwk, BrMusicalSend_SeqReturn);
        }
        if (BrBtn_GetTrg(wk->btn[BTN_SEND], x, y)) {
            BrPoint pos;

            pos.x = x;
            pos.y = y;
            BrBallEff_Start(wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            wk->param->isSend = TRUE;
            BrSeq_SetNext(p_seqwk, BrMusicalSend_SeqSend);
        }
    }
}

// Sends the photo, shows the result and goes back to the menu
static void BrMusicalSend_SeqSend(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    enum {
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_CHANGE_SCREEN,
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_SEND_START,
        SEQ_SEND_WAIT,
        SEQ_RESULT,
        SEQ_TOUCH_WAIT,
        SEQ_END_FADEOUT_START,
        SEQ_END_FADEOUT_WAIT,
        SEQ_END,
    };
    BrMusicalSendWork *wk = p_wk_adrs;

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
    case SEQ_CHANGE_SCREEN: {
        BrPoint pos;

        BrMusicalSend_DeleteButtons(wk, wk->param);
        BrMusicalSend_DeletePhoto(wk, wk->param);
        wk->text = BrText_Init(wk->param->res, wk->que, wk->heapId);
        BrText_Print(wk->text, wk->param->res, 0x145);
        pos.x = 0x80;
        pos.y = 0x60;
        BrBallEff_Start(wk->ballEff[CLACT_SURFACE_MAIN], BR_BALL_EFFECT_ROT, &pos);
        *p_seq = SEQ_FADEIN_START;
        break;
    }
    case SEQ_FADEIN_START:
        BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_MASTER_BRIGHT_AND_ALPHA, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *p_seq = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_SEND_START;
        }
        break;
    case SEQ_SEND_START: {
        BrNetRequestParam req;

        MI_CpuFill8(&req, 0, sizeof(BrNetRequestParam));
        req.data = wk->shot;
        BrNet_StartRequest(wk->param->net, 0, &req);
        wk->cnt = 0;
        *p_seq = SEQ_SEND_WAIT;
        break;
    }
    case SEQ_SEND_WAIT:
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
            *p_seq = SEQ_END_FADEOUT_START;
        } else {
            BrText_Print(wk->text, wk->param->res, msgID);
            *p_seq = SEQ_TOUCH_WAIT;
        }
        break;
    }
    case SEQ_TOUCH_WAIT:
        if (GFL_HIDIsTouchPressed()) {
            u32 x;
            u32 y;
            BrPoint pos;

            GFL_HIDGetTouchPressedPoint(&x, &y);
            pos.x = x;
            pos.y = y;
            BrBallEff_Start(wk->ballEff[CLACT_SURFACE_SUB], BR_BALL_EFFECT_SPREAD, &pos);
            GFL_SndSEPlay(SEQ_SE_BREC_06);
            *p_seq = SEQ_END_FADEOUT_START;
        }
        break;
    case SEQ_END_FADEOUT_START:
        BrFade_StartFade(wk->param->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *p_seq = SEQ_END_FADEOUT_WAIT;
        break;
    case SEQ_END_FADEOUT_WAIT:
        if (BrFade_IsEnd(wk->param->fade)) {
            *p_seq = SEQ_END;
        }
        break;
    case SEQ_END:
        BrSeq_End(p_seqwk);
        BrProcSys_Pop(wk->param->procSys);
        break;
    }
}

// Shows the photo on the main screen in place of its BG and sidebars
static void BrMusicalSend_CreatePhoto(BrMusicalSendWork *wk, BrMusicalSendProcParam *param) {
    if (wk->photo == NULL) {
        BrSidebar_SetVisible(param->sidebar, CLACT_SURFACE_MAIN, FALSE);
        BrRes_UnloadBG(param->res, 0);
        BrRes_UnloadCommonPltt(param->res, CLACT_VRAM_MAIN);
        BrGraphic_StartMain3D(param->graphic, wk->heapId);
        wk->photo = MusShotPhoto_Create(wk->shot, wk->heapId);
    }
}

static void BrMusicalSend_DeletePhoto(BrMusicalSendWork *wk, BrMusicalSendProcParam *param) {
    if (wk->photo != NULL) {
        MusShotPhoto_Delete(wk->photo);
        wk->photo = NULL;
        BrGraphic_EndMain3D(param->graphic, wk->heapId);
        BrRes_LoadCommonPltt(param->res, CLACT_VRAM_MAIN, wk->heapId);
        BrRes_LoadBG(param->res, 0, wk->heapId);
        BrSidebar_SetVisible(param->sidebar, CLACT_SURFACE_MAIN, TRUE);
    }
}

static void BrMusicalSend_CreateButtons(BrMusicalSendWork *wk, BrMusicalSendProcParam *param) {
    ClActUnit *unit;
    Font *font;
    MsgData *msg;
    int i;
    u32 msgID;
    ClActorSetup setup;
    BrResObjData obj;

    if (wk->btn[0] == NULL) {
        BrRes_LoadBG(param->res, BR_RES_BG_MUSICAL_SEND, wk->heapId);
        BrRes_LoadOBJ(param->res, BR_RES_OBJ_MUSICAL_SEND_BTN, wk->heapId);
        font = BrRes_GetFont(param->res);
        msg = BrRes_GetMsgData(param->res);

        MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
        setup.y = 168;
        setup.priority = 1;
        unit = BrGraphic_GetClunit(param->graphic);
        BrRes_GetObjData(param->res, BR_RES_OBJ_MUSICAL_SEND_BTN, &obj);

        for (i = 0; i < BTN_MAX; i++) {
            if (i == BTN_RETURN) {
                msgID = 4;
                setup.x = 32;
                setup.sequence = 0;
            } else {
                msgID = 43;
                setup.x = 128;
                setup.sequence = 4;
            }
            wk->btn[i] =
                BrBtn_InitEx(&setup, msgID, 96, CLACT_SURFACE_SUB, unit, wk->bmpoam, font, msg, &obj, wk->heapId);
        }
    }
}

static void BrMusicalSend_DeleteButtons(BrMusicalSendWork *wk, BrMusicalSendProcParam *param) {
    int i;

    if (wk->btn[0] != NULL) {
        BrRes_UnloadOBJ(param->res, BR_RES_OBJ_MUSICAL_SEND_BTN);
        BrRes_UnloadBG(param->res, BR_RES_BG_MUSICAL_SEND);
        for (i = 0; i < BTN_MAX; i++) {
            BrBtn_Exit(wk->btn[i]);
            wk->btn[i] = NULL;
        }
    }
}
