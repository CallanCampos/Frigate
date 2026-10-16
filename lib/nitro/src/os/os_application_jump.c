// TwlSDK OS: the end of OS_Init on a DSi (os_application_jump.c, a guessed name after TwlSDK's application-jump API,
// which this code serves). After the arguments of the program that ran before are taken, it keeps the ID of the
// sender, and drops the sender and the arguments when the ID is not accepted. This file is ARM code.
#include "nitro/os.h"
#include "twl/os.h"

// The first character of a title ID: the top byte of its low word, which is the first of the four characters of the
// game code
#define TITLE_ID_FIRST_CHAR(titleId) ((s8)((titleId) >> 24))

// Takes the arguments that the last program left (OS_InitDeliverArg and OS_DecodeDeliverArg of os_deliverArg.c) and
// keeps the ID of the sender. If the ID is not in the system's title list, or its first character is one of '0' to '4',
// the sender is forgotten and the next program gets an empty buffer (set up, then sent and cleared). Otherwise the
// buffer is just forgotten and the ID stays
void func_0207c888(void) {
    OSDeliverArgInfo info;

    OS_InitDeliverArg(&info, 0);
    if (OS_DecodeDeliverArg() == 0) {
        data_0276b0dc = func_0207c6a4();
    }
    if ((data_0276b0dc != 0 && func_0276934c(data_0276b0dc, -1) == 0) || TITLE_ID_FIRST_CHAR(data_0276b0dc) == '0' ||
        TITLE_ID_FIRST_CHAR(data_0276b0dc) == '1' || TITLE_ID_FIRST_CHAR(data_0276b0dc) == '2' ||
        TITLE_ID_FIRST_CHAR(data_0276b0dc) == '3' || TITLE_ID_FIRST_CHAR(data_0276b0dc) == '4') {
        data_0276b0dc = 0;
        OS_InitDeliverArg(&info, 0);
        OS_EncodeDeliverArg();
    } else {
        func_0207c698();
    }
}
