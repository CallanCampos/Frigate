#include "types.h"
#include "battle/battle_overlay.h"
#include "gfl/overlay.h"
#include "nitro/os.h"

// Whether overlay 219 was loaded
static BOOL sOverlayLoaded;

// The higher of the development console's bits of the console type (OS_CONSOLE_DEV_MASK)
#define CONSOLE_TYPE_DEV_HIGH_BIT 0x2000000

// Loads overlay 219, only on a development console
void func_ov167_021ce10c(void) {
    if (OS_GetConsoleType() & CONSOLE_TYPE_DEV_HIGH_BIT) {
        GFL_OvlLoad(OVERLAY_ID(219));
        sOverlayLoaded = TRUE;
    } else {
        sOverlayLoaded = FALSE;
    }
}

void func_ov167_021ce138(void) {
    if (sOverlayLoaded) {
        GFL_OvlUnload(OVERLAY_ID(219));
        sOverlayLoaded = FALSE;
    }
}
