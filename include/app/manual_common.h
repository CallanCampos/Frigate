#ifndef POKEBW2_APP_MANUAL_COMMON_H
#define POKEBW2_APP_MANUAL_COMMON_H

// What the manual's screens share (overlay 319), named after the ROM's "manual_common.c"

#include "types.h"
#include "app/manual_data.h"
#include "app/manual_graphic.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "struct_decls.h"
#include "system/gf_font.h"
#include "system/printsys.h"

typedef struct ManualTouchBar ManualTouchBar;

typedef struct {
    GameData *gameData;
    HeapID heapId;
    ManualGraphic *graphic;
    Font *font;
    // The queues of the explanation's title, of the buttons and the explanation's text, and of the touch bar
    PrintQueue *titleQueue;
    PrintQueue *queue;
    PrintQueue *touchBarQueue;
    MsgData *msgSystem;
    MsgData *msgManual;
    ArcTool *arc;
    ArcTool *imageArc;
    ManualData *data;
    // The picture on the upper screen, or MANUAL_IMAGE_NONE
    u16 image;
    ManualTouchBar *touchBar;
} ManualCommon;

// A window of the lower screen, and where its text goes
typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
    u8 fromEnd;
    u8 textX;
    u8 textY;
} ManualWindowSetup;

ManualCommon *ManualCommon_Create(GameData *gameData, HeapID heapId);
void ManualCommon_Free(ManualCommon *common);
void ManualCommon_Main(ManualCommon *common);
// Shows a picture of ARCID_MANUAL_IMAGE on the upper screen, or clears it with MANUAL_IMAGE_NONE
void ManualCommon_LoadImage(ManualCommon *common, u16 image);

#endif // POKEBW2_APP_MANUAL_COMMON_H
