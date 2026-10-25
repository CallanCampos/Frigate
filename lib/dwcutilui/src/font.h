#ifndef POKEBW2_DWCUTIL_FONT_H
#define POKEBW2_DWCUTIL_FONT_H

#include "types.h"
#include "cell.h"
#include "nnsys/g2d.h"
#include "struct_decls.h"

// The connection utility's text: its two fonts, read from the resource archive, and the canvases they draw into,
// either BG 0 of a screen or a run of OBJ characters. A screen is 1 for the sub engine's and anything else for the
// main engine's.
// The file is the DWC utility library's font.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that
// library's objects (layout/symbols_sdk.txt) shows it.

struct DWCUtilText {
    NNSG2dCharCanvas canvas;
    NNSG2dTextCanvas text;
    // A BG canvas's characters, or the OBJ characters' block of VRAM
    void *buffer;
    union {
        // The task that uploads a BG canvas's characters
        DWCUtilTask *task;
        struct {
            u16 charName;
            u8 width;
            u8 height;
        } obj;
    } u;
};

void DWCi_FNTlInit(void);
void DWCi_FNTlEnd(void);
// A canvas of OBJ characters, width by height characters, on the screen, and the number of OAM entries it needs
DWCUtilText *DWCi_FNTlFormEx(int screen, int width, int height, s16 fromFront, int *objCount, int font);
void DWCi_FNTlDelete(DWCUtilText *text);
// The screen's BG 0 canvas, uploaded at the next VBlank after each DWCUtilText_MarkDirty
DWCUtilText *DWCi_FNTlFormBgEx(int screen, int font);
void DWCUtilText_UploadTask(DWCUtilTask *task, void *arg);
void DWCi_FNTlDeleteBg(DWCUtilText *text);
void DWCi_FNTlDeleteBgLcd(int screen);
void DWCi_FNTlDraw(DWCUtilText *text, int x, int y, int cl, u32 flags, const void *str);
void DWCi_FNTlDrawCharEx(DWCUtilText *text, int x, int y, int cl, u16 ch, int font);
// Draws the string with each character centered in a cell of cellWidth pixels
void DWCi_FNTlDrawStringEx(DWCUtilText *text, int x, int y, int cl, int cellWidth, const u16 *str, int font);
void DWCi_FNTlDrawRect(DWCUtilText *text, int x, int y, int w, int h, int cl, u32 flags, const void *str);
void DWCi_FNTlClear(DWCUtilText *text, int cl);
// Shows the OBJ canvas at x, y in the run of OAM entries
void DWCi_FNTlDisp(DWCUtilText *text, int x, int y, DWCUtilOamNode *node, int priority);
void DWCUtilText_MarkDirty(DWCUtilText *text);

#endif // POKEBW2_DWCUTIL_FONT_H
