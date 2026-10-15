#include "types.h"
#include "battle/btl_main.h"
#include "battle/btlv.h"
#include "battle/btlv_effect.h"
#include "battle/pokewood_cutin.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/palanm.h"

// Overlay 167's pokewood_cutin.c (named by its string): the cut-ins of the Pokestar Studios movies. A cut-in is a
// script, chosen by the movie and its scene, of steps that load the cell actors' graphics, move the actors in and
// out, print the scene's messages and play sounds.

#define CUTIN_ACTOR_COUNT 4
#define CUTIN_STEP_ARG_COUNT 8
// A script begins with 11 values, then each step is a command and its arguments
#define CUTIN_SCRIPT_HEADER_COUNT 11
#define CUTIN_STEP_SIZE (1 + CUTIN_STEP_ARG_COUNT)
// The screen's center, where a cut-in's actors move in to or out from
#define CUTIN_CENTER_X 128
#define CUTIN_CENTER_Y 96

// A loaded cut-in script
typedef struct {
    u16 *data;
    u32 size;
} CutinScript;

typedef struct {
    s32 x;
    s32 y;
} CutinPoint;

// A move from start to end in even steps
typedef struct {
    CutinPoint pos;
    CutinPoint start;
    CutinPoint end;
    // fx32
    CutinPoint step;
    s32 frame;
    s32 frames;
} CutinLinearMotion;

// A move from start to end that starts at a speed and accelerates
typedef struct {
    VecFx32 pos;
    VecFx32 start;
    VecFx32 end;
    VecFx32 dir;
    fx32 speed;
    fx32 accel;
    s32 frame;
    s32 frames;
} CutinAccelMotion;

// A nudge of an actor 10 pixels out and back
typedef struct {
    u32 seq;
    CutinPoint start;
    CutinPoint end;
} CutinNudge;

typedef BOOL (*CutinCommand)(PokewoodCutin *cutin, u16 *seq, const u16 *args);

struct PokewoodCutin {
    const BtlScriptedRules *rules;
    ClActUnit *unit;
    ClActRenderer *renderer;
    CutinScript *script;
    u16 seq;
    u16 cmdSeq;
    BOOL done;
    u16 scene;
    u16 step;
    CutinCommand cmd;
    HeapID heapId;
    ClActor *actors[CUTIN_ACTOR_COUNT];
    BtlvStringParam strParam;
    BtlvCore *viewCore;
    u16 args[CUTIN_STEP_ARG_COUNT];
    u16 palette;
    u16 chars;
    u16 cellAnims;
    u16 loaded;
    u16 gender;
    u16 choice;
    void *cellData;
    NNSG2dCellDataBank *cells;
    u32 wait;
    union {
        CutinLinearMotion linear;
        CutinAccelMotion accel;
    } motion;
    CutinNudge nudge;
};

// How a move command moves an actor: the start for a move to the screen's center or from it, the start for a move by
// a distance, and the move's update and position
typedef struct {
    void (*init)(PokewoodCutin *cutin, ClActor *actor, const u16 *args);
    void (*initDist)(PokewoodCutin *cutin, ClActor *actor, const u16 *args);
    BOOL (*update)(PokewoodCutin *cutin);
    void (*getPos)(PokewoodCutin *cutin, ClActorPos *pos);
} CutinMoveFuncs;

typedef struct {
    void (*init)(PokewoodCutin *cutin, CutinNudge *nudge, ClActor *actor);
    BOOL (*update)(PokewoodCutin *cutin, CutinNudge *nudge);
    void (*getPos)(PokewoodCutin *cutin, CutinNudge *nudge, ClActorPos *pos);
} CutinNudgeFuncs;

// The cut-ins of each movie: the scene after which each plays, and whether it plays on the scene's result 1 (type
// 0), on any (1) or on another (2). The index is the script's file
typedef struct {
    u32 movie : 12;
    u32 scene : 10;
    u32 type : 10;
} CutinEntry;

static void Cutin_LoadResources(PokewoodCutin *cutin, u32 paletteFile, u32 charFile, u32 cellFile, u32 animFile);
static void Cutin_UnloadResources(PokewoodCutin *cutin);
static u32 Cutin_FindScript(s16 movie, u16 scene);
static BOOL Cutin_CmdMove(PokewoodCutin *cutin, u16 *seq, const u16 *args);
static BOOL Cutin_CmdMoveDist(PokewoodCutin *cutin, u16 *seq, const u16 *args);
static BOOL Cutin_CmdSetVisible(PokewoodCutin *cutin, u16 *seq, const u16 *args);
static BOOL Cutin_CmdNudge(PokewoodCutin *cutin, u16 *seq, const u16 *args);
static BOOL Cutin_CmdLoadResources(PokewoodCutin *cutin, u16 *seq, const u16 *args);
static BOOL Cutin_CmdWait(PokewoodCutin *cutin, u16 *seq, const u16 *args);
static BOOL Cutin_CmdPrintMessage(PokewoodCutin *cutin, u16 *seq, const u16 *args);
static BOOL Cutin_CmdViewEffect(PokewoodCutin *cutin, u16 *seq, const u16 *args);
static BOOL Cutin_CmdPlaySE(PokewoodCutin *cutin, u16 *seq, const u16 *args);
static BOOL Cutin_CmdWaitSound(PokewoodCutin *cutin, u16 *seq, const u16 *args);
static BOOL Cutin_CmdStopSound(PokewoodCutin *cutin, u16 *seq, const u16 *args);
static void Cutin_LinearInit(PokewoodCutin *cutin, ClActor *actor, const u16 *args);
static void Cutin_LinearInitDist(PokewoodCutin *cutin, ClActor *actor, const u16 *args);
static BOOL Cutin_LinearUpdate(PokewoodCutin *cutin);
static void Cutin_LinearGetPos(PokewoodCutin *cutin, ClActorPos *pos);
static void Cutin_AccelInit(PokewoodCutin *cutin, ClActor *actor, const u16 *args);
static void Cutin_AccelInitDist(PokewoodCutin *cutin, ClActor *actor, const u16 *args);
static BOOL Cutin_AccelUpdate(PokewoodCutin *cutin);
static void Cutin_AccelGetPos(PokewoodCutin *cutin, ClActorPos *pos);
static void Cutin_GetCenterMove(u16 dir, ClActor *actor, CutinPoint *start, CutinPoint *end);
static void Cutin_GetDistMove(u16 dir, u16 dist, ClActor *actor, CutinPoint *start, CutinPoint *end);
static void Cutin_NudgeUpInit(PokewoodCutin *cutin, CutinNudge *nudge, ClActor *actor);
static void Cutin_NudgeDownInit(PokewoodCutin *cutin, CutinNudge *nudge, ClActor *actor);
static void Cutin_NudgeLeftInit(PokewoodCutin *cutin, CutinNudge *nudge, ClActor *actor);
static void Cutin_NudgeRightInit(PokewoodCutin *cutin, CutinNudge *nudge, ClActor *actor);
static BOOL Cutin_NudgeUpdate(PokewoodCutin *cutin, CutinNudge *nudge);
static void Cutin_NudgeGetPos(PokewoodCutin *cutin, CutinNudge *nudge, ClActorPos *pos);
static void LinearMotion_Init(CutinLinearMotion *motion, const CutinPoint *start, const CutinPoint *end, s32 frames);
static BOOL LinearMotion_Update(CutinLinearMotion *motion);
static void LinearMotion_GetPos(const CutinLinearMotion *motion, ClActorPos *pos);
static void AccelMotion_Init(CutinAccelMotion *motion, const CutinPoint *start, const CutinPoint *end, fx32 speed,
                             s32 frames);
static BOOL AccelMotion_Update(CutinAccelMotion *motion);
static void AccelMotion_GetPos(const CutinAccelMotion *motion, ClActorPos *pos);
static CutinScript *CutinScript_Load(u32 fileId, HeapID heapId);
static void CutinScript_Free(CutinScript *script);
static u16 CutinScript_GetHeader(CutinScript *script, u32 index);
static u16 CutinScript_GetStepValue(CutinScript *script, u16 step, u32 index);
static u32 CutinScript_GetStepCount(CutinScript *script);

static const ClActorSetup sActorSetup = { 0, 0, 0, 0, 1 };

static const ClActSurfaceSetup sSurfaceSetup = { 0, 0, 256, 192, 0, 0 };

static const CutinMoveFuncs sMoveFuncs[] = {
    { Cutin_LinearInit, Cutin_LinearInitDist, Cutin_LinearUpdate, Cutin_LinearGetPos },
    { Cutin_AccelInit, Cutin_AccelInitDist, Cutin_AccelUpdate, Cutin_AccelGetPos },
};

static const CutinCommand sCommands[] = {
    Cutin_CmdMove,          Cutin_CmdMoveDist,  Cutin_CmdSetVisible,   Cutin_CmdNudge,
    Cutin_CmdLoadResources, Cutin_CmdWait,      Cutin_CmdPrintMessage, Cutin_CmdViewEffect,
    Cutin_CmdPlaySE,        Cutin_CmdWaitSound, Cutin_CmdStopSound,
};

static const CutinNudgeFuncs sNudgeFuncs[] = {
    { Cutin_NudgeUpInit, Cutin_NudgeUpdate, Cutin_NudgeGetPos },
    { Cutin_NudgeDownInit, Cutin_NudgeUpdate, Cutin_NudgeGetPos },
    { Cutin_NudgeLeftInit, Cutin_NudgeUpdate, Cutin_NudgeGetPos },
    { Cutin_NudgeRightInit, Cutin_NudgeUpdate, Cutin_NudgeGetPos },
};

static const CutinEntry sCutins[] = {
    { 1, 1, 0 },  { 1, 6, 2 },   { 1, 4, 2 },  { 1, 2, 2 },  { 1, 5, 2 },  { 2, 1, 0 },   { 2, 2, 0 },  { 2, 5, 2 },
    { 2, 3, 0 },  { 24, 7, 0 },  { 24, 8, 0 }, { 24, 6, 1 }, { 24, 9, 0 }, { 24, 4, 1 },  { 25, 1, 0 }, { 25, 2, 0 },
    { 25, 5, 1 }, { 26, 1, 0 },  { 26, 5, 0 }, { 3, 0, 0 },  { 26, 2, 1 }, { 26, 0, 0 },  { 27, 0, 0 }, { 27, 1, 0 },
    { 27, 2, 0 }, { 27, 5, 1 },  { 27, 6, 1 }, { 27, 7, 1 }, { 27, 8, 1 }, { 28, 0, 0 },  { 3, 1, 0 },  { 28, 2, 0 },
    { 28, 3, 0 }, { 28, 4, 0 },  { 28, 5, 0 }, { 28, 6, 1 }, { 28, 8, 1 }, { 29, 1, 0 },  { 29, 5, 1 }, { 30, 1, 0 },
    { 30, 5, 1 }, { 3, 2, 2 },   { 31, 1, 0 }, { 31, 5, 1 }, { 31, 4, 0 }, { 31, 7, 1 },  { 32, 1, 0 }, { 32, 2, 0 },
    { 32, 6, 0 }, { 32, 4, 1 },  { 33, 1, 0 }, { 33, 2, 0 }, { 3, 3, 0 },  { 33, 5, 1 },  { 33, 7, 0 }, { 33, 8, 1 },
    { 33, 6, 1 }, { 34, 1, 0 },  { 34, 2, 0 }, { 34, 5, 1 }, { 34, 6, 1 }, { 34, 8, 1 },  { 35, 0, 0 }, { 3, 5, 1 },
    { 35, 1, 0 }, { 35, 5, 0 },  { 35, 2, 0 }, { 35, 4, 1 }, { 36, 0, 0 }, { 36, 1, 0 },  { 36, 2, 0 }, { 36, 4, 1 },
    { 36, 5, 1 }, { 37, 0, 0 },  { 4, 1, 0 },  { 37, 1, 0 }, { 37, 2, 0 }, { 37, 4, 1 },  { 37, 5, 1 }, { 38, 0, 0 },
    { 38, 1, 0 }, { 38, 2, 0 },  { 38, 3, 0 }, { 38, 4, 1 }, { 38, 5, 0 }, { 4, 5, 1 },   { 38, 6, 1 }, { 38, 7, 1 },
    { 39, 1, 0 }, { 39, 3, 0 },  { 39, 2, 0 }, { 39, 8, 1 }, { 39, 4, 0 }, { 39, 5, 0 },  { 39, 6, 1 }, { 39, 7, 1 },
    { 4, 7, 1 },  { 39, 10, 0 }, { 40, 1, 0 }, { 40, 2, 0 }, { 40, 3, 0 }, { 40, 4, 1 },  { 40, 5, 1 }, { 41, 6, 1 },
    { 5, 1, 0 },  { 5, 3, 1 },   { 5, 5, 1 },  { 6, 1, 0 },  { 6, 2, 0 },  { 6, 4, 1 },   { 6, 6, 1 },  { 6, 7, 1 },
    { 7, 0, 0 },  { 7, 8, 0 },   { 7, 5, 1 },  { 7, 6, 1 },  { 8, 2, 0 },  { 8, 3, 1 },   { 8, 4, 1 },  { 9, 1, 0 },
    { 9, 3, 1 },  { 9, 4, 1 },   { 9, 6, 1 },  { 10, 1, 0 }, { 10, 3, 1 }, { 10, 4, 1 },  { 11, 0, 0 }, { 11, 1, 0 },
    { 11, 2, 0 }, { 11, 4, 1 },  { 11, 5, 1 }, { 12, 1, 0 }, { 12, 3, 1 }, { 13, 1, 0 },  { 13, 3, 0 }, { 13, 4, 1 },
    { 14, 1, 0 }, { 14, 4, 1 },  { 14, 6, 1 }, { 15, 1, 0 }, { 15, 2, 0 }, { 15, 4, 1 },  { 15, 8, 1 }, { 15, 6, 0 },
    { 16, 0, 0 }, { 16, 5, 1 },  { 16, 7, 1 }, { 17, 1, 0 }, { 17, 6, 1 }, { 18, 1, 0 },  { 18, 6, 1 }, { 18, 8, 1 },
    { 18, 5, 0 }, { 19, 0, 0 },  { 19, 1, 0 }, { 19, 2, 0 }, { 19, 6, 0 }, { 19, 3, 0 },  { 19, 4, 1 }, { 19, 7, 1 },
    { 20, 0, 0 }, { 20, 1, 0 },  { 20, 2, 0 }, { 20, 3, 1 }, { 20, 5, 1 }, { 20, 6, 1 },  { 21, 1, 0 }, { 21, 2, 0 },
    { 21, 3, 0 }, { 21, 4, 1 },  { 21, 6, 1 }, { 21, 7, 1 }, { 21, 9, 0 }, { 21, 5, 0 },  { 22, 3, 0 }, { 22, 8, 1 },
    { 22, 1, 0 }, { 23, 1, 0 },  { 23, 7, 0 }, { 23, 2, 1 }, { 23, 3, 1 }, { 23, 10, 1 }, { 23, 9, 1 }, { 24, 0, 0 },
    { 24, 3, 0 },
};

PokewoodCutin *func_ov167_021d5e1c(HeapID heapId) {
    PokewoodCutin *cutin = GFL_HeapAllocate(heapId, sizeof(PokewoodCutin), TRUE, "pokewood_cutin.c", 292);

    cutin->unit = func_0204bf1c(CUTIN_ACTOR_COUNT, 0, heapId);
    cutin->renderer = func_0204be9c(&sSurfaceSetup, 1, heapId);
    func_0204bf14(cutin->renderer, TRUE);
    func_0204c018(cutin->unit, cutin->renderer);
    cutin->heapId = heapId;
    return cutin;
}

void func_ov167_021d5e68(PokewoodCutin *cutin) {
    if (cutin->script != NULL) {
        CutinScript_Free(cutin->script);
        cutin->script = NULL;
    }
    func_0204becc(cutin->renderer);
    func_0204bf98(cutin->unit);
    GFL_HeapFree(cutin);
}

// Runs the cut-in that func_ov167_021d5fc4 started, a step of its script at a time
void func_ov167_021d5e90(PokewoodCutin *cutin) {
    u16 paletteFile;
    u16 charFile;
    u16 cellFile;
    u16 animFile;
    int i;

    if (cutin->done) {
        return;
    }
    switch (cutin->seq) {
    case 0:
        break;
    case 1:
        cutin->script = CutinScript_Load(Cutin_FindScript(cutin->rules->movie, cutin->scene), cutin->heapId);
        if (cutin->gender == 0) {
            paletteFile = CutinScript_GetHeader(cutin->script, 3);
            charFile = CutinScript_GetHeader(cutin->script, 4);
            cellFile = CutinScript_GetHeader(cutin->script, 5);
            animFile = CutinScript_GetHeader(cutin->script, 6);
        } else {
            paletteFile = CutinScript_GetHeader(cutin->script, 7);
            charFile = CutinScript_GetHeader(cutin->script, 8);
            cellFile = CutinScript_GetHeader(cutin->script, 9);
            animFile = CutinScript_GetHeader(cutin->script, 10);
        }
        Cutin_LoadResources(cutin, paletteFile, charFile, cellFile, animFile);
        cutin->seq++;
        break;
    case 2:
        cutin->cmd = sCommands[CutinScript_GetStepValue(cutin->script, cutin->step, 0)];
        cutin->cmdSeq = 0;
        for (i = 0; i < CUTIN_STEP_ARG_COUNT; i++) {
            cutin->args[i] = CutinScript_GetStepValue(cutin->script, cutin->step, i + 1);
        }
        cutin->seq++;
        break;
    case 3:
        if (cutin->cmd(cutin, &cutin->cmdSeq, cutin->args)) {
            cutin->seq++;
        }
        break;
    case 4:
        cutin->step++;
        if (cutin->step < CutinScript_GetStepCount(cutin->script)) {
            cutin->seq = 2;
        } else {
            cutin->seq++;
        }
        break;
    case 5:
        Cutin_UnloadResources(cutin);
        CutinScript_Free(cutin->script);
        cutin->script = NULL;
        cutin->done = TRUE;
        break;
    }
}

BOOL func_ov167_021d5fc0(PokewoodCutin *cutin) {
    return cutin->done;
}

void func_ov167_021d5fc4(PokewoodCutin *cutin, const BtlScriptedRules *rules, s8 scene, u16 choice, u32 gender) {
    cutin->seq = 1;
    cutin->done = FALSE;
    cutin->step = 0;
    cutin->rules = rules;
    cutin->scene = scene;
    cutin->gender = gender;
    cutin->choice = choice;
}

void func_ov167_021d5fe4(PokewoodCutin *cutin, BtlvCore *viewCore) {
    cutin->viewCore = viewCore;
}

// Whether the movie has a cut-in after the scene for its result
BOOL func_ov167_021d5fe8(const BtlScriptedRules *rules, s8 scene, u8 result, u32 unused) {
    u32 i;
    BOOL play;

    for (i = 0; i < NELEMS(sCutins); i++) {
        if (rules->movie == sCutins[i].movie && scene == sCutins[i].scene) {
            play = FALSE;
            switch (sCutins[i].type) {
            case 0:
                if (result == 1) {
                    play = TRUE;
                }
                break;
            case 1:
                play = TRUE;
                break;
            case 2:
                if (result != 1) {
                    play = TRUE;
                }
                break;
            }
            if (play) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void Cutin_LoadResources(PokewoodCutin *cutin, u32 paletteFile, u32 charFile, u32 cellFile, u32 animFile) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEWOOD_CUTIN_GRA, HEAPID_TAIL(cutin->heapId));
    ClActorSetup setup;
    int i;

    cutin->chars = func_0204b81c(arc, charFile, TRUE, CLACT_VRAM_MAIN, cutin->heapId);
    cutin->palette = func_0204bba0(arc, paletteFile, CLACT_VRAM_MAIN, 0, cutin->heapId);
    PaletteFade_LoadFromVRAM(BtlvEffect_GetPaletteFade(), PALFADE_VRAM_MAIN_OBJ, 0, 0x1e0);
    cutin->cellAnims = func_0204bde0(arc, cellFile, animFile, cutin->heapId);
    cutin->cellData = GFL_G2DIOReadNCERArc(arc, cellFile, FALSE, &cutin->cells, cutin->heapId);
    GFL_ArcToolFree(arc);
    setup = sActorSetup;
    for (i = 0; i < CUTIN_ACTOR_COUNT; i++) {
        cutin->actors[i] =
            func_0204c040(cutin->unit, cutin->chars, cutin->palette, cutin->cellAnims, &setup, 0, cutin->heapId);
        func_0204c124(cutin->actors[i], FALSE);
    }
    cutin->loaded = TRUE;
}

static void Cutin_UnloadResources(PokewoodCutin *cutin) {
    int i;

    if (cutin->loaded) {
        for (i = 0; i < CUTIN_ACTOR_COUNT; i++) {
            func_0204c108(cutin->actors[i]);
        }
        GFL_HeapFree(cutin->cellData);
        func_0204be64(cutin->cellAnims);
        func_0204b98c(cutin->chars);
        func_0204bcd0(cutin->palette);
        cutin->loaded = FALSE;
    }
}

static u32 Cutin_FindScript(s16 movie, u16 scene) {
    u32 i;

    for (i = 0; i < NELEMS(sCutins); i++) {
        if (movie == sCutins[i].movie && scene == sCutins[i].scene) {
            return i;
        }
    }
    return 0xffff;
}

// Moves an actor from or to the screen's center, playing the animation of its number
static BOOL Cutin_CmdMove(PokewoodCutin *cutin, u16 *seq, const u16 *args) {
    ClActor *actor = cutin->actors[args[0]];
    ClActorPos pos;

    switch (*seq) {
    case 0:
        sMoveFuncs[args[1]].init(cutin, actor, args);
        sMoveFuncs[args[1]].update(cutin);
        sMoveFuncs[args[1]].getPos(cutin, &pos);
        func_0204c140(actor, &pos, 0);
        func_0204c488(actor, args[0]);
        func_0204c124(actor, TRUE);
        (*seq)++;
        break;
    case 1:
        if (sMoveFuncs[args[1]].update(cutin)) {
            (*seq)++;
        }
        sMoveFuncs[args[1]].getPos(cutin, &pos);
        func_0204c140(actor, &pos, 0);
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

// Moves an actor by a distance
static BOOL Cutin_CmdMoveDist(PokewoodCutin *cutin, u16 *seq, const u16 *args) {
    ClActor *actor = cutin->actors[args[0]];
    ClActorPos pos;

    switch (*seq) {
    case 0:
        sMoveFuncs[args[1]].initDist(cutin, actor, args);
        sMoveFuncs[args[1]].update(cutin);
        sMoveFuncs[args[1]].getPos(cutin, &pos);
        func_0204c140(actor, &pos, 0);
        func_0204c488(actor, args[0]);
        func_0204c124(actor, TRUE);
        (*seq)++;
        break;
    case 1:
        if (sMoveFuncs[args[1]].update(cutin)) {
            (*seq)++;
        }
        sMoveFuncs[args[1]].getPos(cutin, &pos);
        func_0204c140(actor, &pos, 0);
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

static BOOL Cutin_CmdSetVisible(PokewoodCutin *cutin, u16 *seq, const u16 *args) {
    func_0204c124(cutin->actors[args[0]], args[1]);
    return TRUE;
}

static BOOL Cutin_CmdNudge(PokewoodCutin *cutin, u16 *seq, const u16 *args) {
    ClActor *actor = cutin->actors[args[0]];
    ClActorPos pos;

    switch (*seq) {
    case 0:
        sys_memset(&cutin->nudge, 0, sizeof(CutinNudge));
        sNudgeFuncs[args[1]].init(cutin, &cutin->nudge, actor);
        sNudgeFuncs[args[1]].update(cutin, &cutin->nudge);
        sNudgeFuncs[args[1]].getPos(cutin, &cutin->nudge, &pos);
        func_0204c140(actor, &pos, 0);
        func_0204c488(actor, args[0]);
        func_0204c124(actor, TRUE);
        (*seq)++;
        break;
    case 1:
        if (sNudgeFuncs[args[1]].update(cutin, &cutin->nudge)) {
            (*seq)++;
        }
        sNudgeFuncs[args[1]].getPos(cutin, &cutin->nudge, &pos);
        func_0204c140(actor, &pos, 0);
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

// Replaces the actors' graphics with those for the player's gender
static BOOL Cutin_CmdLoadResources(PokewoodCutin *cutin, u16 *seq, const u16 *args) {
    switch (*seq) {
    case 0:
        Cutin_UnloadResources(cutin);
        (*seq)++;
        break;
    case 1:
        if (cutin->gender == 0) {
            Cutin_LoadResources(cutin, args[0], args[1], args[2], args[3]);
        } else {
            Cutin_LoadResources(cutin, args[4], args[5], args[6], args[7]);
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL Cutin_CmdWait(PokewoodCutin *cutin, u16 *seq, const u16 *args) {
    if (cutin->wait++ >= args[0]) {
        cutin->wait = 0;
        return TRUE;
    }
    return FALSE;
}

// Prints a message of the scene: of its step args[0] from one of its arrays, or of the choice made from 10 on
static BOOL Cutin_CmdPrintMessage(PokewoodCutin *cutin, u16 *seq, const u16 *args) {
    const BtlStudioScene *scene;
    s16 msgId;

    switch (*seq) {
    case 0:
        scene = &cutin->rules->scenes[cutin->scene];
        if (args[0] >= 10) {
            if (args[1] == 0) {
                msgId = scene->choices[cutin->choice].msgId;
            } else {
                msgId = scene->choices[cutin->choice].msgId2;
            }
        } else {
            msgId = scene->messages[args[1]][args[0]];
        }
        Btlv_StringParam_Setup(&cutin->strParam, 8, msgId);
        func_ov167_021d01ec(cutin->viewCore, &cutin->strParam);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d02e8(cutin->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL Cutin_CmdViewEffect(PokewoodCutin *cutin, u16 *seq, const u16 *args) {
    switch (*seq) {
    case 0:
        func_ov167_021d04bc(cutin->viewCore);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d04cc(cutin->viewCore)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL Cutin_CmdPlaySE(PokewoodCutin *cutin, u16 *seq, const u16 *args) {
    GFL_SndSEPlay(args[0]);
    return TRUE;
}

static BOOL Cutin_CmdWaitSound(PokewoodCutin *cutin, u16 *seq, const u16 *args) {
    if (!GFL_SndPlayerIsActiveAny()) {
        return TRUE;
    }
    return FALSE;
}

static BOOL Cutin_CmdStopSound(PokewoodCutin *cutin, u16 *seq, const u16 *args) {
    GFL_SndStop();
    return TRUE;
}

static void Cutin_LinearInit(PokewoodCutin *cutin, ClActor *actor, const u16 *args) {
    CutinPoint start;
    CutinPoint end;

    Cutin_GetCenterMove(args[2], actor, &start, &end);
    LinearMotion_Init(&cutin->motion.linear, &start, &end, args[3]);
}

static void Cutin_LinearInitDist(PokewoodCutin *cutin, ClActor *actor, const u16 *args) {
    CutinPoint start;
    CutinPoint end;

    Cutin_GetDistMove(args[2], args[4], actor, &start, &end);
    LinearMotion_Init(&cutin->motion.linear, &start, &end, args[3]);
}

static BOOL Cutin_LinearUpdate(PokewoodCutin *cutin) {
    return LinearMotion_Update(&cutin->motion.linear);
}

static void Cutin_LinearGetPos(PokewoodCutin *cutin, ClActorPos *pos) {
    LinearMotion_GetPos(&cutin->motion.linear, pos);
}

static void Cutin_AccelInit(PokewoodCutin *cutin, ClActor *actor, const u16 *args) {
    CutinPoint start;
    CutinPoint end;

    Cutin_GetCenterMove(args[2], actor, &start, &end);
    AccelMotion_Init(&cutin->motion.accel, &start, &end, FX32_CONST(50), args[3]);
}

static void Cutin_AccelInitDist(PokewoodCutin *cutin, ClActor *actor, const u16 *args) {
    CutinPoint start;
    CutinPoint end;

    Cutin_GetDistMove(args[2], args[4], actor, &start, &end);
    AccelMotion_Init(&cutin->motion.accel, &start, &end, FX32_CONST(50), args[3]);
}

static BOOL Cutin_AccelUpdate(PokewoodCutin *cutin) {
    return AccelMotion_Update(&cutin->motion.accel);
}

static void Cutin_AccelGetPos(PokewoodCutin *cutin, ClActorPos *pos) {
    AccelMotion_GetPos(&cutin->motion.accel, pos);
}

// A move between the screen's center and 256 pixels from it in a direction: out from the center when the actor is
// there, in to it otherwise. The directions are down, up, left, right, then the diagonals up and left, up and right,
// down and left, down and right
static void Cutin_GetCenterMove(u16 dir, ClActor *actor, CutinPoint *start, CutinPoint *end) {
    ClActorPos pos;
    CutinPoint offset = { 0, 0 };

    func_0204c178(actor, &pos, 0);
    switch (dir) {
    case 0:
        offset.y = 256;
        break;
    case 1:
        offset.y = -256;
        break;
    case 2:
        offset.x = -256;
        break;
    case 3:
        offset.x = 256;
        break;
    case 4:
        offset.x = -256;
        offset.y = -256;
        break;
    case 5:
        offset.x = 256;
        offset.y = -256;
        break;
    case 6:
        offset.x = -256;
        offset.y = 256;
        break;
    case 7:
        offset.x = 256;
        offset.y = 256;
        break;
    }
    if (pos.x == CUTIN_CENTER_X && pos.y == CUTIN_CENTER_Y) {
        start->y = CUTIN_CENTER_Y;
        start->x = CUTIN_CENTER_X;
        end->x = start->x + offset.x;
        end->y = start->y + offset.y;
    } else {
        end->y = CUTIN_CENTER_Y;
        end->x = CUTIN_CENTER_X;
        start->x = end->x - offset.x;
        start->y = end->y - offset.y;
    }
}

// A move from the actor's position by a distance in a direction, as Cutin_GetCenterMove's
static void Cutin_GetDistMove(u16 dir, u16 dist, ClActor *actor, CutinPoint *start, CutinPoint *end) {
    ClActorPos pos;
    CutinPoint offset = { 0, 0 };
    // The distance along each axis of a diagonal move
    // BUG: sqrt(2) is 1.4142, so diagonal moves go about 1.24 times as far as the script says
#ifdef BUGFIX
    s32 diagonal = FX_Div(dist * FX32_ONE, FX32_CONST(1.4142)) >> FX32_SHIFT;
#else
    s32 diagonal = FX_Div(dist * FX32_ONE, FX32_CONST(1.1414)) >> FX32_SHIFT;
#endif

    func_0204c178(actor, &pos, 0);
    switch (dir) {
    case 0:
        offset.y = dist;
        break;
    case 1:
        offset.y = -dist;
        break;
    case 2:
        offset.x = -dist;
        break;
    case 3:
        offset.x = dist;
        break;
    case 4:
        offset.x = -diagonal;
        offset.y = -diagonal;
        break;
    case 5:
        offset.x = diagonal;
        offset.y = -diagonal;
        break;
    case 6:
        offset.x = -diagonal;
        offset.y = diagonal;
        break;
    case 7:
        offset.x = diagonal;
        offset.y = diagonal;
        break;
    }
    start->x = pos.x;
    start->y = pos.y;
    end->x = start->x + offset.x;
    end->y = start->y + offset.y;
}

static void Cutin_NudgeUpInit(PokewoodCutin *cutin, CutinNudge *nudge, ClActor *actor) {
    ClActorPos pos;

    func_0204c178(actor, &pos, 0);
    nudge->start.x = pos.x;
    nudge->start.y = pos.y;
    nudge->end.x = pos.x;
    nudge->end.y = pos.y - 10;
}

static void Cutin_NudgeDownInit(PokewoodCutin *cutin, CutinNudge *nudge, ClActor *actor) {
    ClActorPos pos;

    func_0204c178(actor, &pos, 0);
    nudge->start.x = pos.x;
    nudge->start.y = pos.y;
    nudge->end.x = pos.x;
    nudge->end.y = pos.y + 10;
}

static void Cutin_NudgeLeftInit(PokewoodCutin *cutin, CutinNudge *nudge, ClActor *actor) {
    ClActorPos pos;

    func_0204c178(actor, &pos, 0);
    nudge->start.x = pos.x;
    nudge->start.y = pos.y;
    nudge->end.x = pos.x - 10;
    nudge->end.y = pos.y;
}

static void Cutin_NudgeRightInit(PokewoodCutin *cutin, CutinNudge *nudge, ClActor *actor) {
    ClActorPos pos;

    func_0204c178(actor, &pos, 0);
    nudge->start.x = pos.x;
    nudge->start.y = pos.y;
    nudge->end.x = pos.x + 10;
    nudge->end.y = pos.y;
}

static BOOL Cutin_NudgeUpdate(PokewoodCutin *cutin, CutinNudge *nudge) {
    switch (nudge->seq) {
    case 0:
        LinearMotion_Init(&cutin->motion.linear, &nudge->start, &nudge->end, 10);
        nudge->seq++;
        break;
    case 1:
        if (LinearMotion_Update(&cutin->motion.linear)) {
            nudge->seq++;
        }
        break;
    case 2:
        LinearMotion_Init(&cutin->motion.linear, &nudge->end, &nudge->start, 10);
        nudge->seq++;
        break;
    case 3:
        if (LinearMotion_Update(&cutin->motion.linear)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void Cutin_NudgeGetPos(PokewoodCutin *cutin, CutinNudge *nudge, ClActorPos *pos) {
    LinearMotion_GetPos(&cutin->motion.linear, pos);
}

static void LinearMotion_Init(CutinLinearMotion *motion, const CutinPoint *start, const CutinPoint *end, s32 frames) {
    motion->pos = *start;
    motion->start = *start;
    motion->end = *end;
    motion->frames = frames;
    if (frames != 0) {
        motion->step.x = FX32_CONST(motion->end.x - motion->start.x) / frames;
        motion->step.y = FX32_CONST(motion->end.y - motion->start.y) / frames;
        motion->frame = 0;
    } else {
        motion->frame = frames - 2;
    }
}

static BOOL LinearMotion_Update(CutinLinearMotion *motion) {
    if (motion->frame < motion->frames - 1) {
        motion->frame++;
        motion->pos.x = motion->start.x + ((motion->step.x * motion->frame) >> FX32_SHIFT);
        motion->pos.y = motion->start.y + ((motion->step.y * motion->frame) >> FX32_SHIFT);
        return FALSE;
    }
    motion->pos = motion->end;
    return TRUE;
}

static void LinearMotion_GetPos(const CutinLinearMotion *motion, ClActorPos *pos) {
    pos->x = motion->pos.x;
    pos->y = motion->pos.y;
}

static void AccelMotion_Init(CutinAccelMotion *motion, const CutinPoint *start, const CutinPoint *end, fx32 speed,
                             s32 frames) {
    fx32 dist;
    fx32 accel;

    VEC_Set(&motion->pos, FX32_CONST(start->x), FX32_CONST(start->y), 0);
    VEC_Set(&motion->start, FX32_CONST(start->x), FX32_CONST(start->y), 0);
    VEC_Set(&motion->end, FX32_CONST(end->x), FX32_CONST(end->y), 0);
    dist = VEC_Distance(&motion->end, &motion->start);
    VEC_Subtract(&motion->end, &motion->start, &motion->dir);
    VEC_Normalize(&motion->dir, &motion->dir);
    // From dist = speed * frames + accel * frames * frames / 2
    accel = FX_Div((dist - speed * frames) * 2, frames * frames * FX32_ONE);
    motion->speed = speed;
    motion->accel = accel;
    motion->frame = 0;
    motion->frames = frames;
}

static BOOL AccelMotion_Update(CutinAccelMotion *motion) {
    if (motion->frame < motion->frames - 1) {
        motion->frame++;
        VEC_MultAdd(FX_Mul(motion->speed, motion->frame * FX32_ONE) +
                         FX_Mul(motion->accel, motion->frame * motion->frame * FX32_ONE) / 2,
                     &motion->dir, &motion->start, &motion->pos);
        return FALSE;
    }
    motion->pos = motion->end;
    return TRUE;
}

static void AccelMotion_GetPos(const CutinAccelMotion *motion, ClActorPos *pos) {
    pos->x = FX_Whole(motion->pos.x);
    pos->y = FX_Whole(motion->pos.y);
}

static CutinScript *CutinScript_Load(u32 fileId, HeapID heapId) {
    CutinScript *script = GFL_HeapAllocate(heapId, sizeof(CutinScript), TRUE, "pokewood_cutin.c", 1678);

    script->data = GFL_ArcSysReadHeapNewLZGetLen(ARCID_POKEWOOD_CUTIN_SCRIPT, fileId, FALSE, heapId, &script->size);
    return script;
}

static void CutinScript_Free(CutinScript *script) {
    GFL_HeapFree(script->data);
    GFL_HeapFree(script);
}

static u16 CutinScript_GetHeader(CutinScript *script, u32 index) {
    return script->data[index];
}

static u16 CutinScript_GetStepValue(CutinScript *script, u16 step, u32 index) {
    return script->data[CUTIN_SCRIPT_HEADER_COUNT + step * CUTIN_STEP_SIZE + index];
}

static u32 CutinScript_GetStepCount(CutinScript *script) {
    return (script->size - CUTIN_SCRIPT_HEADER_COUNT * sizeof(u16)) / (CUTIN_STEP_SIZE * sizeof(u16));
}
