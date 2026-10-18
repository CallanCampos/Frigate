/* Force-included (-include) by the compile probe's "arm64 + shim" pass (run_all.sh's shim2_arm64): the MWCC built-ins
   and keywords that clang lacks, so that the pass counts the 64-bit hazards in the game's and the libraries' C and not
   MWCC's dialect (port/docs/research/compile-probe.md). A research copy of 2026-10-16: Frigate's own build gives the
   decomp's C the same through port/include/port_host.h, which it force-includes into every decomp file. */
#define __memcpy __builtin_memcpy
