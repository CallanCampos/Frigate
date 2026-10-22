#ifndef POKEBW2_TWL_FATFS_H
#define POKEBW2_TWL_FATFS_H

#include "types.h"

// TwlSDK's FATFS, the DSi's file system, whose ARM9 side sends its requests to the ARM7 over the PXI tag 0x14. Only the
// part that the game keeps is here: the check that the library is set up and the one request that the program's exit
// (OS_Terminate, on a DSi) sends the ARM7. The ROM names none of it: the functions have the names of libntr's
// reconstruction of TwlSDK's fatfs/common/api.h, by what they do and the command they send

// Whether the library is set up: a flag in its bss, which nothing in the game sets
BOOL FATFSi_IsInitialized(void);
// Sends the ARM7 the request 0x1a (FATFS_COMMAND_UNMOUNT_ALL) and waits for the answer. TRUE when the ARM7 reports
// success (OS_Terminate ignores it)
BOOL FATFS_UnmountAll(void);

#endif // POKEBW2_TWL_FATFS_H
