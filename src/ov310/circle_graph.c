#include "types.h"
#include "app/research_radar/circle_graph.h"
#include "app/research_radar/queue.h"
#include "gfl/heap.h"
#include "math.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/g3d.h"

#define PI 3.1415927f

#define ITEM_MAX 20
// The points of the rim, one per percent
#define POINT_COUNT 100
// Each percent is a triangle from the centre to two points of the rim
#define VERTEX_COUNT (POINT_COUNT * 3)

// The animations of the graph, which run one after another from a queue
enum {
    STATE_NONE,
    STATE_GROW_SLOW,
    STATE_GROW_FAST,
    STATE_SHRINK,
    STATE_FULL,
    STATE_GROW,
};

// The frames of the animations
#define GROW_SLOW_FRAMES 120
#define GROW_FAST_FRAMES 30
#define SHRINK_FRAMES 20
#define GROW_FRAMES 60

typedef struct {
    u8 id;
    u32 count;
    u8 percent;
    // The first and last percents of the slice
    u8 start;
    u8 end;
    u8 color[3];
    u8 centerColor[3];
} CircleGraphItem;

typedef struct {
    VecFx16 pos;
    GXRgb color;
    u8 polygonId;
} CircleGraphVertex;

struct CircleGraph {
    HeapID heapId;
    BOOL visible;
    u32 state;
    Queue *queue;
    u32 timer;
    // While waiting, the next animation doesn't start
    BOOL waiting;
    u32 waitFrames;
    CircleGraphItem items[ITEM_MAX];
    u8 itemCount;
    fx32 radius;
    // The centre
    VecFx16 pos;
    VecFx16 points[POINT_COUNT];
    CircleGraphVertex vertices[VERTEX_COUNT];
};

static void CircleGraph_SetPos(CircleGraph *graph, const VecFx16 *pos);
static void CircleGraph_DrawSlices(CircleGraph *graph);
static void CircleGraph_DrawBorders(CircleGraph *graph);
static void CircleGraph_SetCamera(CircleGraph *graph);
static void CircleGraph_Update(CircleGraph *graph);
static void CircleGraph_UpdateNone(CircleGraph *graph);
static void CircleGraph_UpdateGrowSlow(CircleGraph *graph);
static void CircleGraph_UpdateGrowFast(CircleGraph *graph);
static void CircleGraph_UpdateShrink(CircleGraph *graph);
static void CircleGraph_UpdateFull(CircleGraph *graph);
static void CircleGraph_UpdateGrow(CircleGraph *graph);
static void CircleGraph_CountFrame(CircleGraph *graph);
static void CircleGraph_UpdateWait(CircleGraph *graph);
static void CircleGraph_Request(CircleGraph *graph, u32 state);
static void CircleGraph_StartNext(CircleGraph *graph);
static void CircleGraph_SetState(CircleGraph *graph, u32 state);
static void CircleGraph_ClearItems(CircleGraph *graph);
static void CircleGraph_AddItem(CircleGraph *graph, const CircleGraphData *data);
static void CircleGraph_SortItems(CircleGraph *graph);
static void CircleGraph_CalcPercents(CircleGraph *graph);
static void CircleGraph_CalcRanges(CircleGraph *graph);
static void CircleGraph_BuildVertices(CircleGraph *graph);
static void CircleGraph_SetVisibleCore(CircleGraph *graph, BOOL visible);
static void CircleGraph_SetWaitCore(CircleGraph *graph, u32 frames);
static void CircleGraph_SetPosCore(CircleGraph *graph, const VecFx16 *pos);
static u8 CircleGraph_GetItemIndexCore(CircleGraph *graph, u8 id);
static CircleGraphItem *CircleGraph_GetItem(CircleGraph *graph, int index);
static CircleGraphItem *CircleGraph_GetItemById(CircleGraph *graph, u8 id);
static u32 CircleGraph_GetTotalCount(CircleGraph *graph);
static int CircleGraph_GetPercentTotal(CircleGraph *graph);
static void CircleGraph_GetItemRange(const CircleGraphItem *item, u8 *start, u8 *end);
static GXRgb CircleGraph_GetItemColor(const CircleGraphItem *item);
static GXRgb CircleGraph_GetItemCenterColor(const CircleGraphItem *item);
static void CircleGraph_GetLabelPos(CircleGraph *graph, const CircleGraphItem *item, VecFx16 *pos);
static BOOL CircleGraph_IsVisible(CircleGraph *graph);
static BOOL CircleGraph_IsMovingCore(CircleGraph *graph);
static void CircleGraph_Init(CircleGraph *graph);
static void CircleGraph_Exit(CircleGraph *graph);
static void CircleGraph_InitWork(CircleGraph *graph, HeapID heapId);
static CircleGraph *CircleGraph_Alloc(HeapID heapId);
static void CircleGraph_Free(CircleGraph *graph);
static void CircleGraph_CreateQueue(CircleGraph *graph);
static void CircleGraph_DeleteQueue(CircleGraph *graph);
static void CircleGraph_InitPoints(CircleGraph *graph);
static void CircleGraph_ToScreenPos(const VecFx16 *pos, int *x, int *y);

CircleGraph *CircleGraph_Create(HeapID heapId) {
    CircleGraph *graph = CircleGraph_Alloc(heapId);

    CircleGraph_Init(graph);
    return graph;
}

void CircleGraph_Delete(CircleGraph *graph) {
    CircleGraph_Exit(graph);
    CircleGraph_Free(graph);
}

void CircleGraph_SetData(CircleGraph *graph, const CircleGraphData *data, int num) {
    int i;

    CircleGraph_ClearItems(graph);
    for (i = 0; i < num; i++) {
        CircleGraph_AddItem(graph, &data[i]);
    }
    CircleGraph_SortItems(graph);
    CircleGraph_CalcPercents(graph);
    CircleGraph_CalcRanges(graph);
    CircleGraph_BuildVertices(graph);
}

void CircleGraph_Main(CircleGraph *graph) {
    CircleGraph_Update(graph);
}

void CircleGraph_Draw(CircleGraph *graph) {
    if (CircleGraph_IsVisible(graph) == TRUE && graph->state != STATE_NONE) {
        CircleGraph_SetCamera(graph);
        CircleGraph_DrawSlices(graph);
        CircleGraph_DrawBorders(graph);
    }
}

void CircleGraph_RequestGrowSlow(CircleGraph *graph) {
    CircleGraph_Request(graph, STATE_GROW_SLOW);
}

void CircleGraph_RequestGrowFast(CircleGraph *graph) {
    CircleGraph_Request(graph, STATE_GROW_FAST);
}

void CircleGraph_RequestShrink(CircleGraph *graph) {
    CircleGraph_Request(graph, STATE_SHRINK);
}

void CircleGraph_RequestGrow(CircleGraph *graph) {
    CircleGraph_Request(graph, STATE_GROW);
}

void CircleGraph_SetVisible(CircleGraph *graph, BOOL visible) {
    CircleGraph_SetVisibleCore(graph, visible);
}

void CircleGraph_SetWait(CircleGraph *graph, u32 frames) {
    CircleGraph_SetWaitCore(graph, frames);
}

static void CircleGraph_SetPos(CircleGraph *graph, const VecFx16 *pos) {
    CircleGraph_SetPosCore(graph, pos);
    CircleGraph_BuildVertices(graph);
}

void CircleGraph_SetDepth(CircleGraph *graph, fx16 z) {
    VecFx16 pos;

    VEC_Fx16Set(&pos, graph->pos.x, graph->pos.y, z);
    CircleGraph_SetPos(graph, &pos);
}

u8 CircleGraph_GetItemIndex(CircleGraph *graph, u8 id) {
    return CircleGraph_GetItemIndexCore(graph, id);
}

u8 CircleGraph_GetItemId(CircleGraph *graph, u8 index) {
    return CircleGraph_GetItem(graph, index)->id;
}

u8 CircleGraph_GetPercentById(CircleGraph *graph, u8 id) {
    return CircleGraph_GetItemById(graph, id)->percent;
}

u8 CircleGraph_GetPercent(CircleGraph *graph, u8 index) {
    return CircleGraph_GetItem(graph, index)->percent;
}

BOOL CircleGraph_IsMoving(CircleGraph *graph) {
    return CircleGraph_IsMovingCore(graph);
}

void CircleGraph_GetLabelScreenPosById(CircleGraph *graph, u8 id, int *x, int *y) {
    VecFx16 pos;

    CircleGraph_GetLabelPos(graph, CircleGraph_GetItemById(graph, id), &pos);
    CircleGraph_ToScreenPos(&pos, x, y);
}

void CircleGraph_GetLabelScreenPos(CircleGraph *graph, u8 index, int *x, int *y) {
    VecFx16 pos;

    CircleGraph_GetLabelPos(graph, CircleGraph_GetItem(graph, index), &pos);
    CircleGraph_ToScreenPos(&pos, x, y);
}

// Draws the slices' triangles shown so far, which end at 100 percent and grow backwards from it
static void CircleGraph_DrawSlices(CircleGraph *graph) {
    int num;
    int start;
    int vtx;
    int i;

    switch (graph->state) {
    case STATE_NONE:
        num = 0;
        break;
    case STATE_GROW_SLOW:
        num = graph->timer * 100 / GROW_SLOW_FRAMES;
        start = 100 - num;
        break;
    case STATE_GROW_FAST:
        num = graph->timer * 100 / GROW_FAST_FRAMES;
        start = 100 - num;
        break;
    case STATE_SHRINK:
        num = (SHRINK_FRAMES - graph->timer) * 100 / SHRINK_FRAMES;
        start = 100 - num;
        break;
    case STATE_FULL:
        num = 100;
        start = 0;
        break;
    case STATE_GROW:
        num = graph->timer * 100 / GROW_FRAMES;
        start = 100 - num;
        break;
    }

    // start isn't set in STATE_NONE, where num is 0 and nothing is drawn
    vtx = start * 3;
    for (i = 0; i < num; i++) {
        G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, graph->vertices[vtx].polygonId, 31, 0);
        G3_Begin(GX_BEGIN_TRIANGLES);
        G3_Color(graph->vertices[vtx].color);
        G3_Vtx(graph->vertices[vtx].pos.x, graph->vertices[vtx].pos.y, graph->vertices[vtx].pos.z);
        vtx = (vtx + 1) % VERTEX_COUNT;
        G3_Color(graph->vertices[vtx].color);
        G3_Vtx(graph->vertices[vtx].pos.x, graph->vertices[vtx].pos.y, graph->vertices[vtx].pos.z);
        vtx = (vtx + 1) % VERTEX_COUNT;
        G3_Color(graph->vertices[vtx].color);
        G3_Vtx(graph->vertices[vtx].pos.x, graph->vertices[vtx].pos.y, graph->vertices[vtx].pos.z);
        vtx = (vtx + 1) % VERTEX_COUNT;
        G3_End();
    }
}

// Draws a black line from the centre to the start of each slice of at least 3 percent, once the graph is still
static void CircleGraph_DrawBorders(CircleGraph *graph) {
    int i;
    int itemCount;
    CircleGraphItem *item;
    u8 start;
    u8 end;
    fx16 centerX;
    fx16 centerY;
    fx16 x;
    fx16 y;

    if (CircleGraph_IsMovingCore(graph) == FALSE) {
        G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 0, 31, 0);
        itemCount = graph->itemCount;
        for (i = 0; i < itemCount; i++) {
            item = CircleGraph_GetItem(graph, i);
            CircleGraph_GetItemRange(item, &start, &end);
            if (item->percent >= 3) {
                centerX = graph->pos.x;
                centerY = graph->pos.y;
                x = centerX + graph->points[start].x;
                y = centerY + graph->points[start].y;
                G3_Begin(GX_BEGIN_TRIANGLES);
                G3_Color(GX_RGB(0, 0, 0));
                G3_Vtx(centerX, centerY, FX16_CONST(4.0));
                G3_Vtx(centerX, centerY, FX16_CONST(4.0));
                G3_Vtx(x, y, FX16_CONST(4.0));
                G3_End();
            }
        }
    }
}

static void CircleGraph_SetCamera(CircleGraph *graph) {
    VecFx32 camPos;
    VecFx32 camUp;
    VecFx32 target;

    VEC_Set(&camPos, 0, 0, FX32_ONE);
    VEC_Set(&camUp, 0, FX32_ONE, 0);
    VEC_Set(&target, 0, 0, 0);

    gfxLookAt(&camPos, &camUp, &target, TRUE, NULL);
    gfxOrtho(FX32_ONE, -FX32_ONE, FX32_CONST(-1.333), FX32_CONST(1.333), FX32_CONST(0.1), FX32_CONST(5.0), FX32_ONE,
             TRUE, NULL);
    NNS_G3DRenderStateInit();
    NNS_G3dGlbLookAt(&camPos, &camUp, &target);
    NNS_G3dGlbOrtho(FX32_ONE, -FX32_ONE, FX32_CONST(-1.333), FX32_CONST(1.333), FX32_CONST(0.1), FX32_CONST(5.0));
    NNS_G3dGlbFlush();
}

static void CircleGraph_Update(CircleGraph *graph) {
    if (graph->waiting == FALSE) {
        switch (graph->state) {
        case STATE_NONE:
            CircleGraph_UpdateNone(graph);
            break;
        case STATE_GROW_SLOW:
            CircleGraph_UpdateGrowSlow(graph);
            break;
        case STATE_GROW_FAST:
            CircleGraph_UpdateGrowFast(graph);
            break;
        case STATE_SHRINK:
            CircleGraph_UpdateShrink(graph);
            break;
        case STATE_FULL:
            CircleGraph_UpdateFull(graph);
            break;
        case STATE_GROW:
            CircleGraph_UpdateGrow(graph);
            break;
        }
    }
    CircleGraph_CountFrame(graph);
    CircleGraph_UpdateWait(graph);
}

static void CircleGraph_UpdateNone(CircleGraph *graph) {
    if (Queue_IsEmpty(graph->queue) == FALSE) {
        CircleGraph_StartNext(graph);
    }
}

static void CircleGraph_UpdateGrowSlow(CircleGraph *graph) {
    if (graph->timer >= GROW_SLOW_FRAMES) {
        CircleGraph_SetState(graph, STATE_FULL);
    }
}

static void CircleGraph_UpdateGrowFast(CircleGraph *graph) {
    if (graph->timer >= GROW_FAST_FRAMES) {
        CircleGraph_SetState(graph, STATE_FULL);
    }
}

static void CircleGraph_UpdateShrink(CircleGraph *graph) {
    if (graph->timer >= SHRINK_FRAMES) {
        CircleGraph_SetState(graph, STATE_NONE);
    }
}

static void CircleGraph_UpdateFull(CircleGraph *graph) {
    if (Queue_IsEmpty(graph->queue) == FALSE) {
        CircleGraph_StartNext(graph);
    }
}

static void CircleGraph_UpdateGrow(CircleGraph *graph) {
    if (graph->timer >= GROW_FRAMES) {
        CircleGraph_SetState(graph, STATE_FULL);
    }
}

// Counts the state's frames, up to its length
static void CircleGraph_CountFrame(CircleGraph *graph) {
    u32 max;

    graph->timer++;
    switch (graph->state) {
    case STATE_NONE:
        max = 0xffffffff;
        break;
    case STATE_GROW_SLOW:
        max = GROW_SLOW_FRAMES;
        break;
    case STATE_GROW_FAST:
        max = GROW_FAST_FRAMES;
        break;
    case STATE_SHRINK:
        max = SHRINK_FRAMES;
        break;
    case STATE_FULL:
        max = 0xffffffff;
        break;
    case STATE_GROW:
        max = GROW_FRAMES;
        break;
    }
    if (max < graph->timer) {
        graph->timer = max;
    }
}

static void CircleGraph_UpdateWait(CircleGraph *graph) {
    if (graph->waiting) {
        if (graph->waitFrames != 0) {
            graph->waitFrames--;
            if (graph->waitFrames == 0) {
                graph->waiting = FALSE;
            }
        }
    }
}

static void CircleGraph_Request(CircleGraph *graph, u32 state) {
    Queue_Push(graph->queue, state);
}

static void CircleGraph_StartNext(CircleGraph *graph) {
    CircleGraph_SetState(graph, Queue_Pop(graph->queue));
}

static void CircleGraph_SetState(CircleGraph *graph, u32 state) {
    graph->state = state;
    graph->timer = 0;
}

static void CircleGraph_ClearItems(CircleGraph *graph) {
    graph->itemCount = 0;
    Queue_Clear(graph->queue);
    CircleGraph_SetState(graph, STATE_NONE);
}

static void CircleGraph_AddItem(CircleGraph *graph, const CircleGraphData *data) {
    graph->items[graph->itemCount].id = data->id;
    graph->items[graph->itemCount].count = data->count;
    graph->items[graph->itemCount].percent = 0;
    graph->items[graph->itemCount].start = 0;
    graph->items[graph->itemCount].end = 0;
    graph->items[graph->itemCount].color[0] = data->color[0];
    graph->items[graph->itemCount].color[1] = data->color[1];
    graph->items[graph->itemCount].color[2] = data->color[2];
    graph->items[graph->itemCount].centerColor[0] = data->centerColor[0];
    graph->items[graph->itemCount].centerColor[1] = data->centerColor[1];
    graph->items[graph->itemCount].centerColor[2] = data->centerColor[2];
    graph->itemCount++;
}

// Sorts the items by count, largest first
static void CircleGraph_SortItems(CircleGraph *graph) {
    int i;
    int itemCount = graph->itemCount;
    int j;
    CircleGraphItem item;

    for (i = 0; i < itemCount - 1; i++) {
        for (j = 0; j < itemCount - 1 - i; j++) {
            if (graph->items[j].count < graph->items[j + 1].count) {
                item = graph->items[j];
                graph->items[j] = graph->items[j + 1];
                graph->items[j + 1] = item;
            }
        }
    }
}

// Gives each item its share in whole percents, and the percents lost to rounding down to the first items
static void CircleGraph_CalcPercents(CircleGraph *graph) {
    int i;
    u32 total = CircleGraph_GetTotalCount(graph);
    int itemCount = graph->itemCount;
    int rest;
    CircleGraphItem *item;

    for (i = 0; i < itemCount; i++) {
        item = &graph->items[i];
        item->percent = item->count * 100 / total;
    }

    rest = 100 - CircleGraph_GetPercentTotal(graph);
    i = 0;
    while (rest > 0) {
        graph->items[i].percent++;
        rest--;
        i = (i + 1) % itemCount;
    }
}

static void CircleGraph_CalcRanges(CircleGraph *graph) {
    int i;
    int itemCount = graph->itemCount;
    int percent = 0;
    CircleGraphItem *item;

    for (i = 0; i < itemCount; i++) {
        item = &graph->items[i];
        item->start = percent;
        item->end = percent + item->percent - 1;
        percent += item->percent;
    }
}

// Builds the triangles of the slices, each slice a little closer to the camera than the one before
static void CircleGraph_BuildVertices(CircleGraph *graph) {
    int itemCount = graph->itemCount;
    int vtx = 0;
    int i;
    int j;
    CircleGraphItem *item;
    GXRgb centerColor;
    GXRgb color;
    u8 start;
    u8 end;
    VecFx16 center;

    for (i = 0; i < itemCount; i++) {
        item = CircleGraph_GetItem(graph, i);
        centerColor = CircleGraph_GetItemCenterColor(item);
        color = CircleGraph_GetItemColor(item);
        CircleGraph_GetItemRange(item, &start, &end);
        center.x = graph->pos.x;
        center.y = graph->pos.y;
        center.z = graph->pos.z + i * FX16_CONST(0.05);
        for (j = start; j <= end; j++) {
            graph->vertices[vtx].color = centerColor;
            graph->vertices[vtx].pos = center;
            graph->vertices[vtx].polygonId = i;
            graph->vertices[vtx + 1].color = color;
            VEC_Fx16Add(&center, &graph->points[j], &graph->vertices[vtx + 1].pos);
            graph->vertices[vtx + 1].polygonId = i;
            graph->vertices[vtx + 2].color = color;
            VEC_Fx16Add(&center, &graph->points[(j + 1) % POINT_COUNT], &graph->vertices[vtx + 2].pos);
            graph->vertices[vtx + 2].polygonId = i;
            vtx += 3;
        }
    }
}

static void CircleGraph_SetVisibleCore(CircleGraph *graph, BOOL visible) {
    graph->visible = visible;
}

static void CircleGraph_SetWaitCore(CircleGraph *graph, u32 frames) {
    graph->waiting = TRUE;
    graph->waitFrames = frames;
}

static void CircleGraph_SetPosCore(CircleGraph *graph, const VecFx16 *pos) {
    VEC_Fx16Set(&graph->pos, pos->x, pos->y, pos->z);
}

static u8 CircleGraph_GetItemIndexCore(CircleGraph *graph, u8 id) {
    int i;
    int itemCount = graph->itemCount;

    for (i = 0; i < itemCount; i++) {
        u8 itemId = graph->items[i].id;

        if (itemId == id) {
            return i;
        }
    }
    return 0;
}

static CircleGraphItem *CircleGraph_GetItem(CircleGraph *graph, int index) {
    return &graph->items[index];
}

static CircleGraphItem *CircleGraph_GetItemById(CircleGraph *graph, u8 id) {
    return CircleGraph_GetItem(graph, CircleGraph_GetItemIndexCore(graph, id));
}

static u32 CircleGraph_GetTotalCount(CircleGraph *graph) {
    int i;
    int itemCount = graph->itemCount;
    u32 total = 0;

    for (i = 0; i < itemCount; i++) {
        total += graph->items[i].count;
    }
    return total;
}

static int CircleGraph_GetPercentTotal(CircleGraph *graph) {
    int i;
    int itemCount = graph->itemCount;
    int total = 0;

    for (i = 0; i < itemCount; i++) {
        total += graph->items[i].percent;
    }
    return total;
}

// The range goes through a float, which changes nothing
static void CircleGraph_GetItemRange(const CircleGraphItem *item, u8 *start, u8 *end) {
    int first = (f32)item->start;
    int last = (f32)item->end;

    *start = first;
    *end = last;
}

static GXRgb CircleGraph_GetItemColor(const CircleGraphItem *item) {
    return GX_RGB(item->color[0], item->color[1], item->color[2]);
}

static GXRgb CircleGraph_GetItemCenterColor(const CircleGraphItem *item) {
    return GX_RGB(item->centerColor[0], item->centerColor[1], item->centerColor[2]);
}

// The position of an item's label, at the middle of its slice, inside the rim
static void CircleGraph_GetLabelPos(CircleGraph *graph, const CircleGraphItem *item, VecFx16 *pos) {
    u8 start;
    u8 end;
    int first;
    int last;
    f32 angle;
    f32 x;
    f32 y;
    f32 z;
    f32 centerX;
    f32 centerY;
    f32 centerZ;

    CircleGraph_GetItemRange(item, &start, &end);
    first = start;
    last = end;
    angle = PI / 2 - (first + last + 1) * PI * 0.01f;
    x = (f32)cos(angle) * 0.42f;
    y = (f32)sin(angle) * 0.42f;
    z = FX_FX16_TO_F32(graph->points[last].z);
    centerX = FX_FX16_TO_F32(graph->pos.x);
    centerY = FX_FX16_TO_F32(graph->pos.y);
    centerZ = FX_FX16_TO_F32(graph->pos.z);
    pos->x = FX_F32_TO_FX16(centerX + x);
    pos->y = FX_F32_TO_FX16(centerY + y);
    pos->z = FX_F32_TO_FX16(centerZ + z);
}

static BOOL CircleGraph_IsVisible(CircleGraph *graph) {
    return graph->visible;
}

static BOOL CircleGraph_IsMovingCore(CircleGraph *graph) {
    switch (graph->state) {
    case STATE_NONE:
        return FALSE;
    case STATE_GROW_SLOW:
        return TRUE;
    case STATE_GROW_FAST:
        return TRUE;
    case STATE_SHRINK:
        return TRUE;
    case STATE_FULL:
        return FALSE;
    case STATE_GROW:
        return TRUE;
    }
    return FALSE;
}

static void CircleGraph_Init(CircleGraph *graph) {
    CircleGraph_CreateQueue(graph);
    CircleGraph_InitPoints(graph);
}

static void CircleGraph_Exit(CircleGraph *graph) {
    CircleGraph_DeleteQueue(graph);
}

static void CircleGraph_InitWork(CircleGraph *graph, HeapID heapId) {
    graph->heapId = heapId;
    graph->visible = FALSE;
    graph->state = STATE_NONE;
    graph->queue = NULL;
    graph->waiting = FALSE;
    graph->waitFrames = 0;
    graph->radius = FX32_CONST(0.491);
    graph->itemCount = 0;
    graph->pos.x = FX16_CONST(-0.699);
    graph->pos.y = FX16_CONST(-0.049);
    graph->pos.z = FX16_CONST(-1.0);
}

static CircleGraph *CircleGraph_Alloc(HeapID heapId) {
    CircleGraph *graph = GFL_HeapAllocate(heapId, sizeof(CircleGraph), FALSE, "circle_graph.c", 1588);

    CircleGraph_InitWork(graph, heapId);
    return graph;
}

static void CircleGraph_Free(CircleGraph *graph) {
    GFL_HeapFree(graph);
}

static void CircleGraph_CreateQueue(CircleGraph *graph) {
    graph->queue = Queue_Create(10, graph->heapId);
}

static void CircleGraph_DeleteQueue(CircleGraph *graph) {
    Queue_Delete(graph->queue);
}

// The points of the rim, clockwise from the top
static void CircleGraph_InitPoints(CircleGraph *graph) {
    int i;
    f32 radius = FX_FX32_TO_F32(graph->radius);
    f32 angle;
    f32 x;
    f32 y;
    f32 z = 0.0f;

    for (i = 0; i < POINT_COUNT; i++) {
        angle = PI / 2 - 2 * PI * i / POINT_COUNT;
        x = (f32)cos(angle) * radius;
        y = (f32)sin(angle) * radius;
        graph->points[i].x = FX_F32_TO_FX16(x);
        graph->points[i].y = FX_F32_TO_FX16(y);
        graph->points[i].z = FX_F32_TO_FX16(z);
    }
}

static void CircleGraph_ToScreenPos(const VecFx16 *pos, int *x, int *y) {
    VecFx32 world;

    world.x = FX_F32_TO_FX32(FX_FX16_TO_F32(pos->x));
    world.y = FX_F32_TO_FX32(FX_FX16_TO_F32(pos->y));
    world.z = FX_F32_TO_FX32(FX_FX16_TO_F32(pos->z));
    NNS_G3DProject(&world, x, y);
}
