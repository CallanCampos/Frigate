// TwlSDK OS: the end of OS_Init on a DSi (os_application_jump.c, TwlSDK's os_application_jump.o, which aletheia's match
// of Black 1's code to libos.TWL.HYB.a gives with its functions' names). After the arguments of the program that ran
// before are taken, it keeps the ID of the sender, and drops the sender and the arguments when the ID is not accepted.
// The file has TwlSDK's DSi part too, the lookup in the system's list of titles, with the sender's ID, in TwlSDK's
// LTDMAIN section, which the LTD autoload's .ltdmain holds: dsd lists it as lib/twl/src/os/os_application_jump_twl.c,
// which config/merged_sources.toml links from this file's object. This file is ARM code. The ID's name, s_prevTitleId,
// is aletheia's.
#include "nitro/os.h"
#include "twl/os.h"

// The first character of a title ID: the top byte of its low word, which is the first of the four characters of the
// game code
#define TITLE_ID_FIRST_CHAR(titleId) ((s8)((titleId) >> 24))

// The titles that the system leaves in the shared memory: their count, and their IDs
#define OS_TITLE_ID_LIST_MAX 0x76

typedef struct OSTitleIdList {
    u8 count;
    u8 unk_01[0x4f];
    u64 titleIds[OS_TITLE_ID_LIST_MAX];
} OSTitleIdList;

#define HW_OS_TITLE_ID_LIST 0x02ffd800
// The same IDs, which the search reads by their own address
#define OS_TITLE_IDS ((u64 *)0x02ffd850)
// A title the list marks as not to be jumped to
#define OS_TITLE_ID_HIDDEN 0x0000000800000000

// clang-format off
// TwlSDK's section for the DSi-only code and data that the LTD autoload's .ltdmain holds
#pragma define_section LTDMAIN ".ltdmain" ".ltdmain.bss" ".ltdmain" abs32 RWX
// clang-format on

// The title ID of the program that left the arguments, in the LTD autoload's .ltdmain.bss
#pragma section LTDMAIN begin
static u64 s_prevTitleId;
#pragma section LTDMAIN end

// Takes the arguments that the last program left (OS_InitDeliverArg and OS_DecodeDeliverArg of os_deliverArg.c) and
// keeps the ID of the sender. If the ID is not in the system's title list, or its first character is one of '0' to '4',
// the sender is forgotten and the next program gets an empty buffer (set up, then sent and cleared). Otherwise the
// buffer is just forgotten and the ID stays
void OSi_InitPrevTitleId(void) {
    OSDeliverArgInfo info;

    OS_InitDeliverArg(&info, 0);
    if (OS_DecodeDeliverArg() == 0) {
        s_prevTitleId = OS_GetTitleIdFromDeliverArg();
    }
    if ((s_prevTitleId != 0 && OSi_SearchTitleIdFromList(s_prevTitleId, -1) == 0) ||
        TITLE_ID_FIRST_CHAR(s_prevTitleId) == '0' || TITLE_ID_FIRST_CHAR(s_prevTitleId) == '1' ||
        TITLE_ID_FIRST_CHAR(s_prevTitleId) == '2' || TITLE_ID_FIRST_CHAR(s_prevTitleId) == '3' ||
        TITLE_ID_FIRST_CHAR(s_prevTitleId) == '4') {
        s_prevTitleId = 0;
        OS_InitDeliverArg(&info, 0);
        OS_EncodeDeliverArg();
    } else {
        OS_FinalizeDeliverArg();
    }
}

// The DSi part, in the LTD autoload: the lookup in the system's list of titles
#pragma section LTDMAIN begin

u64 OSi_SearchTitleIdFromList(u64 titleId, u64 mask) {
    const OSTitleIdList *list = (const OSTitleIdList *)HW_OS_TITLE_ID_LIST;
    u8 count = list->count;
    int i;

    if (count > OS_TITLE_ID_LIST_MAX) {
        count = OS_TITLE_ID_LIST_MAX;
    }
    for (i = 0; i < count; i++) {
        u64 id = OS_TITLE_IDS[i];
        if ((titleId & mask) == (mask & id) && (id & OS_TITLE_ID_HIDDEN) == 0) {
            return list->titleIds[i];
        }
    }
    return 0;
}

#pragma section LTDMAIN end
