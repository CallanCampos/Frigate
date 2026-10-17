// 32-bit handles for host pointers that sound data keeps in 32-bit fields (port/hal/snd7/snd_bank_host.c). Plain C
// types only, so that a game change can declare them where the decomp's sound code needs them.
//
// The DS's sound files hold ARM9 addresses in 32-bit fields once they are loaded: a bank's links to its wave archives,
// a wave archive's addresses of waves loaded by themselves (the host's snd_bank.c keeps these), and NitroSystem's sound
// archive's file table (NNSSndArcFileInfo's mem, NNS_SndArcSetFileAddress). A host pointer doesn't fit, so such a field
// holds a handle: 0x80000000 and an index in a table of pointers, never 0, never below 0x02000000 (where the files keep
// offsets), the same for the same pointer for the whole run. 0 is NULL both ways; a value that isn't a handle gives
// NULL.
#ifndef PORT_HAL_SND7_SND_HANDLE_H
#define PORT_HAL_SND7_SND_HANDLE_H

#include <stdint.h>

uint32_t port_snd_handle(const void *pointer);
void *port_snd_pointer(uint32_t handle);

#endif // PORT_HAL_SND7_SND_HANDLE_H
