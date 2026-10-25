// DWC's functions that differ by console (TWL-DWC's dwc_nonport.c, overlay 11, Thumb): the console's ID and name and
// the game's code, which the login gives GP. Its .data is the console's name and its .bss the game's code.
// The ROM names none of DWC's files. Black 1's DWC, whose functions aletheia matched to TWL-DWC's library objects
// (layout/symbols_sdk.txt), has overlay 11's run of them in libdwcnonport's dwc_nonport.o, libdwcnas's dwc_nasfunc.o
// and dwc_nastime.o, and libdwcmatch's dwc_common.o, dwc_friend.o and dwc_login.o, in this order and with the same
// functions, and each file's .data and .bss follow in the same order.

#include "dwc/dwc_nonport.h"
#include "nitro/hw.h"
#include "nitro/mi.h"

u64 DWCi_Np_GetConsoleId(void) {
    return 0;
}

const char *DWCi_Np_GetConsoleName(void) {
    return "NDS";
}

// The game's code from the ROM's header, as a text
char *DWCi_Np_GetAppGameName(void) {
    static char gameCode[5];

    MI_CpuCopy8((void *)(HW_ROM_HEADER_BUF + 0xc), gameCode, 4);
    gameCode[4] = 0;
    return gameCode;
}
