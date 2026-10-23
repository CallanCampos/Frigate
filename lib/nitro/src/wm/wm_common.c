// The checks of whether the wireless has been initialized (libwm, TwlSDK's wm/ARM9.TWL/common/src/wm_common.c by the
// place of its code in the library: the file name is a guess). The second asks the DSi's own wireless library, whose
// check the linker reaches through a veneer
#include "nitro/os.h"
#include "nitro/wm.h"
#include "twl/nwm.h"

int WM_CheckInitialized(void) {
    return WMi_CheckInitialized();
}

// On a DS there is no such library, which its return code calls an illegal state
int NWM_CheckInitialized(void) {
    if (OS_IsRunOnTwl()) {
        return NWMi_CheckInitialized();
    }
    return WM_ERRCODE_ILLEGAL_STATE;
}
