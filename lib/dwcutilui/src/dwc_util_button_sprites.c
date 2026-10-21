#include "dwc_util_button_sprites.h"
#include "dwc_util_cell.h"
#include "dwc_util_heap.h"
#include "dwc_util_oam.h"

typedef struct DWCUtilButtonSprites {
    DWCUtilOamNode *sprites[4];
} DWCUtilButtonSprites;

static const u8 sPairCells[6][2] = { { 0x0a, 0x0b }, { 0x04, 0x05 }, { 0x02, 0x03 },
                                     { 0x0c, 0x0d }, { 0xe5, 0x00 }, { 0x26, 0x00 } };
static DWCUtilButtonSprites *sButtonSprites;

void DWCUtilButtonSprites_Init(void) {
    sButtonSprites = DWCUtilHeap_AllocClear(sizeof(DWCUtilButtonSprites), 4);
}

void DWCUtilButtonSprites_Exit(void) {
    DWCUtilButtonSprites_Hide();
    DWCUtilHeap_FreeAndNull(&sButtonSprites);
}

void DWCUtilButtonSprites_ShowPair(int pair, int x0, int x1, int y) {
    int i;

    DWCUtilButtonSprites_Hide();
    for (i = 0; i < 2; i++) {
        sButtonSprites->sprites[i] = DWCUtilCell_AllocOamRun(0, sPairCells[pair][i], TRUE);
        DWCUtilOam_SetPriority(sButtonSprites->sprites[i], -1, 1);
    }
    DWCUtilOam_SetNodePosition(sButtonSprites->sprites[0], -1, x0, y);
    DWCUtilOam_SetNodePosition(sButtonSprites->sprites[1], -1, x1, y);
}

void DWCUtilButtonSprites_ShowFour(int x0, int x1, int y0, int y1) {
    int cell = 6;
    int i;

    DWCUtilButtonSprites_Hide();
    for (i = 0; i < 4; i++) {
        sButtonSprites->sprites[i] = DWCUtilCell_AllocOamRun(0, cell, TRUE);
        DWCUtilOam_SetPriority(sButtonSprites->sprites[i], -1, 1);
        cell++;
    }
    DWCUtilOam_SetNodePosition(sButtonSprites->sprites[0], -1, x0, y0);
    DWCUtilOam_SetNodePosition(sButtonSprites->sprites[1], -1, x1, y0);
    DWCUtilOam_SetNodePosition(sButtonSprites->sprites[2], -1, x0, y1);
    DWCUtilOam_SetNodePosition(sButtonSprites->sprites[3], -1, x1, y1);
}

void DWCUtilButtonSprites_Hide(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sButtonSprites->sprites[i] != NULL) {
            DWCUtilOam_Free(sButtonSprites->sprites[i]);
            sButtonSprites->sprites[i] = NULL;
        }
    }
}
