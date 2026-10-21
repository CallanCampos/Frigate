// TwlSDK's NWM, the DSi's wireless manager, ARM9 side: the system area, the events that come back from the ARM7, the
// state that it reports and the sleep callback. ARM code in the DSi-only LTD autoload; the file's name is ours

#include "types.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "nwm_private.h"
#include "twl/nwm.h"

static void func_02702a78(void);
static void func_02702a8c(void *arg);

// The system area, the callback that exits the application when the console sleeps, and the one that unloads the
// device before it exits (NWM_LoadDevice puts it). Built with -ipa file, the functions reach all three from the
// section's base, and the two callbacks are laid out in the reverse of these definitions
NWMSys *NWMi_SystemArea = NULL;
static PMSleepCallbackInfo sNwmSleepCallback;
PMExitCallbackInfo NWMi_UnloadOnExit;

// The library's system area. The commands' bare calls of it, whose result they drop, are left from the SDK's fetches of
// the work for assertions that its release build compiles out
NWMSys *func_027026dc(void) {
    return NWMi_SystemArea;
}

// Clears the flag by which the ARM7 says it has something for the ARM9
void func_027026ec(void) {
    NWMInfo *info;

    if (NWMi_SystemArea == NULL) {
        return;
    }
    info = NWMi_SystemArea->info;
    DC_InvalidateRange(info, 1);
    if (info->flags & NWM_INFO_PENDING) {
        info->flags &= ~NWM_INFO_PENDING;
        DC_StoreRange(info, 1);
    }
}

// The PXI callback: takes the event that the ARM7 points to, runs the callbacks and gives the buffer back
void func_02702740(PXIFifoTag tag, u32 data, BOOL err) {
    NWMSys *sys = NWMi_SystemArea;
    NWMEvent *event = (NWMEvent *)data;
    NWMCallback callback;

    if (sys == NULL) {
        return;
    }
    if (err) {
        return;
    }
    if (event == NULL) {
        return;
    }
    // BUG: the event is read before its cache lines are invalidated, so the check can see the event that was there
#ifdef BUGFIX
    DC_InvalidateRange(event, sizeof(NWMEvent));
#endif
    if (event->cmd >= NWM_CMD_COUNT) {
        return;
    }
    DC_InvalidateRange(sys->event, sizeof(NWMEvent));
    DC_InvalidateRange(sys->status, sizeof(NWMStatus));
    if (event != sys->event) {
        DC_InvalidateRange(event, sizeof(NWMEvent));
    }
    if ((event->cmd == NWM_CMD_LOAD_DEVICE && event->result != 0) ||
        (event->cmd == NWM_CMD_UNLOAD_DEVICE && event->result == 0) || event->cmd == NWM_CMD_19) {
        func_02702a78();
    }
    if (event->cmd == NWM_CMD_SEND_FRAME && event->result != NWM_RESULT_12 && event->u04.sendCallback != NULL) {
        event->u04.sendCallback(event);
    }
    if (event->cmd != NWM_CMD_SEND_FRAME) {
        if (event->cmd == NWM_CMD_UNLOAD_DEVICE || event->cmd == NWM_CMD_19 ||
            (event->cmd == NWM_CMD_LOAD_DEVICE && event->result != 0)) {
            PM_DeletePreExitCallback(&NWMi_UnloadOnExit);
        }
        callback = sys->callbacks[event->cmd];
        if (event->cmd == NWM_CMD_SCAN) {
            DC_InvalidateRange(event->data, event->u0c.size);
        }
        if (callback != NULL) {
            callback(event);
        }
    }
    MI_CpuFill8(event, 0, sizeof(NWMEvent));
    DC_StoreRange(event, sizeof(NWMEvent));
    if (sys != NULL) {
        func_027026ec();
    }
}

u16 func_027028b0(void) {
    NWMSys *sys = func_027026dc();
    NWMStatus *status;
    u16 state = 0;

    if (sys != NULL) {
        status = sys->status;
        // BUG: the state is 0x24 bytes into the block, in another cache line than the one that is invalidated
#ifdef BUGFIX
        DC_InvalidateRange(&status->state, 2);
#else
        DC_InvalidateRange(status, 2);
#endif
        state = status->state;
    }
    return state;
}

// Whether the state is one of the count states that follow
NWMRetCode func_027028e4(u32 count, ...) {
    NWMRetCode result;
    u16 state;
    va_list args;
    NWMSys *sys = func_027026dc();

    result = func_02702698();
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    DC_InvalidateRange(&sys->status->state, 2);
    state = sys->status->state;
    va_start(args, count);
    result = NWM_RETCODE_ILLEGAL_STATE;
    for (; count != 0; count--) {
        if (va_arg(args, u32) == state) {
            result = NWM_RETCODE_SUCCESS;
        }
    }
    return result;
}

u16 func_02702980(void) {
    return func_027026dc()->status->unk_9e;
}

u16 func_02702998(int type) {
    u8 mac[6];
    u16 sum;
    int i;
    int value;

    OS_GetMacAddress(mac);
    for (i = 0, sum = 0; i < 6; i++) {
        sum += mac[i];
    }
    sum += OS_GetVBlankCount();
    value = sum * 13;
    if (type == 1) {
        return value % 10 + 30;
    }
    return value % 10 + 105;
}

// Puts the callback that exits the application before the console sleeps (the wireless would be on)
void func_02702a40(void) {
    PM_SetSleepCallbackInfo(&sNwmSleepCallback, func_02702a8c, NULL);
    PM_InsertPreSleepCallback(&sNwmSleepCallback, NWM_PM_PRIORITY);
}

static void func_02702a78(void) {
    PM_DeletePreSleepCallback(&sNwmSleepCallback);
}

static void func_02702a8c(void *arg) {
    OS_Terminate();
}
