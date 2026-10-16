#include "dwc_ac.h"
#include "dwc/wcm.h"
#include "gfl/std.h"

// The access-point connection of DWC's settings (see dwc_ac.h): the second copy of the WCM wrapper, and the code that
// drives it.

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
            sys_memcpy32(WCM_PointApList(i), out, 0xc0);
            out += 0xc0;
        }
    }
    WCM_LockApList(FALSE);
    return count;
}
