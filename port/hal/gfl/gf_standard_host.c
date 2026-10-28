// Game Freak's standard library on a host: the decomp's src/gfl/gf_standard.c (the Mersenne Twister, the CRC table,
// the random seed, the checksums and the mystery gift's and battle video's encryption), compiled unchanged through this
// file (port/hal/gfl/replaced.txt keeps it out of the game's own sources), with buildSeed wrapped. buildSeed builds the
// seed of GFL's random numbers at start-up (GFL_RandomInit, src/system/gfl_use.c) and of each battle's
// (src/ov167/btl_main.c) from the console's entropy, which on the DS is its CPU's timing and its stack's leftovers, so
// a host builds another seed than any DS (port/docs/decomp-findings.md). The wrapper runs gf_standard.c's buildSeed,
// then gives the context the seed random_seed.c says the game uses: the one an input file gives in its place, as the
// DS built it, else the one built. gf_standard.c's other functions are its own. --run-dump reads the Mersenne Twister's
// state through sGFLStd's handle (port/hal/gfl/random_state.h).
#define buildSeed buildSeed_ds
#include "src/gfl/gf_standard.c"
#undef buildSeed

#include "hal/gfl/random_seed.h"
#include "hal/gfl/random_state.h"

void *const port_gfl_std_tables = &sGFLStd;

// The headers' prototype was renamed with the function
void buildSeed(MATHRandContext32 *context);

// gf_standard.c's buildSeed, then the seed it built or the one given in its place, set up as MATH_InitRand32 sets up a
// seed (the multiplier and the increment are the same for every seed)
void buildSeed(MATHRandContext32 *context) {
    port_random_seed_check();
    buildSeed_ds(context);
    MATH_InitRand32(context, port_random_seed_take(context->x));
}
