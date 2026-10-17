// Functions the decomp declares under a placeholder name because its target isn't identified yet. The ROM build links
// them to an address; the host gives each the effect the call has on the DS, recorded in port/docs/decomp-findings.md.

// include/save/resh_zek.h: the fused-Pokémon save block's initializer (func_0200af78, during SaveControl_Create) calls
// through a relocation to 0x0215cd8c. While overlay 11 is loaded that address is WCM_RssiToQuality, which reads its
// argument as a signal strength, writes nothing and returns a value the caller ignores; so on the DS the call has no
// effect, and the host's does nothing
void ReshZekSave_UnresolvedInitTarget(void *block) {
    (void)block;
}
