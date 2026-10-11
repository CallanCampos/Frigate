#ifndef POKEBW2_APP_MEDAL_INFO_VIEWER_H
#define POKEBW2_APP_MEDAL_INFO_VIEWER_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"

// The viewer of overlay 187's medal screens (medal_info_viewer.c): one medal, or a list of the medals of the visitors
// that a beacon log holds, which the arrow buttons step through

typedef struct MedalInfoViewer MedalInfoViewer;

// The viewer's modes
#define MEDAL_INFO_VIEWER_MODE_BACK 0
#define MEDAL_INFO_VIEWER_MODE_BACK_OR_FIELD 1
#define MEDAL_INFO_VIEWER_MODE_LIST 2

// The medal id of an entry that holds no medal
#define MEDAL_INFO_NO_MEDAL 0xff

// What the viewer shows for one visitor. In a list, index is the entry's place in the beacon log
typedef struct {
    u16 medal;
    StrBuf *name;
    u16 count;
    u8 rank;
    // The date, as the visitor's data packs it
    u8 year;
    u8 month;
    u8 day;
    u16 index;
} MedalInfoEntry;

typedef struct {
    HeapID heapId;
    MedalInfoEntry *entries;
    u32 count;
    // The entry shown first
    u32 startIndex;
    u32 mode;
    // The entry that was shown last, set when the viewer closes
    u32 selected;
} MedalInfoViewerParam;

// Creates the viewer, which shows the screen at once
MedalInfoViewer *MedalInfoViewer_Create(MedalInfoViewerParam *param);
// Updates it each frame. Returns whether it is done
BOOL MedalInfoViewer_Main(MedalInfoViewer *viewer);
// Frees it and returns how it was left, 0 for its back button and 1 for the exit to the field
u32 MedalInfoViewer_Delete(MedalInfoViewer *viewer);

#endif // POKEBW2_APP_MEDAL_INFO_VIEWER_H
