// The host's size of the workspace that NitroWiFi's WCM (lib/nitrowifi/src/wcm) needs for its DS wireless backend,
// which the game's Wi-Fi connection (Nintendo Wi-Fi Connection's access-point search, through DWC) sets up. Native
// pointers enlarge WCMWork beyond the original WCM_WORK_SIZE, so under PLATFORM_PC WCM_GetRequiredWorkSize's DS case
// (lib/nitrowifi/src/wcm/wcm.c) and DWC's two wrappers (lib/dwcutil/src/dwc_wcm.c, dwcutil_ac.c) use this extent,
// and place their access-point descriptors after it; the test os_wcm_workspace (port/tests/os) checks it.
#ifndef PORT_WCM_H
#define PORT_WCM_H

#include "nitrowifi/wcm_internal.h"
#include <stddef.h>

// The DSi backend shares a union with the DS backend, so sizeof(WCMWork) includes unrelated NWM storage.
#define PORT_WCM_DS_WORK_SIZE ((offsetof(WCMWork, unk_22ec) + sizeof(((WCMWork *)0)->unk_22ec) + 31) & ~31)

#endif // PORT_WCM_H
