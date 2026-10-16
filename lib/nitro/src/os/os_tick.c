// NitroSDK OS: the tick counter (os_tick.c). Timer 0 counts the system clock divided by 64 and every time it wraps,
// its interrupt adds one to a counter in memory: the tick is that counter followed by the timer's 16 bits.
#include "nitro/os.h"

static void clock_irq(void);

static u16 OSi_UseTick;
static BOOL OSi_NeedResetTimer;
// NitroSDK's OSi_TickCounter: volatile, since the timer's interrupt changes it. os_entropy.c reads it
volatile u64 g_SystemClockBaseCounter;

// NitroSDK's OS_InitTick, under swan's name
void clock_init(void) {
    if (OSi_UseTick) {
        return;
    }
    OSi_UseTick = TRUE;
    func_0207ba64(0);
    g_SystemClockBaseCounter = 0;
    reg_OS_TM0CNT_H = 0;
    reg_OS_TM0CNT_L = 0;
    reg_OS_TM0CNT_H = REG_OS_TMCNT_H_E_MASK | REG_OS_TMCNT_H_I_MASK | OS_TIMER_PRESCALER_64;
    CPU_SetIRQFunc(OS_IE_TIMER0, clock_irq);
    CPU_EnableInterrupts(OS_IE_TIMER0);
    OSi_NeedResetTimer = FALSE;
}

// NitroSDK's OS_IsTickAvailable, under swan's name
BOOL clock_is_inited(void) {
    return OSi_UseTick;
}

// NitroSDK's OSi_CountUpTick, the handler of timer 0's interrupt
static void clock_irq(void) {
    g_SystemClockBaseCounter++;
    if (OSi_NeedResetTimer) {
        reg_OS_TM0CNT_H = 0;
        reg_OS_TM0CNT_L = 0;
        reg_OS_TM0CNT_H = REG_OS_TMCNT_H_E_MASK | REG_OS_TMCNT_H_I_MASK | OS_TIMER_PRESCALER_64;
        OSi_NeedResetTimer = FALSE;
    }
    OSi_EnterTimerCallback(0, (OSIrqCallback)clock_irq, NULL);
}

// NitroSDK's OS_GetTick, under swan's name. The wrap's interrupt may be pending with the timer already past 0, in
// which case the counter is one behind. The timer and the counter are read into volatile locals, as the original's
// code does, which keeps the order of the reads
u64 clock(void) {
    u32 enabled = CPU_IRQDisable();
    vu16 countL = reg_OS_TM0CNT_L;
    volatile u64 countH = g_SystemClockBaseCounter & 0x0000ffffffffffffULL;

    if ((reg_OS_IF & OS_IE_TIMER0) && !(countL & 0x8000)) {
        countH++;
    }
    CPU_SetIRQMask(enabled);
    return (countH << 16) | countL;
}

// NitroSDK's OS_GetTickLo
u16 func_0207bb84(void) {
    return reg_OS_TM0CNT_L;
}
