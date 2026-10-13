#include "types.h"
#include "app/zukan_common.h"
#include "app/zukan_open.h"
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
    ZukanOpenParam *param;
    TCB *vblankTask;
    u16 timer;
    u8 frame;
    u8 part;
    u32 step;
} ZukanOpenWork;

static BOOL ZukanOpen_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanOpen_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanOpen_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanOpen_RunStep(ZukanOpenWork *work);
static void ZukanOpen_SetVRAM(void);
static void ZukanOpen_VBlank(TCB *tcb, void *data);
static void ZukanOpen_AddVBlank(ZukanOpenWork *work);
static void ZukanOpen_RemoveVBlank(ZukanOpenWork *work);
static inline void ZukanOpen_InitCoverTopBG(void);
static inline void ZukanOpen_InitCoverBackBG(void);
static inline void ZukanOpen_InitCoverFrontBG(void);
static void ZukanOpen_InitBG(void);
static inline void ZukanOpen_InitSubTopBG(void);
static inline void ZukanOpen_InitCoverSubBG(void);
static void ZukanOpen_FreeBG(void);
static void ZukanOpen_LoadGraphics(ZukanOpenWork *work);
static void ZukanOpen_LoadSkin(ZukanOpenWork *work);
static u32 ZukanOpen_StepSetup(ZukanOpenWork *work);
static u32 ZukanOpen_StepEnd(ZukanOpenWork *work);
static u32 ZukanOpen_StepWaitWipeIn(ZukanOpenWork *work);
static u32 ZukanOpen_StepWaitWipeOut(ZukanOpenWork *work);
static u32 ZukanOpen_StepWaitInput(ZukanOpenWork *work);
static u32 ZukanOpen_StepOpen(ZukanOpenWork *work);
static void ZukanOpen_ZoomCover(void);

const GameProcFunctions ZUKAN_OPEN_PROC_FUNCTIONS = { ZukanOpen_Init, ZukanOpen_Main, ZukanOpen_Exit };

static u32 (*const sSteps[])(ZukanOpenWork *work) = {
    ZukanOpen_StepSetup,       ZukanOpen_StepEnd,       ZukanOpen_StepWaitWipeIn,
    ZukanOpen_StepWaitWipeOut, ZukanOpen_StepWaitInput, ZukanOpen_StepOpen,
};

static BOOL ZukanOpen_Init(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanOpenWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_ZUKAN_OPEN, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(ZukanOpenWork), HEAPID_ZUKAN_OPEN);
    sys_memset(wk, 0, sizeof(ZukanOpenWork));
    wk->param = param;
    return TRUE;
}

static BOOL ZukanOpen_Main(GameProc *proc, u32 *state, void *param, void *work) {
    if (ZukanOpen_RunStep(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ZukanOpen_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_ZUKAN_OPEN);
    return TRUE;
}

static BOOL ZukanOpen_RunStep(ZukanOpenWork *work) {
    work->step = sSteps[work->step](work);
    if (work->step != STEP_DONE) {
        return TRUE;
    }
    return FALSE;
}

static void ZukanOpen_SetVRAM(void) {
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

static void ZukanOpen_VBlank(TCB *tcb, void *data) {
    GFL_BGSysUpdate();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

static void ZukanOpen_AddVBlank(ZukanOpenWork *work) {
    work->vblankTask = GFL_VBlankTCBAdd(ZukanOpen_VBlank, work, 0);
}

static void ZukanOpen_RemoveVBlank(ZukanOpenWork *work) {
    GFL_TCBRemove(work->vblankTask);
}

static inline void ZukanOpen_InitCoverTopBG(void) {
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

static inline void ZukanOpen_InitCoverBackBG(void) {
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
    GFL_BGSysClearCharCore(BG_COVER_BACK, BGSYS_TILE_SIZE_16, 0x7fe0, HEAPID_TAIL(HEAPID_ZUKAN_OPEN));
    GFL_BGSysFillScrAsync(BG_COVER_BACK, 0x7fe0 / BGSYS_TILE_SIZE_16);
}

static inline void ZukanOpen_InitCoverFrontBG(void) {
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

static void ZukanOpen_InitBG(void) {
    GFL_BGSysCreate(HEAPID_ZUKAN_OPEN);
    BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_3, GX_BGMODE_3, FALSE };
    GFL_BGSysSetLCDConfig(&lcdConfig);
    ZukanOpen_InitCoverTopBG();
    ZukanOpen_InitCoverBackBG();
    ZukanOpen_InitCoverFrontBG();
    ZukanOpen_InitSubTopBG();
    ZukanOpen_InitCoverSubBG();
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG3, TRUE);
}

static inline void ZukanOpen_InitSubTopBG(void) {
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

static inline void ZukanOpen_InitCoverSubBG(void) {
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

static void ZukanOpen_FreeBG(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysReleaseBG(BG_COVER_SUB);
    GFL_BGSysReleaseBG(BG_SUB_TOP);
    GFL_BGSysReleaseBG(BG_COVER_FRONT);
    GFL_BGSysReleaseBG(BG_COVER_BACK);
    GFL_BGSysReleaseBG(BG_COVER_TOP);
    GFL_BGSysFree();
}

static void ZukanOpen_LoadGraphics(ZukanOpenWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_TAIL(HEAPID_ZUKAN_OPEN));

    if (getTrainerGender(work->param->playerInfo) == GENDER_MALE) {
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x68, PALTYPE_MAIN_BG_EX, 0x4000, 0, HEAPID_ZUKAN_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x68, PALTYPE_MAIN_BG_EX, 0x6000, 0, HEAPID_ZUKAN_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x68, PALTYPE_SUB_BG_EX, 0x4000, 0, HEAPID_ZUKAN_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x68, PALTYPE_SUB_BG_EX, 0x6000, 0, HEAPID_ZUKAN_OPEN);
    } else {
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x69, PALTYPE_MAIN_BG_EX, 0x4000, 0, HEAPID_ZUKAN_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x69, PALTYPE_MAIN_BG_EX, 0x6000, 0, HEAPID_ZUKAN_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x69, PALTYPE_SUB_BG_EX, 0x4000, 0, HEAPID_ZUKAN_OPEN);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0x69, PALTYPE_SUB_BG_EX, 0x6000, 0, HEAPID_ZUKAN_OPEN);
    }
    GFL_BGSysLoadArcNCGRStatic(arc, 0x6b, BG_COVER_FRONT, 0, 0, FALSE, HEAPID_ZUKAN_OPEN);
    GFL_BGSysLoadArcNCGRStatic(arc, 0x6b, BG_COVER_SUB, 0, 0, FALSE, HEAPID_ZUKAN_OPEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x6e, BG_COVER_TOP, 0, 0, FALSE, HEAPID_ZUKAN_OPEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x71, BG_COVER_BACK, 0, 0, FALSE, HEAPID_ZUKAN_OPEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x70, BG_COVER_FRONT, 0, 0, FALSE, HEAPID_ZUKAN_OPEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x72, BG_SUB_TOP, 0, 0, FALSE, HEAPID_ZUKAN_OPEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0x6f, BG_COVER_SUB, 0, 0, FALSE, HEAPID_ZUKAN_OPEN);
    GFL_ArcToolFree(arc);
}

// Replaces the cover's characters and palette with the downloaded Pokédex skin, if there is a valid one
static void ZukanOpen_LoadSkin(ZukanOpenWork *work) {
    SaveControl *save = GameData_GetSaveControl(work->param->gameData);
    void *block;
    void *characters;
    void *palette;

    if (func_020074ec(save, SKIN_BLOCK, HEAPID_TAIL(HEAPID_ZUKAN_OPEN)) == TRUE) {
        block = getAddressOfExtraSaveBlk(save, SKIN_BLOCK, 0);
        if (block != NULL && func_0200f18c(block) == TRUE) {
            characters = func_0200f180(block);
            palette = func_0200f184(block);
            GFL_BGSysLoadChar(BG_COVER_BACK, characters, SKIN_CHAR_SIZE, 0);
            GFL_BGSysUploadStdPalette(BG_COVER_BACK, palette, SKIN_PALETTE_SIZE, 0);
            GFL_BGSysUploadStdPalette(BG_COVER_SUB, palette, SKIN_PALETTE_SIZE, 0);
            cp15_flushDC(palette, SKIN_PALETTE_SIZE);
            gfxBeginBGExtPltAUpload();
            gfxUploadExtPaletteBGA(palette, 0x4000, SKIN_PALETTE_SIZE);
            gfxUploadExtPaletteBGA(palette, 0x6000, SKIN_PALETTE_SIZE);
            gfxEndBGExtPltAUpload();
            gfxBeginBGExtPltBUpload();
            gfxUploadExtPaletteBGB(palette, 0x4000, SKIN_PALETTE_SIZE);
            gfxUploadExtPaletteBGB(palette, 0x6000, SKIN_PALETTE_SIZE);
            gfxEndBGExtPltBUpload();
        }
    }
    freeIntermediateSaveExtraBlksAfterLoad(save, SKIN_BLOCK);
}

static u32 ZukanOpen_StepSetup(ZukanOpenWork *work) {
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetDisplayLayout(0);
    ZukanOpen_SetVRAM();
    ZukanOpen_InitBG();
    ZukanOpen_LoadGraphics(work);
    ZukanOpen_LoadSkin(work);
    func_02042ba8(TRUE, HEAPID_ZUKAN_OPEN);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    ZukanOpen_AddVBlank(work);
    ZukanCommon_WipeIn(HEAPID_ZUKAN_OPEN);
    return STEP_WAIT_WIPE_IN;
}

static u32 ZukanOpen_StepEnd(ZukanOpenWork *work) {
    ZukanOpen_RemoveVBlank(work);
    ZukanOpen_FreeBG();
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    return STEP_DONE;
}

static u32 ZukanOpen_StepWaitWipeIn(ZukanOpenWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        return STEP_WAIT_INPUT;
    }
    return STEP_WAIT_WIPE_IN;
}

static u32 ZukanOpen_StepWaitWipeOut(ZukanOpenWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        return STEP_END;
    }
    return STEP_WAIT_WIPE_OUT;
}

// Waits for the player to open the cover, or to leave
static u32 ZukanOpen_StepWaitInput(ZukanOpenWork *work) {
    if (work->timer == WAIT_FRAMES) {
        return STEP_OPEN;
    }
    work->timer++;
    if (func_0203da2c() == TRUE) {
        func_0203d564(TRUE);
        return STEP_OPEN;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        func_0203d564(FALSE);
        return STEP_OPEN;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        func_0203d564(FALSE);
        work->param->result = ZUKAN_OPEN_RESULT_BACK;
        ZukanCommon_WipeOut(HEAPID_ZUKAN_OPEN);
        return STEP_WAIT_WIPE_OUT;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        func_0203d564(FALSE);
        work->param->result = ZUKAN_OPEN_RESULT_CLOSE;
        ZukanCommon_WipeOut(HEAPID_ZUKAN_OPEN);
        return STEP_WAIT_WIPE_OUT;
    }
    return STEP_WAIT_INPUT;
}

// Slides the cover's halves apart, zooms into the cover and wipes out
static u32 ZukanOpen_StepOpen(ZukanOpenWork *work) {
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
        ZukanOpen_ZoomCover();
        if (work->frame == 4) {
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 4,
                        HEAPID_ZUKAN_OPEN);
            work->frame = 0;
            work->part++;
        } else {
            work->frame++;
        }
        break;
    case OPEN_WIPE:
        ZukanOpen_ZoomCover();
        if (GFL_WipeIsFinished() == TRUE) {
            work->param->result = ZUKAN_OPEN_RESULT_DONE;
            return STEP_END;
        }
        break;
    }
    return STEP_OPEN;
}

// Grows the cover and the sub screen's, a little each frame
static void ZukanOpen_ZoomCover(void) {
    GFL_BGSysScaleBGReq(BG_COVER_TOP, BG_SCALE_ADD_X, 0x80);
    GFL_BGSysScaleBGReq(BG_COVER_TOP, BG_SCALE_ADD_Y, 0x80);
    GFL_BGSysScaleBGReq(BG_SUB_TOP, BG_SCALE_ADD_X, 0x80);
    GFL_BGSysScaleBGReq(BG_SUB_TOP, BG_SCALE_ADD_Y, 0x80);
}
