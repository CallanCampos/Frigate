#ifndef POKEBW2_NITRO_CP_H
#define POKEBW2_NITRO_CP_H

#include "types.h"

// Snapshot of the divider and square-root hardware registers.
typedef struct {
    u32 divisionNumerator[2];
    u32 divisionDenominator[2];
    u32 squareRootValue[2];
    u16 divisionMode;
    u16 squareRootMode;
} CPContext;

// The divider and square root unit's registers
#define reg_CP_DIVCNT (*(vu16 *)0x04000280)
#define reg_CP_DIV_NUMER (*(volatile u64 *)0x04000290)
#define reg_CP_DIV_NUMER_LO (*(vu32 *)0x04000290)
#define reg_CP_DIV_DENOM (*(volatile u64 *)0x04000298)
// Not volatile: FX_InvFx64c and FX_GetDivResultFx64c read the 64-bit result as plain memory, after the busy wait
#define reg_CP_DIV_RESULT (*(u64 *)0x040002a0)
#define reg_CP_DIV_RESULT_LO (*(vu32 *)0x040002a0)
#define reg_CP_DIVREM_RESULT (*(vu32 *)0x040002a8)
#define reg_CP_SQRTCNT (*(vu16 *)0x040002b0)
#define reg_CP_SQRT_RESULT (*(vu32 *)0x040002b4)
#define reg_CP_SQRT_PARAM (*(volatile u64 *)0x040002b8)
#define REG_CP_BUSY_MASK 0x8000

// The divider's registers by address, which NitroSDK's divider inlines write and read through plain pointers
#define REG_DIV_NUMER_ADDR 0x04000290
#define REG_DIV_DENOM_ADDR 0x04000298
#define REG_DIV_RESULT_ADDR 0x040002a0

// The divider's mode for a 64-bit numerator and a 32-bit denominator
#define CP_DIV_64_32BIT_MODE 1

static inline void CP_SetDivImm64_32(u64 numer, u32 denom) {
    *(u64 *)REG_DIV_NUMER_ADDR = numer;
    *(u64 *)REG_DIV_DENOM_ADDR = denom;
}

static inline void CP_SetDiv64_32(u64 numer, u32 denom) {
    reg_CP_DIVCNT = CP_DIV_64_32BIT_MODE;
    CP_SetDivImm64_32(numer, denom);
}

static inline s32 CP_IsDivBusy(void) {
    return reg_CP_DIVCNT & REG_CP_BUSY_MASK;
}

static inline void CP_WaitDiv(void) {
    while (CP_IsDivBusy()) {
    }
}

static inline s32 CP_GetDivResultImm32(void) {
    return (s32)(*(u32 *)REG_DIV_RESULT_ADDR);
}

static inline s32 CP_GetDivResult32(void) {
    CP_WaitDiv();
    return CP_GetDivResultImm32();
}

void CP_SaveContext(CPContext *context);
void CPi_RestoreContext(const CPContext *context);

#endif // POKEBW2_NITRO_CP_H
