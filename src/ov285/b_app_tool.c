#include "types.h"
#include "battle/b_app_tool.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "system/cursor_move.h"
#include "system/printsys.h"

BAppCursor *BAppCursor_Create(HeapID heapId) {
    BAppCursor *cursor = GFL_HeapAllocate(heapId, sizeof(BAppCursor), FALSE, "b_app_tool.c", 43);
    cursor->heapId = heapId;
    cursor->visible = TRUE;
    return cursor;
}

void BAppCursor_Delete(BAppCursor *cursor) {
    GFL_HeapFree(cursor);
}

void BAppCursor_CreateActors(BAppCursor *cursor, ClActUnit *unit, u32 chars, u32 palette, u32 cellAnims) {
    ClActorSetup setup;
    u32 i;

    setup.x = 0;
    setup.y = 0;
    setup.priority = 0;
    setup.bgPriority = 1;
    for (i = 0; i < 4; i++) {
        setup.sequence = i;
        cursor->corners[i] = ClActor_Create(unit, chars, palette, cellAnims, &setup, 1, cursor->heapId);
        ClActor_SetPalette(cursor->corners[i], 0, 1);
        ClActor_SetAutoAnim(cursor->corners[i], TRUE);
    }
}

void BAppCursor_DeleteActors(BAppCursor *cursor) {
    u32 i;

    for (i = 0; i < 4; i++) {
        ClActor_Delete(cursor->corners[i]);
    }
}

void BAppCursor_SetVisible(BAppCursor *cursor, BOOL visible) {
    u32 i;

    if (cursor->visible == visible) {
        return;
    }
    for (i = 0; i < 4; i++) {
        ClActor_SetVisible(cursor->corners[i], visible);
    }
    cursor->visible = visible;
}

void BAppCursor_SetPos(BAppCursor *cursor, const CursorMoveData *pos) {
    ClActorPos actorPos;
    u8 halfWidth = pos->width / 2;
    u8 halfHeight = pos->height / 2;

    actorPos.x = pos->x - halfWidth;
    actorPos.y = pos->y - halfHeight;
    ClActor_SetSurfacePos(cursor->corners[0], &actorPos, 1);
    actorPos.x = pos->x - halfWidth;
    actorPos.y = pos->y + halfHeight;
    ClActor_SetSurfacePos(cursor->corners[1], &actorPos, 1);
    actorPos.x = pos->x + halfWidth;
    actorPos.y = pos->y - halfHeight;
    ClActor_SetSurfacePos(cursor->corners[2], &actorPos, 1);
    actorPos.x = pos->x + halfWidth;
    actorPos.y = pos->y + halfHeight;
    ClActor_SetSurfacePos(cursor->corners[3], &actorPos, 1);
}

void BAppTool_QueueWindowScreen(PrintWindow *printWindow) {
    BmpWin_FlushMap(printWindow->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(printWindow->window));
}

void BAppTool_QueueWindowScreens(PrintWindow *printWindows, const u8 *list) {
    u32 i;

    i = 0;
    while (TRUE) {
        if (list[i] == 0xff) {
            break;
        }
        BAppTool_QueueWindowScreen(&printWindows[list[i]]);
        i++;
    }
}

void BAppTool_FlushPrintWindows(PrintWindow *printWindows, PrintQueue *queue, u32 count) {
    u32 i;

    PrintQueue_Main(queue);
    for (i = 0; i < count; i++) {
        PrintWindow_Flush(&printWindows[i], queue);
    }
}
