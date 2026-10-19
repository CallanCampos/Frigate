#include "types.h"
#include "app/research_radar/arrow.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

#define PIECE_MAX 26

// The length of a piece, in pixels
#define PIECE_SIZE 8

enum {
    STATE_WAIT,
    STATE_RUN,
    STATE_DONE,
};

// The kinds of piece, which pick their animation
enum {
    PIECE_ROW,
    PIECE_COLUMN,
    PIECE_CORNER,
    PIECE_HEAD,
};

typedef struct {
    BOOL shown;
    // The frame when the piece is shown
    u16 showFrame;
    // Whether the piece's animation stops, and when
    BOOL stops;
    u16 stopFrame;
    u8 x;
    u8 y;
    u32 kind;
    ClActor *actor;
} ArrowPiece;

struct Arrow {
    HeapID heapId;
    ClActUnit *unit;
    ArrowResources resources;
    u32 state;
    u32 frames;
    ArrowPiece pieces[PIECE_MAX];
    u8 pieceCount;
};

static void Arrow_UpdateCore(Arrow *arrow);
static void Arrow_StateWait(Arrow *arrow);
static void Arrow_StateRun(Arrow *arrow);
static void Arrow_StateDone(Arrow *arrow);
static void Arrow_SetState(Arrow *arrow, u32 state);
static BOOL Arrow_IsAllShown(Arrow *arrow);
static void Arrow_ShowPiece(Arrow *arrow, u8 index);
static void Arrow_StopPiece(Arrow *arrow, u8 index);
static void Arrow_HideAll(Arrow *arrow);
static void Arrow_SetPathCore(Arrow *arrow, int startX, int startY, int endX, int endY);
static void Arrow_InitPiece(ArrowPiece *piece, u8 x, u8 y, u32 frame, u32 kind);
static Arrow *Arrow_Alloc(HeapID heapId);
static void Arrow_InitWork(Arrow *arrow, HeapID heapId);
static void Arrow_Free(Arrow *arrow);
static void Arrow_CreateUnit(Arrow *arrow);
static void Arrow_DeleteUnit(Arrow *arrow);
static void Arrow_CreateActors(Arrow *arrow);
static void Arrow_DeleteActors(Arrow *arrow);
static void Arrow_SetResources(Arrow *arrow, const ArrowResources *resources);

Arrow *Arrow_Create(HeapID heapId, const ArrowResources *resources) {
    Arrow *arrow = Arrow_Alloc(heapId);

    Arrow_InitWork(arrow, heapId);
    Arrow_SetResources(arrow, resources);
    Arrow_CreateUnit(arrow);
    Arrow_CreateActors(arrow);
    return arrow;
}

void Arrow_Delete(Arrow *arrow) {
    Arrow_DeleteActors(arrow);
    Arrow_DeleteUnit(arrow);
    Arrow_Free(arrow);
}

void Arrow_SetPath(Arrow *arrow, int startX, int startY, int endX, int endY) {
    Arrow_SetPathCore(arrow, startX, startY, endX, endY);
}

void Arrow_Update(Arrow *arrow) {
    Arrow_UpdateCore(arrow);
}

void Arrow_Start(Arrow *arrow) {
    Arrow_SetState(arrow, STATE_RUN);
}

void Arrow_Hide(Arrow *arrow) {
    Arrow_SetState(arrow, STATE_WAIT);
    Arrow_HideAll(arrow);
}

static void Arrow_UpdateCore(Arrow *arrow) {
    switch (arrow->state) {
    case STATE_WAIT:
        Arrow_StateWait(arrow);
        break;
    case STATE_RUN:
        Arrow_StateRun(arrow);
        break;
    case STATE_DONE:
        Arrow_StateDone(arrow);
        break;
    }
    arrow->frames++;
}

static void Arrow_StateWait(Arrow *arrow) {
}

static void Arrow_StateRun(Arrow *arrow) {
    int i;

    for (i = 0; i < arrow->pieceCount; i++) {
        if (arrow->pieces[i].shown == FALSE && arrow->pieces[i].showFrame <= arrow->frames) {
            Arrow_ShowPiece(arrow, i);
        }
        if (arrow->pieces[i].shown == TRUE && arrow->pieces[i].stops == TRUE &&
            arrow->pieces[i].stopFrame <= arrow->frames) {
            Arrow_StopPiece(arrow, i);
        }
    }
    if (Arrow_IsAllShown(arrow)) {
        Arrow_SetState(arrow, STATE_DONE);
    }
}

static void Arrow_StateDone(Arrow *arrow) {
}

static void Arrow_SetState(Arrow *arrow, u32 state) {
    arrow->state = state;
    arrow->frames = 0;
}

static BOOL Arrow_IsAllShown(Arrow *arrow) {
    int i;

    for (i = 0; i < arrow->pieceCount; i++) {
        if (arrow->pieces[i].shown == FALSE) {
            return FALSE;
        }
    }
    return TRUE;
}

static void Arrow_ShowPiece(Arrow *arrow, u8 index) {
    ArrowPiece *piece = &arrow->pieces[index];
    ClActor *actor = piece->actor;
    ClActorPos pos;
    u16 sequence;

    pos.x = piece->x;
    pos.y = piece->y;
    piece->shown = TRUE;
    switch (piece->kind) {
    case PIECE_ROW:
        sequence = arrow->resources.sequences[PIECE_ROW];
        break;
    case PIECE_COLUMN:
        sequence = arrow->resources.sequences[PIECE_COLUMN];
        break;
    case PIECE_CORNER:
        sequence = arrow->resources.sequences[PIECE_CORNER];
        break;
    case PIECE_HEAD:
        sequence = arrow->resources.sequences[PIECE_HEAD];
        break;
    }
    ClActor_SetSurfacePos(actor, &pos, arrow->resources.surface);
    ClActor_SetSequence(actor, sequence);
    ClActor_SetAnimTime(actor, 0);
    ClActor_SetPlayMode(actor, 1);
    func_0204c520(actor, TRUE);
    func_0204c53c(actor, FX32_CONST(18));
    ClActor_StartAnim(actor);
    ClActor_SetVisible(actor, TRUE);
}

static void Arrow_StopPiece(Arrow *arrow, u8 index) {
    ClActor_StopAnim(arrow->pieces[index].actor);
}

static void Arrow_HideAll(Arrow *arrow) {
    int i;
    int count = arrow->pieceCount;

    for (i = 0; i < count; i++) {
        ClActor_SetVisible(arrow->pieces[i].actor, FALSE);
    }
}

// Each piece is shown a frame after the one before it. A row or column that isn't a whole number of pieces ends with a
// piece that overlaps the one before it, shown that much earlier
static void Arrow_SetPathCore(Arrow *arrow, int startX, int startY, int endX, int endY) {
    int rowY;
    int width;
    int rowPieces;
    int index;
    f32 frame;
    int i;
    int columnPieces;
    int height;
    int columnX;
    int y;

    frame = 0;
    index = 0;
    width = startX - endX;
    rowPieces = (width - 4) / PIECE_SIZE;
    rowY = startY - 4;
    startX -= PIECE_SIZE;
    for (i = 0; i < rowPieces; i++) {
        Arrow_InitPiece(&arrow->pieces[index], startX, rowY, frame, PIECE_ROW);
        startX -= PIECE_SIZE;
        frame += 1.0f;
        index++;
    }
    if (rowPieces == 0) {
        if (width - 4 > 0) {
            Arrow_InitPiece(&arrow->pieces[index], startX, rowY, frame, PIECE_ROW);
            if (width - 4 < 4) {
                arrow->pieces[index].stops = TRUE;
                arrow->pieces[index].stopFrame = 1;
            }
            frame += (width - 4) * 2.0f / PIECE_SIZE - 1.0f;
            index++;
        }
    } else if ((width - 4) % PIECE_SIZE != 0) {
        frame -= 2.0f - (width - 4 - rowPieces * PIECE_SIZE) * 2.0f / PIECE_SIZE;
        Arrow_InitPiece(&arrow->pieces[index], endX + 4, rowY, frame, PIECE_ROW);
        frame += 1.0f;
        index++;
    }

    Arrow_InitPiece(&arrow->pieces[index], endX - 4, startY - 4, frame, PIECE_CORNER);
    frame += 1.0f;
    index++;

    height = endY - startY;
    columnPieces = (height - 4) / PIECE_SIZE;
    columnX = endX - 4;
    y = startY + 4;
    for (i = 0; i < columnPieces; i++) {
        Arrow_InitPiece(&arrow->pieces[index], columnX, y, frame, PIECE_COLUMN);
        y += PIECE_SIZE;
        frame += 1.0f;
        index++;
    }
    if ((height - 4) % PIECE_SIZE != 0) {
        frame -= 2.0f - (height - 4 - columnPieces * PIECE_SIZE) * 2.0f / PIECE_SIZE;
        Arrow_InitPiece(&arrow->pieces[index], columnX, endY - PIECE_SIZE, frame, PIECE_COLUMN);
        frame += 1.0f;
        index++;
    }

    Arrow_InitPiece(&arrow->pieces[index], endX - 4, endY - 4, frame, PIECE_HEAD);
    arrow->pieceCount = index + 1;
}

static void Arrow_InitPiece(ArrowPiece *piece, u8 x, u8 y, u32 frame, u32 kind) {
    piece->x = x;
    piece->shown = FALSE;
    piece->stops = FALSE;
    piece->stopFrame = 0;
    piece->y = y;
    piece->showFrame = frame;
    piece->kind = kind;
}

static Arrow *Arrow_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(Arrow), FALSE, "arrow.c", 558);
}

static void Arrow_InitWork(Arrow *arrow, HeapID heapId) {
    int i;

    arrow->heapId = heapId;
    arrow->state = STATE_WAIT;
    arrow->frames = 0;
    arrow->pieceCount = 0;
    arrow->unit = NULL;
    for (i = 0; i < PIECE_MAX; i++) {
        arrow->pieces[i].shown = FALSE;
        arrow->pieces[i].showFrame = 0;
        arrow->pieces[i].stops = FALSE;
        arrow->pieces[i].stopFrame = 0;
        arrow->pieces[i].x = 0;
        arrow->pieces[i].y = 0;
        arrow->pieces[i].actor = NULL;
    }
}

static void Arrow_Free(Arrow *arrow) {
    GFL_HeapFree(arrow);
}

static void Arrow_CreateUnit(Arrow *arrow) {
    arrow->unit = ClActUnit_Create(PIECE_MAX, 0, arrow->heapId);
}

static void Arrow_DeleteUnit(Arrow *arrow) {
    ClActUnit_Delete(arrow->unit);
}

static void Arrow_CreateActors(Arrow *arrow) {
    ClActorSetup setup;
    ArrowResources *resources = &arrow->resources;
    int i;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    for (i = 0; i < PIECE_MAX; i++) {
        arrow->pieces[i].actor = ClActor_Create(arrow->unit, resources->chars, resources->palette, resources->cellAnims,
                                                &setup, resources->surface, arrow->heapId);
    }
}

static void Arrow_DeleteActors(Arrow *arrow) {
    int i;

    for (i = 0; i < PIECE_MAX; i++) {
        ClActor_Delete(arrow->pieces[i].actor);
    }
}

static void Arrow_SetResources(Arrow *arrow, const ArrowResources *resources) {
    arrow->resources = *resources;
}
