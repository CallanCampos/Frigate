// NitroSDK's power management library (TwlSDK's libspi, spi/ARM9/src/pm.c): the ARM9's side of the requests to the
// ARM7's power management through the PXI FIFO, for the backlights and the screens' power, the battery, the sound
// amplifier of the microphone, the sleep mode and the power-off. The file name is the SDK's, and the split is certain:
// the functions come in the SDK's order, and the .bss and .data hold its statics. The file has TwlSDK's DSi part too,
// in TwlSDK's LTDMAIN section, which the LTD autoload's .ltdmain holds: dsd lists it as lib/twl/src/spi/pm_twl.c, which
// config/merged_sources.toml links from this file's object, since the parts share the object's statics, and the DSi
// part's place among them sets the layout of the .bss. The main part calls the DSi part through the linker's veneers
// (func_0207ece4/e8, func_0207f058/5c and func_0207f16c/70, after their callers), and the DSi part calls back through
// veneers of its own. The functions of both parts have TwlSDK's names, which aletheia's match of Black 1's code to
// TwlSDK's pm.o (libspi.TWL.HYB.a) gives in the same order, as does PMi_ResetLocked; the other statics have NitroSDK's
// names, as ntrtwl's reconstruction of NitroSDK has them, or names of ours.
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

// A request is one or two PXI words with the tag of the power management. The first has SPI_PXI_START_BIT and the
// command in bits 8 to 14, the last has SPI_PXI_END_BIT, and the other carries its index in bits 16 to 22 and 16 bits
// of data
#define SPI_PXI_START_BIT 0x02000000
#define SPI_PXI_END_BIT 0x01000000
#define PM_PXI_INDEX(index) ((index) << 16)
#define PM_PXI_COMMAND(command) ((command) << 8)
#define SPI_PXI_RESULT_COMMAND_MASK 0x7f00
#define SPI_PXI_RESULT_DATA_MASK 0xff
#define SPI_PXI_DATA_MASK 0xffff

// The commands that the ARM9 sends: the one before the sleep, a request to the ARM7 with its number in the low byte of
// the first word and its data in the second, and the one that starts the sleep, with the trigger in the low byte and
// the key pattern in the second word
#define SPI_PXI_COMMAND_PM_SYNC 0x60
#define PM_PXI_COMMAND_REQUEST 0x61
#define PM_PXI_COMMAND_SLEEP_START 0x62
// What the ARM7 answers with: the same commands, then the end of the sleep, and the ARM7's own notifications
#define PM_PXI_COMMAND_WOKE_UP 0x63
#define PM_PXI_COMMAND_NOTIFY 0x70

// The notifications. Two start the exit sequence of the DSi, which is told why, and one is for a low battery
#define PM_PXI_NOTIFY_EXIT_1 0
#define PM_PXI_NOTIFY_BATTERY_LOW 4
#define PM_PXI_NOTIFY_EXIT_2 5

// The requests, in the low byte of the first word of a PM_PXI_COMMAND_REQUEST: three that follow the screens' power,
// six for the backlights, the power-off, the settings and reads of the registers, and the DSi's shutdown and restart
#define PM_UTIL_LED_ON 1
#define PM_UTIL_LED_BLINK_HIGH_SPEED 2
#define PM_UTIL_LED_BLINK_LOW_SPEED 3
#define PM_UTIL_LCD1_BACKLIGHT_ON 4
#define PM_UTIL_LCD1_BACKLIGHT_OFF 5
#define PM_UTIL_LCD2_BACKLIGHT_ON 6
#define PM_UTIL_LCD2_BACKLIGHT_OFF 7
#define PM_UTIL_LCD12_BACKLIGHT_ON 8
#define PM_UTIL_LCD12_BACKLIGHT_OFF 9
#define PM_UTIL_FORCE_POWER_OFF 0xe
#define PM_REQ_READ_REGISTER 0xf
#define PM_REQ_SET_AMP 0x10
#define PM_REQ_SET_AMP_GAIN 0x11
#define PM_REQ_UNK_12 0x12
#define PM_REQ_SHUTDOWN 0x13
#define PM_REQ_RESET 0x14
#define PM_REQ_SET_AMP_GAIN_LEVEL 0x16

// The registers that PM_REQ_READ_REGISTER reads, with the DSi's battery level, and the bits of the backlight one
#define PM_REG_BATTERY 0
#define PM_REG_BATTERY_LEVEL 1
#define PM_REG_BACKLIGHT 3
#define PM_REG_AMP 6
#define PM_REG_UNK_8 8
#define PMIC_CTL_BKLT2 0x08
#define PMIC_CTL_BKLT1 0x04

// What PMi_SetLCDPower tells the ARM7 about the screens' power: on or off, which map to the requests, and a third
#define PM_LED_ON 1
#define PM_LED_BLINK_LOW 2
#define PM_LED_BLINK_HIGH 3

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
// The wait between two reads of the vertical blank count in the DSi's exit
#define PM_VBLANK_WAIT_CYCLES 0x32

// The word of main memory that a restart after an exit notification clears
#define PM_RESET_PARAM (*(u32 *)0x02000300)
// The shared memory of the handshake with the ARM7 before the exit, and the byte that says the ARM9 is done
#define PM_EXIT_SYNC_WORK ((vu8 *)0x02fffff0)
#define PM_EXIT_SYNC_DONE (*(vu8 *)0x02fffff4)

// The sleep leaves on the PXI FIFO's interrupt, and the timer of the clock when it runs. The card's pull-out interrupt
// request (OS_IE_CARD_IREQ) wakes the system. The card's control register keeps its start bit set while a transfer runs

typedef struct {
    // The request being answered, with its callback, and where the answer's data goes
    BOOL lock;
    PMCallback callback;
    void *callbackArg;
    u16 *work;
} PMiWork;

// The order of these decides the layout of the .bss, which MWCC sorts by size from the order of declaration (the two
// words of the .data count in it too)
static u16 PMi_IsInit;
static PMiWork PMi_Work;
// The callbacks that run after the system wakes
static PMGenCallbackInfo *PMi_PostSleepCallbackList;
// Set when the ARM7 says that the system woke up
static BOOL PMi_SleepEndFlag;
// The vertical blank count when the screens' power went off, and when the display did
static u32 PMi_LCDCount;
static u32 PMi_DisplayOffCount;
// Why the system is exiting
static u32 PMi_ExitType;
// The DMA control registers before the sleep
static u32 PMi_PreDmaCnt[4];
// clang-format off
// TwlSDK's section for the DSi-only code and data that the LTD autoload's .ltdmain holds
#pragma define_section LTDMAIN ".ltdmain" ".ltdmain.bss" ".ltdmain" abs32 RWX
// clang-format on
// The DSi part's statics, in the LTD autoload's .ltdmain.bss, are declared here, among the others: their place and
// order set both the main part's .bss offsets and the layout of the ROM's .ltdmain.bss (docs/matching.md). Each has a
// section of its own, which the linker places in the order of the size sort too
#pragma section LTDMAIN begin
// The callback for a low battery
static PMGenCallbackInfo PMi_BatteryLowCallbackInfo;
// The callbacks that run when the exit sequence starts
static PMGenCallbackInfo *PMi_PreExitCallbackList;
// Set once the exit sequence has started
static BOOL PMi_ResetLocked;
// The callbacks that run when the application is ready to exit
static PMGenCallbackInfo *PMi_PostExitCallbackList;
#pragma section LTDMAIN end
// What the amplifier was set to, for when the screens come back on
static int sAmpSwitch;
// The callbacks that run before the system sleeps
static PMGenCallbackInfo *PMi_PreSleepCallbackList;
// How PMi_WaitBusy waits; the power-off waits another way
static u32 PMi_WaitBusyMethod = PM_WAIT_BUSY_IRQ_MODE;
// Whether the system exits by itself once the exit callbacks have run
static BOOL PMi_AutoExitFlag = TRUE;

static void PMi_WaitBusy(void);
static void PMi_DummyCallback(u32 result, void *arg);
static void PMi_CallCallbackAndUnlock(u32 result);
static void PMi_WaitVBlank(void);
static void PMi_CommonCallback(PXIFifoTag tag, u32 data, BOOL err);
static u32 PMi_TryToSendPxiData(const u32 *words, int length, u16 *result, PMCallback callback, void *arg);
static void PMi_TryToSendPxiDataTillSuccess(const u32 *words, int length);
static u32 PMi_SendSleepStart(u16 trigger, u16 keyPattern);
static u32 PM_SendUtilityCommandAsync(u32 request, u32 data, u16 *result, PMCallback callback, void *arg);
static u32 PMi_SetLEDAsync(u32 power, PMCallback callback, void *arg);
static u32 PMi_SetLED(u32 power);
static u32 PM_ForceToPowerOffAsync(PMCallback callback, void *arg);
static u32 PMi_ForceToPowerOff(void);
static u32 PMi_SetAmp(int amp);
static void PMi_SendPxiData(u32 word);
static void PMi_PreSleepForDma(void);
static BOOL PMi_SetLCDPower(BOOL on, u32 power, BOOL force, BOOL sync);
static BOOL PM_GetLCDPower(void);
static void PMi_LCDOnAvoidReset(void);
static void PMi_FinalizeDebugger(void);
static void PMi_CallPostExitCallbackAndReset(BOOL reset);
static BOOL PMi_TryLockForReset(void);

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
static void PMi_DummyCallback(u32 result, void *arg) {
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
static void PMi_WaitVBlank(void) {
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
    PMi_LCDCount = PMi_DisplayOffCount = OS_GetVBlankCount();
}

// The ARM7's answers, and its notifications
static void PMi_CommonCallback(PXIFifoTag tag, u32 data, BOOL err) {
    u16 command, value;
    BOOL finish = TRUE;

    command = (data & SPI_PXI_RESULT_COMMAND_MASK) >> 8;
    value = data & SPI_PXI_RESULT_DATA_MASK;

    if (err) {
        PMi_CallCallbackAndUnlock(command == PM_PXI_COMMAND_REQUEST || command == PM_PXI_COMMAND_SLEEP_START
                                      ? PM_RESULT_BUSY
                                      : PM_RESULT_ERROR);
        return;
    }
    switch (command) {
    case PM_PXI_COMMAND_REQUEST:
        if (PMi_Work.work != NULL) {
            *PMi_Work.work = value;
        }
        value = 0;
        break;
    case SPI_PXI_COMMAND_PM_SYNC:
        value = 0;
        break;
    case PM_PXI_COMMAND_SLEEP_START:
        break;
    case PM_PXI_COMMAND_WOKE_UP:
        PMi_SleepEndFlag = TRUE;
        break;
    case PM_PXI_COMMAND_NOTIFY:
        switch (value) {
        case PM_PXI_NOTIFY_EXIT_1:
            PMi_ProceedToExit(PM_EXIT_FACTOR_PWSW);
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
            PMi_ProceedToExit(PM_EXIT_FACTOR_BATTERY);
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

// Sends the words of a request, if the ARM7 is not on another. PM_RESULT_BUSY if it is. The name is ours: NitroSDK's
// PMi_SendPxiData is the one-word sender that this calls
static u32 PMi_TryToSendPxiData(const u32 *words, int length, u16 *result, PMCallback callback, void *arg) {
    OSIntrMode irq = OS_DisableInterrupts();
    int i;

    if (PMi_Work.lock) {
        OS_RestoreInterrupts(irq);
        return PM_RESULT_BUSY;
    }
    PMi_Work.lock = TRUE;
    PMi_Work.work = result;
    PMi_Work.callback = callback;
    PMi_Work.callbackArg = arg;
    for (i = 0; i < length; i++) {
        PMi_SendPxiData(words[i]);
    }
    OS_RestoreInterrupts(irq);
    return PM_RESULT_SUCCESS;
}

// Sends the words, retrying until the ARM7 accepts them and answers with success
static void PMi_TryToSendPxiDataTillSuccess(const u32 *words, int length) {
    // Set by the callback, from the interrupt
    vu32 result;

    for (;;) {
        result = PM_SYNC_PENDING;
        while (PMi_TryToSendPxiData(words, length, NULL, PMi_DummyCallback, (void *)&result) != PM_RESULT_SUCCESS) {
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

    words[0] = SPI_PXI_START_BIT | SPI_PXI_END_BIT | PM_PXI_COMMAND(SPI_PXI_COMMAND_PM_SYNC);
    PMi_TryToSendPxiDataTillSuccess(words, 1);
    while (PMi_SetLCDPower(FALSE, PM_LED_BLINK_LOW, FALSE, TRUE) != TRUE) {
    }
    words[0] = SPI_PXI_START_BIT | PM_PXI_COMMAND(PM_PXI_COMMAND_SLEEP_START) | (trigger & SPI_PXI_RESULT_DATA_MASK);
    words[1] = SPI_PXI_END_BIT | PM_PXI_INDEX(1) | (keyPattern & SPI_PXI_DATA_MASK);
    PMi_TryToSendPxiDataTillSuccess(words, 2);
    return PM_RESULT_SUCCESS;
}

// Sends a request with a halfword of data. The ARM7's data goes to *result
static u32 PM_SendUtilityCommandAsync(u32 request, u32 data, u16 *result, PMCallback callback, void *arg) {
    u32 words[2];

    words[0] = SPI_PXI_START_BIT | PM_PXI_COMMAND(PM_PXI_COMMAND_REQUEST) | (request & SPI_PXI_RESULT_DATA_MASK);
    words[1] = SPI_PXI_END_BIT | PM_PXI_INDEX(1) | (data & SPI_PXI_DATA_MASK);
    return PMi_TryToSendPxiData(words, 2, result, callback, arg);
}

// PM_SendUtilityCommand (pokeheartgold's PMi_ReadRegister): sends a request with a halfword of data and waits for the
// answer
u32 PM_SendUtilityCommand(u8 request, u16 data, u16 *result) {
    u32 res;
    u32 sent = PM_SendUtilityCommandAsync(request, data, result, PMi_DummyCallback, &res);

    if (sent == PM_RESULT_SUCCESS) {
        PMi_WaitBusy();
        return res;
    }
    return sent;
}

// Sends the request that the screens' power change has
static u32 PMi_SetLEDAsync(u32 power, PMCallback callback, void *arg) {
    u32 request;

    switch (power) {
    case PM_LED_ON:
        request = PM_UTIL_LED_ON;
        break;
    case PM_LED_BLINK_HIGH:
        request = PM_UTIL_LED_BLINK_HIGH_SPEED;
        break;
    case PM_LED_BLINK_LOW:
        request = PM_UTIL_LED_BLINK_LOW_SPEED;
        break;
    default:
        request = 0;
        break;
    }
    if (request != 0) {
        return PM_SendUtilityCommandAsync(request, 0, NULL, callback, arg);
    }
    return PM_INVALID_COMMAND;
}

// The same, waiting for the answer
static u32 PMi_SetLED(u32 power) {
    u32 res;
    u32 sent = PMi_SetLEDAsync(power, PMi_DummyCallback, &res);

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
            request = PM_UTIL_LCD2_BACKLIGHT_ON;
        }
        if (backlight == PM_BACKLIGHT_OFF) {
            request = PM_UTIL_LCD2_BACKLIGHT_OFF;
        }
    } else if (target == PM_LCD_BOTTOM) {
        if (backlight == PM_BACKLIGHT_ON) {
            request = PM_UTIL_LCD1_BACKLIGHT_ON;
        }
        if (backlight == PM_BACKLIGHT_OFF) {
            request = PM_UTIL_LCD1_BACKLIGHT_OFF;
        }
    } else if (target == PM_LCD_ALL) {
        if (backlight == PM_BACKLIGHT_ON) {
            request = PM_UTIL_LCD12_BACKLIGHT_ON;
        }
        if (backlight == PM_BACKLIGHT_OFF) {
            request = PM_UTIL_LCD12_BACKLIGHT_OFF;
        }
    }
    if (request != 0) {
        return PM_SendUtilityCommandAsync(request, 0, NULL, callback, arg);
    }
    return PM_INVALID_COMMAND;
}

// NitroSDK's PM_SetBackLight
u32 PM_SetBackLight(PMLCDTarget target, PMBackLightSwitch backlight) {
    u32 res;
    u32 sent = PM_SetBackLightAsync(target, backlight, PMi_DummyCallback, &res);

    if (sent == PM_RESULT_SUCCESS) {
        PMi_WaitBusy();
        return res;
    }
    return sent;
}

static u32 PM_ForceToPowerOffAsync(PMCallback callback, void *arg) {
    PMi_ExitType = PM_EXIT_FACTOR_USER;
    PMi_LCDOnAvoidReset();
    if (OS_IsRunOnTwl()) {
        PMi_ExecuteList(PMi_PostExitCallbackList);
    }
    return PM_SendUtilityCommandAsync(PM_UTIL_FORCE_POWER_OFF, 0, NULL, callback, arg);
}

// Sends the power-off request and waits for it, polling for the answer in every state
static u32 PMi_ForceToPowerOff(void) {
    u32 res;
    u32 sent = PM_ForceToPowerOffAsync(PMi_DummyCallback, &res);

    if (sent == PM_RESULT_SUCCESS) {
        PMi_WaitBusyMethod = PM_WAIT_BUSY_IRQ_MASKED | PM_WAIT_BUSY_IME_OFF;
        PMi_WaitBusy();
        PMi_WaitBusyMethod = PM_WAIT_BUSY_IRQ_MODE;
        return res;
    }
    return sent;
}

void PM_ForceToPowerOff(void) {
    while (PMi_ForceToPowerOff() != PM_RESULT_SUCCESS) {
        OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
    }
    OS_DisableInterrupts();
    MI_StopAllDma();
    if (OS_IsRunOnTwl()) {
        MI_StopAllNDma();
    }
    for (;;) {
        OS_Halt();
    }
}

u32 PM_SetAmp(PMAmpSwitch amp) {
    sAmpSwitch = amp;
    return PMi_SetAmp(amp);
}

// NitroSDK's PMi_SetAmp: sets the amplifier, if the screens are on
static u32 PMi_SetAmp(int amp) {
    if (PM_GetLCDPower()) {
        return PM_SendUtilityCommand(PM_REQ_SET_AMP, amp, NULL);
    }
    return PM_RESULT_SUCCESS;
}

u32 PM_SetAmpGain(PMAmpGain gain) {
    return PM_SendUtilityCommand(PM_REQ_SET_AMP_GAIN, gain, NULL);
}

u32 PM_SetAmpGainLevel(u8 level) {
    return PM_SendUtilityCommand(PM_REQ_SET_AMP_GAIN_LEVEL, level, NULL);
}

// PM_GetBattery
u32 PM_GetBattery(PMBattery *battery) {
    u16 data;
    u32 state = PM_BATTERY_HIGH;
    u32 result = PM_SendUtilityCommand(PM_REQ_READ_REGISTER, PM_REG_BATTERY, &data);

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
    u32 result = PM_SendUtilityCommand(PM_REQ_READ_REGISTER, PM_REG_BACKLIGHT, &data);

    if (result == PM_RESULT_SUCCESS) {
        if (top != NULL) {
            *top = (data & PMIC_CTL_BKLT2) ? PM_BACKLIGHT_ON : PM_BACKLIGHT_OFF;
        }
        if (bottom != NULL) {
            *bottom = (data & PMIC_CTL_BKLT1) ? PM_BACKLIGHT_ON : PM_BACKLIGHT_OFF;
        }
    }
    return result;
}

u32 PM_GetAmp(PMAmpSwitch *amp) {
    u16 data;
    u32 result = PM_SendUtilityCommand(PM_REQ_READ_REGISTER, PM_REG_AMP, &data);

    if (result == PM_RESULT_SUCCESS) {
        if (amp != NULL) {
            *amp = (data != 0) ? PM_AMP_ON : PM_AMP_OFF;
        }
    }
    return result;
}

// NitroSDK's PMi_SendPxiData: sends a word to the ARM7 until the FIFO takes it
static void PMi_SendPxiData(u32 word) {
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_PM, word, FALSE) != 0) {
    }
}

// PMi_PreSleepForDma: keeps the DMA channels from running over the sleep, by waiting for those that finish and stopping
// those that don't, and the new DMA channels of the DSi
static void PMi_PreSleepForDma(void) {
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
            PMi_PreSleepForNdma(i);
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

    PMi_ExecuteList(PMi_PreSleepCallbackList);
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
    PMi_WaitVBlank();
    reg_GX_DISPCNT &= ~REG_GX_DISPCNT_MODE_MASK;
    reg_GXS_DB_DISPCNT &= ~REG_GXS_DB_DISPCNT_MODE_MASK;
    PMi_WaitVBlank();
    PMi_WaitVBlank();
    PMi_PreSleepForDma();
    PMi_SleepEndFlag = FALSE;
    startTrigger = trigger | (top ? PM_TRIGGER_BACKLIGHT_TOP : 0) | (bottom ? PM_TRIGGER_BACKLIGHT_BOTTOM : 0);
    OS_SetIrqMask(OS_IE_FIFO_RECV);
    PMi_SendSleepStart(startTrigger, logic | keyPattern);
    while (!PMi_SleepEndFlag) {
        OS_Halt();
    }

    OS_SetIrqMask(OS_IE_FIFO_RECV | (OS_IsTickAvailable() ? OS_IE_TIMER0 : 0));
    if ((trigger & PM_TRIGGER_CARD) && (reg_OS_IF & OS_IE_CARD_IREQ)) {
        cardPulledOut = TRUE;
    }
    if (!cardPulledOut) {
        if (screensOn == TRUE) {
            while (PMi_SetLCDPower(TRUE, PM_LED_ON, TRUE, TRUE) != TRUE) {
            }
        } else {
            while (PMi_SetLED(PM_LED_ON) != PM_RESULT_SUCCESS) {
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
    PMi_ExecuteList(PMi_PostSleepCallbackList);
}

// PMi_SetLCDPower: turns the screens' power on or off, with the waits between the changes that the hardware needs, and
// tells the ARM7 about it with the power request if there is one. FALSE when the screens went off too recently to be on
static BOOL PMi_SetLCDPower(BOOL on, u32 power, BOOL force, BOOL sync) {
    switch (on) {
    case TRUE:
        if (!force && OS_GetVBlankCount() - PMi_LCDCount <= 7) {
            return FALSE;
        }
        if (power != 0) {
            if (sync) {
                while (PMi_SetLED(power) != PM_RESULT_SUCCESS) {
                    OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
                }
            } else {
                while (PMi_SetLEDAsync(power, NULL, NULL) != PM_RESULT_SUCCESS) {
                    OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
                }
            }
        }
        reg_GX_POWCNT |= REG_GX_POWCNT_LCD_MASK;
        while (PMi_SetAmp(sAmpSwitch) != PM_RESULT_SUCCESS) {
            OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
        }
        break;
    case FALSE:
        while (PMi_SetAmp(PM_AMP_OFF) != PM_RESULT_SUCCESS) {
            OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
        }
        if (OS_GetVBlankCount() - PMi_DisplayOffCount <= 2) {
            PMi_WaitVBlank();
            PMi_WaitVBlank();
        }
        reg_GX_POWCNT &= ~REG_GX_POWCNT_LCD_MASK;
        PMi_LCDCount = OS_GetVBlankCount();
        if (power != 0) {
            if (sync) {
                while (PMi_SetLED(power) != PM_RESULT_SUCCESS) {
                    OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
                }
            } else {
                while (PMi_SetLEDAsync(power, NULL, NULL) != PM_RESULT_SUCCESS) {
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

u32 PMi_SendLEDPatternCommand(u32 value) {
    return PM_SendUtilityCommand(PM_REQ_UNK_12, value, NULL);
}

u32 PM_GetLEDPattern(u32 *value) {
    u16 data;
    u32 result = PM_SendUtilityCommand(PM_REQ_READ_REGISTER, PM_REG_UNK_8, &data);

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

void PMi_ExecuteList(PMGenCallbackInfo *list) {
    for (; list != NULL; list = list->next) {
        list->callback(list->arg);
    }
}

void PM_PrependPreSleepCallback(PMSleepCallbackInfo *info) {
    PMi_InsertList(&PMi_PreSleepCallbackList, info, PM_CALLBACK_PRIORITY_MIN, PM_INSERT_PREPEND);
}

void PM_AppendPostSleepCallback(PMSleepCallbackInfo *info) {
    PMi_InsertList(&PMi_PostSleepCallbackList, info, PM_CALLBACK_PRIORITY_MAX, PM_INSERT_APPEND);
}

void PMi_InsertPreSleepCallbackEx(PMSleepCallbackInfo *info, int priority) {
    PMi_InsertList(&PMi_PreSleepCallbackList, info, priority, PM_INSERT_APPEND);
}

void PM_DeletePreSleepCallback(PMSleepCallbackInfo *info) {
    PMi_DeleteList(&PMi_PreSleepCallbackList, info);
}

void PM_DeletePostSleepCallback(PMSleepCallbackInfo *info) {
    PMi_DeleteList(&PMi_PostSleepCallbackList, info);
}

// Waits out the screens before the power-off: turns the backlights off and the screens on if they were off, in every
// state of the interrupts
static void PMi_LCDOnAvoidReset(void) {
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

// The DSi part, in the LTD autoload: the DSi's battery level, the stop of a new DMA channel before the sleep, and the
// exit sequence, with its callbacks, that the power button and the power-off start
#pragma section LTDMAIN begin

u32 PMi_GetBatteryLevelCore(u16 *level) {
    u16 value;
    u32 result = PM_SendUtilityCommand(PM_REQ_READ_REGISTER, PM_REG_BATTERY_LEVEL, &value);

    if (result == 0 && level != NULL) {
        *level = value;
    }
    return result;
}

void PMi_PreSleepForNdma(u32 ndmaNo) {
    if (MI_NDMA_REG(ndmaNo, MI_NDMA_CNT) & MI_NDMA_IMM) {
        MI_WaitNDma(ndmaNo);
        return;
    }
    switch (MI_NDMA_REG(ndmaNo, MI_NDMA_CNT) & MI_NDMA_TIMING_MASK) {
    case MI_NDMA_TIMING_CARD:
        while (reg_MI_MCCNT1 & REG_MI_MCCNT1_START_MASK) {
        }
        break;
    case MI_NDMA_TIMING_DISP:
    case MI_NDMA_TIMING_DISP_MMEM:
        MI_WaitNDma(ndmaNo);
    case MI_NDMA_TIMING_CAMERA:
        MI_StopNDma(ndmaNo);
        break;
    default:
        if (!(MI_NDMA_REG(ndmaNo, MI_NDMA_CNT) & MI_NDMA_CONTINUOUS)) {
            MI_WaitNDma(ndmaNo);
        } else {
            MI_StopNDma(ndmaNo);
        }
        break;
    }
}

void PMi_ProceedToExit(PMExitFactor exitType) {
    if (PMi_TryLockForReset()) {
        PMi_ExitType = exitType;
        PMi_ExecuteList(PMi_PreExitCallbackList);
        if (PMi_AutoExitFlag) {
            PM_ReadyToExit();
        }
    }
}

void PM_ReadyToExit(void) {
    PMi_CallPostExitCallbackAndReset(TRUE);
}

// Tells the ARM7 that the ARM9 is about to stop
static void PMi_FinalizeDebugger(void) {
    PM_EXIT_SYNC_DONE = FALSE;
    OSi_SyncWithOtherProc(0, PM_EXIT_SYNC_WORK);
    OSi_SyncWithOtherProc(1, PM_EXIT_SYNC_WORK);
    OS_DisableInterrupts();
    PM_EXIT_SYNC_DONE = TRUE;
}

PMExitFactor PM_GetExitFactor(void) {
    return PMi_ExitType;
}

void PM_AppendPreExitCallback(PMExitCallbackInfo *info) {
    PMi_InsertList(&PMi_PreExitCallbackList, info, PM_CALLBACK_PRIORITY_MAX, PM_INSERT_APPEND);
}

void PMi_InsertPostExitCallbackEx(PMExitCallbackInfo *info, int priority) {
    PMi_InsertList(&PMi_PostExitCallbackList, info, priority, PM_INSERT_APPEND);
}

void PM_DeletePostExitCallback(PMExitCallbackInfo *info) {
    PMi_DeleteList(&PMi_PostExitCallbackList, info);
}

void PMi_ExecutePostExitCallbackList(void) {
    PMi_ExecuteList(PMi_PostExitCallbackList);
}

void PM_SetAutoExit(BOOL enabled) {
    PMi_AutoExitFlag = enabled;
}

BOOL PM_GetAutoExit(void) {
    return PMi_AutoExitFlag;
}

// Runs the post-exit callbacks, turns the displays off, waits for three vertical blanks, has the ARM7 restart or shut
// the system down until it answers, and stops the program
static void PMi_CallPostExitCallbackAndReset(BOOL reset) {
    int i;
    u16 result;

    PMi_ExecuteList(PMi_PostExitCallbackList);
    GX_DispOff();
    GXS_DispOff();
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    for (i = 0; i < 3; i++) {
        u32 count = OS_GetVBlankCount();
        while (count == OS_GetVBlankCount()) {
            OS_SpinWaitSysCycles(PM_VBLANK_WAIT_CYCLES);
        }
    }

    for (;;) {
        u8 request;
        if (reset) {
            request = PM_REQ_RESET;
        } else {
            request = PM_REQ_SHUTDOWN;
        }
        if (*(u32 *)HW_PM_EXIT_FLAG == 1) {
            request = PM_REQ_RESET;
            PM_RESET_PARAM = 0;
        }
        while (PM_SendUtilityCommand(request, 0, &result) != 0) {
            OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
        }
        if (result == 0) {
            break;
        }
        OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
    }

    OS_DisableInterrupts();
    MI_StopAllDma();
    MI_StopAllNDma();
    PMi_FinalizeDebugger();
    OSi_TerminateCore();
}

static BOOL PMi_TryLockForReset(void) {
    OSIntrMode enabled = OS_DisableInterrupts();

    if (PMi_ResetLocked) {
        OS_RestoreInterrupts(enabled);
        return FALSE;
    }
    PMi_ResetLocked = TRUE;
    OS_RestoreInterrupts(enabled);
    return TRUE;
}

#pragma section LTDMAIN end
