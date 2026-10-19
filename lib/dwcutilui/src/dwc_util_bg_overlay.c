#include "dwc_util_bg_overlay.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_shared.h"
#include "dwc_util_task.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

#define DWC_UTIL_BG_OVERLAY_SIZE 0xc0

static void func_ov182_021ab9d4(DWCUtilTask *task, void *arg);

static char *sScreenPaths[20] = {
    "char/jb2HlWiFi.nsc.l",   "char/jb2HlAp.nsc.l",      "char/jb3HlList1.nsc.l", "char/jb3HlList2.nsc.l",
    "char/jb3HlList3.nsc.l",  "char/jb4HlSet1.nsc.l",    "char/jb4HlSet2.nsc.l",  "char/jb4HlSet3.nsc.l",
    "char/jb4HlUsb.nsc.l",    "char/jb4HlSsid.nsc.l",    "char/jb4HlWep.nsc.l",   "char/jb4HlIp.nsc.l",
    "char/jb4HlMask.nsc.l",   "char/jb4HlGateway.nsc.l", "char/jb4HlDns0.nsc.l",  "char/jb4HlDns1.nsc.l",
    "char/jb5HlOption.nsc.l", "char/jb5HlInfo.nsc.l",    "char/jb5HlErase.nsc.l", "char/jb5HlMove.nsc.l",
};
static void *sOverlayScreen;

void func_ov182_021ab90c(void) {
    sOverlayScreen = func_ov182_021bc4d8(DWC_UTIL_BG_OVERLAY_SIZE, 4);
    func_ov182_021ab67c("char/jbBgHl.ncg.l", GX_LoadBG1Char);
    switch (func_ov182_021ae024()) {
    case 0:
        func_ov182_021ab67c(sScreenPaths[0], GX_LoadBG1Scr);
        break;
    case 1:
        func_ov182_021ab67c(sScreenPaths[1], GX_LoadBG1Scr);
        break;
    }
}

void func_ov182_021ab95c(void) {
    func_ov182_021bc4f0(&sOverlayScreen);
}

void func_ov182_021ab96c(int index) {
    void *data = func_ov182_021bb37c(func_ov182_021ab63c(sScreenPaths[index]), NULL, 4);

    MIi_CpuCopyFast(data, sOverlayScreen, DWC_UTIL_BG_OVERLAY_SIZE);
    func_ov182_021bb420(data);
    func_ov182_021bd200(1, func_ov182_021ab9d4, NULL, 0x78);
}

void func_ov182_021ab9ac(void) {
    func_ov182_021ab96c(func_ov182_021b781c()->index + 5);
}

void func_ov182_021ab9c0(void) {
    func_ov182_021ab96c(func_ov182_021b781c()->index + 2);
}

static void func_ov182_021ab9d4(DWCUtilTask *task, void *arg) {
    DC_FlushRange(sOverlayScreen, DWC_UTIL_BG_OVERLAY_SIZE);
    GX_LoadBG1Scr(sOverlayScreen, 0, DWC_UTIL_BG_OVERLAY_SIZE);
    func_ov182_021bd268(1, task);
}
