// The checks of whether the wireless has been initialized (libwm, TwlSDK's wm/ARM9.TWL/common/src/wm_common.c by the
// place of its code in the library: the file name is a guess). The second asks the DSi's own wireless library, whose
// check the linker reaches through a veneer
#include "nitro/os.h"
#include "nitro/wm.h"
#include "twl/nwm.h"

// WM_CheckInitialized
int func_020825e4(void) {
    return func_02080a70();
}

// NWM_CheckInitialized: on a DS there is no such library, which its return code calls an illegal state
int func_020825ec(void) {
    if (hw_isDSi()) {
        return func_02702698();
    }
    return WM_ERRCODE_ILLEGAL_STATE;
}
