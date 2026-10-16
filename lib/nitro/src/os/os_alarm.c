// NitroSDK OS: alarms (os_alarm.c). The alarms wait in a queue sorted by the tick they fire at, and timer 1 is set to
// interrupt for the first of them; its handler calls that alarm's handler and sets the timer for the next.
#include "nitro/os.h"

typedef struct OSiAlarmQueue {
    OSAlarm *head;
    OSAlarm *tail;
} OSiAlarmQueue;

static void func_0207bb90(OSAlarm *alarm);
static void func_0207bc2c(OSAlarm *alarm, OSTick fire);
static asm void func_0207bd90(void *arg);
static void func_0207bda0(void);

static u16 OSi_UseAlarm;
static OSiAlarmQueue OSi_AlarmQueue;

// NitroSDK's OSi_SetTimer: sets timer 1 to interrupt when the alarm fires, by starting it at the complement of the
// ticks left so that it wraps then. An alarm more than a wrap of the timer away gets the whole wrap, and one that is
// late already fires two ticks from now
static void func_0207bb90(OSAlarm *alarm) {
    OSTick tick = clock();
    s64 delta;
    u16 count;

    reg_OS_TM1CNT_H = 0;
    delta = alarm->fire - tick;
    OSi_EnterTimerCallback(1, func_0207bd90, NULL);
    if (delta < 0) {
        count = 0xfffe;
    } else if (delta < 0x10000) {
        count = (u16)~delta;
    } else {
        count = 0;
    }
    reg_OS_TM1CNT_L = count;
    reg_OS_TM1CNT_H = REG_OS_TMCNT_H_E_MASK | REG_OS_TMCNT_H_I_MASK | OS_TIMER_PRESCALER_64;
    CPU_EnableInterrupts(OS_IE_TIMER1);
}

void OS_InitAlarm(void) {
    if (!OSi_UseAlarm) {
        OSi_UseAlarm = TRUE;
        func_0207ba64(1);
        OSi_AlarmQueue.head = NULL;
        OSi_AlarmQueue.tail = NULL;
        CPU_DisableInterrupts(OS_IE_TIMER1);
    }
}

BOOL OS_IsAlarmAvailable(void) {
    return OSi_UseAlarm;
}

void OS_CreateAlarm(OSAlarm *alarm) {
    alarm->handler = NULL;
    alarm->tag = 0;
}

// NitroSDK's InsertAlarm: puts the alarm in the queue, before the first that fires later, and sets the timer if it
// is the first. A periodic alarm fires at its next period from its start, not at the tick given
static void func_0207bc2c(OSAlarm *alarm, OSTick fire) {
    OSAlarm *next;
    OSAlarm *prev;

    if (alarm->period != 0) {
        OSTick tick = clock();

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
                func_0207bb90(alarm);
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
        func_0207bb90(alarm);
    }
}

// Fires the alarm once, in some ticks from now. An alarm that is set already is a fatal error
void OS_SetAlarm(OSAlarm *alarm, OSTick tick, OSAlarmHandler handler, void *arg) {
    u32 enabled;

    if (alarm == NULL || alarm->handler != NULL) {
        sys_exit();
    }
    enabled = CPU_IRQDisable();
    alarm->period = 0;
    alarm->handler = handler;
    alarm->arg = arg;
    func_0207bc2c(alarm, tick + clock());
    CPU_SetIRQMask(enabled);
}

void OS_CancelAlarm(OSAlarm *alarm) {
    u32 enabled = CPU_IRQDisable();
    OSAlarm *next;

    if (alarm->handler == NULL) {
        CPU_SetIRQMask(enabled);
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
            func_0207bb90(next);
        }
    }
    alarm->handler = NULL;
    alarm->period = 0;
    CPU_SetIRQMask(enabled);
}

#pragma thumb off
// clang-format off
// The handler of timer 1's interrupt, in ARM code that keeps the stack 8-byte aligned (the SDK's OSi_AlarmHandler
// calls the Thumb function that does the work): assembly in the SDK, since a C function would not push r0 for that
static asm void func_0207bd90(void *arg) {
    stmdb sp!, {r0, lr}
    bl func_0207bda0
    ldmia sp!, {r0, lr}
    bx lr
}
// clang-format on
#pragma thumb on

// Runs the first alarm of the queue if it is due (an alarm that is not due yet, if the timer interrupted at a wrap,
// sets the timer again), then sets the timer for the next. A periodic alarm is put back in the queue
static void func_0207bda0(void) {
    OSAlarm *alarm;
    OSAlarm *next;
    OSAlarmHandler handler;
    OSTick tick;

    reg_OS_TM1CNT_H = 0;
    CPU_DisableInterrupts(OS_IE_TIMER1);
    OS_SetIrqCheckFlag(OS_IE_TIMER1);
    tick = clock();
    alarm = OSi_AlarmQueue.head;
    if (alarm == NULL) {
        return;
    }
    if (tick < alarm->fire) {
        func_0207bb90(alarm);
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
        func_0207bc2c(alarm, 0);
    }
    if (OSi_AlarmQueue.head != NULL) {
        func_0207bb90(OSi_AlarmQueue.head);
    }
}
