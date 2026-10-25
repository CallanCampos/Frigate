#include "common.h"
#include "archive.h"
#include "button.h"
#include "dwc_util_shared.h"
#include "font.h"
#include "main.h"
#include "message.h"
#include "nitro/os.h"
#include "sceneinit.h"
#include "step.h"
#include <string.h>

typedef struct DWCUtilTextPos {
    u16 x;
    u16 y;
} DWCUtilTextPos;

// The message box's text area, under a string and alone
typedef struct DWCUtilTextBox {
    u16 x;
    u16 y;
    u16 width;
    u16 height;
} DWCUtilTextBox;

static const DWCUtilTextBox sStrMsgBox = { 0xd, 0x3c, 0xe6, 0x5e };
static const DWCUtilTextBox sMsgBox = { 0xd, 0x28, 0xe6, 0x70 };
static const char sLanguageChars[] = "jefgisk";
static const u32 sTextFlags[7] = { 0x480, 0x280, 0x280, 0x280, 0x280, 0x280, 0x280 };
static const DWCUtilTextPos sStrPositions[7] = {
    { 0x6b, 0x22 }, { 0x6c, 0x22 }, { 0x7c, 0x22 }, { 0x5d, 0x22 }, { 0x5f, 0x22 }, { 0x7d, 0x22 }, { 0x50, 0x22 },
};
static char sPath[0x40];

char *DWCi_Char(const char *path) {
    int language;

    strncpy(sPath, path, sizeof(sPath) - 1);
    if (path[5] == 'x') {
        return sPath;
    }
    language = DWCi_Language();
    if (path[5] == 'y' && language != 0) {
        return sPath;
    }
    sPath[5] = sLanguageChars[language];
    return sPath;
}

void DWCi_CMNlLoadResource(const char *path, void (*upload)(const void *, u32, u32)) {
    u32 size;
    void *data;

    data = DWCi_ARClReadEx(DWCi_Char(path), &size, 4);
    DC_FlushRange(data, size);
    upload(data, 0, size);
    DWCi_ARClRelease(data);
}

void DWCi_CMNlStep(int variant) {
    u32 layout;

    DWCi_GetParam_(&layout, NULL);
    if (layout == 1) {
        DWCi_STEPlBegin(variant);
    } else if (layout == 2) {
        DWCi_STEPlChange(variant);
    }
}

void DWCi_CMNlButton(int variant) {
    u32 layout;

    DWCi_GetParam_(NULL, &layout);
    if (layout == 1) {
        DWCi_BTNlInit(variant);
    }
}

void DWCi_CMNlMsgDraw(int msgIndex) {
    DWCUtilText *text = DWCi_FNTlFormBgEx(0, 0);
    u16 *str = DWCi_MSGlGet(gDWCUtilMsg, msgIndex);

    DWCi_FNTlDrawRect(text, sMsgBox.x, sMsgBox.y, sMsgBox.width, sMsgBox.height, 2, DWCi_CMNlFontFlag(), str);
    DWCi_FNTlRenewBg(text);
}

void DWCi_CMNlMsgNameDraw(const void *str, int msgIndex) {
    DWCUtilText *text = DWCi_FNTlFormBgEx(0, 0);
    u16 *msg = DWCi_MSGlGet(gDWCUtilMsg, msgIndex);

    DWCi_FNTlDrawRect(text, sStrMsgBox.x, sStrMsgBox.y, sStrMsgBox.width, sStrMsgBox.height, 2, DWCi_CMNlFontFlag(),
                      msg);
    DWCi_FNTlDraw(text, sStrPositions[DWCi_Language()].x, sStrPositions[DWCi_Language()].y, 2, 0x209, str);
    DWCi_FNTlRenewBg(text);
}

u32 DWCi_CMNlFontFlag(void) {
    return sTextFlags[DWCi_Language()];
}
