#ifndef PORT_HAL_HW_H
#define PORT_HAL_HW_H

#include <stddef.h>
#include <stdint.h>

// The host memory behind the DS's fixed addresses (ADR 0004). Each region of the DS memory map, by the top byte of its
// addresses (0x01 ITCM, 0x02 main memory, 0x03 shared WRAM, 0x04 I/O registers, 0x05 palettes, 0x06 VRAM, 0x07 OAM,
// 0x08-0x0a the GBA slot), is a 16 MiB window of host memory, so a DS address becomes a host address with
// PORT_HW_ADDR() (port/include/port_host.h). Pages are committed when first touched.

// Prepares the windows; called once at startup, before the game runs. Nothing to do until VRAM banks are mapped
void port_hw_init(void);

// The host address of a DS address, for the platform layer's own use
void *port_hw_pointer(uint32_t address);

// The scanline VCOUNT reports from now on (0-262); the frame loop sets it
void port_hw_set_vcount(uint16_t line);

// One geometry command port write, as port_hw_g3_port and port_hw_gxfifo log them for the geometry engine: the port's
// DS address (0x04000400 for GXFIFO) and the word written
typedef struct PortG3Write {
    uint32_t port;
    uint32_t value;
} PortG3Write;

// The writes logged since the last call, in order, and starts a new log. The pointer is valid until the next call
const PortG3Write *port_hw_g3_take(size_t *count);

// Logs the words that MI's CPU sends and copies (port/hal/mi) store to a geometry port. The game passes them the
// port's address, &reg_G3X_GXFIFO or a reg_G3_* register, and evaluating that here logs an entry and gives its value,
// which holds one word. When dest is that value, of the last entry logged, the entry is taken back and count words from
// src are logged instead: all to the entry's port when step is 0, or each to the address step bytes after the last
// one, as an STM stores them (GXFIFO's mirrors, 0x04000400-0x0400043f, are logged as 0x04000400). Returns 1 then;
// otherwise it logs nothing and returns 0, and dest is memory, for the caller to store to
int port_hw_g3_send(volatile void *dest, const void *src, uint32_t count, uint32_t step);

// The geometry engine's readback registers (GXSTAT, RAM_COUNT, VTXRAM_COUNT, CLIPMTX_RESULT, VECMTX_RESULT): each
// gives the memory one access to the register uses. The program attaches the engine's (port/gfx/ge/ge_hal.h); until
// then, as in the platform layer's own tests, the engine reads as idle, with empty buffers and zero matrices
typedef struct PortHwG3Readbacks {
    void *(*gxstat)(void);
    void *(*ram_count)(void);
    void *(*vtxram_count)(void);
    void *(*clipmtx_result)(void);
    void *(*vecmtx_result)(void);
} PortHwG3Readbacks;

void port_hw_attach_g3_readbacks(const PortHwG3Readbacks *readbacks);

#endif // PORT_HAL_HW_H
