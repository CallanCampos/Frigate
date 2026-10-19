#include "dwcutil/dwcutil_ac.h"
#include "dwc/wcm.h"
#include "nitro/mi.h"

// The access-point connection of DWC's settings (see dwcutil_ac.h): the second copy of the WCM wrapper, and the code
// that drives it. The name says the utility's copy, because ARM9 main has a dwc_ac.c of its own (lib/dwc/src/ac)

int func_ov182_021a7bcc(void *dest, int max) {
    u8 *out = dest;
    int count;
    int i;

    WCM_LockApList(TRUE);
    count = WCM_CountApList();
    if (count > 0) {
        for (i = 0; i < count; i++) {
            if (i >= max) {
                break;
            }
            MIi_CpuCopy32(WCM_PointApList(i), out, 0xc0);
            out += 0xc0;
        }
    }
    WCM_LockApList(FALSE);
    return count;
}
