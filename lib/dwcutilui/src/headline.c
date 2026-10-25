#include "headline.h"
#include "archive.h"
#include "common.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "setting.h"
#include "task.h"

#define DWC_UTIL_BG_OVERLAY_SIZE 0xc0

static void taskLoad(DWCUtilTask *task, void *arg);

static char *sScreenPaths[20] = {
    "char/jb2HlWiFi.nsc.l",   "char/jb2HlAp.nsc.l",      "char/jb3HlList1.nsc.l", "char/jb3HlList2.nsc.l",
    "char/jb3HlList3.nsc.l",  "char/jb4HlSet1.nsc.l",    "char/jb4HlSet2.nsc.l",  "char/jb4HlSet3.nsc.l",
    "char/jb4HlUsb.nsc.l",    "char/jb4HlSsid.nsc.l",    "char/jb4HlWep.nsc.l",   "char/jb4HlIp.nsc.l",
    "char/jb4HlMask.nsc.l",   "char/jb4HlGateway.nsc.l", "char/jb4HlDns0.nsc.l",  "char/jb4HlDns1.nsc.l",
    "char/jb5HlOption.nsc.l", "char/jb5HlInfo.nsc.l",    "char/jb5HlErase.nsc.l", "char/jb5HlMove.nsc.l",
};
static void *sOverlayScreen;

void DWCi_HLlInit(void) {
    sOverlayScreen = DWCi_HEAPlAllocEx(DWC_UTIL_BG_OVERLAY_SIZE, 4);
    DWCi_CMNlLoadResource("char/jbBgHl.ncg.l", GX_LoadBG1Char);
    switch (DWCi_Entry()) {
    case 0:
        DWCi_CMNlLoadResource(sScreenPaths[0], GX_LoadBG1Scr);
        break;
    case 1:
        DWCi_CMNlLoadResource(sScreenPaths[1], GX_LoadBG1Scr);
        break;
    }
}

void DWCi_HLlEnd(void) {
    DWCi_HEAPlFree_(&sOverlayScreen);
}

void DWCi_HLlLoad(int index) {
    void *data = DWCi_ARClReadEx(DWCi_Char(sScreenPaths[index]), NULL, 4);

    MIi_CpuCopyFast(data, sOverlayScreen, DWC_UTIL_BG_OVERLAY_SIZE);
    DWCi_ARClRelease(data);
    DWCi_TSKlForm(1, taskLoad, NULL, 0x78);
}

void DWCi_HLlLoadSet(void) {
    DWCi_HLlLoad(DWCi_SETTIGNlGet()->index + 5);
}

void DWCi_HLlLoadList(void) {
    DWCi_HLlLoad(DWCi_SETTIGNlGet()->index + 2);
}

static void taskLoad(DWCUtilTask *task, void *arg) {
    DC_FlushRange(sOverlayScreen, DWC_UTIL_BG_OVERLAY_SIZE);
    GX_LoadBG1Scr(sOverlayScreen, 0, DWC_UTIL_BG_OVERLAY_SIZE);
    DWCi_TSKlDelete(1, task);
}
