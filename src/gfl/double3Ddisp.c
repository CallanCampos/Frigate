#include "types.h"
#include "gfl/double3Ddisp.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "nitro/gx.h"
#include "nitro/os.h"

// 3D on both screens, as in NitroSDK's dual 3D demo: the main engine renders to each screen on alternate frames, and
// the display capture keeps each frame in VRAM C or D for the sub engine to show on the other screen. The sub engine
// shows the capture in VRAM C as a direct color bitmap BG, or the one in VRAM D as bitmap OBJ, which tile the screen

// The OBJ that show the capture, 64x64 each, at the end of OAM
#define OAM_COUNT 12
#define OAM_FIRST (128 - OAM_COUNT)

typedef struct {
    HeapID heapId;
    // Whether the main engine renders to the bottom screen this frame
    BOOL renderBottom;
    BOOL needRender;
    GXOamAttr oam[OAM_COUNT];
} Dual3D;

static void GFL_G3DDual3DInitOAM(void);
static void GFL_G3DDual3DSetupTopScreen(void);
static void GFL_G3DDual3DSetupBottomScreen(void);

static Dual3D *sDual3D;

void GFL_G3DDual3DInit(HeapID heapId) {
    sDual3D = GFL_HeapAllocate(heapId, sizeof(Dual3D), TRUE, "double3Ddisp.c", 63);
    sDual3D->heapId = heapId;
    sDual3D->renderBottom = TRUE;
    sDual3D->needRender = FALSE;
    GFL_G3DDual3DInitOAM();
}

static void GFL_G3DDual3DInitOAM(void) {
    int i;
    int x;
    int y;
    int n = 0;

    GXS_SetOBJVRamModeBmp(GX_OBJVRAMMODE_BMP_2D_W256);
    for (i = 0; i < OAM_COUNT; i++) {
        sDual3D->oam[i].attr01 = 0;
        sDual3D->oam[i].attr23 = 0;
    }
    for (y = 0; y < 192; y += 64) {
        for (x = 0; x < 256; x += 64) {
            G2_SetOBJAttr(&sDual3D->oam[n], x, y, 0, GX_OAM_MODE_BITMAPOBJ, FALSE, GX_OAM_EFFECT_NONE,
                          GX_OAM_SHAPE_64x64, GX_OAM_COLORMODE_16, (y / 8) * 32 + (x / 8), 15, 0);
            n++;
        }
    }
    DC_FlushRange(sDual3D->oam, sizeof(sDual3D->oam));
    GXS_LoadOAM(sDual3D->oam, OAM_FIRST * sizeof(GXOamAttr), sizeof(sDual3D->oam));
}

void GFL_G3DDual3DFree(void) {
    GX_DisableBankForLCDC();
    GX_DisableBankForSubBG();
    GX_DisableBankForSubOBJ();
    GFL_HeapFree(sDual3D);
    sDual3D = NULL;
}

BOOL GFL_G3DDual3DGetRenderDisp(void) {
    return sDual3D->renderBottom;
}

void GFL_G3DDual3DRaiseNeedRenderFlag(void) {
    sDual3D->needRender = TRUE;
}

void GFL_G3DDual3DExecVBlank(void) {
    if (reg_G3X_VTXRAM_COUNT != 0 || !sDual3D->needRender) {
        return;
    }
    if (sDual3D->renderBottom) {
        GFL_G3DDual3DSetupBottomScreen();
    } else {
        GFL_G3DDual3DSetupTopScreen();
    }
    sDual3D->needRender = FALSE;
    sDual3D->renderBottom = !sDual3D->renderBottom;
}

// The main engine renders to the top screen and the capture goes to VRAM D, while the sub engine shows the capture in
// VRAM C on the bottom screen
static void GFL_G3DDual3DSetupTopScreen(void) {
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GX_ResetBankForSubOBJ();
    GX_SetBankForSubBG(GX_VRAM_SUB_BG_128_C);
    GX_SetBankForLCDC(GX_VRAM_D);
    GX_SetCapture(GX_CAPTURE_SIZE_256x192, GX_CAPTURE_MODE_A, GX_CAPTURE_SRCA_2D3D, GX_CAPTURE_SRCB_VRAM_0x00000,
                  GX_CAPTURE_DEST_VRAM_D_0x00000, 16, 0);
    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_3D);
    GXS_SetGraphicsMode(GX_BGMODE_5);
    GXS_SetVisiblePlane(GX_PLANEMASK_BG2);
    G2S_SetBG2ControlDCBmp(GX_BG_SCRSIZE_DCBMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x00000);
    G2S_BG2Mosaic(FALSE);
}

// The main engine renders to the bottom screen and the capture goes to VRAM C, while the sub engine shows the capture
// in VRAM D on the top screen
static void GFL_G3DDual3DSetupBottomScreen(void) {
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    GX_ResetBankForSubBG();
    GX_SetBankForSubOBJ(GX_VRAM_SUB_OBJ_128_D);
    GX_SetBankForLCDC(GX_VRAM_C);
    GX_SetCapture(GX_CAPTURE_SIZE_256x192, GX_CAPTURE_MODE_A, GX_CAPTURE_SRCA_2D3D, GX_CAPTURE_SRCB_VRAM_0x00000,
                  GX_CAPTURE_DEST_VRAM_C_0x00000, 16, 0);
    GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_3D);
    GXS_SetGraphicsMode(GX_BGMODE_5);
    GXS_SetVisiblePlane(GX_PLANEMASK_OBJ);
}
