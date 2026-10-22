// NitroSDK's file system (libfs): the entry points of the whole system, FS_Init and FS_IsAvailable. The file name is a
// guess (fs_api.c); no string in the ROM names it, and ntrtwl's reconstruction of NitroSDK 4.2 has the two functions
// in fs_file.c. The functions and is_init have NitroSDK's names, as that reconstruction has them.

#include "nitro/fs.h"

// A word that follows the flag in the ROM and that nothing reads or writes, probably something that only code the link
// dead-stripped used; the name is ours. MWCC drops a static that nothing reads, so it is a global, declared so that
// .bss holds the file's objects in the ROM's order and size
u32 FSi_UnusedWord;
static BOOL is_init; // swan's g_IsFSInitialized

// NitroSDK's FS_Init: makes the archive "rom", with the default DMA channel given, and the state of the overlays; only
// the first call does
void FS_Init(u32 dma) {
    if (!is_init) {
        is_init = TRUE;
        FSi_InitRom(dma);
        FSi_InitOverlay();
    }
}

// NitroSDK's FS_IsAvailable: whether FS_Init ran
BOOL FS_IsAvailable(void) {
    return is_init;
}
