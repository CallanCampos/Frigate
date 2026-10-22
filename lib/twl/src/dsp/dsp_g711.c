#include "types.h"
#include "twl/dsp.h"

// TwlSDK's DSP library, the G.711 component (guessed file name, after the SDK's dsp_g711.c): a DSP process, as a COFF
// image in the LTD autoload's .rodata. The opener hands the image to the file system as a file of memory, the loader
// starts the process under the name "g711" and the unloader quits it. ARM code in the DSi-only LTD autoload. The ROM
// names none of it; the functions have the SDK's names where twl/dsp.h knows them, and keep their default names
// otherwise.

// The state of the component. Nothing in the game's code touches its first two words
typedef struct DSPG711State {
    u32 unk_00;
    u32 unk_04;
    BOOL loaded;
} DSPG711State;

static DSPG711State sG711State;
static DSPProcessContext sG711Context;

// Opens the component's image, as a file of memory, in file
void DSPi_OpenStaticComponentG711Core(FSFile *file) {
    if (!FS_IsAvailable()) {
        FS_Init(FS_DMA_NOT_USE);
    }
    FS_InitFile(file);
    FS_CreateFileFromMemory(file, (void *)DSP_G711Image, DSP_COMPONENT_IMAGE_SIZE);
}

// Starts the process from the image in file, into the banks of WRAM B and C that slotB and slotC allow
BOOL DSPi_LoadG711Core(FSFile *file, int slotB, int slotC) {
    if (!sG711State.loaded) {
        DSP_InitProcessContext(&sG711Context, "g711");
        if (DSP_ExecuteProcess(&sG711Context, file, slotB, slotC)) {
            sG711State.loaded = TRUE;
        }
    }
    return sG711State.loaded;
}

void DSPi_UnloadG711Core(void) {
    if (sG711State.loaded) {
        DSP_QuitProcess(&sG711Context);
        sG711State.loaded = FALSE;
    }
}
