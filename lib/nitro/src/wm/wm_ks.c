// The wireless manager's key sharing (libwm, TwlSDK's wm/ARM9/src/wm_ks.c, by its place in the library; ntrtwl's
// reconstruction of NitroSDK has the same file): the data sharing with two bytes of keys for every console. The names
// are NitroSDK's, as that reconstruction has them
#include "nitro/wm.h"

// Shares with all the AIDs, in the double mode
WMErrCode WM_StartKeySharing(WMKeySetBuf *buf, u16 port) {
    return WM_StartDataSharing(buf, port, 0xffff, 2, TRUE);
}

WMErrCode WM_EndKeySharing(WMKeySetBuf *buf) {
    return WM_EndDataSharing(buf);
}
