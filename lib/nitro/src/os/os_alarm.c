// NitroSDK OS: alarms (os_alarm.c). The alarms wait in a queue sorted by the tick they fire at, and timer 1 is set to
// interrupt for the first of them; its handler calls that alarm's handler and sets the timer for the next.
#include "nitro/os.h"

typedef struct OSiAlarmQueue {
    OSAlarm *head;
    OSAlarm *tail;
} OSiAlarmQueue;

static void OSi_SetTimer(OSAlarm *alarm);
static void OSi_InsertAlarm(OSAlarm *alarm, OSTick fire);
static asm void OSi_AlarmHandler(void *arg);
static void OSi_ArrangeTimer(void);

static u16 OSi_UseAlarm;
static OSiAlarmQueue OSi_AlarmQueue;

// NitroSDK's OSi_SetTimer: sets timer 1 to interrupt when the alarm fires, by starting it at the complement of the
// ticks left so that it wraps then. An alarm more than a wrap of the timer away gets the whole wrap, and one that is
// late already fires two ticks from now
static void OSi_SetTimer(OSAlarm *alarm) {
    OSTick tick = OS_GetTick();
    s64 delta;
    u16 count;

    reg_OS_TM1CNT_H = 0;
    delta = alarm->fire - tick;
    OSi_EnterTimerCallback(1, OSi_AlarmHandler, NULL);
    if (delta < 0) {
        count = 0xfffe;
    } else if (delta < 0x10000) {
        count = (u16)~delta;
    } else {
        count = 0;
    }
    reg_OS_TM1CNT_L = count;
    reg_OS_TM1CNT_H = REG_OS_TMCNT_H_E_MASK | REG_OS_TMCNT_H_I_MASK | OS_TIMER_PRESCALER_64;
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
    OSIntrMode enabled;

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
    OSIntrMode enabled = OS_DisableInterrupts();
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

#pragma thumb off
// clang-format off
// NitroSDK's OSi_AlarmHandler, the handler of timer 1's interrupt: ARM code that keeps the stack 8-byte aligned around
// the call of OSi_ArrangeTimer, which does the work. Assembly in the SDK, since a C function would not push r0 for that
static asm void OSi_AlarmHandler(void *arg) {
    stmdb sp!, {r0, lr}
    bl OSi_ArrangeTimer
    ldmia sp!, {r0, lr}
    bx lr
}
// clang-format on
#pragma thumb on

// NitroSDK's OSi_ArrangeTimer: runs the first alarm of the queue if it is due (an alarm that is not due yet, if the
// timer interrupted at a wrap, sets the timer again), then sets the timer for the next. A periodic alarm is put back in
// the queue
static void OSi_ArrangeTimer(void) {
    OSAlarm *alarm;
    OSAlarm *next;
    OSAlarmHandler handler;
    OSTick tick;

    reg_OS_TM1CNT_H = 0;
    OS_DisableIrqMask(OS_IE_TIMER1);
    OS_SetIrqCheckFlag(OS_IE_TIMER1);
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
