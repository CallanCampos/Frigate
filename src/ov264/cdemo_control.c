#include "app/cdemo.h"
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

// Descriptive name: these callbacks control the demo process and movie playback.
typedef struct {
    u16 arcId;
    u16 frameCount;
    u32 frameSize;
    u32 screenOffset;
} CDemoMovie;
typedef u32 (*CDemoStateFunc)(CDemoWork *work);
static u32 CDemo_StateInit(CDemoWork *work);
static u32 CDemo_StateExit(CDemoWork *work);
static u32 CDemo_StateWaitFade(CDemoWork *work);
static u32 CDemo_StateTransition(CDemoWork *work);
static u32 CDemo_StateScript(CDemoWork *work);
static u32 CDemo_StateMovie(CDemoWork *work);
static void CDemo_LoadMovieFrame(ArcTool *arc, u32 fileId, u32 offset, u32 size, u32 buffer);

static const CDemoStateFunc CDEMO_STATES[] = { CDemo_StateInit,       CDemo_StateExit,  CDemo_StateWaitFade,
                                               CDemo_StateTransition, CDemo_StateMovie, CDemo_StateScript };
static const CDemoMovie CDEMO_MOVIES[] = { { ARCID_CDEMO_GFLOGO, 151, 0x18000, 0 },
                                           { ARCID_CDEMO_OPENINGSW, 626, 0x18000, 0 },
                                           { ARCID_CDEMO_OPENINGSW, 626, 0x18000, 0 } };

BOOL CDemo_Main(CDemoWork *work) {
    if (work->param->canSkip) {
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_START)) {
            if (GFL_SndBGMIsPlaying() == TRUE) {
                GFL_SndBGMFadeOut(8);
            }
            work->state = 1;
            work->param->skipped = TRUE;
        }
    }
    work->state = CDEMO_STATES[work->state](work);
    return work->state != 6;
}
static u32 CDemo_StateInit(CDemoWork *work) {
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    reg_G2_BLDCNT = 0;
    reg_G2S_DB_BLDCNT = 0;
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_MAIN_SUB);
    CDemo_ConfigureVRAM();
    CDemo_InitGraphics(work);
    work->arc = GFL_ArcSysCreateFileHandle(CDEMO_MOVIES[work->param->demo].arcId, HEAPID_CDEMO);
    CDemo_InitScript(work);
    CDemo_StartVBlank(work);
    CDemo_FadeInBlack(work, 1, 1, 5);
    work->initialized = TRUE;
    return 2;
}
static u32 CDemo_StateExit(CDemoWork *work) {
    if (!GFL_WipeIsFinished())
        return 1;
    if (GFL_SndBGMIsFading() == TRUE)
        return 1;
    func_02005d8c();
    if (work->initialized == TRUE) {
        CDemo_StopVBlank(work);
        GFL_ArcToolFree(work->arc);
        CDemo_FreeScript(work);
        CDemo_DisableGraphics();
    }
    return 6;
}
static u32 CDemo_StateWaitFade(CDemoWork *work) {
    if (GFL_WipeIsFinished() == TRUE)
        return work->resumeState;
    return 2;
}
static u32 CDemo_StateTransition(CDemoWork *work) {
    return 3;
}
static u32 CDemo_StateScript(CDemoWork *work) {
    while (TRUE) {
        if (*work->scriptCursor == 20) {
            work->param->skipped = FALSE;
            return 1;
        }
        if (CDemo_RunCommand(work) != 1)
            break;
    }
    return work->state;
}
static u32 CDemo_StateMovie(CDemoWork *work) {
    const CDemoMovie *movie = &CDEMO_MOVIES[work->param->demo];
    if (work->lastTick == 0) {
        work->lastTick = OS_GetTick();
    } else {
        u64 now = OS_GetTick();
        work->elapsedFrameTime += OS_TicksToMicroSeconds(now - work->lastTick);
        work->lastTick = now;
        if (work->elapsedFrameTime < 0x1046b)
            return 4;
        work->elapsedFrameTime -= 0x1046b;
    }
    switch (work->frameState) {
    case 0:
        CDemo_LoadMovieFrame(work->arc, work->nextFrame, movie->screenOffset, movie->frameSize, 0);
        work->nextFrame++;
        CDemo_LoadMovieFrame(work->arc, work->nextFrame, movie->screenOffset, movie->frameSize, 1);
        work->nextFrame++;
        GFL_BGSysSetBGPriority(2, 0);
        GFL_BGSysSetBGPriority(3, 1);
        work->frameState = 1;
        break;
    case 1:
        GFL_BGSysSetBGPriority((u8)((work->currentBuffer ^ 1) + 2), 0);
        GFL_BGSysSetBGPriority((u8)(work->currentBuffer + 2), 1);
        CDemo_LoadMovieFrame(work->arc, work->nextFrame, movie->screenOffset, movie->frameSize, work->currentBuffer);
        work->nextFrame++;
        if (work->nextFrame >= movie->frameCount)
            work->frameState = 2;
        else
            work->currentBuffer ^= 1;
        break;
    case 2:
        GFL_BGSysSetBGPriority((u8)(work->currentBuffer + 2), 0);
        GFL_BGSysSetBGPriority((u8)((work->currentBuffer ^ 1) + 2), 1);
        work->frameState = 3;
        break;
    case 3:
        return 5;
    }
    return 4;
}
static void CDemo_LoadMovieFrame(ArcTool *arc, u32 fileId, u32 offset, u32 size, u32 buffer) {
    void *data = GFL_ArcToolReadHeapNewLZ(arc, fileId, TRUE, HEAPID_TAIL(HEAPID_CDEMO));
    DC_FlushRange(data, size);
    if (buffer == 0)
        GX_LoadBG2Scr(data, offset, size);
    else
        GX_LoadBG3Scr(data, offset, size);
    GFL_HeapFree(data);
}
