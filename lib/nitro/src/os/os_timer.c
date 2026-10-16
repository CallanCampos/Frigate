// NitroSDK OS: the record of the hardware timers that the OS keeps for itself (os_timer.c). OS_InitTick and
// OS_InitAlarm each take one.
#include "nitro/os.h"

// A bit for each timer taken
static u16 OSi_TimerReserved;

// NitroSDK's OSi_SetTimerReserved: takes a timer for the OS
void OSi_SetTimerReserved(int timerNo) {
    OSi_TimerReserved |= (u16)(1 << timerNo);
}
