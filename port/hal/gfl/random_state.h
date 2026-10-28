// GFL's random state, as --run-dump reads it (port/app/run_dir.c) for the oracle's sequences, which compare it with the
// DS's at each frame (port/tools/oracle/oracle.py, RANDOM_STATE; port/tools/oracle/README.md, "Random seeds"). It is in
// two of the game's statics, each a pointer to memory GFL allocates, which no global names: GFL's work
// (src/system/gfl_use.c's sWork), whose first member is the context of GFL's linear congruential generator (a
// MATHRandContext32), and GFL's standard tables (src/gfl/gf_standard.c's sGFLStd), the Mersenne Twister's 624 words and
// then its index. Each handle below is the address of its static, so --run-dump's ** reads the static's pointer and
// then the memory it points at, the same bytes pbw-oracle reads on the DS with * and the static's name. The handles
// are defined in the host files the two decomp files are compiled through. Plain C types only.
#ifndef PORT_HAL_GFL_RANDOM_STATE_H
#define PORT_HAL_GFL_RANDOM_STATE_H

// &sWork, a GFLUseWork ** (port/hal/gfl/gfl_use_host.c)
extern void *const port_gfl_use_work;
// &sGFLStd, a GFLStdTables ** (port/hal/gfl/gf_standard_host.c)
extern void *const port_gfl_std_tables;

#endif // PORT_HAL_GFL_RANDOM_STATE_H
