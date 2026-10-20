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

void func_ov182_021ab7c8(void) {
    sButtonSprites = DWCUtilHeap_AllocClear(sizeof(DWCUtilButtonSprites), 4);
}

void func_ov182_021ab7dc(void) {
    func_ov182_021ab8e4();
    DWCUtilHeap_FreeAndNull(&sButtonSprites);
}

void func_ov182_021ab7f0(int pair, int x0, int x1, int y) {
    int i;

    func_ov182_021ab8e4();
    for (i = 0; i < 2; i++) {
        sButtonSprites->sprites[i] = func_ov182_021bb97c(0, sPairCells[pair][i], TRUE);
        func_ov182_021bb878(sButtonSprites->sprites[i], -1, 1);
    }
    func_ov182_021bb7e0(sButtonSprites->sprites[0], -1, x0, y);
    func_ov182_021bb7e0(sButtonSprites->sprites[1], -1, x1, y);
}

void func_ov182_021ab85c(int x0, int x1, int y0, int y1) {
    int cell = 6;
    int i;

    func_ov182_021ab8e4();
    for (i = 0; i < 4; i++) {
        sButtonSprites->sprites[i] = func_ov182_021bb97c(0, cell, TRUE);
        func_ov182_021bb878(sButtonSprites->sprites[i], -1, 1);
        cell++;
    }
    func_ov182_021bb7e0(sButtonSprites->sprites[0], -1, x0, y0);
    func_ov182_021bb7e0(sButtonSprites->sprites[1], -1, x1, y0);
    func_ov182_021bb7e0(sButtonSprites->sprites[2], -1, x0, y1);
    func_ov182_021bb7e0(sButtonSprites->sprites[3], -1, x1, y1);
}

void func_ov182_021ab8e4(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sButtonSprites->sprites[i] != NULL) {
            func_ov182_021bb690(sButtonSprites->sprites[i]);
            sButtonSprites->sprites[i] = NULL;
        }
    }
}
