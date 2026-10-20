#include "battle/b_bag_anm.h"
#include "battle/b_bag_item.h"
#include "battle/b_bag_main.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "system/bgwinfrm.h"

// The battle bag's buttons (b_bag_anm.c, our guess after the ROM's b_bag_main.c and Diamond and Pearl's
// b_bag_anm.c): the 14 window frames on BG 6, from members of archive 98, which pages show which, their palette rows
// for normal, pressed and disabled, and the press animation that draws a button pressed and then released

// A button's member of archive 98 and its size in tiles
typedef struct {
    u16 member;
    u8 width;
    u8 height;
} BBagButton;

// The buttons: 0-3 the pockets, 4-9 the item slots, 10 back, 11 and 12 the previous and next page, 13 the wide bar
static const BBagButton data_ov286_021f6e90[14] = {
    { 5, 16, 8 }, { 6, 16, 8 }, { 7, 16, 8 }, { 8, 16, 8 }, { 9, 16, 6 }, { 9, 16, 6 }, { 9, 16, 6 },
    { 9, 16, 6 }, { 9, 16, 6 }, { 9, 16, 6 }, { 12, 5, 5 }, { 10, 5, 5 }, { 11, 5, 5 }, { 13, 25, 5 },
};

void BBagAnm_CreateButtons(BBagWork *work) {
    ArcTool *arc;
    u32 i;

    work->buttons = BGWinFrame_Create(2, 14, work->param->heapId);
    arc = GFL_ArcSysCreateFileHandle(ARCID_B_BAG, HEAPID_TAIL(work->param->heapId));
    for (i = 0; i < 14; i++) {
        BGWinFrame_InitFrame(work->buttons, i, 6, data_ov286_021f6e90[i].width, data_ov286_021f6e90[i].height);
        BGWinFrame_SetArea(work->buttons, i, 0, 64, 0, 64);
        BGWinFrame_LoadScreenArc(work->buttons, i, arc, data_ov286_021f6e90[i].member, TRUE);
    }
    GFL_ArcToolFree(arc);

    BGWinFrame_Put(work->buttons, 0, 0, 1);
    BGWinFrame_Put(work->buttons, 1, 0, 10);
    BGWinFrame_Put(work->buttons, 2, 16, 1);
    BGWinFrame_Put(work->buttons, 3, 16, 10);
    BGWinFrame_Put(work->buttons, 4, 32, 1);
    BGWinFrame_Put(work->buttons, 5, 48, 1);
    BGWinFrame_Put(work->buttons, 6, 32, 7);
    BGWinFrame_Put(work->buttons, 7, 48, 7);
    BGWinFrame_Put(work->buttons, 8, 32, 13);
    BGWinFrame_Put(work->buttons, 9, 48, 13);
    BGWinFrame_Put(work->buttons, 11, 32, 19);
    BGWinFrame_Put(work->buttons, 12, 37, 19);
}

void BBagAnm_DeleteButtons(BBagWork *work) {
    BGWinFrame_Delete(work->buttons);
}

// Shows the buttons of a page, each in its palette row: 1 or 4 normal, 3 or 6 disabled
void BBagAnm_PutPageButtons(BBagWork *work, u8 page) {
    u32 i;

    for (i = 0; i < 14; i++) {
        BGWinFrame_Hide(work->buttons, i);
    }

    switch (page) {
    case 0:
        BGWinFrame_Put(work->buttons, 10, 27, 19);
        BGWinFrame_Put(work->buttons, 13, 1, 19);
        if (work->lastItem == 0) {
            BGWinFrame_SetPalette(work->buttons, 13, 0, 0, 25, 5, 3);
        } else {
            BGWinFrame_SetPalette(work->buttons, 13, 0, 0, 25, 5, 1);
        }
        BGWinFrame_Show(work->buttons, 0);
        BGWinFrame_Show(work->buttons, 1);
        BGWinFrame_Show(work->buttons, 2);
        BGWinFrame_Show(work->buttons, 3);
        BGWinFrame_Show(work->buttons, 10);
        BGWinFrame_Show(work->buttons, 13);
        break;
    case 1:
        BGWinFrame_Put(work->buttons, 10, 59, 19);
        for (i = 0; i < 6; i++) {
            if (BBagItem_GetSlotItem(work, i) == 0) {
                BGWinFrame_SetPalette(work->buttons, i + 4, 0, 0, data_ov286_021f6e90[i + 4].width,
                                      data_ov286_021f6e90[i + 4].height, 6);
            } else {
                BGWinFrame_SetPalette(work->buttons, i + 4, 0, 0, data_ov286_021f6e90[i + 4].width,
                                      data_ov286_021f6e90[i + 4].height, 4);
            }
        }
        if (work->lastPage[work->pocket] == 0) {
            BGWinFrame_SetPalette(work->buttons, 11, 0, 0, 5, 5, 3);
            BGWinFrame_SetPalette(work->buttons, 12, 0, 0, 5, 5, 3);
        } else {
            BGWinFrame_SetPalette(work->buttons, 11, 0, 0, 5, 5, 1);
            BGWinFrame_SetPalette(work->buttons, 12, 0, 0, 5, 5, 1);
        }
        BGWinFrame_Show(work->buttons, 4);
        BGWinFrame_Show(work->buttons, 5);
        BGWinFrame_Show(work->buttons, 6);
        BGWinFrame_Show(work->buttons, 7);
        BGWinFrame_Show(work->buttons, 8);
        BGWinFrame_Show(work->buttons, 9);
        BGWinFrame_Show(work->buttons, 11);
        BGWinFrame_Show(work->buttons, 12);
        BGWinFrame_Show(work->buttons, 10);
        break;
    case 2:
        BGWinFrame_SetPalette(work->buttons, 13, 0, 0, 25, 5, 1);
        BGWinFrame_Put(work->buttons, 10, 27, 51);
        BGWinFrame_Put(work->buttons, 13, 1, 51);
        BGWinFrame_Show(work->buttons, 10);
        BGWinFrame_Show(work->buttons, 13);
        break;
    }
}

void BBagAnm_StartButtonAnm(BBagWork *work, u8 button) {
    work->animSeq = 0;
    work->animCount = 0;
    work->animButton = button;
    work->animActive = TRUE;
}

// Draws the pressed button in the pressed palette row (2, or 5 for the pockets and items) for 5 frames, then in the
// normal one for 2
void BBagAnm_MainButtonAnm(BBagWork *work) {
    u8 palette;

    if (work->animActive == FALSE) {
        return;
    }

    switch (work->animSeq) {
    case 0:
        if (work->animButton >= 10) {
            palette = 2;
        } else {
            palette = 5;
        }
        BGWinFrame_SetPalette(work->buttons, work->animButton, 0, 0, data_ov286_021f6e90[work->animButton].width,
                              data_ov286_021f6e90[work->animButton].height, palette);
        BGWinFrame_Show(work->buttons, work->animButton);
        work->animSeq++;
        break;
    case 1:
        if (work->animCount == 4) {
            if (work->animButton >= 10) {
                palette = 1;
            } else {
                palette = 4;
            }
            BGWinFrame_SetPalette(work->buttons, work->animButton, 0, 0, data_ov286_021f6e90[work->animButton].width,
                                  data_ov286_021f6e90[work->animButton].height, palette);
            BGWinFrame_Show(work->buttons, work->animButton);
            work->animCount = 0;
            work->animSeq++;
        } else {
            work->animCount++;
        }
        break;
    case 2:
        if (work->animCount == 1) {
            work->animCount = 0;
            work->animSeq = 0;
            work->animActive = FALSE;
        } else {
            work->animCount++;
        }
        break;
    }
}
