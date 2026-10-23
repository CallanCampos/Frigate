// NitroSDK libgx's state of the VRAM banks, which gx_vramcnt.c updates, and its initialization. The SDK's file is
// gxstate.c, as ntrtwl's reconstruction of NitroSDK has it.
#include "nitro/gx.h"

GXState gGXState;

void GX_InitGXState(void) {
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
    reg_GX_VRAMCNT = 0;
    reg_GX_VRAMCNT_E = 0;
    reg_GX_VRAMCNT_F = 0;
    reg_GX_VRAMCNT_G = 0;
    reg_GX_VRAM_HI_CNT = 0;
}
