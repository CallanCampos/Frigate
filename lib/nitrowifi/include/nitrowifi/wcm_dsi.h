#ifndef POKEBW2_NITROWIFI_WCM_DSI_H
#define POKEBW2_NITROWIFI_WCM_DSI_H

#include "types.h"

// WCM's DSi backend (wcm_dsi.c), in the LTD autoload's .ltdmain: the steps that wcm_system.c takes through TwlSDK's NWM
// in mode 2. They share WCM's work, globals and notifications. The ROM names none of them: the file has a descriptive
// name, and the functions the names of the TwlWiFi symbols of libwcm's nwmif.o that Black 1's decompilation (aletheia)
// recovered.
s32 WCMi_NwmifStartupAsync(void);
s32 WCMi_NwmifCleanupAsync(void);
s32 WCMi_NwmifBeginSearchAsync(const u8 *bssid, const u8 *essid, u32 option);
s32 WCMi_NwmifConnectAsync(void);
s32 WCMi_NwmifDisconnectAsync(void);
s32 WCMi_NwmifTerminateAsync(void);
// dst is a six-byte address. The caller supplies a narrowed byte length; completion may be NULL for a blocking send.
s32 WCMi_NwmifSetDCFData(const u8 *dst, const void *buffer, u16 byteLength, void (*completion)(s32 result));
s32 WCMi_NwmifGetAllowedChannel(void);

#endif // POKEBW2_NITROWIFI_WCM_DSI_H
