#ifndef POKEBW2_NITRO_GX_H
#define POKEBW2_NITRO_GX_H

#include "types.h"
#include "nitro/fx.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "twl/mi.h"

// The parts of NitroSDK's graphics registers and inline functions that the game's code uses

typedef u16 GXRgb;

#define GX_RGB(r, g, b) ((GXRgb)((r) | ((g) << 5) | ((b) << 10)))
#define GX_RGB_R_SHIFT 0
#define GX_RGB_R_MASK 0x001f
#define GX_RGB_G_SHIFT 5
#define GX_RGB_G_MASK 0x03e0
#define GX_RGB_B_SHIFT 10
#define GX_RGB_B_MASK 0x7c00

#define GX_RGB_R_SHIFT 0
#define GX_RGB_R_MASK 0x001f
#define GX_RGB_G_SHIFT 5
#define GX_RGB_G_MASK 0x03e0
#define GX_RGB_B_SHIFT 10
#define GX_RGB_B_MASK 0x7c00

#define reg_GX_DISPCNT (*(vu32 *)0x04000000)
#define reg_GX_DISPSTAT (*(vu16 *)0x04000004)
#define REG_GX_DISPSTAT_VBLK_MASK 0x0001
#define reg_G2_BG0CNT (*(vu16 *)0x04000008)
#define reg_G2_WIN0H (*(vu16 *)0x04000040)
#define reg_G2_WIN1H (*(vu16 *)0x04000042)
#define reg_G2_WIN0V (*(vu16 *)0x04000044)
#define reg_G2_WIN1V (*(vu16 *)0x04000046)
#define reg_G2_WININ (*(vu16 *)0x04000048)
#define reg_G2_WINOUT (*(vu16 *)0x0400004a)
#define reg_G2_BLDCNT (*(vu16 *)0x04000050)
#define reg_G2_BLDALPHA (*(vu16 *)0x04000052)
#define reg_G3X_DISP3DCNT (*(vu16 *)0x04000060)
#define reg_GX_DISPCAPCNT (*(vu32 *)0x04000064)
#define reg_G3X_GXFIFO (*(vu32 *)0x04000400)
#define reg_G3_MTX_MODE (*(vu32 *)0x04000440)
#define reg_G3_MTX_PUSH (*(vu32 *)0x04000444)
#define reg_G3_MTX_POP (*(vu32 *)0x04000448)
#define reg_G3_MTX_STORE (*(vu32 *)0x0400044c)
#define reg_G3_MTX_RESTORE (*(vu32 *)0x04000450)
#define reg_G3_MTX_IDENTITY (*(vu32 *)0x04000454)
#define reg_G3_MTX_SCALE (*(vu32 *)0x0400046c)
#define reg_G3_MTX_TRANS (*(vu32 *)0x04000470)
#define reg_G3_COLOR (*(vu32 *)0x04000480)
#define reg_G3_NORMAL (*(vu32 *)0x04000484)
#define reg_G3_TEXCOORD (*(vu32 *)0x04000488)
#define reg_G3_VTX_16 (*(vu32 *)0x0400048c)
#define reg_G3_VTX_10 (*(vu32 *)0x04000490)
#define reg_G3_POLYGON_ATTR (*(vu32 *)0x040004a4)
#define reg_G3_TEXIMAGE_PARAM (*(vu32 *)0x040004a8)
#define reg_G3_TEXPLTT_BASE (*(vu32 *)0x040004ac)
#define reg_G3_DIF_AMB (*(vu32 *)0x040004c0)
#define reg_G3_SPE_EMI (*(vu32 *)0x040004c4)
#define reg_G3_LIGHT_VECTOR (*(vu32 *)0x040004c8)
#define reg_G3_LIGHT_COLOR (*(vu32 *)0x040004cc)
#define reg_G3_BEGIN_VTXS (*(vu32 *)0x04000500)
#define reg_G3_END_VTXS (*(vu32 *)0x04000504)
#define reg_G3_SWAP_BUFFERS (*(vu32 *)0x04000540)
#define reg_G3_VIEWPORT (*(vu32 *)0x04000580)
// The counts of polygons in polygon list RAM and of vertices in vertex RAM
#define reg_G3X_RAM_COUNT (*(vu16 *)0x04000604)
#define reg_G3X_VTXRAM_COUNT (*(vu16 *)0x04000606)
#define reg_GXS_DB_DISPCNT (*(vu32 *)0x04001000)
#define reg_G2S_DB_WIN0H (*(vu16 *)0x04001040)
#define reg_G2S_DB_WIN1H (*(vu16 *)0x04001042)
#define reg_G2S_DB_WIN0V (*(vu16 *)0x04001044)
#define reg_G2S_DB_WIN1V (*(vu16 *)0x04001046)
#define reg_G2S_DB_WININ (*(vu16 *)0x04001048)
#define reg_G2S_DB_WINOUT (*(vu16 *)0x0400104a)
#define reg_G2S_DB_BLDCNT (*(vu16 *)0x04001050)
#define reg_G2S_DB_BLDALPHA (*(vu16 *)0x04001052)

#define REG_GX_DISPCNT_W0_SHIFT 13
#define REG_GX_DISPCNT_W0_MASK 0x00002000
#define REG_GX_DISPCNT_W1_MASK 0x00004000
#define REG_GX_DISPCNT_OW_MASK 0x00008000

#define REG_G2_BG0CNT_PRIORITY_SHIFT 0
#define REG_G2_BG0CNT_PRIORITY_MASK 0x0003

#define REG_G3X_DISP3DCNT_THS_SHIFT 1
#define REG_G3X_DISP3DCNT_THS_MASK 0x0002
#define REG_G3X_DISP3DCNT_ATE_MASK 0x0004
#define REG_G3X_DISP3DCNT_ABE_MASK 0x0008
#define REG_G3X_DISP3DCNT_AAE_MASK 0x0010
#define REG_G3X_DISP3DCNT_EME_MASK 0x0020
#define REG_G3X_DISP3DCNT_RO_MASK 0x1000
#define REG_G3X_DISP3DCNT_GO_MASK 0x2000

#define GX_WNDMASK_NONE 0x00
#define GX_WNDMASK_W0 0x01
#define GX_WNDMASK_W1 0x02
#define GX_WNDMASK_OW 0x04

#define GX_WND_PLANEMASK_BG0 0x01
#define GX_WND_PLANEMASK_BG1 0x02
#define GX_WND_PLANEMASK_BG2 0x04
#define GX_WND_PLANEMASK_BG3 0x08
#define GX_WND_PLANEMASK_OBJ 0x10

#define REG_G2S_DB_WININ_WIN0IN_SHIFT 0
#define REG_G2S_DB_WININ_WIN0IN_MASK 0x003f
#define REG_G2S_DB_WININ_WIN1IN_SHIFT 8
#define REG_G2S_DB_WININ_WIN1IN_MASK 0x3f00
#define REG_G2S_DB_WINOUT_WINOUT_SHIFT 0
#define REG_G2S_DB_WINOUT_WINOUT_MASK 0x003f
// A window's effect enable, above its planes
#define GX_WND_EFFECT_SHIFT 5

#define GX_OAM_MODE_NORMAL 0
#define GX_OAM_MODE_XLU 1
#define GX_OAM_MODE_BITMAPOBJ 3

// OAM attributes. Bitmap OBJ take an alpha in place of a palette
typedef union {
    u16 attr[4];
    struct {
        u32 attr01;
        u32 attr23;
    };
    struct {
        u16 attr0;
        u16 attr1;
        u16 attr2;
        u16 affineParam;
    };
    struct {
        u32 y : 8;
        u32 rsMode : 2;
        u32 objMode : 2;
        u32 mosaic : 1;
        u32 colorMode : 1;
        u32 shape : 2;
        u32 x : 9;
        u32 rsParam : 5;
        u32 size : 2;
        u32 charNo : 10;
        u32 priority : 2;
        u32 cParam : 4;
        u32 : 16;
    };
    // The flips, the top bits of an rsParam that isn't affine
    struct {
        u32 : 28;
        u32 flipH : 1;
        u32 flipV : 1;
        u32 : 2;
        u32 : 32;
    };
} GXOamAttr;

#define GX_OAM_ATTR01_Y_SHIFT 0
#define GX_OAM_ATTR01_CM_MASK 0x00002000
#define GX_OAM_ATTR01_MODE_SHIFT 10
#define GX_OAM_ATTR01_MOSAIC_SHIFT 12
#define GX_OAM_ATTR01_CM_SHIFT 13
#define GX_OAM_ATTR01_X_SHIFT 16
#define GX_OAM_ATTR01_RS_SHIFT 25
// The shape and size bits of the first two attributes, a GXOamShape
#define GX_OAM_ATTR01_SHAPE_MASK 0xc000c000
// NitroSDK's GXOamShape is an enum of these bits, which MWCC makes an int: its shape bits shift as signed
typedef int GXOamShape;
#define GX_OAM_ATTR2_NAME_SHIFT 0
#define GX_OAM_ATTR2_NAME_MASK 0x03ff
#define GX_OAM_ATTR2_PRIORITY_SHIFT 10
#define GX_OAM_ATTR2_CPARAM_SHIFT 12

#define GX_OAM_EFFECT_NONE 0
#define GX_OAM_SHAPE_8x8 0x00000000
#define GX_OAM_SHAPE_16x16 0x40000000
#define GX_OAM_SHAPE_32x32 0x80000000
#define GX_OAM_SHAPE_64x64 0xc0000000
#define GX_OAM_SHAPE_16x8 0x00004000
#define GX_OAM_SHAPE_32x8 0x40004000
#define GX_OAM_SHAPE_32x16 0x80004000
#define GX_OAM_SHAPE_64x32 0xc0004000
#define GX_OAM_SHAPE_8x16 0x00008000
#define GX_OAM_SHAPE_8x32 0x40008000
#define GX_OAM_SHAPE_16x32 0x80008000
#define GX_OAM_SHAPE_32x64 0xc0008000
#define GX_OAM_COLORMODE_16 0

// The affine, flip and position fields of the first two attributes, and the effects that G2_GetOBJEffect and
// G2_SetOBJEffect read and write
#define GX_OAM_ATTR01_Y_MASK 0x000000ff
#define GX_OAM_ATTR01_RSENABLE_SHIFT 8
#define GX_OAM_ATTR01_RSENABLE_MASK 0x00000300
#define GX_OAM_ATTR01_SHAPE_SHIFT 14
#define GX_OAM_ATTR0_SHAPE_MASK 0x0000c000
#define GX_OAM_ATTR01_X_MASK 0x01ff0000
#define GX_OAM_ATTR01_RS_MASK 0x3e000000
#define GX_OAM_ATTR01_HF_SHIFT 28
#define GX_OAM_ATTR01_HF_MASK 0x10000000
#define GX_OAM_ATTR01_VF_SHIFT 29
#define GX_OAM_ATTR01_VF_MASK 0x20000000
#define GX_OAM_ATTR01_SIZE_SHIFT 30
#define GX_OAM_ATTR01_SIZE_MASK 0xc0000000
#define GX_OAM_ATTR2_CPARAM_MASK 0xf000
#define GX_OAM_ATTR01_MODE_MASK 0x00000c00
#define GX_OAM_ATTR01_MOSAIC_MASK 0x00001000
#define GX_OAM_EFFECT_AFFINE 0x00000100
#define GX_OAM_EFFECT_AFFINE_DOUBLE 0x00000300

static inline u32 G2_GetOBJEffect(const GXOamAttr *oam) {
    u32 rs = oam->attr01 & GX_OAM_ATTR01_RSENABLE_MASK;

    if (rs == GX_OAM_EFFECT_AFFINE || rs == GX_OAM_EFFECT_AFFINE_DOUBLE) {
        return rs;
    }
    return rs | (oam->attr01 & (GX_OAM_ATTR01_HF_MASK | GX_OAM_ATTR01_VF_MASK));
}

static inline void G2_SetOBJEffect(GXOamAttr *oam, u32 effect, int rsParam) {
    // The plain store comes first: with constant arguments, MWCC folds the other order into one store
    if (effect != GX_OAM_EFFECT_AFFINE && effect != GX_OAM_EFFECT_AFFINE_DOUBLE) {
        oam->attr01 = (oam->attr01 & ~(GX_OAM_ATTR01_RSENABLE_MASK | GX_OAM_ATTR01_HF_MASK | GX_OAM_ATTR01_VF_MASK |
                                       GX_OAM_ATTR01_RS_MASK)) |
                      effect;
    } else {
        oam->attr01 = (oam->attr01 & ~(GX_OAM_ATTR01_RSENABLE_MASK | GX_OAM_ATTR01_HF_MASK | GX_OAM_ATTR01_VF_MASK |
                                       GX_OAM_ATTR01_RS_MASK)) |
                      effect | (rsParam << GX_OAM_ATTR01_RS_SHIFT);
    }
}

static inline void G2_SetOBJPosition(GXOamAttr *oam, int x, int y) {
    oam->attr01 = (oam->attr01 & ~(GX_OAM_ATTR01_X_MASK | GX_OAM_ATTR01_Y_MASK)) |
                  ((x & 0x1ff) << GX_OAM_ATTR01_X_SHIFT) | ((y & 0xff) << GX_OAM_ATTR01_Y_SHIFT);
}

static inline void G2_SetOBJAttr(GXOamAttr *oam, int x, int y, int priority, int mode, BOOL mosaic, int effect,
                                 u32 shape, int color, int charName, int cParam, int rsParam) {
    oam->attr01 = (u32)(shape | ((y & 0xff) << GX_OAM_ATTR01_Y_SHIFT) | (mode << GX_OAM_ATTR01_MODE_SHIFT) |
                        (mosaic << GX_OAM_ATTR01_MOSAIC_SHIFT) | effect | (color << GX_OAM_ATTR01_CM_SHIFT) |
                        ((x & 0x1ff) << GX_OAM_ATTR01_X_SHIFT) | (rsParam << GX_OAM_ATTR01_RS_SHIFT));
    oam->attr2 = (u16)((charName << GX_OAM_ATTR2_NAME_SHIFT) | (priority << GX_OAM_ATTR2_PRIORITY_SHIFT) |
                       (cParam << GX_OAM_ATTR2_CPARAM_SHIFT));
}

// NitroSDK's setters of single OBJ attributes
static inline void G2_SetOBJShape(GXOamAttr *oam, u32 shape) {
    oam->attr01 = (oam->attr01 & ~GX_OAM_ATTR01_SHAPE_MASK) | shape;
}

static inline void G2_SetOBJCharName(GXOamAttr *oam, int name) {
    oam->attr2 = (u16)((oam->attr2 & ~GX_OAM_ATTR2_NAME_MASK) | name);
}

static inline void G2_SetOBJColorMode(GXOamAttr *oam, int color) {
    oam->attr01 = (oam->attr01 & ~GX_OAM_ATTR01_CM_MASK) | (color << GX_OAM_ATTR01_CM_SHIFT);
}

#define GX_PLANEMASK_BG0 0x01
#define GX_PLANEMASK_BG1 0x02
#define GX_PLANEMASK_BG2 0x04
#define GX_PLANEMASK_BG3 0x08
#define GX_PLANEMASK_OBJ 0x10
#define GX_PLANEMASK_ALL 0x1f

// The planes that blending takes, which include the backdrop
#define GX_BLEND_PLANEMASK_NONE 0x00
#define GX_BLEND_PLANEMASK_BG0 0x01
#define GX_BLEND_PLANEMASK_BG1 0x02
#define GX_BLEND_PLANEMASK_BG2 0x04
#define GX_BLEND_PLANEMASK_BG3 0x08
#define GX_BLEND_PLANEMASK_OBJ 0x10
#define GX_BLEND_PLANEMASK_BD 0x20

#define GX_DISPMODE_GRAPHICS 1
// Show VRAM A to D instead of the main engine's output; the display capture can write to D
#define GX_DISPMODE_VRAM_A 2
#define GX_DISPMODE_VRAM_B 6
#define GX_DISPMODE_VRAM_C 10
#define GX_DISPMODE_VRAM_D 0xe
#define GX_BGMODE_0 0
#define GX_BGMODE_1 1
#define GX_BGMODE_3 3
#define GX_BGMODE_5 5
#define GX_BG0_AS_2D 0
#define GX_BG0_AS_3D 1

#define GX_SHADING_TOON 0
#define GX_SHADING_HIGHLIGHT 1

typedef enum {
    GX_SORTMODE_AUTO,
    GX_SORTMODE_MANUAL,
} GXSortMode;

typedef enum {
    GX_BUFFERMODE_Z,
    GX_BUFFERMODE_W,
} GXBufferMode;

// VRAM banks, as masks
#define GX_VRAM_NONE 0x000
#define GX_VRAM_A 0x001
#define GX_VRAM_B 0x002
#define GX_VRAM_C 0x004
#define GX_VRAM_D 0x008
#define GX_VRAM_E 0x010
#define GX_VRAM_F 0x020
#define GX_VRAM_G 0x040
#define GX_VRAM_H 0x080
#define GX_VRAM_I 0x100

// The banks that hold the main engine's BG: A to D have 128 KB each, E 64 KB, and F and G 16 KB. The size in the name
// is the total
typedef enum {
    GX_VRAM_BG_NONE = GX_VRAM_NONE,
    GX_VRAM_BG_128_A = GX_VRAM_A,
    GX_VRAM_BG_128_B = GX_VRAM_B,
    GX_VRAM_BG_256_AB = GX_VRAM_A | GX_VRAM_B,
    GX_VRAM_BG_128_C = GX_VRAM_C,
    GX_VRAM_BG_256_AC = GX_VRAM_A | GX_VRAM_C,
    GX_VRAM_BG_256_BC = GX_VRAM_B | GX_VRAM_C,
    GX_VRAM_BG_384_ABC = GX_VRAM_A | GX_VRAM_B | GX_VRAM_C,
    GX_VRAM_BG_128_D = GX_VRAM_D,
    GX_VRAM_BG_256_AD = GX_VRAM_A | GX_VRAM_D,
    GX_VRAM_BG_256_BD = GX_VRAM_B | GX_VRAM_D,
    GX_VRAM_BG_384_ABD = GX_VRAM_A | GX_VRAM_B | GX_VRAM_D,
    GX_VRAM_BG_256_CD = GX_VRAM_C | GX_VRAM_D,
    GX_VRAM_BG_384_ACD = GX_VRAM_A | GX_VRAM_C | GX_VRAM_D,
    GX_VRAM_BG_384_BCD = GX_VRAM_B | GX_VRAM_C | GX_VRAM_D,
    GX_VRAM_BG_512_ABCD = GX_VRAM_A | GX_VRAM_B | GX_VRAM_C | GX_VRAM_D,
    GX_VRAM_BG_64_E = GX_VRAM_E,
    GX_VRAM_BG_16_F = GX_VRAM_F,
    GX_VRAM_BG_80_EF = GX_VRAM_E | GX_VRAM_F,
    GX_VRAM_BG_16_G = GX_VRAM_G,
    GX_VRAM_BG_80_EG = GX_VRAM_E | GX_VRAM_G,
    GX_VRAM_BG_32_FG = GX_VRAM_F | GX_VRAM_G,
    GX_VRAM_BG_96_EFG = GX_VRAM_E | GX_VRAM_F | GX_VRAM_G,
} GXVRamBG;
// The banks that hold the main engine's BG extended palettes: E, F (BGs 0 and 1), G (BGs 2 and 3) or F and G
typedef enum {
    GX_VRAM_BGEXTPLTT_NONE = GX_VRAM_NONE,
    GX_VRAM_BGEXTPLTT_0123_E = GX_VRAM_E,
    GX_VRAM_BGEXTPLTT_01_F = GX_VRAM_F,
    GX_VRAM_BGEXTPLTT_23_G = GX_VRAM_G,
    GX_VRAM_BGEXTPLTT_0123_FG = GX_VRAM_F | GX_VRAM_G,
} GXVRamBGExtPltt;
// The banks that hold the sub engine's BG: C, or H and I
typedef enum {
    GX_VRAM_SUB_BG_NONE = GX_VRAM_NONE,
    GX_VRAM_SUB_BG_128_C = GX_VRAM_C,
    GX_VRAM_SUB_BG_32_H = GX_VRAM_H,
    GX_VRAM_SUB_BG_48_HI = GX_VRAM_H | GX_VRAM_I,
} GXVRamSubBG;
// The bank that holds the sub engine's BG extended palettes
typedef enum {
    GX_VRAM_SUB_BGEXTPLTT_NONE = GX_VRAM_NONE,
    GX_VRAM_SUB_BGEXTPLTT_0123_H = GX_VRAM_H,
} GXVRamSubBGExtPltt;
// The banks that hold the main engine's OBJ, sized as the BG banks are
typedef enum {
    GX_VRAM_OBJ_NONE = GX_VRAM_NONE,
    GX_VRAM_OBJ_128_A = GX_VRAM_A,
    GX_VRAM_OBJ_128_B = GX_VRAM_B,
    GX_VRAM_OBJ_256_AB = GX_VRAM_A | GX_VRAM_B,
    GX_VRAM_OBJ_64_E = GX_VRAM_E,
    GX_VRAM_OBJ_16_F = GX_VRAM_F,
    GX_VRAM_OBJ_80_EF = GX_VRAM_E | GX_VRAM_F,
    GX_VRAM_OBJ_16_G = GX_VRAM_G,
    GX_VRAM_OBJ_80_EG = GX_VRAM_E | GX_VRAM_G,
    GX_VRAM_OBJ_32_FG = GX_VRAM_F | GX_VRAM_G,
    GX_VRAM_OBJ_96_EFG = GX_VRAM_E | GX_VRAM_F | GX_VRAM_G,
} GXVRamOBJ;
// The banks that hold the main engine's OBJ extended palettes: F or G
typedef enum {
    GX_VRAM_OBJEXTPLTT_NONE = GX_VRAM_NONE,
    GX_VRAM_OBJEXTPLTT_0_F = GX_VRAM_F,
    GX_VRAM_OBJEXTPLTT_0_G = GX_VRAM_G,
} GXVRamOBJExtPltt;
// The banks that hold the sub engine's OBJ: D or I
typedef enum {
    GX_VRAM_SUB_OBJ_NONE = GX_VRAM_NONE,
    GX_VRAM_SUB_OBJ_128_D = GX_VRAM_D,
    GX_VRAM_SUB_OBJ_16_I = GX_VRAM_I,
} GXVRamSubOBJ;
// The bank that holds the sub engine's OBJ extended palette
typedef enum {
    GX_VRAM_SUB_OBJEXTPLTT_NONE = GX_VRAM_NONE,
    GX_VRAM_SUB_OBJEXTPLTT_0_I = GX_VRAM_I,
} GXVRamSubOBJExtPltt;
// The banks that the ARM7 uses as WRAM (C, D, or both)
typedef enum {
    GX_VRAM_ARM7_NONE = GX_VRAM_NONE,
    GX_VRAM_ARM7_128_C = GX_VRAM_C,
    GX_VRAM_ARM7_128_D = GX_VRAM_D,
    GX_VRAM_ARM7_256_CD = GX_VRAM_C | GX_VRAM_D,
} GXVRamARM7;
// The banks that hold the textures, which are slots of 128 KB in the order of the banks: the number says which slots
// the textures have, and the letters the banks
typedef enum {
    GX_VRAM_TEX_NONE = GX_VRAM_NONE,
    GX_VRAM_TEX_0_A = GX_VRAM_A,
    GX_VRAM_TEX_0_B = GX_VRAM_B,
    GX_VRAM_TEX_01_AB = GX_VRAM_A | GX_VRAM_B,
    GX_VRAM_TEX_0_C = GX_VRAM_C,
    GX_VRAM_TEX_01_AC = GX_VRAM_A | GX_VRAM_C,
    GX_VRAM_TEX_01_BC = GX_VRAM_B | GX_VRAM_C,
    GX_VRAM_TEX_012_ABC = GX_VRAM_A | GX_VRAM_B | GX_VRAM_C,
    GX_VRAM_TEX_0_D = GX_VRAM_D,
    GX_VRAM_TEX_01_AD = GX_VRAM_A | GX_VRAM_D,
    GX_VRAM_TEX_01_BD = GX_VRAM_B | GX_VRAM_D,
    GX_VRAM_TEX_012_ABD = GX_VRAM_A | GX_VRAM_B | GX_VRAM_D,
    GX_VRAM_TEX_01_CD = GX_VRAM_C | GX_VRAM_D,
    GX_VRAM_TEX_012_ACD = GX_VRAM_A | GX_VRAM_C | GX_VRAM_D,
    GX_VRAM_TEX_012_BCD = GX_VRAM_B | GX_VRAM_C | GX_VRAM_D,
    GX_VRAM_TEX_0123_ABCD = GX_VRAM_A | GX_VRAM_B | GX_VRAM_C | GX_VRAM_D,
} GXVRamTex;
// The banks that hold the palettes of the textures: E, F or G, or E and F, F and G, or all three
typedef enum {
    GX_VRAM_TEXPLTT_NONE = GX_VRAM_NONE,
    GX_VRAM_TEXPLTT_0123_E = GX_VRAM_E,
    GX_VRAM_TEXPLTT_0_F = GX_VRAM_F,
    GX_VRAM_TEXPLTT_01234_EF = GX_VRAM_E | GX_VRAM_F,
    GX_VRAM_TEXPLTT_0_G = GX_VRAM_G,
    GX_VRAM_TEXPLTT_01_FG = GX_VRAM_F | GX_VRAM_G,
    GX_VRAM_TEXPLTT_012345_EFG = GX_VRAM_E | GX_VRAM_F | GX_VRAM_G,
} GXVRamTexPltt;
// The banks that hold the rear plane image, whose two images take 128 KB each: A and B, or C and D. A bank alone has
// the second image
typedef enum {
    GX_VRAM_CLEARIMAGE_NONE = GX_VRAM_NONE,
    GX_VRAM_CLEARIMAGE_128_A = GX_VRAM_A,
    GX_VRAM_CLEARIMAGE_128_B = GX_VRAM_B,
    GX_VRAM_CLEARIMAGE_256_AB = GX_VRAM_A | GX_VRAM_B,
    GX_VRAM_CLEARIMAGE_128_C = GX_VRAM_C,
    GX_VRAM_CLEARIMAGE_128_D = GX_VRAM_D,
    GX_VRAM_CLEARIMAGE_256_CD = GX_VRAM_C | GX_VRAM_D,
} GXVRamClearImage;

typedef enum {
    GX_OBJVRAMMODE_CHAR_2D = 0x00000000,
    GX_OBJVRAMMODE_CHAR_1D_32K = 0x00000010,
    GX_OBJVRAMMODE_CHAR_1D_64K = 0x00100010,
    GX_OBJVRAMMODE_CHAR_1D_128K = 0x00200010,
    GX_OBJVRAMMODE_CHAR_1D_256K = 0x00300010,
} GXOBJVRamModeChar;

typedef enum {
    GX_OBJVRAMMODE_BMP_2D_W128 = 0x00000000,
    GX_OBJVRAMMODE_BMP_2D_W256 = 0x00000020,
    GX_OBJVRAMMODE_BMP_1D_128K = 0x00000040,
} GXOBJVRamModeBmp;

// NitroSDK passes the fields of the BG control registers as enums, which MWCC schedules differently from ints
typedef enum {
    GX_BG_COLORMODE_16 = 0,
    GX_BG_COLORMODE_256 = 1,
} GXBGColorMode;

// The screen base in units of 0x800 bytes, and the character base in units of 0x4000
#define GX_BG_SCRBASE(offset) ((offset) / 0x800)
#define GX_BG_CHARBASE(offset) ((offset) / 0x4000)

typedef enum {
    GX_BG_SCRBASE_0x0000 = 0,
    GX_BG_SCRBASE_0xf800 = 31,
} GXBGScrBase;

typedef enum {
    GX_BG_CHARBASE_0x00000 = 0,
    GX_BG_CHARBASE_0x04000 = 1,
    GX_BG_CHARBASE_0x10000 = 4,
    GX_BG_CHARBASE_0x3c000 = 15,
} GXBGCharBase;

typedef enum {
    GX_BG_EXTPLTT_01 = 0,
    GX_BG_EXTPLTT_23 = 1,
} GXBGExtPltt;

typedef enum {
    GX_BG_AREAOVER_XLU = 0,
    GX_BG_AREAOVER_REPEAT = 1,
} GXBGAreaOver;

// Direct color bitmap BGs, of 16 bit colors
typedef enum {
    GX_BG_SCRSIZE_DCBMP_128x128 = 0,
    GX_BG_SCRSIZE_DCBMP_256x256 = 1,
    GX_BG_SCRSIZE_DCBMP_512x256 = 2,
    GX_BG_SCRSIZE_DCBMP_512x512 = 3,
} GXBGScrSizeDcBmp;

typedef enum {
    GX_BG_BMPSCRBASE_0x00000 = 0,
    GX_BG_BMPSCRBASE_0x10000 = 4,
    GX_BG_BMPSCRBASE_0x14000 = 5,
    GX_BG_BMPSCRBASE_0x20000 = 8,
    GX_BG_BMPSCRBASE_0x28000 = 10,
} GXBGBmpScrBase;

#define GX_PACK_VIEWPORT_PARAM(x1, y1, x2, y2) ((u32)(x1) | ((u32)(y1) << 8) | ((u32)(x2) << 16) | ((u32)(y2) << 24))

// The geometry engine's matrix stacks, primitives, polygon attributes and textures
#define GX_MTXMODE_PROJECTION 0
#define GX_MTXMODE_POSITION 1
#define GX_MTXMODE_POSITION_VECTOR 2
#define GX_MTXMODE_TEXTURE 3

#define GX_BEGIN_TRIANGLES 0
#define GX_BEGIN_QUADS 1
#define GX_BEGIN_TRIANGLE_STRIP 2
#define GX_BEGIN_QUAD_STRIP 3

typedef enum {
    GX_POLYGONMODE_MODULATE,
    GX_POLYGONMODE_DECAL,
    GX_POLYGONMODE_TOON,
    GX_POLYGONMODE_SHADOW,
} GXPolygonMode;

typedef enum {
    GX_LIGHTMASK_NONE = 0,
    GX_LIGHTMASK_0 = 1,
    GX_LIGHTMASK_1 = 2,
    GX_LIGHTMASK_2 = 4,
    GX_LIGHTMASK_3 = 8,
} GXLightMask;
#define GX_POLYGON_ATTR_MISC_FAR_CLIPPING 0x1000
#define GX_POLYGON_ATTR_MISC_DISP_1DOT 0x2000
#define GX_POLYGON_ATTR_MISC_FOG 0x8000

// A box for the geometry engine's box test, in model coordinates
typedef struct {
    fx16 x;
    fx16 y;
    fx16 z;
    fx16 width;
    fx16 height;
    fx16 depth;
} GXBoxTestParam;

typedef enum {
    GX_LIGHTID_0,
    GX_LIGHTID_1,
    GX_LIGHTID_2,
    GX_LIGHTID_3,
} GXLightId;

// Geometry commands, as NitroSystem buffers them
#define G3OP_MTX_MODE 0x10
#define G3OP_MTX_PUSH 0x11
#define G3OP_MTX_POP 0x12
#define G3OP_MTX_STORE 0x13
#define G3OP_MTX_RESTORE 0x14
#define G3OP_MTX_IDENTITY 0x15
#define G3OP_MTX_LOAD_4x4 0x16
#define G3OP_MTX_LOAD_4x3 0x17
#define G3OP_MTX_MULT_4x4 0x18
#define G3OP_MTX_MULT_4x3 0x19
#define G3OP_MTX_MULT_3x3 0x1a
#define G3OP_MTX_SCALE 0x1b
#define G3OP_MTX_TRANS 0x1c
#define G3OP_TEXCOORD 0x22
#define G3OP_POLYGON_ATTR 0x29
#define G3OP_TEXIMAGE_PARAM 0x2a
#define G3OP_BEGIN 0x40
#define G3OP_END 0x41
#define G3OP_BOX_TEST 0x70

typedef enum {
    GX_CULL_ALL,
    GX_CULL_FRONT,
    GX_CULL_BACK,
    GX_CULL_NONE,
} GXCull;

// The texture parameters as the SDK's enums, which G3_TexImageParam takes: SPL's texture setup only matches with them
typedef enum {
    GX_TEXFMT_NONE,
    GX_TEXFMT_A3I5,
    GX_TEXFMT_PLTT4,
    GX_TEXFMT_PLTT16,
    GX_TEXFMT_PLTT256,
    GX_TEXFMT_COMP4x4,
    GX_TEXFMT_A5I3,
    GX_TEXFMT_DIRECT,
} GXTexFmt;

typedef enum {
    GX_TEXGEN_NONE,
    GX_TEXGEN_TEXCOORD,
    GX_TEXGEN_NORMAL,
    GX_TEXGEN_VERTEX,
} GXTexGen;

typedef enum {
    GX_TEXSIZE_S8,
    GX_TEXSIZE_S16,
    GX_TEXSIZE_S32,
    GX_TEXSIZE_S64,
    GX_TEXSIZE_S128,
    GX_TEXSIZE_S256,
    GX_TEXSIZE_S512,
    GX_TEXSIZE_S1024,
} GXTexSizeS;

typedef enum {
    GX_TEXSIZE_T8,
    GX_TEXSIZE_T16,
    GX_TEXSIZE_T32,
    GX_TEXSIZE_T64,
    GX_TEXSIZE_T128,
    GX_TEXSIZE_T256,
    GX_TEXSIZE_T512,
    GX_TEXSIZE_T1024,
} GXTexSizeT;

typedef enum {
    GX_TEXREPEAT_NONE,
    GX_TEXREPEAT_S,
    GX_TEXREPEAT_T,
    GX_TEXREPEAT_ST,
} GXTexRepeat;

typedef enum {
    GX_TEXFLIP_NONE,
    GX_TEXFLIP_S,
    GX_TEXFLIP_T,
    GX_TEXFLIP_ST,
} GXTexFlip;

typedef enum {
    GX_TEXPLTTCOLOR0_USE,
    GX_TEXPLTTCOLOR0_TRNS,
} GXTexPlttColor0;

#define REG_G3_POLYGON_ATTR_LE_SHIFT 0
#define REG_G3_POLYGON_ATTR_PM_SHIFT 4
#define REG_G3_POLYGON_ATTR_BK_SHIFT 6
#define REG_G3_POLYGON_ATTR_ALPHA_SHIFT 16
#define REG_G3_POLYGON_ATTR_ID_SHIFT 24

#define REG_G3_TEXIMAGE_PARAM_RS_SHIFT 16
#define REG_G3_TEXIMAGE_PARAM_FS_SHIFT 18
#define REG_G3_TEXIMAGE_PARAM_V_SIZE_SHIFT 20
#define REG_G3_TEXIMAGE_PARAM_T_SIZE_SHIFT 23
#define REG_G3_TEXIMAGE_PARAM_TEXFMT_SHIFT 26
#define REG_G3_TEXIMAGE_PARAM_TR_SHIFT 29
#define REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT 30

#define REG_G3_DIF_AMB_AMBIENT_RED_SHIFT 16
#define REG_G3_DIF_AMB_C_SHIFT 15
#define REG_G3_SPE_EMI_EMISSION_RED_SHIFT 16
#define REG_G3_SPE_EMI_S_SHIFT 15

#define GX_PACK_POLYGONATTR_PARAM(light, polyMode, cullMode, polygonID, alpha, misc)                                   \
    ((u32)(((light) << REG_G3_POLYGON_ATTR_LE_SHIFT) | ((polyMode) << REG_G3_POLYGON_ATTR_PM_SHIFT) |                  \
           ((cullMode) << REG_G3_POLYGON_ATTR_BK_SHIFT) | (misc) | ((polygonID) << REG_G3_POLYGON_ATTR_ID_SHIFT) |     \
           ((alpha) << REG_G3_POLYGON_ATTR_ALPHA_SHIFT)))

#define GX_PACK_TEXIMAGE_PARAM(texFmt, texGen, s, t, repeat, flip, pltt0, addr)                                        \
    ((u32)(((addr) >> 3) | ((texFmt) << REG_G3_TEXIMAGE_PARAM_TEXFMT_SHIFT) |                                          \
           ((texGen) << REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT) | ((s) << REG_G3_TEXIMAGE_PARAM_V_SIZE_SHIFT) |              \
           ((t) << REG_G3_TEXIMAGE_PARAM_T_SIZE_SHIFT) | ((repeat) << REG_G3_TEXIMAGE_PARAM_RS_SHIFT) |                \
           ((flip) << REG_G3_TEXIMAGE_PARAM_FS_SHIFT) | ((pltt0) << REG_G3_TEXIMAGE_PARAM_TR_SHIFT)))

#define GX_PACK_TEXPLTTBASE_PARAM(addr, texFmt) ((u32)((addr) >> (4 - ((texFmt) == GX_TEXFMT_PLTT4))))

#define GX_PACK_DIFFAMB_PARAM(diffuse, ambient, IsSetVtxColor)                                                         \
    ((u32)((diffuse) | ((ambient) << REG_G3_DIF_AMB_AMBIENT_RED_SHIFT) |                                               \
           (((IsSetVtxColor) != FALSE) << REG_G3_DIF_AMB_C_SHIFT)))

#define GX_PACK_SPECEMI_PARAM(specular, emission, IsShininess)                                                         \
    ((u32)((specular) | ((emission) << REG_G3_SPE_EMI_EMISSION_RED_SHIFT) |                                            \
           (((IsShininess) != FALSE) << REG_G3_SPE_EMI_S_SHIFT)))

// A normal's components, 10 bits each, and a texture coordinate's, fixed point with 4 fractional bits
#define GX_FX16_FX10(x) ((fx16)((x) >> 3))
#define GX_VECFX10(x, y, z) ((u32)(((x) & 0x3ff) | (((y) & 0x3ff) << 10) | (((z) & 0x3ff) << 20)))
#define REG_G3_LIGHT_VECTOR_LNUM_SHIFT 30
#define GX_PACK_LIGHTVECTOR_PARAM(lightID, x, y, z)                                                                    \
    ((u32)(((lightID) << REG_G3_LIGHT_VECTOR_LNUM_SHIFT) |                                                             \
           GX_VECFX10(GX_FX16_FX10(x), GX_FX16_FX10(y), GX_FX16_FX10(z))))
#define GX_PACK_LIGHTCOLOR_PARAM(lightID, rgb) ((u32)(((lightID) << REG_G3_LIGHT_VECTOR_LNUM_SHIFT) | (rgb)))
// Packs texture coordinates. The game's SDK narrows each to an fx16 first; the older NitroSDK that SPL was built
// against does not
#ifdef OLD_NITRO_SDK
#define GX_ST(s, t) ((u32)(u16)((s) >> 8) | ((u32)(u16)((t) >> 8) << 16))
#else
#define GX_ST(s, t) ((u32)(u16)(fx16)((s) >> 8) | ((u32)(u16)(fx16)((t) >> 8) << 16))
#endif

#define REG_GX_POWCNT_DSEL_SHIFT 15

#define REG_GX_DISPCAPCNT_E_MASK 0x80000000
#define REG_GX_DISPCAPCNT_MOD_SHIFT 29
#define REG_GX_DISPCAPCNT_SRCB_SHIFT 25
#define REG_GX_DISPCAPCNT_SRCA_SHIFT 24
#define REG_GX_DISPCAPCNT_WSIZE_SHIFT 20
#define REG_GX_DISPCAPCNT_WB_SHIFT 16
#define REG_GX_DISPCAPCNT_EVB_SHIFT 8
#define REG_GX_DISPCAPCNT_EVA_SHIFT 0

// The display capture, which writes the main engine's output to VRAM, blended with an image in VRAM with AB.
// NitroSDK passes its fields as enums too
typedef enum {
    GX_CAPTURE_SIZE_256x192 = 3,
} GXCaptureSize;

typedef enum {
    GX_CAPTURE_MODE_A = 0,
    GX_CAPTURE_MODE_AB = 2,
} GXCaptureMode;

typedef enum {
    GX_CAPTURE_SRCA_2D3D = 0,
    GX_CAPTURE_SRCA_3D = 1,
} GXCaptureSrcA;

typedef enum {
    GX_CAPTURE_SRCB_VRAM_0x00000 = 0,
} GXCaptureSrcB;

typedef enum {
    GX_CAPTURE_DEST_VRAM_A_0x00000 = 0,
    GX_CAPTURE_DEST_VRAM_B_0x00000 = 1,
    GX_CAPTURE_DEST_VRAM_C_0x00000 = 2,
    GX_CAPTURE_DEST_VRAM_D_0x00000 = 3,
} GXCaptureDest;

// Which engine draws to the top screen
#define GX_DISP_SELECT_SUB_MAIN 0
#define GX_DISP_SELECT_MAIN_SUB 1

// The line the display is drawing
static inline s32 GX_GetVCount(void) {
    return reg_GX_VCOUNT;
}

// Whether the display is in a line's H-blank
static inline BOOL GX_IsHBlank(void) {
    return reg_GX_DISPSTAT & REG_GX_DISPSTAT_HBLK_MASK;
}

static inline void GX_SetDispSelect(int select) {
    reg_GX_POWCNT = (u16)((reg_GX_POWCNT & ~REG_GX_POWCNT_DSEL_MASK) | (select << REG_GX_POWCNT_DSEL_SHIFT));
}

// The main screen's OBJ characters
static inline void *G2_GetOBJCharPtr(void) {
    return (void *)HW_OBJ_VRAM;
}

static inline int GX_GetDispSelect(void) {
    return (reg_GX_POWCNT & REG_GX_POWCNT_DSEL_MASK) >> REG_GX_POWCNT_DSEL_SHIFT;
}

// The fields go from the highest bit to the lowest, as NitroSDK's register field macros write them
static inline void GX_SetCapture(GXCaptureSize sz, GXCaptureMode mode, GXCaptureSrcA a, GXCaptureSrcB b,
                                 GXCaptureDest dest, int eva, int evb) {
    reg_GX_DISPCAPCNT = REG_GX_DISPCAPCNT_E_MASK | (mode << REG_GX_DISPCAPCNT_MOD_SHIFT) |
                        (b << REG_GX_DISPCAPCNT_SRCB_SHIFT) | (a << REG_GX_DISPCAPCNT_SRCA_SHIFT) |
                        (sz << REG_GX_DISPCAPCNT_WSIZE_SHIFT) | (dest << REG_GX_DISPCAPCNT_WB_SHIFT) |
                        (evb << REG_GX_DISPCAPCNT_EVB_SHIFT) | (eva << REG_GX_DISPCAPCNT_EVA_SHIFT);
}

static inline void GX_SetVisibleWnd(int window) {
    reg_GX_DISPCNT = (reg_GX_DISPCNT & ~(REG_GX_DISPCNT_W0_MASK | REG_GX_DISPCNT_W1_MASK | REG_GX_DISPCNT_OW_MASK)) |
                     (window << REG_GX_DISPCNT_W0_SHIFT);
}

static inline int GX_GetVisibleWnd(void) {
    return (reg_GX_DISPCNT & (REG_GX_DISPCNT_W0_MASK | REG_GX_DISPCNT_W1_MASK | REG_GX_DISPCNT_OW_MASK)) >>
           REG_GX_DISPCNT_W0_SHIFT;
}

static inline void GXS_SetVisibleWnd(int window) {
    reg_GXS_DB_DISPCNT =
        (reg_GXS_DB_DISPCNT & ~(REG_GX_DISPCNT_W0_MASK | REG_GX_DISPCNT_W1_MASK | REG_GX_DISPCNT_OW_MASK)) |
        (window << REG_GX_DISPCNT_W0_SHIFT);
}

// The planes inside a window or outside all of them, and whether color effects apply there. NitroSDK's
// G2_SetWnd0InsidePlane, G2_SetWnd1InsidePlane, G2_SetWndOBJInsidePlane and G2_SetWndOutsidePlane, and their G2S_
// forms for the sub screen
#define REG_G2_WININ_WIN0IN_MASK 0x003f
#define REG_G2_WININ_WIN1IN_SHIFT 8
#define REG_G2_WININ_WIN1IN_MASK 0x3f00
#define REG_G2_WINOUT_WINOUT_MASK 0x003f
#define REG_G2_WINOUT_OBJWININ_SHIFT 8
#define REG_G2_WINOUT_OBJWININ_MASK 0x3f00
#define G2_WND_EFFECT 0x20

// The planes outside all windows, and whether color effects apply there, as the register's low byte
typedef struct {
    u8 planeMask : 5;
    u8 effect : 1;
    u8 : 2;
} GXWndPlane;

static inline GXWndPlane G2_GetWndOutsidePlane(void) {
    return *(volatile GXWndPlane *)&reg_G2_WINOUT;
}

static inline void G2_SetWnd0InsidePlane(int wnd, BOOL effect) {
    u32 tmp = (reg_G2_WININ & ~REG_G2_WININ_WIN0IN_MASK) | wnd;

    if (effect) {
        tmp |= G2_WND_EFFECT;
    }
    reg_G2_WININ = (u16)tmp;
}

static inline void G2_SetWnd1InsidePlane(int wnd, BOOL effect) {
    u32 tmp = (reg_G2_WININ & ~REG_G2_WININ_WIN1IN_MASK) | (wnd << REG_G2_WININ_WIN1IN_SHIFT);

    if (effect) {
        tmp |= G2_WND_EFFECT << REG_G2_WININ_WIN1IN_SHIFT;
    }
    reg_G2_WININ = (u16)tmp;
}

static inline void G2_SetWndOutsidePlane(int wnd, BOOL effect) {
    u32 tmp = (reg_G2_WINOUT & ~REG_G2_WINOUT_WINOUT_MASK) | wnd;

    if (effect) {
        tmp |= G2_WND_EFFECT;
    }
    reg_G2_WINOUT = (u16)tmp;
}

static inline void G2_SetWndOBJInsidePlane(int wnd, BOOL effect) {
    u32 tmp = (reg_G2_WINOUT & ~REG_G2_WINOUT_OBJWININ_MASK) | (wnd << REG_G2_WINOUT_OBJWININ_SHIFT);

    if (effect) {
        tmp |= G2_WND_EFFECT << REG_G2_WINOUT_OBJWININ_SHIFT;
    }
    reg_G2_WINOUT = (u16)tmp;
}

static inline void G2S_SetWnd0InsidePlane(int wnd, BOOL effect) {
    u32 tmp = (reg_G2S_DB_WININ & ~REG_G2_WININ_WIN0IN_MASK) | wnd;

    if (effect) {
        tmp |= G2_WND_EFFECT;
    }
    reg_G2S_DB_WININ = (u16)tmp;
}

static inline void G2S_SetWnd1InsidePlane(int wnd, BOOL effect) {
    u32 tmp = (reg_G2S_DB_WININ & ~REG_G2_WININ_WIN1IN_MASK) | (wnd << REG_G2_WININ_WIN1IN_SHIFT);

    if (effect) {
        tmp |= G2_WND_EFFECT << REG_G2_WININ_WIN1IN_SHIFT;
    }
    reg_G2S_DB_WININ = (u16)tmp;
}

static inline void G2S_SetWndOBJInsidePlane(int wnd, BOOL effect) {
    u32 tmp = (reg_G2S_DB_WINOUT & ~REG_G2_WINOUT_OBJWININ_MASK) | (wnd << REG_G2_WINOUT_OBJWININ_SHIFT);

    if (effect) {
        tmp |= G2_WND_EFFECT << REG_G2_WINOUT_OBJWININ_SHIFT;
    }
    reg_G2S_DB_WINOUT = (u16)tmp;
}

static inline void G2S_SetWndOutsidePlane(int wnd, BOOL effect) {
    u32 tmp = (reg_G2S_DB_WINOUT & ~REG_G2_WINOUT_WINOUT_MASK) | wnd;

    if (effect) {
        tmp |= G2_WND_EFFECT;
    }
    reg_G2S_DB_WINOUT = (u16)tmp;
}

// A window's rectangle, from (x1, y1) to before (x2, y2). NitroSDK's G2_SetWnd0Position, G2_SetWnd1Position and their
// G2S_ forms
static inline void G2_SetWnd0Position(int x1, int y1, int x2, int y2) {
    reg_G2_WIN0H = (u16)(((x1 << 8) & 0xff00) | (u8)x2);
    reg_G2_WIN0V = (u16)(((y1 << 8) & 0xff00) | (u8)y2);
}

static inline void G2_SetWnd1Position(int x1, int y1, int x2, int y2) {
    reg_G2_WIN1H = (u16)(((x1 << 8) & 0xff00) | (u8)x2);
    reg_G2_WIN1V = (u16)(((y1 << 8) & 0xff00) | (u8)y2);
}

static inline void G2S_SetWnd0Position(int x1, int y1, int x2, int y2) {
    reg_G2S_DB_WIN0H = (u16)(((x1 << 8) & 0xff00) | (u8)x2);
    reg_G2S_DB_WIN0V = (u16)(((y1 << 8) & 0xff00) | (u8)y2);
}

static inline void G2S_SetWnd1Position(int x1, int y1, int x2, int y2) {
    reg_G2S_DB_WIN1H = (u16)(((x1 << 8) & 0xff00) | (u8)x2);
    reg_G2S_DB_WIN1V = (u16)(((y1 << 8) & 0xff00) | (u8)y2);
}

static inline void G2_BlendNone(void) {
    reg_G2_BLDCNT = 0;
}

static inline void G2S_BlendNone(void) {
    reg_G2S_DB_BLDCNT = 0;
}

static inline void G2_ChangeBlendAlpha(int eva, int evb) {
    reg_G2_BLDALPHA = (u16)(eva | (evb << 8));
}

static inline void G2S_ChangeBlendAlpha(int eva, int evb) {
    reg_G2S_DB_BLDALPHA = (u16)(eva | (evb << 8));
}

static inline void G3X_SetShading(int shading) {
    reg_G3X_DISP3DCNT = (u16)((reg_G3X_DISP3DCNT &
                               ~(REG_G3X_DISP3DCNT_THS_MASK | REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK)) |
                              (shading << REG_G3X_DISP3DCNT_THS_SHIFT));
}

static inline void G3X_AntiAlias(BOOL enable) {
    if (enable) {
        reg_G3X_DISP3DCNT = (u16)((reg_G3X_DISP3DCNT & ~(REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK)) |
                                  REG_G3X_DISP3DCNT_AAE_MASK);
    } else {
        reg_G3X_DISP3DCNT &= ~(REG_G3X_DISP3DCNT_AAE_MASK | REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK);
    }
}

static inline void G3X_AlphaTest(BOOL enable, int ref) {
    if (enable) {
        reg_G3X_DISP3DCNT = (u16)((reg_G3X_DISP3DCNT & ~(REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK)) |
                                  REG_G3X_DISP3DCNT_ATE_MASK);
        *(vu16 *)0x04000340 = (u16)ref;
    } else {
        reg_G3X_DISP3DCNT &= ~(REG_G3X_DISP3DCNT_ATE_MASK | REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK);
    }
}

static inline void G3X_AlphaBlend(BOOL enable) {
    if (enable) {
        reg_G3X_DISP3DCNT = (u16)((reg_G3X_DISP3DCNT & ~(REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK)) |
                                  REG_G3X_DISP3DCNT_ABE_MASK);
    } else {
        reg_G3X_DISP3DCNT &= ~(REG_G3X_DISP3DCNT_ABE_MASK | REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK);
    }
}

static inline void G3X_EdgeMarking(BOOL enable) {
    if (enable) {
        reg_G3X_DISP3DCNT = (u16)((reg_G3X_DISP3DCNT & ~(REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK)) |
                                  REG_G3X_DISP3DCNT_EME_MASK);
    } else {
        reg_G3X_DISP3DCNT &= ~(REG_G3X_DISP3DCNT_EME_MASK | REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK);
    }
}

static inline void G3X_SetFogColor(GXRgb rgb, int alpha) {
    *(vu32 *)0x04000358 = (u32)(rgb | (alpha << 16));
}

// Swaps the geometry engine's buffers at the next vertical blank, with how it sorts translucent polygons and whether
// it depth tests with Z or W
static inline void G3_SwapBuffers(GXSortMode am, GXBufferMode zw) {
    reg_G3_SWAP_BUFFERS = am | (zw << 1);
}

static inline void G3_ViewPort(int x1, int y1, int x2, int y2) {
    reg_G3_VIEWPORT = GX_PACK_VIEWPORT_PARAM(x1, y1, x2, y2);
}

static inline void G3_MtxMode(int mode) {
    reg_G3_MTX_MODE = mode;
}

static inline void G3_PushMtx(void) {
    reg_G3_MTX_PUSH = 0;
}

static inline void G3_PopMtx(int num) {
    reg_G3_MTX_POP = num;
}

static inline void G3_StoreMtx(int num) {
    reg_G3_MTX_STORE = (u32)num;
}

static inline void G3_RestoreMtx(int num) {
    reg_G3_MTX_RESTORE = (u32)num;
}

static inline void G3_Identity(void) {
    reg_G3_MTX_IDENTITY = 0;
}

static inline void G3_Translate(fx32 x, fx32 y, fx32 z) {
    reg_G3_MTX_TRANS = (u32)x;
    reg_G3_MTX_TRANS = (u32)y;
    reg_G3_MTX_TRANS = (u32)z;
}

static inline void G3_Scale(fx32 x, fx32 y, fx32 z) {
    reg_G3_MTX_SCALE = (u32)x;
    reg_G3_MTX_SCALE = (u32)y;
    reg_G3_MTX_SCALE = (u32)z;
}

static inline void G3_PolygonAttr(int light, GXPolygonMode polyMode, GXCull cullMode, int polygonID, int alpha,
                                  int misc) {
    reg_G3_POLYGON_ATTR = GX_PACK_POLYGONATTR_PARAM(light, polyMode, cullMode, polygonID, alpha, misc);
}

static inline void G3_TexImageParam(GXTexFmt texFmt, GXTexGen texGen, GXTexSizeS s, GXTexSizeT t, GXTexRepeat repeat,
                                    GXTexFlip flip, GXTexPlttColor0 pltt0, u32 addr) {
    reg_G3_TEXIMAGE_PARAM = GX_PACK_TEXIMAGE_PARAM(texFmt, texGen, s, t, repeat, flip, pltt0, addr);
}

static inline void G3_TexPlttBase(u32 addr, GXTexFmt texFmt) {
    reg_G3_TEXPLTT_BASE = GX_PACK_TEXPLTTBASE_PARAM(addr, texFmt);
}

static inline void G3_MaterialColorDiffAmb(GXRgb diffuse, GXRgb ambient, BOOL IsSetVtxColor) {
    reg_G3_DIF_AMB = GX_PACK_DIFFAMB_PARAM(diffuse, ambient, IsSetVtxColor);
}

static inline void G3_MaterialColorSpecEmi(GXRgb specular, GXRgb emission, BOOL IsShininess) {
    reg_G3_SPE_EMI = GX_PACK_SPECEMI_PARAM(specular, emission, IsShininess);
}

static inline void G3_Begin(int primitive) {
    reg_G3_BEGIN_VTXS = (u32)primitive;
}

static inline void G3_End(void) {
    reg_G3_END_VTXS = 0;
}

static inline void G3_Normal(fx16 x, fx16 y, fx16 z) {
    reg_G3_NORMAL = GX_VECFX10(GX_FX16_FX10(x), GX_FX16_FX10(y), GX_FX16_FX10(z));
}

static inline void G3_TexCoord(fx32 s, fx32 t) {
    reg_G3_TEXCOORD = GX_ST(s, t);
}

static inline void G3_Color(GXRgb rgb) {
    reg_G3_COLOR = rgb;
}

static inline void G3_LightVector(GXLightId lightID, fx16 x, fx16 y, fx16 z) {
    reg_G3_LIGHT_VECTOR = GX_PACK_LIGHTVECTOR_PARAM(lightID, x, y, z);
}

static inline void G3_LightColor(GXLightId lightID, GXRgb rgb) {
    reg_G3_LIGHT_COLOR = GX_PACK_LIGHTCOLOR_PARAM(lightID, rgb);
}

static inline void G3_Vtx(fx16 x, fx16 y, fx16 z) {
    reg_G3_VTX_16 = (u32)((u16)x | ((u16)y << 16));
    reg_G3_VTX_16 = (u32)(u16)z;
}
#define reg_G2_BG2PA (*(vu16 *)0x04000020)

// A vertex with 10-bit coordinates, of 6 fractional bits
static inline void G3_Vtx10(fx16 x, fx16 y, fx16 z) {
    reg_G3_VTX_10 = (u32)((x >> 6) & 0x3ff) | ((u32)((y >> 6) & 0x3ff) << 10) | ((u32)((z >> 6) & 0x3ff) << 20);
}

// The BG registers of both engines. The sub engine's are at the main engine's address plus 0x1000
#define reg_G2_BG1CNT (*(vu16 *)0x0400000a)
#define reg_G2_BG2CNT (*(vu16 *)0x0400000c)
#define reg_G2_BG3CNT (*(vu16 *)0x0400000e)
#define reg_G2_BG0OFS (*(vu32 *)0x04000010)
#define reg_G2_BG1OFS (*(vu32 *)0x04000014)
#define reg_G2_BG2OFS (*(vu32 *)0x04000018)
#define reg_G2_BG3OFS (*(vu32 *)0x0400001c)
#define reg_G2_BG2PA (*(vu16 *)0x04000020)
#define reg_G2_BG3PA (*(vu16 *)0x04000030)
#define reg_G2S_DB_BG0CNT (*(vu16 *)0x04001008)
#define reg_G2S_DB_BG1CNT (*(vu16 *)0x0400100a)
#define reg_G2S_DB_BG2CNT (*(vu16 *)0x0400100c)
#define reg_G2S_DB_BG3CNT (*(vu16 *)0x0400100e)
#define reg_G2S_DB_BG0OFS (*(vu32 *)0x04001010)
#define reg_G2S_DB_BG1OFS (*(vu32 *)0x04001014)
#define reg_G2S_DB_BG2OFS (*(vu32 *)0x04001018)
#define reg_G2S_DB_BG3OFS (*(vu32 *)0x0400101c)
#define reg_G2S_DB_BG2PA (*(vu16 *)0x04001020)
#define reg_G2S_DB_BG3PA (*(vu16 *)0x04001030)

// The fields of a BG control register. Bit 13 is the extended palette slot of BG 0 and 1, and the area overflow
// mode of BG 2 and 3
#define REG_G2_BGCNT_CHARBASE_SHIFT 2
#define REG_G2_BGCNT_CHARBASE_MASK 0x003c
#define REG_G2_BGCNT_MOSAIC_MASK 0x0040
#define REG_G2_BGCNT_COLORMODE_SHIFT 7
#define REG_G2_BGCNT_COLORMODE_MASK 0x0080
#define REG_G2_BGCNT_SCREENBASE_SHIFT 8
#define REG_G2_BGCNT_SCREENBASE_MASK 0x1f00
#define REG_G2_BGCNT_BGPLTTSLOT_SHIFT 13
#define REG_G2_BGCNT_AREAOVER_SHIFT 13
#define REG_G2_BGCNT_SCREENSIZE_SHIFT 14

#define REG_G2_BGOFS_HOFFSET_MASK 0x000001ff
#define REG_G2_BGOFS_VOFFSET_SHIFT 16
#define REG_G2_BGOFS_VOFFSET_MASK 0x01ff0000

// The OBJ character boundary of 1D mapping, as a shift of 32 bytes
#define REG_GX_DISPCNT_EXOBJ_SHIFT 20
#define REG_GX_DISPCNT_EXOBJ_MASK 0x00700000
#define REG_GX_DISPCNT_BGCHAROFFSET_SHIFT 24
#define REG_GX_DISPCNT_BGCHAROFFSET_MASK 0x07000000
#define REG_GX_DISPCNT_BGSCREENOFFSET_SHIFT 27
#define REG_GX_DISPCNT_BGSCREENOFFSET_MASK 0x38000000

#define GX_BGSCROFFSET_0x00000 0
#define GX_BGCHAROFFSET_0x00000 0

// The screen sizes of each kind of BG, as the control registers take them
typedef enum {
    GX_BG_SCRSIZE_TEXT_256x256 = 0,
    GX_BG_SCRSIZE_TEXT_512x256 = 1,
    GX_BG_SCRSIZE_TEXT_256x512 = 2,
    GX_BG_SCRSIZE_TEXT_512x512 = 3,
} GXBGScrSizeText;

typedef enum {
    GX_BG_SCRSIZE_AFFINE_128x128 = 0,
    GX_BG_SCRSIZE_AFFINE_256x256 = 1,
    GX_BG_SCRSIZE_AFFINE_512x512 = 2,
    GX_BG_SCRSIZE_AFFINE_1024x1024 = 3,
} GXBGScrSizeAffine;

typedef enum {
    GX_BG_SCRSIZE_256x16PLTT_128x128 = 0,
    GX_BG_SCRSIZE_256x16PLTT_256x256 = 1,
    GX_BG_SCRSIZE_256x16PLTT_512x512 = 2,
    GX_BG_SCRSIZE_256x16PLTT_1024x1024 = 3,
} GXBGScrSize256x16Pltt;

#define GX_BG_EXTMODE_256x16PLTT 0
#define GX_BG_EXTMODE_DCBMP 0x84

typedef union {
    u16 raw;
    struct {
        u16 priority : 2;
        u16 charBase : 4;
        u16 mosaic : 1;
        u16 colorMode : 1;
        u16 screenBase : 5;
        u16 bgExtPltt : 1;
        u16 screenSize : 2;
    };
} GXBg01Control;

typedef union {
    u16 raw;
    struct {
        u16 priority : 2;
        u16 charBase : 4;
        u16 mosaic : 1;
        u16 colorMode : 1;
        u16 screenBase : 5;
        u16 _reserve : 1;
        u16 screenSize : 2;
    };
} GXBg23ControlText;

typedef union {
    u16 raw;
    struct {
        u16 priority : 2;
        u16 charBase : 4;
        u16 mosaic : 1;
        u16 _reserve : 1;
        u16 screenBase : 5;
        u16 areaOver : 1;
        u16 screenSize : 2;
    };
} GXBg23ControlAffine;

typedef union {
    u16 raw;
    struct {
        u16 priority : 2;
        u16 _reserve1 : 1;
        u16 charBase : 3;
        u16 mosaic : 1;
        u16 _reserve2 : 1;
        u16 screenBase : 5;
        u16 areaOver : 1;
        u16 screenSize : 2;
    };
} GXBg23Control256x16Pltt;

static inline void GX_SetBGScrOffset(int offset) {
    reg_GX_DISPCNT =
        (u32)((reg_GX_DISPCNT & ~REG_GX_DISPCNT_BGSCREENOFFSET_MASK) | (offset << REG_GX_DISPCNT_BGSCREENOFFSET_SHIFT));
}

static inline void GX_SetBGCharOffset(int offset) {
    reg_GX_DISPCNT =
        (u32)((reg_GX_DISPCNT & ~REG_GX_DISPCNT_BGCHAROFFSET_MASK) | (offset << REG_GX_DISPCNT_BGCHAROFFSET_SHIFT));
}

// NitroSDK has an inline function of each of these for each BG of each engine, such as G2_SetBG0Control and
// G2S_SetBG0Control. They are generated here from the register

#define GX_DEFINE_BG01_CONTROL(name, reg)                                                                              \
    static inline void name(GXBGScrSizeText screenSize, GXBGColorMode colorMode, GXBGScrBase screenBase,               \
                            GXBGCharBase charBase, GXBGExtPltt bgExtPltt) {                                            \
        reg = (u16)((reg & (REG_G2_BG0CNT_PRIORITY_MASK | REG_G2_BGCNT_MOSAIC_MASK)) |                                 \
                    (screenSize << REG_G2_BGCNT_SCREENSIZE_SHIFT) | (colorMode << REG_G2_BGCNT_COLORMODE_SHIFT) |      \
                    (screenBase << REG_G2_BGCNT_SCREENBASE_SHIFT) | (charBase << REG_G2_BGCNT_CHARBASE_SHIFT) |        \
                    (bgExtPltt << REG_G2_BGCNT_BGPLTTSLOT_SHIFT));                                                     \
    }

#define GX_DEFINE_BG23_CONTROL_TEXT(name, reg)                                                                         \
    static inline void name(GXBGScrSizeText screenSize, GXBGColorMode colorMode, GXBGScrBase screenBase,               \
                            GXBGCharBase charBase) {                                                                   \
        reg = (u16)((reg & (REG_G2_BG0CNT_PRIORITY_MASK | REG_G2_BGCNT_MOSAIC_MASK)) |                                 \
                    (screenSize << REG_G2_BGCNT_SCREENSIZE_SHIFT) | (colorMode << REG_G2_BGCNT_COLORMODE_SHIFT) |      \
                    (screenBase << REG_G2_BGCNT_SCREENBASE_SHIFT) | (charBase << REG_G2_BGCNT_CHARBASE_SHIFT));        \
    }

#define GX_DEFINE_BG23_CONTROL_AFFINE(name, reg)                                                                       \
    static inline void name(GXBGScrSizeAffine screenSize, GXBGAreaOver areaOver, GXBGScrBase screenBase,               \
                            GXBGCharBase charBase) {                                                                   \
        reg = (u16)((reg & (REG_G2_BG0CNT_PRIORITY_MASK | REG_G2_BGCNT_MOSAIC_MASK)) |                                 \
                    (screenSize << REG_G2_BGCNT_SCREENSIZE_SHIFT) | (screenBase << REG_G2_BGCNT_SCREENBASE_SHIFT) |    \
                    (charBase << REG_G2_BGCNT_CHARBASE_SHIFT) | (areaOver << REG_G2_BGCNT_AREAOVER_SHIFT));            \
    }

#define GX_DEFINE_BG23_CONTROL_256x16PLTT(name, reg)                                                                   \
    static inline void name(GXBGScrSize256x16Pltt screenSize, GXBGAreaOver areaOver, GXBGScrBase screenBase,           \
                            GXBGCharBase charBase) {                                                                   \
        reg = (u16)((reg & (REG_G2_BG0CNT_PRIORITY_MASK | REG_G2_BGCNT_MOSAIC_MASK)) |                                 \
                    (screenSize << REG_G2_BGCNT_SCREENSIZE_SHIFT) | (charBase << REG_G2_BGCNT_CHARBASE_SHIFT) |        \
                    GX_BG_EXTMODE_256x16PLTT | (screenBase << REG_G2_BGCNT_SCREENBASE_SHIFT) |                         \
                    (areaOver << REG_G2_BGCNT_AREAOVER_SHIFT));                                                        \
    }

#define GX_DEFINE_BG23_CONTROL_DCBMP(name, reg)                                                                        \
    static inline void name(GXBGScrSizeDcBmp screenSize, GXBGAreaOver areaOver, GXBGBmpScrBase screenBase) {           \
        reg = (u16)((reg & (REG_G2_BG0CNT_PRIORITY_MASK | REG_G2_BGCNT_MOSAIC_MASK)) |                                 \
                    (screenSize << REG_G2_BGCNT_SCREENSIZE_SHIFT) | GX_BG_EXTMODE_DCBMP |                              \
                    (screenBase << REG_G2_BGCNT_SCREENBASE_SHIFT) | (areaOver << REG_G2_BGCNT_AREAOVER_SHIFT));        \
    }

#define GX_DEFINE_BG_PRIORITY(name, reg)                                                                               \
    static inline void name(int priority) {                                                                            \
        reg = (u16)((reg & ~REG_G2_BG0CNT_PRIORITY_MASK) | (priority << REG_G2_BG0CNT_PRIORITY_SHIFT));                \
    }

#define GX_DEFINE_BG_MOSAIC(name, reg)                                                                                 \
    static inline void name(BOOL enable) {                                                                             \
        if (enable) {                                                                                                  \
            reg |= REG_G2_BGCNT_MOSAIC_MASK;                                                                           \
        } else {                                                                                                       \
            reg &= ~REG_G2_BGCNT_MOSAIC_MASK;                                                                          \
        }                                                                                                              \
    }

#define GX_DEFINE_BG_OFFSET(name, reg)                                                                                 \
    static inline void name(int hOffset, int vOffset) {                                                                \
        reg = (u32)((hOffset & REG_G2_BGOFS_HOFFSET_MASK) |                                                            \
                    ((vOffset << REG_G2_BGOFS_VOFFSET_SHIFT) & REG_G2_BGOFS_VOFFSET_MASK));                            \
    }

#define GX_DEFINE_BG_GET_CONTROL(name, type, reg)                                                                      \
    static inline type name(void) {                                                                                    \
        return *(volatile type *)&reg;                                                                                 \
    }

// clang-format off
GX_DEFINE_BG01_CONTROL(G2_SetBG0Control, reg_G2_BG0CNT)
GX_DEFINE_BG01_CONTROL(G2_SetBG1Control, reg_G2_BG1CNT)
GX_DEFINE_BG01_CONTROL(G2S_SetBG0Control, reg_G2S_DB_BG0CNT)
GX_DEFINE_BG01_CONTROL(G2S_SetBG1Control, reg_G2S_DB_BG1CNT)

GX_DEFINE_BG23_CONTROL_TEXT(G2_SetBG2ControlText, reg_G2_BG2CNT)
GX_DEFINE_BG23_CONTROL_TEXT(G2_SetBG3ControlText, reg_G2_BG3CNT)
GX_DEFINE_BG23_CONTROL_TEXT(G2S_SetBG2ControlText, reg_G2S_DB_BG2CNT)
GX_DEFINE_BG23_CONTROL_TEXT(G2S_SetBG3ControlText, reg_G2S_DB_BG3CNT)

GX_DEFINE_BG23_CONTROL_AFFINE(G2_SetBG2ControlAffine, reg_G2_BG2CNT)
GX_DEFINE_BG23_CONTROL_AFFINE(G2_SetBG3ControlAffine, reg_G2_BG3CNT)
GX_DEFINE_BG23_CONTROL_AFFINE(G2S_SetBG2ControlAffine, reg_G2S_DB_BG2CNT)
GX_DEFINE_BG23_CONTROL_AFFINE(G2S_SetBG3ControlAffine, reg_G2S_DB_BG3CNT)

GX_DEFINE_BG23_CONTROL_256x16PLTT(G2_SetBG2Control256x16Pltt, reg_G2_BG2CNT)
GX_DEFINE_BG23_CONTROL_256x16PLTT(G2_SetBG3Control256x16Pltt, reg_G2_BG3CNT)
GX_DEFINE_BG23_CONTROL_256x16PLTT(G2S_SetBG2Control256x16Pltt, reg_G2S_DB_BG2CNT)
GX_DEFINE_BG23_CONTROL_256x16PLTT(G2S_SetBG3Control256x16Pltt, reg_G2S_DB_BG3CNT)

GX_DEFINE_BG23_CONTROL_DCBMP(G2_SetBG2ControlDCBmp, reg_G2_BG2CNT)
GX_DEFINE_BG23_CONTROL_DCBMP(G2_SetBG3ControlDCBmp, reg_G2_BG3CNT)
GX_DEFINE_BG23_CONTROL_DCBMP(G2S_SetBG2ControlDCBmp, reg_G2S_DB_BG2CNT)
GX_DEFINE_BG23_CONTROL_DCBMP(G2S_SetBG3ControlDCBmp, reg_G2S_DB_BG3CNT)

GX_DEFINE_BG_PRIORITY(G2_SetBG0Priority, reg_G2_BG0CNT)
GX_DEFINE_BG_PRIORITY(G2_SetBG1Priority, reg_G2_BG1CNT)
GX_DEFINE_BG_PRIORITY(G2_SetBG2Priority, reg_G2_BG2CNT)
GX_DEFINE_BG_PRIORITY(G2_SetBG3Priority, reg_G2_BG3CNT)
GX_DEFINE_BG_PRIORITY(G2S_SetBG0Priority, reg_G2S_DB_BG0CNT)
GX_DEFINE_BG_PRIORITY(G2S_SetBG1Priority, reg_G2S_DB_BG1CNT)
GX_DEFINE_BG_PRIORITY(G2S_SetBG2Priority, reg_G2S_DB_BG2CNT)
GX_DEFINE_BG_PRIORITY(G2S_SetBG3Priority, reg_G2S_DB_BG3CNT)

GX_DEFINE_BG_MOSAIC(G2_BG0Mosaic, reg_G2_BG0CNT)
GX_DEFINE_BG_MOSAIC(G2_BG1Mosaic, reg_G2_BG1CNT)
GX_DEFINE_BG_MOSAIC(G2_BG2Mosaic, reg_G2_BG2CNT)
GX_DEFINE_BG_MOSAIC(G2_BG3Mosaic, reg_G2_BG3CNT)
GX_DEFINE_BG_MOSAIC(G2S_BG0Mosaic, reg_G2S_DB_BG0CNT)
GX_DEFINE_BG_MOSAIC(G2S_BG1Mosaic, reg_G2S_DB_BG1CNT)
GX_DEFINE_BG_MOSAIC(G2S_BG2Mosaic, reg_G2S_DB_BG2CNT)
GX_DEFINE_BG_MOSAIC(G2S_BG3Mosaic, reg_G2S_DB_BG3CNT)
// clang-format on

#define REG_G2_MOSAIC_BG_V_SHIFT 4

                                                                                static inline void G2_SetBGMosaicSize(
                                                                                    int hSize, int vSize) {
    *(vu8 *)REG_MOSAIC_ADDR = (u8)(hSize | (vSize << REG_G2_MOSAIC_BG_V_SHIFT));
}

static inline void G2S_SetBGMosaicSize(int hSize, int vSize) {
    *(vu8 *)REG_DB_MOSAIC_ADDR = (u8)(hSize | (vSize << REG_G2_MOSAIC_BG_V_SHIFT));
}
// clang-format off

GX_DEFINE_BG_OFFSET(G2_SetBG0Offset, reg_G2_BG0OFS)
GX_DEFINE_BG_OFFSET(G2_SetBG1Offset, reg_G2_BG1OFS)
GX_DEFINE_BG_OFFSET(G2_SetBG2Offset, reg_G2_BG2OFS)
GX_DEFINE_BG_OFFSET(G2_SetBG3Offset, reg_G2_BG3OFS)
GX_DEFINE_BG_OFFSET(G2S_SetBG0Offset, reg_G2S_DB_BG0OFS)
GX_DEFINE_BG_OFFSET(G2S_SetBG1Offset, reg_G2S_DB_BG1OFS)
GX_DEFINE_BG_OFFSET(G2S_SetBG2Offset, reg_G2S_DB_BG2OFS)
GX_DEFINE_BG_OFFSET(G2S_SetBG3Offset, reg_G2S_DB_BG3OFS)

GX_DEFINE_BG_GET_CONTROL(G2_GetBG0Control, GXBg01Control, reg_G2_BG0CNT)
GX_DEFINE_BG_GET_CONTROL(G2_GetBG1Control, GXBg01Control, reg_G2_BG1CNT)
GX_DEFINE_BG_GET_CONTROL(G2_GetBG2ControlText, GXBg23ControlText, reg_G2_BG2CNT)
GX_DEFINE_BG_GET_CONTROL(G2_GetBG2ControlAffine, GXBg23ControlAffine, reg_G2_BG2CNT)
GX_DEFINE_BG_GET_CONTROL(G2_GetBG2Control256x16Pltt, GXBg23Control256x16Pltt, reg_G2_BG2CNT)
GX_DEFINE_BG_GET_CONTROL(G2_GetBG3ControlText, GXBg23ControlText, reg_G2_BG3CNT)
GX_DEFINE_BG_GET_CONTROL(G2_GetBG3ControlAffine, GXBg23ControlAffine, reg_G2_BG3CNT)
GX_DEFINE_BG_GET_CONTROL(G2_GetBG3Control256x16Pltt, GXBg23Control256x16Pltt, reg_G2_BG3CNT)
GX_DEFINE_BG_GET_CONTROL(G2S_GetBG0Control, GXBg01Control, reg_G2S_DB_BG0CNT)
GX_DEFINE_BG_GET_CONTROL(G2S_GetBG1Control, GXBg01Control, reg_G2S_DB_BG1CNT)
GX_DEFINE_BG_GET_CONTROL(G2S_GetBG2ControlText, GXBg23ControlText, reg_G2S_DB_BG2CNT)
GX_DEFINE_BG_GET_CONTROL(G2S_GetBG2ControlAffine, GXBg23ControlAffine, reg_G2S_DB_BG2CNT)
GX_DEFINE_BG_GET_CONTROL(G2S_GetBG2Control256x16Pltt, GXBg23Control256x16Pltt, reg_G2S_DB_BG2CNT)
GX_DEFINE_BG_GET_CONTROL(G2S_GetBG3ControlText, GXBg23ControlText, reg_G2S_DB_BG3CNT)
GX_DEFINE_BG_GET_CONTROL(G2S_GetBG3ControlAffine, GXBg23ControlAffine, reg_G2S_DB_BG3CNT)
GX_DEFINE_BG_GET_CONTROL(G2S_GetBG3Control256x16Pltt, GXBg23Control256x16Pltt, reg_G2S_DB_BG3CNT)
// clang-format on

// Sets a BG's affine matrix, its center and its offset. NitroSDK's G2x_SetBGyAffine_
void gfxRegSetBGTransform(u32 addr, const MtxFx22 *mtx, int centerX, int centerY, int x, int y);

static inline void G2_SetBG2Affine(const MtxFx22 *mtx, int centerX, int centerY, int x, int y) {
    gfxRegSetBGTransform((u32)&reg_G2_BG2PA, mtx, centerX, centerY, x, y);
}

static inline void G2_SetBG3Affine(const MtxFx22 *mtx, int centerX, int centerY, int x, int y) {
    gfxRegSetBGTransform((u32)&reg_G2_BG3PA, mtx, centerX, centerY, x, y);
}

static inline void G2S_SetBG2Affine(const MtxFx22 *mtx, int centerX, int centerY, int x, int y) {
    gfxRegSetBGTransform((u32)&reg_G2S_DB_BG2PA, mtx, centerX, centerY, x, y);
}

static inline void G2S_SetBG3Affine(const MtxFx22 *mtx, int centerX, int centerY, int x, int y) {
    gfxRegSetBGTransform((u32)&reg_G2S_DB_BG3PA, mtx, centerX, centerY, x, y);
}

// NitroSDK's GX_SetGraphicsMode and GXS_SetGraphicsMode
// NitroSDK's GX_BeginLoadTex, GX_LoadTex and GX_EndLoadTex, and the same for palettes. The destination is an offset
// into the memory of the textures or the palettes (gx_load3d.c)
void gfxBeginTextureUpload(void);
void gfxUploadTexture(const void *src, u32 offset, u32 size);
void gfxEndTextureUpload(void);
void gfxBeginPaletteUpload(void);
void gfxUploadPalette(const void *src, u32 offset, u32 size);
void gfxEndPaletteUpload(void);
// NitroSDK's GX_BeginLoadClearImage, GX_LoadClearImageColor, GX_LoadClearImageDepth and GX_EndLoadClearImage
void gfxBeginRearPlaneImageUpload(void);
void gfxUploadRearPlaneImageA(const void *src, u32 size);
void gfxUploadRearPlaneImageB(const void *src, u32 size);
void gfxEndRearPlaneImageUpload(void);
// NitroSDK's G3_LoadMtx43, G3_MultMtx43 and G3_MultMtx33
void gfxLoadMatrix4x3(const MtxFx43 *mtx);
void gfxMultMatrix4x3(const MtxFx43 *mtx);
void gfxMultMatrix3x3(const MtxFx33 *mtx);
void gfxSetEngineModeA(int dispMode, int bgMode, int bg0As3D);
void gfxSetBGModeB(int bgMode);
// NitroSDK's GX_HBlankIntr and GX_VBlankIntr, which return whether the interrupt was enabled
s32 gfxSetHBlankIRQEnabled(BOOL enable);
s32 gfxSetVBlankIRQEnabled(BOOL enable);

// NitroSDK's loads to BG VRAM: GX_LoadBG0Scr to GXS_LoadBG3Scr, GX_LoadBG0Char to GXS_LoadBG3Char, and GX_LoadBGPltt
// and GXS_LoadBGPltt
void gfxUploadBGScreen0A(const void *src, u32 offset, u32 size);
void gfxUploadBGScreen1A(const void *src, u32 offset, u32 size);
void gfxUploadBGScreen2A(const void *src, u32 offset, u32 size);
void gfxUploadBGScreen3A(const void *src, u32 offset, u32 size);
void gfxUploadBGScreen0B(const void *src, u32 offset, u32 size);
void gfxUploadBGScreen1B(const void *src, u32 offset, u32 size);
void gfxUploadBGScreen2B(const void *src, u32 offset, u32 size);
void gfxUploadBGScreen3B(const void *src, u32 offset, u32 size);
void gfxUploadBGChar0A(const void *src, u32 offset, u32 size);
void gfxUploadBGChar1A(const void *src, u32 offset, u32 size);
void gfxUploadBGChar2A(const void *src, u32 offset, u32 size);
void gfxUploadBGChar3A(const void *src, u32 offset, u32 size);
void gfxUploadBGChar0B(const void *src, u32 offset, u32 size);
void gfxUploadBGChar1B(const void *src, u32 offset, u32 size);
void gfxUploadBGChar2B(const void *src, u32 offset, u32 size);
void gfxUploadBGChar3B(const void *src, u32 offset, u32 size);
void gfxUploadStdPaletteBGA(const void *src, u32 offset, u32 size);
void gfxUploadStdPaletteBGB(const void *src, u32 offset, u32 size);

// NitroSDK's GX_LoadOBJPltt, GXS_LoadOBJPltt, GX_LoadOBJ and GXS_LoadOBJ
void gfxUploadStdPaletteObjA(const void *src, u32 offset, u32 size);
void gfxUploadStdPaletteObjB(const void *src, u32 offset, u32 size);
void gfxUploadObjCharA(const void *src, u32 offset, u32 size);
void gfxUploadObjCharB(const void *src, u32 offset, u32 size);
// The DMA channel that GX loads and fills with, or GX_DMA_NOT_USE for the CPU. swan calls it g_GfxDMANo
extern u32 GXi_DmaId;

#define GX_DMA_NOT_USE 0xffffffff

// Fills by DMA on GX's channel, or with the CPU when it has none
static inline void GXi_DmaFill32(u32 dmaNo, void *dest, u32 data, u32 size) {
    if (dmaNo > MI_DMA_MAX_NUM) {
        dmaNo = GX_DMA_NOT_USE;
    }
    if (dmaNo != GX_DMA_NOT_USE) {
        dma_fill(dmaNo, dest, data, size, TRUE);
    } else {
        sys_memset32(data, dest, size);
    }
}

// NitroSDK's GX_LoadOAM and GXS_LoadOAM
void gfxUploadOAMA(const void *src, u32 offset, u32 size);
void gfxUploadOAMB(const void *src, u32 offset, u32 size);

// NitroSDK's loads to extended palette VRAM: GX_BeginLoadBGExtPltt, GX_LoadBGExtPltt and GX_EndLoadBGExtPltt, and the
// same for OBJ and for the sub engine
void gfxBeginBGExtPltAUpload(void);
void gfxUploadExtPaletteBGA(const void *src, u32 offset, u32 size);
void gfxEndBGExtPltAUpload(void);
void gfxBeginObjExtPltAUpload(void);
void gfxUploadExtPaletteObjA(const void *src, u32 offset, u32 size);
void gfxEndObjExtPltAUpload(void);
void gfxBeginBGExtPltBUpload(void);
void gfxUploadExtPaletteBGB(const void *src, u32 offset, u32 size);
void gfxEndBGExtPltBUpload(void);
void gfxBeginObjExtPltBUpload(void);
void gfxUploadExtPaletteObjB(const void *src, u32 offset, u32 size);
void gfxEndObjExtPltBUpload(void);

// NitroSDK's GX_GetBankForOBJ and GX_GetBankForSubOBJ
u16 gfxGetObjBanksA(void);
u16 gfxGetObjBanksB(void);

// NitroSDK's G2_GetBG0ScrPtr to G2_GetBG3ScrPtr and G2S_GetBG0ScrPtr to G2S_GetBG3ScrPtr (gx_bgcnt.c)
void *gfxGetScreenAddrBG0A(void);
void *gfxGetScreenAddrBG1A(void);
void *gfxGetScreenAddrBG2A(void);
void *gfxGetScreenAddrBG3A(void);
void *gfxGetScreenAddrBG0B(void);
void *gfxGetScreenAddrBG1B(void);
void *gfxGetScreenAddrBG2B(void);
void *gfxGetScreenAddrBG3B(void);

// NitroSDK's G2_GetBG0CharPtr to G2S_GetBG3CharPtr
void *gfxGetCharAddrBG0A(void);
void *gfxGetCharAddrBG1A(void);
void *gfxGetCharAddrBG2A(void);
void *gfxGetCharAddrBG3A(void);
void *gfxGetCharAddrBG0B(void);
void *gfxGetCharAddrBG1B(void);
void *gfxGetCharAddrBG2B(void);
void *gfxGetCharAddrBG3B(void);

// NitroSDK libgx's gx_vramcnt.c, under swan's names: gfxSet<Use>Banks is NitroSDK's GX_SetBankFor<Use>,
// gfxAcquire<Use>Banks its GX_ResetBankFor<Use>, gfxDisable<Use>Banks its GX_DisableBankFor<Use> and
// gfxGet<Use>Banks its GX_GetBankFor<Use>. The uses are BG (the SDK's BG), Obj (OBJ), BGExtPlt (BGExtPltt), ObjExtPlt
// (OBJExtPltt), Texture (Tex), Palette (TexPltt), RearPlaneImage (ClearImage), PlainARM7 (ARM7) and LCDC; the sub
// engine's, with a B, are SubBG, SubOBJ, SubBGExtPltt and SubOBJExtPltt. Set takes the banks for a use, which the
// enums above name, and puts the ones it had back to LCDC mode; Acquire (Reset) gives the banks of a use back to LCDC
// mode and returns them; Disable turns them off and unlocks them for the lock ID that GX_Init took. The Acquire and
// Disable of an extended palette also turn it off in DISPCNT
void gfxSetLCDCBanks(u32 banks);
void gfxSetBGBanksA(GXVRamBG banks);
void gfxSetObjBanksA(GXVRamOBJ banks);
void gfxSetBGExtPltBanksA(GXVRamBGExtPltt banks);
void gfxSetObjExtPltBanksA(GXVRamOBJExtPltt banks);
void gfxSetTextureBanks(GXVRamTex banks);
void gfxSetPaletteBanks(GXVRamTexPltt banks);
void gfxSetRearPlaneImageBanks(GXVRamClearImage banks);
void gfxSetPlainARM7Banks(GXVRamARM7 banks);
void gfxSetBGBanksB(GXVRamSubBG banks);
void gfxSetObjBanksB(GXVRamSubOBJ banks);
void gfxSetBGExtPltBanksB(GXVRamSubBGExtPltt banks);
void gfxSetObjExtPltBanksB(GXVRamSubOBJExtPltt banks);
u32 gfxAcquireBGBanksA(void);
u32 gfxAcquireObjBanksA(void);
GXVRamBGExtPltt gfxAcquireBGExtPltBanksA(void);
GXVRamOBJExtPltt gfxAcquireObjExtPltBanksA(void);
// Returns the banks the textures had
u32 gfxAcquireTextureBanks(void);
GXVRamTexPltt gfxAcquirePaletteBanks(void);
u32 gfxAcquireRearPlaneImageBanks(void);
u32 gfxAcquireBGBanksB(void);
u32 gfxAcquireObjBanksB(void);
u32 gfxAcquireBGExtPltBanksB(void);
u32 gfxAcquireObjExtPltBanksB(void);
void gfxDisableBGBanksA(void);
void gfxDisableObjBanksA(void);
void gfxDisableBGExtPltBanksA(void);
void gfxDisableObjExtPltBanksA(void);
void gfxDisableTextureBanks(void);
void gfxDisablePaletteBanks(void);
void gfxDisableRearPlaneImageBanks(void);
void gfxDisablePlainARM7Banks(void);
void gfxDisableLCDCBanks(void);
void gfxDisableBGBanksB(void);
void gfxDisableObjBanksB(void);
void gfxDisableBGExtPltBanksB(void);
void gfxDisableObjExtPltBanksB(void);
u32 gfxGetLCDCBanks(void);
u32 gfxGetTextureBanks(void);
u32 gfxGetObjExtPltBanksA(void);
u32 gfxGetObjExtPltBanksB(void);

// NitroSDK's GX_DispOn
void gfxEngineEnableA(void);

#define GX_VRAM_LCDC_ALL 0x1ff

// The control registers of the VRAM banks: one byte each (WRAMCNT is between G and H). A bank is given to a use by its
// master select, and some uses take an offset to tell the banks of the same use apart
#define reg_GX_VRAMCNT_A (*(vu8 *)0x04000240)
#define reg_GX_VRAMCNT_B (*(vu8 *)0x04000241)
#define reg_GX_VRAMCNT_C (*(vu8 *)0x04000242)
#define reg_GX_VRAMCNT_D (*(vu8 *)0x04000243)
#define reg_GX_VRAMCNT_E (*(vu8 *)0x04000244)
#define reg_GX_VRAMCNT_F (*(vu8 *)0x04000245)
#define reg_GX_VRAMCNT_G (*(vu8 *)0x04000246)
#define reg_GX_VRAMCNT_H (*(vu8 *)0x04000248)
#define reg_GX_VRAMCNT_I (*(vu8 *)0x04000249)
// A to D together, and H and I together
#define reg_GX_VRAMCNT_ABCD (*(vu32 *)0x04000240)
#define reg_GX_VRAMCNT_HI (*(vu16 *)0x04000248)
#define REG_GX_VRAMCNT_MST_SHIFT 0
#define REG_GX_VRAMCNT_OFS_SHIFT 3
#define REG_GX_VRAMCNT_E_MASK 0x80
#define GX_VRAMCNT(mst, ofs) ((u8)(REG_GX_VRAMCNT_E_MASK | ((ofs) << REG_GX_VRAMCNT_OFS_SHIFT) | (mst)))
// The bank in LCDC mode: the CPU sees it at its HW_LCDC_VRAM address
#define GX_VRAMCNT_LCDC REG_GX_VRAMCNT_E_MASK

// The bits of DISPCNT and DISP3DCNT that the banks of the extended palettes and of the textures turn on
#define REG_GX_DISPCNT_BGEXTPLTT_MASK 0x40000000
#define REG_GX_DISPCNT_OBJEXTPLTT_MASK 0x80000000
#define REG_G3X_DISP3DCNT_TME_MASK 0x0001
#define REG_G3X_DISP3DCNT_CLRIMG_MASK 0x4000

// The state of the VRAM banks that NitroSDK keeps in gGXState (gx_state.c): the banks that each use has taken, and the
// free ones, which are in LCDC mode. A bank is in only one of the fields
typedef struct GXState {
    u16 lcdc;
    u16 bg;
    u16 obj;
    u16 arm7;
    u16 tex;
    u16 texPltt;
    u16 clrImg;
    u16 bgExtPltt;
    u16 objExtPltt;
    u16 subBg;
    u16 subObj;
    u16 subBgExtPltt;
    u16 subObjExtPltt;
} GXState;

extern GXState gGXState;

// VRAM as the CPU sees it with every bank given to it, and OAM
#define HW_LCDC_VRAM 0x06800000
#define HW_LCDC_VRAM_SIZE 0xa4000
#define HW_LCDC_VRAM_A 0x06800000
#define HW_LCDC_VRAM_B 0x06820000
#define HW_LCDC_VRAM_C 0x06840000
#define HW_LCDC_VRAM_D 0x06860000
#define HW_LCDC_VRAM_E 0x06880000
#define HW_LCDC_VRAM_F 0x06890000
#define HW_LCDC_VRAM_G 0x06894000
#define HW_LCDC_VRAM_H 0x06898000
#define HW_LCDC_VRAM_I 0x068a0000
// The BG and OBJ VRAM of each engine, at their largest
#define HW_BG_VRAM 0x06000000
#define HW_BG_VRAM_SIZE 0x80000
#define HW_DB_BG_VRAM 0x06200000
#define HW_DB_BG_VRAM_SIZE 0x20000
#define HW_OBJ_VRAM_SIZE 0x40000
// The sub engine's OBJ characters
#define HW_DB_OBJ_VRAM 0x06600000
#define HW_DB_OBJ_VRAM_SIZE 0x20000
#define HW_OAM 0x07000000
#define HW_DB_OAM 0x07000400
#define HW_OAM_SIZE 0x400
#define HW_PLTT_SIZE 0x400

#define REG_GX_DISPCNT_DISPLAY_SHIFT 8
#define REG_GX_DISPCNT_DISPLAY_MASK 0x00001f00
#define REG_GX_DISPCNT_OBJMAP_CH_MASK 0x00000010
#define REG_GX_DISPCNT_EXOBJ_CH_MASK 0x00300000
#define REG_GXS_DB_DISPCNT_MODE_MASK 0x00010000

static inline void GX_SetVisiblePlane(int plane) {
    reg_GX_DISPCNT = (u32)((reg_GX_DISPCNT & ~REG_GX_DISPCNT_DISPLAY_MASK) | (plane << REG_GX_DISPCNT_DISPLAY_SHIFT));
}

static inline void GXS_SetVisiblePlane(int plane) {
    reg_GXS_DB_DISPCNT =
        (u32)((reg_GXS_DB_DISPCNT & ~REG_GX_DISPCNT_DISPLAY_MASK) | (plane << REG_GX_DISPCNT_DISPLAY_SHIFT));
}

static inline void GX_SetOBJVRamModeChar(GXOBJVRamModeChar mode) {
    reg_GX_DISPCNT = (u32)((reg_GX_DISPCNT & ~(REG_GX_DISPCNT_EXOBJ_CH_MASK | REG_GX_DISPCNT_OBJMAP_CH_MASK)) | mode);
}

static inline void GXS_SetOBJVRamModeChar(GXOBJVRamModeChar mode) {
    reg_GXS_DB_DISPCNT =
        (u32)((reg_GXS_DB_DISPCNT & ~(REG_GX_DISPCNT_EXOBJ_CH_MASK | REG_GX_DISPCNT_OBJMAP_CH_MASK)) | mode);
}

static inline GXOBJVRamModeChar GX_GetOBJVRamModeChar(void) {
    return (GXOBJVRamModeChar)(reg_GX_DISPCNT & (REG_GX_DISPCNT_EXOBJ_CH_MASK | REG_GX_DISPCNT_OBJMAP_CH_MASK));
}

static inline GXOBJVRamModeChar GXS_GetOBJVRamModeChar(void) {
    return (GXOBJVRamModeChar)(reg_GXS_DB_DISPCNT & (REG_GX_DISPCNT_EXOBJ_CH_MASK | REG_GX_DISPCNT_OBJMAP_CH_MASK));
}

#define REG_GX_DISPCNT_OBJMAP_BM_MASK 0x00000060

static inline void GXS_SetOBJVRamModeBmp(GXOBJVRamModeBmp mode) {
    reg_GXS_DB_DISPCNT = (u32)((reg_GXS_DB_DISPCNT & ~REG_GX_DISPCNT_OBJMAP_BM_MASK) | mode);
}

static inline void GXS_DispOn(void) {
    reg_GXS_DB_DISPCNT |= REG_GXS_DB_DISPCNT_MODE_MASK;
}

// NitroSDK's G3X_Reset, G3X_InitMtxStack and G3X_GetBoxTestResult, under swan's names. The box test result is 0 when
// the box is outside the view, and the function returns nonzero while the test is still running
void gfxReset3D(void);
// NitroSDK's G3X_Init, under swan's name
void gfxInit3D(void);
// NitroSDK's G3X_GetClipMtx and G3X_GetVectorMtx, under swan's names: copy the clip matrix (the position matrix with
// the projection matrix applied) or the vector matrix that the geometry engine last made into dest. Both return -1
// without copying while the engine is busy, and 0 once they have
int gfxGetClipMatrix(MtxFx44 *dest);
int gfxGetNormalMatrix(MtxFx33 *dest);
// NitroSDK's G3i_LookAt_, which loads the camera matrix into the geometry engine when isLoad is set, G3_RotX, G3_RotY,
// G3_RotZ and G3_MultTransMtx33, under swan's names
void gfxLookAt(const VecFx32 *camPos, const VecFx32 *camUp, const VecFx32 *target, BOOL isLoad, MtxFx43 *mtx);
// NitroSDK's G3i_PerspectiveW_, which loads the projection matrix into the geometry engine when isLoad is set
void gfxPerspective(fx32 fovySin, fx32 fovyCos, fx32 aspect, fx32 n, fx32 f, fx32 scaleW, BOOL isLoad, MtxFx44 *mtx);
// NitroSDK's G3i_OrthoW_, which loads the projection matrix into the geometry engine when isLoad is set
void gfxOrtho(fx32 t, fx32 b, fx32 l, fx32 r, fx32 n, fx32 f, fx32 scaleW, BOOL isLoad, MtxFx44 *mtx);
void gfxRotateX(fx32 sin, fx32 cos);
void gfxRotateY(fx32 sin, fx32 cos);
void gfxRotateZ(fx32 sin, fx32 cos);
// NitroSDK's G3_MultMtx44
void gfxMultMatrix4x4(const MtxFx44 *mtx);
void gfxMultTransRot4x3(const MtxFx33 *mtx, const VecFx32 *trans);
void gfxResetMatrixStack(void);
int gfxGetBoxTestResult(s32 *in);

// NitroSDK libgx's gx.c: the power and display registers it sets up, and its entry points
#define reg_G2_BG2PD (*(vu16 *)0x04000026)
#define reg_G2_BG3PD (*(vu16 *)0x04000036)
#define reg_G2S_DB_BG2PD (*(vu16 *)0x04001026)
#define reg_G2S_DB_BG3PD (*(vu16 *)0x04001036)
#define reg_GX_MASTER_BRIGHT (*(vu16 *)REG_MASTER_BRIGHT_ADDR)

// The default DMA channel of GXi_DmaId
#define GX_DEFAULT_DMAID 3

// The engines that GX_SetPower turns on: the 2D engine A, the rendering and geometry engines and the 2D engine B
#define GX_POWER_ALL 0x020e
#define REG_GX_POWCNT_LCD_MASK 0x0001

static inline void GX_SetPower(int power) {
    reg_GX_POWCNT = (u16)((reg_GX_POWCNT & ~GX_POWER_ALL) | power);
}

// The status flags of DISPSTAT, the enables of its interrupts and the line that its V-count interrupt waits for
#define REG_GX_DISPSTAT_STATUS_MASK 0x0007
#define REG_GX_DISPSTAT_VBI_MASK 0x0008
#define REG_GX_DISPSTAT_HBI_MASK 0x0010
#define REG_GX_DISPSTAT_VCI_MASK 0x0020
// What a new V-count line leaves: the flags and the enables
#define REG_GX_DISPSTAT_FLAGS_MASK 0x003f

// The fields of DISPCNT that GX_SetGraphicsMode writes: the BG mode, whether BG 0 is 3D, the display mode and the
// VRAM block that it shows
#define REG_GX_DISPCNT_BGMODE_MASK 0x00000007
#define REG_GX_DISPCNT_BG02D3D_SHIFT 3
#define REG_GX_DISPCNT_BG02D3D_MASK 0x00000008
#define REG_GX_DISPCNT_MODE_SHIFT 16
#define REG_GX_DISPCNT_MODE_MASK 0x00030000
#define REG_GX_DISPCNT_VRAM_MASK 0x000c0000
// All of them, which GX_SetGraphicsMode replaces
#define REG_GX_DISPCNT_GRAPHICS_MASK 0x000f000f
#define REG_GXS_DB_DISPCNT_BGMODE_MASK 0x00000007

// The master brightness registers: a factor of 0 to 16 and a direction
#define REG_GX_MASTER_BRIGHT_FACTOR_MASK 0x001f
#define REG_GX_MASTER_BRIGHT_MODE_MASK 0xc000
#define REG_GX_MASTER_BRIGHT_MODE_UP 0x4000
#define REG_GX_MASTER_BRIGHT_MODE_DOWN 0x8000

// NitroSDK's GX_Init, GX_SetVCountEqVal, GX_IsDispOn and GX_DispOff, under swan's names
void gfxInit(void);
void gfxSetVCountIntrLine(int vcount);
BOOL gfxEngineIsEnabledA(void);
void gfxEngineDisableA(void);
// NitroSDK's G2x_SetMasterBrightness_ and G2x_GetMasterBrightness_, which take the address of the register: -16 to 16,
// 0 for none
void GFXRegSetMasterBrightness(u32 reg, s32 brightness);
s32 gfxRegGetMasterBrightness(u32 reg);
// NitroSDK's GX_InitGXState (gx_state.c)
void gfxInitVRAM(void);
// The lock ID that GX_Init takes for the VRAM banks. NitroSDK's GXi_VRamLockId, swan's g_GfxCartLock
extern u16 g_GfxCartLock;

// NitroSDK libgx's g2.c: the 2D engines' blending and brightness effect, set through the address of BLDCNT, which is
// followed by BLDALPHA and, 4 bytes on, BLDY
#define REG_G2_BLDCNT_EFFECT_MASK 0x00c0
#define REG_G2_BLDCNT_EFFECT_ALPHA 0x0040
#define REG_G2_BLDCNT_EFFECT_INCREASE 0x0080
#define REG_G2_BLDCNT_EFFECT_DECREASE 0x00c0

// NitroSDK's G2x_SetBlendAlpha_: alpha blending of the planes of plane1 over those of plane2. The sub screen's BLDCNT
// is at REG_DB_BLDCNT_ADDR
void gfxRegSetAlphaBlend(u32 reg, u32 plane1, u32 plane2, s32 alpha1, s32 alpha2);
// NitroSDK's G2x_SetBlendBrightness_: brightens the planes by a positive brightness or darkens them by a negative one
void gfxRegSetBrightnessBlend(u32 reg, u32 plane, s32 brightness);
// NitroSDK's G2x_SetBlendBrightnessExt_: the brightness effect, with the alphas that BLDALPHA holds for the planes
// that it is not applied to
void gfxRegSetBlend(u32 reg, u32 plane1, u32 plane2, s32 alpha1, s32 alpha2, s32 brightness);
// NitroSDK's G2x_ChangeBlendBrightness_: changes the brightness effect's level and keeps its planes, turning a
// brightening into a darkening and back
void gfxRegAdjustBrightnessBlend(u32 reg, s32 brightness);

// NitroSDK libgx's g3imm.c and g3_util.c: matrix commands through the FIFO port, and rotations through the matrix
// multiplication port
#define reg_G3_MTX_MULT_3x3 (*(vu32 *)0x04000468)

// NitroSDK's G3_LoadMtx44
void gfxLoadMatrix4x4(const MtxFx44 *mtx);
// NitroSDK's GX_SendFifo64B and GX_SendFifo48B, which copy 64 or 48 bytes to one address (gxasm.c)
void gfxUploadMatrix4x4(const void *src, volatile void *dest);
void gfxUploadMatrix4x3(const void *src, volatile void *dest);

// NitroSDK libgx's g3x.c: the 3D engine's registers that it clears, polls and loads, and its entry points
#define reg_G3X_GXSTAT (*(vu32 *)0x04000600)
#define reg_G3X_DISP_1DOT_DEPTH (*(vu16 *)0x04000610)
#define reg_G3X_CLIPMTX_RESULT (*(vu32 *)0x04000640)
#define reg_G3X_VECMTX_RESULT (*(vu32 *)0x04000680)
#define reg_G3X_EDGE_COLOR (*(vu16 *)0x04000330)
#define reg_G3X_CLEAR_COLOR (*(vu32 *)0x04000350)
#define reg_G3X_CLEAR_DEPTH (*(vu16 *)0x04000354)
#define reg_G3X_CLRIMAGE_OFFSET (*(vu16 *)0x04000356)
#define reg_G3X_FOG_COLOR (*(vu32 *)0x04000358)
#define reg_G3X_FOG_OFFSET (*(vu16 *)0x0400035c)
#define reg_G3X_FOG_TABLE (*(vu32 *)0x04000360)
#define reg_G3X_SHININESS (*(vu32 *)0x040004d0)

// GXSTAT: the box test's state and result, the matrix stacks' levels and errors, the command FIFO's state (bit 25 of
// it, the second of the three, is set while the FIFO is less than half full) and whether the geometry engine is busy,
// with the condition of the FIFO's interrupt
#define REG_G3X_GXSTAT_BT_MASK 0x00000001
#define REG_G3X_GXSTAT_BR_MASK 0x00000002
#define REG_G3X_GXSTAT_PVMTX_SHIFT 8
#define REG_G3X_GXSTAT_PVMTX_MASK 0x00001f00
#define REG_G3X_GXSTAT_PJMTX_SHIFT 13
#define REG_G3X_GXSTAT_PJMTX_MASK 0x00002000
#define REG_G3X_GXSTAT_MB_MASK 0x00004000
#define REG_G3X_GXSTAT_SE_MASK 0x00008000
#define REG_G3X_GXSTAT_FIFO_STAT_SHIFT 24
#define REG_G3X_GXSTAT_FIFO_STAT_MASK 0x07000000
#define REG_G3X_GXSTAT_FIFO_LESS_HALF 0x2
#define REG_G3X_GXSTAT_B_MASK 0x08000000
#define REG_G3X_GXSTAT_FIFO_IRQ_SHIFT 30
#define REG_G3X_GXSTAT_FIFO_IRQ_MASK 0xc0000000

typedef enum {
    GX_FIFOINTR_COND_NEVER,
    GX_FIFOINTR_COND_UNDERHALF,
    GX_FIFOINTR_COND_EMPTY,
} GXFifoIntrCond;

// The fog of DISP3DCNT: its blending mode, its enable and the shift of its depth slope
#define REG_G3X_DISP3DCNT_FMOD_SHIFT 6
#define REG_G3X_DISP3DCNT_FMOD_MASK 0x0040
#define REG_G3X_DISP3DCNT_FE_MASK 0x0080
#define REG_G3X_DISP3DCNT_FSHIFT_SHIFT 8
#define REG_G3X_DISP3DCNT_FSHIFT_MASK 0x0f00

// The clear color: the color, an alpha and a polygon ID, with the fog
#define REG_G3X_CLEAR_COLOR_ALPHA_SHIFT 16
#define REG_G3X_CLEAR_COLOR_ID_SHIFT 24
#define REG_G3X_CLEAR_COLOR_FOG_MASK 0x00008000

static inline void G3X_SetFifoIntrCond(GXFifoIntrCond cond) {
    reg_G3X_GXSTAT = (reg_G3X_GXSTAT & ~REG_G3X_GXSTAT_FIFO_IRQ_MASK) | (cond << REG_G3X_GXSTAT_FIFO_IRQ_SHIFT);
}

// NitroSDK's GXi_NopClearFifo128_, which writes 128 zero words to the FIFO port
void gfxClearFIFO(void *fifo);
// NitroSDK's G3X_ClearFifo
void gfxResetFIFO(void);
// NitroSDK's G3X_ResetMtxStack: G3X_InitMtxStack (gfxResetMatrixStack) without the projection matrix's identity
void gfxResetMatrixStackKeepProjection(void);
// NitroSDK's G3X_SetFog
void gfxSetFog(BOOL enabled, int alphaMode, int depthShift, int offset);
// NitroSDK's G3X_SetEdgeColorTable, G3X_SetFogTable and G3X_SetClearColor
void gfxSetEdgeColorTable(const GXRgb *table);
void gfxSetFogTable(const u32 *table);
void gfxClearColor(GXRgb color, int alpha, int depth, int polygonId, BOOL fog);
// NitroSDK's G3X_InitTable: clears the edge color, fog and toon tables and the shininess table
void gfxResetLUTs(void);
// NitroSDK's G3X_GetMtxStackLevelPV and G3X_GetMtxStackLevelPJ, which return -1 while the stacks are busy
int gfxGetWorldMatrixStackLevel(u32 *level);
int gfxGetProjectionMatrixStackLevel(u32 *level);
// NitroSDK's G3X_SetDisp1DotDepth
void gfxSet1DotDepth(fx32 depth);

// The loaders' copies (gx_load2d.c, gx_load3d.c): by DMA on GX's channel, by NDMA when it is one of the DSi's (numbers
// above 3), or with the CPU when it has none or the copy is small (up to 28 bytes by halfwords, 48 by words). NDMA
// moves words, so a halfword copy goes by it only when the source, the destination and the size are all aligned
static inline void GXi_DmaCopy16(u32 dmaNo, const void *src, void *dest, u32 size) {
    if (dmaNo != GX_DMA_NOT_USE && size > 28) {
        if (dmaNo > MI_DMA_MAX_NUM) {
            if ((((u32)src | (u32)dest | size) & 3) == 0) {
                func_02768180(dmaNo - 4, src, dest, size);
            } else {
                MI_CpuCopy16(src, dest, size);
            }
        } else {
            func_020780e8(dmaNo, src, dest, size, TRUE);
        }
    } else {
        MI_CpuCopy16(src, dest, size);
    }
}

static inline void GXi_DmaCopy32(u32 dmaNo, const void *src, void *dest, u32 size) {
    if (dmaNo != GX_DMA_NOT_USE && size > 48) {
        if (dmaNo > MI_DMA_MAX_NUM) {
            func_02768180(dmaNo - 4, src, dest, size);
        } else {
            dma_copy(dmaNo, src, dest, size, TRUE);
        }
    } else {
        sys_memcpy32(src, dest, size);
    }
}

// The same, whatever the size, and calling callback with arg when the copy is done
static inline void GXi_DmaCopy32Async(u32 dmaNo, const void *src, void *dest, u32 size, void (*callback)(void *arg),
                                      void *arg) {
    if (dmaNo != GX_DMA_NOT_USE) {
        if (dmaNo > MI_DMA_MAX_NUM) {
            func_027681f4(dmaNo - 4, src, dest, size, callback, arg);
        } else {
            dma_copyEx(dmaNo, src, dest, size, callback, arg, TRUE);
        }
    } else {
        sys_memcpy32(src, dest, size);
    }
}

// Waits for the channel of a copy to be done
static inline void GXi_WaitDma(u32 dmaNo) {
    if (dmaNo != GX_DMA_NOT_USE) {
        if (dmaNo > MI_DMA_MAX_NUM) {
            func_0276824c(dmaNo - 4);
        } else {
            func_0207829c(dmaNo);
        }
    }
}

#endif // POKEBW2_NITRO_GX_H
