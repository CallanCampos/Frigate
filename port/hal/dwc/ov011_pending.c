// Overlay 11 code that DWC's initialization reaches before upstream has written its C. Overlay 11 (DWC and GameSpy) is
// claimed upstream and the port leaves it to that claim; until its C lands, the game's boot calls
// DWC_InitForProduction (lib/dwc/src/base/dwc_init.c), which needs the two GameSpy setters and two buffers below.
// Each gives the effect the original has as far as playing offline depends on it; Nintendo Wi-Fi Connection comes in
// phase 7 (PLAN.md §5.4) and needs the real code. When upstream's C for any of these lands, the link fails with a
// duplicate symbol: delete that definition here.
#include "types.h"
#include "dwc/dwc.h"

// GameSpy's server choice: the original picks one of three host names by the mode (0 development, 1 production,
// 2 a third) and hands it to func_ov011_021694d4, which has no C yet either. Offline nothing reads it
static int sGameSpyServer = -1;

void func_ov011_02169df8(int server) {
    sGameSpyServer = server;
}

// GameSpy's memory callbacks: the original stores the four in a row at data_ov011_02186ec4, as here
static struct {
    void *(*alloc)(u32 size);
    void (*free)(void *ptr);
    void *(*realloc)(void *ptr, u32 size);
    void *(*memalign)(u32 align, u32 size);
} sGameSpyMemory;

void func_ov011_0216ddc8(void *(*alloc)(u32 size), void (*free)(void *ptr), void *(*realloc)(void *ptr, u32 size),
                         void *(*memalign)(u32 align, u32 size)) {
    sGameSpyMemory.alloc = alloc;
    sGameSpyMemory.free = free;
    sGameSpyMemory.realloc = realloc;
    sGameSpyMemory.memalign = memalign;
}

// The game name and the statistics server's host name that DWC_InitForProduction copies in: 256 and 64 bytes on the
// DS (the distance to the next symbols)
char data_ov011_021917bc[256];
char data_ov011_02188fe4[64];
