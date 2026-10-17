// NitroSDK's alarms (os_alarm.c) on the virtual clock: the decomp's lib/nitro/src/os/os_alarm.c, with timer 1
// modelled by core.c instead of programmed through its registers, OSi_AlarmHandler, which is assembly in the SDK only
// to keep the stack aligned around its call, as plain C, and OS_SetIrqCheckFlag's word reached through the host's DTCM
// (os.h's HW_DTCM truncates its address; see os_irqTable.c). The alarms wait in a queue sorted by the tick they fire
// at, and timer 1 is set to interrupt for the first of them; its handler calls that alarm's handler and sets the timer
// for the next.
#include "hal/os/os_internal.h"
#include "nitro/os.h"

// The prescaler that the SDK sets for the alarms' timer, as for the tick's: one count every 64 cycles
#define ALARM_PRESCALER 64

typedef struct OSiAlarmQueue {
    OSAlarm *head;
    OSAlarm *tail;
} OSiAlarmQueue;

static void OSi_SetTimer(OSAlarm *alarm);
static void OSi_InsertAlarm(OSAlarm *alarm, OSTick fire);
static void OSi_AlarmHandler(void *arg);
static void OSi_ArrangeTimer(void);

static u16 OSi_UseAlarm;
static OSiAlarmQueue OSi_AlarmQueue;

void port_osi_reset_alarms(void) {
    OSi_UseAlarm = FALSE;
    OSi_AlarmQueue.head = NULL;
    OSi_AlarmQueue.tail = NULL;
}

// NitroSDK's OSi_SetTimer: sets timer 1 to interrupt when the alarm fires, by starting it at the complement of the
// ticks left so that it wraps then. An alarm more than a wrap of the timer away gets the whole wrap, and one that is
// late already fires two ticks from now
static void OSi_SetTimer(OSAlarm *alarm) {
    OSTick tick = OS_GetTick();
    s64 delta;
    u16 count;

    port_osi_stop_timer(1);
    delta = alarm->fire - tick;
    OSi_EnterTimerCallback(1, OSi_AlarmHandler, NULL);
    if (delta < 0) {
        count = 0xfffe;
    } else if (delta < 0x10000) {
        count = (u16)~delta;
    } else {
        count = 0;
    }
    port_osi_start_timer(1, count, ALARM_PRESCALER, TRUE);
    OS_EnableIrqMask(OS_IE_TIMER1);
}

void OS_InitAlarm(void) {
    if (!OSi_UseAlarm) {
        OSi_UseAlarm = TRUE;
        OSi_SetTimerReserved(1);
        OSi_AlarmQueue.head = NULL;
        OSi_AlarmQueue.tail = NULL;
        OS_DisableIrqMask(OS_IE_TIMER1);
    }
}

BOOL OS_IsAlarmAvailable(void) {
    return OSi_UseAlarm;
}

void OS_CreateAlarm(OSAlarm *alarm) {
    alarm->handler = NULL;
    alarm->tag = 0;
}

// NitroSDK's OSi_InsertAlarm: puts the alarm in the queue, before the first that fires later, and sets the timer if it
// is the first. A periodic alarm fires at its next period from its start, not at the tick given
static void OSi_InsertAlarm(OSAlarm *alarm, OSTick fire) {
    OSAlarm *next;
    OSAlarm *prev;

    if (alarm->period != 0) {
        OSTick tick = OS_GetTick();

        fire = alarm->start;
        if (fire < tick) {
            fire += alarm->period * (1 + (tick - fire) / alarm->period);
        }
    }
    alarm->fire = fire;
    for (next = OSi_AlarmQueue.head; next != NULL; next = next->next) {
        if ((s64)(fire - next->fire) < 0) {
            alarm->prev = next->prev;
            next->prev = alarm;
            alarm->next = next;
            if (alarm->prev != NULL) {
                alarm->prev->next = alarm;
            } else {
                OSi_AlarmQueue.head = alarm;
                OSi_SetTimer(alarm);
            }
            return;
        }
    }
    alarm->next = NULL;
    prev = OSi_AlarmQueue.tail;
    OSi_AlarmQueue.tail = alarm;
    alarm->prev = prev;
    if (prev != NULL) {
        prev->next = alarm;
    } else {
        OSi_AlarmQueue.head = OSi_AlarmQueue.tail = alarm;
        OSi_SetTimer(alarm);
    }
}

// Fires the alarm once, in some ticks from now. An alarm that is set already is a fatal error
void OS_SetAlarm(OSAlarm *alarm, OSTick tick, OSAlarmHandler handler, void *arg) {
    u32 enabled;

    if (alarm == NULL || alarm->handler != NULL) {
        OS_Terminate();
    }
    enabled = OS_DisableInterrupts();
    alarm->period = 0;
    alarm->handler = handler;
    alarm->arg = arg;
    OSi_InsertAlarm(alarm, tick + OS_GetTick());
    OS_RestoreInterrupts(enabled);
}

void OS_CancelAlarm(OSAlarm *alarm) {
    u32 enabled = OS_DisableInterrupts();
    OSAlarm *next;

    if (alarm->handler == NULL) {
        OS_RestoreInterrupts(enabled);
        return;
    }
    next = alarm->next;
    if (next == NULL) {
        OSi_AlarmQueue.tail = alarm->prev;
    } else {
        next->prev = alarm->prev;
    }
    if (alarm->prev != NULL) {
        alarm->prev->next = next;
    } else {
        OSi_AlarmQueue.head = next;
        if (next != NULL) {
            OSi_SetTimer(next);
        }
    }
    alarm->handler = NULL;
    alarm->period = 0;
    OS_RestoreInterrupts(enabled);
}

// NitroSDK's OSi_AlarmHandler, the callback of timer 1's interrupt
static void OSi_AlarmHandler(void *arg) {
    (void)arg;
    OSi_ArrangeTimer();
}

// NitroSDK's OSi_ArrangeTimer: runs the first alarm of the queue if it is due (an alarm that is not due yet, if the
// timer interrupted at a wrap, sets the timer again), then sets the timer for the next. A periodic alarm is put back in
// the queue
static void OSi_ArrangeTimer(void) {
    OSAlarm *alarm;
    OSAlarm *next;
    OSAlarmHandler handler;
    OSTick tick;

    port_osi_stop_timer(1);
    OS_DisableIrqMask(OS_IE_TIMER1);
    *port_osi_irq_check_word() |= OS_IE_TIMER1;
    tick = OS_GetTick();
    alarm = OSi_AlarmQueue.head;
    if (alarm == NULL) {
        return;
    }
    if (tick < alarm->fire) {
        OSi_SetTimer(alarm);
        return;
    }
    next = alarm->next;
    OSi_AlarmQueue.head = next;
    if (next == NULL) {
        OSi_AlarmQueue.tail = NULL;
    } else {
        next->prev = NULL;
    }
    handler = alarm->handler;
    if (alarm->period == 0) {
        alarm->handler = NULL;
    }
    if (handler != NULL) {
        handler(alarm->arg);
    }
    if (alarm->period != 0) {
        alarm->handler = handler;
        OSi_InsertAlarm(alarm, 0);
    }
    if (OSi_AlarmQueue.head != NULL) {
        OSi_SetTimer(OSi_AlarmQueue.head);
    }
}
