#include "dwc_util_screen.h"
#include "dwc_util_connection.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_msg_resource.h"
#include "dwc_util_shared.h"
#include "nitro/gx.h"

// The three functions and their data occupy one original object. The name is descriptive.
static char sBlankScreenPath[] = "char/jtNull.nsc.l";
static u8 sScreenActive;

void DWCUtilScreen_Reset(void) {
    sScreenActive = FALSE;
}

BOOL DWCUtilScreen_Start(int msgIndex, int slot, int value) {
    DWCUtilText *text;
    u16 *str;
    u32 flags;

    if (sScreenActive) {
        return FALSE;
    }

    DWCUtilGfxResource_Load(sBlankScreenPath, GXS_LoadBG0Scr);
    reg_G2S_DB_BG0OFS = 0x01920000;
    text = DWCUtilText_CreateBg(1, 0);
    str = DWCUtilMsg_GetWithDigit(gDWCUtilMsg, msgIndex, slot, value);
    flags = DWCUtilGfxResource_GetTextFlags();
    DWCUtilText_DrawRect(text, 0x14, 0, 0xd8, 0x40, 2, flags, str);
    DWCUtilText_MarkDirty(text);
    sScreenActive = TRUE;
    return TRUE;
}

BOOL DWCUtilScreen_Stop(void) {
    if (!sScreenActive) {
        return FALSE;
    }
    DWCUtilText_DestroyScreenBg(1);
    sScreenActive = FALSE;
    return TRUE;
}
