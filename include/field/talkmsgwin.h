#ifndef POKEBW2_FIELD_TALKMSGWIN_H
#define POKEBW2_FIELD_TALKMSGWIN_H

#include "types.h"
#include "gfl/bmpwin.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/printsys.h"

// The field's talk windows (talkmsgwin.c, overlay 36): 32 numbered windows, framed in a BG's screen, that print a
// string and can point a tail at a spot in the 3D scene, and the frame drawn around other windows. The field's message
// BG (field_msgbg.c) uses them, and other overlays draw the frame around their own windows with overlay 36 loaded.
// loadTexture_wrapper and the ActorMsgWin_ functions are swan's names; the others are unnamed

// The frame's styles, drawn with the frame's characters: rounded corners, spikes, and bumps
enum {
    TALKMSG_WIN_FRAME_NORMAL,
    TALKMSG_WIN_FRAME_SPIKY,
    TALKMSG_WIN_FRAME_BUMPY,
};

// Where a window's tail leaves its frame. Given to ActorMsgWin_CreateAtTop and ActorMsgWin_CreateAtBottom, NONE picks
// the side the target is on
enum {
    TALKMSG_WIN_TAIL_NONE,
    TALKMSG_WIN_TAIL_TOP,
    TALKMSG_WIN_TAIL_BOTTOM,
    TALKMSG_WIN_TAIL_LEFT,
    TALKMSG_WIN_TAIL_RIGHT,
    TALKMSG_WIN_TAIL_TOP_LEFT,
    TALKMSG_WIN_TAIL_TOP_RIGHT,
    TALKMSG_WIN_TAIL_BOTTOM_LEFT,
    TALKMSG_WIN_TAIL_BOTTOM_RIGHT,
};

typedef struct {
    HeapID heapId;
    // The camera the tails are projected with; NULL when no window has a tail
    G3DCamera *camera;
    Font *font;
    // The frame's first character in the BG
    u16 frameChar;
    u32 bg;
    // The frame's palette and the windows' palette
    u8 framePalette;
    u8 winPalette;
} TalkMsgWinSetup;

// Loads the frame's characters and palette to the BG and allocates the tails' texture and palette in VRAM
TalkMsgWinSys *TalkMsgWinSys_Create(const TalkMsgWinSetup *setup);
// Loads the tails' texture and palette
void loadTexture_wrapper(TalkMsgWinSys *sys);
// Once a frame: runs the windows' print streams and opens, moves and closes them
void TalkMsgWinSys_Update(TalkMsgWinSys *sys);
void TalkMsgWin_EndUpdate(TalkMsgWinSys *sys);
// Draws the tails and the windows opening and closing
void TalkMsgWinSys_Draw(TalkMsgWinSys *sys);
void TalkMsgWinSys_Free(TalkMsgWinSys *sys);
// Creates window index at a place of the screen, without a tail, and prints strbuf at textX and textY when opened. A
// frameType is a TALKMSG_WIN_FRAME_ style
void TalkMsgWin_SetupPlainWindow(TalkMsgWinSys *sys, u32 index, StrBuf *strbuf, u8 x, u8 y, u8 width, u8 height,
                                 u32 unused, u32 frameType, u32 wait, u8 textX, u8 textY);
// Create window index across the top or the bottom of the screen with a tail pointing at target, or at the bottom when
// target is in the top half of the screen. tail picks the tail's corner, or TALKMSG_WIN_TAIL_NONE for the side of the
// screen target is on
void ActorMsgWin_CreateAtTop(TalkMsgWinSys *sys, u32 index, const VecFx32 *target, StrBuf *strbuf, u8 unused,
                             u32 frameType, u32 tail, u32 wait);
void ActorMsgWin_CreateAtBottom(TalkMsgWinSys *sys, u32 index, const VecFx32 *target, StrBuf *strbuf, u8 unused,
                                u32 frameType, u32 tail, u32 wait);
void ActorMsgWin_CreateAtAutoPos(TalkMsgWinSys *sys, u32 index, const VecFx32 *target, StrBuf *strbuf, u8 unused,
                                 u32 frameType, u32 wait);
// Deletes window index, opens it and closes it
void TalkMsgWin_Release(TalkMsgWinSys *sys, u32 index);
void TalkMsgWin_Open(TalkMsgWinSys *sys, u32 index);
void TalkMsgWin_Close(TalkMsgWinSys *sys, u32 index);
// Whether window index is free, and whether it is open
BOOL TalkMsgWin_IsWindowFree(TalkMsgWinSys *sys, u32 index);
BOOL TalkMsgWin_IsWindowShown(TalkMsgWinSys *sys, u32 index);
PrintStream *TalkMsgWin_GetPrintStream(TalkMsgWinSys *sys, u32 index);
BmpWin *TalkMsgWin_GetBmpWin(TalkMsgWinSys *sys, u32 index);
// Prints another string in window index, or only stops the printing when strbuf is NULL
void TalkMsgWin_Reprint(TalkMsgWinSys *sys, u32 index, StrBuf *strbuf, u32 wait);
// Creates a window with the talk window's frame, draws the frame around a window, and clears it, then also frees the
// window
BmpWin *TalkMsgWin_AddFrameWindow(TalkMsgWinSys *sys, u8 x, u8 y, u8 width, u8 height, u32 frameType);
void TalkMsgWin_DrawWindowFrame(TalkMsgWinSys *sys, BmpWin *window, u32 frameType);
void TalkMsgWin_ClearWindowFrame(TalkMsgWinSys *sys, BmpWin *window);
void TalkMsgWin_DeleteFrameWindow(TalkMsgWinSys *sys, BmpWin *window);
// Loads the frame's characters and palette again
void TalkMsgWin_ReloadFrameGraphics(TalkMsgWinSys *sys);

#endif // POKEBW2_FIELD_TALKMSGWIN_H
