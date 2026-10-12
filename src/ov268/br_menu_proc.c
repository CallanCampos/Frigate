// The Battle Recorder's menu screen: it runs one of br_btn.c's button menus until the player chooses a screen or goes
// back, and changes the Battle Recorder's color when the player touches a sidebar. The name is a guess: the ROM gives
// none, and br_core.c runs it as BR_PROCID_MENU

#include "types.h"
#include "app/battle_recorder/br_menu_proc.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "constants/sound.h"
#include "gfl/clact.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"

#define BR_MENU_COLOR_NUM 7

typedef struct {
    BrBtnSys *btnSys;
    BrBallEffect *ballEff;
    TCB *vblankTcb;
    BrMenuProcParam *param;
    // Set by the VBlank task when it has loaded the new color
    BOOL isColorLoaded;
    HeapID heapId;
} BrMenuWork;

static BOOL BrMenu_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrMenu_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrMenu_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrMenu_VBlankLoadColor(TCB *tcb, void *data);

const GameProcFunctions BR_MENU_PROC_FUNCTIONS = {
    BrMenu_ProcInit,
    BrMenu_ProcMain,
    BrMenu_ProcExit,
};

static BOOL BrMenu_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrMenuProcParam *p = param;
    BrMenuWork *wk;

    wk = GFL_ProcInitSubsystem(proc, sizeof(BrMenuWork), BrProcSys_GetHeapID(p->procSys));
    sys_memset(wk, 0, sizeof(BrMenuWork));
    wk->param = p;
    wk->heapId = BrProcSys_GetHeapID(p->procSys);

    BrRes_UnloadCommonPltt(p->res, CLACT_SURFACE_MAIN);
    BrRes_UnloadCommonPltt(p->res, CLACT_SURFACE_SUB);
    BrRes_LoadCommonPltt(p->res, CLACT_SURFACE_MAIN, wk->heapId);
    BrRes_LoadCommonPltt(p->res, CLACT_SURFACE_SUB, wk->heapId);
    if (p->menuID < 5) {
        BrRes_LoadBG(p->res, 3, wk->heapId);
    } else {
        BrRes_LoadBG(p->res, 4, wk->heapId);
    }

    wk->ballEff = BrBallEff_Init(p->unit, p->res, CLACT_SURFACE_SUB, wk->heapId);
    wk->btnSys = BrBtnSys_Init(p->menuID, p->unit, p->res, p->recordInfo, p->btnRecovery, wk->ballEff, wk->heapId);

    if (p->fadeType == BR_FADE_TYPE_MASTER_BRIGHT_AND_ALPHA) {
        BrFade_SetAlpha(p->fade, BR_FADE_DISPLAY_SUB, 0);
    } else {
        BrFade_SetAlpha(p->fade, BR_FADE_DISPLAY_BOTH, 0);
    }
    return TRUE;
}

static BOOL BrMenu_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrMenuProcParam *p = param;
    BrMenuWork *wk = work;

    if (p->menuID < 5) {
        BrRes_UnloadBG(p->res, 3);
    } else {
        BrRes_UnloadBG(p->res, 4);
    }
    BrBtnSys_Exit(wk->btnSys);
    BrBallEff_Exit(wk->ballEff);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BrMenu_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    enum {
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_MAIN,
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
        SEQ_COLOR_FADEOUT_START,
        SEQ_COLOR_FADEOUT_WAIT,
        SEQ_COLOR_LOAD_START,
        SEQ_COLOR_LOAD_WAIT,
        SEQ_COLOR_FADEIN_START,
        SEQ_COLOR_FADEIN_WAIT,
    };
    // The colors, in the order that touching a sidebar goes through them
    static const u32 sBrMenuColors[BR_MENU_COLOR_NUM] = { 0, 2, 3, 4, 5, 6, 7 };
    BrMenuProcParam *p = param;
    BrMenuWork *wk = work;

    switch (*state) {
    case SEQ_FADEIN_START:
        BrFade_StartFade(p->fade, p->fadeType, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *state = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p->fade)) {
            *state = SEQ_MAIN;
        }
        break;
    case SEQ_MAIN:
        // Only the top menus change the color, when a sidebar is touched
        if (p->menuID <= 4 && !BrBtnSys_IsBusy(wk->btnSys)) {
            u32 x;
            u32 y;

            if (func_0203dac8(&x, &y) && (x < 8 || x > 248)) {
                GFL_SndSEPlay(SEQ_SE_BREC_06);
                *state = SEQ_COLOR_FADEOUT_START;
                break;
            }
        }
        BrBtnSys_Main(wk->btnSys);
        if (BrBtnSys_GetInput(wk->btnSys, &p->unk4, &p->nextMode) != 0) {
            *state = SEQ_FADEOUT_START;
        }
        break;
    case SEQ_FADEOUT_START:
        if (p->unk4 == BR_PROCID_MUSICAL_SEND) {
            BrFade_StartFade(p->fade, BR_FADE_TYPE_MASTER_BRIGHT_AND_ALPHA, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        } else {
            BrFade_StartFade(p->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        }
        *state = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p->fade)) {
            *state = SEQ_END;
        }
        break;
    case SEQ_END:
        switch (BrBtnSys_GetInput(wk->btnSys, &p->unk4, &p->nextMode)) {
        case 1:
            BrProcSys_Push(p->procSys, p->unk4);
            return TRUE;
        case 2:
            BrProcSys_Pop(p->procSys);
            *p->result = BrBtnSys_GetUnk54(wk->btnSys);
            return TRUE;
        }
        break;
    case SEQ_COLOR_FADEOUT_START: {
        u32 color;
        u32 i;

        color = BrRes_GetColor(p->res);
        for (i = 0; i < BR_MENU_COLOR_NUM; i++) {
            if (color == sBrMenuColors[i]) {
                color = sBrMenuColors[(i + 1) % BR_MENU_COLOR_NUM];
                break;
            }
        }
        BrRes_SetColor(p->res, color);
        BrFade_SetColor(p->fade, BrRes_GetFadeColor(p->res));
        BrFade_StartFade(p->fade, BR_FADE_TYPE_PLTT, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *state = SEQ_COLOR_FADEOUT_WAIT;
        break;
    }
    case SEQ_COLOR_FADEOUT_WAIT:
        if (BrFade_IsEnd(p->fade)) {
            *state = SEQ_COLOR_LOAD_START;
        }
        break;
    case SEQ_COLOR_LOAD_START:
        wk->vblankTcb = GFL_VBlankTCBAdd(BrMenu_VBlankLoadColor, wk, 0);
        wk->isColorLoaded = FALSE;
        *state = SEQ_COLOR_LOAD_WAIT;
        break;
    case SEQ_COLOR_LOAD_WAIT:
        if (wk->isColorLoaded) {
            GFL_TCBRemove(wk->vblankTcb);
            *state = SEQ_COLOR_FADEIN_START;
        }
        break;
    case SEQ_COLOR_FADEIN_START:
        BrFade_StartFade(p->fade, BR_FADE_TYPE_PLTT, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *state = SEQ_COLOR_FADEIN_WAIT;
        break;
    case SEQ_COLOR_FADEIN_WAIT:
        if (BrFade_IsEnd(p->fade)) {
            *state = SEQ_MAIN;
        }
        break;
    }

    BrBallEff_Main(wk->ballEff);
    return FALSE;
}

// Loads the palettes of the new color, once
static void BrMenu_VBlankLoadColor(TCB *tcb, void *data) {
    BrMenuWork *wk = data;
    BrMenuProcParam *p = wk->param;

    if (!wk->isColorLoaded) {
        BrRes_LoadColorPlttToFade(p->res, p->fade, wk->heapId);
    }
    wk->isColorLoaded = TRUE;
}
