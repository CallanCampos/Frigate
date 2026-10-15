#ifndef POKEBW2_NITROWIFI_WCM_DSI_H
#define POKEBW2_NITROWIFI_WCM_DSI_H

#include "types.h"

// Descriptive header for WCM's DSi backend in LTD. These helpers share WCM's work, globals and notifications;
// their underlying wireless primitives are separate from this WCM-level interface. The original source filename,
// SDK symbol names and exact object boundaries remain unconfirmed, so the functions retain their default names.
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
