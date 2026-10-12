#ifndef POKEBW2_APP_DEMO3D_DEMO3D_CAPTURE_H
#define POKEBW2_APP_DEMO3D_DEMO3D_CAPTURE_H

// The 3D demos' motion blur (overlay 293): the display capture blended with the frame before it. The ROM has no name
// for this file; demo3d_capture.c is a guess

#include "types.h"
#include "gfl/heap.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

typedef struct Demo3DCapture Demo3DCapture;

typedef struct {
    // The display mode that shows the capture, and the BG mode and 3D BG0 with it
    int dispMode;
    int bgMode;
    int bg0As3D;
    int size;
    int mode;
    int sourceA;
    int sourceB;
    int dest;
    // The blend of the new frame and of the captured one, of 16
    int eva;
    int evb;
    HeapID heapId;
} Demo3DCaptureParam;

Demo3DCapture *Demo3DCapture_Start(TCBExManager *manager, int eva, int evb);
void Demo3DCapture_Stop(Demo3DCapture *capture);
Demo3DCapture *Demo3DCapture_Create(TCBExManager *manager, const Demo3DCaptureParam *param);
// Ends the capture and goes back to a display mode
void Demo3DCapture_End(Demo3DCapture *capture, int dispMode, int bgMode, int bg0As3D);

// The display modes that show VRAM A to C, as GX_DISPMODE_VRAM_D shows VRAM D. These belong in nitro/gx.h
#define GX_DISPMODE_VRAM_A 2
#define GX_DISPMODE_VRAM_B 6
#define GX_DISPMODE_VRAM_C 10

// NitroSDK's GX_GetBankForLCDC and G3X_SetDisp1DotDepth. These belong in gfl/graphics.h
u32 gfxGetLCDCBanks(void);
void gfxSet1DotDepth(fx32 depth);

#endif // POKEBW2_APP_DEMO3D_DEMO3D_CAPTURE_H
