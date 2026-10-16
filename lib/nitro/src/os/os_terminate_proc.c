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

// NitroSDK's OS_Terminate, under swan's name sys_exit
void sys_exit(void) {
    if (OSi_terminate.callback != NULL) {
        OSTerminateCallback callback = OSi_terminate.callback;

        OSi_terminate.callback = NULL;
        callback(OSi_terminate.arg);
    }
    if (!OSi_terminate.isTerminating) {
        OSi_terminate.isTerminating = TRUE;
        if (CPU_GetMode() != OS_PROCMODE_IRQ) {
            OS_EnableIrq();
            CPU_IRQEnable();
        }
        if (hw_isDSi()) {
            if (func_02704318()) {
                func_02704328();
            }
        }
        func_02079f44(OS_PXI_COMMAND_TERMINATE);
    }
    abort();
}

// The loop that OS_Terminate ends in: swan names it abort
void abort(void) {
    CPU_IRQDisable();
    while (TRUE) {
        cp15_halt();
    }
}

#pragma thumb off
// clang-format off
// NitroSDK's OS_Halt, under swan's name cp15_halt: stops the CPU until an interrupt. It is assembly in the SDK
asm void cp15_halt(void) {
    mov r0, #0
    mcr p15, 0, r0, c7, c0, 4
    bx lr
}
// clang-format on
#pragma thumb on
