// NitroSDK libgx's g3x.c: the 3D engine's state as a whole: its start-up and reset, the geometry FIFO, the matrix
// stacks and the results of the geometry engine's tests, the fog and the edge and fog tables, and the clear color. The
// name is NitroSDK's. The linker's veneer pair for the NDMA fill with a callback follows G3X_InitTable, its caller; it
// is not part of the source.
#include "nitro/gx.h"

void G3X_Init(void) {
    G3X_ClearFifo();
    G3_End();
    while (reg_G3X_GXSTAT & REG_G3X_GXSTAT_B_MASK) {
    }
    reg_G3X_DISP3DCNT = 0;
    reg_G3X_GXSTAT = 0;
    reg_G2_BG0OFS = 0;
    reg_G3X_DISP3DCNT |= REG_G3X_DISP3DCNT_GO_MASK;
    reg_G3X_DISP3DCNT |= REG_G3X_DISP3DCNT_RO_MASK;
    G3X_SetShading(GX_SHADING_TOON);
    G3X_AntiAlias(TRUE);
    G3X_AlphaTest(FALSE, 0);
    reg_G3X_GXSTAT |= REG_G3X_GXSTAT_SE_MASK;
    G3X_SetFifoIntrCond(GX_FIFOINTR_COND_EMPTY);
    G3X_InitMtxStack();
    reg_G3X_CLEAR_COLOR = 0;
    reg_G3X_CLEAR_DEPTH = 0x7fff;
    reg_G3X_CLRIMAGE_OFFSET = 0;
    reg_G3X_FOG_COLOR = 0;
    reg_G3X_FOG_OFFSET = 0;
    G2_SetBG0Priority(0);
    G3X_InitTable();
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_BACK, 0, 31, 0);
    G3_TexImageParam(GX_TEXFMT_NONE, GX_TEXGEN_NONE, GX_TEXSIZE_S8, GX_TEXSIZE_T8, GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE,
                     GX_TEXPLTTCOLOR0_USE, 0);
    G3_TexPlttBase(0, GX_TEXFMT_NONE);
}

void G3X_Reset(void) {
    while (reg_G3X_GXSTAT & REG_G3X_GXSTAT_B_MASK) {
    }
    reg_G3X_GXSTAT |= REG_G3X_GXSTAT_SE_MASK;
    reg_G3X_DISP3DCNT |= REG_G3X_DISP3DCNT_GO_MASK;
    reg_G3X_DISP3DCNT |= REG_G3X_DISP3DCNT_RO_MASK;
    G3X_ResetMtxStack();
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_BACK, 0, 31, 0);
    G3_TexImageParam(GX_TEXFMT_NONE, GX_TEXGEN_NONE, GX_TEXSIZE_S8, GX_TEXSIZE_T8, GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE,
                     GX_TEXPLTTCOLOR0_USE, 0);
    G3_TexPlttBase(0, GX_TEXFMT_NONE);
}

void G3X_ClearFifo(void) {
    GXi_NopClearFifo128_(&reg_G3X_GXFIFO);
    while (reg_G3X_GXSTAT & REG_G3X_GXSTAT_B_MASK) {
    }
}

void G3X_InitMtxStack(void) {
    s32 posLevel;
    s32 projLevel;
    reg_G3X_GXSTAT |= REG_G3X_GXSTAT_SE_MASK;
    while (G3X_GetMtxStackLevelPV(&posLevel) != 0) {
    }
    while (G3X_GetMtxStackLevelPJ(&projLevel) != 0) {
    }
    G3_MtxMode(GX_MTXMODE_TEXTURE);
    G3_Identity();
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    if (projLevel != 0) {
        G3_PopMtx(projLevel);
    }
    G3_Identity();
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_PopMtx(posLevel);
    G3_Identity();
}

void G3X_ResetMtxStack(void) {
    s32 posLevel;
    s32 projLevel;
    reg_G3X_GXSTAT |= REG_G3X_GXSTAT_SE_MASK;
    while (G3X_GetMtxStackLevelPV(&posLevel) != 0) {
    }
    while (G3X_GetMtxStackLevelPJ(&projLevel) != 0) {
    }
    G3_MtxMode(GX_MTXMODE_TEXTURE);
    G3_Identity();
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    if (projLevel != 0) {
        G3_PopMtx(projLevel);
    }
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_PopMtx(posLevel);
    G3_Identity();
}

void G3X_SetFog(BOOL enabled, GXFogBlend alphaMode, GXFogSlope depthShift, int offset) {
    if (enabled) {
        reg_G3X_FOG_OFFSET = (u16)offset;
        reg_G3X_DISP3DCNT = (u16)((depthShift << REG_G3X_DISP3DCNT_FSHIFT_SHIFT) |
                                  (alphaMode << REG_G3X_DISP3DCNT_FMOD_SHIFT) | REG_G3X_DISP3DCNT_FE_MASK |
                                  (reg_G3X_DISP3DCNT & ~(REG_G3X_DISP3DCNT_FMOD_MASK | REG_G3X_DISP3DCNT_FSHIFT_MASK |
                                                         REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK)));
    } else {
        reg_G3X_DISP3DCNT &= ~(REG_G3X_DISP3DCNT_FE_MASK | REG_G3X_DISP3DCNT_RO_MASK | REG_G3X_DISP3DCNT_GO_MASK);
    }
}

int G3X_GetClipMtx(MtxFx44 *dest) {
    if (reg_G3X_GXSTAT & REG_G3X_GXSTAT_B_MASK) {
        return -1;
    }
    MI_Copy64B((const void *)&reg_G3X_CLIPMTX_RESULT, dest);
    return 0;
}

int G3X_GetVectorMtx(MtxFx33 *dest) {
    if (reg_G3X_GXSTAT & REG_G3X_GXSTAT_B_MASK) {
        return -1;
    }
    MI_Copy36B((const void *)&reg_G3X_VECMTX_RESULT, dest);
    return 0;
}

void G3X_SetEdgeColorTable(const GXRgb *table) {
    MI_CpuCopy16(table, (void *)&reg_G3X_EDGE_COLOR, 16);
}

void G3X_SetFogTable(const u32 *table) {
    MI_Copy32B(table, (void *)&reg_G3X_FOG_TABLE);
}

void G3X_SetClearColor(GXRgb color, int alpha, int depth, int polygonId, BOOL fog) {
    u32 value = color | (alpha << REG_G3X_CLEAR_COLOR_ALPHA_SHIFT) | (polygonId << REG_G3X_CLEAR_COLOR_ID_SHIFT);
    if (fog) {
        value |= REG_G3X_CLEAR_COLOR_FOG_MASK;
    }
    reg_G3X_CLEAR_COLOR = value;
    reg_G3X_CLEAR_DEPTH = (u16)depth;
}

void G3X_InitTable(void) {
    int i;
    if (GXi_DmaId != GX_DMA_NOT_USE) {
        if (GXi_DmaId > MI_DMA_MAX_NUM) {
            MI_NDmaFillAsync(GXi_DmaId - 4, (void *)&reg_G3X_EDGE_COLOR, 0, 0x10, NULL, NULL);
            MI_NDmaFill(GXi_DmaId - 4, (void *)&reg_G3X_FOG_TABLE, 0, 0x60);
        } else {
            MIi_DmaFill32Async(GXi_DmaId, (void *)&reg_G3X_EDGE_COLOR, 0, 0x10, NULL, NULL, TRUE);
            MIi_DmaFill32(GXi_DmaId, (void *)&reg_G3X_FOG_TABLE, 0, 0x60, TRUE);
        }
    } else {
        MI_CpuClear32((void *)&reg_G3X_EDGE_COLOR, 0x10);
        MI_CpuClear32((void *)&reg_G3X_FOG_TABLE, 0x60);
    }
    for (i = 0; i < 32; i++) {
        reg_G3X_SHININESS = 0;
    }
}

int G3X_GetMtxStackLevelPV(s32 *level) {
    if (reg_G3X_GXSTAT & REG_G3X_GXSTAT_MB_MASK) {
        return -1;
    }
    *level = (reg_G3X_GXSTAT & REG_G3X_GXSTAT_PVMTX_MASK) >> REG_G3X_GXSTAT_PVMTX_SHIFT;
    return 0;
}

int G3X_GetMtxStackLevelPJ(s32 *level) {
    if (reg_G3X_GXSTAT & REG_G3X_GXSTAT_MB_MASK) {
        return -1;
    }
    *level = (reg_G3X_GXSTAT & REG_G3X_GXSTAT_PJMTX_MASK) >> REG_G3X_GXSTAT_PJMTX_SHIFT;
    return 0;
}

void G3X_SetDisp1DotDepth(fx32 depth) {
    reg_G3X_DISP_1DOT_DEPTH = (u16)((depth >> 9) & 0x7fff);
}

int G3X_GetBoxTestResult(s32 *result) {
    if (reg_G3X_GXSTAT & REG_G3X_GXSTAT_BT_MASK) {
        return -1;
    }
    *result = reg_G3X_GXSTAT & REG_G3X_GXSTAT_BR_MASK;
    return 0;
}

// clang-format off
#pragma thumb off
// The SDK wrote this in assembly: 128 words of 0 stored to the FIFO port, which push out whatever it still holds. The
// 32 stores of 4 words are not a loop
asm void GXi_NopClearFifo128_(void *fifo) {
    mov r1, #0
    mov r2, #0
    mov r3, #0
    mov r12, #0
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    stmia r0, {r1, r2, r3, r12}
    bx lr
}
#pragma thumb on
// clang-format on
