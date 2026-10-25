#include "focus.h"
#include "cell.h"
#include "cellinfo.h"
#include "dwc_util_heap.h"

typedef struct DWCUtilButtonSprites {
    DWCUtilOamNode *sprites[4];
} DWCUtilButtonSprites;

static const u8 sPairCells[6][2] = { { 0x0a, 0x0b }, { 0x04, 0x05 }, { 0x02, 0x03 },
                                     { 0x0c, 0x0d }, { 0xe5, 0x00 }, { 0x26, 0x00 } };
static DWCUtilButtonSprites *sButtonSprites;

void DWCi_FOCUSlInit(void) {
    sButtonSprites = DWCi_HEAPlAllocEx(sizeof(DWCUtilButtonSprites), 4);
}

void DWCi_FOCUSlEnd(void) {
    DWCi_FOCUSlDelete();
    DWCi_HEAPlFree_(&sButtonSprites);
}

void DWCi_FOCUSlDisp(int pair, int x0, int x1, int y) {
    int i;

    DWCi_FOCUSlDelete();
    for (i = 0; i < 2; i++) {
        sButtonSprites->sprites[i] = DWCUtilCell_AllocOamRun(0, sPairCells[pair][i], TRUE);
        DWCi_CELLlSetPriority(sButtonSprites->sprites[i], -1, 1);
    }
    DWCi_CELLlSetPosition(sButtonSprites->sprites[0], -1, x0, y);
    DWCi_CELLlSetPosition(sButtonSprites->sprites[1], -1, x1, y);
}

void DWCi_FOCUSlDisp4(int x0, int x1, int y0, int y1) {
    int cell = 6;
    int i;

    DWCi_FOCUSlDelete();
    for (i = 0; i < 4; i++) {
        sButtonSprites->sprites[i] = DWCUtilCell_AllocOamRun(0, cell, TRUE);
        DWCi_CELLlSetPriority(sButtonSprites->sprites[i], -1, 1);
        cell++;
    }
    DWCi_CELLlSetPosition(sButtonSprites->sprites[0], -1, x0, y0);
    DWCi_CELLlSetPosition(sButtonSprites->sprites[1], -1, x1, y0);
    DWCi_CELLlSetPosition(sButtonSprites->sprites[2], -1, x0, y1);
    DWCi_CELLlSetPosition(sButtonSprites->sprites[3], -1, x1, y1);
}

void DWCi_FOCUSlDelete(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sButtonSprites->sprites[i] != NULL) {
            DWCi_CELLlDelete(sButtonSprites->sprites[i]);
            sButtonSprites->sprites[i] = NULL;
        }
    }
}
