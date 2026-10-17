#ifndef PORT_HOST_H
#define PORT_HOST_H

// Force-included into every decomp C file of the host build (port/CMakeLists.txt): what MWCC provides built in and
// clang spells differently. Nothing here may change what a file means on the DS.

// MWCC's memcpy built-in, which the DPW, DWC and POKE_NET libraries call by this name
#define __memcpy __builtin_memcpy

#endif // PORT_HOST_H
