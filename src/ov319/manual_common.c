#include "types.h"
#include "app/manual.h"
#include "app/manual_common.h"
#include "app/manual_data.h"
#include "app/manual_graphic.h"
#include "app/manual_touchbar.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "system/gf_font.h"
#include "system/printsys.h"

// What the manual's screens share, named after the ROM's "manual_common.c": the graphics, the font, the messages, the
// tables, the touch bar, and the picture of the upper screen, a 256x192 direct-color bitmap on BG 2

#define IMAGE_SIZE (256 * 192 * 2)

static void ManualCommon_InitBG(ManualCommon *common);
static void ManualCommon_FreeBG(ManualCommon *common);

ManualCommon *ManualCommon_Create(GameData *gameData, HeapID heapId) {
    ManualCommon *common = GFL_HeapAllocate(heapId, sizeof(ManualCommon), TRUE, "manual_common.c", 75);

    common->gameData = gameData;
    common->heapId = heapId;
    common->graphic = ManualGraphic_Create(1, common->heapId);
    common->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, common->heapId);
    common->titleQueue = func_02021998(common->heapId);
    common->queue = func_02021998(common->heapId);
    common->touchBarQueue = func_02021998(common->heapId);
    common->msgSystem = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_7, common->heapId);
    common->msgManual = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_8, common->heapId);
    common->arc = GFL_ArcSysCreateFileHandle(ARCID_MANUAL, common->heapId);
    common->imageArc = GFL_ArcSysCreateFileHandle(ARCID_MANUAL_IMAGE, common->heapId);
    GFL_G2DIOLoadNCLR(ARCID_FONT, 5, PALTYPE_SUB_BG, 0, 2 * 0x20, 0x20, common->heapId);
    common->data = ManualData_Create(common->arc, common->heapId);
    ManualCommon_InitBG(common);
    common->image = MANUAL_IMAGE_NONE;
    ManualCommon_LoadImage(common, ManualData_GetDefaultImage(common->data));
    common->touchBar = ManualTouchBar_Create(common);
    return common;
}

void ManualCommon_Free(ManualCommon *common) {
    ManualTouchBar_Free(common->touchBar);
    ManualCommon_LoadImage(common, MANUAL_IMAGE_NONE);
    ManualCommon_FreeBG(common);
    ManualData_Free(common->data);
    GFL_ArcToolFree(common->arc);
    GFL_ArcToolFree(common->imageArc);
    GFL_MsgDataFree(common->msgSystem);
    GFL_MsgDataFree(common->msgManual);
    func_02021c44(common->titleQueue);
    func_02021a18(common->titleQueue);
    func_02021c44(common->queue);
    func_02021a18(common->queue);
    func_02021c44(common->touchBarQueue);
    func_02021a18(common->touchBarQueue);
    GFL_FontFree(common->font);
    ManualGraphic_Free(common->graphic);
    GFL_HeapFree(common);
}

void ManualCommon_Main(ManualCommon *common) {
    func_02021a3c(common->titleQueue);
    func_02021a3c(common->queue);
    func_02021a3c(common->touchBarQueue);
    ManualGraphic_Update(common->graphic);
}

void ManualCommon_LoadImage(ManualCommon *common, u16 image) {
    void *buffer;
    u32 size;

    if (common->image != image) {
        common->image = image;
        if (common->image == MANUAL_IMAGE_NONE) {
            buffer = GFL_HeapAllocate(common->heapId, IMAGE_SIZE, TRUE, "manual_common.c", 245);
            size = IMAGE_SIZE;
        } else {
            buffer = GFL_ArcToolReadHeapNewLZGetLen(common->imageArc, common->image, TRUE, common->heapId, &size);
        }
        cp15_flushDC(buffer, size);
        gfxUploadBGScreen2A(buffer, 0, IMAGE_SIZE);
        GFL_HeapFree(buffer);
    }
}

static void ManualCommon_InitBG(ManualCommon *common) {
    G2_SetBG2ControlDCBmp(GX_BG_SCRSIZE_DCBMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x00000);
    G2_SetBG2Priority(0);
    G2_BG0Mosaic(FALSE);
    GFL_BGSysSetBGEnabled(2, TRUE);
}

static void ManualCommon_FreeBG(ManualCommon *common) {
}
