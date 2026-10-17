// Functions the decomp declares under a placeholder name because its target isn't identified yet. The ROM build links
// them to an address; the host build traps if the game ever calls one, like the stubs of port/tools/gen_stubs.py.
#include "hal/trap.h"

// include/save/resh_zek.h: the save initializer of the fused-Pokémon block calls through a relocation into overlay 11
// whose target the decomp hasn't identified
void ReshZekSave_UnresolvedInitTarget(void *block) {
    static const PortTrapSymbol symbol = { "ReshZekSave_UnresolvedInitTarget", 0, 0, "ov011" };
    (void)block;
    port_trap_function(&symbol, __builtin_return_address(0));
}
