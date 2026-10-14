#include "types.h"
#include "nitro/fx.h"
#include "spl_internal.h"

// SPL's random numbers. This compiler emits a file's functions last to first

u32 gSPLRandomState;

void SPLRandom_VecFx32(VecFx32 *vec) {
    vec->x = SPLRandom_Fx32(24);
    vec->y = SPLRandom_Fx32(24);
    vec->z = SPLRandom_Fx32(24);
    VEC_Normalize(vec, vec);
}

void SPLRandom_VecFx32_XY(VecFx32 *vec) {
    vec->x = SPLRandom_Fx32(24);
    vec->y = SPLRandom_Fx32(24);
    vec->z = 0;
    VEC_Normalize(vec, vec);
}
