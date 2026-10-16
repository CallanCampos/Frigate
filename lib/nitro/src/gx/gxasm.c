// NitroSDK libgx's gxasm.c: copies of a matrix to a graphics FIFO port (GX_SendFifo48B for a 4x3 matrix and
// GX_SendFifo64B for a 4x4 one), which the SDK wrote in ARM assembly. The file has no strings; its name is the SDK's,
// from the layout of libgx.
#include "nitro/gx.h"

// clang-format off
// The SDK wrote these in assembly: each stmia without write-back starts again at the FIFO port, whose 64 bytes all reach
// the FIFO. dest is the port, which the geometry engine reads
asm void GX_SendFifo48B(const void *src, volatile void *dest) {
    ldmia r0!, {r2, r3, r12}
    stmia r1, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1, {r2, r3, r12}
    bx lr
}

// The same for a 4x4 matrix, in assembly in the SDK; dest is the port, which the geometry engine reads
asm void GX_SendFifo64B(const void *src, volatile void *dest) {
    stmdb sp!, {r4, r5, r6, r7, r8}
    ldmia r0!, {r2, r3, r4, r5, r6, r7, r8, r12}
    stmia r1, {r2, r3, r4, r5, r6, r7, r8, r12}
    ldmia r0!, {r2, r3, r4, r5, r6, r7, r8, r12}
    stmia r1, {r2, r3, r4, r5, r6, r7, r8, r12}
    ldmia sp!, {r4, r5, r6, r7, r8}
    bx lr
}
// clang-format on
