// NitroSDK's file system (libfs): the entry points of the whole system, FS_Init and FS_IsAvailable. The file name is a
// guess from NitroSDK's own layout (fs_api.c); no string in the ROM names it. The functions are under swan's names,
// with NitroSDK's in their comments.

#include "nitro/fs.h"

// A word that follows the flag in the ROM and that nothing reads or writes, probably something that only code the link
// dead-stripped used. MWCC drops a static that nothing reads, so it is a global, declared so that .bss holds the
// file's objects in the ROM's order and size
u32 data_0214c048;
static BOOL sIsInitialized;

// NitroSDK's FS_Init: makes the archive "rom", with the default DMA channel given, and the state of the overlays; only
// the first call does
void fs_init(u32 dma) {
    if (!sIsInitialized) {
        sIsInitialized = TRUE;
        fs_init_core(dma);
        func_02070ebc();
    }
}

// NitroSDK's FS_IsAvailable: whether fs_init ran
BOOL fs_init_check(void) {
    return sIsInitialized;
}
