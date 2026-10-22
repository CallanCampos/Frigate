// NitroSDK libgx's gx_bgcnt.c: where each BG of each engine keeps its screen and its characters in VRAM, from the
// BG's control register and, for the main engine, the offsets in DISPCNT. The name is NitroSDK's, from the layout of
// libgx. BG 2 and 3 can be bitmaps (their color mode bit set in the modes that allow it): the "screen" is then the
// bitmap, at a base in units of 16 KB and without the main engine's offset, and there are no characters. In the
// modes where a BG does not exist the functions return NULL. The main engine's screen functions of BG 2 and 3 are
// ARM code in this Thumb file.
#include "nitro/gx.h"

void *G2_GetBG0ScrPtr(void) {
    return (void *)(((reg_G2_BG0CNT & REG_G2_BG0CNT_SCREENBASE_MASK) >> REG_G2_BG0CNT_SCREENBASE_SHIFT) * 0x800 +
                    (HW_BG_VRAM +
                     ((reg_GX_DISPCNT & REG_GX_DISPCNT_BGSCREENOFFSET_MASK) >> REG_GX_DISPCNT_BGSCREENOFFSET_SHIFT) *
                         0x10000));
}

void *G2S_GetBG0ScrPtr(void) {
    return (void *)(HW_DB_BG_VRAM +
                    ((reg_G2S_DB_BG0CNT & REG_G2_BG0CNT_SCREENBASE_MASK) >> REG_G2_BG0CNT_SCREENBASE_SHIFT) * 0x800);
}

void *G2_GetBG1ScrPtr(void) {
    return (void *)(((reg_G2_BG1CNT & REG_G2_BG0CNT_SCREENBASE_MASK) >> REG_G2_BG0CNT_SCREENBASE_SHIFT) * 0x800 +
                    (HW_BG_VRAM +
                     ((reg_GX_DISPCNT & REG_GX_DISPCNT_BGSCREENOFFSET_MASK) >> REG_GX_DISPCNT_BGSCREENOFFSET_SHIFT) *
                         0x10000));
}

void *G2S_GetBG1ScrPtr(void) {
    return (void *)(HW_DB_BG_VRAM +
                    ((reg_G2S_DB_BG1CNT & REG_G2_BG0CNT_SCREENBASE_MASK) >> REG_G2_BG0CNT_SCREENBASE_SHIFT) * 0x800);
}

#pragma thumb off
void *G2_GetBG2ScrPtr(void) {
    int mode = reg_GX_DISPCNT & REG_GX_DISPCNT_BGMODE_MASK;
    u32 bgcnt = reg_G2_BG2CNT;
    u32 offset = ((reg_GX_DISPCNT & REG_GX_DISPCNT_BGSCREENOFFSET_MASK) >> REG_GX_DISPCNT_BGSCREENOFFSET_SHIFT) << 16;
    u32 screenBase = (bgcnt & REG_G2_BG0CNT_SCREENBASE_MASK) >> REG_G2_BG0CNT_SCREENBASE_SHIFT;

    switch (mode) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        return (void *)(HW_BG_VRAM + offset + screenBase * 0x800);
    case 5:
        if (bgcnt & REG_G2_BG0CNT_COLORMODE_MASK) {
            return (void *)(HW_BG_VRAM + screenBase * 0x4000);
        }
        return (void *)(HW_BG_VRAM + offset + screenBase * 0x800);
    case 6:
        return (void *)HW_BG_VRAM;
    default:
        return NULL;
    }
}
#pragma thumb on

void *G2S_GetBG2ScrPtr(void) {
    int mode = reg_GXS_DB_DISPCNT & REG_GXS_DB_DISPCNT_BGMODE_MASK;
    u32 bgcnt = reg_G2S_DB_BG2CNT;
    u32 screenBase = (bgcnt & REG_G2_BG0CNT_SCREENBASE_MASK) >> REG_G2_BG0CNT_SCREENBASE_SHIFT;

    switch (mode) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        return (void *)(HW_DB_BG_VRAM + screenBase * 0x800);
    case 5:
        if (bgcnt & REG_G2_BG0CNT_COLORMODE_MASK) {
            return (void *)(HW_DB_BG_VRAM + screenBase * 0x4000);
        }
        return (void *)(HW_DB_BG_VRAM + screenBase * 0x800);
    case 6:
        return NULL;
    default:
        return NULL;
    }
}

#pragma thumb off
void *G2_GetBG3ScrPtr(void) {
    int mode = reg_GX_DISPCNT & REG_GX_DISPCNT_BGMODE_MASK;
    u32 bgcnt = reg_G2_BG3CNT;
    u32 offset = ((reg_GX_DISPCNT & REG_GX_DISPCNT_BGSCREENOFFSET_MASK) >> REG_GX_DISPCNT_BGSCREENOFFSET_SHIFT) << 16;
    u32 screenBase = (bgcnt & REG_G2_BG0CNT_SCREENBASE_MASK) >> REG_G2_BG0CNT_SCREENBASE_SHIFT;

    switch (mode) {
    case 0:
    case 1:
    case 2:
        return (void *)(HW_BG_VRAM + offset + screenBase * 0x800);
    case 3:
    case 4:
    case 5:
        if (bgcnt & REG_G2_BG0CNT_COLORMODE_MASK) {
            return (void *)(HW_BG_VRAM + screenBase * 0x4000);
        }
        return (void *)(HW_BG_VRAM + offset + screenBase * 0x800);
    case 6:
        return NULL;
    default:
        return NULL;
    }
}
#pragma thumb on

void *G2S_GetBG3ScrPtr(void) {
    int mode = reg_GXS_DB_DISPCNT & REG_GXS_DB_DISPCNT_BGMODE_MASK;
    u32 bgcnt = reg_G2S_DB_BG3CNT;
    u32 screenBase = (bgcnt & REG_G2_BG0CNT_SCREENBASE_MASK) >> REG_G2_BG0CNT_SCREENBASE_SHIFT;

    switch (mode) {
    case 0:
    case 1:
    case 2:
        return (void *)(HW_DB_BG_VRAM + screenBase * 0x800);
    case 3:
    case 4:
    case 5:
        if (bgcnt & REG_G2_BG0CNT_COLORMODE_MASK) {
            return (void *)(HW_DB_BG_VRAM + screenBase * 0x4000);
        }
        return (void *)(HW_DB_BG_VRAM + screenBase * 0x800);
    case 6:
        return NULL;
    default:
        return NULL;
    }
}

void *G2_GetBG0CharPtr(void) {
    return (
        void *)(((reg_G2_BG0CNT & REG_G2_BG0CNT_CHARBASE_MASK) >> REG_G2_BG0CNT_CHARBASE_SHIFT) * 0x4000 +
                (HW_BG_VRAM +
                 ((reg_GX_DISPCNT & REG_GX_DISPCNT_BGCHAROFFSET_MASK) >> REG_GX_DISPCNT_BGCHAROFFSET_SHIFT) * 0x10000));
}

void *G2S_GetBG0CharPtr(void) {
    return (void *)(HW_DB_BG_VRAM +
                    ((reg_G2S_DB_BG0CNT & REG_G2_BG0CNT_CHARBASE_MASK) >> REG_G2_BG0CNT_CHARBASE_SHIFT) * 0x4000);
}

void *G2_GetBG1CharPtr(void) {
    return (
        void *)(((reg_G2_BG1CNT & REG_G2_BG0CNT_CHARBASE_MASK) >> REG_G2_BG0CNT_CHARBASE_SHIFT) * 0x4000 +
                (HW_BG_VRAM +
                 ((reg_GX_DISPCNT & REG_GX_DISPCNT_BGCHAROFFSET_MASK) >> REG_GX_DISPCNT_BGCHAROFFSET_SHIFT) * 0x10000));
}

void *G2S_GetBG1CharPtr(void) {
    return (void *)(HW_DB_BG_VRAM +
                    ((reg_G2S_DB_BG1CNT & REG_G2_BG0CNT_CHARBASE_MASK) >> REG_G2_BG0CNT_CHARBASE_SHIFT) * 0x4000);
}

void *G2_GetBG2CharPtr(void) {
    int mode = reg_GX_DISPCNT & REG_GX_DISPCNT_BGMODE_MASK;
    u32 bgcnt = reg_G2_BG2CNT;

    if (mode < 5 || !(bgcnt & REG_G2_BG0CNT_COLORMODE_MASK)) {
        return (void *)(HW_BG_VRAM +
                        ((reg_GX_DISPCNT & REG_GX_DISPCNT_BGCHAROFFSET_MASK) >> REG_GX_DISPCNT_BGCHAROFFSET_SHIFT) *
                            0x10000 +
                        ((bgcnt & REG_G2_BG0CNT_CHARBASE_MASK) >> REG_G2_BG0CNT_CHARBASE_SHIFT) * 0x4000);
    }
    return NULL;
}

void *G2S_GetBG2CharPtr(void) {
    int mode = reg_GXS_DB_DISPCNT & REG_GXS_DB_DISPCNT_BGMODE_MASK;
    u32 bgcnt = reg_G2S_DB_BG2CNT;

    if (mode < 5 || !(bgcnt & REG_G2_BG0CNT_COLORMODE_MASK)) {
        return (void *)(HW_DB_BG_VRAM +
                        ((bgcnt & REG_G2_BG0CNT_CHARBASE_MASK) >> REG_G2_BG0CNT_CHARBASE_SHIFT) * 0x4000);
    }
    return NULL;
}

void *G2_GetBG3CharPtr(void) {
    int mode = reg_GX_DISPCNT & REG_GX_DISPCNT_BGMODE_MASK;
    u32 bgcnt = reg_G2_BG3CNT;

    if (mode < 3 || (mode < 6 && !(bgcnt & REG_G2_BG0CNT_COLORMODE_MASK))) {
        return (void *)(HW_BG_VRAM +
                        ((reg_GX_DISPCNT & REG_GX_DISPCNT_BGCHAROFFSET_MASK) >> REG_GX_DISPCNT_BGCHAROFFSET_SHIFT) *
                            0x10000 +
                        ((bgcnt & REG_G2_BG0CNT_CHARBASE_MASK) >> REG_G2_BG0CNT_CHARBASE_SHIFT) * 0x4000);
    }
    return NULL;
}

void *G2S_GetBG3CharPtr(void) {
    int mode = reg_GXS_DB_DISPCNT & REG_GXS_DB_DISPCNT_BGMODE_MASK;
    u32 bgcnt = reg_G2S_DB_BG3CNT;

    if (mode < 3 || (mode < 6 && !(bgcnt & REG_G2_BG0CNT_COLORMODE_MASK))) {
        return (void *)(HW_DB_BG_VRAM +
                        ((bgcnt & REG_G2_BG0CNT_CHARBASE_MASK) >> REG_G2_BG0CNT_CHARBASE_SHIFT) * 0x4000);
    }
    return NULL;
}
