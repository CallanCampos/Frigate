#include "types.h"
#include "app/demo3d/demo3d_engine.h"
#include "app/demo3d/demo3d_exp.h"
#include "app/demo3d/demo3d_graphic.h"
#include "app/ui/ui_scene.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

// The Royal Unova's cruise's own part of its 3D demo, named after the ROM's "demo3d_exp_c_cruiser.c": a map of the
// bay on the lower screen, with a marker that follows the ship's course

// The archive of the map and its marker. Our name
#define ARCID_DEMO3D_C_CRUISER 84
// The marker moves every 100 frames of the camera's animation
#define POSITION_FRAMES 100
#define POSITION_COUNT 54

typedef struct {
    // The next of the marker's positions
    u8 position;
    UIObjRes objRes;
    ClActor *marker;
} Demo3DExpCCruiser;

static void Demo3DExpCCruiser_LoadBG(HeapID heapId);
static void Demo3DExpCCruiser_Init(Demo3DExp *exp);
static void Demo3DExpCCruiser_Main(Demo3DExp *exp);
static void Demo3DExpCCruiser_Exit(Demo3DExp *exp);

const Demo3DExpFuncs gDemo3DExpCCruiserFuncs = {
    Demo3DExpCCruiser_Init,
    Demo3DExpCCruiser_Main,
    Demo3DExpCCruiser_Exit,
};

static const ClActorPos sDemo3DExpCCruiserPositions[POSITION_COUNT] = {
    { 120, 136 }, { 120, 132 }, { 120, 128 }, { 120, 124 }, { 120, 120 }, { 120, 116 }, { 120, 112 }, { 120, 108 },
    { 120, 104 }, { 120, 100 }, { 120, 96 },  { 120, 92 },  { 120, 88 },  { 120, 84 },  { 120, 80 },  { 120, 76 },
    { 120, 72 },  { 120, 68 },  { 120, 64 },  { 120, 60 },  { 120, 56 },  { 120, 52 },  { 120, 48 },  { 120, 44 },
    { 120, 40 },  { 124, 36 },  { 132, 36 },  { 136, 40 },  { 136, 44 },  { 136, 48 },  { 136, 52 },  { 136, 56 },
    { 136, 60 },  { 136, 64 },  { 136, 68 },  { 136, 72 },  { 136, 76 },  { 136, 80 },  { 136, 84 },  { 136, 88 },
    { 136, 92 },  { 136, 96 },  { 136, 100 }, { 136, 104 }, { 136, 110 }, { 136, 116 }, { 136, 122 }, { 136, 128 },
    { 136, 134 }, { 136, 140 }, { 136, 146 }, { 136, 152 }, { 136, 158 }, { 136, 164 },
};

static void Demo3DExpCCruiser_LoadBG(HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_DEMO3D_C_CRUISER, heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 3, 4, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 9, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 18, 6, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
}

static void Demo3DExpCCruiser_Init(Demo3DExp *exp) {
    Demo3DExpCCruiser *work = GFL_HeapAllocate(exp->heapId, sizeof(Demo3DExpCCruiser), TRUE, "demo3d_exp_c_cruiser.c", 179);
    UIObjResSetup setup;
    ClActUnit *unit;

    exp->work = work;
    Demo3DExpCCruiser_LoadBG(exp->heapId);
    work->position = (Demo3DEngine_GetNowFrame(exp->engine) >> FX32_SHIFT) / POSITION_FRAMES;
    if (work->position >= POSITION_COUNT) {
        work->position = POSITION_COUNT - 1;
    }
    unit = Demo3DGraphic_GetClActUnit(exp->graphic);
    setup.vramType = 1;
    setup.flags = 0;
    setup.arcId = ARCID_DEMO3D_C_CRUISER;
    setup.paletteFile = 4;
    setup.charFile = 10;
    setup.cellFile = 13;
    setup.animFile = 20;
    setup.paletteOffset = 0;
    setup.paletteStart = 0;
    setup.paletteCount = 1;
    UIObjRes_Load(&work->objRes, &setup, unit, exp->heapId);
    work->marker = UIObjRes_CreateActor(&work->objRes, unit, sDemo3DExpCCruiserPositions[work->position].x,
                                        sDemo3DExpCCruiserPositions[work->position].y, 0, exp->heapId);
    func_0204c124(work->marker, TRUE);
    func_0204c520(work->marker, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void Demo3DExpCCruiser_Main(Demo3DExp *exp) {
    Demo3DExpCCruiser *work = exp->work;
    int frame = Demo3DEngine_GetNowFrame(exp->engine) >> FX32_SHIFT;

    if (work->position < POSITION_COUNT && work->position * POSITION_FRAMES <= frame) {
        func_0204c140(work->marker, &sDemo3DExpCCruiserPositions[work->position], 1);
        work->position++;
        if (work->position == POSITION_COUNT) {
            func_0204c124(work->marker, FALSE);
        }
    }
}

static void Demo3DExpCCruiser_Exit(Demo3DExp *exp) {
    UIObjRes_Free(&((Demo3DExpCCruiser *)exp->work)->objRes);
    GFL_HeapFree(exp->work);
}
