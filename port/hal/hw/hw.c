// The host side of the DS's fixed addresses and of the registers whose accesses act (ADR 0004).
#include "hal/hw/hw.h"
#include "include/port_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The windows PORT_HW_ADDR() reads, 16 MiB each and page aligned (16 KiB on Apple Silicon), so that VRAM banks can be
// mapped into them later. They are zero-filled memory the system commits only when touched
#define REGION_SIZE (1u << 24)
#define REGION_STORAGE __attribute__((aligned(16384)))
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

// TM0CNT_L and TM1CNT_L, read as counters. Nothing the boot path needs reads them yet; until the virtual clock drives
// them (port/hal/os), each read advances them by one, which keeps them deterministic
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

// The geometry command ports and GXFIFO: each write is logged for the geometry engine (phase 2). The log grows as
// needed; a frame of the game's 3D sends some tens of thousands of words
static PortG3Write *sG3Log;
static size_t sG3Count;
static size_t sG3Capacity;
static PortG3Write *sG3Taken;

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
    free(sG3Taken);
    sG3Taken = sG3Log;
    *count = sG3Count;
    sG3Log = NULL;
    sG3Count = 0;
    sG3Capacity = 0;
    return sG3Taken;
}

// The geometry engine's status and readbacks. Until the engine exists (phase 2) it is always idle with empty buffers,
// and the matrices it reports are zero
static uint32_t sGxStat;
static uint16_t sRamCount;
static uint16_t sVtxRamCount;
static uint32_t sClipMtx[16];
static uint32_t sVecMtx[9];

void *port_hw_gxstat(void) {
    return &sGxStat;
}

void *port_hw_ram_count(void) {
    return &sRamCount;
}

void *port_hw_vtxram_count(void) {
    return &sVtxRamCount;
}

void *port_hw_clipmtx_result(void) {
    return sClipMtx;
}

void *port_hw_vecmtx_result(void) {
    return sVecMtx;
}
