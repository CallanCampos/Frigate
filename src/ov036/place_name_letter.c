// place_name_letter.c: the two animated letters of a place name sign: the zone's icon and name text that slide in
// and out on the field. The name is the ROM's own, from GFL_HeapAllocate's file argument
#include "types.h"
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

typedef void (*LetterInitFn)(PlaceNameLetter *letter, void *work, void *arg);
typedef void (*LetterExitFn)(PlaceNameLetter *letter, void *work);
typedef u32 (*LetterUpdateFn)(PlaceNameLetter *letter, void *work);

typedef struct {
    u32 size;
    LetterInitFn init;
    LetterExitFn exit;
    LetterInitFn set;
    LetterUpdateFn update;
} PlaceNameLetterOps;

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
    u16 param0;
    u16 param1;
};

typedef struct {
    s32 x0;
    s32 y0;
    s32 x1;
    s32 y1;
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
} LetterA;

typedef struct {
    u32 iconId;
    Anim anim;
    Mover mover;
    ClActor *actor;
    ClActorPos pos;
} LetterB;

static const ClActorPos PLACE_NAME_LETTER_POSITIONS[5] = {
    { 0x10, 8 }, { 0x80, 0x60 }, { 0x1e, 3 }, { 0xe2, 0xbc }, { 0x80, 0x60 }
};
// Two moves that no code uses
const MoveSpec PLACE_NAME_LETTER_UNUSED_MOVE_A = { { 0, 0x20, 0, 0 }, 0, 0 };
const MoveSpec PLACE_NAME_LETTER_UNUSED_MOVE_B = { { 0, -0x20, 0, 0 }, 0, 0 };

static void func_ov036_021b6950(PlaceNameLetter *letter, void *work, void *arg);
static void func_ov036_021b6a3c(PlaceNameLetter *letter, void *work);
static void func_ov036_021b6a94(PlaceNameLetter *letter, void *work, void *arg);
static u32 func_ov036_021b6aac(PlaceNameLetter *letter, void *work);
static void func_ov036_021b6dec(PlaceNameLetter *letter, void *work, void *arg);
static void func_ov036_021b6e44(PlaceNameLetter *letter, void *work);
static void func_ov036_021b6e60(PlaceNameLetter *letter, void *work, void *arg);
static u32 func_ov036_021b6e70(PlaceNameLetter *letter, void *work);
static void func_ov036_021b692c(PlaceNameLetter *letter, s32 state);

static const PlaceNameLetterOps PLACE_NAME_LETTER_OPS[2] = {
    { sizeof(LetterA), func_ov036_021b6950, func_ov036_021b6a3c, func_ov036_021b6a94, func_ov036_021b6aac },
    { sizeof(LetterB), func_ov036_021b6dec, func_ov036_021b6e44, func_ov036_021b6e60, func_ov036_021b6e70 },
};

PlaceNameLetter *func_ov036_021b6824(HeapID heapId, u32 type, void *arg, ClActUnit *unit, ArcTool *arc, u16 bgPriority,
                                     u16 palette, u16 chars, u16 cellAnims) {
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
        letter->actor = func_0204c040(unit, chars, palette, cellAnims, &setup, 0, heapId);
    }
    func_0204c318(letter->actor, 2);
    func_0204c124(letter->actor, FALSE);
    letter->ops->init(letter, letter->work, arg);
    return letter;
}

void func_ov036_021b68bc(PlaceNameLetter *letter) {
    letter->ops->exit(letter, letter->work);
    func_0204c108(letter->actor);
    GFL_HeapFree(letter);
}

void func_ov036_021b68d8(PlaceNameLetter *letter, void *arg) {
    letter->ops->set(letter, letter->work, arg);
}

void func_ov036_021b68e8(PlaceNameLetter *letter) {
    if (letter->changed != 0) {
        letter->changed = letter->ops->update(letter, letter->work);
    }
}

void func_ov036_021b6900(PlaceNameLetter *letter, s16 param0, u16 param1) {
    letter->param0 = param0;
    letter->param1 = param1;
    func_ov036_021b692c(letter, 1);
}

void func_ov036_021b6910(PlaceNameLetter *letter, s16 param0, u16 param1) {
    letter->param0 = param0;
    letter->param1 = param1;
    func_ov036_021b692c(letter, 2);
}

void func_ov036_021b6920(PlaceNameLetter *letter) {
    func_ov036_021b692c(letter, 3);
}

static void func_ov036_021b692c(PlaceNameLetter *letter, s32 state) {
    if (letter->state != state) {
        letter->state = state;
        letter->step = 0;
        letter->changed = TRUE;
    }
}

BOOL func_ov036_021b6940(PlaceNameLetter *letter) {
    if (letter->changed == 0) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_021b6950(PlaceNameLetter *letter, void *work, void *arg) {
    LetterA *wk = work;
    PlaceNameTextSetup *setup = arg;
    BmpOamActorSetup oamSetup;

    wk->queue = setup->printQueue;
    func_0204c488(letter->actor, 0);
    wk->bitmap = GFL_BitmapCreate(25, 2, 32, letter->heapId);
    oamSetup.bitmap = wk->bitmap;
    oamSetup.x = 0;
    oamSetup.y = 0;
    oamSetup.paletteOffset = 0;
    oamSetup.palette = setup->palette;
    oamSetup.priority = 3;
    oamSetup.bgPriority = 1;
    oamSetup.surface = letter->bgPriority;
    oamSetup.vramType = 0;
    wk->oamActor = BmpOam_ActorAdd(setup->bmpOam, &oamSetup);
    BmpOam_ActorSetDrawEnable(wk->oamActor, FALSE);
    wk->palette = func_0204bbb8(letter->arc, 26, 0, 96, 0, 1, letter->heapId);
    wk->chars = func_0204b81c(letter->arc, 25, 0, 0, letter->heapId);
    wk->cellAnims = func_0204bde0(letter->arc, 24, 23, letter->heapId);
    {
        ClActorSetup setup = { 0, 0, 0, 0, 0 };
        wk->actor = func_0204c040(letter->unit, wk->chars, wk->palette, wk->cellAnims, &setup, 0, letter->heapId);
    }
    func_0204c124(wk->actor, FALSE);
    func_0204c468(wk->actor, 1);
    func_0204c438(wk->actor, 3);
}

static void func_ov036_021b6a3c(PlaceNameLetter *letter, void *work) {
    LetterA *wk = work;

    if (wk->actor != NULL) {
        func_0204c108(wk->actor);
        wk->actor = NULL;
    }
    func_0204bcd0(wk->palette);
    func_0204b98c(wk->chars);
    func_0204be64(wk->cellAnims);
    if (wk->oamActor != NULL) {
        BmpOam_ActorDel(wk->oamActor);
        wk->oamActor = NULL;
    }
    if (wk->bitmap != NULL) {
        GFL_BitmapFree(wk->bitmap);
        wk->bitmap = NULL;
    }
}

static void func_ov036_021b6a94(PlaceNameLetter *letter, void *work, void *arg) {
    LetterA *wk = work;
    PlaceNameTextParam *text = arg;

    wk->text = *text;
    func_ov036_021b692c(letter, 4);
}

static u32 func_ov036_021b6adc(PlaceNameLetter *letter, LetterA *wk, s32 *step);
static u32 func_ov036_021b6b9c(PlaceNameLetter *letter, LetterA *wk, s32 *step);
static u32 func_ov036_021b6c7c(PlaceNameLetter *letter, LetterA *wk, s32 *step);
static u32 func_ov036_021b6d2c(PlaceNameLetter *letter, LetterA *wk, s32 *step);

static u32 func_ov036_021b6aac(PlaceNameLetter *letter, void *work) {
    u32 (*handlers[5])(PlaceNameLetter *, LetterA *, s32 *) = {
        NULL, func_ov036_021b6b9c, func_ov036_021b6c7c, func_ov036_021b6d2c, func_ov036_021b6adc,
    };

    return handlers[letter->state](letter, work, &letter->step);
}

static u32 func_ov036_021b6adc(PlaceNameLetter *letter, LetterA *wk, s32 *step) {
    switch (*step) {
    case 0:
        wk->iconId = GetZoneNameIcon(wk->text.zoneId);
        GFL_BitmapFill(wk->bitmap, 0);
        (*step)++;
        break;
    case 1: {
        u16 color = 0x440;
        if (getGameVersion() == 0x16) {
            color = 0x3c40;
        }
        func_02021c7c(wk->queue, wk->bitmap, 0, 0, wk->text.name, wk->text.font, color);
        (*step)++;
        break;
    }
    case 2:
        if (!func_02021c1c(wk->queue, wk->bitmap)) {
            BmpOam_ActorBmpTrans(wk->oamActor);
            (*step)++;
        }
        break;
    case 3:
        (*step)++;
        break;
    case 4: {
        NNSG2dCharacterData *character;
        void *file = GFL_G2DIOReadOBJNCGRArc(letter->arc, wk->iconId + 2, FALSE, &character, letter->heapId);
        func_0204ba40(wk->chars, character);
        GFL_HeapFree(file);
        (*step)++;
        break;
    }
    case 5:
        return FALSE;
    }
    return TRUE;
}

static void func_ov036_021b7224(Anim *anim, const AnimSpec *spec);
static void func_ov036_021b7280(Anim *anim);
static BOOL func_ov036_021b731c(Anim *anim);
static void func_ov036_021b7320(Anim *anim, ClActorPos *pos);
static void func_ov036_021b70bc(Mover *mover, const MoveSpec *spec);
static void func_ov036_021b70e8(Mover *mover);
static BOOL func_ov036_021b712c(Mover *mover);
static void func_ov036_021b7140(Mover *mover, ClActorPos *pos);
static void func_ov036_021b6d50(PlaceNameLetter *letter, LetterA *wk, ClActorPos *pos);
static void func_ov036_021b704c(PlaceNameLetter *letter, LetterB *wk, ClActorPos *pos);

static u32 func_ov036_021b6b9c(PlaceNameLetter *letter, LetterA *wk, s32 *step) {
    switch (*step) {
    case 0: {
        AnimSpec spec = { { 0, -0x20, 0, 0 }, 0, 0x7fff, 0, -3, 1, 0, 0, 0, 0, 0 };

        spec.moveStart = letter->param0;
        spec.moveFrames = (s16)(letter->param1 * 0.6f);
        spec.swingStart = letter->param0 >= spec.moveFrames ? letter->param0 - spec.moveFrames : letter->param0;
        spec.swingFrames = (s16)(letter->param1 * 0.4f);
        func_ov036_021b7224(&wk->anim, &spec);
        BmpOam_ActorSetDrawEnable(wk->oamActor, TRUE);
        func_0204c124(letter->actor, TRUE);
        func_0204c124(wk->actor, TRUE);
        (*step)++;
        break;
    }
    case 1:
        func_ov036_021b7280(&wk->anim);
        if (func_ov036_021b731c(&wk->anim)) {
            (*step)++;
        }
        break;
    case 2:
        return FALSE;
    }
    func_ov036_021b7320(&wk->anim, &wk->pos);
    func_ov036_021b6d50(letter, wk, &wk->pos);
    return TRUE;
}

static u32 func_ov036_021b6c7c(PlaceNameLetter *letter, LetterA *wk, s32 *step) {
    switch (*step) {
    case 0: {
        MoveSpec spec = { { 0, 0, 0, -0x40 }, 0, 0 };

        spec.path.x0 = wk->pos.x;
        spec.path.y0 = wk->pos.y;
        spec.startFrame = letter->param0;
        spec.frames = letter->param1;
        func_ov036_021b70bc(&wk->mover, &spec);
        (*step)++;
        break;
    }
    case 1:
        func_ov036_021b70e8(&wk->mover);
        if (func_ov036_021b712c(&wk->mover)) {
            func_0204c124(letter->actor, FALSE);
            func_0204c124(wk->actor, FALSE);
            BmpOam_ActorSetDrawEnable(wk->oamActor, FALSE);
            (*step)++;
        }
        break;
    case 2:
        return FALSE;
    }
    func_ov036_021b7140(&wk->mover, &wk->pos);
    func_ov036_021b6d50(letter, wk, &wk->pos);
    return TRUE;
}

static u32 func_ov036_021b6d2c(PlaceNameLetter *letter, LetterA *wk, s32 *step) {
    func_0204c124(letter->actor, FALSE);
    func_0204c124(wk->actor, FALSE);
    BmpOam_ActorSetDrawEnable(wk->oamActor, FALSE);
    return FALSE;
}

static void func_ov036_021b6d50(PlaceNameLetter *letter, LetterA *wk, ClActorPos *pos) {
    ClActorPos letterPos;
    ClActorPos oamPos;
    ClActorPos actorPos;
    letterPos = PLACE_NAME_LETTER_POSITIONS[4];
    oamPos = PLACE_NAME_LETTER_POSITIONS[2];
    actorPos = PLACE_NAME_LETTER_POSITIONS[0];
    letterPos.x += pos->x;
    letterPos.y += pos->y;
    func_0204c140(letter->actor, &letterPos, letter->bgPriority);
    actorPos.x += pos->x;
    actorPos.y += pos->y;
    func_0204c140(wk->actor, &actorPos, letter->bgPriority);
    oamPos.x += pos->x;
    oamPos.y += pos->y;
    BmpOam_ActorSetPos(wk->oamActor, oamPos.x, oamPos.y);
}

static void func_ov036_021b6dec(PlaceNameLetter *letter, void *work, void *arg) {
    LetterB *wk = work;
    PlaceNameFrameSetup *frame = arg;

    func_0204c488(letter->actor, 1);
    {
        ClActorSetup setup = { 0, 0, 0, 0, 0 };
        wk->actor =
            func_0204c040(letter->unit, frame->chars, frame->palette, frame->cellAnims, &setup, 0, letter->heapId);
    }
    func_0204c124(wk->actor, FALSE);
    func_0204c468(wk->actor, 1);
    func_0204c438(wk->actor, 3);
}

static void func_ov036_021b6e44(PlaceNameLetter *letter, void *work) {
    LetterB *wk = work;

    if (wk->actor != NULL) {
        func_0204c108(wk->actor);
        wk->actor = NULL;
    }
}

static void func_ov036_021b6e60(PlaceNameLetter *letter, void *work, void *arg) {
    LetterB *wk = work;
    u32 *season = arg;

    wk->iconId = *season;
    func_ov036_021b692c(letter, 4);
}

static u32 func_ov036_021b6ea0(PlaceNameLetter *letter, LetterB *wk, s32 *step);
static u32 func_ov036_021b6eb8(PlaceNameLetter *letter, LetterB *wk, s32 *step);
static u32 func_ov036_021b6f88(PlaceNameLetter *letter, LetterB *wk, s32 *step);
static u32 func_ov036_021b7030(PlaceNameLetter *letter, LetterB *wk, s32 *step);

static u32 func_ov036_021b6e70(PlaceNameLetter *letter, void *work) {
    u32 (*handlers[5])(PlaceNameLetter *, LetterB *, s32 *) = {
        NULL, func_ov036_021b6eb8, func_ov036_021b6f88, func_ov036_021b7030, func_ov036_021b6ea0,
    };

    return handlers[letter->state](letter, work, &letter->step);
}

static u32 func_ov036_021b6ea0(PlaceNameLetter *letter, LetterB *wk, s32 *step) {
    func_0204c488(wk->actor, (u16)(wk->iconId + 2));
    return FALSE;
}

static u32 func_ov036_021b6eb8(PlaceNameLetter *letter, LetterB *wk, s32 *step) {
    switch (*step) {
    case 0: {
        AnimSpec spec = { { 0, 0x20, 0, 0 }, 0, 0x7fff, 0, 3, 1, 0, 0, 0, 0, 0 };

        spec.moveStart = letter->param0;
        spec.moveFrames = (s16)(letter->param1 * 0.6f);
        spec.swingStart = letter->param0 >= spec.moveFrames ? letter->param0 - spec.moveFrames : letter->param0;
        spec.swingFrames = (s16)(letter->param1 * 0.4f);
        func_ov036_021b7224(&wk->anim, &spec);
        func_0204c124(letter->actor, TRUE);
        func_0204c124(wk->actor, TRUE);
        (*step)++;
        break;
    }
    case 1:
        func_ov036_021b7280(&wk->anim);
        if (func_ov036_021b731c(&wk->anim)) {
            (*step)++;
        }
        break;
    case 2:
        return FALSE;
    }
    func_ov036_021b7320(&wk->anim, &wk->pos);
    func_ov036_021b704c(letter, wk, &wk->pos);
    return TRUE;
}

static u32 func_ov036_021b6f88(PlaceNameLetter *letter, LetterB *wk, s32 *step) {
    switch (*step) {
    case 0: {
        MoveSpec spec = { { 0, 0, 0, 0x20 }, 0, 0 };

        spec.path.x0 = wk->pos.x;
        spec.path.y0 = wk->pos.y;
        spec.startFrame = letter->param0;
        spec.frames = letter->param1;
        func_ov036_021b70bc(&wk->mover, &spec);
        (*step)++;
        break;
    }
    case 1:
        func_ov036_021b70e8(&wk->mover);
        if (func_ov036_021b712c(&wk->mover)) {
            func_0204c124(letter->actor, FALSE);
            func_0204c124(wk->actor, FALSE);
            (*step)++;
        }
        break;
    case 2:
        return FALSE;
    }
    func_ov036_021b7140(&wk->mover, &wk->pos);
    func_ov036_021b704c(letter, wk, &wk->pos);
    return TRUE;
}

static u32 func_ov036_021b7030(PlaceNameLetter *letter, LetterB *wk, s32 *step) {
    func_0204c124(letter->actor, FALSE);
    func_0204c124(wk->actor, FALSE);
    return FALSE;
}

static void func_ov036_021b704c(PlaceNameLetter *letter, LetterB *wk, ClActorPos *pos) {
    ClActorPos letterPos;
    ClActorPos actorPos;

    letterPos = PLACE_NAME_LETTER_POSITIONS[1];
    actorPos = PLACE_NAME_LETTER_POSITIONS[3];
    letterPos.x += pos->x;
    letterPos.y += pos->y;
    func_0204c140(letter->actor, &letterPos, letter->bgPriority);
    actorPos.x += pos->x;
    actorPos.y += pos->y;
    func_0204c140(wk->actor, &actorPos, letter->bgPriority);
}

static void func_ov036_021b70bc(Mover *mover, const MoveSpec *spec) {
    MI_CpuFill8(mover, 0, sizeof(Mover));
    mover->frame = spec->startFrame;
    mover->x = spec->path.x0;
    mover->y = spec->path.y0;
    mover->spec = *spec;
}

static void func_ov036_021b70e8(Mover *mover) {
    s32 frame = mover->frame++;

    if (frame < mover->spec.frames) {
        s32 x0 = mover->spec.path.x0;
        s32 y0;

        mover->x = x0 + (mover->spec.path.x1 - x0) * mover->frame / mover->spec.frames;
        y0 = mover->spec.path.y0;
        mover->y = y0 + (mover->spec.path.y1 - y0) * mover->frame / mover->spec.frames;
        return;
    }
    mover->x = mover->spec.path.x1;
    mover->y = mover->spec.path.y1;
}

static BOOL func_ov036_021b712c(Mover *mover) {
    if (mover->frame > mover->spec.frames) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_021b7140(Mover *mover, ClActorPos *pos) {
    pos->x = mover->x;
    pos->y = mover->y;
}

static void func_ov036_021b7150(Swing *swing, const SwingSpec *spec) {
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

static void func_ov036_021b7184(Swing *swing) {
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

static BOOL func_ov036_021b71e8(Swing *swing) {
    if (swing->frame > swing->spec.frames) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_021b71f8(Swing *swing, ClActorPos *pos) {
    s32 sin = swing->sin;

    pos->x = swing->spec.baseX + (swing->spec.ampX * sin >> 12);
    pos->y = swing->spec.baseY + (swing->spec.ampY * sin >> 12);
}

static void func_ov036_021b7224(Anim *anim, const AnimSpec *spec) {
    MoveSpec move;

    MI_CpuFill8(anim, 0, sizeof(Anim));
    anim->done = 0;
    anim->spec = *spec;
    MI_CpuFill8(&move, 0, sizeof(MoveSpec));
    move.path.x0 = anim->spec.path.x0;
    move.path.y0 = anim->spec.path.y0;
    move.path.x1 = anim->spec.path.x1;
    move.path.y1 = anim->spec.path.y1;
    move.startFrame = anim->spec.moveStart;
    move.frames = anim->spec.moveFrames;
    func_ov036_021b70bc(&anim->mover, &move);
}

static void func_ov036_021b7280(Anim *anim) {
    switch (anim->stage) {
    case 0:
        func_ov036_021b70e8(&anim->mover);
        if (func_ov036_021b712c(&anim->mover)) {
            SwingSpec swing;

            MI_CpuFill8(&swing, 0, sizeof(SwingSpec));
            swing.startAngle = anim->spec.startAngle;
            swing.endAngle = anim->spec.endAngle;
            swing.startFrame = anim->spec.swingStart;
            swing.frames = anim->spec.swingFrames;
            swing.baseX = anim->spec.path.x1;
            swing.baseY = anim->spec.path.y1;
            swing.ampX = anim->spec.ampX;
            swing.ampY = anim->spec.ampY;
            swing.direction = anim->spec.direction;
            func_ov036_021b7150(&anim->swing, &swing);
            anim->stage++;
        }
        break;
    case 1:
        func_ov036_021b7184(&anim->swing);
        if (func_ov036_021b71e8(&anim->swing)) {
            anim->stage++;
        }
        break;
    default:
        anim->done = 1;
        break;
    }
}

static BOOL func_ov036_021b731c(Anim *anim) {
    return anim->done;
}

static void func_ov036_021b7320(Anim *anim, ClActorPos *pos) {
    switch (anim->stage) {
    case 0:
        func_ov036_021b7140(&anim->mover, pos);
        break;
    case 1:
    default:
        func_ov036_021b71f8(&anim->swing, pos);
        break;
    }
}
