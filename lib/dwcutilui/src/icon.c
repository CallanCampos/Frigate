#include "icon.h"
#include "cellinfo.h"
#include "dwc_util_heap.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/wm.h"
#include "obj.h"
#include "task.h"

typedef struct DWCUtilLinkIcon {
    GXOamAttr *oam;
    DWCUtilTask *task;
    s8 set;
} DWCUtilLinkIcon;

static void taskRenew(DWCUtilTask *task, void *arg);

// The cell of each link level, from none to the strongest
static const u8 sLevelCells[2][4] = { { 0x18, 0x17, 0x16, 0x15 }, { 0x5f, 0x5e, 0x5d, 0x5c } };
static DWCUtilLinkIcon *sLinkIcon;

void DWCi_ICONlInit(int set) {
    if (sLinkIcon != NULL) {
        return;
    }
    sLinkIcon = DWCi_HEAPlAllocEx(sizeof(DWCUtilLinkIcon), 4);
    sLinkIcon->set = set;
    sLinkIcon->oam = DWCi_CEINlSetExObj(0, sLevelCells[set][0]);
    G2_SetOBJPosition(sLinkIcon->oam, 0xe5, 0x26);
    G2_SetOBJPriority(sLinkIcon->oam, 2);
    sLinkIcon->task = DWCi_TSKlForm(0, taskRenew, NULL, 0x78);
}

void DWCi_ICONlEnd(void) {
    if (sLinkIcon != NULL) {
        DWCi_TSKlDeleteEx(0, sLinkIcon->task);
        DWCi_OBJlDelete(sLinkIcon->oam);
        DWCi_HEAPlFree_(&sLinkIcon);
    }
}

static void taskRenew(DWCUtilTask *task, void *arg) {
    WMLinkLevel level = WM_LINK_LEVEL_0;
    BOOL enabled = OS_DisableIrq();

    if (WM_GetAllowedChannel() != 0x8000) {
        level = WM_GetLinkLevel();
    }
    OS_RestoreIrq(enabled);
    DWCi_CEINlSet(0, sLevelCells[sLinkIcon->set][level], sLinkIcon->oam);
    G2_SetOBJPosition(sLinkIcon->oam, 0xe5, 0x26);
    G2_SetOBJPriority(sLinkIcon->oam, 2);
}
