#include "dwc_util_bg_overlay.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_shared.h"
#include "dwc_util_task.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

#define DWC_UTIL_BG_OVERLAY_SIZE 0xc0

static void DWCUtilBgOverlay_UploadTask(DWCUtilTask *task, void *arg);

static char *sScreenPaths[20] = {
    "char/jb2HlWiFi.nsc.l",   "char/jb2HlAp.nsc.l",      "char/jb3HlList1.nsc.l", "char/jb3HlList2.nsc.l",
    "char/jb3HlList3.nsc.l",  "char/jb4HlSet1.nsc.l",    "char/jb4HlSet2.nsc.l",  "char/jb4HlSet3.nsc.l",
    "char/jb4HlUsb.nsc.l",    "char/jb4HlSsid.nsc.l",    "char/jb4HlWep.nsc.l",   "char/jb4HlIp.nsc.l",
    "char/jb4HlMask.nsc.l",   "char/jb4HlGateway.nsc.l", "char/jb4HlDns0.nsc.l",  "char/jb4HlDns1.nsc.l",
    "char/jb5HlOption.nsc.l", "char/jb5HlInfo.nsc.l",    "char/jb5HlErase.nsc.l", "char/jb5HlMove.nsc.l",
};
static void *sOverlayScreen;

void DWCUtilBgOverlay_Init(void) {
    sOverlayScreen = DWCUtilHeap_AllocClear(DWC_UTIL_BG_OVERLAY_SIZE, 4);
    DWCUtilGfxResource_Load("char/jbBgHl.ncg.l", GX_LoadBG1Char);
    switch (DWCUtilMain_GetStartScreen()) {
    case 0:
        DWCUtilGfxResource_Load(sScreenPaths[0], GX_LoadBG1Scr);
        break;
    case 1:
        DWCUtilGfxResource_Load(sScreenPaths[1], GX_LoadBG1Scr);
        break;
    }
}

void DWCUtilBgOverlay_Exit(void) {
    DWCUtilHeap_FreeAndNull(&sOverlayScreen);
}

void DWCUtilBgOverlay_Show(int index) {
    void *data = DWCUtilRomResource_LoadFile(DWCUtilGfxResource_LocalizePath(sScreenPaths[index]), NULL, 4);

    MIi_CpuCopyFast(data, sOverlayScreen, DWC_UTIL_BG_OVERLAY_SIZE);
    DWCUtilRomResource_FreeFile(data);
    DWCUtilTask_Create(1, DWCUtilBgOverlay_UploadTask, NULL, 0x78);
}

void DWCUtilBgOverlay_ShowConnectionSet(void) {
    DWCUtilBgOverlay_Show(DWCUtilWifiSettings_GetEdit()->index + 5);
}

void DWCUtilBgOverlay_ShowConnectionList(void) {
    DWCUtilBgOverlay_Show(DWCUtilWifiSettings_GetEdit()->index + 2);
}

static void DWCUtilBgOverlay_UploadTask(DWCUtilTask *task, void *arg) {
    DC_FlushRange(sOverlayScreen, DWC_UTIL_BG_OVERLAY_SIZE);
    GX_LoadBG1Scr(sOverlayScreen, 0, DWC_UTIL_BG_OVERLAY_SIZE);
    DWCUtilTask_QueueRemoval(1, task);
}
