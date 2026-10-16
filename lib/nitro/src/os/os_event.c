// TwlSDK OS: events (os_event.c). An event is a word of flags and a queue of the threads that wait for some of them
// to be set; the card's code and the DSi's photo signing use them.
#include "nitro/os.h"

// TwlSDK's OS_InitEvent
void func_0207c7f4(OSEvent *event) {
    OS_InitThreadQueue(&event->queue);
    event->flag = 0;
}

// TwlSDK's OS_WaitEventEx: sleeps until all (OS_EVENT_MODE_AND) or any (OS_EVENT_MODE_OR) of the flags of pattern are
// set, then clears the flags of clearBits and gives the flags that were set, or 0 for a mode that is neither
u32 func_0207c800(OSEvent *event, u32 pattern, u32 mode, u32 clearBits) {
    u32 flags = 0;
    u32 enabled = CPU_IRQDisable();

    switch (mode) {
    case OS_EVENT_MODE_AND:
        while ((event->flag & pattern) != pattern) {
            scheduler_yield(&event->queue);
        }
        flags = event->flag;
        break;
    case OS_EVENT_MODE_OR:
        while (!(event->flag & pattern)) {
            scheduler_yield(&event->queue);
        }
        flags = event->flag;
        break;
    }
    if (flags != 0) {
        event->flag &= ~clearBits;
    }
    CPU_SetIRQMask(enabled);
    return flags;
}

// TwlSDK's OS_SignalEvent: sets the flags and wakes the threads that wait on the event
void func_0207c864(OSEvent *event, u32 setPattern) {
    u32 enabled = CPU_IRQDisable();

    if (setPattern != 0) {
        event->flag |= setPattern;
        OS_WakeupThread(&event->queue);
    }
    CPU_SetIRQMask(enabled);
}
