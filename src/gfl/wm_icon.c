#include "types.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/wm_icon.h"
#include "nitro/gx.h"
#include "nitro/hw.h"

#define WM_ICON_PLTT_OFFSET (14 * 0x20)

// The icon's OBJ, and the one behind it as an OBJ window
#define WM_ICON_ATTR01 0x40000000
#define WM_ICON_ATTR01_WINDOW 0x40000800
#define WM_ICON_ATTR01_HIDDEN 0x40000200
#define WM_ICON_ATTR2 0xe000

typedef struct {
    int level;
    u16 x;
    u16 y;
    u8 unk8[4];
    u8 type;
    u8 screen;
    // Whether the icon may be in the sub engine's OAM
    u8 subOam;
    u8 visible;
    GXOamAttr *oam;
} WmIcon;

typedef struct {
    u16 x;
    u16 y : 14;
    u16 screen : 2;
    WmIcon *icon;
} WmIconSys;

static WmIcon *loadWifiStrengthIcons(u32 unused, u32 heapId, u32 x, u32 y, u32 type, WmIcon *icon, u32 screen,
                                     u32 subOam, int level);
static void func_0203e520(WmIcon *icon);
static u32 func_0203e5ac(WmIcon *icon);
static u32 func_0203e5ec(u32 engine, u32 charNo);
static void func_0203e65c(WmIcon *icon, int level);
static void func_0203e664(WmIcon *icon);
static void func_0203e694(WmIcon *icon, BOOL top, HeapID heapId);
static u32 getObjBank(u32 engine);
static void func_0203e6cc(u32 engine, u32 type, HeapID heapId);
static void copyEmptyOAM(void);
static void func_0203e890(u32 screen);

// Two OBJ off the bottom of the screen
static const u32 sEmptyOam[8] = { 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0 };

static WmIconSys sWmIcon;

static inline void HideOam(GXOamAttr *oam) {
    oam->attr01 = WM_ICON_ATTR01_HIDDEN;
    oam->attr2 = 0;
}

static WmIcon *loadWifiStrengthIcons(u32 unused, u32 heapId, u32 x, u32 y, u32 type, WmIcon *icon, u32 screen,
                                     u32 subOam, int level) {
    if (icon == NULL) {
        icon = GFL_HeapAllocate(HEAPID_TAIL((HeapID)heapId), sizeof(WmIcon), FALSE, "wm_icon.c", 100);
    }
    icon->x = x;
    icon->y = y;
    if (sWmIcon.x != 0) {
        icon->x = sWmIcon.x;
        icon->y = sWmIcon.y;
    }
    icon->level = level;
    icon->type = type;
    icon->subOam = subOam;
    icon->screen = screen;
    icon->visible = FALSE;
    icon->oam = (GXOamAttr *)HW_OAM;
    func_0203e6cc(func_0203e5ac(icon), icon->type, heapId);
    return icon;
}

static void func_0203e520(WmIcon *icon) {
    u32 engine = func_0203e5ac(icon);
    u32 charNo = func_0203e5ec(engine, icon->level);
    GXOamAttr *oam;

    if (engine == 1) {
        oam = (GXOamAttr *)HW_OAM;
    } else {
        oam = (GXOamAttr *)HW_DB_OAM;
    }
    if (icon->visible == TRUE) {
        oam[0].attr01 = ((icon->x & 0x1ff) << 16) | (WM_ICON_ATTR01_WINDOW | (icon->y & 0xff));
        oam[0].attr2 = WM_ICON_ATTR2 | charNo;
    } else {
        oam[0].attr01 = WM_ICON_ATTR01_HIDDEN;
        oam[0].attr2 = 0;
    }
    oam[1].attr01 = ((icon->x & 0x1ff) << 16) | (WM_ICON_ATTR01 | (icon->y & 0xff));
    oam[1].attr2 = charNo | WM_ICON_ATTR2;
    if (oam != icon->oam) {
        HideOam(icon->oam);
        HideOam(icon->oam + 1);
        icon->oam = oam;
    }
}

static u32 func_0203e5ac(WmIcon *icon) {
    switch (icon->screen) {
    case WM_ICON_SCREEN_TOP:
        if (GX_GetDispSelect() == GX_DISP_SELECT_MAIN_SUB) {
            return 1;
        }
        return 2;
    case WM_ICON_SCREEN_BOTTOM:
        if (GX_GetDispSelect() == GX_DISP_SELECT_MAIN_SUB) {
            return 2;
        }
        return 1;
    }
    return 1;
}

// The character name of character charNo in the OBJ VRAM's mapping
static u32 func_0203e5ec(u32 engine, u32 charNo) {
    GXOBJVRamModeChar mode;

    if (engine == 1) {
        gfxGetObjBanksA();
        mode = GX_GetOBJVRamModeChar();
    } else {
        gfxGetObjBanksB();
        mode = GXS_GetOBJVRamModeChar();
    }
    switch (mode) {
    case GX_OBJVRAMMODE_CHAR_1D_32K:
        return charNo << 2;
    case GX_OBJVRAMMODE_CHAR_1D_64K:
        return charNo << 1;
    case GX_OBJVRAMMODE_CHAR_1D_128K:
        return charNo;
    case GX_OBJVRAMMODE_CHAR_1D_256K:
    default:
        GFL_ASSERT(0 && "非対応のOBJマッピングモード"); // "Unsupported OBJ mapping mode"
        return charNo;
    }
}

static void func_0203e65c(WmIcon *icon, int level) {
    if (level < 4) {
        icon->level = level;
    }
}

static void func_0203e664(WmIcon *icon) {
    GXOamAttr *oam = (GXOamAttr *)HW_OAM;

    oam[0].attr01 = WM_ICON_ATTR01_HIDDEN;
    oam[0].attr2 = 0;
    oam[1].attr01 = WM_ICON_ATTR01_HIDDEN;
    oam[1].attr2 = 0;
    if (icon->subOam) {
        oam = (GXOamAttr *)HW_DB_OAM;
        oam[0].attr01 = WM_ICON_ATTR01_HIDDEN;
        oam[0].attr2 = 0;
        oam[1].attr01 = WM_ICON_ATTR01_HIDDEN;
        oam[1].attr2 = 0;
    }
    GFL_HeapFree(icon);
}

static void func_0203e694(WmIcon *icon, BOOL top, HeapID heapId) {
    icon->screen = top ? WM_ICON_SCREEN_TOP : WM_ICON_SCREEN_BOTTOM;
    icon->subOam = TRUE;
    func_0203e6cc(func_0203e5ac(icon), icon->type, heapId);
}

static u32 getObjBank(u32 engine) {
    if (engine == 1) {
        gfxGetObjBanksA();
    } else {
        gfxGetObjBanksB();
    }
    return 0;
}

static void func_0203e6cc(u32 engine, u32 type, HeapID heapId) {
    u32 obj;
    ArcTool *arc;
    WmIconFiles files;
    u32 pltt;

    arc = GFL_ArcSysCreateFileHandle(getWifiIconNarcIdxNum(), HEAPID_TAIL(heapId));
    func_020116b0(&files);
    if (engine == 1) {
        pltt = 1;
        obj = 0;
    } else {
        pltt = 5;
        obj = 1;
    }
    GFL_G2DIOLoadArcNCLRDefault(arc, files.pltt, pltt, WM_ICON_PLTT_OFFSET, 0x20, heapId);
    loadOBJCharToVram(arc, files.chars[type == 0 ? 1 : 0], obj, getObjBank(engine), 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
}

static void copyEmptyOAM(void) {
    gfxUploadOAMA(sEmptyOam, 0, sizeof(sEmptyOam));
    gfxUploadOAMB(sEmptyOam, 0, sizeof(sEmptyOam));
}

void func_0203e76c(u32 x, u32 y, u32 type, HeapID heapId) {
    copyEmptyOAM();
    if (sWmIcon.icon != NULL) {
        u32 screen;
        int level;
        u32 subOam;

        subOam = sWmIcon.icon->subOam;
        screen = sWmIcon.icon->screen;
        level = sWmIcon.icon->level;
        func_0203e7dc();
        sWmIcon.icon = loadWifiStrengthIcons(0, heapId, x, y, type, sWmIcon.icon, screen, subOam, level);
    } else {
        sWmIcon.icon = loadWifiStrengthIcons(0, heapId, x, y, type, sWmIcon.icon, sWmIcon.screen, FALSE, 3);
    }
}

void func_0203e7dc(void) {
    copyEmptyOAM();
    if (sWmIcon.icon != NULL) {
        func_0203e664(sWmIcon.icon);
        sWmIcon.icon = NULL;
    }
}

void func_0203e7f8(int level) {
    if (sWmIcon.icon != NULL) {
        func_0203e65c(sWmIcon.icon, level);
    }
}

void func_0203e810(BOOL top, HeapID heapId) {
    func_0203e890(top ? WM_ICON_SCREEN_TOP : WM_ICON_SCREEN_BOTTOM);
    if (sWmIcon.icon != NULL) {
        func_0203e694(sWmIcon.icon, top, heapId);
    }
}

void func_0203e838(void) {
    if (sWmIcon.icon != NULL) {
        func_0203e520(sWmIcon.icon);
    }
}

void func_0203e84c(void) {
    if (sWmIcon.icon != NULL) {
        sWmIcon.icon->visible = TRUE;
    }
}

void func_0203e860(void) {
    if (sWmIcon.icon != NULL) {
        sWmIcon.icon->visible = FALSE;
    }
}

void func_0203e874(u16 x, u16 y) {
    sWmIcon.x = x;
    sWmIcon.y = y;
}

static void func_0203e890(u32 screen) {
    sWmIcon.screen = screen;
}

u32 func_0203e8b0(void) {
    if (sWmIcon.icon != NULL) {
        return func_0203e5ac(sWmIcon.icon);
    }
    return 1;
}
