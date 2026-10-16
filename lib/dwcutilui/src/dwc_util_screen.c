#include "dwc_util_screen.h"
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

    func_ov182_021ab67c(sBlankScreenPath, GXS_LoadBG0Scr);
    reg_G2S_DB_BG0OFS = 0x01920000;
    text = func_ov182_021bbdec(1, 0);
    str = func_ov182_021ae118(data_ov182_021c40b8, msgIndex, slot, value);
    flags = func_ov182_021ab7b4();
    func_ov182_021bc0a4(text, 0x14, 0, 0xd8, 0x40, 2, flags, str);
    func_ov182_021bc184(text);
    sScreenActive = TRUE;
    return TRUE;
}

BOOL DWCUtilScreen_Stop(void) {
    if (!sScreenActive) {
        return FALSE;
    }
    func_ov182_021bbf7c(1);
    sScreenActive = FALSE;
    return TRUE;
}
