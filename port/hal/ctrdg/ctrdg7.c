// The host console's cartridge slot (slot 2, for GBA cartridges and accessories) and the ARM7's side of its PXI tag
// (CTRDG), for the decomp's lib/nitro/src/ctrdg. The slot is empty. On the DS the system notes at boot what the slot
// holds (HW_CTRDG_MODULE_INFO_BUF: the ROM's identifying halfword, the flags and the game code), and an empty slot
// reads as 0xffff there, which CTRDG_IsExisting checks first. The game starts the library with CTRDG_DummyInit, which
// sets a callback that ignores the ARM7's words and sends the ARM7 one word (1); the ARM7 takes it without an answer.
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "nitro/ctrdg.h"
#include "nitro/pxi.h"
#include <string.h>

// What an empty slot reads as
#define CTRDG_EMPTY_ROM_CHECK 0xffff

static void CtrdgService(u32 tag, u32 data, BOOL err) {
}

void port_ctrdg7_start(void) {
    CTRDGModuleInfo *info = CTRDG_MODULE_INFO;

    memset(info, 0, sizeof(*info));
    info->moduleID = CTRDG_EMPTY_ROM_CHECK;
    port_pxi_register_service(PXI_FIFO_TAG_CTRDG, CtrdgService);
}
