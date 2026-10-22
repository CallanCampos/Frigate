// NitroSDK libgx's gx_vramcnt.c: giving the VRAM banks their uses (BG, OBJ, extended palettes, textures, the rear plane
// image, the ARM7's WRAM) and taking them back. The file has no strings; its name is the SDK's, from the layout of
// libgx. Each use keeps the banks it has in gGXState (gx_state.c), and a bank that has no use is in LCDC mode, where
// the CPU sees it. The three statics are NitroSDK's GX_VRAMCNT_SetLCDC_, which puts banks in LCDC mode, and the
// resetBankForX_ and disableBankForX_ of the Reset and Disable functions.
#include "nitro/gx.h"
#include "nitro/os.h"

// Puts the banks of a mask in LCDC mode
// swan's gfxSetLCDCBanksCore
static void GX_VRAMCNT_SetLCDC_(u32 banks) {
    if (banks & GX_VRAM_A) {
        reg_GX_VRAMCNT_A = GX_VRAMCNT_LCDC;
    }
    if (banks & GX_VRAM_B) {
        reg_GX_VRAMCNT_B = GX_VRAMCNT_LCDC;
    }
    if (banks & GX_VRAM_C) {
        reg_GX_VRAMCNT_C = GX_VRAMCNT_LCDC;
    }
    if (banks & GX_VRAM_D) {
        reg_GX_VRAMCNT_D = GX_VRAMCNT_LCDC;
    }
    if (banks & GX_VRAM_E) {
        reg_GX_VRAMCNT_E = GX_VRAMCNT_LCDC;
    }
    if (banks & GX_VRAM_F) {
        reg_GX_VRAMCNT_F = GX_VRAMCNT_LCDC;
    }
    if (banks & GX_VRAM_G) {
        reg_GX_VRAMCNT_G = GX_VRAMCNT_LCDC;
    }
    if (banks & GX_VRAM_H) {
        reg_GX_VRAMCNT_H = GX_VRAMCNT_LCDC;
    }
    if (banks & GX_VRAM_I) {
        reg_GX_VRAMCNT_I = GX_VRAMCNT_LCDC;
    }
}

// A Set function takes the banks of a use: the ones the use had go back to the free banks (gGXState.lcdc), the new
// ones leave them, and each of these is enabled in its VRAMCNT with the master select that tells the use (A and B: 1
// BG, 2 OBJ, 3 texture; C and D: 1 BG, 2 ARM7, 3 texture, 4 sub BG for C and sub OBJ for D; E to G: 1 BG, 2 OBJ, 3
// texture palette, 4 BG extended palette, and 5 OBJ extended palette for F and G; H: 1 sub BG, 2 sub BG extended
// palette; I: 1 sub BG, 2 sub OBJ, 3 sub OBJ extended palette) and with the offset, which is the bank's place among
// the banks of its use. A case of several banks sets the last one and falls into the case of the others
void GX_SetBankForBG(GXVRamBG banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.bg) & ~banks;
    gGXState.bg = banks;
    switch (banks) {
    case GX_VRAM_BG_128_D:
        reg_GX_VRAMCNT_D = GX_VRAMCNT(1, 0);
        break;
    case GX_VRAM_BG_256_CD:
        reg_GX_VRAMCNT_D = GX_VRAMCNT(1, 1);
        // fallthrough
    case GX_VRAM_BG_128_C:
        reg_GX_VRAMCNT_C = GX_VRAMCNT(1, 0);
        break;
    case GX_VRAM_BG_384_BCD:
        reg_GX_VRAMCNT_D = GX_VRAMCNT(1, 2);
        // fallthrough
    case GX_VRAM_BG_256_BC:
        reg_GX_VRAMCNT_C = GX_VRAMCNT(1, 1);
        // fallthrough
    case GX_VRAM_BG_128_B:
        reg_GX_VRAMCNT_B = GX_VRAMCNT(1, 0);
        break;
    case GX_VRAM_BG_512_ABCD:
        reg_GX_VRAMCNT_D = GX_VRAMCNT(1, 3);
        // fallthrough
    case GX_VRAM_BG_384_ABC:
        reg_GX_VRAMCNT_C = GX_VRAMCNT(1, 2);
        // fallthrough
    case GX_VRAM_BG_256_AB:
        reg_GX_VRAMCNT_B = GX_VRAMCNT(1, 1);
        // fallthrough
    case GX_VRAM_BG_128_A:
        reg_GX_VRAMCNT_A = GX_VRAMCNT(1, 0);
        break;
    case GX_VRAM_BG_384_ABD:
        reg_GX_VRAMCNT_A = GX_VRAMCNT(1, 0);
        reg_GX_VRAMCNT_B = GX_VRAMCNT(1, 1);
        reg_GX_VRAMCNT_D = GX_VRAMCNT(1, 2);
        break;
    case GX_VRAM_BG_384_ACD:
        reg_GX_VRAMCNT_D = GX_VRAMCNT(1, 2);
        // fallthrough
    case GX_VRAM_BG_256_AC:
        reg_GX_VRAMCNT_A = GX_VRAMCNT(1, 0);
        reg_GX_VRAMCNT_C = GX_VRAMCNT(1, 1);
        break;
    case GX_VRAM_BG_256_AD:
        reg_GX_VRAMCNT_A = GX_VRAMCNT(1, 0);
        reg_GX_VRAMCNT_D = GX_VRAMCNT(1, 1);
        break;
    case GX_VRAM_BG_256_BD:
        reg_GX_VRAMCNT_B = GX_VRAMCNT(1, 0);
        reg_GX_VRAMCNT_D = GX_VRAMCNT(1, 1);
        break;
    case GX_VRAM_BG_96_EFG:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(1, 3);
        // fallthrough
    case GX_VRAM_BG_80_EF:
        reg_GX_VRAMCNT_F = GX_VRAMCNT(1, 2);
        // fallthrough
    case GX_VRAM_BG_64_E:
        reg_GX_VRAMCNT_E = GX_VRAMCNT(1, 0);
        break;
    case GX_VRAM_BG_80_EG:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(1, 2);
        reg_GX_VRAMCNT_E = GX_VRAMCNT(1, 0);
        break;
    case GX_VRAM_BG_32_FG:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(1, 1);
        // fallthrough
    case GX_VRAM_BG_16_F:
        reg_GX_VRAMCNT_F = GX_VRAMCNT(1, 0);
        break;
    case GX_VRAM_BG_16_G:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(1, 0);
        break;
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

void GX_SetBankForOBJ(GXVRamOBJ banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.obj) & ~banks;
    gGXState.obj = banks;
    switch (banks) {
    case GX_VRAM_OBJ_256_AB:
        reg_GX_VRAMCNT_B = GX_VRAMCNT(2, 1);
        // fallthrough
    case GX_VRAM_OBJ_128_A:
        reg_GX_VRAMCNT_A = GX_VRAMCNT(2, 0);
        break;
    case GX_VRAM_OBJ_128_B:
        reg_GX_VRAMCNT_B = GX_VRAMCNT(2, 0);
        break;
    case GX_VRAM_OBJ_96_EFG:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(2, 3);
        // fallthrough
    case GX_VRAM_OBJ_80_EF:
        reg_GX_VRAMCNT_F = GX_VRAMCNT(2, 2);
        // fallthrough
    case GX_VRAM_OBJ_64_E:
        reg_GX_VRAMCNT_E = GX_VRAMCNT(2, 0);
        break;
    case GX_VRAM_OBJ_80_EG:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(2, 2);
        reg_GX_VRAMCNT_E = GX_VRAMCNT(2, 0);
        break;
    case GX_VRAM_OBJ_32_FG:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(2, 1);
        // fallthrough
    case GX_VRAM_OBJ_16_F:
        reg_GX_VRAMCNT_F = GX_VRAMCNT(2, 0);
        break;
    case GX_VRAM_OBJ_16_G:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(2, 0);
        break;
    case GX_VRAM_OBJ_NONE:
        break;
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

void GX_SetBankForBGExtPltt(GXVRamBGExtPltt banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.bgExtPltt) & ~banks;
    gGXState.bgExtPltt = banks;
    switch (banks) {
    case GX_VRAM_BGEXTPLTT_0123_E:
        reg_GX_DISPCNT |= REG_GX_DISPCNT_BGEXTPLTT_MASK;
        reg_GX_VRAMCNT_E = GX_VRAMCNT(4, 0);
        break;
    case GX_VRAM_BGEXTPLTT_23_G:
        reg_GX_DISPCNT |= REG_GX_DISPCNT_BGEXTPLTT_MASK;
        reg_GX_VRAMCNT_G = GX_VRAMCNT(4, 1);
        break;
    case GX_VRAM_BGEXTPLTT_0123_FG:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(4, 1);
        // fallthrough
    case GX_VRAM_BGEXTPLTT_01_F:
        reg_GX_VRAMCNT_F = GX_VRAMCNT(4, 0);
        reg_GX_DISPCNT |= REG_GX_DISPCNT_BGEXTPLTT_MASK;
        break;
    case GX_VRAM_BGEXTPLTT_NONE:
        reg_GX_DISPCNT &= ~REG_GX_DISPCNT_BGEXTPLTT_MASK;
        break;
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

void GX_SetBankForOBJExtPltt(GXVRamOBJExtPltt banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.objExtPltt) & ~banks;
    gGXState.objExtPltt = banks;
    switch (banks) {
    case GX_VRAM_OBJEXTPLTT_0_F:
        reg_GX_DISPCNT |= REG_GX_DISPCNT_OBJEXTPLTT_MASK;
        reg_GX_VRAMCNT_F = GX_VRAMCNT(5, 0);
        break;
    case GX_VRAM_OBJEXTPLTT_0_G:
        reg_GX_DISPCNT |= REG_GX_DISPCNT_OBJEXTPLTT_MASK;
        reg_GX_VRAMCNT_G = GX_VRAMCNT(5, 0);
        break;
    case GX_VRAM_OBJEXTPLTT_NONE:
        reg_GX_DISPCNT &= ~REG_GX_DISPCNT_OBJEXTPLTT_MASK;
        break;
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

void GX_SetBankForTex(GXVRamTex banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.tex) & ~banks;
    gGXState.tex = banks;
    if (banks == GX_VRAM_TEX_NONE) {
        reg_G3X_DISP3DCNT &= ~(REG_G3X_DISP3DCNT_TME_MASK | REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK);
    } else {
        reg_G3X_DISP3DCNT =
            (reg_G3X_DISP3DCNT & ~(REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK)) | REG_G3X_DISP3DCNT_TME_MASK;
        switch (banks) {
        case GX_VRAM_TEX_01_AC:
            reg_GX_VRAMCNT_A = GX_VRAMCNT(3, 0);
            reg_GX_VRAMCNT_C = GX_VRAMCNT(3, 1);
            break;
        case GX_VRAM_TEX_01_AD:
            reg_GX_VRAMCNT_A = GX_VRAMCNT(3, 0);
            reg_GX_VRAMCNT_D = GX_VRAMCNT(3, 1);
            break;
        case GX_VRAM_TEX_01_BD:
            reg_GX_VRAMCNT_B = GX_VRAMCNT(3, 0);
            reg_GX_VRAMCNT_D = GX_VRAMCNT(3, 1);
            break;
        case GX_VRAM_TEX_012_ABD:
            reg_GX_VRAMCNT_A = GX_VRAMCNT(3, 0);
            reg_GX_VRAMCNT_B = GX_VRAMCNT(3, 1);
            reg_GX_VRAMCNT_D = GX_VRAMCNT(3, 2);
            break;
        case GX_VRAM_TEX_012_ACD:
            reg_GX_VRAMCNT_A = GX_VRAMCNT(3, 0);
            reg_GX_VRAMCNT_C = GX_VRAMCNT(3, 1);
            reg_GX_VRAMCNT_D = GX_VRAMCNT(3, 2);
            break;
        case GX_VRAM_TEX_0_D:
            reg_GX_VRAMCNT_D = GX_VRAMCNT(3, 0);
            break;
        case GX_VRAM_TEX_01_CD:
            reg_GX_VRAMCNT_D = GX_VRAMCNT(3, 1);
            // fallthrough
        case GX_VRAM_TEX_0_C:
            reg_GX_VRAMCNT_C = GX_VRAMCNT(3, 0);
            break;
        case GX_VRAM_TEX_012_BCD:
            reg_GX_VRAMCNT_D = GX_VRAMCNT(3, 2);
            // fallthrough
        case GX_VRAM_TEX_01_BC:
            reg_GX_VRAMCNT_C = GX_VRAMCNT(3, 1);
            // fallthrough
        case GX_VRAM_TEX_0_B:
            reg_GX_VRAMCNT_B = GX_VRAMCNT(3, 0);
            break;
        case GX_VRAM_TEX_0123_ABCD:
            reg_GX_VRAMCNT_D = GX_VRAMCNT(3, 3);
            // fallthrough
        case GX_VRAM_TEX_012_ABC:
            reg_GX_VRAMCNT_C = GX_VRAMCNT(3, 2);
            // fallthrough
        case GX_VRAM_TEX_01_AB:
            reg_GX_VRAMCNT_B = GX_VRAMCNT(3, 1);
            // fallthrough
        case GX_VRAM_TEX_0_A:
            reg_GX_VRAMCNT_A = GX_VRAMCNT(3, 0);
            break;
        }
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

void GX_SetBankForTexPltt(GXVRamTexPltt banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.texPltt) & ~banks;
    gGXState.texPltt = banks;
    switch (banks) {
    case GX_VRAM_TEXPLTT_01_FG:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(3, 1);
        // fallthrough
    case GX_VRAM_TEXPLTT_0_F:
        reg_GX_VRAMCNT_F = GX_VRAMCNT(3, 0);
        break;
    case GX_VRAM_TEXPLTT_0_G:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(3, 0);
        break;
    case GX_VRAM_TEXPLTT_012345_EFG:
        reg_GX_VRAMCNT_G = GX_VRAMCNT(3, 3);
        // fallthrough
    case GX_VRAM_TEXPLTT_01234_EF:
        reg_GX_VRAMCNT_F = GX_VRAMCNT(3, 2);
        // fallthrough
    case GX_VRAM_TEXPLTT_0123_E:
        reg_GX_VRAMCNT_E = GX_VRAMCNT(3, 0);
        break;
    case GX_VRAM_TEXPLTT_NONE:
        break;
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

void GX_SetBankForClearImage(GXVRamClearImage banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.clrImg) & ~banks;
    gGXState.clrImg = banks;
    switch (banks) {
    case GX_VRAM_CLEARIMAGE_256_AB:
        reg_GX_VRAMCNT_A = GX_VRAMCNT(3, 2);
        // fallthrough
    case GX_VRAM_CLEARIMAGE_128_B:
        reg_GX_VRAMCNT_B = GX_VRAMCNT(3, 3);
        reg_G3X_DISP3DCNT |= REG_G3X_DISP3DCNT_CLRIMG_MASK;
        break;
    case GX_VRAM_CLEARIMAGE_256_CD:
        reg_GX_VRAMCNT_C = GX_VRAMCNT(3, 2);
        // fallthrough
    case GX_VRAM_CLEARIMAGE_128_D:
        reg_GX_VRAMCNT_D = GX_VRAMCNT(3, 3);
        reg_G3X_DISP3DCNT |= REG_G3X_DISP3DCNT_CLRIMG_MASK;
        break;
    case GX_VRAM_CLEARIMAGE_NONE:
        reg_G3X_DISP3DCNT &= ~REG_G3X_DISP3DCNT_CLRIMG_MASK;
        break;
    case GX_VRAM_CLEARIMAGE_128_A:
        reg_GX_VRAMCNT_A = GX_VRAMCNT(3, 3);
        reg_G3X_DISP3DCNT |= REG_G3X_DISP3DCNT_CLRIMG_MASK;
        break;
    case GX_VRAM_CLEARIMAGE_128_C:
        reg_GX_VRAMCNT_C = GX_VRAMCNT(3, 3);
        reg_G3X_DISP3DCNT |= REG_G3X_DISP3DCNT_CLRIMG_MASK;
        break;
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

void GX_SetBankForARM7(GXVRamARM7 banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.arm7) & ~banks;
    gGXState.arm7 = banks;
    switch (banks) {
    case GX_VRAM_ARM7_256_CD:
        reg_GX_VRAMCNT_D = GX_VRAMCNT(2, 1);
        reg_GX_VRAMCNT_C = GX_VRAMCNT(2, 0);
        break;
    case GX_VRAM_ARM7_128_C:
        reg_GX_VRAMCNT_C = GX_VRAMCNT(2, 0);
        break;
    case GX_VRAM_ARM7_128_D:
        reg_GX_VRAMCNT_D = GX_VRAMCNT(2, 0);
        break;
    case GX_VRAM_ARM7_NONE:
        break;
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

void GX_SetBankForLCDC(int banks) {
    gGXState.lcdc |= banks;
    GX_VRAMCNT_SetLCDC_(banks);
}

void GX_SetBankForSubBG(GXVRamSubBG banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.subBg) & ~banks;
    gGXState.subBg = banks;
    switch (banks) {
    case GX_VRAM_SUB_BG_128_C:
        reg_GX_VRAMCNT_C = GX_VRAMCNT(4, 0);
        break;
    case GX_VRAM_SUB_BG_48_HI:
        reg_GX_VRAMCNT_I = GX_VRAMCNT(1, 0);
        // fallthrough
    case GX_VRAM_SUB_BG_32_H:
        reg_GX_VRAMCNT_H = GX_VRAMCNT(1, 0);
        break;
    case GX_VRAM_SUB_BG_NONE:
        break;
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

void GX_SetBankForSubOBJ(GXVRamSubOBJ banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.subObj) & ~banks;
    gGXState.subObj = banks;
    switch (banks) {
    case GX_VRAM_SUB_OBJ_128_D:
        reg_GX_VRAMCNT_D = GX_VRAMCNT(4, 0);
        break;
    case GX_VRAM_SUB_OBJ_16_I:
        reg_GX_VRAMCNT_I = GX_VRAMCNT(2, 0);
        break;
    case GX_VRAM_SUB_OBJ_NONE:
        break;
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

void GX_SetBankForSubBGExtPltt(GXVRamSubBGExtPltt banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.subBgExtPltt) & ~banks;
    gGXState.subBgExtPltt = banks;
    switch (banks) {
    case GX_VRAM_SUB_BGEXTPLTT_0123_H:
        reg_GXS_DB_DISPCNT |= REG_GX_DISPCNT_BGEXTPLTT_MASK;
        reg_GX_VRAMCNT_H = GX_VRAMCNT(2, 0);
        break;
    case GX_VRAM_SUB_BGEXTPLTT_NONE:
        reg_GXS_DB_DISPCNT &= ~REG_GX_DISPCNT_BGEXTPLTT_MASK;
        break;
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

void GX_SetBankForSubOBJExtPltt(GXVRamSubOBJExtPltt banks) {
    gGXState.lcdc = (gGXState.lcdc | gGXState.subObjExtPltt) & ~banks;
    gGXState.subObjExtPltt = banks;
    switch (banks) {
    case GX_VRAM_SUB_OBJEXTPLTT_0_I:
        reg_GXS_DB_DISPCNT |= REG_GX_DISPCNT_OBJEXTPLTT_MASK;
        reg_GX_VRAMCNT_I = GX_VRAMCNT(3, 0);
        break;
    case GX_VRAM_SUB_OBJEXTPLTT_NONE:
        reg_GXS_DB_DISPCNT &= ~REG_GX_DISPCNT_OBJEXTPLTT_MASK;
        break;
    }
    GX_VRAMCNT_SetLCDC_(gGXState.lcdc);
}

// Takes the banks that a use has, which go to LCDC mode, and returns them
// swan's gfxAcquireBanksForLCDC
static u32 resetBankForX_(u16 *banks) {
    u32 acquired = *banks;
    *banks = 0;
    gGXState.lcdc |= acquired;
    GX_VRAMCNT_SetLCDC_(acquired);
    return acquired;
}

GXVRamBG GX_ResetBankForBG(void) {
    return resetBankForX_(&gGXState.bg);
}

GXVRamOBJ GX_ResetBankForOBJ(void) {
    return resetBankForX_(&gGXState.obj);
}

GXVRamBGExtPltt GX_ResetBankForBGExtPltt(void) {
    reg_GX_DISPCNT &= ~REG_GX_DISPCNT_BGEXTPLTT_MASK;
    return (GXVRamBGExtPltt)resetBankForX_(&gGXState.bgExtPltt);
}

GXVRamOBJExtPltt GX_ResetBankForOBJExtPltt(void) {
    reg_GX_DISPCNT &= ~REG_GX_DISPCNT_OBJEXTPLTT_MASK;
    return (GXVRamOBJExtPltt)resetBankForX_(&gGXState.objExtPltt);
}

GXVRamTex GX_ResetBankForTex(void) {
    return resetBankForX_(&gGXState.tex);
}

GXVRamTexPltt GX_ResetBankForTexPltt(void) {
    return (GXVRamTexPltt)resetBankForX_(&gGXState.texPltt);
}

GXVRamClearImage GX_ResetBankForClearImage(void) {
    return resetBankForX_(&gGXState.clrImg);
}

GXVRamSubBG GX_ResetBankForSubBG(void) {
    return resetBankForX_(&gGXState.subBg);
}

GXVRamSubOBJ GX_ResetBankForSubOBJ(void) {
    return resetBankForX_(&gGXState.subObj);
}

GXVRamSubBGExtPltt GX_ResetBankForSubBGExtPltt(void) {
    reg_GXS_DB_DISPCNT &= ~REG_GX_DISPCNT_BGEXTPLTT_MASK;
    return resetBankForX_(&gGXState.subBgExtPltt);
}

GXVRamSubOBJExtPltt GX_ResetBankForSubOBJExtPltt(void) {
    reg_GXS_DB_DISPCNT &= ~REG_GX_DISPCNT_OBJEXTPLTT_MASK;
    return resetBankForX_(&gGXState.subObjExtPltt);
}

// Turns off the banks that a use has, and unlocks them for the lock ID that GX_Init took
// swan's gfxDisableQueuedBanks
static u32 disableBankForX_(u16 *banks) {
    u32 disabled = *banks;
    *banks = 0;
    if (disabled & GX_VRAM_A) {
        reg_GX_VRAMCNT_A = 0;
    }
    if (disabled & GX_VRAM_B) {
        reg_GX_VRAMCNT_B = 0;
    }
    if (disabled & GX_VRAM_C) {
        reg_GX_VRAMCNT_C = 0;
    }
    if (disabled & GX_VRAM_D) {
        reg_GX_VRAMCNT_D = 0;
    }
    if (disabled & GX_VRAM_E) {
        reg_GX_VRAMCNT_E = 0;
    }
    if (disabled & GX_VRAM_F) {
        reg_GX_VRAMCNT_F = 0;
    }
    if (disabled & GX_VRAM_G) {
        reg_GX_VRAMCNT_G = 0;
    }
    if (disabled & GX_VRAM_H) {
        reg_GX_VRAMCNT_H = 0;
    }
    if (disabled & GX_VRAM_I) {
        reg_GX_VRAMCNT_I = 0;
    }
    OSi_UnlockVram(disabled, GXi_VRamLockId);
    return disabled;
}

GXVRamBG GX_DisableBankForBG(void) {
    return disableBankForX_(&gGXState.bg);
}

GXVRamOBJ GX_DisableBankForOBJ(void) {
    return disableBankForX_(&gGXState.obj);
}

GXVRamBGExtPltt GX_DisableBankForBGExtPltt(void) {
    reg_GX_DISPCNT &= ~REG_GX_DISPCNT_BGEXTPLTT_MASK;
    return disableBankForX_(&gGXState.bgExtPltt);
}

GXVRamOBJExtPltt GX_DisableBankForOBJExtPltt(void) {
    reg_GX_DISPCNT &= ~REG_GX_DISPCNT_OBJEXTPLTT_MASK;
    return disableBankForX_(&gGXState.objExtPltt);
}

GXVRamTex GX_DisableBankForTex(void) {
    return disableBankForX_(&gGXState.tex);
}

GXVRamTexPltt GX_DisableBankForTexPltt(void) {
    return disableBankForX_(&gGXState.texPltt);
}

GXVRamClearImage GX_DisableBankForClearImage(void) {
    return disableBankForX_(&gGXState.clrImg);
}

GXVRamARM7 GX_DisableBankForARM7(void) {
    return disableBankForX_(&gGXState.arm7);
}

GXVRamLCDC GX_DisableBankForLCDC(void) {
    return disableBankForX_(&gGXState.lcdc);
}

GXVRamSubBG GX_DisableBankForSubBG(void) {
    return disableBankForX_(&gGXState.subBg);
}

GXVRamSubOBJ GX_DisableBankForSubOBJ(void) {
    return disableBankForX_(&gGXState.subObj);
}

GXVRamSubBGExtPltt GX_DisableBankForSubBGExtPltt(void) {
    reg_GXS_DB_DISPCNT &= ~REG_GX_DISPCNT_BGEXTPLTT_MASK;
    return disableBankForX_(&gGXState.subBgExtPltt);
}

GXVRamSubOBJExtPltt GX_DisableBankForSubOBJExtPltt(void) {
    reg_GXS_DB_DISPCNT &= ~REG_GX_DISPCNT_OBJEXTPLTT_MASK;
    return disableBankForX_(&gGXState.subObjExtPltt);
}

GXVRamOBJ GX_GetBankForOBJ(void) {
    return gGXState.obj;
}

GXVRamOBJExtPltt GX_GetBankForOBJExtPltt(void) {
    return gGXState.objExtPltt;
}

GXVRamTex GX_GetBankForTex(void) {
    return gGXState.tex;
}

GXVRamLCDC GX_GetBankForLCDC(void) {
    return gGXState.lcdc;
}

GXVRamSubOBJ GX_GetBankForSubOBJ(void) {
    return gGXState.subObj;
}

GXVRamSubOBJExtPltt GX_GetBankForSubOBJExtPltt(void) {
    return gGXState.subObjExtPltt;
}
