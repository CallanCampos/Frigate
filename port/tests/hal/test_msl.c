// Unit tests of port/hal/msl, MSL's runtime routines that the game's C calls by name: `_u32_div_f`, the unsigned
// division that the field's lens flare data (src/ov036/field_goout_effect_data.c) and the Unity Tower's visitors
// (src/ov033/unity_visitor.c) call. Each expected result is worked out by hand from the ARM routine
// (lib/msl/src/fp/fp_fastI.s): the quotient in the low half, the remainder in the high half, and for a divisor of 0
// the dividend and a remainder of 0, as the routine returns at once. It prints each failure and returns nonzero if
// there was one.
#include "types.h"
#include "system/aeabi.h"
#include <stdio.h>

static int sFailures;

static void Check(u32 dividend, u32 divisor, u32 quotient, u32 remainder, const char *why) {
    u64 result = _u32_div_f(dividend, divisor);
    u64 expected = (u64)quotient | (u64)remainder << 32;

    if (result != expected) {
        printf("FAIL _u32_div_f(0x%08x, 0x%08x), %s: 0x%016llx, expected 0x%016llx\n", (unsigned)dividend,
               (unsigned)divisor, why, (unsigned long long)result, (unsigned long long)expected);
        sFailures++;
    }
}

int main(void) {
    Check(7, 2, 3, 1, "a remainder");
    Check(100, 7, 14, 2, "7 * 14 = 98");
    Check(0x30, 0xc, 4, 0, "no remainder: four 12-byte entries");
    Check(13, 5, 2, 3, "the remainder that unity_visitor.c takes");
    Check(5, 10, 0, 5, "a dividend below the divisor, which the routine returns as the remainder at once");
    Check(0, 5, 0, 0, "a dividend of 0");
    Check(0x80000000, 3, 0x2aaaaaaa, 2, "3 * 0x2aaaaaaa = 0x7ffffffe, unsigned: the top bit is no sign");
    Check(0xffffffff, 1, 0xffffffff, 0, "the largest dividend by 1");
    Check(0xffffffff, 0xffffffff, 1, 0, "the largest dividend by itself");
    Check(0xffffffff, 0x10000, 0xffff, 0xffff, "a divisor of 16 bits");
    Check(0xfffffffe, 0xffffffff, 0, 0xfffffffe, "the largest divisor, just above the dividend");
    Check(1234, 0, 1234, 0, "a divisor of 0: the dividend, as the DS's routine returns it");
    Check(0, 0, 0, 0, "0 by 0");
    Check(0xffffffff, 0, 0xffffffff, 0, "the largest dividend by 0");
    if (sFailures == 0) {
        printf("_u32_div_f: every value as the DS's routine gives it\n");
    }
    return sFailures != 0;
}
