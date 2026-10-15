#include "types.h"
#include "app/mic_test.h"
#include "app/title.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/math_util.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mic.h"
#include "nitro/os.h"
#include "nnsys/gfd.h"
#include "system/app_menu_common.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "text/system/intro_btl_server_flow.h"

// The microphone test, which the title screen starts by holding Down, X and Y: the player speaks into the
// microphone, and the louder, the more the speaker on the screen shakes and the more notes it sends up. The Back
// button on the lower screen returns to the title screen

// The microphone test's graphics, archive 107
#define ARCID_MIC_TEST 107

#define BG_COUNT 5
#define WINDOW_COUNT 3
#define ACTOR_COUNT 7
#define NOTE_COUNT 3
// The first of the actors that are notes
#define ACTOR_NOTE_FIRST 4
#define ACTOR_BACK_BUTTON 3
#define ACTOR_CENTER 2

// The frames between the notes that a sound sends up
#define NOTE_INTERVAL 10

// The first resource of each group of an actor's resources in MicTestWork.resources: its characters, its palette and
// its cell animations
#define RES_SPEAKER 0
#define RES_CENTER 3
#define RES_NOTE 6
#define RES_BACK_BUTTON 9

typedef struct MicTestSeq MicTestSeq;
typedef void (*MicTestSeqFunc)(MicTestSeq *seq, u32 *state);

// Runs a function once a frame with its state, until it halts
struct MicTestSeq {
    void *work;
    MicTestSeqFunc func;
    u32 state;
    BOOL halted;
};

typedef struct {
    BmpWin *windows[WINDOW_COUNT];
    MsgData *msgData;
    Font *font;
} MicTestBG;

// A MICAutoParam with what the test keeps of the microphone's samples
typedef struct {
    MICAutoParam param;
    u8 *buffer;
    // The loudest sample that the speaker shows, which falls with time
    int peak;
    BOOL newPeak;
    BOOL sampling;
    // Whether to start again when the console wakes up
    BOOL resume;
    BOOL asleep;
    BOOL wasOpen;
} MicTestMic;

// A note, which flies from the speaker, swaying
typedef struct {
    ClActor *actor;
    u32 delay;
    VecFx32 position;
    VecFx32 velocity;
    fx32 phase;
    fx32 phaseStep;
    int swayRadius;
    TCB *task;
} MicTestNote;

typedef struct {
    ClActor *actors[ACTOR_COUNT];
    ClActUnit *unit;
    u32 resources[12];
    MicTestNote notes[NOTE_COUNT];
    u32 spawnTimer;
    MicTestBG bg;
    MicTestMic mic;
    MicTestSeq seq;
    void *tcbMemory;
    TCBManager *tcbManager;
    TCB *vblankTask;
    u32 heapId;
} MicTestWork;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
    u16 charPos;
    u8 textX;
    u8 unused[3];
} MicTestWindowSetup;

typedef struct {
    BGSetup setup;
    u8 bg;
    u8 mode;
} MicTestBGSetup;

typedef struct {
    u32 resource;
    u32 surface;
    u32 setup;
} MicTestActorSetup;

static BOOL MicTest_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MicTest_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MicTest_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void MicTestSeq_Init(MicTestSeq *seq, void *work, MicTestSeqFunc func);
static void MicTestSeq_Run(MicTestSeq *seq);
static void MicTestSeq_SetFunc(MicTestSeq *seq, MicTestSeqFunc func);
static void *MicTestSeq_GetWork(MicTestSeq *seq);
static BOOL MicTestSeq_IsHalted(MicTestSeq *seq);
static void MicTestSeq_Halt(MicTestSeq *seq);
static void MicTest_SeqWipeIn(MicTestSeq *seq, u32 *state);
static void MicTest_SeqListen(MicTestSeq *seq, u32 *state);
static void MicTest_SeqPressBack(MicTestSeq *seq, u32 *state);
static void MicTest_SeqWipeOut(MicTestSeq *seq, u32 *state);
static void MicTest_SeqEnd(MicTestSeq *seq, u32 *state);
static void MicTest_InitDisplay(HeapID heapId);
static void MicTest_ExitDisplay(void);
static void MicTest_SetVRAM(void);
static void MicTest_VBlank(TCB *tcb, void *data);
static void MicTest_InitClAct(MicTestWork *wk, u32 heapId);
static void MicTest_FreeClAct(MicTestWork *wk);
static void MicTest_UpdateClAct(MicTestWork *wk);
static void MicTest_CreateActors(MicTestWork *wk, HeapID heapId);
static void MicTest_FreeActors(MicTestWork *wk);
static void MicTest_VBlankClAct(MicTestWork *wk);
static void MicTest_InitBG(MicTestBG *bg, u32 heapId);
static void MicTest_FreeBG(MicTestBG *bg);
static void MicTest_UpdateBG(MicTestBG *bg);
static void MicTest_LoadGraphics(MicTestBG *bg, u32 heapId);
static void MicTest_VBlankBG(MicTestBG *bg);
static void MicTest_CreateWindows(MicTestBG *bg, u32 heapId);
static void MicTest_FreeWindows(MicTestBG *bg);
static void MicTestMic_Init(MicTestMic *mic, u32 heapId, MICCallback callback, void *arg);
static void MicTestMic_Free(MicTestMic *mic);
static void MicTestMic_Update(MicTestMic *mic);
static void MicTestMic_Start(MicTestMic *mic);
static void MicTestMic_Stop(MicTestMic *mic);
static int MicTestMic_GetPeak(MicTestMic *mic);
static void MicTest_OnBufferFull(MICResult result, void *arg);
static u32 MicTest_GetLevel(u8 peak);
static BOOL MicTest_AreNotesFree(MicTestWork *wk);
static BOOL MicTest_SpawnNote(MicTestWork *wk, u32 heapId, s16 x, s16 y, TCBManager *tcbManager);
static void MicTestNote_Start(TCBManager *tcbManager, ClActor *actor, MicTestNote *note, s16 x, s16 y, fx32 speed,
                              int angle, int swayRadius, u32 delay);
static void MicTestNote_Task(TCB *tcb, void *data);
static void MicTest_StopNotes(MicTestWork *wk);
static s32 MicTest_GetTouchedRect(void);
static BOOL MicTest_IsExitPressed(void);

const GameProcFunctions MIC_TEST_PROC_FUNCTIONS = { MicTest_Init, MicTest_Main, MicTest_Exit };

static u32 sMicTestBufferFullCount;

static const u16 sMicTestWindowColors[WINDOW_COUNT] = { 0x044f, 0x044f, 0x3c40 };

// The Back button
static const TouchRect sMicTestExitRects[2] = {
    { 160, 192, 176, 0 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const BGSysLCDConfig sMicTestLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const MicTestWindowSetup sMicTestWindowSetups[WINDOW_COUNT] = {
    { 0, 0, 1, 10, 3, 13, 1, 0, { 0 } },
    { 4, 5, 6, 22, 8, 13, 1, 0, { 0 } },
    { 4, 23, 21, 6, 2, 13, 177, 0, { 0 } },
};

static const ClActorSetup sMicTestActorSetups[5] = {
    { 64, 96, 0, 0, 0 }, { 192, 96, 0, 0, 0 }, { 128, 96, 0, 0, 0 }, { 216, 176, 0, 0, 1 }, { 0, 0, 0, 0, 0 },
};

static const BGSysVRAMConfig sMicTestVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const MicTestActorSetup sMicTestActors[ACTOR_COUNT] = {
    { RES_SPEAKER, CLACT_SURFACE_MAIN, 0 }, { RES_SPEAKER, CLACT_SURFACE_MAIN, 1 },
    { RES_CENTER, CLACT_SURFACE_MAIN, 2 },  { RES_BACK_BUTTON, CLACT_SURFACE_SUB, 3 },
    { RES_NOTE, CLACT_SURFACE_MAIN, 4 },    { RES_NOTE, CLACT_SURFACE_MAIN, 4 },
    { RES_NOTE, CLACT_SURFACE_MAIN, 4 },
};

static const MicTestBGSetup sMicTestBGSetups[BG_COUNT] = {
    { { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800), GX_BG_CHARBASE(0x0000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      0,
      BGMODE_TEXT },
    { { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf000), GX_BG_CHARBASE(0x4000), 0x4000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      1,
      BGMODE_TEXT },
    { { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe800), GX_BG_CHARBASE(0x8000), 0x4000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      2,
      BGMODE_TEXT },
    { { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800), GX_BG_CHARBASE(0x0000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      4,
      BGMODE_TEXT },
    { { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf000), GX_BG_CHARBASE(0x4000), 0x4000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      5,
      BGMODE_TEXT },
};

static BOOL MicTest_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MicTestWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MICTEST, 0x70000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(MicTestWork), HEAPID_MICTEST);
    sys_memset(wk, 0, sizeof(MicTestWork));
    wk->heapId = HEAPID_MICTEST;
    MicTest_InitDisplay(HEAPID_MICTEST);
    MicTest_SetVRAM();
    wk->tcbMemory = GFL_HeapAllocate(wk->heapId, GFL_TCBMgrCalcAllocSize(5), FALSE, "mictest.c", 541);
    wk->tcbManager = GFL_TCBMgrCreate(5, wk->tcbMemory);
    wk->vblankTask = GFL_VBlankTCBAdd(MicTest_VBlank, wk, 1);
    MicTest_InitBG(&wk->bg, wk->heapId);
    MicTest_LoadGraphics(&wk->bg, wk->heapId);
    MicTest_CreateWindows(&wk->bg, wk->heapId);
    MicTest_InitClAct(wk, wk->heapId);
    MicTest_CreateActors(wk, wk->heapId);
    MicTestMic_Init(&wk->mic, wk->heapId, MicTest_OnBufferFull, wk);
    MicTestSeq_Init(&wk->seq, wk, MicTest_SeqWipeIn);
    GFL_SndBGMFadeOut(10);
    return TRUE;
}

static BOOL MicTest_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MicTestWork *wk = work;
    HeapID heapId;

    MicTestMic_Free(&wk->mic);
    GFL_TCBRemove(wk->vblankTask);
    func_0203a610(wk->tcbManager);
    GFL_HeapFree(wk->tcbMemory);
    MicTest_FreeActors(wk);
    MicTest_FreeClAct(wk);
    MicTest_FreeWindows(&wk->bg);
    MicTest_FreeBG(&wk->bg);
    MicTest_ExitDisplay();
    heapId = wk->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_SndBGMFadeIn(10);
    GCTX_ProcMgrReplaceProc(OVERLAY_ID(162), &TITLE_PROC_FUNCTIONS, NULL);
    return TRUE;
}

static BOOL MicTest_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MicTestWork *wk = work;

    if (MicTestSeq_IsHalted(&wk->seq)) {
        return TRUE;
    }
    MicTestSeq_Run(&wk->seq);
    MicTest_UpdateClAct(wk);
    MicTest_UpdateBG(&wk->bg);
    MicTestMic_Update(&wk->mic);
    GFL_TCBMgrUpdate(wk->tcbManager);
    return FALSE;
}

static void MicTestSeq_Init(MicTestSeq *seq, void *work, MicTestSeqFunc func) {
    seq->work = work;
    seq->halted = FALSE;
    MicTestSeq_SetFunc(seq, func);
}

static void MicTestSeq_Run(MicTestSeq *seq) {
    if (!MicTestSeq_IsHalted(seq)) {
        seq->func(seq, &seq->state);
    }
}

static void MicTestSeq_SetFunc(MicTestSeq *seq, MicTestSeqFunc func) {
    seq->func = func;
    seq->state = 0;
}

static void *MicTestSeq_GetWork(MicTestSeq *seq) {
    return seq->work;
}

static BOOL MicTestSeq_IsHalted(MicTestSeq *seq) {
    return seq->halted;
}

static void MicTestSeq_Halt(MicTestSeq *seq) {
    seq->halted = TRUE;
}

static void MicTest_SeqWipeIn(MicTestSeq *seq, u32 *state) {
    MicTestWork *wk = MicTestSeq_GetWork(seq);

    switch (*state) {
    case 0:
        GFL_WipeSet(0, 1, 1, 0, 6, 1, wk->heapId);
        *state = 1;
        break;
    case 1:
        if (GFL_WipeIsFinished()) {
            *state = 2;
        }
        break;
    case 2:
        MicTestSeq_SetFunc(seq, MicTest_SeqListen);
        break;
    }
}

static void MicTest_SeqListen(MicTestSeq *seq, u32 *state) {
    MicTestWork *wk = MicTestSeq_GetWork(seq);

    switch (*state) {
    case 0:
        MicTestMic_Start(&wk->mic);
        *state = 1;
        break;
    case 1:
        if (MicTest_IsExitPressed()) {
            if (func_02006e3c()) {
                GFL_SndSEPlay(SEQ_SE_CANCEL1);
                *state = 2;
            }
        }
        break;
    case 2:
        MicTestMic_Stop(&wk->mic);
        *state = 3;
        break;
    case 3:
        MicTestSeq_SetFunc(seq, MicTest_SeqPressBack);
        break;
    }
}

static void MicTest_SeqPressBack(MicTestSeq *seq, u32 *state) {
    MicTestWork *wk = MicTestSeq_GetWork(seq);

    switch (*state) {
    case 0:
        func_0204c488(wk->actors[ACTOR_BACK_BUTTON], 1);
        *state = 1;
        break;
    case 1:
        if (!func_0204c560(wk->actors[ACTOR_BACK_BUTTON])) {
            *state = 2;
        }
        break;
    case 2:
        MicTestSeq_SetFunc(seq, MicTest_SeqWipeOut);
        break;
    }
}

static void MicTest_SeqWipeOut(MicTestSeq *seq, u32 *state) {
    MicTestWork *wk = MicTestSeq_GetWork(seq);

    switch (*state) {
    case 0:
        GFL_WipeSet(0, 0, 0, 0, 6, 1, wk->heapId);
        *state = 1;
        break;
    case 1:
        if (GFL_WipeIsFinished()) {
            *state = 2;
        }
        break;
    case 2:
        MicTestSeq_SetFunc(seq, MicTest_SeqEnd);
        break;
    }
}

static void MicTest_SeqEnd(MicTestSeq *seq, u32 *state) {
    MicTest_StopNotes(MicTestSeq_GetWork(seq));
    MicTestSeq_Halt(seq);
}

static void MicTest_InitDisplay(HeapID heapId) {
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    Wipe_HideWindows(0);
    Wipe_HideWindows(1);
}

static void MicTest_ExitDisplay(void) {
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
}

static void MicTest_SetVRAM(void) {
    GFL_BGSysSetVRAMBanks(&sMicTestVRAMConfig);
}

static void MicTest_VBlank(TCB *tcb, void *data) {
    MicTestWork *wk = data;

    NNS_GfdDoVramTransfer();
    MicTest_VBlankClAct(wk);
    MicTest_VBlankBG(&wk->bg);
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

static void MicTest_InitClAct(MicTestWork *wk, u32 heapId) {
    ClActSys_Create(&data_02093f08, &sMicTestVRAMConfig, heapId);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void MicTest_FreeClAct(MicTestWork *wk) {
    func_0204b758();
}

static void MicTest_UpdateClAct(MicTestWork *wk) {
    u16 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c4e0(wk->actors[i], FX32_ONE);
    }
    func_0204b794();
}

static void MicTest_CreateActors(MicTestWork *wk, HeapID heapId) {
    u16 i;
    ArcTool *arc;
    ArcTool *commonArc;

    wk->unit = func_0204bf1c(32, 0, heapId);
    arc = GFL_ArcSysCreateFileHandle(ARCID_MIC_TEST, heapId);
    commonArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    wk->resources[RES_SPEAKER + 1] = func_0204bba0(arc, 6, 0, 0, heapId);
    wk->resources[RES_SPEAKER] = func_0204b81c(arc, 7, 0, 0, heapId);
    wk->resources[RES_SPEAKER + 2] = func_0204bde0(arc, 8, 9, heapId);
    wk->resources[RES_CENTER + 1] = func_0204bba0(arc, 10, 0, 32, heapId);
    wk->resources[RES_CENTER] = func_0204b81c(arc, 11, 0, 0, heapId);
    wk->resources[RES_CENTER + 2] = func_0204bde0(arc, 12, 13, heapId);
    wk->resources[RES_NOTE + 1] = func_0204bba0(arc, 14, 0, 64, heapId);
    wk->resources[RES_NOTE] = func_0204b81c(arc, 15, 0, 0, heapId);
    wk->resources[RES_NOTE + 2] = func_0204bde0(arc, 16, 17, heapId);
    wk->resources[RES_BACK_BUTTON + 1] = func_0204bba0(commonArc, 31, 1, 0, heapId);
    wk->resources[RES_BACK_BUTTON] = func_0204b81c(commonArc, 32, 0, 1, heapId);
    wk->resources[RES_BACK_BUTTON + 2] = func_0204bde0(arc, 18, 19, heapId);
    GFL_ArcToolFree(commonArc);
    GFL_ArcToolFree(arc);
    for (i = 0; i < ACTOR_COUNT; i++) {
        const MicTestActorSetup *actor = &sMicTestActors[i];

        wk->actors[i] = func_0204c040(wk->unit, wk->resources[actor->resource], wk->resources[actor->resource + 1],
                                      wk->resources[actor->resource + 2], &sMicTestActorSetups[actor->setup],
                                      (u16)actor->surface, heapId);
        func_0204c520(wk->actors[i], TRUE);
    }
    for (i = 0; i < NOTE_COUNT; i++) {
        func_0204c124(wk->actors[ACTOR_NOTE_FIRST + i], FALSE);
    }
    func_0204c244(wk->actors[0], 0);
    func_0204c2b0(wk->actors[0], 1, 1);
    wk->spawnTimer = NOTE_INTERVAL;
}

static void MicTest_FreeActors(MicTestWork *wk) {
    func_0204bf98(wk->unit);
}

static void MicTest_VBlankClAct(MicTestWork *wk) {
    func_0204b7c8();
}

static void MicTest_InitBG(MicTestBG *bg, u32 heapId) {
    u32 i;

    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sMicTestLCDConfig);
    bg->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    for (i = 0; i < BG_COUNT; i++) {
        u8 bgNum = sMicTestBGSetups[i].bg;

        GFL_BGSysCreateBG(bgNum, &sMicTestBGSetups[i].setup, sMicTestBGSetups[i].mode);
        GFL_BGSysClearScr(bgNum);
        GFL_BGSysClearCharCore(bgNum, 0x20, 0, heapId);
        GFL_BGSysSetBGEnabled(bgNum, TRUE);
    }
}

static void MicTest_FreeBG(MicTestBG *bg) {
    u32 i;

    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysReleaseBG(sMicTestBGSetups[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void MicTest_UpdateBG(MicTestBG *bg) {
}

static void MicTest_LoadGraphics(MicTestBG *bg, u32 heapId) {
    GFL_BGSysLoadNCLRDefault(ARCID_MIC_TEST, 0, 0, 0, 0x20, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_MIC_TEST, 1, 4, 0, 0x20, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1a0, 0x20, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1a0, 0x20, heapId);
    GFL_BGSysLoadNCGRStatic(ARCID_MIC_TEST, 4, 5, 0, 0, FALSE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_MIC_TEST, 5, 5, 0, 0, FALSE, heapId);
    GFL_BGSysLoadNCGRStatic(ARCID_MIC_TEST, 2, 1, 0, 0, FALSE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_MIC_TEST, 3, 1, 0, 0, FALSE, heapId);
}

static void MicTest_VBlankBG(MicTestBG *bg) {
    GFL_BGSysUpdate();
}

static void MicTest_CreateWindows(MicTestBG *bg, u32 heapId) {
    int i;

    bg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_INTRO_BTL_SERVER_FLOW, heapId);
    for (i = 0; i < WINDOW_COUNT; i++) {
        const MicTestWindowSetup *setup = &sMicTestWindowSetups[i];
        GFLBitmap *bitmap;
        StrBuf *str;
        BmpWin *window;

        bg->windows[i] = BmpWin_CreateStatic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette,
                                             setup->charPos);
        bitmap = BmpWin_GetBitmap(bg->windows[i]);
        GFL_BitmapFill(bitmap, 0);
        BmpWin_ClearScreen(bg->windows[i]);
        str = GFL_MsgDataLoadStrbufNew(bg->msgData, i);
        GFL_TextRendererDrawToBitmapEx(bitmap, setup->textX, 0, str, bg->font, sMicTestWindowColors[i]);
        GFL_StrBufFree(str);
        window = bg->windows[i];
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    }
}

static void MicTest_FreeWindows(MicTestBG *bg) {
    int i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(bg->windows[i]), 0);
        BmpWin_ClearScreen(bg->windows[i]);
        BmpWin_Free(bg->windows[i]);
    }
    GFL_MsgDataFree(bg->msgData);
    GFL_FontFree(bg->font);
}

static void MicTestMic_Init(MicTestMic *mic, u32 heapId, MICCallback callback, void *arg) {
    u8 *buffer;
    u32 bufferSize;

    setupMic(heapId);
    GCTX_HIDBlockSleep(8);
    bufferSize = 0x120;
    buffer = GFL_HeapAllocate(heapId, bufferSize, FALSE, "mictest.c", 1539);
    sys_memset(buffer, 0, bufferSize);
    mic->buffer = buffer;
    mic->param.type = MIC_SAMPLING_TYPE_8BIT;
    mic->param.buffer = (void *)(((u32)buffer + 0x1f) & ~0x1f);
    mic->param.rate = MIC_SAMPLING_RATE_8180;
    mic->param.loop = TRUE;
    // Less the 32 bytes that aligning the start may skip
    mic->param.size = bufferSize;
    mic->param.size -= 0x20;
    mic->param.fullCallback = callback;
    mic->param.fullArg = arg;
    mic->wasOpen = func_02006e3c();
}

static void MicTestMic_Free(MicTestMic *mic) {
    GFL_HeapFree(mic->buffer);
    ampOffFreeBlocks();
    GCTX_HIDUnblockSleep(8);
}

static void MicTestMic_Update(MicTestMic *mic) {
    BOOL open;

    func_02006e0c(1);
    open = func_02006e3c();
    if (!open || !mic->wasOpen) {
        if (!mic->asleep) {
            if (mic->sampling) {
                MicTestMic_Stop(mic);
                mic->resume = TRUE;
            }
            mic->asleep = TRUE;
        }
        if (open && !mic->wasOpen) {
            mic->asleep = FALSE;
            if (mic->resume) {
                MicTestMic_Start(mic);
                mic->resume = FALSE;
            }
        }
    }
    mic->wasOpen = open;
}

static void MicTestMic_Start(MicTestMic *mic) {
    if (!mic->asleep) {
        MIC_StartAutoSampling(&mic->param);
        mic->sampling = TRUE;
    } else {
        mic->resume = TRUE;
    }
}

static void MicTestMic_Stop(MicTestMic *mic) {
    MIC_StopAutoSampling();
    mic->sampling = FALSE;
}

static int MicTestMic_GetPeak(MicTestMic *mic) {
    int i;
    int peak = 0;
    int last = mic->param.size - 1;
    u8 *samples = mic->param.buffer;

    for (i = 0; i < last; i++) {
        u8 sample = samples[i];

        if (sample < 0x80) {
            sample = 0xff - sample;
        }
        if (sample > peak) {
            peak = sample;
        }
    }
    return peak;
}

static void MicTest_OnBufferFull(MICResult result, void *arg) {
    MicTestWork *wk = arg;
    MicTestMic *mic;
    int peak;
    u32 level;

    if (result != MIC_RESULT_SUCCESS) {
        return;
    }
    mic = &wk->mic;
    peak = MicTestMic_GetPeak(mic);
    if (mic->peak < peak) {
        level = MicTest_GetLevel(peak);
        mic->peak = peak;
        mic->newPeak = TRUE;
    } else {
        level = MicTest_GetLevel(mic->peak);
        mic->peak -= 2;
    }
    if (level != 0) {
        if (mic->newPeak) {
            func_0204c488(wk->actors[ACTOR_CENTER], 1);
            mic->newPeak = FALSE;
        }
        if (MicTest_AreNotesFree(wk)) {
            MicTest_SpawnNote(wk, wk->heapId, 128, 64, wk->tcbManager);
        } else {
            wk->spawnTimer--;
            if (wk->spawnTimer == 0) {
                MicTest_SpawnNote(wk, wk->heapId, 128, 64, wk->tcbManager);
                wk->spawnTimer = NOTE_INTERVAL;
            }
        }
        func_0204c488(wk->actors[0], level);
        func_0204c488(wk->actors[1], level);
    } else if (level == 0) {
        func_0204c488(wk->actors[ACTOR_CENTER], 0);
        func_0204c488(wk->actors[0], 0);
        func_0204c488(wk->actors[1], 0);
    }
    sMicTestBufferFullCount++;
}

// The animation of the speaker for a sample as loud as peak
static u32 MicTest_GetLevel(u8 peak) {
    if (peak <= 150) {
        return 0;
    }
    if (peak >= 151 && peak <= 162) {
        return 1;
    }
    if (peak >= 163 && peak <= 176) {
        return 2;
    }
    if (peak >= 177 && peak <= 197) {
        return 3;
    }
    if (peak >= 198) {
        return 4;
    }
}

static BOOL MicTest_AreNotesFree(MicTestWork *wk) {
    int i;

    for (i = 0; i < NOTE_COUNT; i++) {
        if (wk->notes[i].task != NULL) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL MicTest_SpawnNote(MicTestWork *wk, u32 heapId, s16 x, s16 y, TCBManager *tcbManager) {
    ClActor *actor = NULL;
    MicTestNote *note = NULL;
    int i;

    for (i = 0; i < NOTE_COUNT; i++) {
        if (wk->notes[i].task == NULL) {
            note = &wk->notes[i];
            actor = wk->actors[ACTOR_NOTE_FIRST + i];
            break;
        }
    }
    if (note != NULL) {
        fx32 speed = GFL_RandomLC(0x1001) + 0x1333;
        int angle = GFL_RandomLC(61) + 60;
        int swayRadius = GFL_RandomLC(15) + 6;

        MicTestNote_Start(tcbManager, actor, note, x, y, speed, angle, swayRadius, 1);
        return TRUE;
    }
    return FALSE;
}

static void MicTestNote_Start(TCBManager *tcbManager, ClActor *actor, MicTestNote *note, s16 x, s16 y, fx32 speed,
                              int angle, int swayRadius, u32 delay) {
    MicTestNote *work;

    note->task = GFL_TCBMgrAddTask(tcbManager, MicTestNote_Task, note, 0);
    work = GFL_TCBGetData(note->task);
    work->actor = actor;
    work->delay = delay;
    work->position.x = FX32_CONST(x);
    work->position.y = FX32_CONST(y);
    work->position.z = 0;
    work->velocity.x = FX_Mul(speed, func_02044304(angle + 90));
    work->velocity.y = -FX_Mul(speed, func_02044304(angle));
    work->velocity.z = 0;
    work->phase = 0;
    work->phaseStep = FX32_CONST(6);
    work->swayRadius = swayRadius;
    func_0204c228(work->actor, x, 0);
    func_0204c228(work->actor, y, 1);
}

static void MicTestNote_Task(TCB *tcb, void *data) {
    MicTestNote *note = data;
    s16 x;
    s16 y;
    s16 sway;

    if (note->delay != 0) {
        note->delay--;
        if (note->delay == 0) {
            func_0204c124(note->actor, TRUE);
        }
        return;
    }
    VEC_Add(&note->position, &note->velocity, &note->position);
    x = note->position.x >> FX32_SHIFT;
    y = note->position.y >> FX32_SHIFT;
    note->phase += note->phaseStep;
    while (note->phase > FX32_CONST(360)) {
        note->phase -= FX32_CONST(360);
    }
    sway = (func_02044304(note->phase >> FX32_SHIFT) * note->swayRadius) >> FX32_SHIFT;
    x += sway;
    func_0204c228(note->actor, x, 0);
    func_0204c228(note->actor, y, 1);
    if (y < -16) {
        func_0204c124(note->actor, FALSE);
        GFL_TCBRemove(tcb);
        note->task = NULL;
    }
}

static void MicTest_StopNotes(MicTestWork *wk) {
    int i;

    for (i = 0; i < NOTE_COUNT; i++) {
        if (wk->notes[i].task != NULL) {
            GFL_TCBRemove(wk->notes[i].task);
            wk->notes[i].task = NULL;
        }
    }
}

static s32 MicTest_GetTouchedRect(void) {
    return func_0203da0c(sMicTestExitRects);
}

static BOOL MicTest_IsExitPressed(void) {
    if (MicTest_GetTouchedRect() == 0 || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
        return TRUE;
    }
    return FALSE;
}
