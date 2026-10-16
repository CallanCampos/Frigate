#include "types.h"
#include "battle/battle_overlay.h"
#include "gfl/overlay.h"
#include "nitro/os.h"
#include "system/dsi.h"

// Whether overlay 219 was loaded
static BOOL sOverlayLoaded;

// Loads overlay 219 when the game's flags ask for it
void func_ov167_021ce10c(void) {
    if (func_0207ac24() & 0x2000000) {
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
