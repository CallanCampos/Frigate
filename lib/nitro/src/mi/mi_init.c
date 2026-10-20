// NitroSDK's mi_init.c (the name is a guess): MI_Init.
#include "nitro/mi.h"
#include "nitro/os.h"
#include "twl/mi.h"

// MI_Init. The call into the LTD autoload goes through a veneer, which the linker adds after this function
void MI_Init(void) {
    MI_SetWramBank(MI_WRAM_ARM7_ALL);
    if (OS_IsRunOnTwl()) {
        MI_InitNDma();
    }
    MI_StopDma(0);
}
