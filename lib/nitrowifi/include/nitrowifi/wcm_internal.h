#ifndef POKEBW2_NITROWIFI_WCM_INTERNAL_H
#define POKEBW2_NITROWIFI_WCM_INTERNAL_H

#include "nitrowifi/wcm.h"

// Descriptive private interface recovered from WCM and its LTD callers. The original header identity and
// source-file boundaries are unconfirmed; these declarations do not establish additional SDK API names.
typedef struct WCMWork WCMWork;
typedef struct WCMGlobal WCMGlobal;

// Borrowed internal storage. WCM_Finish invalidates the work pointer; the globals have static lifetime.
// Callers must obey WCM's initialization and IRQ synchronization requirements.
WCMWork *WCM_GetWork(void);
WCMGlobal *WCM_GetGlobal(void);
void WCM_SetState(s32 newState);
u16 WCM_PickChannel(s32 channelIndex);

// Notification callbacks run synchronously with temporary notice storage and must not retain its pointer.
// Mixed pointer/integer payloads use ARM32 words; a host port must widen this representation.
void WCM_Notify(s16 notify, s16 result, s32 parameter, void *param1, s32 param2);
void WCM_NotifyResult(s16 result, void *parameter, u32 detail, u32 line);

// Adds a mode-dependent encoded signal sample; its physical units are unconfirmed.
void WCM_AddRssiSample(s32 rawSample);
// Cancels the timeout alarm and rearms it while connected in state 9.
void WCM_CancelTimeout(void);
// Copies six MAC-address bytes into WCM's globals; the source pointer is not retained.
void WCM_SetMacAddress(const u8 *mac);

#endif // POKEBW2_NITROWIFI_WCM_INTERNAL_H
