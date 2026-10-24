// MSL's runtime division, `_u32_div_f`, which the game's C calls by name (include/system/aeabi.h), on a host. It is
// not a game function: it is the C runtime that MWCC links into the ROM (lib/msl/src/fp/fp_fastI.s, swan's
// __aeabi_uidivmod), the routine MWCC calls for every unsigned 32-bit division. The host build leaves MSL out for the
// host's C library (port/hal/replaced.txt), whose compiler divides inline, so a game file that calls the routine
// itself would stop on its trap stub. Two do:
// - the field's lens flare data counts its entries with it at every map change (FieldLensFlareData_BytesToEntryCount,
//   src/ov036/field_goout_effect_data.c), so the first change of map after New Game's opening stopped there;
// - the Unity Tower's visitors take a message from the remainder of a division by 5 (src/ov033/unity_visitor.c).
// The ARM routine returns the quotient in r0 and the remainder in r1; the decomp's declaration returns them as one u64,
// the quotient in the low half and the remainder in the high half, which is what this returns. A divisor of 0 gives
// what the DS's gives, which C leaves undefined: the routine returns at once, with the dividend in r0 and the divisor,
// 0, in r1, so the quotient is the dividend and the remainder 0 (port/docs/lessons.md, "A division by zero is the
// runtime's on the DS"). port/tests/hal/test_msl.c checks it against values worked out by hand.
#include "types.h"
#include "system/aeabi.h"

u64 _u32_div_f(u32 dividend, u32 divisor) {
    if (divisor == 0) {
        return dividend;
    }
    return (u64)(dividend / divisor) | (u64)(dividend % divisor) << 32;
}
