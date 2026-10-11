#ifndef POKEBW2_APP_MARINE_TUBE_BOARD_H
#define POKEBW2_APP_MARINE_TUBE_BOARD_H

// The Marine Tube's board. The ROM names marine_tube_board_graphic.c; marine_tube_board.c (the proc) and
// marine_tube_board_message.c (its message window and printer) are guesses

#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"
#include "system/app_printsys_common.h"
#include "system/printsys.h"

// The Marine Tube's board, overlay 318 (marine_tube_board.c, marine_tube_board_message.c and
// marine_tube_board_graphic.c): a message on the main screen, which A, B or a touch closes, over the sub screen's
// C-Gear background
#define OVERLAY_MARINE_TUBE_BOARD OVERLAY_ID(318)

// The BG of the message window, and its frame
#define MARINE_TUBE_BOARD_BG_MESSAGE 5
#define MARINE_TUBE_BOARD_FRAME_CHAR 1
#define MARINE_TUBE_BOARD_FRAME_PALETTE 14

typedef struct {
    GameSystem *gsys;
    // Which board it is, which also picks its message
    u16 board;
} MarineTubeBoardParam;

typedef struct MarineTubeBoardGraphic MarineTubeBoardGraphic;

// A window that prints a message
typedef struct {
    MsgData *msgData;
    Font *font;
    BmpWin *window2;
    u8 unk0C;
    PrintQueue *printQueue;
    BmpWin *window;
    WordSet *wordSet;
    StrBuf *strBuf;
    // The background color index
    u16 color;
} MarineTubeBoardMsgWin;

// What prints a message a character at a time
typedef struct {
    PrintStream *stream;
    TCBExManager *tcbMgr;
    u32 unk08;
    u16 unk0C;
    u16 unk0E;
    AppPrintsysCommon printCommon;
    KeyCursor *keyCursor;
} MarineTubeBoardPrinter;

extern const GameProcFunctions MARINE_TUBE_BOARD_PROC_FUNCTIONS;

// marine_tube_board_message.c
void MarineTubeBoardMsgWin_Init(MarineTubeBoardMsgWin *win, Font *font, MsgData *msgData, PrintQueue *printQueue,
                                WordSet *wordSet, HeapID heapId);
void MarineTubeBoardMsgWin_Free(MarineTubeBoardMsgWin *win);
void MarineTubeBoardPrinter_Init(MarineTubeBoardPrinter *printer, HeapID heapId);
void MarineTubeBoardPrinter_Free(MarineTubeBoardPrinter *printer);
void MarineTubeBoardPrinter_Print(MarineTubeBoardPrinter *printer, MarineTubeBoardMsgWin *win, u32 messageId,
                                  HeapID heapId);
// Returns TRUE once the message is all shown and the player has seen it
BOOL MarineTubeBoardPrinter_Update(MarineTubeBoardPrinter *printer, MarineTubeBoardMsgWin *win);

// marine_tube_board_graphic.c
MarineTubeBoardGraphic *MarineTubeBoardGraphic_Create(u32 layout, HeapID heapId);
void MarineTubeBoardGraphic_Free(MarineTubeBoardGraphic *graphic);
void MarineTubeBoardGraphic_Update(MarineTubeBoardGraphic *graphic);

#endif // POKEBW2_APP_MARINE_TUBE_BOARD_H
