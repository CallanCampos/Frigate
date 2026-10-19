// The DS's memory map on the host (hw.h, ADR 0004): the windows of host memory that PORT_HW_ADDR() turns the DS's
// fixed addresses into, and the accessors of the registers whose accesses act, which the build's host variants of the
// SDK headers call instead of reading or writing memory (port/tools/gen_host_tree.py, HW_REGISTER_POLICY): VCOUNT,
// which the OS core's virtual clock sets; the timer counters; the divider and square root (GBATEK "DS Maths"), which
// NitroSDK's fixed-point maths uses (lib/nitro/src/fx/fx_cp.c); the geometry command ports and GXFIFO, whose writes are
// logged for the geometry engine (port/gfx/ge); and the 3D readbacks, which the program attaches to that engine.
// Every register write and fixed address of the game's code lands here: the screens' registers, palettes, VRAM and OAM
// that the renderers read, the shared memory the ARM7's services fill, and the 3D commands of everything that draws.
#include "hal/hw/hw.h"
#include "hal/host/host.h"
#include "include/port_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The windows PORT_HW_ADDR() reads, 16 MiB each and page aligned, so that the VRAM banks can be mapped into region
// 0x06's window by pages (port/hal/gx/vram.c). They are zero-filled memory the system commits only when touched. The
// alignment is PORT_HOST_PAGE_ALIGNMENT (port/hal/host/host.h): 16 KiB, Apple Silicon's page and the largest of the
// hosts, but 4 KiB on Windows, which copies the banks instead, and in an x86_64 Mach-O program (the macOS app's Intel
// slice, ADR 0015), whose linker holds a section's alignment to its 4 KiB page
#define REGION_SIZE (1u << 24)
#define REGION_STORAGE __attribute__((aligned(PORT_HOST_PAGE_ALIGNMENT)))
REGION_STORAGE unsigned char port_hw_mem_01[REGION_SIZE]; // ITCM
REGION_STORAGE unsigned char port_hw_mem_02[REGION_SIZE]; // main memory and its system area (0x02fff000)
REGION_STORAGE unsigned char port_hw_mem_03[REGION_SIZE]; // shared WRAM
REGION_STORAGE unsigned char port_hw_mem_04[REGION_SIZE]; // I/O registers
REGION_STORAGE unsigned char port_hw_mem_05[REGION_SIZE]; // palettes
REGION_STORAGE unsigned char port_hw_mem_06[REGION_SIZE]; // VRAM
REGION_STORAGE unsigned char port_hw_mem_07[REGION_SIZE]; // OAM
REGION_STORAGE unsigned char port_hw_mem_08[REGION_SIZE]; // the GBA slot
REGION_STORAGE unsigned char port_hw_mem_09[REGION_SIZE];
REGION_STORAGE unsigned char port_hw_mem_0a[REGION_SIZE];
// Any other region: a DS address the port doesn't expect
REGION_STORAGE unsigned char port_hw_mem_none[REGION_SIZE];

void port_hw_init(void) {
}

void *port_hw_pointer(uint32_t address) {
    return (void *)PORT_HW_ADDR(address);
}

static uint32_t read32(uint32_t address) {
    uint32_t value;
    memcpy(&value, port_hw_pointer(address), sizeof(value));
    return value;
}

static uint64_t read64(uint32_t address) {
    uint64_t value;
    memcpy(&value, port_hw_pointer(address), sizeof(value));
    return value;
}

// VCOUNT
static uint16_t sVCount;

void port_hw_set_vcount(uint16_t line) {
    sVCount = line;
}

void *port_hw_vcount(void) {
    return &sVCount;
}

// TM0CNT_L and TM1CNT_L, read as counters. The SDK's tick and alarms don't read them: port/hal/os runs timers 0 and 1
// on the virtual clock (os_tick.c, os_alarm.c, core.c), and nothing else the host build compiles reads a timer's
// counter. Each read advances the counter by one, which keeps any reader deterministic
static uint16_t sTimerCounter[2];

void *port_hw_timer_counter0(void) {
    sTimerCounter[0]++;
    return &sTimerCounter[0];
}

void *port_hw_timer_counter1(void) {
    sTimerCounter[1]++;
    return &sTimerCounter[1];
}

// The divider (GBATEK "DS Maths"). DIVCNT's mode: 0 is 32 by 32 bits, 1 and 3 are 64 by 32, 2 is 64 by 64. Operands
// are signed; 32-bit ones are the low words, and results are sign-extended to 64 bits. Dividing by zero gives the
// numerator as the remainder and 1 with the sign opposite to the numerator's as the quotient; -MAX / -1 gives -MAX. On
// either overflow in the 32-bit mode, the upper word of the sign-extended quotient is inverted.
enum {
    REG_DIVCNT = 0x04000280,
    REG_DIV_NUMER = 0x04000290,
    REG_DIV_DENOM = 0x04000298,
    REG_SQRTCNT = 0x040002b0,
    REG_SQRT_PARAM = 0x040002b8,
};

static uint64_t sDivResult;
static uint64_t sDivRemainder;

// Works out both results from DIVCNT's mode and the operands as they are now, by the rules above. Each read of a
// result calls it, so a read sees the operands written last
static void divide(void) {
    uint32_t mode = read32(REG_DIVCNT) & 3;
    uint64_t numer64 = read64(REG_DIV_NUMER);
    uint64_t denom64 = read64(REG_DIV_DENOM);
    int64_t quotient;
    int64_t remainder;
    if (mode == 0) {
        int32_t numer = (int32_t)(uint32_t)numer64;
        int32_t denom = (int32_t)(uint32_t)denom64;
        int overflow = 1;
        if (denom == 0) {
            quotient = numer < 0 ? 1 : -1;
            remainder = numer;
        } else if (numer == INT32_MIN && denom == -1) {
            quotient = INT32_MIN;
            remainder = 0;
        } else {
            quotient = numer / denom;
            remainder = numer % denom;
            overflow = 0;
        }
        sDivResult = (uint64_t)quotient;
        if (overflow) {
            sDivResult ^= 0xffffffff00000000ull;
        }
        sDivRemainder = (uint64_t)remainder;
        return;
    }
    int64_t numer = (int64_t)numer64;
    int64_t denom = mode == 2 ? (int64_t)denom64 : (int64_t)(int32_t)(uint32_t)denom64;
    if (denom == 0) {
        quotient = numer < 0 ? 1 : -1;
        remainder = numer;
    } else if (numer == INT64_MIN && denom == -1) {
        quotient = INT64_MIN;
        remainder = 0;
    } else {
        quotient = numer / denom;
        remainder = numer % denom;
    }
    sDivResult = (uint64_t)quotient;
    sDivRemainder = (uint64_t)remainder;
}

void *port_hw_div_result(void) {
    divide();
    return &sDivResult;
}

void *port_hw_divrem_result(void) {
    divide();
    return &sDivRemainder;
}

// The square root unit: the floor of the square root of the unsigned 32-bit (SQRTCNT mode 0) or 64-bit (mode 1)
// parameter
static uint32_t sSqrtResult;

void *port_hw_sqrt_result(void) {
    uint64_t value = read64(REG_SQRT_PARAM);
    if ((read32(REG_SQRTCNT) & 1) == 0) {
        value = (uint32_t)value;
    }
    // Bit by bit, which is exact for every 64-bit value
    uint64_t root = 0;
    uint64_t bit = 1ull << 62;
    while (bit > value) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (value >= root + bit) {
            value -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    sSqrtResult = (uint32_t)root;
    return &sSqrtResult;
}

// The geometry command ports and GXFIFO: each write is logged for the geometry engine (port/gfx/ge). The log grows
// as needed; a frame of the game's 3D sends some tens of thousands of words. Taking the log swaps it with the one taken
// before, so that the engine's runs at every readback allocate nothing
static PortG3Write *sG3Log;
static size_t sG3Count;
static size_t sG3Capacity;
static PortG3Write *sG3Taken;
static size_t sG3TakenCapacity;

static void *log_g3_write(uint32_t port) {
    if (sG3Count == sG3Capacity) {
        sG3Capacity = sG3Capacity == 0 ? 4096 : sG3Capacity * 2;
        sG3Log = realloc(sG3Log, sG3Capacity * sizeof(*sG3Log));
        if (sG3Log == NULL) {
            fprintf(stderr, "port: out of memory for the geometry command log\n");
            exit(1);
        }
    }
    PortG3Write *entry = &sG3Log[sG3Count++];
    entry->port = port;
    entry->value = 0;
    return &entry->value;
}

void *port_hw_g3_port(uint32_t port) {
    return log_g3_write(port);
}

void *port_hw_gxfifo(void) {
    return log_g3_write(0x04000400);
}

int port_hw_g3_send(volatile void *dest, const void *src, uint32_t count, uint32_t step) {
    if (sG3Count == 0 || dest != &sG3Log[sG3Count - 1].value) {
        return 0;
    }
    uint32_t port = sG3Log[--sG3Count].port;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t address = port + i * step;
        uint32_t value;
        memcpy(&value, (const unsigned char *)src + 4 * i, sizeof(value));
        *(uint32_t *)log_g3_write(address >= 0x04000400 && address < 0x04000440 ? 0x04000400 : address) = value;
    }
    return 1;
}

const PortG3Write *port_hw_g3_take(size_t *count) {
    PortG3Write *log = sG3Log;
    size_t capacity = sG3Capacity;

    sG3Log = sG3Taken;
    sG3Capacity = sG3TakenCapacity;
    sG3Taken = log;
    sG3TakenCapacity = capacity;
    *count = sG3Count;
    sG3Count = 0;
    return sG3Taken;
}

// The geometry engine's status and readbacks: the attached engine's, or an idle engine's
static const PortHwG3Readbacks *sG3Readbacks;
static uint32_t sGxStat;
static uint16_t sRamCount;
static uint16_t sVtxRamCount;
static uint32_t sClipMtx[16];
static uint32_t sVecMtx[9];

void port_hw_attach_g3_readbacks(const PortHwG3Readbacks *readbacks) {
    sG3Readbacks = readbacks;
}

void *port_hw_gxstat(void) {
    return sG3Readbacks != NULL ? sG3Readbacks->gxstat() : &sGxStat;
}

void *port_hw_ram_count(void) {
    return sG3Readbacks != NULL ? sG3Readbacks->ram_count() : &sRamCount;
}

void *port_hw_vtxram_count(void) {
    return sG3Readbacks != NULL ? sG3Readbacks->vtxram_count() : &sVtxRamCount;
}

void *port_hw_clipmtx_result(void) {
    return sG3Readbacks != NULL ? sG3Readbacks->clipmtx_result() : sClipMtx;
}

void *port_hw_vecmtx_result(void) {
    return sG3Readbacks != NULL ? sG3Readbacks->vecmtx_result() : sVecMtx;
}
