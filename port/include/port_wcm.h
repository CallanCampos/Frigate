// Host workspace extent for the DS wireless backend of NitroWiFi's WCM. DWC's two wrappers reserve this extent
// before their access-point descriptors; native pointers enlarge WCMWork beyond the original WCM_WORK_SIZE.
#ifndef FRIGATE_PORT_WCM_H
#define FRIGATE_PORT_WCM_H

#include "nitrowifi/wcm_internal.h"
#include <stddef.h>

// The DSi backend shares a union with the DS backend, so sizeof(WCMWork) includes unrelated NWM storage.
#define PORT_WCM_DS_WORK_SIZE ((offsetof(WCMWork, unk_22ec) + sizeof(((WCMWork *)0)->unk_22ec) + 31) & ~31)

#endif
