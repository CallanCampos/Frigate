// msgsearch.c: finding the lines of a message file that start with a prefix, as the easy chat input searches its words

#include "app/ui/msgsearch.h"
#include "types.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/mi.h"

struct MsgSearch {
    StrBuf *line;
    u32 count;
    MsgData *files[];
};

static BOOL StartsWith(const StrBuf *str, const StrBuf *prefix, u32 length);

MsgSearch *MsgSearch_Create(MsgData **files, u32 count, HeapID heapId) {
    u32 size = sizeof(MsgSearch) + count * sizeof(MsgData *);
    MsgSearch *search = GFL_HeapAllocate(heapId, size, FALSE, "msgsearch.c", 64);
    u32 i;

    sys_memset(search, 0, size);
    search->count = count;
    for (i = 0; i < count; i++) {
        search->files[i] = files[i];
    }
    search->line = GFL_StrBufCreate(128, heapId);
    return search;
}

void MsgSearch_Free(MsgSearch *search) {
    GFL_StrBufFree(search->line);
    GFL_HeapFree(search);
}

u32 MsgSearch_Find(MsgSearch *search, u32 file, u32 unused, StrBuf *prefix, MsgSearchResult *results, u32 max) {
    u32 found = 0;
    u32 line;
    u32 stored;
    u32 lineCount;
    MsgData *msgData;
    u32 length = GFL_StrBufGetCharCount(prefix);

    sys_memset32(0xffffffff, results, max * sizeof(MsgSearchResult));
    if (length != 0) {
        lineCount = GFL_MsgDataGetLineCount(search->files[file]);
        msgData = search->files[file];
        line = 0;
        stored = 0;
        for (;;) {
            if (line >= lineCount || stored >= max) {
                break;
            }
            GFL_MsgDataLoadStrbuf(msgData, line, search->line);
            if (StartsWith(prefix, search->line, GFL_StrBufGetCharCount(prefix))) {
                if (stored < max) {
                    results[stored].file = file;
                    results[stored].line = line;
                    stored++;
                }
                found++;
            }
            line++;
        }
    }
    return found;
}

static BOOL StartsWith(const StrBuf *str, const StrBuf *prefix, u32 length) {
    const u16 *a;
    const u16 *b;
    u32 i;

    if (GFL_StrBufGetCharCount(str) < length) {
        return FALSE;
    }
    if (GFL_StrBufGetCharCount(prefix) < length) {
        return FALSE;
    }
    a = GFL_StrBufGetStringPtr(str);
    b = GFL_StrBufGetStringPtr(prefix);
    for (i = 0; i < length; i++) {
        if (*a++ != *b++) {
            return FALSE;
        }
    }
    return TRUE;
}
