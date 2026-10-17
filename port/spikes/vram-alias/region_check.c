// The static-region design compiled for each target. CMakeLists.txt runs it on the host, compiles it for three Windows
// targets, and compiles it with SDK_U16_TABLE expecting the error. It declares the views as a PLATFORM_PC build would
// over a static array, with HW_LCDC_VRAM_* as host addresses, and builds the three kinds of use BW2 makes of them
#include "vram.h"

static _Alignas(VRAM_STATIC_REGION_ALIGN) uint8_t gHostVram[VRAM_REGION_SIZE];

// LCDC is the first view, after the first guard slot (vram.c)
#define HW_LCDC_VRAM ((uintptr_t)gHostVram + VRAM_SLOT_SIZE)
#define HW_LCDC_VRAM_A (HW_LCDC_VRAM + 0x00000)
#define HW_LCDC_VRAM_B (HW_LCDC_VRAM + 0x20000)
#define HW_LCDC_VRAM_C (HW_LCDC_VRAM + 0x40000)
#define HW_LCDC_VRAM_D (HW_LCDC_VRAM + 0x60000)
#define HW_LCDC_VRAM_E (HW_LCDC_VRAM + 0x80000)

// screentex.c's SCREEN_CAPTURE_BANK_INFO: pointers made from the constants in a static table. These are address
// constants, so they compile on every target
void *const gCaptureBankAddrs[4] = {
    (void *)HW_LCDC_VRAM_A,
    (void *)HW_LCDC_VRAM_B,
    (void *)HW_LCDC_VRAM_C,
    (void *)HW_LCDC_VRAM_D,
};

#if defined(SDK_U16_TABLE)
// gx_load3d.c's TEXPLTT_VRAM_BANK_ADDRS and TEX_VRAM_BANK_RANGES: the addresses shifted right by 12 into u16. A
// shifted address is not a constant expression, so this fails on every target ("initializer element is not a
// compile-time constant"); it couldn't hold a host address anyway
const uint16_t gTexPlttBankAddrs[2] = { 0, HW_LCDC_VRAM_E >> 12 };
#endif

#if !defined(_WIN32) && !defined(SDK_U16_TABLE)
#include <stdio.h>

// gx_load2d.c's sBGExtPlttDestA and gx_load3d.c's sTexAddr keep these addresses in u32 variables, as do game sites such
// as btlv_gauge.c's (void *)((u32)vram + ...). On a 64-bit host whose image is above 4 GiB (every macOS arm64
// executable: __PAGEZERO covers the low 4 GiB) the address doesn't survive
int main(void) {
    volatile uint32_t sBGExtPlttDestA = (uint32_t)HW_LCDC_VRAM_E;
    int failures = 0;

    for (int i = 0; i < 4; i++) {
        if ((uintptr_t)gCaptureBankAddrs[i] != HW_LCDC_VRAM + (uintptr_t)i * 0x20000) {
            failures++;
        }
    }
    printf("static region at %p: the static pointer table is %s; HW_LCDC_VRAM_E in a u32 %s\n", (void *)gHostVram,
           failures == 0 ? "right" : "WRONG",
           (uintptr_t)sBGExtPlttDestA == HW_LCDC_VRAM_E ? "survives" : "is truncated");
    return failures != 0;
}
#endif
