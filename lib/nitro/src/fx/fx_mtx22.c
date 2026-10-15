// NitroSDK's libfx fx_mtx22.c: the 2x2 fixed point matrix functions, at the start of libfx in ARM9 main. The file split
// is by function group (libfx's source files are one per matrix size) and fairly sure; the names are the SDK's.
// MTX_Identity22_ and MAT2_Scaling store through stm with writeback, which MWCC does not generate (MAT2_Scaling even
// stores a single register that way), and MTX_Rot22_ stores in an order MWCC does not schedule for the same C, so
// these three were hand-written assembly and are asm functions here.
#include "nitro/fx.h"

#pragma thumb off
asm void MTX_Identity22_(MtxFx22 *mtx) {
    mov r1, #0
    mov r2, #0x1000
    mov r3, #0
    stmia r0!, {r2, r3}
    stmia r0!, {r1, r2}
    bx lr
}
#pragma thumb on

asm void MAT2_Scaling(MtxFx22 *mtx, fx32 x, fx32 y) {
    stmia r0!, {r1}
    mov r1, #0
    str r2, [r0, #8]
    mov r2, #0
    stmia r0!, {r1, r2}
    bx lr
}

asm void MTX_Rot22_(MtxFx22 *mtx, fx32 sin, fx32 cos) {
    str r2, [r0, #0]
    str r1, [r0, #4]
    neg r1, r1
    str r1, [r0, #8]
    str r2, [r0, #0xc]
    bx lr
}

void MTX_ScaleApply22(const MtxFx22 *src, MtxFx22 *dest, fx32 x, fx32 y) {
    dest->_00 = (fx32)(((fx64)x * src->_00) >> FX32_SHIFT);
    dest->_01 = (fx32)(((fx64)x * src->_01) >> FX32_SHIFT);
    dest->_10 = (fx32)(((fx64)y * src->_10) >> FX32_SHIFT);
    dest->_11 = (fx32)(((fx64)y * src->_11) >> FX32_SHIFT);
}
