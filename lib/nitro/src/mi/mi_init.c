// NitroSDK's mi_init.c (the name is a guess): MI_Init.
#include "nitro/mi.h"
#include "nitro/os.h"
#include "twl/mi.h"

// MI_Init. The call into the LTD autoload goes through a veneer, which the linker adds after this function
void func_020791d0(void) {
    func_02078020(MI_WRAM_ARM7_ALL);
    if (hw_isDSi()) {
        func_02768138();
    }
    dma_end(0);
}
