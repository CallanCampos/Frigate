#include "battle/b_plist_anm.h"
#include "battle/b_plist_main.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "system/palanm.h"

// The battle party list's buttons (b_plist_anm.c, named by the ROM's embedded string): the pieces of each button in
// each of its states, cut from the list's tilemaps, drawn onto the sub screen's BG 2, and the press animation that
// draws a button pressed and then released

// Where a button is on the screen and its size, in tiles
typedef struct {
    u8 x;
    u8 y;
    u8 width;
    u8 height;
} BPlistButtonRect;

static void BPlistAnm_CopyScrnRect(u16 *dst, const u16 *src, u8 x, u8 y, u8 width, u8 height);
static u16 *BPlistAnm_GetButtonScrn(BPlistWork *work, u8 button, u8 state, u8 mode);
static void BPlistAnm_MakeButtonScrn(BPlistWork *work, u16 *buf, u8 button, u8 state, u8 mode);
static void BPlistAnm_PutButton(BPlistWork *work, u8 button, u8 state, u8 mode);
static u8 BPlistAnm_CanSwitch(BPlistWork *work);

static const BPlistButtonRect data_ov287_021fadac[32] = {
    { 0x00, 0x00, 0x10, 0x06 }, { 0x10, 0x01, 0x10, 0x06 }, { 0x00, 0x06, 0x10, 0x06 }, { 0x10, 0x07, 0x10, 0x06 },
    { 0x00, 0x0c, 0x10, 0x06 }, { 0x10, 0x0d, 0x10, 0x06 }, { 0x1b, 0x13, 0x05, 0x05 }, { 0x00, 0x01, 0x1e, 0x11 },
    { 0x00, 0x13, 0x0d, 0x05 }, { 0x0c, 0x13, 0x0d, 0x05 }, { 0x0d, 0x13, 0x0d, 0x05 }, { 0x0c, 0x13, 0x0d, 0x05 },
    { 0x00, 0x13, 0x05, 0x05 }, { 0x05, 0x13, 0x05, 0x05 }, { 0x00, 0x06, 0x10, 0x06 }, { 0x10, 0x06, 0x10, 0x06 },
    { 0x00, 0x0c, 0x10, 0x06 }, { 0x10, 0x0c, 0x10, 0x06 }, { 0x00, 0x06, 0x10, 0x06 }, { 0x10, 0x06, 0x10, 0x06 },
    { 0x00, 0x0c, 0x10, 0x06 }, { 0x10, 0x0c, 0x10, 0x06 }, { 0x00, 0x06, 0x10, 0x06 }, { 0x10, 0x06, 0x10, 0x06 },
    { 0x00, 0x0c, 0x10, 0x06 }, { 0x10, 0x0c, 0x10, 0x06 }, { 0x08, 0x12, 0x10, 0x06 }, { 0x00, 0x13, 0x1a, 0x05 },
    { 0x0b, 0x13, 0x05, 0x02 }, { 0x10, 0x13, 0x05, 0x02 }, { 0x0b, 0x15, 0x05, 0x02 }, { 0x10, 0x15, 0x05, 0x02 },
};

void BPlistAnm_CutButtonScrn(BPlistWork *work, const void *scrn) {
    BPlistAnm_CopyScrnRect(work->plateScrn[0][0], scrn, 0, 0, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[0][1], scrn, 0, 6, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[0][2], scrn, 0, 12, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[0][3], scrn, 0, 18, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[1][0], scrn, 16, 0, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[1][1], scrn, 16, 6, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[1][2], scrn, 16, 12, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[1][3], scrn, 16, 18, 16, 6);
    BPlistAnm_CopyScrnRect(work->moveButtonScrn[0], scrn, 0, 39, 13, 5);
    BPlistAnm_CopyScrnRect(work->moveButtonScrn[1], scrn, 0, 44, 13, 5);
    BPlistAnm_CopyScrnRect(work->moveButtonScrn[2], scrn, 13, 39, 13, 5);
    BPlistAnm_CopyScrnRect(work->moveButtonScrn[3], scrn, 13, 44, 13, 5);
    BPlistAnm_CopyScrnRect(work->button12Scrn[0], scrn, 0, 49, 5, 5);
    BPlistAnm_CopyScrnRect(work->button12Scrn[1], scrn, 5, 49, 5, 5);
    BPlistAnm_CopyScrnRect(work->button12Scrn[2], scrn, 10, 49, 5, 5);
    BPlistAnm_CopyScrnRect(work->button12Scrn[3], scrn, 15, 49, 5, 5);
    BPlistAnm_CopyScrnRect(work->button13Scrn[0], scrn, 0, 54, 5, 5);
    BPlistAnm_CopyScrnRect(work->button13Scrn[1], scrn, 5, 54, 5, 5);
    BPlistAnm_CopyScrnRect(work->button13Scrn[2], scrn, 10, 54, 5, 5);
    BPlistAnm_CopyScrnRect(work->button13Scrn[3], scrn, 15, 54, 5, 5);
    BPlistAnm_CopyScrnRect(work->button6Scrn[0], scrn, 26, 24, 5, 5);
    BPlistAnm_CopyScrnRect(work->button6Scrn[1], scrn, 26, 29, 5, 5);
    BPlistAnm_CopyScrnRect(work->button6Scrn[2], scrn, 26, 34, 5, 5);
    BPlistAnm_CopyScrnRect(work->button6Scrn[3], scrn, 26, 39, 5, 5);
    BPlistAnm_CopyScrnRect(work->button27Scrn[0], scrn, 0, 24, 26, 5);
    BPlistAnm_CopyScrnRect(work->button27Scrn[1], scrn, 0, 29, 26, 5);
    BPlistAnm_CopyScrnRect(work->button27Scrn[2], scrn, 0, 34, 26, 5);
    BPlistAnm_CopyScrnRect(work->button28Scrn[0], scrn, 0, 59, 5, 2);
    BPlistAnm_CopyScrnRect(work->button28Scrn[1], scrn, 5, 59, 5, 2);
    BPlistAnm_CopyScrnRect(work->button28Scrn[2], scrn, 10, 59, 5, 2);
}

void BPlistAnm_CutPageScrn(BPlistWork *work, const void *scrn) {
    BPlistAnm_CopyScrnRect(work->button7Scrn[0], scrn, 0, 0, 30, 17);
    BPlistAnm_CopyScrnRect(work->button7Scrn[1], scrn, 0, 17, 30, 17);
    BPlistAnm_CopyScrnRect(work->button7Scrn[2], scrn, 0, 34, 30, 17);
    BPlistAnm_CopyScrnRect(work->button14Scrn[0], scrn, 0, 51, 16, 6);
    BPlistAnm_CopyScrnRect(work->button14Scrn[1], scrn, 16, 51, 16, 6);
    BPlistAnm_CopyScrnRect(work->button14Scrn[2], scrn, 0, 57, 16, 6);
    BPlistAnm_CopyScrnRect(work->button14Scrn[3], scrn, 16, 57, 16, 6);
}

// Copies a width by height piece of a 32-tile-wide tilemap
static void BPlistAnm_CopyScrnRect(u16 *dst, const u16 *src, u8 x, u8 y, u8 width, u8 height) {
    u16 i, j;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            dst[i * width + j] = src[(y + i) * 32 + x + j];
        }
    }
}

static u16 *BPlistAnm_GetButtonScrn(BPlistWork *work, u8 button, u8 state, u8 mode) {
    switch (button) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        if (mode == 0) {
            return work->plateScrn[0][state];
        }
        return work->plateScrn[1][state];
    case 6:
        return work->button6Scrn[state];
    case 7:
        return work->button7Scrn[state];
    case 8:
    case 9:
    case 10:
    case 11:
        return work->moveButtonScrn[state];
    case 12:
        return work->button12Scrn[state];
    case 13:
        return work->button13Scrn[state];
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
        return work->button14Scrn[state];
    case 27:
        return work->button27Scrn[state];
    case 28:
    case 29:
    case 30:
    case 31:
        return work->button28Scrn[state];
    }
    return NULL;
}

static void BPlistAnm_MakeButtonScrn(BPlistWork *work, u16 *buf, u8 button, u8 state, u8 mode) {
    u16 *src;
    u8 width;
    int size;
    u8 idx;
    u16 tiles[2];
    u8 i, j;

    src = BPlistAnm_GetButtonScrn(work, button, state, mode);
    width = data_ov287_021fadac[button].width;
    size = width * data_ov287_021fadac[button].height;
    MI_CpuCopy8(src, buf, size * 2);

    switch (button) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        idx = BPlistMain_GetPartySlot(work, button);
        if (work->pokemon[idx].species == 0) {
            break;
        }
        if (work->pokemon[idx].isEgg) {
            tiles[0] = buf[width * 2 + 5];
            tiles[1] = buf[width * 3 + 5];
            for (i = 0; i < 2; i++) {
                for (j = 0; j < 9; j++) {
                    buf[(i + 2) * width + j + 6] = tiles[i];
                }
            }
        } else if (work->pokemon[idx].hp == 0) {
            for (i = 0; i < size; i++) {
                buf[i] = (buf[i] & 0xfff) | (((state & 1) + 7) << 12);
            }
        } else if (BPlistMain_IsPartnerSlot(work, idx) == TRUE) {
            for (i = 0; i < size; i++) {
                buf[i] = (buf[i] & 0xfff) | (((state & 1) + 5) << 12);
            }
        }
        break;
    }
}

static void BPlistAnm_PutButton(BPlistWork *work, u8 button, u8 state, u8 mode) {
    u16 *buf = GFL_HeapAllocate(HEAPID_TAIL(work->param->heapId),
                                data_ov287_021fadac[button].width * data_ov287_021fadac[button].height * 2, FALSE,
                                "b_plist_anm.c", 517);

    BPlistAnm_MakeButtonScrn(work, buf, button, state, mode);
    GFL_BGSysLoadScrAreaAll(6, buf, data_ov287_021fadac[button].x, data_ov287_021fadac[button].y,
                            data_ov287_021fadac[button].width, data_ov287_021fadac[button].height);
    GFL_BGSysQueueScrLoad(6);
    GFL_HeapFree(buf);
}

void BPlistAnm_StartButtonAnm(BPlistWork *work, u8 button) {
    work->animMode = 0;
    if (button <= 5 && BPlistMain_CheckPos(work, button) == 2) {
        work->animMode = 1;
    }
    work->animSeq = 0;
    work->animCount = 0;
    work->animButton = button;
    work->animActive = TRUE;
}

void BPlistAnm_MainButtonAnm(BPlistWork *work) {
    if (!work->animActive) {
        return;
    }

    switch (work->animSeq) {
    case 0:
        BPlistAnm_PutButton(work, work->animButton, 1, work->animMode);
        work->animSeq++;
        break;
    case 1:
        if (work->animCount == 4) {
            BPlistAnm_PutButton(work, work->animButton, 0, work->animMode);
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

void BPlistAnm_PutPageButtons(BPlistWork *work, u8 page) {
    u16 i;
    int state;

    switch (page) {
    case 0:
        for (i = 0; i < 6; i++) {
            state = BPlistMain_CheckPos(work, i);
            if (state == 0) {
                BPlistAnm_PutButton(work, i, 3, 1);
            } else if (state == 1) {
                BPlistAnm_PutButton(work, i, 0, 0);
            } else if (state == 2) {
                BPlistAnm_PutButton(work, i, 0, 1);
            }
        }
        if (work->param->unk1F == 1 || work->param->unk1F == 2) {
            BPlistAnm_PutReturnButton(work);
        } else {
            BPlistAnm_PutButton(work, 6, 0, 0);
        }
        break;
    case 8:
        for (i = 0; i < 6; i++) {
            state = BPlistMain_CheckPos(work, i);
            if (state == 0) {
                BPlistAnm_PutButton(work, i, 3, 1);
            } else if (state == 1) {
                BPlistAnm_PutButton(work, i, 0, 0);
            } else if (state == 2) {
                BPlistAnm_PutButton(work, i, 0, 1);
            }
        }
        BPlistAnm_PutButton(work, 6, 0, 0);
        break;
    case 1:
        BPlistAnm_PutButton(work, 6, 0, 0);
        BPlistAnm_PutButton(work, 7, 0, 0);
        if (work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].isEgg) {
            BPlistAnm_PutButton(work, 8, 3, 0);
            BPlistAnm_PutButton(work, 10, 3, 0);
        } else {
            BPlistAnm_PutButton(work, 8, 0, 0);
            BPlistAnm_PutButton(work, 10, 0, 0);
        }
        break;
    case 2:
        if (BPlistAnm_CanSwitch(work) == TRUE) {
            BPlistAnm_PutButton(work, 12, 0, 0);
            BPlistAnm_PutButton(work, 13, 0, 0);
        } else {
            BPlistAnm_PutButton(work, 12, 3, 0);
            BPlistAnm_PutButton(work, 13, 3, 0);
        }
        BPlistAnm_PutButton(work, 11, 0, 0);
        BPlistAnm_PutButton(work, 6, 0, 0);
        break;
    case 3:
        if (BPlistAnm_CanSwitch(work) == TRUE) {
            BPlistAnm_PutButton(work, 12, 0, 0);
            BPlistAnm_PutButton(work, 13, 0, 0);
        } else {
            BPlistAnm_PutButton(work, 12, 3, 0);
            BPlistAnm_PutButton(work, 13, 3, 0);
        }
        for (i = 0; i < 4; i++) {
            if (work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].moves[i].move != 0) {
                BPlistAnm_PutButton(work, i + 14, 0, 0);
            } else {
                BPlistAnm_PutButton(work, i + 14, 3, 0);
            }
        }
        BPlistAnm_PutButton(work, 9, 0, 0);
        BPlistAnm_PutButton(work, 6, 0, 0);
        break;
    case 4:
        BPlistAnm_PutButton(work, 6, 0, 0);
        for (i = 0; i < 4; i++) {
            if (work->param->slot == i) {
                BPlistAnm_PutButton(work, i + 28, 2, 0);
            } else {
                BPlistAnm_PutButton(work, i + 28, 0, 0);
            }
        }
        break;
    case 5:
        for (i = 0; i < 4; i++) {
            if (work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].moves[i].move != 0) {
                BPlistAnm_PutButton(work, i + 18, 0, 0);
            } else {
                BPlistAnm_PutButton(work, i + 18, 3, 0);
            }
        }
        BPlistAnm_PutButton(work, 6, 0, 0);
        break;
    case 6:
        BPlistAnm_PutButton(work, 22, 0, 0);
        BPlistAnm_PutButton(work, 23, 0, 0);
        BPlistAnm_PutButton(work, 24, 0, 0);
        BPlistAnm_PutButton(work, 25, 0, 0);
        BPlistAnm_PutButton(work, 26, 0, 0);
        BPlistAnm_PutButton(work, 6, 0, 0);
        break;
    case 7:
        BPlistAnm_PutButton(work, 27, 0, 0);
        BPlistAnm_PutButton(work, 6, 0, 0);
        break;
    }
}

// The caller passes the page, which this does not use
void BPlistAnm_RestorePalette(BPlistWork *work, u8 page) {
    PaletteFade_LoadData(work->paletteFade, work->savedPalette, PALFADE_BUFFER_SUB_BG, 0xc0,
                         sizeof(work->savedPalette));
}

// Whether at least two of the party can fight
static u8 BPlistAnm_CanSwitch(BPlistWork *work) {
    u16 i;
    u16 count = 0;

    for (i = 0; i < 6; i++) {
        if (work->pokemon[i].species != 0 && !work->pokemon[i].isEgg) {
            count++;
        }
    }
    return count >= 2;
}

void BPlistAnm_PutReturnButton(BPlistWork *work) {
    u8 pos1, pos2;

    if (BPlistMain_PopSwap(work, &pos1, &pos2, FALSE) == TRUE) {
        BPlistAnm_PutButton(work, 6, 0, 0);
    } else {
        BPlistAnm_PutButton(work, 6, 3, 0);
    }
}
