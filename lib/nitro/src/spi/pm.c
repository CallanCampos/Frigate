// NitroSDK's power management library (TwlSDK's libspi, spi/ARM9/src/pm.c): the ARM9's side of the requests to the
// ARM7's power management through the PXI FIFO, for the backlights and the screens' power, the battery, the sound
// amplifier of the microphone, the sleep mode and the power-off. The file name is the SDK's, and the split is certain:
// the functions come in the SDK's order, and the .bss and .data hold its statics. The same object has TwlSDK's DSi
// part in the LTD autoload (the exit callbacks and the battery level), which this part reaches through the linker's
// veneers (func_0207ece4/e8, func_0207f058/5c and func_0207f16c/70, after their callers) and which reaches back to
// the work of this part by the addresses of its first objects, so PMi_IsInit, PMi_WaitBusyMethod and PMi_AutoExitFlag
// are not static. The named functions have the SDK's names; the statics still named by address are not known.
#include "nitro/pm.h"
#include "nitro/card.h"
#include "nitro/ctrdg.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "nitro/svc.h"
#include "twl/mi.h"
#include "twl/pm.h"

// A request is one or two PXI words with the tag of the power management. The first has PM_PXI_START and the command
// in bits 8 to 14, the last has PM_PXI_END, and the other carries its index in bits 16 to 22 and 16 bits of data
#define PM_PXI_START 0x02000000
#define PM_PXI_END 0x01000000
#define PM_PXI_INDEX(index) ((index) << 16)
#define PM_PXI_COMMAND(command) ((command) << 8)
#define PM_PXI_COMMAND_MASK 0x7f00
#define PM_PXI_DATA_MASK 0xff
#define PM_PXI_DATA16_MASK 0xffff

// The commands that the ARM9 sends: the one before the sleep, a request to the ARM7 with its number in the low byte of
// the first word and its data in the second, and the one that starts the sleep, with the trigger in the low byte and
// the key pattern in the second word
#define PM_PXI_COMMAND_SLEEP_PREPARE 0x60
#define PM_PXI_COMMAND_REQUEST 0x61
#define PM_PXI_COMMAND_SLEEP_START 0x62
// What the ARM7 answers with: the same commands, then the end of the sleep, and the ARM7's own notifications
#define PM_PXI_COMMAND_WOKE_UP 0x63
#define PM_PXI_COMMAND_NOTIFY 0x70

// The notifications. Two start the exit sequence of the DSi, which is told why, and one is for a low battery
#define PM_PXI_NOTIFY_EXIT_1 0
#define PM_PXI_NOTIFY_BATTERY_LOW 4
#define PM_PXI_NOTIFY_EXIT_2 5
#define PM_EXIT_TYPE_1 1
#define PM_EXIT_TYPE_2 2
#define PM_EXIT_TYPE_FORCED 3

// The requests, in the low byte of the first word of a PM_PXI_COMMAND_REQUEST: three that follow the screens' power,
// six for the backlights, the power-off, and the settings and reads of the registers
#define PM_REQ_LCD_POWER_1 1
#define PM_REQ_LCD_POWER_2 2
#define PM_REQ_LCD_POWER_3 3
#define PM_REQ_BACKLIGHT_BOTTOM_ON 4
#define PM_REQ_BACKLIGHT_BOTTOM_OFF 5
#define PM_REQ_BACKLIGHT_TOP_ON 6
#define PM_REQ_BACKLIGHT_TOP_OFF 7
#define PM_REQ_BACKLIGHT_ALL_ON 8
#define PM_REQ_BACKLIGHT_ALL_OFF 9
#define PM_REQ_POWER_OFF 0xe
#define PM_REQ_READ_REGISTER 0xf
#define PM_REQ_SET_AMP 0x10
#define PM_REQ_SET_AMP_GAIN 0x11
#define PM_REQ_UNK_12 0x12
#define PM_REQ_SET_AMP_GAIN_LEVEL 0x16

// The registers that PM_REQ_READ_REGISTER reads, and the bits of the backlight one
#define PM_REG_BATTERY 0
#define PM_REG_BACKLIGHT 3
#define PM_REG_AMP 6
#define PM_REG_UNK_8 8
#define PM_BACKLIGHT_TOP_MASK 0x08
#define PM_BACKLIGHT_BOTTOM_MASK 0x04

// What PMi_SetLCDPower tells the ARM7 about the screens' power: on or off, which map to the requests, and a third
#define PM_LCD_POWER_ON 1
#define PM_LCD_POWER_OFF 2
#define PM_LCD_POWER_3 3

// What PM_GoSleepMode adds to the trigger to have the backlights back when the system wakes
#define PM_TRIGGER_BACKLIGHT_TOP 0x40
#define PM_TRIGGER_BACKLIGHT_BOTTOM 0x80

// How PMi_WaitBusy waits: it reads the FIFO itself when the interrupt that does is blocked, in the IRQ mode (bit 1),
// with the IRQ masked (bit 2) or with IME off (bit 3)
#define PM_WAIT_BUSY_IRQ_MODE 0x2
#define PM_WAIT_BUSY_IRQ_MASKED 0x4
#define PM_WAIT_BUSY_IME_OFF 0x8
#define PM_WAIT_BUSY_ALL (PM_WAIT_BUSY_IRQ_MODE | PM_WAIT_BUSY_IRQ_MASKED | PM_WAIT_BUSY_IME_OFF)
#define PM_PROC_MODE_IRQ 0x12
#define PM_CPSR_IRQ_DISABLE 0x80

// A synchronous call starts with this in its result, which no answer has
#define PM_SYNC_PENDING 0xffff0000

// The wait before the screens are touched on the way out (about 100 ms), and the one of their power
#define PM_SCREENS_SETTLE_CYCLES 0x360000
#define PM_SCREENS_RETRY_CYCLES 5

// The sleep leaves on the PXI FIFO's interrupt, and the timer of the clock when it runs. The card's pull-out interrupt
// request (OS_IE_CARD_IREQ) wakes the system. The card's control register keeps its start bit set while a transfer runs

typedef struct {
    // The request being answered, with its callback, and where the answer's data goes
    BOOL lock;
    PMCallback callback;
    void *callbackArg;
    u16 *data;
} PMiWork;

// The order of these decides the layout of the .bss, which MWCC sorts by size from the order of declaration (the two
// words of the .data count in it too)
u16 PMi_IsInit;
static PMiWork PMi_Work;
// The callbacks that run after the system wakes
static PMGenCallbackInfo *PMi_PostSleepList;
// Set when the ARM7 says that the system woke up
static BOOL PMi_WokeUp;
// The vertical blank count when the screens' power went off, and when the display did
static u32 PMi_ScreensOffCount;
static u32 PMi_DisplayOffCount;
// Why the system is exiting, for TwlSDK's exit sequence
u32 PMi_ExitType;
// The DMA control registers before the sleep
static u32 PMi_PreDmaCnt[4];
// What the amplifier was set to, for when the screens come back on
static int PMi_Amp;
// The callbacks that run before the system sleeps
static PMGenCallbackInfo *PMi_PreSleepList;
// How PMi_WaitBusy waits; the power-off waits another way
u32 PMi_WaitBusyMethod = PM_WAIT_BUSY_IRQ_MODE;
BOOL PMi_AutoExitFlag = TRUE;

static void PMi_WaitBusy(void);
static void PMi_SyncCallback(u32 result, void *arg);
static void PMi_CallCallbackAndUnlock(u32 result);
static void func_0207eb9c(void);
static void PMi_CommonCallback(PXIFifoTag tag, u32 data, BOOL err);
static u32 PMi_SendPxiData(const u32 *words, int length, u16 *result, PMCallback callback, void *arg);
static void PMi_SendPxiDataSync(const u32 *words, int length);
static u32 PMi_SendSleepStart(u16 trigger, u16 keyPattern);
static u32 PMi_SendRequestAsync(u32 request, u32 data, u16 *result, PMCallback callback, void *arg);
static u32 func_0207ee5c(u32 power, PMCallback callback, void *arg);
static u32 func_0207ee94(u32 power);
static u32 PM_ForceToPowerOffAsync(PMCallback callback, void *arg);
static u32 func_0207ef60(void);
static u32 PMi_SetAmp(int amp);
static void func_0207f0d8(u32 word);
static void PMi_StopDma(void);
static BOOL PMi_SetLCDPower(BOOL on, u32 power, BOOL force, BOOL sync);
static BOOL PM_GetLCDPower(void);
static void func_0207f64c(void);

// PMi_WaitBusy: waits for the answer of the request that is being sent, reading the ARM7's words itself where the
// interrupt that would run the callback can't. The lock is polled through its address, which the callback clears
static void PMi_WaitBusy(void) {
    BOOL *lock = &PMi_Work.lock;

    while (*lock) {
        if (((PMi_WaitBusyMethod & PM_WAIT_BUSY_IRQ_MODE) && OS_GetProcMode() == PM_PROC_MODE_IRQ) ||
            ((PMi_WaitBusyMethod & PM_WAIT_BUSY_IRQ_MASKED) && OS_GetCpsrIrq() == PM_CPSR_IRQ_DISABLE) ||
            ((PMi_WaitBusyMethod & PM_WAIT_BUSY_IME_OFF) && reg_OS_IME == 0)) {
            PXIi_HandlerRecvFifoNotEmpty();
        }
    }
}

// The callback of the synchronous calls: gives the result to the caller, which waits for it
static void PMi_SyncCallback(u32 result, void *arg) {
    if (arg != NULL) {
        *(u32 *)arg = result;
    }
}

// Ends the request, calling its callback once
static void PMi_CallCallbackAndUnlock(u32 result) {
    PMCallback callback = PMi_Work.callback;
    void *arg = PMi_Work.callbackArg;

    PMi_Work.lock = FALSE;
    if (callback != NULL) {
        PMi_Work.callback = NULL;
        callback(result, arg);
    }
}

// Waits for the next vertical blank, by polling. The count is read back from the stack at every turn of the loop
static void func_0207eb9c(void) {
    vu32 count = OS_GetVBlankCount();

    while (count == OS_GetVBlankCount()) {
    }
}

void PM_Init(void) {
    if (PMi_IsInit) {
        return;
    }
    PMi_IsInit = TRUE;
    PMi_Work.lock = FALSE;
    PMi_Work.callback = NULL;
    *(vu32 *)HW_PM_EXIT_FLAG = 0;
    PXI_Init();
    while (!PXI_IsCallbackReady(PXI_FIFO_TAG_PM, PXI_PROC_ARM7)) {
        SVC_WaitByLoop(100);
    }
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_PM, PMi_CommonCallback);
    PMi_ScreensOffCount = PMi_DisplayOffCount = OS_GetVBlankCount();
}

// The ARM7's answers, and its notifications
static void PMi_CommonCallback(PXIFifoTag tag, u32 data, BOOL err) {
    u16 command, value;
    BOOL finish = TRUE;

    command = (data & PM_PXI_COMMAND_MASK) >> 8;
    value = data & PM_PXI_DATA_MASK;

    if (err) {
        PMi_CallCallbackAndUnlock(command == PM_PXI_COMMAND_REQUEST || command == PM_PXI_COMMAND_SLEEP_START
                                      ? PM_RESULT_BUSY
                                      : PM_RESULT_ERROR);
        return;
    }
    switch (command) {
    case PM_PXI_COMMAND_REQUEST:
        if (PMi_Work.data != NULL) {
            *PMi_Work.data = value;
        }
        value = 0;
        break;
    case PM_PXI_COMMAND_SLEEP_PREPARE:
        value = 0;
        break;
    case PM_PXI_COMMAND_SLEEP_START:
        break;
    case PM_PXI_COMMAND_WOKE_UP:
        PMi_WokeUp = TRUE;
        break;
    case PM_PXI_COMMAND_NOTIFY:
        switch (value) {
        case PM_PXI_NOTIFY_EXIT_1:
            PMi_ProcessExitRequest(PM_EXIT_TYPE_1);
            *(vu32 *)HW_PM_EXIT_FLAG = 1;
            break;
        case 1:
        case 2:
        case 3:
            break;
        case PM_PXI_NOTIFY_BATTERY_LOW:
            if (PMi_BatteryLowCallbackInfo.callback != NULL) {
                PMi_BatteryLowCallbackInfo.callback(PMi_BatteryLowCallbackInfo.arg);
            }
            break;
        case PM_PXI_NOTIFY_EXIT_2:
            PMi_ProcessExitRequest(PM_EXIT_TYPE_2);
            *(vu32 *)HW_PM_EXIT_FLAG = 1;
            break;
        }
        finish = FALSE;
        break;
    }
    if (finish) {
        PMi_CallCallbackAndUnlock(value);
    }
}

// Sends the words of a request, if the ARM7 is not on another. PM_RESULT_BUSY if it is
static u32 PMi_SendPxiData(const u32 *words, int length, u16 *result, PMCallback callback, void *arg) {
    OSIntrMode irq = OS_DisableInterrupts();
    int i;

    if (PMi_Work.lock) {
        OS_RestoreInterrupts(irq);
        return PM_RESULT_BUSY;
    }
    PMi_Work.lock = TRUE;
    PMi_Work.data = result;
    PMi_Work.callback = callback;
    PMi_Work.callbackArg = arg;
    for (i = 0; i < length; i++) {
        func_0207f0d8(words[i]);
    }
    OS_RestoreInterrupts(irq);
    return PM_RESULT_SUCCESS;
}

// Sends the words, retrying until the ARM7 accepts them and answers with success
static void PMi_SendPxiDataSync(const u32 *words, int length) {
    // Set by the callback, from the interrupt
    vu32 result;

    for (;;) {
        result = PM_SYNC_PENDING;
        while (PMi_SendPxiData(words, length, NULL, PMi_SyncCallback, (void *)&result) != PM_RESULT_SUCCESS) {
            OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
        }
        while (result == PM_SYNC_PENDING) {
            OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
        }
        if (result == PM_RESULT_SUCCESS) {
            break;
        }
        OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
    }
}

// Has the ARM7 prepare, turns the screens off and sends the trigger and the key pattern
static u32 PMi_SendSleepStart(u16 trigger, u16 keyPattern) {
    u32 words[2];

    words[0] = PM_PXI_START | PM_PXI_END | PM_PXI_COMMAND(PM_PXI_COMMAND_SLEEP_PREPARE);
    PMi_SendPxiDataSync(words, 1);
    while (PMi_SetLCDPower(FALSE, PM_LCD_POWER_OFF, FALSE, TRUE) != TRUE) {
    }
    words[0] = PM_PXI_START | PM_PXI_COMMAND(PM_PXI_COMMAND_SLEEP_START) | (trigger & PM_PXI_DATA_MASK);
    words[1] = PM_PXI_END | PM_PXI_INDEX(1) | (keyPattern & PM_PXI_DATA16_MASK);
    PMi_SendPxiDataSync(words, 2);
    return PM_RESULT_SUCCESS;
}

// Sends a request with a halfword of data. The ARM7's data goes to *result
static u32 PMi_SendRequestAsync(u32 request, u32 data, u16 *result, PMCallback callback, void *arg) {
    u32 words[2];

    words[0] = PM_PXI_START | PM_PXI_COMMAND(PM_PXI_COMMAND_REQUEST) | (request & PM_PXI_DATA_MASK);
    words[1] = PM_PXI_END | PM_PXI_INDEX(1) | (data & PM_PXI_DATA16_MASK);
    return PMi_SendPxiData(words, 2, result, callback, arg);
}

// NitroSDK's PMi_ReadRegister
u32 PMi_ReadRegister(u8 request, u16 data, u16 *result) {
    u32 res;
    u32 sent = PMi_SendRequestAsync(request, data, result, PMi_SyncCallback, &res);

    if (sent == PM_RESULT_SUCCESS) {
        PMi_WaitBusy();
        return res;
    }
    return sent;
}

// Sends the request that the screens' power change has
static u32 func_0207ee5c(u32 power, PMCallback callback, void *arg) {
    u32 request;

    switch (power) {
    case PM_LCD_POWER_ON:
        request = PM_REQ_LCD_POWER_1;
        break;
    case PM_LCD_POWER_3:
        request = PM_REQ_LCD_POWER_2;
        break;
    case PM_LCD_POWER_OFF:
        request = PM_REQ_LCD_POWER_3;
        break;
    default:
        request = 0;
        break;
    }
    if (request != 0) {
        return PMi_SendRequestAsync(request, 0, NULL, callback, arg);
    }
    return PM_RESULT_ILLEGAL_PARAMETER;
}

// The same, waiting for the answer
static u32 func_0207ee94(u32 power) {
    u32 res;
    u32 sent = func_0207ee5c(power, PMi_SyncCallback, &res);

    if (sent == PM_RESULT_SUCCESS) {
        PMi_WaitBusy();
        return res;
    }
    return sent;
}

// PM_SetBackLightAsync
u32 PM_SetBackLightAsync(PMLCDTarget target, PMBackLightSwitch backlight, PMCallback callback, void *arg) {
    u32 request = 0;

    if (target == PM_LCD_TOP) {
        if (backlight == PM_BACKLIGHT_ON) {
            request = PM_REQ_BACKLIGHT_TOP_ON;
        }
        if (backlight == PM_BACKLIGHT_OFF) {
            request = PM_REQ_BACKLIGHT_TOP_OFF;
        }
    } else if (target == PM_LCD_BOTTOM) {
        if (backlight == PM_BACKLIGHT_ON) {
            request = PM_REQ_BACKLIGHT_BOTTOM_ON;
        }
        if (backlight == PM_BACKLIGHT_OFF) {
            request = PM_REQ_BACKLIGHT_BOTTOM_OFF;
        }
    } else if (target == PM_LCD_ALL) {
        if (backlight == PM_BACKLIGHT_ON) {
            request = PM_REQ_BACKLIGHT_ALL_ON;
        }
        if (backlight == PM_BACKLIGHT_OFF) {
            request = PM_REQ_BACKLIGHT_ALL_OFF;
        }
    }
    if (request != 0) {
        return PMi_SendRequestAsync(request, 0, NULL, callback, arg);
    }
    return PM_RESULT_ILLEGAL_PARAMETER;
}

// NitroSDK's PM_SetBackLight
u32 PM_SetBackLight(PMLCDTarget target, PMBackLightSwitch backlight) {
    u32 res;
    u32 sent = PM_SetBackLightAsync(target, backlight, PMi_SyncCallback, &res);

    if (sent == PM_RESULT_SUCCESS) {
        PMi_WaitBusy();
        return res;
    }
    return sent;
}

static u32 PM_ForceToPowerOffAsync(PMCallback callback, void *arg) {
    PMi_ExitType = PM_EXIT_TYPE_FORCED;
    func_0207f64c();
    if (OS_IsRunOnTwl()) {
        PMi_CallCallbackList(PMi_PostExitCallbackList);
    }
    return PMi_SendRequestAsync(PM_REQ_POWER_OFF, 0, NULL, callback, arg);
}

// Sends the power-off request and waits for it, polling for the answer in every state
static u32 func_0207ef60(void) {
    u32 res;
    u32 sent = PM_ForceToPowerOffAsync(PMi_SyncCallback, &res);

    if (sent == PM_RESULT_SUCCESS) {
        PMi_WaitBusyMethod = PM_WAIT_BUSY_IRQ_MASKED | PM_WAIT_BUSY_IME_OFF;
        PMi_WaitBusy();
        PMi_WaitBusyMethod = PM_WAIT_BUSY_IRQ_MODE;
        return res;
    }
    return sent;
}

void PM_ForceToPowerOff(void) {
    while (func_0207ef60() != PM_RESULT_SUCCESS) {
        OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
    }
    OS_DisableInterrupts();
    MI_StopAllDma();
    if (OS_IsRunOnTwl()) {
        MI_StopNDmaChannels();
    }
    for (;;) {
        OS_Halt();
    }
}

u32 PM_SetAmp(PMAmpSwitch amp) {
    PMi_Amp = amp;
    return PMi_SetAmp(amp);
}

// NitroSDK's PMi_SetAmp: sets the amplifier, if the screens are on
static u32 PMi_SetAmp(int amp) {
    if (PM_GetLCDPower()) {
        return PMi_ReadRegister(PM_REQ_SET_AMP, amp, NULL);
    }
    return PM_RESULT_SUCCESS;
}

u32 PM_SetAmpGain(PMAmpGain gain) {
    return PMi_ReadRegister(PM_REQ_SET_AMP_GAIN, gain, NULL);
}

u32 PM_SetAmpGainLevel(u8 level) {
    return PMi_ReadRegister(PM_REQ_SET_AMP_GAIN_LEVEL, level, NULL);
}

// PM_GetBattery
u32 PM_GetBattery(PMBattery *battery) {
    u16 data;
    u32 state = PM_BATTERY_HIGH;
    u32 result = PMi_ReadRegister(PM_REQ_READ_REGISTER, PM_REG_BATTERY, &data);

    if (result == PM_RESULT_SUCCESS && battery != NULL) {
        if (data != 0) {
            state = PM_BATTERY_LOW;
        }
        *battery = state;
    }
    return result;
}

// PM_GetBatteryLevel
u32 PM_GetBatteryLevel(u16 *level) {
    if (OS_IsRunOnTwl()) {
        return PMi_GetBatteryLevelCore(level);
    }
    return PM_RESULT_ERROR;
}

// NitroSDK's PM_GetBackLight
u32 PM_GetBackLight(PMBackLightSwitch *top, PMBackLightSwitch *bottom) {
    u16 data;
    u32 result = PMi_ReadRegister(PM_REQ_READ_REGISTER, PM_REG_BACKLIGHT, &data);

    if (result == PM_RESULT_SUCCESS) {
        if (top != NULL) {
            *top = (data & PM_BACKLIGHT_TOP_MASK) ? PM_BACKLIGHT_ON : PM_BACKLIGHT_OFF;
        }
        if (bottom != NULL) {
            *bottom = (data & PM_BACKLIGHT_BOTTOM_MASK) ? PM_BACKLIGHT_ON : PM_BACKLIGHT_OFF;
        }
    }
    return result;
}

u32 PM_GetAmp(PMAmpSwitch *amp) {
    u16 data;
    u32 result = PMi_ReadRegister(PM_REQ_READ_REGISTER, PM_REG_AMP, &data);

    if (result == PM_RESULT_SUCCESS) {
        if (amp != NULL) {
            *amp = (data != 0) ? PM_AMP_ON : PM_AMP_OFF;
        }
    }
    return result;
}

// Sends a word to the ARM7 until the FIFO takes it
static void func_0207f0d8(u32 word) {
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_PM, word, FALSE) != 0) {
    }
}

// PMi_StopDma: keeps the DMA channels from running over the sleep, by waiting for those that finish and stopping those
// that don't, and the new DMA channels of the DSi
static void PMi_StopDma(void) {
    u32 i;

    for (i = 0; i <= MI_DMA_MAX_NUM; i++) {
        PMi_PreDmaCnt[i] = MI_DMA_REGS[i].cnt;
        switch (MI_DMA_REGS[i].cnt & MI_DMA_TIMING_MASK) {
        case MI_DMA_TIMING_CARD:
            while (reg_MI_MCCNT1 & REG_MI_MCCNT1_START_MASK) {
            }
            break;
        case MI_DMA_TIMING_DISP:
        case MI_DMA_TIMING_DISP_MMEM:
            MI_WaitDma(i);
            break;
        default:
            if (!(MI_DMA_REGS[i].cnt & MI_DMA_CONTINUOUS_ON)) {
                MI_WaitDma(i);
            } else {
                MI_StopDma(i);
            }
            break;
        }
        if (OS_IsRunOnTwl()) {
            PMi_StopNDma(i);
        }
    }
}

// PM_GoSleepMode: runs the pre-sleep callbacks, turns the backlights and the displays off, stops the DMA, and has the
// ARM7 put the system to sleep with the trigger, the logic and the keys that wake it, waiting for it to wake. If the
// card was pulled out meanwhile, the system is turned off; else the screens come back as they were, and the post-sleep
// callbacks run
void PM_GoSleepMode(u32 trigger, u32 logic, u16 keyPattern) {
    BOOL cardPulledOut = FALSE;
    BOOL ime;
    OSIntrMode irq;
    u32 interrupts;
    u32 dispcnt;
    u32 dispcntSub;
    BOOL screensOn;
    PMBackLightSwitch top;
    PMBackLightSwitch bottom;
    u32 startTrigger;

    PMi_CallCallbackList(PMi_PreSleepList);
    ime = OS_DisableIrq();
    irq = OS_DisableInterrupts();
    interrupts = OS_DisableIrqMask(~0);
    OS_SetIrqMask(OS_IE_FIFO_RECV | (OS_IsTickAvailable() ? OS_IE_TIMER0 : 0));
    OS_RestoreInterrupts(irq);
    OS_EnableIrq();

    // No card to pull out when the game was started by the Download Play or from the DSi's flash. The test of the two
    // boot types is a range check in 16 bits, which adds the u16 of -OS_BOOTTYPE_DOWNLOAD_MB (0xfffe)
    if (trigger & PM_TRIGGER_CARD) {
        if ((u16)(OS_GetBootType() + (u16)-OS_BOOTTYPE_DOWNLOAD_MB) <= OS_BOOTTYPE_NAND - OS_BOOTTYPE_DOWNLOAD_MB) {
            trigger &= ~PM_TRIGGER_CARD;
        }
    }
    if (trigger & PM_TRIGGER_CARTRIDGE) {
        if (OS_IsRunOnTwl() || !CTRDG_IsExisting()) {
            trigger &= ~PM_TRIGGER_CARTRIDGE;
        }
    }

    dispcnt = reg_GX_DISPCNT;
    dispcntSub = reg_GXS_DB_DISPCNT;
    screensOn = PM_GetLCDPower();
    while (PM_GetBackLight(&top, &bottom) != PM_RESULT_SUCCESS) {
        OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
    }
    while (PM_SetBackLight(PM_LCD_ALL, PM_BACKLIGHT_OFF) != PM_RESULT_SUCCESS) {
        OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
    }
    func_0207eb9c();
    reg_GX_DISPCNT &= ~REG_GX_DISPCNT_MODE_MASK;
    reg_GXS_DB_DISPCNT &= ~REG_GXS_DB_DISPCNT_MODE_MASK;
    func_0207eb9c();
    func_0207eb9c();
    PMi_StopDma();
    PMi_WokeUp = FALSE;
    startTrigger = trigger | (top ? PM_TRIGGER_BACKLIGHT_TOP : 0) | (bottom ? PM_TRIGGER_BACKLIGHT_BOTTOM : 0);
    OS_SetIrqMask(OS_IE_FIFO_RECV);
    PMi_SendSleepStart(startTrigger, logic | keyPattern);
    while (!PMi_WokeUp) {
        OS_Halt();
    }

    OS_SetIrqMask(OS_IE_FIFO_RECV | (OS_IsTickAvailable() ? OS_IE_TIMER0 : 0));
    if ((trigger & PM_TRIGGER_CARD) && (reg_OS_IF & OS_IE_CARD_IREQ)) {
        cardPulledOut = TRUE;
    }
    if (!cardPulledOut) {
        if (screensOn == TRUE) {
            while (PMi_SetLCDPower(TRUE, PM_LCD_POWER_ON, TRUE, TRUE) != TRUE) {
            }
        } else {
            while (func_0207ee94(PM_LCD_POWER_ON) != PM_RESULT_SUCCESS) {
                OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
            }
        }
        reg_GX_DISPCNT = dispcnt;
        reg_GXS_DB_DISPCNT = dispcntSub;
    }
    OS_SpinWaitSysCycles(PM_SCREENS_SETTLE_CYCLES);
    OS_DisableIrq();
    OS_SetIrqMask(interrupts);
    OS_RestoreInterrupts(irq);
    OS_RestoreIrq(ime);
    if (cardPulledOut) {
        PM_ForceToPowerOff();
    }
    PMi_CallCallbackList(PMi_PostSleepList);
}

// PMi_SetLCDPower: turns the screens' power on or off, with the waits between the changes that the hardware needs, and
// tells the ARM7 about it with the power request if there is one. FALSE when the screens went off too recently to be on
static BOOL PMi_SetLCDPower(BOOL on, u32 power, BOOL force, BOOL sync) {
    switch (on) {
    case TRUE:
        if (!force && OS_GetVBlankCount() - PMi_ScreensOffCount <= 7) {
            return FALSE;
        }
        if (power != 0) {
            if (sync) {
                while (func_0207ee94(power) != PM_RESULT_SUCCESS) {
                    OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
                }
            } else {
                while (func_0207ee5c(power, NULL, NULL) != PM_RESULT_SUCCESS) {
                    OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
                }
            }
        }
        reg_GX_POWCNT |= REG_GX_POWCNT_LCD_MASK;
        while (PMi_SetAmp(PMi_Amp) != PM_RESULT_SUCCESS) {
            OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
        }
        break;
    case FALSE:
        while (PMi_SetAmp(PM_AMP_OFF) != PM_RESULT_SUCCESS) {
            OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
        }
        if (OS_GetVBlankCount() - PMi_DisplayOffCount <= 2) {
            func_0207eb9c();
            func_0207eb9c();
        }
        reg_GX_POWCNT &= ~REG_GX_POWCNT_LCD_MASK;
        PMi_ScreensOffCount = OS_GetVBlankCount();
        if (power != 0) {
            if (sync) {
                while (func_0207ee94(power) != PM_RESULT_SUCCESS) {
                    OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
                }
            } else {
                while (func_0207ee5c(power, NULL, NULL) != PM_RESULT_SUCCESS) {
                    OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
                }
            }
        }
        break;
    }
    return TRUE;
}

// NitroSDK's PM_SetLCDPower
BOOL PM_SetLCDPower(BOOL on) {
    if (on != TRUE) {
        on = FALSE;
        if (GX_IsDispOn()) {
            GX_DispOff();
        }
    }
    return PMi_SetLCDPower(on, 0, FALSE, TRUE);
}

// NitroSDK's PM_GetLCDPower: whether the screens' power is on
static BOOL PM_GetLCDPower(void) {
    return (reg_GX_POWCNT & REG_GX_POWCNT_LCD_MASK) != 0;
}

u32 func_0207f50c(u32 value) {
    return PMi_ReadRegister(PM_REQ_UNK_12, value, NULL);
}

u32 func_0207f520(u32 *value) {
    u16 data;
    u32 result = PMi_ReadRegister(PM_REQ_READ_REGISTER, PM_REG_UNK_8, &data);

    if (result == PM_RESULT_SUCCESS) {
        if (value != NULL) {
            *value = data;
        }
    }
    return result;
}

void PMi_InsertList(PMGenCallbackInfo **list, PMGenCallbackInfo *info, int priority, int insertType) {
    PMGenCallbackInfo *cur;
    PMGenCallbackInfo *prev;
    OSIntrMode irq;

    if (list == NULL) {
        return;
    }
    info->priority = priority;
    irq = OS_DisableInterrupts();
    prev = NULL;
    for (cur = *list; cur != NULL; prev = cur, cur = cur->next) {
        if (insertType == PM_INSERT_APPEND && cur->priority > priority) {
            break;
        }
        if (insertType == PM_INSERT_PREPEND && cur->priority >= priority) {
            break;
        }
    }
    if (cur != NULL) {
        info->next = cur;
    } else {
        info->next = NULL;
    }
    if (prev != NULL) {
        prev->next = info;
    } else {
        *list = info;
    }
    OS_RestoreInterrupts(irq);
}

void PMi_DeleteList(PMGenCallbackInfo **list, PMGenCallbackInfo *info) {
    PMGenCallbackInfo *cur;
    PMGenCallbackInfo *prev;
    OSIntrMode irq;

    if (list == NULL) {
        return;
    }
    irq = OS_DisableInterrupts();
    prev = cur = *list;
    for (; cur != NULL; prev = cur, cur = cur->next) {
        if (cur == info) {
            if (cur == prev) {
                *list = cur->next;
            } else {
                prev->next = cur->next;
            }
            break;
        }
    }
    OS_RestoreInterrupts(irq);
}

void PMi_CallCallbackList(PMGenCallbackInfo *list) {
    for (; list != NULL; list = list->next) {
        list->callback(list->arg);
    }
}

void PM_PrependPreSleepCallback(PMSleepCallbackInfo *info) {
    PMi_InsertList(&PMi_PreSleepList, info, PM_CALLBACK_PRIORITY_MIN, PM_INSERT_PREPEND);
}

void PM_AppendPostSleepCallback(PMSleepCallbackInfo *info) {
    PMi_InsertList(&PMi_PostSleepList, info, PM_CALLBACK_PRIORITY_MAX, PM_INSERT_APPEND);
}

void PM_InsertPreSleepCallback(PMSleepCallbackInfo *info, int priority) {
    PMi_InsertList(&PMi_PreSleepList, info, priority, PM_INSERT_APPEND);
}

void PM_DeletePreSleepCallback(PMSleepCallbackInfo *info) {
    PMi_DeleteList(&PMi_PreSleepList, info);
}

void PM_DeletePostSleepCallback(PMSleepCallbackInfo *info) {
    PMi_DeleteList(&PMi_PostSleepList, info);
}

// Waits out the screens before the power-off: turns the backlights off and the screens on if they were off, in every
// state of the interrupts
static void func_0207f64c(void) {
    u32 method;

    OS_SpinWaitSysCycles(PM_SCREENS_SETTLE_CYCLES);
    method = PMi_WaitBusyMethod;
    PMi_WaitBusyMethod = PM_WAIT_BUSY_ALL;
    if (PM_GetLCDPower() != TRUE) {
        while (PM_SetBackLight(PM_LCD_ALL, PM_BACKLIGHT_OFF) != PM_RESULT_SUCCESS) {
            OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
        }
        while (!PM_SetLCDPower(TRUE)) {
            OS_SpinWaitSysCycles(PM_SCREENS_RETRY_CYCLES);
        }
    }
    PMi_WaitBusyMethod = method;
}

// NitroSDK's PMi_SetDispOffCount (the name is recalled): records the vertical blank count at which the display went
// off
void PMi_SetDispOffCount(void) {
    PMi_DisplayOffCount = OS_GetVBlankCount();
}
