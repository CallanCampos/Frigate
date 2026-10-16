#ifndef POKEBW2_TWL_RTC_H
#define POKEBW2_TWL_RTC_H

#include "types.h"
#include "nitro/rtc.h"

// TwlSDK's RTC library, DSi extension, in the LTD autoload (rtc_api_ex.c). The RTC's PXI callback in main
// (RtcCommonCallback) hands it the commands that the NitroSDK's RTC does not know, when the game runs on a DSi
// (OS_IsRunOnTwl). Its name is descriptive.

// Runs the command that main's RTC work holds (RTCi_GetSysWork in nitro/rtc.h), which is 0x10 to 0x12 here, and returns
// an RTCResult: any other command is RTC_RESULT_INVALID_COMMAND
int RTCi_ProcessExCommand(void);

#endif // POKEBW2_TWL_RTC_H
