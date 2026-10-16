#ifndef POKEBW2_NITRO_PM_H
#define POKEBW2_NITRO_PM_H

#include "types.h"

// NitroSDK's power management (TwlSDK's libspi pm.c, whose ARM9 part is lib/nitro/src/spi/pm.c): the backlights, the
// battery, the sound amplifier of the microphone, the sleep mode and the power-off. Each call sends the ARM7 a request
// through the PXI FIFO and returns a result, PM_RESULT_SUCCESS or why not. The functions have the SDK's names, by what
// the code does

#define PM_RESULT_SUCCESS 0
// The ARM7 is still on a request
#define PM_RESULT_BUSY 1
#define PM_RESULT_ERROR 2
// A parameter has no request
#define PM_RESULT_ILLEGAL_PARAMETER 0xffff

// Called with the result when the ARM7 has answered
typedef void (*PMCallback)(u32 result, void *arg);

#define PM_LCD_TOP 0
#define PM_LCD_BOTTOM 1
#define PM_LCD_ALL 2

#define PM_BACKLIGHT_OFF 0
#define PM_BACKLIGHT_ON 1

#define PM_BATTERY_HIGH 0
#define PM_BATTERY_LOW 1

#define PM_AMP_OFF 0
#define PM_AMP_ON 1

// What wakes the system from the sleep mode: opening the lid, or the card in the DS slot being pulled out. The
// cartridge of the GBA slot works only on a DS
#define PM_TRIGGER_COVER_OPEN 0x04
#define PM_TRIGGER_CARD 0x08
#define PM_TRIGGER_CARTRIDGE 0x10

// Turns the power management on, as the first call
void PM_Init(void);

// Sends the ARM7 a request with a number and a halfword, waits for the answer and returns its result. A request that
// reads gives the ARM7's answer in *result. Swan calls it PMi_ReadRegister, after the register reads that it is
// mostly used for
u32 PMi_ReadRegister(u8 request, u16 data, u16 *result);

// PM_SetBackLight sends the request and waits for the answer, PM_SetBackLightAsync calls back
u32 PM_SetBackLightAsync(u32 target, u32 backlight, PMCallback callback, void *arg);
u32 PM_SetBackLight(u32 target, u32 backlight);
u32 PM_GetBackLight(u32 *top, u32 *bottom);
u32 PM_GetBattery(u32 *battery);
// The battery level of the DSi, 2 (PM_RESULT_ERROR) on a DS
u32 PM_GetBatteryLevel(u16 *level);

// The sound amplifier of the microphone. The setting is kept and sent again when the screens come back on
u32 PM_SetAmp(int amp);
u32 PM_GetAmp(int *amp);
u32 PM_SetAmpGain(int gain);
u32 PM_SetAmpGainLevel(u8 level);

// Turns the system off, and never returns
void PM_ForceToPowerOff(void);
// Turns the screens' power on or off, and the upper engine off with it. TRUE when it is done, FALSE when the screens
// went off too recently to be turned on
BOOL PM_SetLCDPower(BOOL on);
// Two more settings of the ARM7 (the request 0x12, and the read of the register 8)
u32 func_0207f50c(u32 value);
u32 func_0207f520(u32 *value);
// NitroSDK's PMi_SetDispOffCount (the name is recalled, not shown): records the vertical blank count at which the
// display went off, which the screens' power waits two frames from
void PMi_SetDispOffCount(void);
void PM_GoSleepMode(u32 trigger, u32 logic, u16 keyPattern);

// Callbacks run before the system sleeps and after it wakes, in lists ordered by priority: the TwlSDK's
// PM_PrependPreSleepCallback, PM_AppendPostSleepCallback, PM_DeletePreSleepCallback and PM_DeletePostSleepCallback,
// by their code
typedef void (*PMGenCallback)(void *arg);

typedef struct PMGenCallbackInfo {
    PMGenCallback callback;
    void *arg;
    int priority;
    struct PMGenCallbackInfo *next;
} PMGenCallbackInfo;

typedef PMGenCallbackInfo PMSleepCallbackInfo;

static inline void PM_SetSleepCallbackInfo(PMSleepCallbackInfo *info, PMGenCallback callback, void *arg) {
    info->callback = callback;
    info->arg = arg;
}

// The priorities of the callbacks that go first and last, and where a new one goes among those of its own priority
#define PM_CALLBACK_PRIORITY_MIN (-255)
#define PM_CALLBACK_PRIORITY_MAX 255
#define PM_INSERT_APPEND 0
#define PM_INSERT_PREPEND 1

// PMi_InsertList, PMi_DeleteList and PMi_CallCallbackList, which TwlSDK's exit callbacks (the LTD autoload) use too
void PMi_InsertList(PMGenCallbackInfo **list, PMGenCallbackInfo *info, int priority, int insertType);
void PMi_DeleteList(PMGenCallbackInfo **list, PMGenCallbackInfo *info);
void PMi_CallCallbackList(PMGenCallbackInfo *list);

void PM_PrependPreSleepCallback(PMSleepCallbackInfo *info);
void PM_AppendPostSleepCallback(PMSleepCallbackInfo *info);
// The pre-sleep callback with its priority, after those of the same one
void PM_InsertPreSleepCallback(PMSleepCallbackInfo *info, int priority);
void PM_DeletePreSleepCallback(PMSleepCallbackInfo *info);
void PM_DeletePostSleepCallback(PMSleepCallbackInfo *info);

#endif // POKEBW2_NITRO_PM_H
