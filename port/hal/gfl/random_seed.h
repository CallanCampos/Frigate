// The seeds of the game's random numbers on a host: GFL builds one from the console's entropy at start-up, for its own
// random numbers (GFL_RandomInit in src/system/gfl_use.c, which every random event of the field draws from: a
// townsperson's walk, a wild Pokémon), and one at each battle's start, for the battle's (src/ov167/btl_main.c: damage
// rolls, critical hits, a Poké Ball's shakes), both with buildSeed (src/gfl/gf_standard.c). The DS hashes its CPU's
// timing (the tick and the line being drawn when the call comes) and 20 bytes of whatever its stack held, which a host
// can't have, so the port's seeds differ from any DS's (port/docs/decomp-findings.md). An input file can give the
// seeds a DS built ("seed" lines, port/tools/oracle/pbwinput.h): the game then takes them, in order, in place of the
// ones it builds, and draws the DS's random numbers. buildSeed comes here through gf_standard_host.c. Plain C types
// only.
#ifndef PORT_HAL_GFL_RANDOM_SEED_H
#define PORT_HAL_GFL_RANDOM_SEED_H

#include <stddef.h>
#include <stdint.h>

// The most seeds a run can be given, as many as an input file can hold (PBW_INPUT_MAX_SEEDS)
#define PORT_RANDOM_SEED_MAX_GIVEN 64
// The most seeds of a run that are kept to be reported (port_random_seed_at): the game builds one at start-up and one
// at each battle's start
#define PORT_RANDOM_SEED_MAX_KEPT 256

// Gives the seeds the game takes in place of the first count it builds, in order; none by default. Before the game
// starts. Returns 0 when there are more than PORT_RANDOM_SEED_MAX_GIVEN
int port_random_seed_give(const uint64_t *seeds, size_t count);

// The seed the game uses now, where buildSeed made `built`: the next given seed while any is left, else `built`
uint64_t port_random_seed_take(uint64_t built);

// Recording-only fail-closed policy: call before an unsupplied seed could reach the game. Handler must not return;
// returning handlers still terminate with status 1. NULL restores ordinary fallback behavior.
void port_random_seed_on_exhaustion(void (*handler)(void));
// Check before buildSeed touches the game context; take checks again for other callers.
void port_random_seed_check(void);

// How many seeds the game has built so far
size_t port_random_seed_count(void);

// The index-th seed the game used (from 0) and whether it was given; returns 0 past the last one kept
int port_random_seed_at(size_t index, uint64_t *seed, int *given);

#endif // PORT_HAL_GFL_RANDOM_SEED_H
