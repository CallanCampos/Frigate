// NitroSDK OS: restarting the game (os_reset.c). OS_InitReset registers the callback for the OS tag of the PXI FIFO,
// and OS_ResetSystem shuts down what the ARM9 started and has the ARM7 reset the console, through code in the ITCM
// that is probably part of this file too (OSi_DoResetSystem, not yet registered). ARM code.
#include "nitro/card.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "twl/mi.h"
#include "twl/os.h"

// The high word of the DSi's title ID in the system area. Bit 2 of it is set for a program installed on the NAND
#define HW_TWL_TITLE_ID_HI 0x02ffe234
#define TWL_TITLE_ID_HI_NAND_APP 0x00000004

static u16 OSi_IsInitReset;

// NitroSDK's OS_InitReset
void func_0207c248(void) {
    if (OSi_IsInitReset) {
        return;
    }
    OSi_IsInitReset = TRUE;
    PXI_Init();
    while (!func_0207ca48(PXI_FIFO_TAG_OS, PXI_PROC_ARM7)) {
    }
    func_0207c9fc(PXI_FIFO_TAG_OS, func_02079f10);
}

// NitroSDK's OS_ResetSystem: the reset parameter goes to the shared memory for the next program. A Download Play
// child, or a DSi program that runs from the NAND, can't reset to a cartridge, so it terminates
void sys_reset(u32 parameter) {
    if (*(u16 *)HW_BOOT_CHECK_INFO_BUF == OS_BOOTTYPE_DOWNLOAD_MB ||
        (hw_isDSi() == TRUE && (*(u32 *)HW_TWL_TITLE_ID_HI & TWL_TITLE_ID_HI_NAND_APP))) {
        sys_exit();
    }
    if (hw_isDSi()) {
        func_02769578();
    }
    func_0206eef0(cart_key_create());
    CPU_SetInterrupts(OS_IE_FIFO_RECV);
    CPU_AcknowledgeInterrupts(0xffffffff);
    dma_endAll();
    if (hw_isDSi()) {
        func_02768298();
    }
    *(u32 *)HW_RESET_PARAMETER_BUF = parameter;
    func_02079f44(OS_PXI_COMMAND_RESET);
    func_01ff8fac();
}
