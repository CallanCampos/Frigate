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

static void func_ov182_021bcc2c(DWCUtilTask *task, void *arg);

void func_ov182_021bcbc8(void) {
    int i;

    sOamBuf = func_ov182_021bc4d8(sizeof(DWCUtilOamBuf), 4);
    MI_CpuFillFast(sOamBuf, GX_OAM_EFFECT_NODISPLAY, sizeof(sOamBuf->oam));
    for (i = 0; i < 2; i++) {
        sOamBuf->freeOam[i] = func_ov182_021bb9c8(64, (u8 *)sOamBuf->oam[i], sizeof(GXOamAttr));
    }
    sOamBuf->task = func_ov182_021bd200(1, func_ov182_021bcc2c, NULL, 200);
}

static void func_ov182_021bcc2c(DWCUtilTask *task, void *arg) {
    DC_FlushRange(sOamBuf, sizeof(sOamBuf->oam));
    GX_LoadOAM(sOamBuf->oam[0], 0, sizeof(sOamBuf->oam[0]));
    GXS_LoadOAM(sOamBuf->oam[1], 0, sizeof(sOamBuf->oam[1]));
}

void func_ov182_021bcc5c(void) {
    func_ov182_021bd280(1, sOamBuf->task);
    func_ov182_021bc4f0(&sOamBuf);
}

GXOamAttr *func_ov182_021bcc80(int screen) {
    return func_ov182_021bba38(sOamBuf->freeOam[screen]);
}

void func_ov182_021bcc9c(GXOamAttr *oam) {
    int screen = 0;

    G2_SetOBJEffect(oam, GX_OAM_EFFECT_NODISPLAY, 0);
    if (oam >= sOamBuf->oam[1]) {
        screen = 1;
    }
    func_ov182_021bba00(sOamBuf->freeOam[screen], oam);
}

GXOamAttr *func_ov182_021bccd8(int screen, int index) {
    return &sOamBuf->oam[screen][index];
}
