// MSL C's rand.c: rand and srand of <stdlib.h>, a linear congruential generator that keeps the upper 15 bits of its
// 32-bit state.
#include <stdlib.h>

// The generator's state, which srand sets (swan's name)
static unsigned long random_next = 1;

int rand(void) {
    random_next = random_next * 0x41C64E6D + 12345;
    return (random_next >> 16) & RAND_MAX;
}

void srand(unsigned int seed) {
    random_next = seed;
}
