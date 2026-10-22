// NitroSDK's mi_wram.c: the shared WRAM's bank setup.
#include "nitro/mi.h"

void MI_SetWramBank(MIWram bank) {
    reg_GX_VRAMCNT_WRAM = bank;
}
