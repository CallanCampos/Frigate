#include "sub.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/pm.h"

// The game's VRAM banks
typedef struct DWCUtilBanks {
    u32 bg;
    u32 obj;
    u32 bgExtPltt;
    u32 objExtPltt;
    u32 tex;
    u32 texPltt;
    u32 clearImage;
    u32 subBg;
    u32 subObj;
    u32 subBgExtPltt;
    u32 subObjExtPltt;
    u32 arm7;
    u32 lcdc;
} DWCUtilBanks;

static DWCUtilBanks sBanks;

void DWCi_ClearVram(void) {
    sBanks.bg = GX_DisableBankForBG();
    sBanks.obj = GX_DisableBankForOBJ();
    sBanks.bgExtPltt = GX_DisableBankForBGExtPltt();
    sBanks.objExtPltt = GX_DisableBankForOBJExtPltt();
    sBanks.tex = GX_DisableBankForTex();
    sBanks.texPltt = GX_DisableBankForTexPltt();
    sBanks.clearImage = GX_DisableBankForClearImage();
    sBanks.subBg = GX_DisableBankForSubBG();
    sBanks.subObj = GX_DisableBankForSubOBJ();
    sBanks.subBgExtPltt = GX_DisableBankForSubBGExtPltt();
    sBanks.subObjExtPltt = GX_DisableBankForSubOBJExtPltt();
    sBanks.arm7 = GX_DisableBankForARM7();
    sBanks.lcdc = GX_DisableBankForLCDC();
    GX_SetBankForARM7(sBanks.arm7);
    clearVram();
}

void DWCi_RepairVram(void) {
    GX_DisableBankForBG();
    GX_DisableBankForOBJ();
    GX_DisableBankForSubBG();
    GX_DisableBankForSubOBJ();
    clearVram();
    GX_SetBankForBG(sBanks.bg);
    GX_SetBankForOBJ(sBanks.obj);
    GX_SetBankForBGExtPltt(sBanks.bgExtPltt);
    GX_SetBankForOBJExtPltt(sBanks.objExtPltt);
    GX_SetBankForTex(sBanks.tex);
    GX_SetBankForTexPltt(sBanks.texPltt);
    GX_SetBankForClearImage(sBanks.clearImage);
    GX_SetBankForSubBG(sBanks.subBg);
    GX_SetBankForSubOBJ(sBanks.subObj);
    GX_SetBankForSubBGExtPltt(sBanks.subBgExtPltt);
    GX_SetBankForSubOBJExtPltt(sBanks.subObjExtPltt);
    GX_SetBankForLCDC(sBanks.lcdc);
    G2_BlendNone();
    G2S_BlendNone();
    G2_SetBG0Offset(0, 0);
    G2_SetBG1Offset(0, 0);
    G2_SetBG2Offset(0, 0);
    G2_SetBG3Offset(0, 0);
    G2S_SetBG0Offset(0, 0);
    G2S_SetBG1Offset(0, 0);
    G2S_SetBG2Offset(0, 0);
    G2S_SetBG3Offset(0, 0);
}

void clearVram(void) {
    GX_SetBankForLCDC(GX_VRAM_A | GX_VRAM_B | GX_VRAM_E | GX_VRAM_F | GX_VRAM_G | GX_VRAM_H | GX_VRAM_I);
    // Banks A and B, and E to I
    MI_CpuClearFast((void *)HW_LCDC_VRAM_A, 0x40000);
    MI_CpuClearFast((void *)HW_LCDC_VRAM_E, 0x24000);
    GX_DisableBankForLCDC();
    MI_CpuFillFast((void *)HW_OAM, GX_OAM_EFFECT_NODISPLAY, HW_OAM_SIZE);
    MI_CpuClearFast((void *)HW_BG_PLTT, HW_PLTT_SIZE);
    MI_CpuFillFast((void *)HW_DB_OAM, GX_OAM_EFFECT_NODISPLAY, HW_OAM_SIZE);
    MI_CpuClearFast((void *)HW_DB_BG_PLTT, HW_PLTT_SIZE);
}

void DWCi_SetLedWireless(void) {
    u32 value;

    if (PM_GetLEDPattern(&value) == 0 && value != 15) {
        PMi_SendLEDPatternCommand(15);
    }
}

void DWCi_SetLedNormal(void) {
    PMi_SendLEDPatternCommand(1);
}
