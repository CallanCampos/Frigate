#include "types.h"
#include "app/pokedex_common.h"
#include "app/pokedex_open.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/wipe.h"

// The Pokédex's opening

// The extra save block of the Pokédex skin
#define SKIN_BLOCK 7
// The size of the skin's characters, and of its palette
#define SKIN_CHAR_SIZE 0x6000
#define SKIN_PALETTE_SIZE 0x200

// The BGs. 1, 2, 3 and 5 are the cover's halves and 7 is its background
#define BG_COVER_BACK 1
#define BG_COVER_FRONT 2
#define BG_COVER_TOP 3
#define BG_COVER_SUB 5
#define BG_SUB_TOP 7

// The frames that the opening waits for the player
#define WAIT_FRAMES 300

// The steps of the opening, which the table of steps follows
enum {
    STEP_SETUP,
    STEP_END,
    STEP_WAIT_WIPE_IN,
    STEP_WAIT_WIPE_OUT,
    STEP_WAIT_INPUT,
    STEP_OPEN,
    STEP_DONE,
};

// The parts of STEP_OPEN, in the frames they take
enum {
    OPEN_SLIDE,
    OPEN_ZOOM_START,
    OPEN_ZOOM,
    OPEN_WIPE,
};

typedef struct {
    PokedexOpenParam *param;
    TCB *vblankTask;
    u16 timer;
    u8 frame;
    u8 part;
    u32 step;
} PokedexOpenWork;

static BOOL PokedexOpen_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexOpen_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexOpen_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexOpen_RunStep(PokedexOpenWork *work);
static void PokedexOpen_SetVRAM(void);
static void PokedexOpen_VBlank(TCB *tcb, void *data);
static void PokedexOpen_AddVBlank(PokedexOpenWork *work);
static void PokedexOpen_RemoveVBlank(PokedexOpenWork *work);
static inline void PokedexOpen_InitCoverTopBG(void);
static inline void PokedexOpen_InitCoverBackBG(void);
static inline void PokedexOpen_InitCoverFrontBG(void);
static void PokedexOpen_InitBG(void);
static inline void PokedexOpen_InitSubTopBG(void);
static inline void PokedexOpen_InitCoverSubBG(void);
static void PokedexOpen_FreeBG(void);
static void PokedexOpen_LoadGraphics(PokedexOpenWork *work);
static void PokedexOpen_LoadSkin(PokedexOpenWork *work);
static u32 PokedexOpen_StepSetup(PokedexOpenWork *work);
static u32 PokedexOpen_StepEnd(PokedexOpenWork *work);
static u32 PokedexOpen_StepWaitWipeIn(PokedexOpenWork *work);
static u32 PokedexOpen_StepWaitWipeOut(PokedexOpenWork *work);
static u32 PokedexOpen_StepWaitInput(PokedexOpenWork *work);
static u32 PokedexOpen_StepOpen(PokedexOpenWork *work);
static void PokedexOpen_ZoomCover(void);

const GameProcFunctions POKEDEX_OPEN_PROC_FUNCTIONS = { PokedexOpen_Init, PokedexOpen_Main, PokedexOpen_Exit };

static u32 (*const sSteps[])(PokedexOpenWork *work) = {
    PokedexOpen_StepSetup,       PokedexOpen_StepEnd,       PokedexOpen_StepWaitWipeIn,
    PokedexOpen_StepWaitWipeOut, PokedexOpen_StepWaitInput, PokedexOpen_StepOpen,
};

static BOOL PokedexOpen_Init(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexOpenWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_POKEDEX_OPEN, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PokedexOpenWork), HEAPID_POKEDEX_OPEN);
    MI_CpuFill8(wk, 0, sizeof(PokedexOpenWork));
    wk->param = param;
    return TRUE;
}

static BOOL PokedexOpen_Main(GameProc *proc, u32 *state, void *param, void *work) {
    if (PokedexOpen_RunStep(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL PokedexOpen_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_POKEDEX_OPEN);
    return TRUE;
}

static BOOL PokedexOpen_RunStep(PokedexOpenWork *work) {
    work->step = sSteps[work->step](work);
    if (work->step != STEP_DONE) {
        return TRUE;
    }
    return FALSE;
}

static void PokedexOpen_SetVRAM(void) {
    BGSysVRAMConfig config = {
        GX_VRAM_BG_128_A,
        GX_VRAM_E,
        GX_VRAM_SUB_BG_128_C,
        GX_VRAM_SUB_BGEXTPLTT_0123_H,
        GX_VRAM_OBJ_128_B,
        GX_VRAM_OBJEXTPLTT_NONE,
        GX_VRAM_SUB_OBJ_128_D,
        GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_NONE,
        GX_VRAM_TEXPLTT_NONE,
        GX_OBJVRAMMODE_CHAR_1D_128K,
        GX_OBJVRAMMODE_CHAR_1D_128K,
    };

    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&config);
}

static void PokedexOpen_VBlank(TCB *tcb, void *data) {
    GFL_BGSysUpdate();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

static void PokedexOpen_AddVBlank(PokedexOpenWork *work) {
    work->vblankTask = GFL_VBlankTCBAdd(PokedexOpen_VBlank, work, 0);
}

static void PokedexOpen_RemoveVBlank(PokedexOpenWork *work) {
    GFL_TCBRemove(work->vblankTask);
}

static inline void PokedexOpen_InitCoverTopBG(void) {
    BGSetup coverTop = { 0,
                         0xe8,
                         0x2000,
                         0,
                         BGRES_512x512,
                         GX_BG_COLORMODE_256,
                         GX_BG_SCRBASE(0xe000),
                         GX_BG_CHARBASE(0x10000),
                         0x10000,
                         GX_BG_EXTPLTT_23,
                         2,
                         GX_BG_AREAOVER_REPEAT,
                         FALSE };
    GFL_BGSysCreateBG(BG_COVER_TOP, &coverTop, BGMODE_EXTENDED);
    GFL_BGSysClearScr(BG_COVER_TOP);
}

static inline void PokedexOpen_InitCoverBackBG(void) {
    BGSetup coverBack = { 0,
                          0,
                          0x1000,
                          0,
                          BGRES_256x512,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xd000),
                          GX_BG_CHARBASE(0),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          1,
                          GX_BG_AREAOVER_REPEAT,
                          FALSE };
    GFL_BGSysCreateBG(BG_COVER_BACK, &coverBack, BGMODE_TEXT);
    GFL_BGSysClearCharCore(BG_COVER_BACK, BGSYS_TILE_SIZE_16, 0x7fe0, HEAPID_TAIL(HEAPID_POKEDEX_OPEN));
    GFL_BGSysFillScrAsync(BG_COVER_BACK, 0x7fe0 / BGSYS_TILE_SIZE_16);
}

static inline void PokedexOpen_InitCoverFrontBG(void) {
    BGSetup coverFront = { 0,
                           0,
                           0x1000,
                           0,
                           BGRES_256x512,
                           GX_BG_COLORMODE_256,
                           GX_BG_SCRBASE(0xc000),
                           GX_BG_CHARBASE(0x10000),
                           0x10000,
                           GX_BG_EXTPLTT_23,
                           0,
                           GX_BG_AREAOVER_REPEAT,
                           FALSE };
    GFL_BGSysCreateBG(BG_COVER_FRONT, &coverFront, BGMODE_TEXT);
    GFL_BGSysClearScr(BG_COVER_FRONT);
}

static void PokedexOpen_InitBG(void) {
    GFL_BGSysCreate(HEAPID_POKEDEX_OPEN);
    BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_3, GX_BGMODE_3, FALSE };
    GFL_BGSysSetLCDConfig(&lcdConfig);
    PokedexOpen_InitCoverTopBG();
    PokedexOpen_InitCoverBackBG();
    PokedexOpen_InitCoverFrontBG();
    PokedexOpen_InitSubTopBG();
    PokedexOpen_InitCoverSubBG();
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG3, TRUE);
}

static inline void PokedexOpen_InitSubTopBG(void) {
    BGSetup subTop = { 0,
                       0,
                       0x800,
                       0,
                       BGRES_256x256,
                       GX_BG_COLORMODE_256,
                       GX_BG_SCRBASE(0xf800),
                       GX_BG_CHARBASE(0x10000),
                       0x10000,
                       GX_BG_EXTPLTT_01,
                       1,
                       GX_BG_AREAOVER_REPEAT,
                       FALSE };
    GFL_BGSysCreateBG(BG_SUB_TOP, &subTop, BGMODE_EXTENDED);
    GFL_BGSysClearScr(BG_SUB_TOP);
}

static inline void PokedexOpen_InitCoverSubBG(void) {
    BGSetup coverSub = { 0,
                         0x18,
                         0x1000,
                         0,
                         BGRES_256x512,
                         GX_BG_COLORMODE_256,
                         GX_BG_SCRBASE(0xe800),
                         GX_BG_CHARBASE(0x10000),
                         0x8000,
                         GX_BG_EXTPLTT_23,
                         0,
                         GX_BG_AREAOVER_REPEAT,
                         FALSE };
    GFL_BGSysCreateBG(BG_COVER_SUB, &coverSub, BGMODE_TEXT);
    GFL_BGSysClearScr(BG_COVER_SUB);
}

static void PokedexOpen_FreeBG(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysReleaseBG(BG_COVER_SUB);
    GFL_BGSysReleaseBG(BG_SUB_TOP);
    GFL_BGSysReleaseBG(BG_COVER_FRONT);
    GFL_BGSysReleaseBG(BG_COVER_BACK);
    GFL_BGSysReleaseBG(BG_COVER_TOP);
    GFL_BGSysFree();
}

static void PokedexOpen_LoadGraphics(PokedexOpenWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, HEAPID_TAIL(HEAPID_POKEDEX_OPEN));

    if (getTrainerGender(work->param->playerInfo) == GENDER_MALE) {
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x68, PALTYPE_MAIN_BG_EX, 0x4000, 0, HEAPID_POKEDEX_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x68, PALTYPE_MAIN_BG_EX, 0x6000, 0, HEAPID_POKEDEX_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x68, PALTYPE_SUB_BG_EX, 0x4000, 0, HEAPID_POKEDEX_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x68, PALTYPE_SUB_BG_EX, 0x6000, 0, HEAPID_POKEDEX_OPEN);
    } else {
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x69, PALTYPE_MAIN_BG_EX, 0x4000, 0, HEAPID_POKEDEX_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x69, PALTYPE_MAIN_BG_EX, 0x6000, 0, HEAPID_POKEDEX_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x69, PALTYPE_SUB_BG_EX, 0x4000, 0, HEAPID_POKEDEX_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x69, PALTYPE_SUB_BG_EX, 0x6000, 0, HEAPID_POKEDEX_OPEN);
    }
    GFL_BGSysLoadArcNCGRStatic(arc, 0x6b, BG_COVER_FRONT, 0, 0, FALSE, HEAPID_POKEDEX_OPEN);
    GFL_BGSysLoadArcNCGRStatic(arc, 0x6b, BG_COVER_SUB, 0, 0, FALSE, HEAPID_POKEDEX_OPEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x6e, BG_COVER_TOP, 0, 0, FALSE, HEAPID_POKEDEX_OPEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x71, BG_COVER_BACK, 0, 0, FALSE, HEAPID_POKEDEX_OPEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x70, BG_COVER_FRONT, 0, 0, FALSE, HEAPID_POKEDEX_OPEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x72, BG_SUB_TOP, 0, 0, FALSE, HEAPID_POKEDEX_OPEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x6f, BG_COVER_SUB, 0, 0, FALSE, HEAPID_POKEDEX_OPEN);
    GFL_ArcToolFree(arc);
}

// Replaces the cover's characters and palette with the downloaded Pokédex skin, if there is a valid one
static void PokedexOpen_LoadSkin(PokedexOpenWork *work) {
    SaveControl *save = GameData_GetSaveControl(work->param->gameData);
    void *block;
    void *characters;
    void *palette;

    if (SaveControl_OpenExtraBlock(save, SKIN_BLOCK, HEAPID_TAIL(HEAPID_POKEDEX_OPEN)) == TRUE) {
        block = getAddressOfExtraSaveBlk(save, SKIN_BLOCK, 0);
        if (block != NULL && PokedexSkinSave_IsValid(block) == TRUE) {
            characters = PokedexSkinSave_GetChars(block);
            palette = PokedexSkinSave_GetPalette(block);
            GFL_BGSysLoadChar(BG_COVER_BACK, characters, SKIN_CHAR_SIZE, 0);
            GFL_BGSysUploadStdPalette(BG_COVER_BACK, palette, SKIN_PALETTE_SIZE, 0);
            GFL_BGSysUploadStdPalette(BG_COVER_SUB, palette, SKIN_PALETTE_SIZE, 0);
            DC_FlushRange(palette, SKIN_PALETTE_SIZE);
            GX_BeginLoadBGExtPltt();
            GX_LoadBGExtPltt(palette, 0x4000, SKIN_PALETTE_SIZE);
            GX_LoadBGExtPltt(palette, 0x6000, SKIN_PALETTE_SIZE);
            GX_EndLoadBGExtPltt();
            GXS_BeginLoadBGExtPltt();
            GXS_LoadBGExtPltt(palette, 0x4000, SKIN_PALETTE_SIZE);
            GXS_LoadBGExtPltt(palette, 0x6000, SKIN_PALETTE_SIZE);
            GXS_EndLoadBGExtPltt();
        }
    }
    freeIntermediateSaveExtraBlksAfterLoad(save, SKIN_BLOCK);
}

static u32 PokedexOpen_StepSetup(PokedexOpenWork *work) {
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetDisplayLayout(0);
    PokedexOpen_SetVRAM();
    PokedexOpen_InitBG();
    PokedexOpen_LoadGraphics(work);
    PokedexOpen_LoadSkin(work);
    GFL_NetSetIconScreen(TRUE, HEAPID_POKEDEX_OPEN);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    PokedexOpen_AddVBlank(work);
    PokedexCommon_WipeIn(HEAPID_POKEDEX_OPEN);
    return STEP_WAIT_WIPE_IN;
}

static u32 PokedexOpen_StepEnd(PokedexOpenWork *work) {
    PokedexOpen_RemoveVBlank(work);
    PokedexOpen_FreeBG();
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    return STEP_DONE;
}

static u32 PokedexOpen_StepWaitWipeIn(PokedexOpenWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        return STEP_WAIT_INPUT;
    }
    return STEP_WAIT_WIPE_IN;
}

static u32 PokedexOpen_StepWaitWipeOut(PokedexOpenWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        return STEP_END;
    }
    return STEP_WAIT_WIPE_OUT;
}

// Waits for the player to open the cover, or to leave
static u32 PokedexOpen_StepWaitInput(PokedexOpenWork *work) {
    if (work->timer == WAIT_FRAMES) {
        return STEP_OPEN;
    }
    work->timer++;
    if (GFL_HIDIsTouchHeld() == TRUE) {
        GCTX_HIDSetUsingTouch(TRUE);
        return STEP_OPEN;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        GCTX_HIDSetUsingTouch(FALSE);
        return STEP_OPEN;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GCTX_HIDSetUsingTouch(FALSE);
        work->param->result = POKEDEX_OPEN_RESULT_BACK;
        PokedexCommon_WipeOut(HEAPID_POKEDEX_OPEN);
        return STEP_WAIT_WIPE_OUT;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        GCTX_HIDSetUsingTouch(FALSE);
        work->param->result = POKEDEX_OPEN_RESULT_CLOSE;
        PokedexCommon_WipeOut(HEAPID_POKEDEX_OPEN);
        return STEP_WAIT_WIPE_OUT;
    }
    return STEP_WAIT_INPUT;
}

// Slides the cover's halves apart, zooms into the cover and wipes out
static u32 PokedexOpen_StepOpen(PokedexOpenWork *work) {
    switch (work->part) {
    case OPEN_SLIDE:
        if (work->frame == 26) {
            GFL_SndSEPlay(SEQ_SE_SYS_76);
            work->frame = 0;
            work->part++;
        } else {
            GFL_BGSysMoveBGReq(BG_COVER_TOP, BG_MOVE_UP, 8);
            GFL_BGSysMoveBGReq(BG_COVER_BACK, BG_MOVE_UP, 8);
            GFL_BGSysMoveBGReq(BG_COVER_FRONT, BG_MOVE_UP, 8);
            GFL_BGSysMoveBGReq(BG_COVER_SUB, BG_MOVE_UP, 8);
            work->frame++;
        }
        break;
    case OPEN_ZOOM_START:
        if (work->frame == 32) {
            GFL_SndSEPlay(SEQ_SE_SYS_77);
            GFL_BGSysAdjustBGOriginReq(BG_COVER_TOP, BG_CENTER_SET_X, 0x80);
            GFL_BGSysAdjustBGOriginReq(BG_COVER_TOP, BG_CENTER_SET_Y, 0x78);
            GFL_BGSysAdjustBGOriginReq(BG_SUB_TOP, BG_CENTER_SET_X, 0x80);
            GFL_BGSysAdjustBGOriginReq(BG_SUB_TOP, BG_CENTER_SET_Y, 0x60);
            work->frame = 0;
            work->part++;
        } else {
            work->frame++;
        }
        break;
    case OPEN_ZOOM:
        PokedexOpen_ZoomCover();
        if (work->frame == 4) {
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 4,
                        HEAPID_POKEDEX_OPEN);
            work->frame = 0;
            work->part++;
        } else {
            work->frame++;
        }
        break;
    case OPEN_WIPE:
        PokedexOpen_ZoomCover();
        if (GFL_WipeIsFinished() == TRUE) {
            work->param->result = POKEDEX_OPEN_RESULT_DONE;
            return STEP_END;
        }
        break;
    }
    return STEP_OPEN;
}

// Grows the cover and the sub screen's, a little each frame
static void PokedexOpen_ZoomCover(void) {
    GFL_BGSysScaleBGReq(BG_COVER_TOP, BG_SCALE_ADD_X, 0x80);
    GFL_BGSysScaleBGReq(BG_COVER_TOP, BG_SCALE_ADD_Y, 0x80);
    GFL_BGSysScaleBGReq(BG_SUB_TOP, BG_SCALE_ADD_X, 0x80);
    GFL_BGSysScaleBGReq(BG_SUB_TOP, BG_SCALE_ADD_Y, 0x80);
}
