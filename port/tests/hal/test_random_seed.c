// Unit tests of port/hal/gfl/random_seed.c, the seeds of the game's random numbers on a host (GFL's at start-up and
// each battle's, which src/gfl/gf_standard.c's buildSeed builds and port/hal/gfl/gf_standard_host.c passes through):
// the seeds an input file gives are taken in their order in place of the first ones the game builds, the game's own
// after the last, and every one is recorded with whether it was given, as the run directory's run.txt reports them.
// More seeds than an input file can hold are refused. It prints each failure and returns nonzero if there was one.
#include "hal/gfl/random_seed.h"
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

static int sFailed;
static jmp_buf sEscape;
static void Exhausted(void) {
    longjmp(sEscape, 1);
}
static void ReturningHandler(void) {
}

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                              \
            sFailed = 1;                                                                                               \
        }                                                                                                              \
    } while (0)

int main(int argc, char **argv) {
    if (argc > 1) {
        uint64_t supplied[64] = { 0 };
        size_t count = strcmp(argv[1], "strict64") == 0 ? 64 : 2;
        CHECK(port_random_seed_give(supplied, count));
        port_random_seed_on_exhaustion(ReturningHandler);
        for (size_t i = 0; i < count; i++) {
            CHECK(port_random_seed_take(0xdeadbeef) == 0);
        }
        port_random_seed_take(0xdeadbeef);
        return 99; // A returning handler must still never deliver fallback to its caller.
    }
    // The first is the seed Black 2 builds at start-up on melonDS, from power-on with the default clock
    static const uint64_t given[] = { 0xea56a175a2fffc8aull, 0x0000000100000001ull };
    uint64_t tooMany[PORT_RANDOM_SEED_MAX_GIVEN + 1] = { 0 };
    uint64_t seed;
    int fromFile;

    // Nothing given: the game's own seeds, recorded
    CHECK(port_random_seed_count() == 0);
    CHECK(!port_random_seed_at(0, &seed, &fromFile));
    CHECK(!port_random_seed_give(tooMany, PORT_RANDOM_SEED_MAX_GIVEN + 1));
    CHECK(port_random_seed_give(tooMany, PORT_RANDOM_SEED_MAX_GIVEN));
    CHECK(port_random_seed_give(given, 2));

    // The given seeds in order, whatever the game built, then the game's own
    CHECK(port_random_seed_take(0x1111111111111111ull) == given[0]);
    CHECK(port_random_seed_take(0) == given[1]);
    CHECK(port_random_seed_take(0x2222222222222222ull) == 0x2222222222222222ull);
    CHECK(port_random_seed_take(0) == 0);
    CHECK(port_random_seed_count() == 4);
    CHECK(port_random_seed_at(0, &seed, &fromFile) && seed == given[0] && fromFile);
    CHECK(port_random_seed_at(1, &seed, &fromFile) && seed == given[1] && fromFile);
    CHECK(port_random_seed_at(2, &seed, &fromFile) && seed == 0x2222222222222222ull && !fromFile);
    CHECK(port_random_seed_at(3, &seed, &fromFile) && seed == 0 && !fromFile);
    CHECK(!port_random_seed_at(4, &seed, &fromFile));

    // Past the seeds kept, the count goes on and the record stops
    for (int i = 4; i < PORT_RANDOM_SEED_MAX_KEPT + 3; i++) {
        port_random_seed_take((uint64_t)i);
    }
    CHECK(port_random_seed_count() == PORT_RANDOM_SEED_MAX_KEPT + 3);
    CHECK(port_random_seed_at(PORT_RANDOM_SEED_MAX_KEPT - 1, &seed, &fromFile) &&
          seed == PORT_RANDOM_SEED_MAX_KEPT - 1 && !fromFile);
    CHECK(!port_random_seed_at(PORT_RANDOM_SEED_MAX_KEPT, &seed, &fromFile));

    port_random_seed_on_exhaustion(Exhausted);
    size_t before = port_random_seed_count();
    if (setjmp(sEscape) == 0) {
        port_random_seed_take(0xdeadbeef);
        CHECK(0); // Exhaustion must leave before a fallback value is returned or recorded.
    }
    CHECK(port_random_seed_count() == before);
    port_random_seed_on_exhaustion(NULL);
    CHECK(port_random_seed_take(42) == 42);
    if (sFailed) {
        return 1;
    }
    printf("random_seed: all checks passed\n");
    return 0;
}
