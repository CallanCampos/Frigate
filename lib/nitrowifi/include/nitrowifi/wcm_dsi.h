#ifndef POKEBW2_NITROWIFI_WCM_DSI_H
#define POKEBW2_NITROWIFI_WCM_DSI_H

#include "types.h"

// WCM's DSi backend (wcm_dsi.c), in the LTD autoload's .ltdmain: the steps that wcm_system.c takes through TwlSDK's NWM
// in mode 2. They share WCM's work, globals and notifications. The ROM names none of them, so the file has a
// descriptive name and the functions keep their default names.
s32 func_02769930(void);
s32 func_02769a30(void);
s32 func_02769aa4(const u8 *bssid, const u8 *essid, u32 option);
s32 func_02769b80(void);
s32 func_02769c70(void);
s32 func_02769ce8(void);
// dst is a six-byte address. The caller supplies a narrowed byte length; completion may be NULL for a blocking send.
s32 func_02769dc4(const u8 *dst, const void *buffer, u16 byteLength, void (*completion)(s32 result));
s32 func_02769e54(void);

#endif // POKEBW2_NITROWIFI_WCM_DSI_H
