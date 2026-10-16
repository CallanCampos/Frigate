#include "types.h"
#include "twl/dsp.h"

// TwlSDK's DSP library, the G.711 component (guessed file name, after the SDK's dsp_g711.c): a DSP process, as a COFF
// image in the LTD autoload's .rodata. The opener hands the image to the file system as a file of memory, the loader
// starts the process under the name "g711" and the unloader quits it. ARM code in the DSi-only LTD autoload. The ROM
// names none of it; the functions keep their default names, and the SDK's are in twl/dsp.h.

// The state of the component. Nothing in the game's code touches its first two words
typedef struct DSPG711State {
    u32 unk_00;
    u32 unk_04;
    BOOL loaded;
} DSPG711State;

static DSPG711State sG711State;
static DSPProcessContext sG711Context;

// Opens the component's image, as a file of memory, in file
BOOL func_0270723c(FSFile *file) {
    if (!fs_init_check()) {
        fs_init(FS_DMA_NOT_USE);
    }
    finit(file);
    return extfs_fopen(file, DSP_G711Image, DSP_COMPONENT_IMAGE_SIZE);
}

// Starts the process from the image in file, into the banks of WRAM B and C that slotB and slotC allow
BOOL func_02707278(FSFile *file, int slotB, int slotC) {
    if (!sG711State.loaded) {
        func_027058d8(&sG711Context, "g711");
        if (func_02705c24(&sG711Context, file, slotB, slotC)) {
            sG711State.loaded = TRUE;
        }
    }
    return sG711State.loaded;
}

void func_027072e0(void) {
    if (sG711State.loaded) {
        func_02705e04(&sG711Context);
        sG711State.loaded = FALSE;
    }
}
