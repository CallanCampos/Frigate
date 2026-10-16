// NitroSDK OS: V-count alarms (os_valarm.c). They fire when the LCD's scan reaches a line, a number of frames from
// now or on the line of the current frame, in a queue sorted by frame and line; the V-count interrupt is set for the
// first.
#include "nitro/gx.h"
#include "nitro/os.h"

// The lines of a frame
#define HW_LCD_LINES 263

typedef struct OSiVAlarmQueue {
    OSVAlarm *head;
    OSVAlarm *tail;
} OSiVAlarmQueue;

static void OSi_InsertVAlarm(OSVAlarm *alarm);
static void OSi_AppendVAlarm(OSVAlarm *alarm);
static void OSi_DetachVAlarm(OSVAlarm *alarm);
static void func_0207bf64(OSVAlarm *alarm);
static void OSi_VAlarmHandler(void);
static int OSi_CompareVCount(OSVAlarm *alarm, u32 frame, int vcount);
static u32 OSi_GetVFrame(int vcount);

static u16 OSi_UseVAlarm;
static u32 OSi_VFrameCount;
static s32 OSi_PrevVCount;
static OSiVAlarmQueue OSi_VAlarmQueue;

// NitroSDK's OS_InitVAlarm
void OS_InitVAlarm(void) {
    if (!OSi_UseVAlarm) {
        OSi_UseVAlarm = TRUE;
        OSi_VAlarmQueue.head = NULL;
        OSi_VAlarmQueue.tail = NULL;
        CPU_DisableInterrupts(OS_IE_V_COUNT);
        OSi_VFrameCount = 0;
        OSi_PrevVCount = 0;
    }
}

// NitroSDK's OSi_InsertVAlarm: puts the alarm in the queue before the first that fires later, or at the end
static void OSi_InsertVAlarm(OSVAlarm *alarm) {
    OSVAlarm *next;
    OSVAlarm *prev;

    for (next = OSi_VAlarmQueue.head; next != NULL; next = next->next) {
        if (next->frame < alarm->frame) {
            continue;
        }
        if (next->frame == alarm->frame && next->vcount <= alarm->vcount) {
            continue;
        }
        prev = next->prev;
        alarm->prev = prev;
        alarm->next = next;
        next->prev = alarm;
        if (prev != NULL) {
            prev->next = alarm;
        } else {
            OSi_VAlarmQueue.head = alarm;
            func_0207bf64(alarm);
        }
        return;
    }
    OSi_AppendVAlarm(alarm);
}

// NitroSDK's OSi_AppendVAlarm: puts the alarm at the end of the queue, and waits for it if the queue was empty
static void OSi_AppendVAlarm(OSVAlarm *alarm) {
    OSVAlarm *prev = OSi_VAlarmQueue.tail;

    alarm->prev = prev;
    alarm->next = NULL;
    OSi_VAlarmQueue.tail = alarm;
    if (prev != NULL) {
        prev->next = alarm;
    } else {
        OSi_VAlarmQueue.head = alarm;
        func_0207bf64(alarm);
    }
}

// NitroSDK's OSi_DetachVAlarm: takes the alarm out of the queue
static void OSi_DetachVAlarm(OSVAlarm *alarm) {
    OSVAlarm *prev;
    OSVAlarm *next;

    if (alarm == NULL) {
        return;
    }
    next = alarm->next;
    prev = alarm->prev;
    if (next != NULL) {
        next->prev = prev;
    } else {
        OSi_VAlarmQueue.tail = prev;
    }
    if (prev != NULL) {
        prev->next = next;
    } else {
        OSi_VAlarmQueue.head = next;
    }
}

// NitroSDK's OS_CreateVAlarm
void func_0207befc(OSVAlarm *alarm) {
    alarm->handler = NULL;
    alarm->tag = 0;
    alarm->unk_20 = 0;
}

// NitroSDK's OS_SetVAlarm: calls the handler with the argument when the scan reaches the line (vcount), or at most
// delay lines after it. If the scan has reached that line already in this frame, it waits for the next
void OS_SetVAlarm(OSVAlarm *alarm, s16 vcount, s16 delay, OSVAlarmHandler handler, void *arg) {
    u32 enabled = CPU_IRQDisable();
    int current;
    u32 frame;

    if (alarm == NULL || alarm->handler != NULL) {
        sys_exit();
    }
    current = GX_GetVCount();
    frame = OSi_GetVFrame(current);
    alarm->periodic = FALSE;
    alarm->vcount = vcount;
    if (vcount <= current) {
        frame++;
    }
    alarm->frame = frame;
    alarm->delay = delay;
    alarm->arg = arg;
    alarm->canceled = FALSE;
    alarm->handler = handler;
    OSi_InsertVAlarm(alarm);
    CPU_SetIRQMask(enabled);
}

// NitroSDK's OSi_SetNextVAlarm: makes the first alarm of the queue the one that the V-count interrupt waits for
static void func_0207bf64(OSVAlarm *alarm) {
    CPU_SetIRQFunc(OS_IE_V_COUNT, OSi_VAlarmHandler);
    gfxSetVCountIntrLine(alarm->vcount);
    reg_GX_DISPSTAT |= REG_GX_DISPSTAT_VCI_MASK;
    CPU_EnableInterrupts(OS_IE_V_COUNT);
}

// NitroSDK's OSi_VAlarmHandler, the V-count interrupt's: runs the alarms that are due in turn, and puts back the
// periodic ones. An alarm that is too late for its delay is not run, but put back for the next frame
static void OSi_VAlarmHandler(void) {
    OSVAlarm *alarm;
    OSVAlarmHandler handler;
    u32 frame;
    int vcount;
    u16 dispstat;

    CPU_DisableInterrupts(OS_IE_V_COUNT);
    reg_GX_DISPSTAT &= ~REG_GX_DISPSTAT_VCI_MASK;
    OS_SetIrqCheckFlag(OS_IE_V_COUNT);
    // The line that the interrupt waited for: its low 8 bits are DISPSTAT's bits 8 to 15 and its ninth is bit 7
    dispstat = reg_GX_DISPSTAT;
    OSi_GetVFrame((((dispstat >> 8) & 0xff) | ((dispstat << 1) & 0x100)) - 1);
    for (alarm = OSi_VAlarmQueue.head; alarm != NULL; alarm = OSi_VAlarmQueue.head) {
        vcount = GX_GetVCount();
        frame = OSi_GetVFrame(vcount);
        switch (OSi_CompareVCount(alarm, frame, vcount)) {
        case 0:
            // Not yet: wait for its line, unless the scan has reached it by now
            func_0207bf64(alarm);
            if (alarm->vcount != GX_GetVCount() || alarm->frame != frame) {
                return;
            }
            CPU_DisableInterrupts(OS_IE_V_COUNT);
            reg_GX_DISPSTAT &= ~REG_GX_DISPSTAT_VCI_MASK;
            CPU_AcknowledgeInterrupts(OS_IE_V_COUNT);
            // fall through
        case 1:
            handler = alarm->handler;
            OSi_DetachVAlarm(alarm);
            alarm->handler = NULL;
            if (handler != NULL) {
                handler(alarm->arg);
            }
            if (alarm->periodic && !alarm->canceled) {
                alarm->handler = handler;
                alarm->frame = OSi_VFrameCount + 1;
                OSi_InsertVAlarm(alarm);
            }
            break;
        case 2:
            OSi_DetachVAlarm(alarm);
            alarm->frame = OSi_VFrameCount + 1;
            OSi_InsertVAlarm(alarm);
            break;
        }
    }
}

// NitroSDK's OSi_CompareVCount: whether the alarm is due: 0 before its line, 1 on it or within its delay after it, 2
// later
static int OSi_CompareVCount(OSVAlarm *alarm, u32 frame, int vcount) {
    s32 frames = frame - alarm->frame;
    s32 lines = vcount - alarm->vcount;

    if (frames < 0 || (frames == 0 && lines < 0)) {
        return 0;
    }
    if (lines < 0) {
        lines += HW_LCD_LINES;
    }
    if (lines <= alarm->delay) {
        return 1;
    }
    return 2;
}

// NitroSDK's OSi_GetVFrame: counts the frames, by a V-count lower than the last one; gives the count
static u32 OSi_GetVFrame(int vcount) {
    u32 enabled = CPU_IRQDisable();

    if (vcount < OSi_PrevVCount) {
        OSi_VFrameCount++;
    }
    OSi_PrevVCount = vcount;
    CPU_SetIRQMask(enabled);
    return OSi_VFrameCount;
}
