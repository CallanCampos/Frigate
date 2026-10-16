// NitroSDK OS: ending the program (os_terminate_proc.c, after the SDK's file of that name). OS_Terminate runs a
// callback, tells the ARM7 and halts the CPU for good; the halt itself is assembly in the SDK, in ARM code. On a DSi it
// first unmounts the FAT file system, which the LTD autoload has (the linker's veneers reach it).
#include "nitro/os.h"
#include "twl/fatfs.h"

typedef void (*OSTerminateCallback)(void *arg);

// The function that OS_Terminate calls, once, with its argument, and whether it was entered already
static struct {
    void *arg;
    OSTerminateCallback callback;
    BOOL isTerminating;
} OSi_terminate;

// NitroSDK's OS_Terminate (swan's sys_exit)
void OS_Terminate(void) {
    if (OSi_terminate.callback != NULL) {
        OSTerminateCallback callback = OSi_terminate.callback;

        OSi_terminate.callback = NULL;
        callback(OSi_terminate.arg);
    }
    if (!OSi_terminate.isTerminating) {
        OSi_terminate.isTerminating = TRUE;
        if (OS_GetProcMode() != OS_PROCMODE_IRQ) {
            OS_EnableIrq();
            OS_EnableInterrupts();
        }
        if (OS_IsRunOnTwl()) {
            if (func_02704318()) {
                func_02704328();
            }
        }
        OSi_SendToPxi(OS_PXI_COMMAND_TERMINATE);
    }
    OSi_TerminateCore();
}

// The loop that OS_Terminate ends in. The name is a guess at TwlSDK's; swan's is abort
void OSi_TerminateCore(void) {
    OS_DisableInterrupts();
    while (TRUE) {
        OS_Halt();
    }
}

#pragma thumb off
// clang-format off
// NitroSDK's OS_Halt (swan's cp15_halt): stops the CPU until an interrupt. It is assembly in the SDK
asm void OS_Halt(void) {
    mov r0, #0
    mcr p15, 0, r0, c7, c0, 4
    bx lr
}
// clang-format on
#pragma thumb on
