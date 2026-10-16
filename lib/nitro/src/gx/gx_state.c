// NitroSDK libgx's gx_state.c: the state that libgx keeps of the VRAM banks, which gx_vramcnt.c updates, and its
// initialization. The file has no strings; its name is the SDK's, from the layout of libgx.
#include "nitro/gx.h"

GXState gGXState;

void gfxInitVRAM(void) {
    gGXState.lcdc = 0;
    gGXState.bg = 0;
    gGXState.obj = 0;
    gGXState.arm7 = 0;
    gGXState.tex = 0;
    gGXState.texPltt = 0;
    gGXState.clrImg = 0;
    gGXState.bgExtPltt = 0;
    gGXState.objExtPltt = 0;
    gGXState.subBg = 0;
    gGXState.subObj = 0;
    gGXState.subBgExtPltt = 0;
    gGXState.subObjExtPltt = 0;
    reg_GX_VRAMCNT_ABCD = 0;
    reg_GX_VRAMCNT_E = 0;
    reg_GX_VRAMCNT_F = 0;
    reg_GX_VRAMCNT_G = 0;
    reg_GX_VRAMCNT_HI = 0;
}
