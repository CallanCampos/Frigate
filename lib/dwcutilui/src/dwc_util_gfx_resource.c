#include "dwc_util_gfx_resource.h"
#include "dwc_util_connection.h"
#include "dwc_util_main.h"
#include "dwc_util_msg_resource.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_shared.h"
#include "dwc_util_sub_screen.h"
#include "dwc_util_text.h"
#include "dwc_util_ui_object.h"
#include "nitro/os.h"
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

char *func_ov182_021ab63c(const char *path) {
    int language;

    strncpy(sPath, path, sizeof(sPath) - 1);
    if (path[5] == 'x') {
        return sPath;
    }
    language = DWCUtilMain_GetLanguage();
    if (path[5] == 'y' && language != 0) {
        return sPath;
    }
    sPath[5] = sLanguageChars[language];
    return sPath;
}

void func_ov182_021ab67c(const char *path, void (*upload)(const void *, u32, u32)) {
    u32 size;
    void *data;

    data = DWCUtilRomResource_LoadFile(func_ov182_021ab63c(path), &size, 4);
    DC_FlushRange(data, size);
    upload(data, 0, size);
    DWCUtilRomResource_FreeFile(data);
}

void func_ov182_021ab6a4(int variant) {
    u32 layout;

    DWCUtilMain_GetLayout(&layout, NULL);
    if (layout == 1) {
        func_ov182_021b8154(variant);
    } else if (layout == 2) {
        func_ov182_021b84c0(variant);
    }
}

void func_ov182_021ab6d0(int variant) {
    u32 layout;

    DWCUtilMain_GetLayout(NULL, &layout);
    if (layout == 1) {
        DWCUtilUiObject_Init(variant);
    }
}

void func_ov182_021ab6f0(int msgIndex) {
    DWCUtilText *text = func_ov182_021bbdec(0, 0);
    u16 *str = DWCUtilMsg_Get(data_ov182_021c40b8, msgIndex);

    func_ov182_021bc0a4(text, sMsgBox.x, sMsgBox.y, sMsgBox.width, sMsgBox.height, 2, func_ov182_021ab7b4(), str);
    func_ov182_021bc184(text);
}

void func_ov182_021ab738(const void *str, int msgIndex) {
    DWCUtilText *text = func_ov182_021bbdec(0, 0);
    u16 *msg = DWCUtilMsg_Get(data_ov182_021c40b8, msgIndex);

    func_ov182_021bc0a4(text, sStrMsgBox.x, sStrMsgBox.y, sStrMsgBox.width, sStrMsgBox.height, 2, func_ov182_021ab7b4(),
                        msg);
    func_ov182_021bbf98(text, sStrPositions[DWCUtilMain_GetLanguage()].x, sStrPositions[DWCUtilMain_GetLanguage()].y, 2,
                        0x209, str);
    func_ov182_021bc184(text);
}

u32 func_ov182_021ab7b4(void) {
    return sTextFlags[DWCUtilMain_GetLanguage()];
}
