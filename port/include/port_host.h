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
// The card slot's SPI bus and the infrared chip on it (port/hal/card/card_spi.h)
void *port_hw_auxspicnt(void);
void *port_hw_auxspidata(void);

// The size the host gives one of the game's heaps (src/gfl/heapsys.c), larger than the DS's for 64-bit structs
// (port/hal/gfl/heap_policy.c). The arguments are u32, u32 and BOOL
unsigned int port_heap_size(unsigned int index, unsigned int size, int root);

// The work a particle system's SPL manager allocates from on a host (src/gfl/particle.c), in place of the caller's
// buffer, which is sized for the DS's structs (port/hal/gfl/particle_work.c). The size is u32, the system's ID 0 to 15
unsigned int port_particle_work_size(unsigned int size);
void *port_particle_work(unsigned int id, unsigned int size);
void port_particle_work_free(unsigned int id);

// The 32-bit handles that sound data keeps in place of a host pointer where the DS keeps an ARM9 address in a 32-bit
// field of a file (port/hal/snd7/snd_handle.h): NitroSystem's file table (lib/nnsys/src/snd/snd_internal.h) and the
// links of wave archives and banks (lib/nitro/include/nitro/snd.h). 0 is NULL both ways
uint32_t port_snd_handle(const void *pointer);
void *port_snd_pointer(uint32_t handle);

// The DS's address of OS_GetTick as the ARM9's code takes it, bit 0 set for a Thumb function: OS_GetTick's address in
// config/<version>/arm9/symbols.txt, and the word in the ROM's two literal pools of lib/dwc/src/bm/util_wifiidtool.c
// (0x020572f0 and 0x0205751c in Black 2, 0x0205731c and 0x02057548 in White 2). DWC adds the function's address to the
// seed of the console's pending Wi-Fi ID where it meant the tick count (that file's BUG), and the ID goes into the
// save; a host function's address moves at every launch
#ifdef WHITE2
#define PORT_DS_OS_GETTICK_ADDRESS 0x0207bb39u
#else
#define PORT_DS_OS_GETTICK_ADDRESS 0x0207bb0du
#endif

#endif // PORT_HOST_H
