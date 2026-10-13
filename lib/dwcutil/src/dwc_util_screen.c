#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "nitro/gx.h"

// The three functions and their data occupy one original object. The name is descriptive.
static char sBlankScreenPath[] = "char/jtNull.nsc.l";
static u8 sScreenActive;

void DWCUtilScreen_Reset(void) {
    sScreenActive = FALSE;
}

BOOL DWCUtilScreen_Start(void *param, int arg1, int arg2) {
    void *object;
    void *work;
    void *resource;

    if (sScreenActive) {
        return FALSE;
    }

    func_ov182_021ab67c(sBlankScreenPath, gfxUploadBGScreen0B);
    reg_G2S_DB_BG0OFS = 0x01920000;
    object = func_ov182_021bbdec(1, 0);
    work = func_ov182_021ae118(data_ov182_021c40b8, param, arg1, arg2);
    resource = func_ov182_021ab7b4();
    func_ov182_021bc0a4(object, 0x14, 0, 0xd8, 0x40, 2, resource, work);
    func_ov182_021bc184(object);
    sScreenActive = TRUE;
    return TRUE;
}

BOOL DWCUtilScreen_Stop(void) {
    u8 *active = &sScreenActive;

    if (!*active) {
        return FALSE;
    }
    func_ov182_021bbf7c(1);
    *active = FALSE;
    return TRUE;
}
