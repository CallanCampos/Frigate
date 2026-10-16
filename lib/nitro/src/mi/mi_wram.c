// NitroSDK's mi_wram.c: the shared WRAM's bank setup.
#include "nitro/mi.h"

void MI_SetWramBank(MIWramBank bank) {
    reg_MI_WRAMCNT = bank;
}
