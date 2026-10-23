// How the game stops, on the host: NitroSDK's OS_Terminate and OSi_TerminateCore (os_terminate_proc.c, whose halt is
// assembly in the SDK) and the end of OS_ResetSystem in ITCM (OSi_DoResetSystem, assembly with no C). OS_Halt, the
// rest of os_terminate_proc.c, is in core.c with the CPU's state.
//
// On the DS, OS_Terminate halts the CPU for good after the game's fatal error path, and OSi_DoResetSystem has the ARM7
// restart the console. The host ends the program instead: a terminate with PORT_OS_TERMINATE_STATUS and a backtrace,
// so that tests and players see where the game gave up; a reset with PORT_OS_RESET_STATUS, since the port doesn't
// restart the game in place yet, and a run that the game reset must not pass for one that ended normally.
#include "hal/host/host.h"
#include "hal/os/os_internal.h"
#include "hal/os/port_os.h"
#include "nitro/os.h"
#include "twl/fatfs.h"
#include <stdio.h>
#include <stdlib.h>

typedef void (*OSTerminateCallback)(void *arg);

// The function that OS_Terminate calls, once, with its argument, and whether it was entered already
static struct {
    void *arg;
    OSTerminateCallback callback;
    BOOL isTerminating;
} OSi_terminate;

void port_osi_reset_terminate(void) {
    OSi_terminate.arg = NULL;
    OSi_terminate.callback = NULL;
    OSi_terminate.isTerminating = FALSE;
}

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
            if (FATFSi_IsInitialized()) {
                FATFS_UnmountAll();
            }
        }
        OSi_SendToPxi(OS_PXI_COMMAND_TERMINATE);
    }
    OSi_TerminateCore();
}

// The loop that OS_Terminate ends in, where the DS disables the interrupts and halts the CPU for good. The host ends
// the program
void OSi_TerminateCore(void) {
    OS_DisableInterrupts();
    fprintf(stderr, "\nport: the game stopped itself (OS_Terminate) at frame %u\n", port_os_frame());
    port_host_print_backtrace();
    fflush(stderr);
    _Exit(PORT_OS_TERMINATE_STATUS);
}

// NitroSDK's OSi_DoResetSystem, in ITCM: where OS_ResetSystem ends, once the ARM7 has been told to reset the console
// and the reset parameter is in the shared memory
void OSi_DoResetSystem(void) {
    OS_DisableInterrupts();
    fprintf(stderr,
            "\nport: the game reset the console (OS_ResetSystem, parameter %#x) at frame %u; the port doesn't "
            "restart the game in place yet\n",
            *(u32 *)HW_RESET_PARAMETER_BUF, port_os_frame());
    fflush(stderr);
    _Exit(PORT_OS_RESET_STATUS);
}
