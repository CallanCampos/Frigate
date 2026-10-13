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

void CP_SaveContext(CPContext *context);
void CPi_RestoreContext(const CPContext *context);

#endif // POKEBW2_NITRO_CP_H
