// NitroSDK's mi_wram.c: the shared WRAM's bank setup.
#include "nitro/mi.h"

void func_02078020(MIWramBank bank) {
    reg_MI_WRAMCNT = bank;
}
