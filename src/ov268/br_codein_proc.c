// The Battle Recorder's screen for entering a battle video's number: it runs br_codein.c's code input until the
// player confirms a number, which the record screen then looks up, or goes back. The name is a guess: the ROM gives
// none, and br_core.c runs it as BR_PROCID_CODEIN

#include "types.h"
#include "app/battle_recorder/br_codein_proc.h"
#include "app/battle_recorder/br_codein.h"
#include "app/battle_recorder/br_core.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "gfl/clact.h"
#include "gfl/std.h"
#include "system/printsys.h"
#include "text/system/0008.h"

// The digits of a battle video's number
#define BR_CODEIN_VIDEO_NUMBER_LENGTH 12

typedef struct {
    BrCodeInParam *codeInParam;
    BrCodeInWork *codeIn;
    BrMsgWin *text;
    PrintQueue *que;
    HeapID heapId;
    BrBallEffect *ballEff;
} BrCodeInProcWork;

static BOOL BrCodeInProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrCodeInProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrCodeInProc_Main(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions BR_CODEIN_PROC_FUNCTIONS = {
    BrCodeInProc_Init,
    BrCodeInProc_Main,
    BrCodeInProc_Exit,
};

static BOOL BrCodeInProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    BrCodeInProcParam *p = param;
    BrCodeInProcWork *wk;
    int blocks[BR_CODEIN_BLOCK_COUNT];

    BrRes_GetFont(p->res);
    BrRes_GetMsgData(p->res);
    wk = GFL_ProcInitSubsystem(proc, sizeof(BrCodeInProcWork), BrProcSys_GetHeapID(p->procSys));
    sys_memset(wk, 0, sizeof(BrCodeInProcWork));
    wk->heapId = BrProcSys_GetHeapID(p->procSys);

    BrRes_LoadBG(p->res, 29, wk->heapId);
    BrRes_LoadOBJ(p->res, 11, wk->heapId);
    BrRes_LoadOBJ(p->res, 12, wk->heapId);
    BrRes_LoadOBJ(p->res, 6, wk->heapId);

    wk->ballEff = BrBallEff_Init(p->unit, p->res, CLACT_SURFACE_SUB, wk->heapId);
    blocks[0] = 2;
    blocks[1] = 5;
    blocks[2] = 5;
    wk->codeInParam =
        BrCodeInParam_Create(wk->heapId, BR_CODEIN_VIDEO_NUMBER_LENGTH, p->unit, p->res, wk->ballEff, blocks);
    wk->codeIn = BrCodeIn_Init(wk->codeInParam, wk->heapId);

    wk->que = func_02021998(wk->heapId);
    wk->text = BrText_Init(p->res, wk->que, wk->heapId);
    BrText_Print(wk->text, p->res, Bank0008_Text_PleaseEnterVideoNumber);
    return TRUE;
}

static BOOL BrCodeInProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    BrCodeInProcParam *p = param;
    BrCodeInProcWork *wk = work;

    BrText_Exit(wk->text, p->res);
    BrCodeIn_Exit(wk->codeIn);
    BrCodeInParam_Free(wk->codeInParam);
    func_02021a18(wk->que);
    BrBallEff_Exit(wk->ballEff);
    BrRes_UnloadBG(p->res, 29);
    BrRes_UnloadOBJ(p->res, 6);
    BrRes_UnloadOBJ(p->res, 12);
    BrRes_UnloadOBJ(p->res, 11);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL BrCodeInProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    enum {
        SEQ_FADEIN_START,
        SEQ_FADEIN_WAIT,
        SEQ_MAIN,
        SEQ_FADEOUT_START,
        SEQ_FADEOUT_WAIT,
        SEQ_END,
    };
    BrCodeInProcParam *p = param;
    BrCodeInProcWork *wk = work;

    switch (*state) {
    case SEQ_FADEIN_START:
        BrFade_StartFade(p->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_IN);
        *state = SEQ_FADEIN_WAIT;
        break;
    case SEQ_FADEIN_WAIT:
        if (BrFade_IsEnd(p->fade)) {
            *state = SEQ_MAIN;
        }
        break;
    case SEQ_MAIN:
        BrCodeIn_Main(wk->codeIn);
        switch (BrCodeIn_GetResult(wk->codeIn)) {
        case BR_CODEIN_RESULT_CANCEL:
            BrProcSys_Pop(p->procSys);
            *state = SEQ_FADEOUT_START;
            break;
        case BR_CODEIN_RESULT_DECIDE:
            p->videoNumber = BrCodeIn_GetNumber(wk->codeIn);
            BrProcSys_Push(p->procSys, BR_PROCID_RECORD);
            *state = SEQ_FADEOUT_START;
            break;
        }
        break;
    case SEQ_FADEOUT_START:
        BrFade_StartFade(p->fade, BR_FADE_TYPE_ALPHA_BG012, BR_FADE_DISPLAY_BOTH, BR_FADE_DIR_OUT);
        *state = SEQ_FADEOUT_WAIT;
        break;
    case SEQ_FADEOUT_WAIT:
        if (BrFade_IsEnd(p->fade)) {
            *state = SEQ_END;
        }
        break;
    case SEQ_END:
        return TRUE;
    }

    if (wk->text != NULL) {
        BrText_Main(wk->text);
    }
    func_02021a3c(wk->que);
    BrBallEff_Main(wk->ballEff);
    return FALSE;
}
