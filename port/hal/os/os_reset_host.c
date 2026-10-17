// NitroSDK's reset functions on the host: the decomp's os_reset.c, compiled unchanged through this file
// (port/hal/os/replaced.txt keeps it out of the game's own sources) so that port_os_run can set its static back for
// each run: OS_InitReset does nothing once it has run. Its end in ITCM, OSi_DoResetSystem, is in os_terminate.c.
#include "hal/os/os_internal.h"
#include "lib/nitro/src/os/os_reset.c"

void port_osi_reset_init_reset(void) {
    OSi_IsInitReset = FALSE;
}
