#ifndef POKEBW2_APP_UI_PRINT_MSG_H
#define POKEBW2_APP_UI_PRINT_MSG_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "system/printsys.h"

// print_msg.c, a guessed name: printing a string into a window through the print queue

// How a string is aligned at its x
#define PRINT_ALIGN_LEFT 0
#define PRINT_ALIGN_RIGHT 1
#define PRINT_ALIGN_CENTER 2

// Prints a string into a window through the queue, aligned at x by a PRINT_ALIGN_*
void PrintStrAligned(PrintWindow *window, PrintQueue *queue, u16 x, u16 y, const StrBuf *strbuf, Font *font,
                         u16 color, u32 align);

// Prints "number1/number2" with the slash centered at x
void PrintFraction(PrintWindow *window, PrintQueue *queue, Font *font, u16 x, u16 y, u16 color, s32 number1,
                         s32 number2, HeapID heapId);

#endif // POKEBW2_APP_UI_PRINT_MSG_H
