#include "types.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/c_gear_savedata.h"
#include "save/save_control.h"
#include "text/script/castelia_city_13.h"

// The word at the end of the C-Gear's save block, which c_gear_savedata.c owns: its initial state, "Awesome!" of the
// passerby survey's messages, and its accessors. The ROM has no string for the file, so the name is a guess.
// Block 0x20.

#define SAVE_BLOCK_CGEAR 0x20
#define CGEAR_NAME_LENGTH 9

static u32 getNameSize(void);

static u32 getNameSize(void) {
    return CGEAR_NAME_LENGTH * sizeof(u16);
}

void func_020114a8(u16 *name) {
    MsgData *msgData;
    StrBuf *str;

    sys_memset(name, 0, getNameSize());
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_CASTELIA_CITY_13, HEAPID_TAIL(HEAPID_SAVEDATA));
    str = GFL_MsgDataLoadStrbufNew(msgData, CasteliaCity13_Text_Awesome);
    GFL_StrBufStoreString(str, name, CGEAR_NAME_LENGTH);
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
}

void *func_020114f0(SaveControl *save) {
    CGearSave *cgear = SaveControl_GetBlockPtr(save, SAVE_BLOCK_CGEAR);

    return cgear->name;
}

void func_020114fc(void *saveBlock, StrBuf *name) {
    GFL_StrBufStoreString(name, saveBlock, CGEAR_NAME_LENGTH);
}

const u16 *func_0201150c(void *saveBlock) {
    return saveBlock;
}
