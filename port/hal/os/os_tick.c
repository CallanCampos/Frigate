// NitroSDK's tick counter (os_tick.c) on the virtual clock: the decomp's lib/nitro/src/os/os_tick.c, which
// port/hal/os/replaced.txt keeps out of the game's sources, with timer 0 modelled by core.c instead of read from its
// registers. Timer 0 counts the system clock divided by 64, and every time it wraps its interrupt adds one to
// OSi_TickCounter: the tick is that counter followed by the timer's 16 bits, as on the DS.
//
// Each OS_GetTick and OS_GetTickLo call is also a step of a modelled busy loop: it moves the clock (by one tick by
// default, port_os_set_tick_step), so that a loop that waits on the tick ends, and the events and interrupts that
// come meanwhile are taken there as the DS would take them.
#include "hal/os/os_internal.h"
#include "nitro/os.h"

// The prescaler that the SDK sets for the system timer: one count every 64 cycles
#define TICK_PRESCALER 64

// The handler of timer 0's interrupt, which OS_SetIrqFunction registers (os_interrupt.c keeps it with its type)
static void OSi_CountUpTick(void); // swan's clock_irq

static u16 OSi_UseTick; // swan's g_ClockIsInitialized
static BOOL OSi_NeedResetTimer;
// NitroSDK's OSi_TickCounter: volatile, since the timer's interrupt changes it. os_entropy.c reads it
volatile u64 OSi_TickCounter;

void port_osi_reset_tick(void) {
    OSi_UseTick = FALSE;
    OSi_NeedResetTimer = FALSE;
    OSi_TickCounter = 0;
}

// NitroSDK's OS_InitTick: starts timer 0 from 0, interrupting at each wrap
void OS_InitTick(void) {
    if (OSi_UseTick) {
        return;
    }
    OSi_UseTick = TRUE;
    OSi_SetTimerReserved(0);
    OSi_TickCounter = 0;
    port_osi_stop_timer(0);
    port_osi_start_timer(0, 0, TICK_PRESCALER, TRUE);
    OS_SetIrqFunction(OS_IE_TIMER0, OSi_CountUpTick);
    OS_EnableIrqMask(OS_IE_TIMER0);
    OSi_NeedResetTimer = FALSE;
}

// NitroSDK's OS_IsTickAvailable
BOOL OS_IsTickAvailable(void) {
    return OSi_UseTick;
}

// NitroSDK's OSi_CountUpTick, the handler of timer 0's interrupt. The SDK sets itself again with
// OSi_EnterTimerCallback, casting itself to a callback (void of a pointer); here OS_SetIrqFunction keeps its type, and
// OS_EnableIrqMask does the rest of what OSi_EnterTimerCallback does (in the interrupt's handler, where nothing else
// can be taken, it only sets timer 0's bit of IE, which is already set)
static void OSi_CountUpTick(void) {
    OSi_TickCounter++;
    if (OSi_NeedResetTimer) {
        port_osi_stop_timer(0);
        port_osi_start_timer(0, 0, TICK_PRESCALER, TRUE);
        OSi_NeedResetTimer = FALSE;
    }
    OS_SetIrqFunction(OS_IE_TIMER0, OSi_CountUpTick);
    OS_EnableIrqMask(OS_IE_TIMER0);
}

// NitroSDK's OS_GetTick. The wrap's interrupt may be pending with the timer already past 0, in which case the counter
// is one behind
u64 OS_GetTick(void) {
    u32 enabled;
    u16 countL;
    u64 countH;

    port_osi_tick_step();
    enabled = OS_DisableInterrupts();
    countL = port_osi_read_timer(0);
    countH = OSi_TickCounter & 0x0000ffffffffffffULL;
    if ((reg_OS_IF & OS_IE_TIMER0) && !(countL & 0x8000)) {
        countH++;
    }
    OS_RestoreInterrupts(enabled);
    return (countH << 16) | countL;
}

u16 OS_GetTickLo(void) {
    port_osi_tick_step();
    return port_osi_read_timer(0);
}
