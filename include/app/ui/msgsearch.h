#ifndef POKEBW2_APP_UI_MSGSEARCH_H
#define POKEBW2_APP_UI_MSGSEARCH_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"

// msgsearch.c: A search of message files for the strings that start with a prefix
typedef struct MsgSearch MsgSearch;

// A string found: its message file and its line
typedef struct {
    u32 file;
    u32 line;
} MsgSearchResult;

MsgSearch *MsgSearch_Create(MsgData **files, u32 count, HeapID heapId);
void MsgSearch_Free(MsgSearch *search);
// Fills results with the strings of a file that start with prefix, up to max, and returns how many it found
u32 MsgSearch_Find(MsgSearch *search, u32 file, u32 unused, StrBuf *prefix, MsgSearchResult *results, u32 max);

#endif // POKEBW2_APP_UI_MSGSEARCH_H
