#include "types.h"
#include "app/cdemo_main.h"
#include "app/cdemo_script.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "system/wipe.h"

// The animation steps every 1/15 second, in microseconds
#define ANIMATION_STEP_US 66667
// The size of a screen as a direct color bitmap: 256 by 192 pixels of 2 bytes
#define SCREEN_SIZE 0x18000

typedef u32 (*CDemoStateFunc)(CDemo *demo);
typedef u32 (*CDemoCmdFunc)(CDemo *demo, const u32 *args);

// What a demo shows: the archive of its frames, how many there are, and where each goes
typedef struct {
    u16 arcId;
    u16 frameCount;
    u32 size;
    u32 offset;
} CDemoFrames;

static u32 CDemo_StateInit(CDemo *demo);
static u32 CDemo_StateFinish(CDemo *demo);
static u32 CDemo_StateWipe(CDemo *demo);
static u32 CDemo_StateIdle(CDemo *demo);
static u32 CDemo_StateScript(CDemo *demo);
static u32 CDemo_StateAnimation(CDemo *demo);
static void CDemo_LoadFrame(ArcTool *arc, u32 fileId, u32 offset, u32 size, u32 layer);
static BOOL CDemo_RunCommand(CDemo *demo);
static u32 CDemo_CmdWait(CDemo *demo, const u32 *args);
static u32 CDemo_CmdFadeInBlack(CDemo *demo, const u32 *args);
static u32 CDemo_CmdFadeOutBlack(CDemo *demo, const u32 *args);
static u32 CDemo_CmdFadeInWhite(CDemo *demo, const u32 *args);
static u32 CDemo_CmdFadeOutWhite(CDemo *demo, const u32 *args);
static u32 CDemo_CmdWaitWipe(CDemo *demo, const u32 *args);
static u32 CDemo_CmdLoadGraphics(CDemo *demo, const u32 *args);
static u32 CDemo_CmdSetBGEnabled(CDemo *demo, const u32 *args);
static u32 CDemo_CmdSetBGPriority(CDemo *demo, const u32 *args);
static u32 CDemo_CmdSetLayerParams(CDemo *demo, const u32 *args);
static u32 CDemo_CmdIdle(CDemo *demo, const u32 *args);
static u32 CDemo_CmdResetLayers(CDemo *demo, const u32 *args);
static u32 CDemo_CmdNop12(CDemo *demo, const u32 *args);
static u32 CDemo_CmdFunc02005d8c(CDemo *demo, const u32 *args);
static u32 CDemo_CmdPlayBGM(CDemo *demo, const u32 *args);
static u32 CDemo_CmdNop15(CDemo *demo, const u32 *args);
static u32 CDemo_CmdNop16(CDemo *demo, const u32 *args);
static u32 CDemo_CmdNop17(CDemo *demo, const u32 *args);
static u32 CDemo_CmdStartAnimation(CDemo *demo, const u32 *args);
static u32 CDemo_CmdNop19(CDemo *demo, const u32 *args);

static const CDemoFrames sFrames[3] = {
    { ARCID_CDEMO_GFLOGO, 151, SCREEN_SIZE, 0 },
    { ARCID_CDEMO_OPENINGSW, 626, SCREEN_SIZE, 0 },
    { ARCID_CDEMO_OPENINGSW, 626, SCREEN_SIZE, 0 },
};

BOOL CDemo_Update(CDemo *demo) {
    static const CDemoStateFunc sStateFuncs[6] = {
        CDemo_StateInit, CDemo_StateFinish, CDemo_StateWipe, CDemo_StateIdle, CDemo_StateAnimation, CDemo_StateScript,
    };

    if (demo->param->canSkip != FALSE &&
        (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_START))) {
        if (GFL_SndBGMIsPlaying() == TRUE) {
            GFL_SndBGMFadeOut(8);
        }
        demo->state = CDEMO_STATE_FINISH;
        demo->param->skipped = TRUE;
    }
    demo->state = sStateFuncs[demo->state](demo);
    if (demo->state == CDEMO_STATE_END) {
        return FALSE;
    }
    return TRUE;
}

static u32 CDemo_StateInit(CDemo *demo) {
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    reg_G2_BLDCNT = 0;
    reg_G2S_DB_BLDCNT = 0;
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_MAIN_SUB);
    CDemo_SetVRAMBanks();
    CDemo_SetupBGs(demo);
    demo->arc = GFL_ArcSysCreateFileHandle(sFrames[demo->param->demo].arcId, HEAPID_CDEMO);
    CDemo_InitScript(demo);
    CDemo_AddVBlankTask(demo);
    CDemo_FadeInBlack(demo, 1, 1, CDEMO_STATE_SCRIPT);
    demo->initialized = TRUE;
    return CDEMO_STATE_WIPE;
}

static u32 CDemo_StateFinish(CDemo *demo) {
    if (GFL_WipeIsFinished() == FALSE) {
        return CDEMO_STATE_FINISH;
    }
    if (GFL_SndBGMIsFading() == TRUE) {
        return CDEMO_STATE_FINISH;
    }
    func_02005d8c();
    if (demo->initialized == TRUE) {
        CDemo_RemoveVBlankTask(demo);
        GFL_ArcToolFree(demo->arc);
        CDemo_Free(demo);
        CDemo_HideBGs();
    }
    return CDEMO_STATE_END;
}

static u32 CDemo_StateWipe(CDemo *demo) {
    if (GFL_WipeIsFinished() == TRUE) {
        return demo->nextState;
    }
    return CDEMO_STATE_WIPE;
}

static u32 CDemo_StateIdle(CDemo *demo) {
    return CDEMO_STATE_IDLE;
}

static u32 CDemo_StateScript(CDemo *demo) {
    while (TRUE) {
        if (*demo->script == CDEMO_CMD_END) {
            demo->param->skipped = FALSE;
            return CDEMO_STATE_FINISH;
        }
        if (CDemo_RunCommand(demo) != 1) {
            break;
        }
    }
    return demo->state;
}

static u32 CDemo_StateAnimation(CDemo *demo) {
    const CDemoFrames *frames = &sFrames[demo->param->demo];
    u64 now;

    if (demo->lastTick == 0) {
        demo->lastTick = clock();
    } else {
        now = clock();
        demo->elapsedUs += OS_TicksToMicroSeconds(now - demo->lastTick);
        demo->lastTick = now;
        if (demo->elapsedUs < ANIMATION_STEP_US) {
            return CDEMO_STATE_ANIMATION;
        }
        demo->elapsedUs -= ANIMATION_STEP_US;
    }
    switch (demo->animationState) {
    case 0:
        CDemo_LoadFrame(demo->arc, demo->frame, frames->offset, frames->size, 0);
        demo->frame++;
        CDemo_LoadFrame(demo->arc, demo->frame, frames->offset, frames->size, 1);
        demo->frame++;
        GFL_BGSysSetBGPriority(2, 0);
        GFL_BGSysSetBGPriority(3, 1);
        demo->animationState = 1;
        break;
    case 1:
        GFL_BGSysSetBGPriority(2 + (demo->layer ^ 1), 0);
        GFL_BGSysSetBGPriority(2 + demo->layer, 1);
        CDemo_LoadFrame(demo->arc, demo->frame, frames->offset, frames->size, demo->layer);
        demo->frame++;
        if (demo->frame >= frames->frameCount) {
            demo->animationState = 2;
        } else {
            demo->layer ^= 1;
        }
        break;
    case 2:
        GFL_BGSysSetBGPriority(2 + demo->layer, 0);
        GFL_BGSysSetBGPriority(2 + (demo->layer ^ 1), 1);
        demo->animationState = 3;
        break;
    case 3:
        return CDEMO_STATE_SCRIPT;
    }
    return CDEMO_STATE_ANIMATION;
}

static void CDemo_LoadFrame(ArcTool *arc, u32 fileId, u32 offset, u32 size, u32 layer) {
    void *file = GFL_ArcToolReadHeapNewLZ(arc, fileId, TRUE, HEAPID_TAIL(HEAPID_CDEMO));

    cp15_flushDC(file, size);
    if (layer == 0) {
        gfxUploadBGScreen2A(file, offset, size);
    } else {
        gfxUploadBGScreen3A(file, offset, size);
    }
    GFL_HeapFree(file);
}

static BOOL CDemo_RunCommand(CDemo *demo) {
    static const CDemoCmdFunc sCommands[CDEMO_CMD_END] = {
        CDemo_CmdWait,          CDemo_CmdFadeInBlack,    CDemo_CmdFadeOutBlack,   CDemo_CmdFadeInWhite,
        CDemo_CmdFadeOutWhite,  CDemo_CmdWaitWipe,       CDemo_CmdLoadGraphics,   CDemo_CmdSetBGEnabled,
        CDemo_CmdSetBGPriority, CDemo_CmdSetLayerParams, CDemo_CmdIdle,           CDemo_CmdResetLayers,
        CDemo_CmdNop12,         CDemo_CmdFunc02005d8c,   CDemo_CmdPlayBGM,        CDemo_CmdNop15,
        CDemo_CmdNop16,         CDemo_CmdNop17,          CDemo_CmdStartAnimation, CDemo_CmdNop19,
    };
    static const u32 sLengths[CDEMO_CMD_COUNT] = {
        2, 3, 3, 3, 3, 1, 3, 3, 3, 4, 3, 1, 1, 1, 2, 2, 3, 2, 2, 2, 1,
    };
    u32 result = sCommands[*demo->script](demo, demo->script);

    if (result != 0) {
        demo->script += sLengths[*demo->script];
    }
    return result;
}

static u32 CDemo_CmdWait(CDemo *demo, const u32 *args) {
    if (demo->waitFrames == args[1]) {
        demo->waitFrames = 0;
        return 2;
    }
    demo->waitFrames++;
    return 0;
}

static u32 CDemo_CmdFadeInBlack(CDemo *demo, const u32 *args) {
    CDemo_FadeInBlack(demo, args[1], args[2], CDEMO_STATE_SCRIPT);
    return 1;
}

static u32 CDemo_CmdFadeOutBlack(CDemo *demo, const u32 *args) {
    CDemo_FadeOutBlack(demo, args[1], args[2], CDEMO_STATE_SCRIPT);
    return 1;
}

static u32 CDemo_CmdFadeInWhite(CDemo *demo, const u32 *args) {
    CDemo_FadeInWhite(demo, args[1], args[2], CDEMO_STATE_SCRIPT);
    return 1;
}

static u32 CDemo_CmdFadeOutWhite(CDemo *demo, const u32 *args) {
    CDemo_FadeOutWhite(demo, args[1], args[2], CDEMO_STATE_SCRIPT);
    return 1;
}

static u32 CDemo_CmdWaitWipe(CDemo *demo, const u32 *args) {
    demo->state = CDEMO_STATE_WIPE;
    demo->nextState = CDEMO_STATE_SCRIPT;
    return 2;
}

static u32 CDemo_CmdLoadGraphics(CDemo *demo, const u32 *args) {
    GFL_BGSysLoadArcNCGRStatic(demo->arc, args[2], args[1], 0, 0, TRUE, HEAPID_CDEMO);
    if (args[2] == 0) {
        loadBGScrToVramByFileNoReserveNegAlign(demo->arc, args[2], args[1], 0, 0, FALSE, HEAPID_CDEMO);
    }
    CDemo_LoadPalette(demo, args[1], args[2]);
    return 1;
}

static u32 CDemo_CmdSetBGEnabled(CDemo *demo, const u32 *args) {
    GFL_BGSysSetBGEnabled(args[1], args[2]);
    return 1;
}

static u32 CDemo_CmdSetBGPriority(CDemo *demo, const u32 *args) {
    GFL_BGSysSetBGPriority(args[1], args[2]);
    return 1;
}

static u32 CDemo_CmdSetLayerParams(CDemo *demo, const u32 *args) {
    if (args[1] == 0) {
        demo->layerParams[0][0] = args[2];
        demo->layerParams[0][1] = args[3];
    } else {
        demo->layerParams[1][0] = args[2];
        demo->layerParams[1][1] = args[3];
    }
    return 1;
}

static u32 CDemo_CmdIdle(CDemo *demo, const u32 *args) {
    demo->idleParams[0] = args[1];
    demo->idleParams[1] = args[2];
    demo->state = CDEMO_STATE_IDLE;
    demo->nextState = CDEMO_STATE_SCRIPT;
    return 2;
}

static u32 CDemo_CmdResetLayers(CDemo *demo, const u32 *args) {
    reg_G2_BLDCNT = 0;
    demo->layerParams[0][0] = 0;
    demo->layerParams[0][1] = 0;
    demo->layerParams[1][0] = 0;
    demo->layerParams[1][1] = 0;
    return 1;
}

static u32 CDemo_CmdNop12(CDemo *demo, const u32 *args) {
    return 1;
}

static u32 CDemo_CmdFunc02005d8c(CDemo *demo, const u32 *args) {
    func_02005d8c();
    return 1;
}

static u32 CDemo_CmdPlayBGM(CDemo *demo, const u32 *args) {
    GFL_SndBGMStop(args[1]);
    return 1;
}

static u32 CDemo_CmdNop15(CDemo *demo, const u32 *args) {
    return 1;
}

static u32 CDemo_CmdNop16(CDemo *demo, const u32 *args) {
    return 1;
}

static u32 CDemo_CmdNop17(CDemo *demo, const u32 *args) {
    return 1;
}

static u32 CDemo_CmdStartAnimation(CDemo *demo, const u32 *args) {
    demo->frame = 0;
    demo->layer = 0;
    demo->animationState = 0;
    demo->lastTick = 0;
    demo->elapsedUs = 0;
    demo->state = CDEMO_STATE_ANIMATION;
    return 2;
}

static u32 CDemo_CmdNop19(CDemo *demo, const u32 *args) {
    return 1;
}
