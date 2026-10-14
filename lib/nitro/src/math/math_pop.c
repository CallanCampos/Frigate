// NitroSDK libmath's math_pop.c: leading zero count and population count.
// The split from math_md5.c is certain (the first two functions of libmath, in front of the MD5 code).
#include "nitro/math.h"

// The SDK wrote this in assembly: a CLZ and a return, in ARM, for Thumb code to call.
#pragma thumb off
asm u32 MATH_CountLeadingZerosFunc(u32 x) {
    clz r0, r0
    bx lr
}
#pragma thumb on

u8 MATH_CountPopulation(u32 x) {
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0f0f0f0f;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return (u8)x;
}
