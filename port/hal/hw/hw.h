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

#endif // PORT_HAL_HW_H
