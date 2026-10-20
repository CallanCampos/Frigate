#ifndef POKEBW2_DWCUTIL_DWC_UTIL_TEXT_H
#define POKEBW2_DWCUTIL_DWC_UTIL_TEXT_H

#include "types.h"
#include "dwc_util_oam.h"
#include "nnsys/g2d.h"
#include "struct_decls.h"

// The connection utility's text: its two fonts, read from the resource archive, and the canvases they draw into,
// either BG 0 of a screen or a run of OBJ characters. A screen is 1 for the sub engine's and anything else for the
// main engine's.
// The ROM does not embed this source file's name; dwc_util_text.c is descriptive.

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

void DWCUtilText_Init(void);
void DWCUtilText_Exit(void);
// A canvas of OBJ characters, width by height characters, on the screen, and the number of OAM entries it needs
DWCUtilText *DWCUtilText_CreateObj(int screen, int width, int height, s16 fromFront, int *objCount, int font);
void DWCUtilText_DestroyObj(DWCUtilText *text);
// The screen's BG 0 canvas, uploaded at the next VBlank after each DWCUtilText_MarkDirty
DWCUtilText *DWCUtilText_CreateBg(int screen, int font);
void DWCUtilText_UploadTask(DWCUtilTask *task, void *arg);
void DWCUtilText_DestroyBg(DWCUtilText *text);
void DWCUtilText_DestroyScreenBg(int screen);
void DWCUtilText_Draw(DWCUtilText *text, int x, int y, int cl, u32 flags, const void *str);
void DWCUtilText_DrawChar(DWCUtilText *text, int x, int y, int cl, u16 ch, int font);
// Draws the string with each character centered in a cell of cellWidth pixels
void DWCUtilText_DrawCentered(DWCUtilText *text, int x, int y, int cl, int cellWidth, const u16 *str, int font);
void DWCUtilText_DrawRect(DWCUtilText *text, int x, int y, int w, int h, int cl, u32 flags, const void *str);
void DWCUtilText_Clear(DWCUtilText *text, int cl);
// Shows the OBJ canvas at x, y in the run of OAM entries
void DWCUtilText_ShowObj(DWCUtilText *text, int x, int y, DWCUtilOamNode *node, int priority);
void DWCUtilText_MarkDirty(DWCUtilText *text);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_TEXT_H
