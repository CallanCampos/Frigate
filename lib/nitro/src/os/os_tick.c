// NitroSDK OS: the tick counter (os_tick.c). Timer 0 counts the system clock divided by 64 and every time it wraps,
// its interrupt adds one to a counter in memory: the tick is that counter followed by the timer's 16 bits.
#include "nitro/os.h"

static void OSi_CountUpTick(void); // swan's clock_irq

static u16 OSi_UseTick; // swan's g_ClockIsInitialized
static BOOL OSi_NeedResetTimer;
// NitroSDK's OSi_TickCounter: volatile, since the timer's interrupt changes it. os_entropy.c reads it
volatile u64 OSi_TickCounter;

// NitroSDK's OS_InitTick
void OS_InitTick(void) {
    if (OSi_UseTick) {
        return;
    }
    OSi_UseTick = TRUE;
    OSi_SetTimerReserved(0);
    OSi_TickCounter = 0;
    reg_OS_TM0CNT_H = 0;
    reg_OS_TM0CNT_L = 0;
    reg_OS_TM0CNT_H = REG_OS_TMCNT_H_E_MASK | REG_OS_TMCNT_H_I_MASK | OS_TIMER_PRESCALER_64;
    OS_SetIrqFunction(OS_IE_TIMER0, OSi_CountUpTick);
    OS_EnableIrqMask(OS_IE_TIMER0);
    OSi_NeedResetTimer = FALSE;
}

// NitroSDK's OS_IsTickAvailable
BOOL OS_IsTickAvailable(void) {
    return OSi_UseTick;
}

// NitroSDK's OSi_CountUpTick, the handler of timer 0's interrupt
static void OSi_CountUpTick(void) {
    OSi_TickCounter++;
    if (OSi_NeedResetTimer) {
        reg_OS_TM0CNT_H = 0;
        reg_OS_TM0CNT_L = 0;
        reg_OS_TM0CNT_H = REG_OS_TMCNT_H_E_MASK | REG_OS_TMCNT_H_I_MASK | OS_TIMER_PRESCALER_64;
        OSi_NeedResetTimer = FALSE;
    }
    OSi_EnterTimerCallback(0, (OSIrqCallback)OSi_CountUpTick, NULL);
}

// NitroSDK's OS_GetTick. The wrap's interrupt may be pending with the timer already past 0, in
// which case the counter is one behind
u64 OS_GetTick(void) {
    u32 enabled = OS_DisableInterrupts();
    // The SDK reads the timer and the counter into volatile locals, which keeps the order of the reads
    vu16 countL = reg_OS_TM0CNT_L;
    volatile u64 countH = OSi_TickCounter & 0x0000ffffffffffffULL;

    if ((reg_OS_IF & OS_IE_TIMER0) && !(countL & 0x8000)) {
        countH++;
    }
    OS_RestoreInterrupts(enabled);
    return (countH << 16) | countL;
}

u16 OS_GetTickLo(void) {
    return reg_OS_TM0CNT_L;
}
