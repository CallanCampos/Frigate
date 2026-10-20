#include "dwc_util_link_icon.h"
#include "dwc_util_cell.h"
#include "dwc_util_heap.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_task.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/wm.h"

typedef struct DWCUtilLinkIcon {
    GXOamAttr *oam;
    DWCUtilTask *task;
    s8 set;
} DWCUtilLinkIcon;

static void func_ov182_021aba9c(DWCUtilTask *task, void *arg);

// The cell of each link level, from none to the strongest
static const u8 sLevelCells[2][4] = { { 0x18, 0x17, 0x16, 0x15 }, { 0x5f, 0x5e, 0x5d, 0x5c } };
static DWCUtilLinkIcon *sLinkIcon;

void func_ov182_021ab9fc(int set) {
    if (sLinkIcon != NULL) {
        return;
    }
    sLinkIcon = DWCUtilHeap_AllocClear(sizeof(DWCUtilLinkIcon), 4);
    sLinkIcon->set = set;
    sLinkIcon->oam = func_ov182_021bb960(0, sLevelCells[set][0]);
    G2_SetOBJPosition(sLinkIcon->oam, 0xe5, 0x26);
    G2_SetOBJPriority(sLinkIcon->oam, 2);
    sLinkIcon->task = DWCUtilTask_Create(0, func_ov182_021aba9c, NULL, 0x78);
}

void func_ov182_021aba70(void) {
    if (sLinkIcon != NULL) {
        DWCUtilTask_Remove(0, sLinkIcon->task);
        func_ov182_021bcc9c(sLinkIcon->oam);
        DWCUtilHeap_FreeAndNull(&sLinkIcon);
    }
}

static void func_ov182_021aba9c(DWCUtilTask *task, void *arg) {
    WMLinkLevel level = WM_LINK_LEVEL_0;
    BOOL enabled = OS_DisableIrq();

    if (WM_GetAllowedChannel() != 0x8000) {
        level = WM_GetLinkLevel();
    }
    OS_RestoreIrq(enabled);
    func_ov182_021bb914(0, sLevelCells[sLinkIcon->set][level], sLinkIcon->oam);
    G2_SetOBJPosition(sLinkIcon->oam, 0xe5, 0x26);
    G2_SetOBJPriority(sLinkIcon->oam, 2);
}
