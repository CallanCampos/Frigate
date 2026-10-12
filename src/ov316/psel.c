#include "types.h"
#include "app/psel.h"
#include "app/psel_graphic.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "pml/poke_graphic.h"
#include "system/app_taskmenu.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wordset.h"
#include "text/script/aspertia_city.h"

// The choice of a starter Pokémon, named after the ROM's "psel.c": the gift box opens on the main screen, a 3D scene
// whose camera follows a curve, and the player picks one of its three Poké Balls with the keys or by touching it. The
// sub screen shows the three Pokémon, the picked one jumping to the front.

#define PSEL_POKE_COUNT 3
// No ball is picked
#define PSEL_CHOICE_NONE 4

// The members of the archive (ARCID_PSEL) that the code names
enum {
    PSEL_ARC_SUB_BG_NCLR = 7,
    PSEL_ARC_CURSOR_NCLR = 11,
    PSEL_ARC_SUB_BG_NCGR = 16,
    PSEL_ARC_CURSOR_NCGR = 17,
    PSEL_ARC_SUB_BG_BACK_NSCR = 19,
    PSEL_ARC_SUB_BG_FRONT_NSCR = 20,
    PSEL_ARC_CURSOR_NCER = 24,
    PSEL_ARC_CURSOR_NANR = 28,
    PSEL_ARC_BOX_NSBMD = 34,
    PSEL_ARC_BOX_ANIM_1 = 35,
    PSEL_ARC_BOX_ANIM_2 = 36,
    PSEL_ARC_BOX_ANIM_3 = 37,
    PSEL_ARC_CAMERA_CURVE = 38,
    PSEL_ARC_BALL_NSBMD = 39,
    PSEL_ARC_BALL_ANIM_1 = 40,
    PSEL_ARC_BALL_ANIM_2 = 41,
};

// The sequences of the process, which sPselSeqFuncs runs
enum {
    PSEL_SEQ_START,
    PSEL_SEQ_INIT,
    PSEL_SEQ_LOAD,
    PSEL_SEQ_FADE_IN,
    PSEL_SEQ_MAIN,
    PSEL_SEQ_FADE_OUT,
    PSEL_SEQ_UNLOAD,
    PSEL_SEQ_FREE,
    PSEL_SEQ_END,
};

// What the VBlank task is asked to show or hide: the main screen's text BGs, the sub screen's, and the message BG alone
#define PSEL_BG_REQ_MAIN_ON 0x4
#define PSEL_BG_REQ_MAIN_OFF 0x8
#define PSEL_BG_REQ_SUB_ON 0x10
#define PSEL_BG_REQ_SUB_OFF 0x20
#define PSEL_BG_REQ_MSG_ON 0x40
#define PSEL_BG_REQ_MSG_OFF 0x80

// The frames of the intro, the box opening: when the screen flashes white, when the balls appear, and its length
#define PSEL_INTRO_FLASH_FRAME 160
#define PSEL_INTRO_BALLS_FRAME 162
#define PSEL_INTRO_SE_FRAME 202
#define PSEL_INTRO_FRAMES 278

// The 3D objects' and the camera curve's animation speed: half a frame each frame
#define PSEL_ANIME_SPEED FX32_HALF

// A Pokémon's palette, and the background's: at its own colors, fading to the other ones, at them, and fading back
enum {
    PSEL_PAL_NORMAL,
    PSEL_PAL_TO_DARK,
    PSEL_PAL_DARK,
    PSEL_PAL_TO_NORMAL,
};

// The states of a Pokémon's picture: small at the back, jumping to the middle, waiting there, jumping to the front,
// and big at the front
enum {
    PSEL_POKE_BACK,
    PSEL_POKE_JUMP_1,
    PSEL_POKE_MIDDLE,
    PSEL_POKE_JUMP_2,
    PSEL_POKE_FRONT,
};

// What a Pokémon's picture is asked to do: come to the front, or go back
enum {
    PSEL_POKE_REQ_NONE,
    PSEL_POKE_REQ_FRONT,
    PSEL_POKE_REQ_BACK,
};

// The 3D objects
enum {
    PSEL_OBJ_BOX,
    PSEL_OBJ_BALL_1,
    PSEL_OBJ_BALL_2,
    PSEL_OBJ_BALL_3,
    PSEL_OBJ_COUNT,
};

typedef struct {
    u16 actor;
    SRTMatrix status;
    BOOL draw;
} PselObj;

typedef struct {
    u32 chars;
    u32 pltt;
    u32 cellAnim;
} PselPokeRes;

// A Pokémon on the sub screen: its still picture (0), shown while it fades in, and the scaled one (1) that jumps
typedef struct {
    PselPokeRes res[2];
    ClActor *clact[2];
    u16 palNormal[16];
    u16 palDark[16];
    u16 palNow[16];
    u16 palState;
    u16 palStep;
    u16 frame;
    u32 state;
    u32 request;
    // Set with a request, and cleared once the picture moves
    BOOL unmoved;
} PselPoke;

typedef struct {
    HeapID heapId;
    PselParam *param;
    BOOL touchMode;
    s32 step;
    u32 frame;
    PselGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    TCB *vblankTask;
    u32 bgRequest;
    G3DManager *g3dMgr;
    u16 scene;
    // The box, then the three balls, as indices into objs
    u16 objIdx[PSEL_OBJ_COUNT];
    u16 objCount;
    PselObj *objs;
    G3DCurve *cameraCurve;
    s32 cameraBall;
    BOOL cameraReverse;
    u32 cameraFrame;
    u32 introFrame;
    PselPoke poke[PSEL_POKE_COUNT];
    s32 fadeCount;
    u32 fadeState;
    u32 cursorChars;
    u32 cursorPltt;
    u32 cursorCellAnim;
    ClActor *cursor;
    s32 choice;
    // The ball whose animation plays
    s32 ball;
    // Set when the picked Pokémon reaches the front
    BOOL pokeArrived;
    MsgData *msgData;
    BmpWin *win[3];
    BOOL winDone[3];
    PrintStream *stream;
    TCBExManager *tcbMgr;
    StrBuf *strbuf;
    AppTaskMenuRes *menuRes;
    AppTaskMenuWin *button;
    AppTaskMenu *yesNo;
    u16 unk264;
    // The sub screen's BG palettes of the three Pokémon and of no choice, the one the fade started from and the shown one
    u16 bgPal[4][0x80];
    u16 bgPalFrom[0x80];
    u16 bgPalNow[0x80];
    BOOL bgPalActive;
    u8 bgPalStep;
    u8 bgPalTarget;
    s32 fadeFreq;
} PselWork;

// The camera curve's frames of a ball: the zoom on it and its end, the picked ball's flight and its end, and when the
// screen starts to fade out
typedef struct {
    u16 zoom[2];
    u16 decideStart;
    u16 decideEnd;
    u16 fadeOut;
} PselCameraFrames;

// A Pokémon's picture at the back, in the middle and at the front: its position, its scale in 16ths, how long it waits
// in the middle, and the length and height of its two jumps
typedef struct {
    u8 pos[3][2];
    u8 scale[3];
    u8 wait;
    u8 duration[2];
    u8 height[2];
} PselPokeMove;

// A Pokémon's files: the scaled picture's characters, cells and animation, and its two palettes
typedef struct {
    u8 chars;
    u8 pltt;
    u8 cell;
    u8 anim;
    u8 plttDark;
} PselPokeFiles;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
    u8 fromEnd;
    u8 unk7;
    u8 unk8;
} PselWinSetup;

typedef struct {
    u8 x;
    u8 y;
} PselCursorPos;

typedef s32 (*PselSeqFunc)(PselWork *wk, s32 *step);

static void PselCurve_SetFrame(G3DCurve *curve, s32 frame);
static PselObj *PselObj_AllocAll(HeapID heapId, u16 count);
static void PselObj_FreeAll(PselObj *objs);
static BOOL PselProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PselProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PselProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static void Psel_VBlank(TCB *tcb, void *data);
static void Psel3D_Draw(PselWork *wk);
static void Psel3D_Init(PselWork *wk);
static void Psel3D_Free(PselWork *wk);
static void Psel3D_Load(PselWork *wk);
static void Psel3D_Unload(PselWork *wk);
static void Psel3D_UpdateIntro(PselWork *wk);
static void PselCamera_StartZoom(PselWork *wk, s32 ball, BOOL reverse);
static BOOL PselCamera_IsZoomDone(PselWork *wk);
static void PselCamera_UpdateZoom(PselWork *wk);
static void PselCamera_StartDecide(PselWork *wk, s32 ball);
static void PselCamera_UpdateDecide(PselWork *wk);
static void PselBall_Select(PselWork *wk, s32 ball);
static void PselBall_Update(PselWork *wk);
static void PselPoke_Load(PselWork *wk);
static void PselPoke_Unload(PselWork *wk);
static s16 PselPoke_CalcJump(ClActorPos *pos, ClActorScale *scale, u8 x0, u8 y0, u8 x1, u8 y1, u8 scale0, u8 scale1,
                             u32 duration, u8 height, u32 frame);
static void PselPoke_Update(PselWork *wk);
static void PselPoke_Select(PselWork *wk, s32 ball);
static void PselPoke_StartFadeIn(PselWork *wk);
static BOOL PselPoke_IsFadeInDone(PselWork *wk);
static void PselPoke_UpdateFadeIn(PselWork *wk);
static void PselCursor_Load(PselWork *wk);
static void PselCursor_Unload(PselWork *wk);
static void PselCursor_Show(PselWork *wk, BOOL show);
static void PselCursor_Move(PselWork *wk);
static void PselBand_Init(PselWork *wk);
static void PselBand_Free(PselWork *wk);
static void PselBand_LoadScreens(PselWork *wk);
static void PselBand_ClearScreens(PselWork *wk);
static void PselSubBG_Load(PselWork *wk);
static void PselSubBG_Unload(PselWork *wk);
static void PselText_Init(PselWork *wk);
static void PselText_Free(PselWork *wk);
static void PselText_Update(PselWork *wk);
static BOOL PselText_Flush(PselWork *wk);
static void PselText_StartStream(PselWork *wk, u32 messageId);
static BOOL PselText_UpdateStream(PselWork *wk);
static void PselText_ClearStream(PselWork *wk);
static void PselText_PrintType(PselWork *wk, s32 ball);
static void PselText_ClearType(PselWork *wk);
static void PselMenu_InitRes(PselWork *wk);
static void PselMenu_FreeRes(PselWork *wk);
static void PselButton_Create(PselWork *wk);
static void PselButton_Free(PselWork *wk);
static void PselButton_Update(PselWork *wk);
static void PselButton_Decide(PselWork *wk);
static BOOL PselButton_IsFlashFinished(PselWork *wk);
static void PselYesNo_Create(PselWork *wk);
static void PselYesNo_Free(PselWork *wk);
static void PselYesNo_Update(PselWork *wk);
static BOOL PselYesNo_IsFlashFinished(PselWork *wk);
static BOOL PselYesNo_IsYes(PselWork *wk);
static void Psel_SplitColor(u16 color, u16 *r, u16 *g, u16 *b);
static void PselPokePal_Load(PselWork *wk);
static void PselPokePal_Free(PselWork *wk);
static void PselPokePal_Update(PselWork *wk);
static void PselBGPal_Load(PselWork *wk);
static void PselBGPal_Free(PselWork *wk);
static void PselBGPal_Update(PselWork *wk);
static void PselPal_Load(PselWork *wk);
static void PselPal_Free(PselWork *wk);
static void PselPal_Update(PselWork *wk);
static void PselPokePal_Request(PselWork *wk, u8 poke, u32 state);
static void PselBGPal_SetTarget(PselWork *wk, u8 target);
static s32 PselSeq_Start(PselWork *wk, s32 *step);
static s32 PselSeq_Init(PselWork *wk, s32 *step);
static s32 PselSeq_Load(PselWork *wk, s32 *step);
static s32 PselSeq_FadeIn(PselWork *wk, s32 *step);
static s32 PselSeq_Main(PselWork *wk, s32 *step);
static s32 PselSeq_FadeOut(PselWork *wk, s32 *step);
static s32 PselSeq_Unload(PselWork *wk, s32 *step);
static s32 PselSeq_Free(PselWork *wk, s32 *step);
static s32 PselSeq_End(PselWork *wk, s32 *step);

// The OBJ palettes of the sub screen that the Pokémon take
static const u8 sPselPokePaletteSlots[PSEL_POKE_COUNT] = { 0, 1, 2 };

// The sub screen's BG palettes of the three Pokémon, and of no choice
static const u8 sPselBGPaletteFiles[4] = { 9, 8, 10, 7 };

static const PselCursorPos sPselCursorPositions[PSEL_POKE_COUNT] = { { 42, 29 }, { 128, 29 }, { 211, 29 } };

static const u16 sPselSpecies[PSEL_POKE_COUNT] = { SPECIES_SNIVY, SPECIES_TEPIG, SPECIES_OSHAWOTT };

static const G3DSceneAnimationSetup sPselBallAnimations[] = { { 5, 0 }, { 6, 0 } };

static const u32 sPselTypeMessages[PSEL_POKE_COUNT] = {
    AspertiaCity_Text_GrassTypePokemon,
    AspertiaCity_Text_FireTypePokemon,
    AspertiaCity_Text_WaterTypePokemon,
};

static const G3DSceneAnimationSetup sPselBoxAnimations[] = { { 1, 0 }, { 2, 0 }, { 3, 0 } };

static const PselPokeFiles sPselPokeFiles[PSEL_POKE_COUNT] = {
    { 12, 0, 21, 25, 1 },
    { 13, 2, 22, 26, 3 },
    { 14, 4, 23, 27, 5 },
};

static const G3DSceneResourceSetup sPselSceneResources[] = {
    { ARCID_PSEL, PSEL_ARC_BOX_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_PSEL, PSEL_ARC_BOX_ANIM_1, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_PSEL, PSEL_ARC_BOX_ANIM_2, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_PSEL, PSEL_ARC_BOX_ANIM_3, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_PSEL, PSEL_ARC_BALL_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_PSEL, PSEL_ARC_BALL_ANIM_1, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_PSEL, PSEL_ARC_BALL_ANIM_2, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneActorSetup sPselSceneActors[] = {
    { 0, 0, 0, 0, sPselBoxAnimations, NELEMS(sPselBoxAnimations) },
    { 4, 0, 4, 0, sPselBallAnimations, NELEMS(sPselBallAnimations) },
    { 4, 0, 4, 0, sPselBallAnimations, NELEMS(sPselBallAnimations) },
    { 4, 0, 4, 0, sPselBallAnimations, NELEMS(sPselBallAnimations) },
};

static const G3DSceneSetup sPselScene = {
    sPselSceneResources,
    NELEMS(sPselSceneResources),
    sPselSceneActors,
    NELEMS(sPselSceneActors),
};

// The three balls, and the button that confirms the choice
static const TouchRect sPselTouchRects[] = {
    { TOUCH_RECT_CIRCLE, 60, 90, 27 },
    { TOUCH_RECT_CIRCLE, 128, 90, 27 },
    { TOUCH_RECT_CIRCLE, 200, 90, 27 },
    { 168, 191, 72, 183 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The window behind the Pokémon's picture, the message window, and the sub screen's
static const PselWinSetup sPselWinSetups[3] = {
    { 2, 4, 5, 24, 14, 14, TRUE, 0, 0 },
    { 2, 1, 19, 30, 2, 14, TRUE, 0, 0 },
    { 7, 1, 18, 30, 4, 1, TRUE, 0, 0 },
};

static const PselCameraFrames sPselCameraFrames[PSEL_POKE_COUNT] = {
    { { 340, 358 }, 440, 558, 500 },
    { { 360, 378 }, 560, 678, 620 },
    { { 380, 398 }, 680, 798, 740 },
};

static const VecFx32 sPselBallPositions[PSEL_POKE_COUNT] = {
    { FX32_CONST(-4.4), FX32_CONST(28.3), FX32_CONST(13.6) },
    { 0, FX32_CONST(28.3), FX32_CONST(13.6) },
    { FX32_CONST(4.4), FX32_CONST(28.3), FX32_CONST(13.6) },
};

static const PselSeqFunc sPselSeqFuncs[] = {
    PselSeq_Start,  PselSeq_Init,   PselSeq_Load, PselSeq_FadeIn, PselSeq_Main,
    PselSeq_FadeOut, PselSeq_Unload, PselSeq_Free, PselSeq_End,
};

static const PselPokeMove sPselPokeMoves[PSEL_POKE_COUNT] = {
    { { { 53, 73 }, { 90, 83 }, { 128, 94 } }, { 8, 12, 16 }, 8, { 4, 4 }, { 32, 32 } },
    { { { 128, 68 }, { 128, 81 }, { 128, 94 } }, { 8, 12, 16 }, 8, { 4, 4 }, { 32, 32 } },
    { { { 202, 73 }, { 165, 83 }, { 128, 94 } }, { 8, 12, 16 }, 8, { 4, 4 }, { 32, 32 } },
};

const GameProcFunctions PselProcFunctions = { PselProc_Init, PselProc_Main, PselProc_Exit };

// The background's palette of a choice
#define PSEL_BG_PAL_OF(choice) ((choice) == PSEL_CHOICE_NONE ? 3 : (choice))

// The camera curve advances half a frame for each frame of the game
static void PselCurve_SetFrame(G3DCurve *curve, s32 frame) {
    GFL_G3DCurveFrameSet(curve, frame / 2);
}

static PselObj *PselObj_AllocAll(HeapID heapId, u16 count) {
    PselObj *objs = GFL_HeapAllocate(heapId, sizeof(PselObj) * count, TRUE, "psel.c", 620);
    u16 i;

    for (i = 0; i < count; i++) {
        VEC_Set(&objs[i].status.translation, 0, 0, 0);
        VEC_Set(&objs[i].status.scale, FX32_ONE, FX32_ONE, FX32_ONE);
        MAT3_Identity(&objs[i].status.rotation);
    }
    return objs;
}

static void PselObj_FreeAll(PselObj *objs) {
    GFL_HeapFree(objs);
}

static BOOL PselProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    PselWork *wk;

    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 16, 0);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_PSEL, 0x100000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PselWork), HEAPID_PSEL);
    sys_memset(wk, 0, sizeof(PselWork));
    wk->heapId = HEAPID_PSEL;
    wk->param = param;
    wk->fadeFreq = GFL_FadeGetUpdateFreq();
    GFL_FadeSetUpdateFreq(1);
    wk->touchMode = func_0203d554();
    wk->choice = PSEL_CHOICE_NONE;
    wk->ball = PSEL_CHOICE_NONE;
    wk->pokeArrived = FALSE;
    wk->step = 0;
    return TRUE;
}

static BOOL PselProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    PselWork *wk = work;

    GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 16, 16, 0);
    func_0203d564(wk->touchMode);
    GFL_FadeSetUpdateFreq(wk->fadeFreq);
    switch (wk->choice) {
    case 0:
        *wk->param->result = 0;
        break;
    case 1:
        *wk->param->result = 1;
        break;
    case 2:
        *wk->param->result = 2;
        break;
    default:
        *wk->param->result = 0;
        break;
    }
    wk->param->choice = wk->choice;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_PSEL);
    return TRUE;
}

static BOOL PselProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    PselWork *wk = work;
    s32 *seq = (s32 *)state;
    s32 prev = *seq;

    *seq = sPselSeqFuncs[prev](wk, &wk->step);
    if (*seq != PSEL_SEQ_END) {
        if (*seq != prev) {
            wk->step = 0;
        }
    } else {
        return TRUE;
    }
    if (*seq >= PSEL_SEQ_LOAD && *seq <= PSEL_SEQ_UNLOAD) {
        func_02021a3c(wk->printQueue);
        PselText_Update(wk);
        PselGraphic_Update(wk->graphic);
        PselGraphic_Begin3D(wk->graphic);
        Psel3D_Draw(wk);
        PselGraphic_End3D(wk->graphic);
    }
    return FALSE;
}

static void Psel_VBlank(TCB *tcb, void *data) {
    PselWork *wk = data;

    if (wk->bgRequest & PSEL_BG_REQ_MAIN_ON) {
        GFL_BGSysSetBGEnabled(2, TRUE);
        GFL_BGSysSetBGEnabled(1, TRUE);
    }
    if (wk->bgRequest & PSEL_BG_REQ_MAIN_OFF) {
        GFL_BGSysSetBGEnabled(2, FALSE);
        GFL_BGSysSetBGEnabled(1, FALSE);
    }
    if (wk->bgRequest & PSEL_BG_REQ_SUB_ON) {
        GFL_BGSysSetBGEnabled(7, TRUE);
        GFL_BGSysSetBGEnabled(6, TRUE);
        GFL_BGSysSetBGEnabled(5, TRUE);
    }
    if (wk->bgRequest & PSEL_BG_REQ_SUB_OFF) {
        GFL_BGSysSetBGEnabled(7, FALSE);
        GFL_BGSysSetBGEnabled(6, FALSE);
        GFL_BGSysSetBGEnabled(5, FALSE);
    }
    if (wk->bgRequest & PSEL_BG_REQ_MSG_ON) {
        GFL_BGSysSetBGEnabled(2, TRUE);
    }
    if (wk->bgRequest & PSEL_BG_REQ_MSG_OFF) {
        GFL_BGSysSetBGEnabled(2, FALSE);
    }
    wk->bgRequest = 0;
}

static void Psel3D_Draw(PselWork *wk) {
    u16 i;

    for (i = 0; i < wk->objCount; i++) {
        PselObj *obj = &wk->objs[i];

        if (obj->draw) {
            GFL_G3DSysDrawObj(GFL_G3DMgrGetActor(wk->g3dMgr, obj->actor), &obj->status);
        }
    }
}

static void Psel3D_Init(PselWork *wk) {
    wk->g3dMgr = GFL_G3DMgrCreate(NELEMS(sPselSceneResources), PSEL_OBJ_COUNT, wk->heapId);
    wk->objCount = 0;
}

static void Psel3D_Free(PselWork *wk) {
    GFL_G3DMgrFree(wk->g3dMgr);
}

static void Psel3D_Load(PselWork *wk) {
    u16 first;
    u16 idx;
    u16 i;
    G3DActor *actor;
    u16 count;
    G3DCamera *camera;

    wk->scene = GFL_G3DMgrNewScene(wk->g3dMgr, &sPselScene);
    wk->objCount = PSEL_OBJ_COUNT;
    wk->objs = PselObj_AllocAll(wk->heapId, wk->objCount);
    idx = 0;
    first = GFL_G3DMgrGetSceneFirstActorIdx(wk->g3dMgr, wk->scene);
    for (i = 0; i < PSEL_OBJ_COUNT; i++) {
        PselObj *obj;

        wk->objIdx[i] = idx;
        obj = &wk->objs[idx];
        obj->actor = first + i;
        obj->draw = TRUE;
        // The balls are hidden until the box opens. The casts give the original's signed compares
        if ((s32)i >= PSEL_OBJ_BALL_1 && (s32)i <= PSEL_OBJ_BALL_3) {
            obj->status.translation = sPselBallPositions[i - PSEL_OBJ_BALL_1];
            obj->draw = FALSE;
        }
        idx++;
    }
    actor = GFL_G3DMgrGetActor(wk->g3dMgr, wk->objs[wk->objIdx[PSEL_OBJ_BOX]].actor);
    count = GFL_G3DActorGetAnmCount(actor);
    for (i = 0; i < count; i++) {
        GFL_G3DActorBindAnm(actor, i);
    }
    wk->cameraCurve = GFL_G3DCurveLoadFileStream(wk->heapId, ARCID_PSEL, PSEL_ARC_CAMERA_CURVE, 10);
    GFL_G3DCurveApplyCamera(PselGraphic_GetCamera(wk->graphic), wk->cameraCurve);
    // The cosine and sine of 4 degrees, half the field of view
    camera = PselGraphic_GetCamera(wk->graphic);
    GFL_G3DCameraPerspectiveSetFOVCos(camera, FX_SinIdx(DEG_TO_IDX(86)));
    GFL_G3DCameraPerspectiveSetFOVSin(camera, FX_CosIdx(DEG_TO_IDX(86)));
    wk->introFrame = 0;
}

static void Psel3D_Unload(PselWork *wk) {
    GFL_G3DMgrDeleteScene(wk->g3dMgr, wk->scene);
    PselObj_FreeAll(wk->objs);
    wk->objCount = 0;
    GFL_G3DCurveFree(wk->cameraCurve);
}

// Plays the box's animations and moves the camera along its curve
static void Psel3D_UpdateIntro(PselWork *wk) {
    G3DActor *actor = GFL_G3DMgrGetActor(wk->g3dMgr, wk->objs[wk->objIdx[PSEL_OBJ_BOX]].actor);
    u16 count = GFL_G3DActorGetAnmCount(actor);
    u16 i;
    // The ROM keeps this constant in .rodata though the code has its value, as a function's static does; which function
    // had it is a guess
    static const fx32 speed = PSEL_ANIME_SPEED;

    for (i = 0; i < count; i++) {
        GFL_G3DActorStepAnmFrameLoop(actor, i, speed);
    }
    GFL_G3DCurveFrameStepLoop(wk->cameraCurve, speed);
    GFL_G3DCurveApplyCamera(PselGraphic_GetCamera(wk->graphic), wk->cameraCurve);
    wk->introFrame++;
}

// Puts the camera at the start of its zoom on a ball, or at its end to play it backwards
static void PselCamera_StartZoom(PselWork *wk, s32 ball, BOOL reverse) {
    u16 frame = sPselCameraFrames[ball].zoom[reverse ? 1 : 0];

    PselCurve_SetFrame(wk->cameraCurve, FX32_CONST(frame));
    GFL_G3DCurveApplyCamera(PselGraphic_GetCamera(wk->graphic), wk->cameraCurve);
    wk->cameraBall = ball;
    wk->cameraReverse = reverse;
    wk->cameraFrame = frame;
}

static BOOL PselCamera_IsZoomDone(PselWork *wk) {
    u8 end = 0;

    if (!wk->cameraReverse) {
        end = 1;
    }
    if (wk->cameraFrame == sPselCameraFrames[wk->cameraBall].zoom[end]) {
        return TRUE;
    }
    return FALSE;
}

static void PselCamera_UpdateZoom(PselWork *wk) {
    u8 endIdx = 0;
    u16 end;

    if (!wk->cameraReverse) {
        endIdx = 1;
    }
    end = sPselCameraFrames[wk->cameraBall].zoom[endIdx];
    if (wk->cameraFrame != end) {
        GFL_G3DCurveFrameStepLoop(wk->cameraCurve, wk->cameraReverse ? -PSEL_ANIME_SPEED : PSEL_ANIME_SPEED);
        GFL_G3DCurveApplyCamera(PselGraphic_GetCamera(wk->graphic), wk->cameraCurve);
        if (wk->cameraReverse) {
            wk->cameraFrame--;
        } else {
            wk->cameraFrame++;
        }
        // Back at the view of the three balls, the end of the intro
        if (wk->cameraFrame == end && wk->cameraReverse) {
            PselCurve_SetFrame(wk->cameraCurve, PSEL_INTRO_FRAMES * FX32_ONE);
            GFL_G3DCurveApplyCamera(PselGraphic_GetCamera(wk->graphic), wk->cameraCurve);
        }
    }
}

static void PselCamera_StartDecide(PselWork *wk, s32 ball) {
    u16 frame = sPselCameraFrames[ball].decideStart;

    PselCurve_SetFrame(wk->cameraCurve, FX32_CONST(frame));
    GFL_G3DCurveApplyCamera(PselGraphic_GetCamera(wk->graphic), wk->cameraCurve);
    wk->cameraBall = ball;
    wk->cameraReverse = FALSE;
    wk->cameraFrame = frame;
}

static void PselCamera_UpdateDecide(PselWork *wk) {
    if (wk->cameraFrame != sPselCameraFrames[wk->cameraBall].decideEnd) {
        GFL_G3DCurveFrameStepLoop(wk->cameraCurve, PSEL_ANIME_SPEED);
        GFL_G3DCurveApplyCamera(PselGraphic_GetCamera(wk->graphic), wk->cameraCurve);
        wk->cameraFrame++;
    }
}

// Stops the animations of the ball that was picked and starts those of the new one
static void PselBall_Select(PselWork *wk, s32 ball) {
    if (wk->ball != PSEL_CHOICE_NONE && wk->ball != ball) {
        G3DActor *actor = GFL_G3DMgrGetActor(wk->g3dMgr, wk->objs[wk->objIdx[wk->ball + 1]].actor);
        u16 count = GFL_G3DActorGetAnmCount(actor);
        u16 i;

        for (i = 0; i < count; i++) {
            GFL_G3DActorUnbindAnm(actor, i);
        }
    }
    if (ball != PSEL_CHOICE_NONE && wk->ball != ball) {
        G3DActor *actor = GFL_G3DMgrGetActor(wk->g3dMgr, wk->objs[wk->objIdx[ball + 1]].actor);
        u16 count = GFL_G3DActorGetAnmCount(actor);
        u16 i;
        fx32 frame = 0;

        for (i = 0; i < count; i++) {
            GFL_G3DActorBindAnm(actor, i);
            GFL_G3DActorSetAnmFrame(actor, i, &frame);
        }
        func_0204c504(wk->cursor, 0);
    }
    wk->ball = ball;
}

static void PselBall_Update(PselWork *wk) {
    if (wk->ball != PSEL_CHOICE_NONE) {
        G3DActor *actor = GFL_G3DMgrGetActor(wk->g3dMgr, wk->objs[wk->objIdx[wk->ball + 1]].actor);
        u16 count = GFL_G3DActorGetAnmCount(actor);
        u16 i;

        for (i = 0; i < count; i++) {
            GFL_G3DActorStepAnmFrameLoop(actor, i, PSEL_ANIME_SPEED);
        }
    }
}

static void PselPoke_Load(PselWork *wk) {
    ArcTool *pokeArc;
    ArcTool *arc;
    u8 i;

    pokeArc = MakePokeGraArcHandle(wk->heapId);
    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        u32 species = sPselSpecies[i];

        wk->poke[i].res[0].chars = PokeGra_LoadClActChars(pokeArc, species, 0, 0, FALSE, 0, FALSE, 0, 1, wk->heapId);
        wk->poke[i].res[0].pltt =
            PokeGra_LoadClActPalette(pokeArc, species, 0, 0, FALSE, 0, FALSE, 1, sPselPokePaletteSlots[i] * 32, wk->heapId);
        wk->poke[i].res[0].cellAnim = PokeGra_LoadClActCellAnims(species, 0, 0, FALSE, 0, FALSE, 0, 1, wk->heapId);
    }
    GFL_ArcToolFree(pokeArc);

    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        ClActorSetup setup;

        sys_memset(&setup, 0, sizeof(setup));
        setup.x = sPselPokeMoves[i].pos[0][0];
        setup.y = sPselPokeMoves[i].pos[0][1];
        setup.sequence = 0;
        setup.priority = 2;
        setup.bgPriority = 3;
        wk->poke[i].clact[0] =
            func_0204c040(PselGraphic_GetClActUnit(wk->graphic), wk->poke[i].res[0].chars, wk->poke[i].res[0].pltt,
                          wk->poke[i].res[0].cellAnim, &setup, 1, wk->heapId);
        func_0204c124(wk->poke[i].clact[0], FALSE);
    }

    arc = GFL_ArcSysCreateFileHandle(ARCID_PSEL, wk->heapId);
    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        wk->poke[i].res[1].chars = func_0204b81c(arc, sPselPokeFiles[i].chars, FALSE, 1, wk->heapId);
        wk->poke[i].res[1].cellAnim = func_0204bde0(arc, sPselPokeFiles[i].cell, sPselPokeFiles[i].anim, wk->heapId);
    }
    GFL_ArcToolFree(arc);

    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        ClActorSetupEx setup;

        sys_memset(&setup, 0, sizeof(setup));
        setup.base.x = sPselPokeMoves[i].pos[0][0];
        setup.base.y = sPselPokeMoves[i].pos[0][1];
        setup.base.sequence = 0;
        setup.base.priority = 2;
        setup.base.bgPriority = 3;
        setup.affineCenter.x = 0;
        setup.affineCenter.y = 0;
        setup.scaleX = FX32_CONST(sPselPokeMoves[i].scale[0] / 16.0f);
        setup.scaleY = FX32_CONST(sPselPokeMoves[i].scale[0] / 16.0f);
        setup.rotation = 0;
        setup.affineMode = 1;
        wk->poke[i].clact[1] =
            func_0204c0a4(PselGraphic_GetClActUnit(wk->graphic), wk->poke[i].res[1].chars, wk->poke[i].res[0].pltt,
                          wk->poke[i].res[1].cellAnim, &setup, 1, wk->heapId);
        func_0204c124(wk->poke[i].clact[1], FALSE);
    }

    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        wk->poke[i].frame = 0;
        wk->poke[i].state = PSEL_POKE_BACK;
        wk->poke[i].request = PSEL_POKE_REQ_NONE;
        wk->poke[i].unmoved = FALSE;
    }
    wk->fadeCount = 0;
    wk->fadeState = 0;
}

static void PselPoke_Unload(PselWork *wk) {
    u8 i;
    u8 j;

    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        for (j = 0; j < 2; j++) {
            func_0204c108(wk->poke[i].clact[j]);
            func_0204be64(wk->poke[i].res[j].cellAnim);
            func_0204b98c(wk->poke[i].res[j].chars);
            // The scaled picture uses the still one's palette
            if (j != 1) {
                func_0204bcd0(wk->poke[i].res[j].pltt);
            }
        }
    }
}

// A frame of a jump from one place and scale to another, along a parabola. Returns the y without the jump's height,
// by which the pictures are sorted
static s16 PselPoke_CalcJump(ClActorPos *pos, ClActorScale *scale, u8 x0, u8 y0, u8 x1, u8 y1, u8 scale0, u8 scale1,
                             u32 duration, u8 height, u32 frame) {
    f32 x = (f32)x0 + (f32)frame * ((f32)x1 - (f32)x0) / (f32)duration;
    f32 y = (f32)y0 + (f32)frame * ((f32)y1 - (f32)y0) / (f32)duration;
    f32 jump = (f32)height + -(f32)height * ((f32)frame - (f32)duration / 2.0f) *
                                 ((f32)frame - (f32)duration / 2.0f) * 4.0f / (f32)duration / (f32)duration;
    f32 size = (f32)scale0 + (f32)frame * ((f32)scale1 - (f32)scale0) / (f32)duration;
    s16 posX = x;
    s16 posY = y - jump;
    fx32 fxSize = FX32_CONST(size / 16.0f);

    pos->x = posX;
    pos->y = posY;
    scale->x = scale->y = fxSize;
    return (s32)y;
}

static void PselPoke_Update(PselWork *wk) {
    BOOL jumped[PSEL_POKE_COUNT] = { FALSE, FALSE, FALSE };
    s16 baseY[PSEL_POKE_COUNT];
    u8 i;
    u8 j;

    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        PselPoke *poke = &wk->poke[i];
        const PselPokeMove *move = &sPselPokeMoves[i];
        s32 dir = 0;

        if (poke->request != PSEL_POKE_REQ_NONE) {
            switch (poke->request) {
            case PSEL_POKE_REQ_NONE:
                break;
            case PSEL_POKE_REQ_FRONT:
                dir = 1;
                break;
            case PSEL_POKE_REQ_BACK:
                dir = -1;
                break;
            }
            if (dir < 0) {
                poke->frame = 0;
                poke->state = PSEL_POKE_JUMP_1;
            }
            switch (poke->state) {
            case PSEL_POKE_BACK:
                if (dir > 0) {
                    func_0204c124(poke->clact[0], FALSE);
                    func_0204c124(poke->clact[1], TRUE);
                    poke->frame = 0;
                    poke->state = PSEL_POKE_JUMP_1;
                    poke->unmoved = FALSE;
                } else {
                    if (poke->request == PSEL_POKE_REQ_BACK) {
                        poke->request = PSEL_POKE_REQ_NONE;
                    }
                    break;
                }
                // fallthrough
            case PSEL_POKE_JUMP_1:
                if (poke->frame == 0) {
                    ClActorPos pos;
                    ClActorScale scale;

                    pos.x = move->pos[0][0];
                    pos.y = move->pos[0][1];
                    scale.x = scale.y = FX32_CONST(move->scale[0] / 16.0f);
                    func_0204c140(poke->clact[1], &pos, 1);
                    func_0204c270(poke->clact[1], &scale);
                } else {
                    ClActorPos pos;
                    ClActorScale scale;

                    baseY[i] = PselPoke_CalcJump(&pos, &scale, move->pos[0][0], move->pos[0][1], move->pos[1][0],
                                                 move->pos[1][1], move->scale[0], move->scale[1], move->duration[0],
                                                 move->height[0], poke->frame);
                    jumped[i] = TRUE;
                    func_0204c140(poke->clact[1], &pos, 1);
                    func_0204c270(poke->clact[1], &scale);
                }
                if (poke->frame == 0 && dir < 0) {
                    func_0204c124(poke->clact[0], TRUE);
                    func_0204c124(poke->clact[1], FALSE);
                    poke->frame = 0;
                    poke->state = PSEL_POKE_BACK;
                    poke->unmoved = FALSE;
                } else {
                    poke->frame = poke->frame + dir;
                    poke->unmoved = FALSE;
                    if (poke->frame == move->duration[0]) {
                        poke->frame = 0;
                        poke->state = PSEL_POKE_MIDDLE;
                    }
                }
                break;
            case PSEL_POKE_MIDDLE: {
                ClActorPos pos;
                ClActorScale scale;

                pos.x = move->pos[1][0];
                pos.y = move->pos[1][1];
                scale.x = scale.y = FX32_CONST(move->scale[1] / 16.0f);
                func_0204c140(poke->clact[1], &pos, 1);
                func_0204c270(poke->clact[1], &scale);
                if (poke->frame == 0 && dir < 0) {
                    poke->frame = move->duration[0] - 1;
                    poke->state = PSEL_POKE_JUMP_1;
                    poke->unmoved = FALSE;
                } else {
                    poke->frame = poke->frame + dir;
                    poke->unmoved = FALSE;
                    if (poke->frame == move->wait) {
                        poke->frame = 0;
                        poke->state = PSEL_POKE_JUMP_2;
                    }
                }
                break;
            }
            case PSEL_POKE_JUMP_2:
                if (poke->frame == 0) {
                    ClActorPos pos;
                    ClActorScale scale;

                    pos.x = move->pos[1][0];
                    pos.y = move->pos[1][1];
                    scale.x = scale.y = FX32_CONST(move->scale[1] / 16.0f);
                    func_0204c140(poke->clact[1], &pos, 1);
                    func_0204c270(poke->clact[1], &scale);
                } else {
                    ClActorPos pos;
                    ClActorScale scale;

                    baseY[i] = PselPoke_CalcJump(&pos, &scale, move->pos[1][0], move->pos[1][1], move->pos[2][0],
                                                 move->pos[2][1], move->scale[1], move->scale[2], move->duration[1],
                                                 move->height[1], poke->frame);
                    jumped[i] = TRUE;
                    func_0204c140(poke->clact[1], &pos, 1);
                    func_0204c270(poke->clact[1], &scale);
                }
                if (poke->frame == 0 && dir < 0) {
                    poke->frame = move->wait - 1;
                    poke->state = PSEL_POKE_MIDDLE;
                    poke->unmoved = FALSE;
                } else {
                    poke->frame = poke->frame + dir;
                    poke->unmoved = FALSE;
                    if (poke->frame == move->duration[1]) {
                        poke->frame = 0;
                        poke->state = PSEL_POKE_FRONT;
                    }
                }
                break;
            case PSEL_POKE_FRONT: {
                ClActorPos pos;
                ClActorScale scale;

                pos.x = move->pos[2][0];
                pos.y = move->pos[2][1];
                scale.x = scale.y = FX32_CONST(move->scale[2] / 16.0f);
                func_0204c140(poke->clact[1], &pos, 1);
                func_0204c270(poke->clact[1], &scale);
                if (dir < 0) {
                    func_0204c468(poke->clact[1], 3);
                    poke->frame = move->duration[1] - 1;
                    poke->state = PSEL_POKE_JUMP_2;
                    poke->unmoved = FALSE;
                } else if (poke->request == PSEL_POKE_REQ_FRONT) {
                    func_0204c468(poke->clact[1], 2);
                    poke->request = PSEL_POKE_REQ_NONE;
                    if (!poke->unmoved) {
                        wk->pokeArrived = TRUE;
                    }
                }
                break;
            }
            }
        }
    }

    // The lower a picture is, the nearer it is drawn
    {
        u8 order[PSEL_POKE_COUNT] = { 0, 1, 2 };
        s16 y[PSEL_POKE_COUNT];
        ClActorPos pos;

        for (i = 0; i < PSEL_POKE_COUNT; i++) {
            if (jumped[i]) {
                y[i] = baseY[i];
            } else {
                func_0204c178(wk->poke[i].clact[1], &pos, 1);
                y[i] = pos.y;
            }
        }
        for (i = 0; i < PSEL_POKE_COUNT; i++) {
            for (j = PSEL_POKE_COUNT - 1; j > i; j--) {
                if (y[order[j]] > y[order[j - 1]]) {
                    u8 tmp = order[j - 1];

                    order[j - 1] = order[j];
                    order[j] = tmp;
                }
            }
        }
        for (i = 0; i < PSEL_POKE_COUNT; i++) {
            u8 priority;

            if (order[0] == i) {
                priority = 0;
            } else if (order[1] == i) {
                priority = 1;
            } else {
                priority = 2;
            }
            func_0204c438(wk->poke[i].clact[1], priority);
        }
    }
}

// Brings a ball's Pokémon to the front and sends the others back
static void PselPoke_Select(PselWork *wk, s32 ball) {
    u8 i;

    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        if (i == ball) {
            PselPokePal_Request(wk, i, PSEL_PAL_NORMAL);
        } else {
            PselPokePal_Request(wk, i, PSEL_PAL_DARK);
        }
        if (i == ball) {
            wk->poke[i].request = PSEL_POKE_REQ_FRONT;
            wk->poke[i].unmoved = TRUE;
        } else {
            wk->poke[i].request = PSEL_POKE_REQ_BACK;
            wk->poke[i].unmoved = TRUE;
        }
    }
}

static void PselPoke_StartFadeIn(PselWork *wk) {
    u8 i;

    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        func_0204c318(wk->poke[i].clact[0], GX_OAM_MODE_XLU);
    }
    wk->fadeCount = 0;
    wk->fadeState = 1;
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, 0, 0x3f, wk->fadeCount, 16 - wk->fadeCount);
}

static BOOL PselPoke_IsFadeInDone(PselWork *wk) {
    if (wk->fadeState == 0) {
        return TRUE;
    }
    return FALSE;
}

static void PselPoke_UpdateFadeIn(PselWork *wk) {
    u8 i;

    switch (wk->fadeState) {
    case 1:
        for (i = 0; i < PSEL_POKE_COUNT; i++) {
            func_0204c124(wk->poke[i].clact[0], TRUE);
        }
        wk->fadeState = 2;
        break;
    case 2:
        if (wk->fadeCount < 16) {
            wk->fadeCount++;
            if (wk->fadeCount == 16) {
                for (i = 0; i < PSEL_POKE_COUNT; i++) {
                    func_0204c318(wk->poke[i].clact[0], GX_OAM_MODE_NORMAL);
                }
                wk->fadeState = 3;
            }
            gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, 0, 0x3f, wk->fadeCount, 16 - wk->fadeCount);
        } else {
            wk->fadeState = 3;
        }
        break;
    case 3:
        gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG2, 0x3f, 8, 8);
        wk->fadeState = 0;
        break;
    }
}

static void PselCursor_Load(PselWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PSEL, wk->heapId);
    ClActorSetup setup;

    wk->cursorPltt = func_0204bbb8(arc, PSEL_ARC_CURSOR_NCLR, 0, 0, 0, 1, wk->heapId);
    wk->cursorChars = func_0204b81c(arc, PSEL_ARC_CURSOR_NCGR, FALSE, 0, wk->heapId);
    wk->cursorCellAnim = func_0204bde0(arc, PSEL_ARC_CURSOR_NCER, PSEL_ARC_CURSOR_NANR, wk->heapId);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(setup));
    setup.x = 42;
    setup.y = 29;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 2;
    wk->cursor = func_0204c040(PselGraphic_GetClActUnit(wk->graphic), wk->cursorChars, wk->cursorPltt,
                               wk->cursorCellAnim, &setup, 0, wk->heapId);
    func_0204c520(wk->cursor, TRUE);
    PselCursor_Show(wk, FALSE);
}

static void PselCursor_Unload(PselWork *wk) {
    func_0204c108(wk->cursor);
    func_0204be64(wk->cursorCellAnim);
    func_0204b98c(wk->cursorChars);
    func_0204bcd0(wk->cursorPltt);
}

// Puts the cursor on the picked ball, or off the screen
static void PselCursor_Show(PselWork *wk, BOOL show) {
    if (show) {
        PselCursor_Move(wk);
    } else {
        ClActorPos pos;

        pos.x = 400;
        pos.y = 300;
        func_0204c140(wk->cursor, &pos, 0);
    }
}

static void PselCursor_Move(PselWork *wk) {
    ClActorPos pos;

    pos.x = sPselCursorPositions[wk->choice].x;
    pos.y = sPselCursorPositions[wk->choice].y;
    func_0204c140(wk->cursor, &pos, 0);
}

// The palette and the two characters of the black bands on BG1 and on the sub screen's BG2
static void PselBand_Init(PselWork *wk) {
    u16 *palette = GFL_HeapAllocate(wk->heapId, 0x20, TRUE, "psel.c", 2178);
    GFLBitmap *bitmap;

    palette[0] = GX_RGB(0, 0, 0);
    palette[1] = GX_RGB(0, 0, 0);
    palette[2] = GX_RGB(31, 31, 31);
    GFL_BGSysUploadStdPalette(1, palette, 0x20, 13 * 0x20);
    GFL_BGSysUploadStdPalette(6, palette, 0x20, 0);
    GFL_HeapFree(palette);

    bitmap = GFL_BitmapCreate(2, 1, 0x20, wk->heapId);
    GFL_BitmapFillArea(bitmap, 0, 0, 8, 8, 0);
    GFL_BitmapFillArea(bitmap, 8, 0, 8, 8, 1);
    GFL_BGSysLoadChar(1, GFL_BitmapGetPixelData(bitmap), GFL_BitmapCalcPixelDataSize(bitmap), 0);
    GFL_BGSysLoadChar(6, GFL_BitmapGetPixelData(bitmap), GFL_BitmapCalcPixelDataSize(bitmap), 0);
    GFL_BitmapFree(bitmap);
}

static void PselBand_Free(PselWork *wk) {
}

// A black band over rows 17 to 22 of both screens, behind the text
static void PselBand_LoadScreens(PselWork *wk) {
    {
        u16 *screen = GFL_HeapAllocate(wk->heapId, 32 * 24 * sizeof(u16), TRUE, "psel.c", 2217);
        u8 x;
        u8 y;
        u16 tile = 0;

        for (y = 0; y < 24; y++) {
            for (x = 0; x < 32; x++) {
                u16 character = (y >= 17 && y < 23) ? 1 : 0;

                screen[tile] = character | (13 << 12);
                tile++;
            }
        }
        GFL_BGSysLoadScrAreaAll(1, screen, 0, 0, 32, 24);
        GFL_BGSysQueueScrLoad(1);
        GFL_HeapFree(screen);
    }
    {
        u16 *screen = GFL_HeapAllocate(wk->heapId, 32 * 24 * sizeof(u16), TRUE, "psel.c", 2242);
        u8 x;
        u8 y;
        u16 tile = 0;

        for (y = 0; y < 24; y++) {
            for (x = 0; x < 32; x++) {
                u16 character = (y >= 17 && y < 23) ? 1 : 0;

                screen[tile] = character;
                tile++;
            }
        }
        GFL_BGSysLoadScrAreaAll(6, screen, 0, 0, 32, 24);
        GFL_BGSysQueueScrLoad(6);
        GFL_HeapFree(screen);
    }
}

static void PselBand_ClearScreens(PselWork *wk) {
    GFL_BGSysFillScrAsync(1, 13 << 12);
    GFL_BGSysFillScrAsync(6, 0);
}

static void PselSubBG_Load(PselWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PSEL, wk->heapId);

    GFL_G2DIOLoadArcNCLR(arc, PSEL_ARC_SUB_BG_NCLR, 4, 0x100, 0x100, 0x100, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, PSEL_ARC_SUB_BG_NCGR, 4, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, PSEL_ARC_SUB_BG_BACK_NSCR, 4, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, PSEL_ARC_SUB_BG_FRONT_NSCR, 5, 0, 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(4);
    GFL_BGSysLoadScr(5);
}

static void PselSubBG_Unload(PselWork *wk) {
}

static void PselText_Init(PselWork *wk) {
    u8 i;

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 14 * 0x20, 0x20, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 1 * 0x20, 0x20, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_ASPERTIA_CITY, wk->heapId);
    wk->tcbMgr = GFL_TCBExMgrCreate(wk->heapId, wk->heapId, 1, 0);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    for (i = 0; i < 3; i++) {
        wk->win[i] = BmpWin_CreateDynamic(sPselWinSetups[i].bg, sPselWinSetups[i].x, sPselWinSetups[i].y,
                                          sPselWinSetups[i].width, sPselWinSetups[i].height,
                                          sPselWinSetups[i].palette, sPselWinSetups[i].fromEnd);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->win[i]), 0);
        BmpWin_FlushChar(wk->win[i]);
        wk->winDone[i] = TRUE;
    }
    wk->stream = NULL;
    wk->strbuf = NULL;
}

static void PselText_Free(PselWork *wk) {
    u8 i;

    if (wk->stream != NULL) {
        func_020223cc(wk->stream);
    }
    if (wk->strbuf != NULL) {
        GFL_StrBufFree(wk->strbuf);
    }
    for (i = 0; i < 3; i++) {
        BmpWin_Free(wk->win[i]);
    }
    GFL_TCBExMgrFree(wk->tcbMgr);
    GFL_MsgDataFree(wk->msgData);
}

static void PselText_Update(PselWork *wk) {
    GFL_TCBExMgrUpdate(wk->tcbMgr);
    PselText_Flush(wk);
}

// Shows the windows whose text the print queue has finished. Returns whether none is left
static BOOL PselText_Flush(PselWork *wk) {
    BOOL done = TRUE;
    u8 i;

    for (i = 0; i < 3; i++) {
        if (!wk->winDone[i]) {
            if (func_02021c1c(wk->printQueue, BmpWin_GetBitmap(wk->win[i]))) {
                done = FALSE;
            } else {
                BmpWin_Transfer(wk->win[i]);
                wk->winDone[i] = TRUE;
            }
        }
    }
    return done;
}

// Starts a message in the message window, centered
static void PselText_StartStream(PselWork *wk, u32 messageId) {
    BmpWin *win = wk->win[1];
    s16 x = 0;
    s16 y = 0;
    u16 width;
    u16 height;
    u16 winWidth;
    u16 winHeight;

    GFL_BitmapFill(BmpWin_GetBitmap(win), 0);
    if (wk->stream != NULL) {
        func_020223cc(wk->stream);
    }
    if (wk->strbuf != NULL) {
        GFL_StrBufFree(wk->strbuf);
    }
    wk->strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, messageId);
    width = GFL_FontGetBlockWidth(wk->strbuf, wk->font, 0);
    height = GFL_FontGetBlockHeight(wk->strbuf, wk->font);
    winWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(win));
    winHeight = GFL_BitmapGetHeight(BmpWin_GetBitmap(win));
    if (winWidth > width) {
        x = (winWidth - width) / 2;
    }
    if (winHeight > height) {
        y = (winHeight - height) / 2;
    }
    wk->stream = func_02022268(win, x, y, wk->strbuf, wk->font, func_02017bcc(), wk->tcbMgr, 2, wk->heapId, 0);
    BmpWin_Transfer(win);
}

// Steps the message, which A, B or a touch speeds up and continues. Returns whether it has ended
static BOOL PselText_UpdateStream(PselWork *wk) {
    BOOL done = FALSE;

    switch (func_020223b4(wk->stream)) {
    case 0:
        if ((GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da2c()) {
            func_020223e0(wk->stream, 0);
            if (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                wk->touchMode = FALSE;
            } else {
                wk->touchMode = TRUE;
            }
        }
        break;
    case 1:
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
            func_020223bc(wk->stream);
            if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                wk->touchMode = FALSE;
            } else {
                wk->touchMode = TRUE;
            }
        }
        break;
    case 2:
        done = TRUE;
        break;
    }
    return done;
}

static void PselText_ClearStream(PselWork *wk) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->win[1]), 0);
    BmpWin_Transfer(wk->win[1]);
    if (wk->stream != NULL) {
        func_020223cc(wk->stream);
    }
    wk->stream = NULL;
    if (wk->strbuf != NULL) {
        GFL_StrBufFree(wk->strbuf);
    }
    wk->strbuf = NULL;
}

// Prints the type and name of a ball's Pokémon on the sub screen
static void PselText_PrintType(PselWork *wk, s32 ball) {
    BmpWin *win = wk->win[2];
    u16 y = 0;
    WordSet *wordSet;
    StrBuf *src;
    StrBuf *str;
    u16 height;
    u16 winHeight;

    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    src = GFL_MsgDataLoadStrbufNew(wk->msgData, sPselTypeMessages[ball]);
    str = GFL_StrBufCreate(64, wk->heapId);
    WordSet_LoadSpeciesName(wordSet, 1, sPselSpecies[ball]);
    GFL_WordSetFormatStrbuf(wordSet, str, src);
    GFL_StrBufFree(src);
    GFL_WordSetSystemFree(wordSet);
    GFL_BitmapFill(BmpWin_GetBitmap(win), 0);
    // The widths are read and not used: the text is centered vertically only
    GFL_FontGetBlockWidth(str, wk->font, 0);
    height = GFL_FontGetBlockHeight(str, wk->font);
    GFL_BitmapGetWidth(BmpWin_GetBitmap(win));
    winHeight = GFL_BitmapGetHeight(BmpWin_GetBitmap(win));
    if (winHeight > height) {
        y = (winHeight - height) / 2;
    }
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(win), 0, y, str, wk->font, PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(str);
    wk->winDone[2] = FALSE;
    PselText_Flush(wk);
}

static void PselText_ClearType(PselWork *wk) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->win[2]), 0);
    BmpWin_Transfer(wk->win[2]);
}

static void PselMenu_InitRes(PselWork *wk) {
    wk->menuRes = AppTaskMenuRes_Create(2, 11, wk->font, wk->printQueue, wk->heapId);
}

static void PselMenu_FreeRes(PselWork *wk) {
    AppTaskMenuRes_Free(wk->menuRes);
}

// The button that confirms a ball picked by touch
static void PselButton_Create(PselWork *wk) {
    AppTaskMenuItem item;

    item.str = GFL_MsgDataLoadStrbufNew(wk->msgData, AspertiaCity_Text_PickOne);
    item.color = PRINT_COLOR(14, 15, 3);
    item.type = 0;
    wk->button = AppTaskMenuWin_Create(wk->menuRes, &item, 9, 21, 14, wk->heapId);
    GFL_StrBufFree(item.str);
}

static void PselButton_Free(PselWork *wk) {
    AppTaskMenuWin_Free(wk->button);
}

static void PselButton_Update(PselWork *wk) {
    AppTaskMenuWin_Update(wk->button);
}

static void PselButton_Decide(PselWork *wk) {
    AppTaskMenuWin_SetActive(wk->button, TRUE);
    AppTaskMenuWin_SetFlashing(wk->button, TRUE);
}

static BOOL PselButton_IsFlashFinished(PselWork *wk) {
    return AppTaskMenuWin_IsFlashFinished(wk->button);
}

static void PselYesNo_Create(PselWork *wk) {
    AppTaskMenuInit init;
    AppTaskMenuItem items[2];
    u32 i;

    for (i = 0; i < NELEMS(items); i++) {
        items[i].str = GFL_MsgDataLoadStrbufNew(wk->msgData, AspertiaCity_Text_Yes + i);
        items[i].color = PRINT_COLOR(14, 15, 3);
        u32 type = 0;

        if (i != 0) {
            type = APP_TASKMENU_ITEM_RETURN;
        }
        items[i].type = type;
    }
    init.heapId = wk->heapId;
    init.itemCount = NELEMS(items);
    init.items = items;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 17;
    init.width = 9;
    init.height = 3;
    wk->yesNo = AppTaskMenu_Create(&init, wk->menuRes);
    for (i = 0; i < NELEMS(items); i++) {
        GFL_StrBufFree(items[i].str);
    }
}

static void PselYesNo_Free(PselWork *wk) {
    AppTaskMenu_Free(wk->yesNo);
}

static void PselYesNo_Update(PselWork *wk) {
    AppTaskMenu_Update(wk->yesNo);
}

static BOOL PselYesNo_IsFlashFinished(PselWork *wk) {
    return AppTaskMenu_IsFlashFinished(wk->yesNo);
}

static BOOL PselYesNo_IsYes(PselWork *wk) {
    if (AppTaskMenu_GetCursorPos(wk->yesNo) == 0) {
        return TRUE;
    }
    return FALSE;
}

static void Psel_SplitColor(u16 color, u16 *r, u16 *g, u16 *b) {
    *r = color & 0x1f;
    *g = (color & 0x3e0) >> 5;
    *b = (color & 0x7c00) >> 10;
}

// A color `step` 16ths of the way from one to another
#define PSEL_BLEND(from, to, step) (((from) * (16 - (step)) + (to) * (step)) / 16)

static void PselPokePal_Load(PselWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PSEL, wk->heapId);
    u8 i;

    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        NNSG2dPaletteData *palette;
        void *normal;
        void *dark;

        normal = GFL_G2DIOReadNCLRArc(arc, sPselPokeFiles[i].pltt, &palette, wk->heapId);
        sys_memcpy(palette->rawData, wk->poke[i].palNormal, sizeof(wk->poke[i].palNormal));
        GFL_HeapFree(normal);
        dark = GFL_G2DIOReadNCLRArc(arc, sPselPokeFiles[i].plttDark, &palette, wk->heapId);
        sys_memcpy(palette->rawData, wk->poke[i].palDark, sizeof(wk->poke[i].palDark));
        GFL_HeapFree(dark);
    }
    GFL_ArcToolFree(arc);

    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        wk->poke[i].palState = PSEL_PAL_DARK;
        wk->poke[i].palStep = 16;
    }
    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        sys_memcpy(wk->poke[i].palDark, wk->poke[i].palNow, sizeof(wk->poke[i].palNow));
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_SUB, sPselPokePaletteSlots[i] * 0x20,
                                           wk->poke[i].palNow, sizeof(wk->poke[i].palNow));
    }
}

static void PselPokePal_Free(PselWork *wk) {
}

static void PselPokePal_Update(PselWork *wk) {
    u8 i;
    u8 j;

    for (i = 0; i < PSEL_POKE_COUNT; i++) {
        BOOL changed = FALSE;

        switch (wk->poke[i].palState) {
        case PSEL_PAL_TO_DARK:
            if (wk->poke[i].palStep == 16) {
                wk->poke[i].palState = PSEL_PAL_DARK;
            } else {
                // The step is overwritten, so the palette darkens at once
                wk->poke[i].palStep++;
                wk->poke[i].palStep = 16;
                changed = TRUE;
            }
            break;
        case PSEL_PAL_TO_NORMAL:
            if (wk->poke[i].palStep == 0) {
                wk->poke[i].palState = PSEL_PAL_NORMAL;
            } else {
                wk->poke[i].palStep--;
                changed = TRUE;
            }
            break;
        }
        if (changed) {
            if (wk->poke[i].palState == PSEL_PAL_TO_DARK || wk->poke[i].palState == PSEL_PAL_TO_NORMAL) {
                for (j = 0; j < 16; j++) {
                    u16 step = wk->poke[i].palStep;
                    u16 from = wk->poke[i].palNormal[j];
                    u16 to = wk->poke[i].palDark[j];
                    u16 fromR, fromG, fromB;
                    u16 toR, toG, toB;
                    u16 r, g, b;

                    Psel_SplitColor(from, &fromR, &fromG, &fromB);
                    Psel_SplitColor(to, &toR, &toG, &toB);
                    r = PSEL_BLEND(fromR, toR, step);
                    g = PSEL_BLEND(fromG, toG, step);
                    b = PSEL_BLEND(fromB, toB, step);
                    wk->poke[i].palNow[j] = r | (g << 5) | (b << 10);
                }
            }
            NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_SUB, sPselPokePaletteSlots[i] * 0x20,
                                               wk->poke[i].palNow, sizeof(wk->poke[i].palNow));
        }
    }
}

static void PselBGPal_Load(PselWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PSEL, wk->heapId);
    u8 i;

    for (i = 0; i < 4; i++) {
        NNSG2dPaletteData *palette;
        void *file = GFL_G2DIOReadNCLRArc(arc, sPselBGPaletteFiles[i], &palette, wk->heapId);

        sys_memcpy(&((u16 *)palette->rawData)[0x80], wk->bgPal[i], sizeof(wk->bgPal[i]));
        GFL_HeapFree(file);
    }
    GFL_ArcToolFree(arc);
    wk->bgPalActive = FALSE;
    wk->bgPalStep = 16;
    wk->bgPalTarget = 3;
    sys_memcpy(wk->bgPal[3], wk->bgPalNow, sizeof(wk->bgPalNow));
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, 0x100, wk->bgPalNow, sizeof(wk->bgPalNow));
}

static void PselBGPal_Free(PselWork *wk) {
}

static void PselBGPal_Update(PselWork *wk) {
    BOOL changed = FALSE;
    u8 i;

    if (wk->bgPalActive == TRUE) {
        if (wk->bgPalStep == 16) {
            wk->bgPalActive = FALSE;
        } else {
            wk->bgPalStep++;
            changed = TRUE;
        }
        if (changed) {
            for (i = 0; i < 0x80; i++) {
                u8 step = wk->bgPalStep;
                u16 from = wk->bgPalFrom[i];
                u16 to = wk->bgPal[wk->bgPalTarget][i];
                u16 fromR, fromG, fromB;
                u16 toR, toG, toB;
                u16 r, g, b;

                Psel_SplitColor(from, &fromR, &fromG, &fromB);
                Psel_SplitColor(to, &toR, &toG, &toB);
                r = PSEL_BLEND(fromR, toR, step);
                g = PSEL_BLEND(fromG, toG, step);
                b = PSEL_BLEND(fromB, toB, step);
                wk->bgPalNow[i] = r | (g << 5) | (b << 10);
            }
            NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, 0x100, wk->bgPalNow, sizeof(wk->bgPalNow));
        }
    }
}

static void PselPal_Load(PselWork *wk) {
    PselPokePal_Load(wk);
    PselBGPal_Load(wk);
}

static void PselPal_Free(PselWork *wk) {
    PselPokePal_Free(wk);
    PselBGPal_Free(wk);
}

static void PselPal_Update(PselWork *wk) {
    PselPokePal_Update(wk);
    PselBGPal_Update(wk);
}

// Fades a Pokémon's palette to its own colors (PSEL_PAL_NORMAL) or to the dark ones (PSEL_PAL_DARK)
static void PselPokePal_Request(PselWork *wk, u8 poke, u32 state) {
    switch (state) {
    case PSEL_PAL_NORMAL:
        if (wk->poke[poke].palState != PSEL_PAL_NORMAL) {
            wk->poke[poke].palState = PSEL_PAL_TO_NORMAL;
        }
        break;
    case PSEL_PAL_DARK:
        if (wk->poke[poke].palState != PSEL_PAL_DARK) {
            wk->poke[poke].palState = PSEL_PAL_TO_DARK;
        }
        break;
    }
}

// Fades the sub screen's BG palette from what it shows to another one
static void PselBGPal_SetTarget(PselWork *wk, u8 target) {
    if (wk->bgPalTarget != target) {
        wk->bgPalActive = TRUE;
        wk->bgPalStep = 0;
        wk->bgPalTarget = target;
        sys_memcpy(wk->bgPalNow, wk->bgPalFrom, sizeof(wk->bgPalFrom));
    }
}

static s32 PselSeq_Start(PselWork *wk, s32 *step) {
    return PSEL_SEQ_INIT;
}

static s32 PselSeq_Init(PselWork *wk, s32 *step) {
    wk->graphic = PselGraphic_Create(1, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    GFL_BGSysSetBGPriority(0, 2);
    GFL_BGSysSetBGPriority(3, 3);
    GFL_BGSysSetBGPriority(1, 1);
    GFL_BGSysSetBGPriority(2, 0);
    GFL_BGSysSetBGPriority(4, 3);
    GFL_BGSysSetBGPriority(5, 2);
    GFL_BGSysSetBGPriority(6, 1);
    GFL_BGSysSetBGPriority(7, 0);
    wk->vblankTask = GFL_VBlankTCBAdd(Psel_VBlank, wk, 1);
    wk->bgRequest = 0;
    Psel3D_Init(wk);
    PselBand_Init(wk);
    PselText_Init(wk);
    PselMenu_InitRes(wk);
    return PSEL_SEQ_LOAD;
}

static s32 PselSeq_Load(PselWork *wk, s32 *step) {
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysResetStdPalette(0, GX_RGB(0, 0, 0));
    GFL_BGSysResetStdPalette(4, GX_RGB(0, 0, 0));
    Psel3D_Load(wk);
    PselPoke_Load(wk);
    PselCursor_Load(wk);
    PselBand_LoadScreens(wk);
    PselSubBG_Load(wk);
    PselPal_Load(wk);
    wk->bgRequest |= PSEL_BG_REQ_MAIN_OFF;
    wk->bgRequest |= PSEL_BG_REQ_SUB_OFF;
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1, 0x3f, 8, 8);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG2, 0x3f, 8, 8);
    wk->frame = 0;
    return PSEL_SEQ_FADE_IN;
}

static s32 PselSeq_FadeIn(PselWork *wk, s32 *step) {
    if (wk->frame >= 16) {
        Psel3D_UpdateIntro(wk);
    }
    PselBall_Update(wk);
    PselPoke_Update(wk);
    PselPal_Update(wk);
    switch (*step) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_WHITE, 16, 0, 2);
        (*step)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            wk->frame = 0;
            return PSEL_SEQ_MAIN;
        }
        break;
    }
    wk->frame++;
    return PSEL_SEQ_FADE_IN;
}

static s32 PselSeq_Main(PselWork *wk, s32 *step) {
    // Set when the Pokémon at the front changes, to clear its name
    BOOL changed = FALSE;

    PselBall_Update(wk);
    PselPoke_Update(wk);
    PselPoke_UpdateFadeIn(wk);
    PselPal_Update(wk);
    switch (*step) {
    case 0:
        Psel3D_UpdateIntro(wk);
        if (wk->introFrame == PSEL_INTRO_FLASH_FRAME) {
            GFL_SndSEPlay(SEQ_SE_SW_BELLBOX_01);
            GFL_SndSEPlay(SEQ_SE_SW_BELLBOX_02);
            GFL_FadeSet(FADE_ENGINE_B_WHITE, 16, 0, 2);
        } else if (wk->introFrame == PSEL_INTRO_SE_FRAME) {
            GFL_SndSEPlay(SEQ_SE_SW_BELLBOX_03);
        }
        if (wk->introFrame == PSEL_INTRO_BALLS_FRAME) {
            s32 i;

            for (i = 1; i <= 3; i++) {
                wk->objs[i].draw = TRUE;
            }
        } else if (wk->introFrame == PSEL_INTRO_FRAMES) {
            *step = 1;
        }
        break;
    case 1:
        if (GFL_FadeIsRunning() == TRUE) {
            return PSEL_SEQ_MAIN;
        }
        wk->bgRequest |= PSEL_BG_REQ_MAIN_ON;
        PselText_StartStream(wk, AspertiaCity_Text_ChoosePokemon);
        *step = 2;
        PselPoke_StartFadeIn(wk);
        break;
    case 2:
        if (PselText_UpdateStream(wk) && PselPoke_IsFadeInDone(wk)) {
            *step = 3;
            wk->touchMode = TRUE;
        }
        break;
    case 3:
        if (!wk->touchMode) {
            if (wk->choice == PSEL_CHOICE_NONE) {
                changed = TRUE;
                wk->choice = 1;
            }
            PselCursor_Show(wk, TRUE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            PselCursor_Move(wk);
            PselBall_Select(wk, wk->choice);
            PselPoke_Select(wk, wk->choice);
            PselBGPal_SetTarget(wk, PSEL_BG_PAL_OF(wk->choice));
        } else {
            PselCursor_Show(wk, FALSE);
            PselBall_Select(wk, PSEL_CHOICE_NONE);
            PselPoke_Select(wk, PSEL_CHOICE_NONE);
            PselBGPal_SetTarget(wk, 3);
            changed = TRUE;
        }
        *step = 4;
        break;
    case 4:
        if (!wk->touchMode) {
            if (GCTX_HIDGetPressedKeys()) {
                BOOL moved = FALSE;

                if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    PselCursor_Show(wk, FALSE);
                    *step = 8;
                } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
                    if (wk->choice > 0) {
                        wk->choice--;
                        moved = TRUE;
                    }
                } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
                    if (wk->choice < 2) {
                        wk->choice++;
                        moved = TRUE;
                    }
                }
                if (moved) {
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                    PselCursor_Move(wk);
                    PselBall_Select(wk, wk->choice);
                    PselPoke_Select(wk, wk->choice);
                    PselBGPal_SetTarget(wk, PSEL_BG_PAL_OF(wk->choice));
                    changed = TRUE;
                }
            } else if (func_0203da48()) {
                s32 hit = func_0203da0c(sPselTouchRects);

                if (hit >= 0 && hit <= 2) {
                    if (wk->choice != hit) {
                        changed = TRUE;
                    }
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                    PselCursor_Show(wk, FALSE);
                    wk->choice = hit;
                    PselBall_Select(wk, hit);
                    PselPoke_Select(wk, wk->choice);
                    PselBGPal_SetTarget(wk, PSEL_BG_PAL_OF(wk->choice));
                    wk->touchMode = TRUE;
                    *step = 5;
                }
            }
        } else {
            if (GCTX_HIDGetPressedKeys()) {
                wk->touchMode = FALSE;
                *step = 3;
            } else if (func_0203da48()) {
                s32 hit = func_0203da0c(sPselTouchRects);

                if (hit >= 0 && hit <= 2) {
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                    wk->choice = hit;
                    PselBall_Select(wk, hit);
                    PselPoke_Select(wk, wk->choice);
                    PselBGPal_SetTarget(wk, PSEL_BG_PAL_OF(wk->choice));
                    changed = TRUE;
                    *step = 5;
                }
            }
        }
        break;
    case 5:
        PselButton_Create(wk);
        *step = 6;
        break;
    case 6:
        PselButton_Update(wk);
        if (GCTX_HIDGetPressedKeys()) {
            wk->touchMode = FALSE;
            if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                PselButton_Decide(wk);
                *step = 7;
            } else {
                PselButton_Free(wk);
                GFL_BGSysQueueScrLoad(2);
                *step = 3;
            }
        } else if (func_0203da48()) {
            s32 hit = func_0203da0c(sPselTouchRects);

            if (hit == 3) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                PselButton_Decide(wk);
                *step = 7;
            } else if (hit >= 0 && hit <= 2) {
                if (wk->choice != hit) {
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                    wk->choice = hit;
                    PselBall_Select(wk, hit);
                    PselPoke_Select(wk, wk->choice);
                    PselBGPal_SetTarget(wk, PSEL_BG_PAL_OF(wk->choice));
                    changed = TRUE;
                }
            }
        }
        break;
    case 7:
        PselButton_Update(wk);
        if (PselButton_IsFlashFinished(wk)) {
            PselButton_Free(wk);
            GFL_BGSysQueueScrLoad(2);
            *step = 8;
        }
        break;
    case 8:
        PselText_StartStream(wk, AspertiaCity_Text_PokemonWant);
        PselCamera_StartZoom(wk, wk->choice, FALSE);
        *step = 9;
        break;
    case 9:
        PselCamera_UpdateZoom(wk);
        if (PselText_UpdateStream(wk) && PselCamera_IsZoomDone(wk)) {
            *step = 10;
        }
        break;
    case 10:
        func_0203d564(wk->touchMode);
        PselYesNo_Create(wk);
        *step = 11;
        break;
    case 11:
        PselYesNo_Update(wk);
        if (PselYesNo_IsFlashFinished(wk)) {
            BOOL yes = PselYesNo_IsYes(wk);

            PselYesNo_Free(wk);
            wk->touchMode = func_0203d554();
            if (yes) {
                wk->bgRequest |= PSEL_BG_REQ_MAIN_OFF;
                PselText_ClearStream(wk);
                *step = 14;
            } else {
                PselText_StartStream(wk, AspertiaCity_Text_ChoosePokemon);
                *step = 12;
            }
        }
        break;
    case 12:
        PselCamera_StartZoom(wk, wk->choice, TRUE);
        *step = 13;
        break;
    case 13:
        PselCamera_UpdateZoom(wk);
        if (PselCamera_IsZoomDone(wk)) {
            *step = 3;
        }
        break;
    case 14:
        PselCamera_StartDecide(wk, wk->choice);
        GFL_SndSEPlay(SEQ_SE_SDEMO_03);
        *step = 15;
        break;
    case 15:
        PselCamera_UpdateDecide(wk);
        if (wk->cameraFrame == sPselCameraFrames[wk->cameraBall].fadeOut - 2) {
            *step = 16;
        }
        break;
    case 16:
        PselCamera_UpdateDecide(wk);
        wk->frame = 0;
        return PSEL_SEQ_FADE_OUT;
    }

    if (changed) {
        wk->bgRequest |= PSEL_BG_REQ_SUB_OFF;
        PselText_ClearType(wk);
    } else if (wk->pokeArrived) {
        wk->bgRequest |= PSEL_BG_REQ_SUB_ON;
        PselText_PrintType(wk, wk->choice);
        PokeVoice_Play(sPselSpecies[wk->choice], 0, 64, FALSE, 0, 0, FALSE, NULL);
    }
    wk->pokeArrived = FALSE;
    wk->frame++;
    return PSEL_SEQ_MAIN;
}

static s32 PselSeq_FadeOut(PselWork *wk, s32 *step) {
    PselCamera_UpdateDecide(wk);
    PselBall_Update(wk);
    PselPoke_Update(wk);
    PselPal_Update(wk);
    switch (*step) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 0, 16, 2);
        (*step)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            wk->frame = 0;
            return PSEL_SEQ_UNLOAD;
        }
        break;
    }
    wk->frame++;
    return PSEL_SEQ_FADE_OUT;
}

static s32 PselSeq_Unload(PselWork *wk, s32 *step) {
    G2_BlendNone();
    G2S_BlendNone();
    PselPal_Free(wk);
    PselSubBG_Unload(wk);
    PselBand_ClearScreens(wk);
    PselCursor_Unload(wk);
    PselPoke_Unload(wk);
    Psel3D_Unload(wk);
    wk->frame = 0;
    return PSEL_SEQ_FREE;
}

static s32 PselSeq_Free(PselWork *wk, s32 *step) {
    PselMenu_FreeRes(wk);
    PselText_Free(wk);
    PselBand_Free(wk);
    Psel3D_Free(wk);
    GFL_TCBRemove(wk->vblankTask);
    func_02021c44(wk->printQueue);
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
    PselGraphic_Free(wk->graphic);
    return PSEL_SEQ_END;
}

static s32 PselSeq_End(PselWork *wk, s32 *step) {
    return PSEL_SEQ_END;
}
