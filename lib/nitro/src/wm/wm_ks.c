// The wireless manager's key sharing (libwm, TwlSDK's wm/ARM9/src/wm_ks.c, by its place in the library: the file name
// is a guess): the data sharing with two bytes of keys for every console. The public functions have the SDK's names,
// from what they do; the static ones' names are ours
#include "nitro/wm.h"

// Shares with all the AIDs, in the double mode
WMErrCode WM_StartKeySharing(WMKeySetBuf *buf, u16 port) {
    return WM_StartDataSharing(buf, port, 0xffff, 2, TRUE);
}

WMErrCode WM_EndKeySharing(WMKeySetBuf *buf) {
    return WM_EndDataSharing(buf);
}
