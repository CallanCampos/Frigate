#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "demo/intro.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "system/gf_font.h"

// The intro's process. ov294 has no file name for this file, which is named after the others (intro_cmd.c and so on)

typedef struct {
    HeapID heapId;
    IntroParam *param;
    IntroGraphic *graphic;
    Font *font;
    MsgData *msgData;
    IntroCmd *cmd;
    IntroMcss *mcss;
    IntroG3d *g3d;
    IntroParticle *particle;
} IntroWork;

BOOL IntroProc_Init(GameProc *proc, u32 *state, void *param, void *work);
BOOL IntroProc_Main(GameProc *proc, u32 *state, void *param, void *work);
BOOL IntroProc_Exit(GameProc *proc, u32 *state, void *param, void *work);

const u32 INTRO_SOUNDS[] = {
    SEQ_SE_NAGERU, SEQ_SE_BOWA2, SEQ_SE_OPEN2, SEQ_SE_KON, SEQ_SE_TOUJOU_INTRO, SEQ_BGM_STARTING, SEQ_BGM_STARTING2,
};

const GameProcFunctions INTRO_PROC_FUNCTIONS = { IntroProc_Init, IntroProc_Main, IntroProc_Exit };

const u32 INTRO_SOUND_COUNT = NELEMS(INTRO_SOUNDS);

BOOL IntroProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    IntroWork *wk;

    GFL_OvlLoad(OVERLAY_ID(139));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_INTRO, 0x100000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(IntroWork), HEAPID_INTRO);
    sys_memset(wk, 0, sizeof(IntroWork));
    wk->heapId = HEAPID_INTRO;
    wk->param = param;
    wk->graphic = IntroGraphic_Create(1, wk->param->mode, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_INTRO_BTL_SERVER_FLOW, wk->heapId);
    wk->mcss = IntroMcss_Create(wk->heapId, wk->param->mode);
    wk->g3d = IntroG3d_Create(wk->graphic, wk->param->mode, wk->heapId);
    wk->particle = IntroParticle_Create(wk->graphic, wk->heapId);
    wk->cmd = IntroCmd_Create(wk->g3d, wk->particle, wk->mcss, wk->param, wk->graphic, wk->heapId);
    return TRUE;
}

BOOL IntroProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    IntroWork *wk = work;
    HeapID heapId;

    IntroParticle_Free(wk->particle);
    IntroMcss_Free(wk->mcss);
    IntroG3d_Free(wk->g3d);
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->font);
    IntroCmd_Free(wk->cmd);
    IntroGraphic_Free(wk->graphic);
    heapId = wk->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_OvlUnload(OVERLAY_ID(139));
    return TRUE;
}

BOOL IntroProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    IntroWork *wk = work;

    if (!IntroCmd_Update(wk->cmd)) {
        return TRUE;
    }
    IntroGraphic_Update(wk->graphic);
    IntroGraphic_Begin3D(wk->graphic);
    IntroMcss_Update(wk->mcss);
    IntroMcss_Draw(wk->mcss);
    IntroParticle_Update(wk->particle);
    IntroG3d_Draw(wk->g3d);
    IntroGraphic_End3D(wk->graphic);
    return FALSE;
}
