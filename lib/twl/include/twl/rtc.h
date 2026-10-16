#ifndef POKEBW2_TWL_RTC_H
#define POKEBW2_TWL_RTC_H

#include "types.h"
#include "nitro/rtc.h"

// TwlSDK's RTC library, DSi extension, in the LTD autoload (rtc_api_ex.c). The RTC's PXI callback in main
// (func_0207cd20) hands it the commands that the NitroSDK's RTC does not know, when the game runs on a DSi
// (hw_isDSi). It keeps its default name until the LTD autoload is analyzed.

// Runs the command that main's RTC work holds (func_0207cd18 in nitro/rtc.h), which is 0x10 to 0x12 here, and returns
// an RTCResult: any other command is RTC_RESULT_INVALID_COMMAND
int func_02700000(void);

#endif // POKEBW2_TWL_RTC_H
