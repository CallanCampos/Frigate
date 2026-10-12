#include "types.h"
#include "app/demo3d/demo3d_capture.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"

// The 3D demos' motion blur: every frame is captured to a VRAM bank, blended with the frame captured before it, and
// the bank is what the upper screen shows. The ROM has no name for this file, which lies between demo3d_data.c and
// the commands that use it; demo3d_capture.c is a guess, and it may be the start of the commands' file instead

struct Demo3DCapture {
    u32 lcdcBanks;
    Demo3DCaptureParam param;
    // The display shows the bank: the capture runs
    BOOL started;
    TCBEx *task;
};

static void Demo3DCapture_Task(TCBEx *task, void *data);
static void Demo3DCapture_VBlank(TCB *tcb, void *data);
static void Demo3DCapture_Begin(const Demo3DCaptureParam *param);

Demo3DCapture *Demo3DCapture_Start(TCBExManager *manager, int eva, int evb) {
    Demo3DCaptureParam param = {
        GX_DISPMODE_VRAM_D,
        GX_BGMODE_0,
        GX_BG0_AS_3D,
        GX_CAPTURE_SIZE_256x192,
        GX_CAPTURE_MODE_AB,
        GX_CAPTURE_SRCA_2D3D,
        GX_CAPTURE_SRCB_VRAM_0x00000,
        GX_CAPTURE_DEST_VRAM_D_0x00000,
        0,
        0,
        HEAPID_DEMO3D,
    };

    param.eva = eva;
    param.evb = evb;
    return Demo3DCapture_Create(manager, &param);
}

void Demo3DCapture_Stop(Demo3DCapture *capture) {
    Demo3DCapture_End(capture, GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_3D);
}

Demo3DCapture *Demo3DCapture_Create(TCBExManager *manager, const Demo3DCaptureParam *param) {
    TCBEx *task = GFL_TCBExMgrAddTask(manager, Demo3DCapture_Task, sizeof(Demo3DCapture), 5);
    Demo3DCapture *capture = GFL_TCBExGetData(task);

    capture->param = *param;
    capture->task = task;
    capture->started = FALSE;
    capture->lcdcBanks = gfxGetLCDCBanks();
    Demo3DCapture_Begin(&capture->param);
    GFL_VBlankTCBAdd(Demo3DCapture_VBlank, capture, 0);
    return capture;
}

void Demo3DCapture_End(Demo3DCapture *capture, int dispMode, int bgMode, int bg0As3D) {
    gfxSetEngineModeA(dispMode, bgMode, bg0As3D);
    gfxSetLCDCBanks(capture->lcdcBanks);
    switch (capture->param.dispMode) {
    case GX_DISPMODE_VRAM_A:
        sys_memset32_fast(0, (void *)HW_LCDC_VRAM_A, 0x20000);
        break;
    case GX_DISPMODE_VRAM_B:
        sys_memset32_fast(0, (void *)HW_LCDC_VRAM_B, 0x20000);
        break;
    case GX_DISPMODE_VRAM_C:
        sys_memset32_fast(0, (void *)HW_LCDC_VRAM_C, 0x20000);
        break;
    case GX_DISPMODE_VRAM_D:
        sys_memset32_fast(0, (void *)HW_LCDC_VRAM_D, 0x20000);
        break;
    default:
        gfxSetLCDCBanks(GX_VRAM_NONE);
        break;
    }
    GFL_TCBExRequestEnd(capture->task);
}

static void Demo3DCapture_Task(TCBEx *task, void *data) {
    Demo3DCapture *capture = data;

    if (capture->started) {
        GX_SetCapture(capture->param.size, capture->param.mode, capture->param.sourceA, capture->param.sourceB,
                      capture->param.dest, capture->param.eva, capture->param.evb);
    }
}

// Shows the bank that the frames are captured to, from the first vertical blank on
static void Demo3DCapture_VBlank(TCB *tcb, void *data) {
    Demo3DCapture *capture = data;

    switch (capture->param.dispMode) {
    case GX_DISPMODE_VRAM_A:
        gfxSetLCDCBanks(GX_VRAM_A);
        break;
    case GX_DISPMODE_VRAM_B:
        gfxSetLCDCBanks(GX_VRAM_B);
        break;
    case GX_DISPMODE_VRAM_C:
        gfxSetLCDCBanks(GX_VRAM_C);
        break;
    case GX_DISPMODE_VRAM_D:
        gfxSetLCDCBanks(GX_VRAM_D);
        break;
    default:
        gfxSetLCDCBanks(GX_VRAM_NONE);
        break;
    }
    gfxSetEngineModeA(capture->param.dispMode, capture->param.bgMode, capture->param.bg0As3D);
    capture->started = TRUE;
    GFL_TCBRemove(tcb);
}

// Clears the bank and captures the first frame alone
static void Demo3DCapture_Begin(const Demo3DCaptureParam *param) {
    switch (param->dispMode) {
    case GX_DISPMODE_VRAM_A:
        sys_memset32_fast(0, (void *)HW_LCDC_VRAM_A, 0x20000);
        break;
    case GX_DISPMODE_VRAM_B:
        sys_memset32_fast(0, (void *)HW_LCDC_VRAM_B, 0x20000);
        break;
    case GX_DISPMODE_VRAM_C:
        sys_memset32_fast(0, (void *)HW_LCDC_VRAM_C, 0x20000);
        break;
    case GX_DISPMODE_VRAM_D:
        sys_memset32_fast(0, (void *)HW_LCDC_VRAM_D, 0x20000);
        break;
    }
    GX_SetCapture(param->size, param->mode, param->sourceA, param->sourceB, param->dest, 16, 0);
}
