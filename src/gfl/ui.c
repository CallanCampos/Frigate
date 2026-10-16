#include "types.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/card.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nitro/pm.h"

// The keys that reset the game when held together: L, R, START and SELECT
#define SOFT_RESET_KEYS (PAD_BUTTON_L | PAD_BUTTON_R | PAD_BUTTON_START | PAD_BUTTON_SELECT)

static void CallSleepCallback(SystemUI *ui);
static void CallWakeCallback(SystemUI *ui);
static void CallLidCloseCallback(SystemUI *ui);
static void CallLidOpenCallback(SystemUI *ui);
static void battery(void);
static void CallInputCallback(SystemUI *ui);
static BOOL IsSleepBlocked(SystemUI *ui);
static void setBacklightBothScreens(u32 backlight);
static void sleepSetup(void);
static void ApplyInputHook(SystemUI *ui);

// Replaces the input of each frame, as for a replay. Never set
static HIDInputHook sInputHook;
static SystemUI *sSystemUI;

void GFL_HIDInit(HeapID heapId, u32 mode) {
    SystemUI *ui = GFL_HeapAllocate(heapId, sizeof(SystemUI), FALSE, "ui.c", 58);

    sys_memset(ui, 0, sizeof(SystemUI));
    ui->keypad = GFL_HIDCreateKeypadManager(heapId);
    ui->touchpad = initTouchpad(heapId);
    ui->usingTouch = FALSE;
    ui->updateRate = 60;
    sSystemUI = ui;
    ui->mode = mode;
    ui->batteryLevel = 5;
    if (mode == 0) {
        u32 backlight;

        PM_GetBackLight(NULL, &backlight);
        ui->backlight = backlight;
    }
    if (mode == 1) {
        u32 backlight;

        PM_GetBackLight(NULL, &backlight);
        ui->backlight = backlight;
        GCTX_HIDBlockSleep(0x10);
        GCTX_HIDBlockSoftReset(4);
    }
}

void GFL_HIDUpdate(SystemUI *ui) {
    ui->frameCount++;
    battery();
    GCTX_HIDUpdateKeypad();
    touchpadData();
    sleepSetup();
    ApplyInputHook(ui);
    if ((GCTX_HIDGetHeldKeys() & SOFT_RESET_KEYS) == SOFT_RESET_KEYS && ui->softResetBlock == 0) {
        if (ui->softResetCallback != NULL) {
            ui->softResetCallback(ui->softResetWork);
        }
        GFL_HIDDoSoftReset(0);
    }
}

void GCTX_HIDUpdate(void) {
    GFL_HIDUpdate(sSystemUI);
}

void GFL_HIDBlockSleep(SystemUI *ui, u8 flags) {
    ui->sleepBlock |= flags;
}

void GCTX_HIDBlockSleep(u8 flags) {
    GFL_HIDBlockSleep(sSystemUI, flags);
}

void GFL_HIDUnblockSleep(SystemUI *ui, u8 flags) {
    ui->sleepBlock &= ~flags;
}

void GCTX_HIDUnblockSleep(u8 flags) {
    GFL_HIDUnblockSleep(sSystemUI, flags);
}

BOOL GFL_HIDIsSleepBlocked(SystemUI *ui, u8 flags) {
    if (ui->sleepBlock & flags) {
        return TRUE;
    }
    return FALSE;
}

BOOL GCTX_HIDIsSleepBlocked(u8 flags) {
    return GFL_HIDIsSleepBlocked(sSystemUI, flags);
}

void GFL_HIDSetSleepCallback(SystemUI *ui, UICallback callback, void *work) {
    ui->sleepCallback = callback;
    ui->sleepWork = work;
}

void GCTX_HIDSetSleepCallback(UICallback callback, void *work) {
    GFL_HIDSetSleepCallback(sSystemUI, callback, work);
}

void GFL_HIDSetWakeCallback(SystemUI *ui, UICallback callback, void *work) {
    ui->wakeCallback = callback;
    ui->sleepWork = work;
}

void GCTX_HIDSetWakeCallback(UICallback callback, void *work) {
    GFL_HIDSetWakeCallback(sSystemUI, callback, work);
}

void GCTX_HIDSetSleepCheckCallback(UISleepCheckCallback callback, void *work) {
    sSystemUI->sleepCheckCallback = callback;
    sSystemUI->sleepCheckWork = work;
}

void GFL_HIDSetLidCloseCallback(SystemUI *ui, UICallback callback, void *work) {
    ui->lidCloseCallback = callback;
    ui->lidWork = work;
}

// Replacing the callbacks while the lid is closed opens it for the old ones and closes it for the new ones
void GCTX_HIDSetLidCallbacks(UICallback close, UICallback open, void *work) {
    SystemUI *ui = sSystemUI;

    if (ui->lidCloseCalled) {
        if (ui->lidOpenCallback != NULL) {
            ui->lidOpenCallback(ui->lidWork);
        }
        if (close != NULL) {
            close(work);
        }
    }
    GFL_HIDSetLidCloseCallback(sSystemUI, close, work);
    GFL_HIDSetLidOpenCallback(sSystemUI, open, work);
}

UICallback GFL_HIDGetLidCloseCallback(SystemUI *ui) {
    return ui->lidCloseCallback;
}

UICallback GCTX_HIDGetLidCloseCallback(void) {
    return GFL_HIDGetLidCloseCallback(sSystemUI);
}

void GFL_HIDSetLidOpenCallback(SystemUI *ui, UICallback callback, void *work) {
    ui->lidOpenCallback = callback;
    ui->lidWork = work;
}

void SetUICallback(SystemUI *ui, UICallback callback, void *work) {
    ui->lowBatteryCallback = callback;
    ui->lowBatteryWork = work;
}

void SetUICallbackStatic(UICallback callback, void *work) {
    SetUICallback(sSystemUI, callback, work);
}

void GFL_HIDBlockSoftReset(SystemUI *ui, u8 flags) {
    ui->softResetBlock |= flags;
}

void GCTX_HIDBlockSoftReset(u8 flags) {
    GFL_HIDBlockSoftReset(sSystemUI, flags);
}

void GFL_HIDUnblockSoftReset(SystemUI *ui, u8 flags) {
    ui->softResetBlock &= ~flags;
}

void GCTX_HIDUnblockSoftReset(u8 flags) {
    GFL_HIDUnblockSoftReset(sSystemUI, flags);
}

BOOL GFL_HIDIsSoftResetBlocked(SystemUI *ui, u8 flags) {
    if (ui->softResetBlock & flags) {
        return TRUE;
    }
    return FALSE;
}

BOOL GCTX_HIDIsSoftResetBlocked(u8 flags) {
    return GFL_HIDIsSoftResetBlocked(sSystemUI, flags);
}

void GFL_HIDSetSoftResetCallback(SystemUI *ui, SoftResetCallback callback, void *work) {
    ui->softResetCallback = callback;
    ui->softResetWork = work;
}

void GCTX_HIDSetSoftResetCallback(SoftResetCallback callback, void *work) {
    GFL_HIDSetSoftResetCallback(sSystemUI, callback, work);
}

KeypadManager *GFL_HIDGetKeypadManager(SystemUI *ui) {
    return ui->keypad;
}

SystemUI *GCTX_HIDGetInstance(void) {
    if (sSystemUI == NULL) {
        sys_exit();
    }
    return sSystemUI;
}

static void CallSleepCallback(SystemUI *ui) {
    if (ui->sleepCallback != NULL) {
        ui->sleepCallback(ui->sleepWork);
    }
}

static void CallWakeCallback(SystemUI *ui) {
    if (ui->wakeCallback != NULL) {
        ui->wakeCallback(ui->sleepWork);
    }
}

static void CallLidCloseCallback(SystemUI *ui) {
    if (ui->lidCloseCallback != NULL) {
        ui->lidCloseCallback(ui->lidWork);
        ui->lidCloseCalled = TRUE;
    }
}

static void CallLidOpenCallback(SystemUI *ui) {
    if (ui->lidOpenCallback != NULL && ui->lidCloseCalled) {
        ui->lidOpenCallback(ui->lidWork);
        ui->lidCloseCalled = FALSE;
    }
}

// Reads the battery level every ten seconds
static void battery(void) {
    u16 level;
    u32 battery;

    if (OS_GetVBlankCount() % 600 == 1) {
        if (hw_isDSi()) {
            if (PM_GetBatteryLevel(&level) == PM_RESULT_SUCCESS) {
                sSystemUI->batteryLevel = level;
            }
        } else if (PM_GetBattery(&battery) == PM_RESULT_SUCCESS) {
            if (battery == PM_BATTERY_HIGH) {
                sSystemUI->batteryLevel = 5;
            } else {
                sSystemUI->batteryLevel = 1;
            }
        }
    }
}

BOOL GCTX_HIDIsLidClosed(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

BOOL GetIsShouldUICallback(void) {
    if (GCTX_HIDGetBatteryLevel() <= 2) {
        return TRUE;
    }
    return FALSE;
}

static void CallInputCallback(SystemUI *ui) {
    if (ui->lowBatteryCallback != NULL && GetIsShouldUICallback()) {
        ui->lowBatteryCallback(ui->lowBatteryWork);
    }
}

static BOOL IsSleepBlocked(SystemUI *ui) {
    if (ui->sleepBlock) {
        return TRUE;
    }
    if (ui->sleepCheckCallback != NULL) {
        return ui->sleepCheckCallback(ui->sleepCheckWork);
    }
    return FALSE;
}

static void setBacklightBothScreens(u32 backlight) {
    int i;

    for (i = 0; i < 1000; i++) {
        if (PM_SetBackLight(PM_LCD_ALL, backlight) == PM_RESULT_SUCCESS) {
            break;
        }
    }
}

// Sleeps while the lid is closed, or turns the screens off if sleep is blocked
static void sleepSetup(void) {
    SystemUI *ui = GCTX_HIDGetInstance();

    if (PAD_DetectFold()) {
        if (!IsSleepBlocked(ui)) {
            func_0203db7c();
            CallSleepCallback(ui);
            PM_GoSleepMode(PM_TRIGGER_COVER_OPEN | PM_TRIGGER_CARD, 0, 0);
            CallInputCallback(ui);
            CallLidOpenCallback(ui);
            CallWakeCallback(ui);
            func_0203db44();
            if (ui->lidClosed) {
                ui->lidClosed = FALSE;
                setBacklightBothScreens(ui->backlight);
            }
        } else if (!ui->lidClosed) {
            ui->lidClosed = TRUE;
            setBacklightBothScreens(PM_BACKLIGHT_OFF);
            CallLidCloseCallback(ui);
            CallSleepCallback(ui);
        }
    } else if (ui->lidClosed) {
        ui->lidClosed = FALSE;
        setBacklightBothScreens(ui->backlight);
        CallWakeCallback(ui);
        CallLidOpenCallback(ui);
    }
}

// Turns the screens white and resets once the network and the card are idle
void GFL_HIDDoSoftReset(u32 parameter) {
    SystemUI *ui = GCTX_HIDGetInstance();

    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, 16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, 16);
    func_02042860(0);
    for (;;) {
        if (func_02042ab8() && func_0206f890()) {
            setBacklightBothScreens(ui->backlight);
            sys_reset(parameter);
        }
        irq_waitFor(TRUE, OS_IE_V_BLANK);
        func_020428e0();
        GFL_FadeUpdate();
    }
}

BOOL func_0203d554(void) {
    return sSystemUI->usingTouch;
}

void func_0203d564(BOOL touch) {
    sSystemUI->usingTouch = touch;
}

void GFL_HIDResetFrameCount(SystemUI *ui) {
    ui->frameCount = 0;
}

// Sets the frame rate the game updates at, 60 or 30, through sSystemUI whatever ui is
void GFL_HIDSetUpdateRate(SystemUI *ui, u8 rate) {
    sSystemUI->updateRate = rate;
}

u8 GFL_HIDGetUpdateRate(SystemUI *ui) {
    return ui->updateRate;
}

void GCTX_HIDSetUpdateRate(u8 rate) {
    GFL_HIDSetUpdateRate(sSystemUI, rate);
}

u8 GCTX_HIDGetUpdateRate(void) {
    return GFL_HIDGetUpdateRate(sSystemUI);
}

void GCTX_HIDResetFrameCount(void) {
    GFL_HIDResetFrameCount(sSystemUI);
}

void GCTX_HIDChangeFPS(u8 rate) {
    GFL_HIDSetUpdateRate(sSystemUI, rate);
    GFL_HIDClearKeypadState(sSystemUI);
    GFL_HIDClearTouchState(sSystemUI);
}

int GCTX_HIDGetBatteryLevel(void) {
    return sSystemUI->batteryLevel;
}

static void ApplyInputHook(SystemUI *ui) {
    HIDInputState state;
    HIDInputState state30;

    if (sInputHook != NULL && sInputHook(&state, &state30)) {
        func_0203df90(ui, state.pressed, state30.pressed);
        func_0203dfa0(ui, state.held, state30.held);
        func_0203dfb0(ui, state.typed, state30.typed);
        func_0203dbec(ui, state.touchX, state.touchY, state30.touchX, state30.touchY);
        func_0203dc14(ui, state.touchPressed, state30.touchPressed);
        func_0203dc2c(ui, state.touchHeld, state30.touchHeld);
    }
}
