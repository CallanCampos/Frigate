// TwlSDK's DSi part of os_application_jump.c (ARM9 main), in the LTD autoload: the lookup in the system's list of
// titles, and the title ID of the program that sent the arguments, which main's half keeps here. ARM code, as main's
// half is. The file name is inferred from main's half; the names are ours
#include "nitro/os.h"
#include "twl/os.h"

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

u64 OSi_SenderTitleId;

#pragma define_section LTDMAIN ".ltdmain"                                                                              \
                               ".ltdmain.bss"                                                                          \
                               ".ltdmain" abs32 RWX
#pragma section LTDMAIN begin

u64 OSi_FindSystemTitleId(u64 titleId, u64 mask) {
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
