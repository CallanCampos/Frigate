#include "dwc_util_screen.h"
#include "dwc_util_connection.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_msg_resource.h"
#include "dwc_util_shared.h"
#include "nitro/gx.h"

// The three functions and their data occupy one original object. The name is descriptive.
static char sBlankScreenPath[] = "char/jtNull.nsc.l";
static u8 sScreenActive;

void DWCi_COMMENTlInit(void) {
    sScreenActive = FALSE;
}

BOOL DWCi_COMMENTlDispEx(int msgIndex, int slot, int value) {
    DWCUtilText *text;
    u16 *str;
    u32 flags;

    if (sScreenActive) {
        return FALSE;
    }

    DWCUtilGfxResource_Load(sBlankScreenPath, GXS_LoadBG0Scr);
    reg_G2S_DB_BG0OFS = 0x01920000;
    text = DWCi_FNTlFormBgEx(1, 0);
    str = DWCi_MSGlGetEx(gDWCUtilMsg, msgIndex, slot, value);
    flags = DWCi_CMNlFontFlag();
    DWCi_FNTlDrawRect(text, 0x14, 0, 0xd8, 0x40, 2, flags, str);
    DWCUtilText_MarkDirty(text);
    sScreenActive = TRUE;
    return TRUE;
}

BOOL DWCi_COMMENTlDelete(void) {
    if (!sScreenActive) {
        return FALSE;
    }
    DWCi_FNTlDeleteBgLcd(1);
    sScreenActive = FALSE;
    return TRUE;
}
