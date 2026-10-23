#include "dwc_util_oam_buf.h"
#include "dwc_util_cell.h"
#include "dwc_util_heap.h"
#include "dwc_util_shared.h"
#include "nitro/mi.h"
#include "nitro/os.h"

#define DWC_UTIL_OAM_COUNT 128

typedef struct DWCUtilOamBuf {
    GXOamAttr oam[2][DWC_UTIL_OAM_COUNT];
    // The free entries of each screen
    DWCUtilStack *freeOam[2];
    DWCUtilTask *task;
} DWCUtilOamBuf;

static DWCUtilOamBuf *sOamBuf;

static void DWCUtilOamBuf_UploadTask(DWCUtilTask *task, void *arg);

void DWCi_OBJlInit(void) {
    int i;

    sOamBuf = DWCi_HEAPlAllocEx(sizeof(DWCUtilOamBuf), 4);
    MI_CpuFillFast(sOamBuf, GX_OAM_EFFECT_NODISPLAY, sizeof(sOamBuf->oam));
    for (i = 0; i < 2; i++) {
        sOamBuf->freeOam[i] = DWCi_QUElFormEx(64, (u8 *)sOamBuf->oam[i], sizeof(GXOamAttr));
    }
    sOamBuf->task = DWCi_TSKlForm(1, DWCUtilOamBuf_UploadTask, NULL, 200);
}

static void DWCUtilOamBuf_UploadTask(DWCUtilTask *task, void *arg) {
    DC_FlushRange(sOamBuf, sizeof(sOamBuf->oam));
    GX_LoadOAM(sOamBuf->oam[0], 0, sizeof(sOamBuf->oam[0]));
    GXS_LoadOAM(sOamBuf->oam[1], 0, sizeof(sOamBuf->oam[1]));
}

void DWCi_OBJlEnd(void) {
    DWCi_TSKlDeleteEx(1, sOamBuf->task);
    DWCi_HEAPlFree_(&sOamBuf);
}

GXOamAttr *DWCi_OBJlFormGx(int screen) {
    return DWCi_QUElPopFront(sOamBuf->freeOam[screen]);
}

void DWCi_MOV_MBP_GetChildBmp(GXOamAttr *oam) {
    int screen = 0;

    G2_SetOBJEffect(oam, GX_OAM_EFFECT_NODISPLAY, 0);
    if (oam >= sOamBuf->oam[1]) {
        screen = 1;
    }
    DWCi_QUElPushBack(sOamBuf->freeOam[screen], oam);
}

GXOamAttr *DWCUtilOamBuf_GetEntry(int screen, int index) {
    return &sOamBuf->oam[screen][index];
}
