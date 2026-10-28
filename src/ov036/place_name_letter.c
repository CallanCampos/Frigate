// place_name_letter.c: the two animated letters of a place name sign: the zone's icon and name text that slide in
// and out on the field. The name is the ROM's own, from GFL_HeapAllocate's file argument
#include "types.h"
#include "constants/version.h"
#include "field/place_name_letter.h"
#include "field/zone.h"
#include "gfl/arc_util.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/bmp_oam.h"
#include "system/printsys.h"
#include "system/version.h"

typedef void (*LetterInitFn)(PlaceNameLetter *letter, void *work, const void *arg);
typedef void (*LetterExitFn)(PlaceNameLetter *letter, void *work);
typedef u32 (*LetterUpdateFn)(PlaceNameLetter *letter, void *work);

typedef struct {
    u32 size;
    LetterInitFn init;
    LetterExitFn exit;
    LetterInitFn set;
    LetterUpdateFn update;
} PlaceNameLetterOps;

// What a part is doing: nothing, sliding in, sliding out, hiding, or taking what it was set up with
enum {
    PLACE_NAME_LETTER_STATE_NONE,
    PLACE_NAME_LETTER_STATE_SLIDE_IN,
    PLACE_NAME_LETTER_STATE_SLIDE_OUT,
    PLACE_NAME_LETTER_STATE_HIDE,
    PLACE_NAME_LETTER_STATE_SET,
    PLACE_NAME_LETTER_STATE_COUNT,
};

// The steps of a slide: start it, move until it ends, and be done
enum {
    PLACE_NAME_LETTER_SLIDE_START,
    PLACE_NAME_LETTER_SLIDE_MOVE,
    PLACE_NAME_LETTER_SLIDE_DONE,
};

// The steps of printing the name: clear the bitmap, print, send it once it is printed, a frame's wait, load the
// zone's icon, and be done
enum {
    PLACE_NAME_TEXT_PRINT_CLEAR,
    PLACE_NAME_TEXT_PRINT_NAME,
    PLACE_NAME_TEXT_PRINT_TRANSFER,
    PLACE_NAME_TEXT_PRINT_WAIT,
    PLACE_NAME_TEXT_PRINT_ICON,
    PLACE_NAME_TEXT_PRINT_DONE,
};

// An animation moves its part along its path, then swings it
enum {
    PLACE_NAME_ANIM_STAGE_MOVE,
    PLACE_NAME_ANIM_STAGE_SWING,
};

struct PlaceNameLetter {
    s32 step;
    u32 bgPriority;
    HeapID heapId;
    ClActUnit *unit;
    ArcTool *arc;
    ClActor *actor;
    s32 state;
    s32 changed;
    const PlaceNameLetterOps *ops;
    void *work;
    // The frame the slide starts from and the frames it takes
    u16 startFrame;
    u16 frames;
};

typedef struct {
    s32 x;
    s32 y;
} MovePoint;

typedef struct {
    MovePoint start;
    MovePoint end;
} MovePath;

typedef struct {
    MovePath path;
    s16 startFrame;
    s16 frames;
} MoveSpec;

typedef struct {
    s16 x;
    s16 y;
    s32 frame;
    MoveSpec spec;
} Mover;

typedef struct {
    u16 startAngle;
    u16 endAngle;
    s32 startFrame;
    s32 frames;
    s16 baseX;
    s16 baseY;
    s16 ampX;
    s16 ampY;
    s8 direction;
} SwingSpec;

typedef struct {
    s32 sin;
    s32 frame;
    SwingSpec spec;
} Swing;

typedef struct {
    MovePath path;
    u16 startAngle;
    u16 endAngle;
    s16 ampX;
    s16 ampY;
    s16 direction;
    s16 moveStart;
    s16 moveFrames;
    s16 swingStart;
    s16 swingFrames;
    s16 pad;
} AnimSpec;

typedef struct {
    AnimSpec spec;
    Mover mover;
    Swing swing;
    s32 stage;
    s32 done;
} Anim;

typedef struct {
    PlaceNameTextParam text;
    BmpOamActor *oamActor;
    GFLBitmap *bitmap;
    Anim anim;
    u8 pad[0x20];
    Mover mover;
    ClActor *actor;
    u16 chars;
    u16 palette;
    u16 cellAnims;
    u16 iconId;
    ClActorPos pos;
    PrintQueue *queue;
} PlaceNameTextWork;

typedef struct {
    u32 iconId;
    Anim anim;
    Mover mover;
    ClActor *actor;
    ClActorPos pos;
} PlaceNameFrameWork;

static const ClActorPos PLACE_NAME_LETTER_POSITIONS[5] = {
    { 0x10, 8 }, { 0x80, 0x60 }, { 0x1e, 3 }, { 0xe2, 0xbc }, { 0x80, 0x60 }
};

static void PlaceNameText_Init(PlaceNameLetter *letter, void *work, const void *arg);
static void PlaceNameText_Exit(PlaceNameLetter *letter, void *work);
static void PlaceNameText_SetParam(PlaceNameLetter *letter, void *work, const void *arg);
static u32 PlaceNameText_Update(PlaceNameLetter *letter, void *work);
static void PlaceNameFrame_Init(PlaceNameLetter *letter, void *work, const void *arg);
static void PlaceNameFrame_Exit(PlaceNameLetter *letter, void *work);
static void PlaceNameFrame_SetSeason(PlaceNameLetter *letter, void *work, const void *arg);
static u32 PlaceNameFrame_Update(PlaceNameLetter *letter, void *work);
static void PlaceNameLetter_SetState(PlaceNameLetter *letter, s32 state);

static const PlaceNameLetterOps PLACE_NAME_LETTER_OPS[2] = {
    { sizeof(PlaceNameTextWork), PlaceNameText_Init, PlaceNameText_Exit, PlaceNameText_SetParam, PlaceNameText_Update },
    { sizeof(PlaceNameFrameWork), PlaceNameFrame_Init, PlaceNameFrame_Exit, PlaceNameFrame_SetSeason,
      PlaceNameFrame_Update },
};

// How each part slides out, and how it slides in and swings; the code fills in the start and the timing. They are
// copied rather than written as initializers of the locals, which would give each a .rodata section of its own, and
// declared in the order that gives the original's .rodata
static const MoveSpec PLACE_NAME_LETTER_TEXT_MOVE_OUT = { { { 0, 0 }, { 0, -0x40 } }, 0, 0 };
static const MoveSpec PLACE_NAME_LETTER_FRAME_MOVE_OUT = { { { 0, 0 }, { 0, 0x20 } }, 0, 0 };
static const AnimSpec PLACE_NAME_LETTER_TEXT_ANIM_IN = {
    { { 0, -0x20 }, { 0, 0 } }, 0, 0x7fff, 0, -3, 1, 0, 0, 0, 0, 0
};
static const AnimSpec PLACE_NAME_LETTER_FRAME_ANIM_IN = {
    { { 0, 0x20 }, { 0, 0 } }, 0, 0x7fff, 0, 3, 1, 0, 0, 0, 0, 0
};

PlaceNameLetter *PlaceNameLetter_Create(HeapID heapId, u32 type, void *arg, ClActUnit *unit, ArcTool *arc,
                                        u16 bgPriority, u16 palette, u16 chars, u16 cellAnims) {
    const PlaceNameLetterOps *ops = &PLACE_NAME_LETTER_OPS[type];
    u32 size = ops->size + sizeof(PlaceNameLetter);
    PlaceNameLetter *letter = GFL_HeapAllocate(heapId, size, FALSE, "place_name_letter.c", 406);

    MI_CpuFill8(letter, 0, size);
    letter->heapId = heapId;
    letter->unit = unit;
    letter->bgPriority = bgPriority;
    letter->arc = arc;
    letter->ops = ops;
    letter->work = letter + 1;
    {
        ClActorSetup setup = { 0, 0, 0, 0, 0 };
        letter->actor = ClActor_Create(unit, chars, palette, cellAnims, &setup, 0, heapId);
    }
    ClActor_SetObjMode(letter->actor, 2);
    ClActor_SetVisible(letter->actor, FALSE);
    letter->ops->init(letter, letter->work, arg);
    return letter;
}

void PlaceNameLetter_Delete(PlaceNameLetter *letter) {
    letter->ops->exit(letter, letter->work);
    ClActor_Delete(letter->actor);
    GFL_HeapFree(letter);
}

void PlaceNameLetter_SetParam(PlaceNameLetter *letter, void *arg) {
    letter->ops->set(letter, letter->work, arg);
}

void PlaceNameLetter_Update(PlaceNameLetter *letter) {
    if (letter->changed != 0) {
        letter->changed = letter->ops->update(letter, letter->work);
    }
}

void PlaceNameLetter_SlideIn(PlaceNameLetter *letter, s16 startFrame, u16 frames) {
    letter->startFrame = startFrame;
    letter->frames = frames;
    PlaceNameLetter_SetState(letter, PLACE_NAME_LETTER_STATE_SLIDE_IN);
}

void PlaceNameLetter_SlideOut(PlaceNameLetter *letter, s16 startFrame, u16 frames) {
    letter->startFrame = startFrame;
    letter->frames = frames;
    PlaceNameLetter_SetState(letter, PLACE_NAME_LETTER_STATE_SLIDE_OUT);
}

void PlaceNameLetter_Hide(PlaceNameLetter *letter) {
    PlaceNameLetter_SetState(letter, PLACE_NAME_LETTER_STATE_HIDE);
}

static void PlaceNameLetter_SetState(PlaceNameLetter *letter, s32 state) {
    if (letter->state != state) {
        letter->state = state;
        letter->step = 0;
        letter->changed = TRUE;
    }
}

BOOL PlaceNameLetter_IsFinished(PlaceNameLetter *letter) {
    if (letter->changed == 0) {
        return TRUE;
    }
    return FALSE;
}

static void PlaceNameText_Init(PlaceNameLetter *letter, void *work, const void *arg) {
    PlaceNameTextWork *wk = work;
    const PlaceNameTextSetup *setup = arg;
    BmpOamActorSetup oamSetup;

    wk->queue = setup->printQueue;
    ClActor_SetSequence(letter->actor, 0);
    wk->bitmap = GFL_BitmapCreate(25, 2, 32, letter->heapId);
    oamSetup.bitmap = wk->bitmap;
    oamSetup.x = 0;
    oamSetup.y = 0;
    oamSetup.palette = setup->palette;
    oamSetup.paletteOffset = 0;
    oamSetup.priority = 3;
    oamSetup.bgPriority = 1;
    oamSetup.surface = letter->bgPriority;
    oamSetup.vramType = 0;
    wk->oamActor = BmpOam_ActorAdd(setup->bmpOam, &oamSetup);
    BmpOam_ActorSetDrawEnable(wk->oamActor, FALSE);
    wk->palette = ClActSys_LoadPlttRange(letter->arc, 26, 0, 96, 0, 1, letter->heapId);
    wk->chars = ClActSys_LoadChars(letter->arc, 25, 0, 0, letter->heapId);
    wk->cellAnims = ClActSys_LoadCellAnims(letter->arc, 24, 23, letter->heapId);
    {
        ClActorSetup setup = { 0, 0, 0, 0, 0 };
        wk->actor = ClActor_Create(letter->unit, wk->chars, wk->palette, wk->cellAnims, &setup, 0, letter->heapId);
    }
    ClActor_SetVisible(wk->actor, FALSE);
    ClActor_SetBgPriority(wk->actor, 1);
    ClActor_SetPriority(wk->actor, 3);
}

static void PlaceNameText_Exit(PlaceNameLetter *letter, void *work) {
    PlaceNameTextWork *wk = work;

    if (wk->actor != NULL) {
        ClActor_Delete(wk->actor);
        wk->actor = NULL;
    }
    ClActSys_FreePltt(wk->palette);
    ClActSys_FreeChars(wk->chars);
    ClActSys_FreeCellAnims(wk->cellAnims);
    if (wk->oamActor != NULL) {
        BmpOam_ActorDel(wk->oamActor);
        wk->oamActor = NULL;
    }
    if (wk->bitmap != NULL) {
        GFL_BitmapFree(wk->bitmap);
        wk->bitmap = NULL;
    }
}

static void PlaceNameText_SetParam(PlaceNameLetter *letter, void *work, const void *arg) {
    PlaceNameTextWork *wk = work;
    const PlaceNameTextParam *text = arg;

    wk->text = *text;
    PlaceNameLetter_SetState(letter, PLACE_NAME_LETTER_STATE_SET);
}

static u32 PlaceNameText_Print(PlaceNameLetter *letter, PlaceNameTextWork *wk, s32 *step);
static u32 PlaceNameText_SlideIn(PlaceNameLetter *letter, PlaceNameTextWork *wk, s32 *step);
static u32 PlaceNameText_SlideOut(PlaceNameLetter *letter, PlaceNameTextWork *wk, s32 *step);
static u32 PlaceNameText_Hide(PlaceNameLetter *letter, PlaceNameTextWork *wk, s32 *step);

static u32 PlaceNameText_Update(PlaceNameLetter *letter, void *work) {
    u32 (*handlers[PLACE_NAME_LETTER_STATE_COUNT])(PlaceNameLetter *, PlaceNameTextWork *, s32 *) = {
        NULL, PlaceNameText_SlideIn, PlaceNameText_SlideOut, PlaceNameText_Hide, PlaceNameText_Print,
    };

    return handlers[letter->state](letter, work, &letter->step);
}

static u32 PlaceNameText_Print(PlaceNameLetter *letter, PlaceNameTextWork *wk, s32 *step) {
    switch (*step) {
    case PLACE_NAME_TEXT_PRINT_CLEAR:
        wk->iconId = GetZoneNameIcon(wk->text.zoneId);
        GFL_BitmapFill(wk->bitmap, 0);
        (*step)++;
        break;
    case PLACE_NAME_TEXT_PRINT_NAME: {
        u16 color = 0x440;
        if (getGameVersion() == VERSION_WHITE2) {
            color = 0x3c40;
        }
        PrintQueue_PrintColor(wk->queue, wk->bitmap, 0, 0, wk->text.name, wk->text.font, color);
        (*step)++;
        break;
    }
    case PLACE_NAME_TEXT_PRINT_TRANSFER:
        if (!PrintQueue_IsBitmapPending(wk->queue, wk->bitmap)) {
            BmpOam_ActorBmpTrans(wk->oamActor);
            (*step)++;
        }
        break;
    case PLACE_NAME_TEXT_PRINT_WAIT:
        (*step)++;
        break;
    case PLACE_NAME_TEXT_PRINT_ICON: {
        NNSG2dCharacterData *character;
        void *file = GFL_G2DIOReadOBJNCGRArc(letter->arc, wk->iconId + 2, FALSE, &character, letter->heapId);
        ClActSys_ReplaceCharData(wk->chars, character);
        GFL_HeapFree(file);
        (*step)++;
        break;
    }
    case PLACE_NAME_TEXT_PRINT_DONE:
        return FALSE;
    }
    return TRUE;
}

static void PlaceNameAnim_Init(Anim *anim, const AnimSpec *spec);
static void PlaceNameAnim_Update(Anim *anim);
static BOOL PlaceNameAnim_IsDone(Anim *anim);
static void PlaceNameAnim_GetPos(Anim *anim, ClActorPos *pos);
static void PlaceNameMover_Init(Mover *mover, const MoveSpec *spec);
static void PlaceNameMover_Update(Mover *mover);
static BOOL PlaceNameMover_IsDone(Mover *mover);
static void PlaceNameMover_GetPos(Mover *mover, ClActorPos *pos);
static void PlaceNameText_SetPos(PlaceNameLetter *letter, PlaceNameTextWork *wk, const ClActorPos *pos);
static void PlaceNameFrame_SetPos(PlaceNameLetter *letter, PlaceNameFrameWork *wk, const ClActorPos *pos);

static u32 PlaceNameText_SlideIn(PlaceNameLetter *letter, PlaceNameTextWork *wk, s32 *step) {
    // The slide in as a plain move, on the path of PLACE_NAME_LETTER_TEXT_ANIM_IN's. Nothing reads it, so which
    // function held it is a guess, one that gives the original's .rodata
    static const MoveSpec sMoveIn = { { { 0, -0x20 }, { 0, 0 } }, 0, 0 };

    switch (*step) {
    case PLACE_NAME_LETTER_SLIDE_START: {
        AnimSpec spec = PLACE_NAME_LETTER_TEXT_ANIM_IN;

        spec.moveStart = letter->startFrame;
        spec.moveFrames = (s16)(letter->frames * 0.6f);
        spec.swingStart =
            letter->startFrame >= spec.moveFrames ? letter->startFrame - spec.moveFrames : letter->startFrame;
        spec.swingFrames = (s16)(letter->frames * 0.4f);
        PlaceNameAnim_Init(&wk->anim, &spec);
        BmpOam_ActorSetDrawEnable(wk->oamActor, TRUE);
        ClActor_SetVisible(letter->actor, TRUE);
        ClActor_SetVisible(wk->actor, TRUE);
        (*step)++;
        break;
    }
    case PLACE_NAME_LETTER_SLIDE_MOVE:
        PlaceNameAnim_Update(&wk->anim);
        if (PlaceNameAnim_IsDone(&wk->anim)) {
            (*step)++;
        }
        break;
    case PLACE_NAME_LETTER_SLIDE_DONE:
        return FALSE;
    }
    PlaceNameAnim_GetPos(&wk->anim, &wk->pos);
    PlaceNameText_SetPos(letter, wk, &wk->pos);
    return TRUE;
}

static u32 PlaceNameText_SlideOut(PlaceNameLetter *letter, PlaceNameTextWork *wk, s32 *step) {
    switch (*step) {
    case PLACE_NAME_LETTER_SLIDE_START: {
        MoveSpec spec = PLACE_NAME_LETTER_TEXT_MOVE_OUT;

        spec.path.start.x = wk->pos.x;
        spec.path.start.y = wk->pos.y;
        spec.startFrame = letter->startFrame;
        spec.frames = letter->frames;
        PlaceNameMover_Init(&wk->mover, &spec);
        (*step)++;
        break;
    }
    case PLACE_NAME_LETTER_SLIDE_MOVE:
        PlaceNameMover_Update(&wk->mover);
        if (PlaceNameMover_IsDone(&wk->mover)) {
            ClActor_SetVisible(letter->actor, FALSE);
            ClActor_SetVisible(wk->actor, FALSE);
            BmpOam_ActorSetDrawEnable(wk->oamActor, FALSE);
            (*step)++;
        }
        break;
    case PLACE_NAME_LETTER_SLIDE_DONE:
        return FALSE;
    }
    PlaceNameMover_GetPos(&wk->mover, &wk->pos);
    PlaceNameText_SetPos(letter, wk, &wk->pos);
    return TRUE;
}

static u32 PlaceNameText_Hide(PlaceNameLetter *letter, PlaceNameTextWork *wk, s32 *step) {
    ClActor_SetVisible(letter->actor, FALSE);
    ClActor_SetVisible(wk->actor, FALSE);
    BmpOam_ActorSetDrawEnable(wk->oamActor, FALSE);
    return FALSE;
}

static void PlaceNameText_SetPos(PlaceNameLetter *letter, PlaceNameTextWork *wk, const ClActorPos *pos) {
    ClActorPos letterPos;
    ClActorPos oamPos;
    ClActorPos actorPos;
    letterPos = PLACE_NAME_LETTER_POSITIONS[4];
    oamPos = PLACE_NAME_LETTER_POSITIONS[2];
    actorPos = PLACE_NAME_LETTER_POSITIONS[0];
    letterPos.x += pos->x;
    letterPos.y += pos->y;
    ClActor_SetSurfacePos(letter->actor, &letterPos, letter->bgPriority);
    actorPos.x += pos->x;
    actorPos.y += pos->y;
    ClActor_SetSurfacePos(wk->actor, &actorPos, letter->bgPriority);
    oamPos.x += pos->x;
    oamPos.y += pos->y;
    BmpOam_ActorSetPos(wk->oamActor, oamPos.x, oamPos.y);
}

static void PlaceNameFrame_Init(PlaceNameLetter *letter, void *work, const void *arg) {
    PlaceNameFrameWork *wk = work;
    const PlaceNameFrameSetup *frame = arg;

    ClActor_SetSequence(letter->actor, 1);
    {
        ClActorSetup setup = { 0, 0, 0, 0, 0 };
        wk->actor =
            ClActor_Create(letter->unit, frame->chars, frame->palette, frame->cellAnims, &setup, 0, letter->heapId);
    }
    ClActor_SetVisible(wk->actor, FALSE);
    ClActor_SetBgPriority(wk->actor, 1);
    ClActor_SetPriority(wk->actor, 3);
}

static void PlaceNameFrame_Exit(PlaceNameLetter *letter, void *work) {
    PlaceNameFrameWork *wk = work;

    if (wk->actor != NULL) {
        ClActor_Delete(wk->actor);
        wk->actor = NULL;
    }
}

static void PlaceNameFrame_SetSeason(PlaceNameLetter *letter, void *work, const void *arg) {
    PlaceNameFrameWork *wk = work;
    const u32 *season = arg;

    wk->iconId = *season;
    PlaceNameLetter_SetState(letter, PLACE_NAME_LETTER_STATE_SET);
}

static u32 PlaceNameFrame_SetIcon(PlaceNameLetter *letter, PlaceNameFrameWork *wk, s32 *step);
static u32 PlaceNameFrame_SlideIn(PlaceNameLetter *letter, PlaceNameFrameWork *wk, s32 *step);
static u32 PlaceNameFrame_SlideOut(PlaceNameLetter *letter, PlaceNameFrameWork *wk, s32 *step);
static u32 PlaceNameFrame_Hide(PlaceNameLetter *letter, PlaceNameFrameWork *wk, s32 *step);

static u32 PlaceNameFrame_Update(PlaceNameLetter *letter, void *work) {
    u32 (*handlers[PLACE_NAME_LETTER_STATE_COUNT])(PlaceNameLetter *, PlaceNameFrameWork *, s32 *) = {
        NULL, PlaceNameFrame_SlideIn, PlaceNameFrame_SlideOut, PlaceNameFrame_Hide, PlaceNameFrame_SetIcon,
    };

    return handlers[letter->state](letter, work, &letter->step);
}

static u32 PlaceNameFrame_SetIcon(PlaceNameLetter *letter, PlaceNameFrameWork *wk, s32 *step) {
    ClActor_SetSequence(wk->actor, (u16)(wk->iconId + 2));
    return FALSE;
}

static u32 PlaceNameFrame_SlideIn(PlaceNameLetter *letter, PlaceNameFrameWork *wk, s32 *step) {
    // The slide in as a plain move, on the path of PLACE_NAME_LETTER_FRAME_ANIM_IN's. Nothing reads it either
    static const MoveSpec sMoveIn = { { { 0, 0x20 }, { 0, 0 } }, 0, 0 };

    switch (*step) {
    case PLACE_NAME_LETTER_SLIDE_START: {
        AnimSpec spec = PLACE_NAME_LETTER_FRAME_ANIM_IN;

        spec.moveStart = letter->startFrame;
        spec.moveFrames = (s16)(letter->frames * 0.6f);
        spec.swingStart =
            letter->startFrame >= spec.moveFrames ? letter->startFrame - spec.moveFrames : letter->startFrame;
        spec.swingFrames = (s16)(letter->frames * 0.4f);
        PlaceNameAnim_Init(&wk->anim, &spec);
        ClActor_SetVisible(letter->actor, TRUE);
        ClActor_SetVisible(wk->actor, TRUE);
        (*step)++;
        break;
    }
    case PLACE_NAME_LETTER_SLIDE_MOVE:
        PlaceNameAnim_Update(&wk->anim);
        if (PlaceNameAnim_IsDone(&wk->anim)) {
            (*step)++;
        }
        break;
    case PLACE_NAME_LETTER_SLIDE_DONE:
        return FALSE;
    }
    PlaceNameAnim_GetPos(&wk->anim, &wk->pos);
    PlaceNameFrame_SetPos(letter, wk, &wk->pos);
    return TRUE;
}

static u32 PlaceNameFrame_SlideOut(PlaceNameLetter *letter, PlaceNameFrameWork *wk, s32 *step) {
    switch (*step) {
    case PLACE_NAME_LETTER_SLIDE_START: {
        MoveSpec spec = PLACE_NAME_LETTER_FRAME_MOVE_OUT;

        spec.path.start.x = wk->pos.x;
        spec.path.start.y = wk->pos.y;
        spec.startFrame = letter->startFrame;
        spec.frames = letter->frames;
        PlaceNameMover_Init(&wk->mover, &spec);
        (*step)++;
        break;
    }
    case PLACE_NAME_LETTER_SLIDE_MOVE:
        PlaceNameMover_Update(&wk->mover);
        if (PlaceNameMover_IsDone(&wk->mover)) {
            ClActor_SetVisible(letter->actor, FALSE);
            ClActor_SetVisible(wk->actor, FALSE);
            (*step)++;
        }
        break;
    case PLACE_NAME_LETTER_SLIDE_DONE:
        return FALSE;
    }
    PlaceNameMover_GetPos(&wk->mover, &wk->pos);
    PlaceNameFrame_SetPos(letter, wk, &wk->pos);
    return TRUE;
}

static u32 PlaceNameFrame_Hide(PlaceNameLetter *letter, PlaceNameFrameWork *wk, s32 *step) {
    ClActor_SetVisible(letter->actor, FALSE);
    ClActor_SetVisible(wk->actor, FALSE);
    return FALSE;
}

static void PlaceNameFrame_SetPos(PlaceNameLetter *letter, PlaceNameFrameWork *wk, const ClActorPos *pos) {
    ClActorPos letterPos;
    ClActorPos actorPos;

    letterPos = PLACE_NAME_LETTER_POSITIONS[1];
    actorPos = PLACE_NAME_LETTER_POSITIONS[3];
    letterPos.x += pos->x;
    letterPos.y += pos->y;
    ClActor_SetSurfacePos(letter->actor, &letterPos, letter->bgPriority);
    actorPos.x += pos->x;
    actorPos.y += pos->y;
    ClActor_SetSurfacePos(wk->actor, &actorPos, letter->bgPriority);
}

static void PlaceNameMover_Init(Mover *mover, const MoveSpec *spec) {
    MI_CpuFill8(mover, 0, sizeof(Mover));
    mover->frame = spec->startFrame;
    mover->x = spec->path.start.x;
    mover->y = spec->path.start.y;
    mover->spec = *spec;
}

static void PlaceNameMover_Update(Mover *mover) {
    s32 frame = mover->frame++;

    if (frame < mover->spec.frames) {
        s32 x0 = mover->spec.path.start.x;
        s32 y0;

        mover->x = x0 + (mover->spec.path.end.x - x0) * mover->frame / mover->spec.frames;
        y0 = mover->spec.path.start.y;
        mover->y = y0 + (mover->spec.path.end.y - y0) * mover->frame / mover->spec.frames;
        return;
    }
    mover->x = mover->spec.path.end.x;
    mover->y = mover->spec.path.end.y;
}

static BOOL PlaceNameMover_IsDone(Mover *mover) {
    if (mover->frame > mover->spec.frames) {
        return TRUE;
    }
    return FALSE;
}

static void PlaceNameMover_GetPos(Mover *mover, ClActorPos *pos) {
    pos->x = mover->x;
    pos->y = mover->y;
}

static void PlaceNameSwing_Init(Swing *swing, const SwingSpec *spec) {
    MI_CpuFill8(swing, 0, sizeof(Swing));
    // BUG: The frame and the sine are read from the swing's spec before the spec is copied in, so every swing starts
    // at frame 0 rather than its start frame
#ifdef BUGFIX
    swing->frame = spec->startFrame;
    swing->sin = FX_SinIdx(spec->startAngle);
#else
    swing->frame = swing->spec.startFrame;
    swing->sin = FX_SinIdx(swing->spec.startAngle);
#endif
    swing->spec = *spec;
}

static void PlaceNameSwing_Update(Swing *swing) {
    s32 frame = swing->frame++;

    if (frame < swing->spec.frames) {
        if (swing->spec.direction > 0) {
            swing->sin = FX_SinIdx(swing->spec.startAngle +
                                   (swing->spec.endAngle - swing->spec.startAngle) * swing->frame / swing->spec.frames);
        } else {
            swing->sin = FX_SinIdx(swing->spec.startAngle - (0xffff - swing->spec.endAngle + swing->spec.startAngle) *
                                                                swing->frame / swing->spec.frames);
        }
        return;
    }
    swing->sin = FX_SinIdx(swing->spec.endAngle);
}

static BOOL PlaceNameSwing_IsDone(Swing *swing) {
    if (swing->frame > swing->spec.frames) {
        return TRUE;
    }
    return FALSE;
}

static void PlaceNameSwing_GetPos(const Swing *swing, ClActorPos *pos) {
    pos->x = swing->spec.baseX + (swing->spec.ampX * swing->sin >> 12);
    pos->y = swing->spec.baseY + (swing->spec.ampY * swing->sin >> 12);
}

static void PlaceNameAnim_Init(Anim *anim, const AnimSpec *spec) {
    MoveSpec move;

    MI_CpuFill8(anim, 0, sizeof(Anim));
    anim->done = 0;
    anim->spec = *spec;
    MI_CpuFill8(&move, 0, sizeof(MoveSpec));
    move.path.start = anim->spec.path.start;
    move.path.end = anim->spec.path.end;
    move.startFrame = anim->spec.moveStart;
    move.frames = anim->spec.moveFrames;
    PlaceNameMover_Init(&anim->mover, &move);
}

static void PlaceNameAnim_Update(Anim *anim) {
    switch (anim->stage) {
    case PLACE_NAME_ANIM_STAGE_MOVE:
        PlaceNameMover_Update(&anim->mover);
        if (PlaceNameMover_IsDone(&anim->mover)) {
            SwingSpec swing;

            MI_CpuFill8(&swing, 0, sizeof(SwingSpec));
            swing.startAngle = anim->spec.startAngle;
            swing.endAngle = anim->spec.endAngle;
            swing.startFrame = anim->spec.swingStart;
            swing.frames = anim->spec.swingFrames;
            swing.baseX = anim->spec.path.end.x;
            swing.baseY = anim->spec.path.end.y;
            swing.ampX = anim->spec.ampX;
            swing.ampY = anim->spec.ampY;
            swing.direction = anim->spec.direction;
            PlaceNameSwing_Init(&anim->swing, &swing);
            anim->stage++;
        }
        break;
    case PLACE_NAME_ANIM_STAGE_SWING:
        PlaceNameSwing_Update(&anim->swing);
        if (PlaceNameSwing_IsDone(&anim->swing)) {
            anim->stage++;
        }
        break;
    default:
        anim->done = 1;
        break;
    }
}

static BOOL PlaceNameAnim_IsDone(Anim *anim) {
    return anim->done;
}

static void PlaceNameAnim_GetPos(Anim *anim, ClActorPos *pos) {
    switch (anim->stage) {
    case PLACE_NAME_ANIM_STAGE_MOVE:
        PlaceNameMover_GetPos(&anim->mover, pos);
        break;
    case PLACE_NAME_ANIM_STAGE_SWING:
    default:
        PlaceNameSwing_GetPos(&anim->swing, pos);
        break;
    }
}
