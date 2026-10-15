#include "types.h"
#include "app/demo3d.h"
#include "app/demo3d/demo3d_data.h"
#include "app/demo3d/demo3d_engine.h"
#include "app/demo3d/demo3d_exp.h"
#include "app/demo3d/demo3d_graphic.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nnsys/snd.h"
#include "system/brightness.h"
#include "system/printsys.h"
#include "system/wipe.h"

// The 3D demos' process, named after the ROM's "demo3d.c": it plays a camera animation through scenes of models,
// with commands at set frames. The Royal Unova's cruise, the ferris wheel and the ships and flights are such demos

#define DEMO3D_ID_C_CRUISER 1
#define DEMO3D_SOUND_GROUP_COUNT 2

typedef struct {
    HeapID heapId;
    Demo3DParam *param;
    u32 unk8;
    Demo3DGraphic *graphic;
    PrintQueue *printQueue;
    MsgData *msgData;
    Demo3DEngine *engine;
    Demo3DExp *exp;
    u32 lastVBlank;
    // The frames the last one took beyond its own, which the engine catches up on
    u32 droppedFrames;
    u32 droppedTotal;
} Demo3DWork;

// A sound group that a demo loads
typedef struct {
    u32 demoId;
    u32 group;
} Demo3DSoundGroup;

static BOOL Demo3D_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Demo3D_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Demo3D_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Demo3D_StartFade(u8 demoId, int type, HeapID heapId);
static BOOL Demo3D_IsSkipRequested(Demo3DWork *work);
static Demo3DExp *Demo3DExp_Create(u32 demoId, Demo3DGraphic *graphic, Demo3DEngine *engine, HeapID heapId);
static void Demo3DExp_Free(Demo3DExp *exp);
static void Demo3DExp_Main(Demo3DExp *exp);
static void Demo3D_LoadSoundGroups(u32 demoId);
static void Demo3D_FreeSoundGroups(u32 demoId);

const GameProcFunctions gDemo3DProcFunctions = { Demo3D_Init, Demo3D_Main, Demo3D_Exit };

static const Demo3DSoundGroup sDemo3DSoundGroups[DEMO3D_SOUND_GROUP_COUNT] = {
    { 18, 3 },
    { 21, 4 },
};

static BOOL Demo3D_Init(GameProc *proc, u32 *state, void *param, void *work) {
    Demo3DParam *demoParam = param;
    Demo3DWork *demo;
    int frames;
    u8 white;
    u8 fadeFrames;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_DEMO3D, 0x130000);
    demo = GFL_ProcInitSubsystem(proc, sizeof(Demo3DWork), HEAPID_DEMO3D);
    sys_memset(demo, 0, sizeof(Demo3DWork));
    demo->heapId = HEAPID_DEMO3D;
    demo->param = demoParam;
    demo->graphic = Demo3DGraphic_Create(1, demoParam->demoId, demo->heapId);
    demo->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_INTRO_BTL_SERVER_FLOW, demo->heapId);
    demo->printQueue = func_02021998(demo->heapId);
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysSetBGEnabled(3, FALSE);
    GFL_BGSysSetBGEnabled(2, FALSE);
    GFL_BGSysSetBGEnabled(1, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
    demo->engine = Demo3DEngine_Create(demo->graphic, demoParam, demo->heapId);
    demo->exp = Demo3DExp_Create(demoParam->demoId, demo->graphic, demo->engine, demo->heapId);
    Demo3D_LoadSoundGroups(demoParam->demoId);
    frames = Demo3DEngine_GetFrameCount(demo->engine) - demoParam->unk08;
    Demo3DData_GetFadeParams(demoParam->demoId, FALSE, &white, &fadeFrames);
    if (frames > fadeFrames) {
        Demo3D_StartFade(demoParam->demoId, WIPE_TYPE_FADE_IN, demo->heapId);
    }
    return TRUE;
}

static BOOL Demo3D_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    Demo3DWork *demo = work;

    BrightnessController_Reset(3);
    GFL_SndPlayerStop(1);
    GFL_SndPlayerStop(2);
    GFL_SndPlayerStop(3);
    GFL_SndPlayerStop(4);
    Demo3D_FreeSoundGroups(demo->param->demoId);
    Demo3DExp_Free(demo->exp);
    GFL_MsgDataFree(demo->msgData);
    func_02021a18(demo->printQueue);
    Demo3DEngine_Free(demo->engine);
    Demo3DGraphic_Free(demo->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_DEMO3D);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL Demo3D_Main(GameProc *proc, u32 *state, void *param, void *work) {
    Demo3DWork *demo = work;
    BOOL ended;
    u32 vblank;
    u32 elapsed;

    Demo3DExp_Main(demo->exp);
    func_02021a3c(demo->printQueue);
    Demo3DGraphic_Update(demo->graphic);
    ended = Demo3DEngine_Main(demo->engine, demo->droppedFrames);
    vblank = OS_GetVBlankCount();
    elapsed = vblank - demo->lastVBlank;
    demo->droppedFrames = 0;
    if (demo->lastVBlank != 0 && elapsed > 1) {
        demo->droppedFrames = elapsed - 1;
    }
    demo->lastVBlank = vblank;
    demo->droppedTotal += demo->droppedFrames;
    switch (*state) {
    case 0:
        if (Demo3D_IsSkipRequested(demo) || ended) {
            demo->param->unk10 = Demo3DEngine_GetNowFrame(demo->engine) >> FX32_SHIFT;
            demo->param->result = ended + 1;
            Demo3DEngine_SetStopped(demo->engine, demo->param->result);
            (*state)++;
        }
        break;
    case 1:
        if (GFL_WipeIsFinished() == TRUE) {
            if (Demo3D_StartFade(demo->param->demoId, WIPE_TYPE_FADE_OUT, demo->heapId)) {
                return TRUE;
            }
            (*state)++;
        }
        break;
    case 2:
        if (GFL_WipeIsFinished() == TRUE) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Starts the demo's fade-in or fade-out. Returns TRUE when the demo has none
static BOOL Demo3D_StartFade(u8 demoId, int type, HeapID heapId) {
    u8 white;
    u8 frames;
    u16 color;

    Demo3DData_GetFadeParams(demoId, type == WIPE_TYPE_FADE_OUT ? TRUE : FALSE, &white, &frames);
    color = WIPE_COLOR_BLACK;
    if (white) {
        color = WIPE_COLOR_WHITE;
    }
    Wipe_SetCovered(color);
    if (frames) {
        u8 freq = GFL_FadeGetUpdateFreq();
        GFL_WipeSet(WIPE_MODE_BOTH, type, type, color, frames * freq, 1, heapId);
        return FALSE;
    }
    return TRUE;
}

static BOOL Demo3D_IsSkipRequested(Demo3DWork *work) {
    if (work->param->result != 0 || !GFL_WipeIsFinished()) {
        return FALSE;
    }
    if (work->exp->skippable &&
        (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_RIGHT | PAD_KEY_LEFT | PAD_KEY_UP |
                                     PAD_KEY_DOWN | PAD_BUTTON_X | PAD_BUTTON_Y))) {
        return TRUE;
    }
    return FALSE;
}

static Demo3DExp *Demo3DExp_Create(u32 demoId, Demo3DGraphic *graphic, Demo3DEngine *engine, HeapID heapId) {
    Demo3DExp *exp = GFL_HeapAllocate(heapId, sizeof(Demo3DExp), TRUE, "demo3d.c", 510);

    exp->graphic = graphic;
    exp->engine = engine;
    exp->heapId = heapId;
    exp->funcs = NULL;
    exp->skippable = FALSE;
    if (demoId == DEMO3D_ID_C_CRUISER) {
        exp->funcs = &gDemo3DExpCCruiserFuncs;
        exp->skippable = TRUE;
    }
    if (exp->funcs != NULL) {
        exp->funcs->init(exp);
    }
    return exp;
}

static void Demo3DExp_Free(Demo3DExp *exp) {
    if (exp->funcs != NULL) {
        exp->funcs->exit(exp);
    }
    GFL_HeapFree(exp);
}

static void Demo3DExp_Main(Demo3DExp *exp) {
    if (exp->funcs != NULL) {
        exp->funcs->main(exp);
    }
}

static void Demo3D_LoadSoundGroups(u32 demoId) {
    int i;

    for (i = 0; i < DEMO3D_SOUND_GROUP_COUNT; i++) {
        if (demoId == sDemo3DSoundGroups[i].demoId) {
            NNSSndHeapHandle heap = func_02005ce4();
            NNS_SndHeapSaveState(heap);
            NNS_SndArcLoadGroup(sDemo3DSoundGroups[i].group, heap);
        }
    }
}

static void Demo3D_FreeSoundGroups(u32 demoId) {
    int i;

    for (i = 0; i < DEMO3D_SOUND_GROUP_COUNT; i++) {
        if (demoId == sDemo3DSoundGroups[i].demoId) {
            NNSSndHeapHandle heap = func_02005ce4();
            NNS_SndHeapLoadState(heap, NNS_SndHeapGetCurrentLevel(heap));
        }
    }
}
