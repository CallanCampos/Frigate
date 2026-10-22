// NitroSDK libgx's gx.c: powering up the graphics engines, the display interrupts, the display mode of the main
// engine and the master brightness. The file has no strings; its name is the SDK's, from the layout of libgx. It
// needs `-ipa file` (its data is reached from one base). The linker's veneer pair for the NDMA fill in TwlSDK's LTD
// module sits after GX_Init, its first caller; it is not part of the source.
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/pm.h"

// Whether the main engine's display is on, and the mode it shows when it is turned on again (swan's names)
static u16 sIsDispOn = TRUE;
u32 GXi_DmaId = GX_DEFAULT_DMAID;
u16 GXi_VRamLockId;
static u16 sDispMode;

void GX_Init(void) {
    reg_GX_POWCNT |= REG_GX_POWCNT_DSEL_MASK;
    GX_SetPower(GX_POWER_ALL);
    reg_GX_POWCNT |= REG_GX_POWCNT_LCD_MASK;
    GX_InitGXState();
    while (GXi_VRamLockId == 0) {
        s32 lockId = OS_GetLockID();
        if (lockId == OS_LOCK_ID_ERROR) {
            OS_Terminate();
        }
        GXi_VRamLockId = lockId;
    }
    reg_GX_DISPSTAT = 0;
    reg_GX_DISPCNT = 0;
    // Clears the 2D engines' registers by DMA, by NDMA on the DSi's channels (numbers above 3) or with the CPU. The
    // kind of channel is tested once for both fills
    if (GXi_DmaId != GX_DMA_NOT_USE) {
        if (GXi_DmaId > MI_DMA_MAX_NUM) {
            MI_NDmaFill(GXi_DmaId - 4, (void *)&reg_G2_BG0CNT, 0, 0x60);
            reg_GX_MASTER_BRIGHT = 0;
            MI_NDmaFill(GXi_DmaId - 4, (void *)&reg_GXS_DB_DISPCNT, 0, 0x70);
        } else {
            MIi_DmaFill32(GXi_DmaId, (void *)&reg_G2_BG0CNT, 0, 0x60, TRUE);
            reg_GX_MASTER_BRIGHT = 0;
            MIi_DmaFill32(GXi_DmaId, (void *)&reg_GXS_DB_DISPCNT, 0, 0x70, TRUE);
        }
    } else {
        MI_CpuClear32((void *)&reg_G2_BG0CNT, 0x60);
        reg_GX_MASTER_BRIGHT = 0;
        MI_CpuClear32((void *)&reg_GXS_DB_DISPCNT, 0x70);
    }
    reg_G2_BG2PA = 0x100;
    reg_G2_BG2PD = 0x100;
    reg_G2_BG3PA = 0x100;
    reg_G2_BG3PD = 0x100;
    reg_G2S_DB_BG2PA = 0x100;
    reg_G2S_DB_BG2PD = 0x100;
    reg_G2S_DB_BG3PA = 0x100;
    reg_G2S_DB_BG3PD = 0x100;
}

void GX_SetVCountEqVal(s32 vcount) {
    reg_GX_DISPSTAT =
        (u16)((reg_GX_DISPSTAT & REG_GX_DISPSTAT_FLAGS_MASK) | ((vcount & 0xff) << 8) | ((vcount & 0x100) >> 1));
}

s32 GX_HBlankIntr(BOOL enable) {
    s32 prev = reg_GX_DISPSTAT & REG_GX_DISPSTAT_HBI_MASK;
    if (enable) {
        reg_GX_DISPSTAT |= REG_GX_DISPSTAT_HBI_MASK;
    } else {
        reg_GX_DISPSTAT &= ~REG_GX_DISPSTAT_HBI_MASK;
    }
    return prev;
}

s32 GX_VBlankIntr(BOOL enable) {
    s32 prev = reg_GX_DISPSTAT & REG_GX_DISPSTAT_VBI_MASK;
    if (enable) {
        reg_GX_DISPSTAT |= REG_GX_DISPSTAT_VBI_MASK;
    } else {
        reg_GX_DISPSTAT &= ~REG_GX_DISPSTAT_VBI_MASK;
    }
    return prev;
}

BOOL GX_IsDispOn(void) {
    return sIsDispOn;
}

void GX_DispOff(void) {
    u32 dispcnt = reg_GX_DISPCNT;
    sIsDispOn = FALSE;
    sDispMode = (dispcnt & REG_GX_DISPCNT_MODE_MASK) >> REG_GX_DISPCNT_MODE_SHIFT;
    reg_GX_DISPCNT = dispcnt & ~REG_GX_DISPCNT_MODE_MASK;
    PMi_SetDispOffCount();
}

void GX_DispOn(void) {
    sIsDispOn = TRUE;
    if (sDispMode != 0) {
        reg_GX_DISPCNT = (reg_GX_DISPCNT & ~REG_GX_DISPCNT_MODE_MASK) | (sDispMode << REG_GX_DISPCNT_MODE_SHIFT);
    } else {
        reg_GX_DISPCNT |= GX_DISPMODE_GRAPHICS << REG_GX_DISPCNT_MODE_SHIFT;
    }
}

void GX_SetGraphicsMode(GXDispMode dispMode, GXBGMode bgMode, GXBG0As bg0As3D) {
    u32 dispcnt = reg_GX_DISPCNT;
    sDispMode = dispMode;
    if (!sIsDispOn) {
        dispMode = 0;
    }
    reg_GX_DISPCNT = (dispcnt & ~REG_GX_DISPCNT_GRAPHICS_MASK) | (dispMode << REG_GX_DISPCNT_MODE_SHIFT) | bgMode |
                     (bg0As3D << REG_GX_DISPCNT_BG02D3D_SHIFT);
    if (sDispMode == 0) {
        sIsDispOn = FALSE;
    }
}

void GXS_SetGraphicsMode(GXBGMode bgMode) {
    reg_GXS_DB_DISPCNT = (reg_GXS_DB_DISPCNT & ~REG_GXS_DB_DISPCNT_BGMODE_MASK) | bgMode;
}

void GXx_SetMasterBrightness_(vu16 *reg, int brightness) {
    if (brightness == 0) {
        *reg = 0;
    } else if (brightness > 0) {
        *reg = (u16)(REG_GX_MASTER_BRIGHT_MODE_UP | brightness);
    } else {
        *reg = (u16)(REG_GX_MASTER_BRIGHT_MODE_DOWN | -brightness);
    }
}

int GXx_GetMasterBrightness_(vu16 *reg) {
    u16 mode = *reg & REG_GX_MASTER_BRIGHT_E_MOD_MASK;
    if (mode == 0) {
        return 0;
    } else if (mode == REG_GX_MASTER_BRIGHT_MODE_UP) {
        return *reg & REG_GX_MASTER_BRIGHT_E_VALUE_MASK;
    } else if (mode == REG_GX_MASTER_BRIGHT_MODE_DOWN) {
        return -(*reg & REG_GX_MASTER_BRIGHT_E_VALUE_MASK);
    }
    return 0;
}
