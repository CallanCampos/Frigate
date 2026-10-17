#ifndef PORT_HOST_H
#define PORT_HOST_H

// Force-included into every decomp C file of the host build (port/CMakeLists.txt): what MWCC provides built in and
// clang spells differently. Nothing here may change what a file means on the DS.

// MWCC's memcpy built-in, which the DPW, DWC and POKE_NET libraries call by this name
#define __memcpy __builtin_memcpy

// The DS's fixed addresses (ADR 0004). The host variants of the SDK headers (port/tools/gen_host_tree.py) write each
// hardware or memory-map address as PORT_HW_ADDR(address), which finds it in host memory: a 16 MiB window for each
// region of the memory map, by the address's top byte, each a static array of port/hal/hw so that an address stays a
// constant the compiler can fold and static tables can hold. Registers whose accesses act go through the accessors
// below instead, each returning the memory the access uses.
#include <stdint.h>

extern unsigned char port_hw_mem_01[], port_hw_mem_02[], port_hw_mem_03[], port_hw_mem_04[], port_hw_mem_05[],
    port_hw_mem_06[], port_hw_mem_07[], port_hw_mem_08[], port_hw_mem_09[], port_hw_mem_0a[], port_hw_mem_none[];
#define PORT_HW_WINDOW(region)                                                                                         \
    ((region) == 0x01   ? port_hw_mem_01                                                                               \
     : (region) == 0x02 ? port_hw_mem_02                                                                               \
     : (region) == 0x03 ? port_hw_mem_03                                                                               \
     : (region) == 0x04 ? port_hw_mem_04                                                                               \
     : (region) == 0x05 ? port_hw_mem_05                                                                               \
     : (region) == 0x06 ? port_hw_mem_06                                                                               \
     : (region) == 0x07 ? port_hw_mem_07                                                                               \
     : (region) == 0x08 ? port_hw_mem_08                                                                               \
     : (region) == 0x09 ? port_hw_mem_09                                                                               \
     : (region) == 0x0a ? port_hw_mem_0a                                                                               \
                        : port_hw_mem_none)
#define PORT_HW_ADDR(address) ((uintptr_t)(PORT_HW_WINDOW(((address) >> 24) & 0xff) + ((address) & 0xffffff)))

void *port_hw_vcount(void);
void *port_hw_timer_counter0(void);
void *port_hw_timer_counter1(void);
void *port_hw_div_result(void);
void *port_hw_divrem_result(void);
void *port_hw_sqrt_result(void);
void *port_hw_gxfifo(void);
void *port_hw_g3_port(uint32_t port);
void *port_hw_gxstat(void);
void *port_hw_ram_count(void);
void *port_hw_vtxram_count(void);
void *port_hw_clipmtx_result(void);
void *port_hw_vecmtx_result(void);

// The size the host gives one of the game's heaps (src/gfl/heapsys.c), larger than the DS's for 64-bit structs
// (port/hal/gfl/heap_policy.c). The arguments are u32, u32 and BOOL
unsigned int port_heap_size(unsigned int index, unsigned int size, int root);

// The 32-bit handles that sound data keeps in place of a host pointer where the DS keeps an ARM9 address in a 32-bit
// field of a file (port/hal/snd7/snd_handle.h): NitroSystem's file table (lib/nnsys/src/snd/snd_internal.h) and the
// links of wave archives and banks (lib/nitro/include/nitro/snd.h). 0 is NULL both ways
uint32_t port_snd_handle(const void *pointer);
void *port_snd_pointer(uint32_t handle);

#endif // PORT_HOST_H
