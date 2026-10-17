// NitroSDK's V-count alarms on the host: the decomp's os_valarm.c, compiled through this file
// (port/hal/os/replaced.txt keeps it out of the game's own sources) so that port_os_run can set its statics back for
// each run. OS_InitVAlarm does nothing once they say the alarms are in use, so a second run in the same process would
// otherwise keep the first run's queue and frame count.
//
// One change, as in os_irqTable.c: the V-count handler sets OS_SetIrqCheckFlag's word through the HAL, since the SDK's
// inline reaches it through HW_DTCM, which os.h defines as (u32)SDK_AUTOLOAD_DTCM_START and so truncates on a 64-bit
// host. os.h comes first, so that the rename reaches only os_valarm.c's call. Once the host variant of os.h keeps
// HW_DTCM whole, the rename can go.
#include "hal/os/os_internal.h"
#include "hal/os/port_os.h"
#include "nitro/os.h"

#define OS_SetIrqCheckFlag port_os_set_irq_check_flag
#include "lib/nitro/src/os/os_valarm.c"
#undef OS_SetIrqCheckFlag

void port_osi_reset_valarms(void) {
    OSi_UseVAlarm = FALSE;
    OSi_VFrameCount = 0;
    OSi_PrevVCount = 0;
    OSi_VAlarmQueue.head = NULL;
    OSi_VAlarmQueue.tail = NULL;
}
