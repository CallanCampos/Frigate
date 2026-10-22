#ifndef POKEBW2_SYSTEM_AEABI_H
#define POKEBW2_SYSTEM_AEABI_H

#include "types.h"

// MSL's runtime division (swan's __aeabi_uidivmod). Quotient is returned in r0 and remainder in r1. The u64
// declaration exposes both registers to C.
u64 _u32_div_f(u32 dividend, u32 divisor);

#endif // POKEBW2_SYSTEM_AEABI_H
