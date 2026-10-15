#include "types.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "demo/staff_roll.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/calctool.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "save/player_info.h"
#include "system/gf_font.h"
#include "system/main.h"
#include "system/printsys.h"
#include "system/wipe.h"

// The credits: lines of names that scroll up from the bottom screen to the top one, titles that fade in and out, and
// the pictures of the player's journey between them, all read from a list in the end archive. The names are ours

// Black 2's credits are on black and White 2's on white, and each version has its own pictures and actor
#ifdef BLACK2
#define BACKGROUND_COLOR GX_RGB(0, 0, 0)
#define WIPE_COLOR WIPE_COLOR_BLACK
// A master brightness or brightness blend toward the background color
#define FADE_BRIGHTNESS(value) (-(value))
#define COPYRIGHT_PALETTE 1
#define ACTOR_PALETTE_FILE 6
#define ACTOR_CHAR_FILE 7
#define ACTOR_CELL_FILE 8
#define ACTOR_ANIM_FILE 9
#define LAST_PICTURE_FILE 10
#define PICTURE_VERSION_OFFSET 0
#else
#define BACKGROUND_COLOR 0xffff
#define WIPE_COLOR WIPE_COLOR_WHITE
#define FADE_BRIGHTNESS(value) (value)
#define COPYRIGHT_PALETTE 2
#define ACTOR_PALETTE_FILE 13
#define ACTOR_CHAR_FILE 14
#define ACTOR_CELL_FILE 15
#define ACTOR_ANIM_FILE 16
#define LAST_PICTURE_FILE 17
#define PICTURE_VERSION_OFFSET 6
#endif

// The bitmaps the pairs of lines are drawn into ahead of time
#define LINE_BITMAP_MAX 64

// The screens a title or a picture is shown on
#define SCREEN_TOP (1 << 0)
#define SCREEN_BOTTOM (1 << 1)

// The steps of the staff roll
enum {
    SEQ_INIT,
    SEQ_WAIT_WIPE,
    SEQ_EXIT,
    SEQ_LOGO,
    SEQ_ROLL,
    SEQ_COPYRIGHT,
    SEQ_END,
};

// The states of the roll, which the commands of the list set
enum {
    ROLL_READ,
    ROLL_WAIT,
    ROLL_FADE_IN,
    ROLL_FADE_OUT,
    ROLL_UNUSED,
    ROLL_END,
};

// The commands of the list
enum {
    CMD_LINE,
    CMD_FADE_IN,
    CMD_FADE_OUT,
    CMD_SCROLL_START,
    CMD_SCROLL_STOP,
    CMD_END,
    CMD_YIELD,
    CMD_LINE_AT,
    CMD_PICTURE,
    CMD_LAST_PICTURE,
};

// How a line is aligned on its x, or for a picture command, the one screen it shows on
enum {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER,
    ALIGN_PICTURE_TOP,
    ALIGN_PICTURE_BOTTOM,
};

// The states of a picture
enum {
    PICTURE_NONE,
    PICTURE_START,
    PICTURE_LOAD_CHAR,
    PICTURE_LOAD_SCREEN,
    PICTURE_SHOW,
    PICTURE_FADE_IN,
    PICTURE_WAIT,
    PICTURE_FADE_OUT,
    PICTURE_LAST_START,
    PICTURE_END,
};

// The files of a picture, which come in one for each gender
enum {
    PICTURE_FILE_PALETTE,
    PICTURE_FILE_CHAR,
    PICTURE_FILE_SCREEN,
};

// An entry of the list, file 1 of the end archive
typedef struct {
    u32 msgId;
    // The frames to wait after it, or the y of a title
    u16 wait;
    u8 command;
    // The screens of a title or a picture, the picture's index, or how far ahead the line of CMD_LINE_AT is
    u8 arg;
    u8 color;
    u8 font;
    // How the line is aligned, or the screen of a picture
    u16 align;
    // The x of the line, or the frames a picture fades over
    s16 x;
    s16 offset;
} StaffRollLine;

typedef struct {
    GFLBitmap *bitmap;
    u32 line;
} StaffRollLineBitmap;

typedef struct {
    u8 state;
    u8 screens;
    // Which eighth of the characters is loaded next
    u8 part;
    u8 index;
    s32 frame;
    s32 fadeFrames;
    s32 wait;
    // The last picture, which fades the brightness and shows the actor
    BOOL isLast;
    NNSG2dCharacterData *charData;
    NNSG2dScreenData *screenData;
    u32 unk1C;
    void *charFile;
    void *screenFile;
} StaffRollPicture;

typedef struct {
    ClActUnit *unit;
    ClActor *actor;
    u32 palette;
    u32 chars;
    u32 cellAnims;
} StaffRollClAct;

typedef struct {
    StaffRollParam *param;
    ArcTool *arc;
    TCB *vblankTask;
    Font *fonts[2];
    MsgData *msgData;
    StrBuf *strbuf;
    StaffRollLineBitmap lineBitmaps[LINE_BITMAP_MAX];
    u16 lineBitmapCount;
    u8 drawnLineBitmaps;
    u8 shownLineBitmaps;
    BmpWin *topWindow;
    u32 unk224;
    BmpWin *bottomWindow;
    u32 unk22C;
    // The windows whose characters are copied at the next VBlank, SCREEN_*
    u8 flushScreens;
    u8 unk231;
    u8 fadeState;
    u8 fadeScreens;
    s8 fadeAlpha;
    u16 logoFrame;
    u8 unk238;
    // Runs the credits four times faster, which nothing sets
    u8 fastForward;
    StaffRollLine *lines;
    u16 lineIndex;
    s16 waitFrames;
    fx32 scrollFraction;
    BOOL scrolling;
    u32 frames;
    u32 seq;
    u32 state;
    u32 nextSeq;
    StaffRollPicture picture;
    StaffRollClAct clact;
} StaffRollWork;

typedef u32 (*StaffRollSeqFunc)(StaffRollWork *work);
typedef BOOL (*StaffRollCmdFunc)(StaffRollWork *work, StaffRollLine *line);

static BOOL StaffRoll_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL StaffRoll_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL StaffRoll_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL StaffRoll_Update(StaffRollWork *work);
static u32 StaffRollSeq_Init(StaffRollWork *work);
static u32 StaffRollSeq_WaitWipe(StaffRollWork *work);
static u32 StaffRollSeq_Exit(StaffRollWork *work);
static u32 StaffRollSeq_Logo(StaffRollWork *work);
static u32 StaffRollSeq_Roll(StaffRollWork *work);
static u32 StaffRollSeq_Copyright(StaffRollWork *work);
static void StaffRoll_InitVRAM(void);
static inline void StaffRoll_InitTopPictureBG(void);
static inline void StaffRoll_InitBottomPictureBG(void);
static void StaffRoll_InitBG(void);
static inline void StaffRoll_InitBottomWindowBG(void);
static void StaffRoll_FreeBG(void);
static void StaffRoll_LoadGraphics(ArcTool *arc);
static void StaffRoll_InitClAct(ArcTool *arc, StaffRollClAct *clact);
static void StaffRoll_FreeClAct(StaffRollClAct *clact);
static void StaffRollPicture_Init(StaffRollPicture *picture);
static void StaffRollPicture_Free(StaffRollPicture *picture);
static void StaffRoll_VBlank(TCB *tcb, void *data);
static void StaffRoll_InitVBlank(StaffRollWork *work);
static void StaffRoll_FreeVBlank(StaffRollWork *work);
static void StaffRoll_InitMsg(StaffRollWork *work);
static void StaffRoll_FreeMsg(StaffRollWork *work);
static void StaffRoll_CreateLineBitmaps(StaffRollWork *work);
static void StaffRoll_FreeLineBitmaps(StaffRollWork *work);
static void StaffRoll_DrawLineBitmap(StaffRollWork *work, GFLBitmap *bitmap, u32 index);
static void StaffRoll_DrawNextLineBitmap(StaffRollWork *work);
static void StaffRoll_ShowLineBitmap(StaffRollWork *work);
static void StaffRoll_LoadLines(StaffRollWork *work);
static void StaffRoll_FreeLines(StaffRollWork *work);
static void StaffRoll_InitWindows(StaffRollWork *work);
static void StaffRoll_FreeWindows(StaffRollWork *work);
static void StaffRoll_RequestFlush(StaffRollWork *work, u32 screens);
static void StaffRoll_Flush(StaffRollWork *work);
static u32 StaffRoll_StartWipeIn(StaffRollWork *work, u32 nextSeq);
static u32 StaffRoll_StartWipeOut(StaffRollWork *work, u32 nextSeq);
static void StaffRoll_ScrollTile(u32 *tile, u32 *above, u32 rows);
static void StaffRoll_Scroll(StaffRollWork *work);
static void StaffRoll_DrawLine(StaffRollWork *work, StaffRollLine *line, GFLBitmap *bitmap, int y);
static void StaffRoll_DrawLineBelow(StaffRollWork *work, StaffRollLine *line);
static void StaffRoll_DrawTitle(StaffRollWork *work, StaffRollLine *line);
static BOOL StaffRoll_FadeIn(StaffRollWork *work);
static BOOL StaffRoll_FadeOut(StaffRollWork *work);
static BOOL StaffRoll_UpdateLogo(StaffRollWork *work);
static void StaffRollPicture_Update(StaffRollWork *work, StaffRollPicture *picture);
static BOOL StaffRollPicture_Fade(StaffRollPicture *picture, BOOL out, u8 fastForward);
static u32 StaffRollPicture_GetFileID(StaffRollPicture *picture, u32 kind, u32 gender);
static BOOL StaffRollCmd_Line(StaffRollWork *work, StaffRollLine *line);
static BOOL StaffRollCmd_FadeIn(StaffRollWork *work, StaffRollLine *line);
static BOOL StaffRollCmd_FadeOut(StaffRollWork *work, StaffRollLine *line);
static BOOL StaffRollCmd_ScrollStart(StaffRollWork *work, StaffRollLine *line);
static BOOL StaffRollCmd_ScrollStop(StaffRollWork *work, StaffRollLine *line);
static BOOL StaffRollCmd_End(StaffRollWork *work, StaffRollLine *line);
static BOOL StaffRollCmd_Yield(StaffRollWork *work, StaffRollLine *line);
static BOOL StaffRollCmd_LineAt(StaffRollWork *work, StaffRollLine *line);
static BOOL StaffRollCmd_Picture(StaffRollWork *work, StaffRollLine *line);
static BOOL StaffRollCmd_LastPicture(StaffRollWork *work, StaffRollLine *line);

const GameProcFunctions STAFF_ROLL_PROC_FUNCTIONS = { StaffRoll_Init, StaffRoll_Main, StaffRoll_Exit };

static const u16 sLineColors[8] = {
#ifdef BLACK2
    PRINT_COLOR(1, 2, 0),  PRINT_COLOR(3, 4, 0),   PRINT_COLOR(5, 6, 0),   PRINT_COLOR(7, 8, 0),
#else
    PRINT_COLOR(5, 6, 0),  PRINT_COLOR(7, 8, 0),   PRINT_COLOR(5, 6, 0),   PRINT_COLOR(7, 8, 0),
#endif
    PRINT_COLOR(9, 10, 0), PRINT_COLOR(11, 12, 0), PRINT_COLOR(13, 14, 0), 0,
};

static const StaffRollSeqFunc sStaffRollSeqFuncs[] = {
    StaffRollSeq_Init, StaffRollSeq_WaitWipe, StaffRollSeq_Exit,
    StaffRollSeq_Logo, StaffRollSeq_Roll,     StaffRollSeq_Copyright,
};

static const StaffRollCmdFunc sCmdFuncs[] = {
    StaffRollCmd_Line, StaffRollCmd_FadeIn, StaffRollCmd_FadeOut, StaffRollCmd_ScrollStart, StaffRollCmd_ScrollStop,
    StaffRollCmd_End,  StaffRollCmd_Yield,  StaffRollCmd_LineAt,  StaffRollCmd_Picture,     StaffRollCmd_LastPicture,
};

static const BGSysVRAMConfig sStaffRollVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_0123_E, GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_0123_H,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE,  GX_VRAM_SUB_OBJ_128_D,      GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,     GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static BOOL StaffRoll_Init(GameProc *proc, u32 *state, void *param, void *work) {
    StaffRollWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_STAFF_ROLL, 0x125000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(StaffRollWork), HEAPID_STAFF_ROLL);
    sys_memset(wk, 0, sizeof(StaffRollWork));
    wk->param = param;
    return TRUE;
}

static BOOL StaffRoll_Main(GameProc *proc, u32 *state, void *param, void *work) {
    if (!StaffRoll_Update(work)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL StaffRoll_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_STAFF_ROLL);
    return TRUE;
}

static BOOL StaffRoll_Update(StaffRollWork *work) {
    work->seq = sStaffRollSeqFuncs[work->seq](work);
    if (work->seq == SEQ_END) {
        return FALSE;
    }
    StaffRoll_DrawNextLineBitmap(work);
    return TRUE;
}

static u32 StaffRollSeq_Init(StaffRollWork *work) {
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_MAIN_SUB);
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, FADE_BRIGHTNESS(16));
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, FADE_BRIGHTNESS(16));
    work->arc = GFL_ArcSysCreateFileHandle(ARCID_THE_END, HEAPID_STAFF_ROLL);
    StaffRoll_InitVRAM();
    StaffRoll_InitBG();
    StaffRoll_InitClAct(work->arc, &work->clact);
    StaffRollPicture_Init(&work->picture);
    StaffRoll_LoadGraphics(work->arc);
    StaffRoll_InitMsg(work);
    StaffRoll_InitWindows(work);
    StaffRoll_LoadLines(work);
    StaffRoll_InitVBlank(work);
    func_020051d8(TRUE);
    return SEQ_LOGO;
}

static u32 StaffRollSeq_WaitWipe(StaffRollWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        return work->nextSeq;
    }
    return SEQ_WAIT_WIPE;
}

static u32 StaffRollSeq_Exit(StaffRollWork *work) {
    func_020051d8(FALSE);
    StaffRoll_FreeVBlank(work);
    StaffRoll_FreeLines(work);
    if (work->arc != NULL) {
        GFL_ArcToolFree(work->arc);
        work->arc = NULL;
    }
    StaffRoll_FreeWindows(work);
    StaffRoll_FreeMsg(work);
    StaffRollPicture_Free(&work->picture);
    StaffRoll_FreeClAct(&work->clact);
    StaffRoll_FreeBG();
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, FADE_BRIGHTNESS(16));
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, FADE_BRIGHTNESS(16));
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return SEQ_END;
}

// Waits, fades the logo in and out, and waits again
static u32 StaffRollSeq_Logo(StaffRollWork *work) {
    switch (work->state) {
    case 0:
        if (work->frames >= 20) {
            work->frames = 0;
            work->state++;
        } else if (work->fastForward == TRUE) {
            work->frames += 4;
        } else {
            work->frames++;
        }
        break;
    case 1:
        if (!StaffRoll_UpdateLogo(work)) {
            work->state++;
        }
        break;
    case 2:
        if (work->frames >= 130) {
            work->frames = 0;
            work->state = 0;
            return SEQ_ROLL;
        }
        if (work->fastForward == TRUE) {
            work->frames += 4;
        } else {
            work->frames++;
        }
        break;
    }
    return SEQ_LOGO;
}

static u32 StaffRollSeq_Roll(StaffRollWork *work) {
    StaffRollLine *line;

    if (work->param->skippable == TRUE && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START)) {
        return StaffRoll_StartWipeOut(work, SEQ_EXIT);
    }
    if (work->state == ROLL_READ) {
        while (TRUE) {
            line = &work->lines[work->lineIndex];
            if (work->lineIndex == work->lineBitmaps[work->shownLineBitmaps].line) {
                StaffRoll_ShowLineBitmap(work);
                work->shownLineBitmaps++;
                work->waitFrames = line->wait;
                work->state = ROLL_WAIT;
                work->lineIndex += 2;
                break;
            }
            if (sCmdFuncs[line->command](work, line) == TRUE) {
                work->lineIndex++;
                break;
            }
            work->lineIndex++;
        }
    }
    switch (work->state) {
    case ROLL_WAIT:
        if (work->waitFrames <= 0) {
            work->waitFrames = 0;
            work->state = ROLL_READ;
        } else if (work->fastForward == TRUE) {
            work->waitFrames -= 4;
        } else {
            work->waitFrames--;
        }
        break;
    case ROLL_FADE_IN:
        if (!StaffRoll_FadeIn(work)) {
            work->state = ROLL_READ;
        }
        break;
    case ROLL_FADE_OUT:
        if (!StaffRoll_FadeOut(work)) {
            work->state = ROLL_READ;
        }
        break;
    case ROLL_END:
        work->frames = 0;
        work->state = ROLL_READ;
        return StaffRoll_StartWipeOut(work, SEQ_COPYRIGHT);
    }
    StaffRoll_Scroll(work);
    if (work->param->skippable != TRUE) {
        StaffRollPicture_Update(work, &work->picture);
    }
    func_0204b794();
    return SEQ_ROLL;
}

// Shows the two copyright screens
static u32 StaffRollSeq_Copyright(StaffRollWork *work) {
    ArcTool *arc;

    switch (work->state) {
    case 0:
        arc = GFL_ArcSysCreateFileHandle(ARCID_COPYRIGHT, HEAPID_TAIL(HEAPID_STAFF_ROLL));
        GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 0, HEAPID_STAFF_ROLL);
        GFL_BGSysLoadArcNCGRStatic(arc, 7, 1, 0, 0x8000, FALSE, HEAPID_STAFF_ROLL);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 8, 1, 0, 0, FALSE, HEAPID_STAFF_ROLL);
        GFL_ArcToolFree(arc);
        GFL_BGSysSetScrPaletteNo(1, 0, 0, 32, 24, COPYRIGHT_PALETTE);
        GFL_BGSysResetStdPalette(1, BACKGROUND_COLOR);
        GFL_BGSysLoadScr(1);
        work->state = 1;
        return StaffRoll_StartWipeIn(work, SEQ_COPYRIGHT);
    case 1:
        if (work->frames >= 200) {
            work->state = 2;
            work->frames = 0;
            return StaffRoll_StartWipeOut(work, SEQ_COPYRIGHT);
        }
        if (work->fastForward == TRUE) {
            work->frames += 4;
        } else {
            work->frames++;
        }
        break;
    case 2:
        arc = GFL_ArcSysCreateFileHandle(ARCID_COPYRIGHT, HEAPID_TAIL(HEAPID_STAFF_ROLL));
        GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 0, HEAPID_STAFF_ROLL);
        GFL_BGSysLoadArcNCGRStatic(arc, 1, 1, 0, 0x8000, FALSE, HEAPID_STAFF_ROLL);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 2, 1, 0, 0, FALSE, HEAPID_STAFF_ROLL);
        GFL_ArcToolFree(arc);
        GFL_BGSysSetScrPaletteNo(1, 0, 0, 32, 24, COPYRIGHT_PALETTE);
        GFL_BGSysResetStdPalette(1, BACKGROUND_COLOR);
        GFL_BGSysLoadScr(1);
        work->state = 3;
        return StaffRoll_StartWipeIn(work, SEQ_COPYRIGHT);
    case 3:
        if (work->frames >= 200) {
            work->frames = 0;
            work->state = 0;
            return StaffRoll_StartWipeOut(work, SEQ_EXIT);
        }
        if (work->fastForward == TRUE) {
            work->frames += 4;
        } else {
            work->frames++;
        }
        break;
    }
    return SEQ_COPYRIGHT;
}

static void StaffRoll_InitVRAM(void) {
    GFL_BGSysInitVRAM(0);
    GFL_BGSysSetVRAMBanks(&sStaffRollVRAMConfig);
}

// The picture BGs and the bottom window's BG are set up through inlines. Which BGs have one, and that the last is
// defined after StaffRoll_InitBG, is a fit: it gives the ROM's order of the equal-size setups in .rodata
static inline void StaffRoll_InitTopPictureBG(void) {
    BGSetup setup = { 0,
                      0,
                      0x1000,
                      0,
                      BGRES_256x256,
                      GX_BG_COLORMODE_256,
                      GX_BG_SCRBASE(0xc000),
                      GX_BG_CHARBASE(0x10000),
                      0x8000,
                      GX_BG_EXTPLTT_01,
                      2,
                      GX_BG_AREAOVER_XLU,
                      FALSE };

    GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
}

static inline void StaffRoll_InitBottomPictureBG(void) {
    BGSetup setup = { 0,
                      0,
                      0x1000,
                      0,
                      BGRES_256x256,
                      GX_BG_COLORMODE_256,
                      GX_BG_SCRBASE(0xc000),
                      GX_BG_CHARBASE(0x10000),
                      0x8000,
                      GX_BG_EXTPLTT_23,
                      2,
                      GX_BG_AREAOVER_XLU,
                      FALSE };

    GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
}

static void StaffRoll_InitBG(void) {
    GFL_BGSysCreate(HEAPID_STAFF_ROLL);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_3, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&lcdConfig);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0x0000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_256,
                          GX_BG_SCRBASE(0xf000),
                          GX_BG_CHARBASE(0x10000),
                          0x10000,
                          GX_BG_EXTPLTT_01,
                          1,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
        MtxFx22 mtx;

        GFL_BGSysCreateBG(3, &setup, BGMODE_EXTENDED);
        MAT2_SetScaleRot(&mtx, 0, FX32_ONE, FX32_ONE, 0);
        GFL_BGSysSetBGTransform(3, &mtx, 128, 96);
    }
    StaffRoll_InitTopPictureBG();
    StaffRoll_InitBottomWindowBG();
    StaffRoll_InitBottomPictureBG();
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, FALSE);
}

static inline void StaffRoll_InitBottomWindowBG(void) {
    BGSetup setup = { 0,
                      0,
                      0x800,
                      0,
                      BGRES_256x256,
                      GX_BG_COLORMODE_16,
                      GX_BG_SCRBASE(0xf800),
                      GX_BG_CHARBASE(0x0000),
                      0x8000,
                      GX_BG_EXTPLTT_01,
                      0,
                      GX_BG_AREAOVER_XLU,
                      FALSE };

    GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
}

static void StaffRoll_FreeBG(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, FALSE);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysFree();
}

// Loads the logo and the palettes of the text
static void StaffRoll_LoadGraphics(ArcTool *arc) {
    ArcTool *title = GFL_ArcSysCreateFileHandle(ARCID_TITLE, HEAPID_TAIL(HEAPID_STAFF_ROLL));

    GFL_G2DIOLoadArcNCLRDefault(title, 2, PALTYPE_MAIN_BG_EX, 0x6000, 0, HEAPID_STAFF_ROLL);
    GFL_BGSysLoadArcNCGRStatic(title, 0, 3, 0, 0, TRUE, HEAPID_STAFF_ROLL);
    loadBGScrToVramByFileNoReserveNegAlign(title, 1, 3, 0, 0, TRUE, HEAPID_STAFF_ROLL);
    GFL_ArcToolFree(title);
    GFL_G2DIOLoadArcNCLRDefault(arc, 20, PALTYPE_MAIN_BG, 0x1e0, 0x20, HEAPID_STAFF_ROLL);
    GFL_G2DIOLoadArcNCLRDefault(arc, 20, PALTYPE_SUB_BG, 0x1e0, 0x20, HEAPID_STAFF_ROLL);
    GFL_BGSysResetStdPalette(1, BACKGROUND_COLOR);
    GFL_BGSysResetStdPalette(5, BACKGROUND_COLOR);
}

// Creates the actor shown with the last picture
static void StaffRoll_InitClAct(ArcTool *arc, StaffRollClAct *clact) {
    ClActorSetup setup;

    ClActSys_Create(&data_02093f08, &sStaffRollVRAMConfig, HEAPID_STAFF_ROLL);
    clact->unit = func_0204bf1c(1, 0, HEAPID_TAIL(HEAPID_STAFF_ROLL));
    func_0204c028(clact->unit);
    clact->chars = func_0204b81c(arc, ACTOR_CHAR_FILE, FALSE, 0, HEAPID_TAIL(HEAPID_STAFF_ROLL));
    clact->palette = func_0204bba0(arc, ACTOR_PALETTE_FILE, 0, 0, HEAPID_TAIL(HEAPID_STAFF_ROLL));
    clact->cellAnims = func_0204bde0(arc, ACTOR_CELL_FILE, ACTOR_ANIM_FILE, HEAPID_TAIL(HEAPID_STAFF_ROLL));
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 0;
    setup.bgPriority = 1;
    setup.priority = 0;
    clact->actor = func_0204c040(clact->unit, clact->chars, clact->palette, clact->cellAnims, &setup, 0,
                                 HEAPID_TAIL(HEAPID_STAFF_ROLL));
    func_0204c520(clact->actor, TRUE);
    func_0204c124(clact->actor, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
}

static void StaffRoll_FreeClAct(StaffRollClAct *clact) {
    func_0204b98c(clact->chars);
    func_0204bcd0(clact->palette);
    func_0204be64(clact->cellAnims);
    func_0204bf98(clact->unit);
    func_0204b758();
}

// Allocates the buffers of the pictures' characters and screens, as large as the largest of them
static void StaffRollPicture_Init(StaffRollPicture *picture) {
    u32 charSize = GFL_ArcSysGetDataLength(ARCID_THE_END, 22);
    u32 screenSize = GFL_ArcSysGetDataLength(ARCID_THE_END, 23);

    picture->charFile = GFL_HeapAllocate(HEAPID_STAFF_ROLL, charSize, FALSE, "staff_roll_main.c", 1074);
    picture->screenFile = GFL_HeapAllocate(HEAPID_STAFF_ROLL, screenSize, FALSE, "staff_roll_main.c", 1075);
}

static void StaffRollPicture_Free(StaffRollPicture *picture) {
    GFL_HeapFree(picture->charFile);
    GFL_HeapFree(picture->screenFile);
}

static void StaffRoll_VBlank(TCB *tcb, void *data) {
    StaffRoll_Flush(data);
    GFL_BGSysUpdate();
    func_0204b7c8();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

static void StaffRoll_InitVBlank(StaffRollWork *work) {
    work->vblankTask = GFL_VBlankTCBAdd(StaffRoll_VBlank, work, 0);
}

static void StaffRoll_FreeVBlank(StaffRollWork *work) {
    GFL_TCBRemove(work->vblankTask);
}

static void StaffRoll_InitMsg(StaffRollWork *work) {
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_15, HEAPID_STAFF_ROLL);
    work->strbuf = GFL_StrBufCreate(128, HEAPID_STAFF_ROLL);
    work->fonts[0] = GFL_FontCreate(ARCID_FONT, 0, 1, FALSE, HEAPID_STAFF_ROLL);
    work->fonts[1] = GFL_FontCreate(ARCID_FONT, 2, 1, FALSE, HEAPID_STAFF_ROLL);
}

static void StaffRoll_FreeMsg(StaffRollWork *work) {
    GFL_FontFree(work->fonts[1]);
    GFL_FontFree(work->fonts[0]);
    GFL_StrBufFree(work->strbuf);
    GFL_MsgDataFree(work->msgData);
}

// Creates a bitmap for each pair of lines that scroll together, a name line followed by one that waits for nothing
static void StaffRoll_CreateLineBitmaps(StaffRollWork *work) {
    u16 i;
    StaffRollLine *line;
    StaffRollLine *next;

    for (i = 0;; i++) {
        line = &work->lines[i];
        if (line->command == CMD_END) {
            return;
        }
        if (line->command == CMD_LINE && line->msgId != 0xffff) {
            next = &work->lines[i + 1];
            if (next->wait == 0 && next->command == CMD_LINE && next->msgId != 0xffff) {
                work->lineBitmaps[work->lineBitmapCount].bitmap = GFL_BitmapCreate(32, 2, 0x20, HEAPID_STAFF_ROLL);
                work->lineBitmaps[work->lineBitmapCount].line = i;
                work->lineBitmapCount++;
                if (work->lineBitmapCount == LINE_BITMAP_MAX) {
                    return;
                }
                i++;
            }
        }
    }
}

static void StaffRoll_FreeLineBitmaps(StaffRollWork *work) {
    u32 i;

    for (i = 0; i < work->lineBitmapCount; i++) {
        GFL_BitmapFree(work->lineBitmaps[i].bitmap);
    }
}

static void StaffRoll_DrawLineBitmap(StaffRollWork *work, GFLBitmap *bitmap, u32 index) {
    StaffRollLine *line = &work->lines[index];

    if (line->font == 0) {
        StaffRoll_DrawLine(work, line, bitmap, 0);
    } else {
        StaffRoll_DrawLine(work, line, bitmap, 2);
    }
}

// Draws one pair of lines a frame, ahead of their turn
static void StaffRoll_DrawNextLineBitmap(StaffRollWork *work) {
    if (work->drawnLineBitmaps < work->lineBitmapCount) {
        StaffRoll_DrawLineBitmap(work, work->lineBitmaps[work->drawnLineBitmaps].bitmap,
                                 work->lineBitmaps[work->drawnLineBitmaps].line);
        StaffRoll_DrawLineBitmap(work, work->lineBitmaps[work->drawnLineBitmaps].bitmap,
                                 work->lineBitmaps[work->drawnLineBitmaps].line + 1);
        work->drawnLineBitmaps++;
    }
}

// Copies the next pair of lines below the bottom screen, from where it scrolls up
static void StaffRoll_ShowLineBitmap(StaffRollWork *work) {
    GFL_BitmapCopyArea(work->lineBitmaps[work->shownLineBitmaps].bitmap, BmpWin_GetBitmap(work->bottomWindow), 0, 0, 0,
                       192, 256, 16, 0);
}

static void StaffRoll_LoadLines(StaffRollWork *work) {
    work->lines = GFL_ArcSysReadHeapNew(ARCID_THE_END, 1, HEAPID_STAFF_ROLL);
    StaffRoll_CreateLineBitmaps(work);
}

static void StaffRoll_FreeLines(StaffRollWork *work) {
    StaffRoll_FreeLineBitmaps(work);
    GFL_HeapFree(work->lines);
}

static void StaffRoll_InitWindows(StaffRollWork *work) {
    BmpWin_InitAllocator(HEAPID_STAFF_ROLL);
    work->topWindow = BmpWin_CreateDynamic(1, 0, 0, 32, 25, 15, TRUE);
    work->bottomWindow = BmpWin_CreateDynamic(5, 0, 0, 32, 28, 15, TRUE);
    BmpWin_TransferNow(work->topWindow);
    BmpWin_TransferNow(work->bottomWindow);
}

static void StaffRoll_FreeWindows(StaffRollWork *work) {
    BmpWin_Free(work->topWindow);
    BmpWin_Free(work->bottomWindow);
    BmpWin_FreeAllocator();
}

static void StaffRoll_RequestFlush(StaffRollWork *work, u32 screens) {
    work->flushScreens |= screens;
}

static void StaffRoll_Flush(StaffRollWork *work) {
    if (work->flushScreens & SCREEN_TOP) {
        BmpWin_FlushChar(work->topWindow);
    }
    if (work->flushScreens & SCREEN_BOTTOM) {
        BmpWin_FlushChar(work->bottomWindow);
    }
    work->flushScreens = 0;
}

static u32 StaffRoll_StartWipeIn(StaffRollWork *work, u32 nextSeq) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR, 6, 1, HEAPID_STAFF_ROLL);
    work->nextSeq = nextSeq;
    return SEQ_WAIT_WIPE;
}

static u32 StaffRoll_StartWipeOut(StaffRollWork *work, u32 nextSeq) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR, 6, 1, HEAPID_STAFF_ROLL);
    work->nextSeq = nextSeq;
    return SEQ_WAIT_WIPE;
}

// Moves the rows of a 16 color tile up, the top rows into the bottom of the tile above
static void StaffRoll_ScrollTile(u32 *tile, u32 *above, u32 rows) {
    u32 i;

    if (above != NULL) {
        for (i = 0; i < rows; i++) {
            above[8 - rows + i] = tile[i];
        }
    }
    for (i = 0; i < 8 - rows; i++) {
        tile[i] = tile[rows + i];
    }
}

// Scrolls the text up, from the bottom window into the top one
static void StaffRoll_Scroll(StaffRollWork *work) {
    u8 *top;
    u8 *bottom;
    u32 rows;
    u32 offset;
    u32 aboveOffset;
    u16 x;
    u16 y;

    if (work->scrolling) {
        work->scrollFraction += FX32_ONE;
        if (work->scrollFraction >= FX32_ONE) {
            work->scrollFraction -= FX32_ONE;
            rows = 1;
            if (work->fastForward) {
                rows = 4;
            }
            top = GFL_BitmapGetPixelData(BmpWin_GetBitmap(work->topWindow));
            bottom = GFL_BitmapGetPixelData(BmpWin_GetBitmap(work->bottomWindow));
            offset = 0;
            for (x = 0; x < 32; x++) {
                StaffRoll_ScrollTile((u32 *)(top + offset), NULL, rows);
                offset += 0x20;
            }
            for (y = 1; y < 25; y++) {
                offset = y * 0x400;
                aboveOffset = (y - 1) * 0x400;
                for (x = 0; x < 32; x++) {
                    StaffRoll_ScrollTile((u32 *)(top + offset), (u32 *)(top + aboveOffset), rows);
                    offset += 0x20;
                    aboveOffset += 0x20;
                }
            }
            offset = 0;
            aboveOffset = 24 * 0x400;
            for (x = 0; x < 32; x++) {
                StaffRoll_ScrollTile((u32 *)(bottom + offset), (u32 *)(top + aboveOffset), rows);
                offset += 0x20;
                aboveOffset += 0x20;
            }
            for (y = 1; y < 28; y++) {
                offset = y * 0x400;
                aboveOffset = (y - 1) * 0x400;
                for (x = 0; x < 32; x++) {
                    StaffRoll_ScrollTile((u32 *)(bottom + offset), (u32 *)(bottom + aboveOffset), rows);
                    offset += 0x20;
                    aboveOffset += 0x20;
                }
            }
            StaffRoll_RequestFlush(work, SCREEN_TOP | SCREEN_BOTTOM);
        }
    }
}

static void StaffRoll_DrawLine(StaffRollWork *work, StaffRollLine *line, GFLBitmap *bitmap, int y) {
    int x;

    GFL_MsgDataLoadStrbuf(work->msgData, line->msgId, work->strbuf);
    if (line->align == ALIGN_LEFT) {
        x = line->x + line->offset;
    } else if (line->align == ALIGN_RIGHT) {
        u32 width = GFL_FontGetBlockWidth(work->strbuf, work->fonts[line->font], 0);

        x = line->x + line->offset - width;
    } else if (line->align == ALIGN_CENTER) {
        // Any other alignment leaves x unset; the original tests the third value too (an else is 8 bytes shorter)
        u32 width = GFL_FontGetBlockWidth(work->strbuf, work->fonts[line->font], 0);

        x = line->x + line->offset - width / 2;
    }
    GFL_TextRendererDrawToBitmapEx(bitmap, x, y, work->strbuf, work->fonts[line->font], sLineColors[line->color]);
}

// Draws a line below the bottom screen
static void StaffRoll_DrawLineBelow(StaffRollWork *work, StaffRollLine *line) {
    if (line->font == 0) {
        StaffRoll_DrawLine(work, line, BmpWin_GetBitmap(work->bottomWindow), 192);
    } else {
        StaffRoll_DrawLine(work, line, BmpWin_GetBitmap(work->bottomWindow), 194);
    }
}

// Draws a title at its y, from the top of the top screen to the bottom of the bottom one
static void StaffRoll_DrawTitle(StaffRollWork *work, StaffRollLine *line) {
    GFLBitmap *bitmap;
    int y;

    if (line->wait < 256) {
        bitmap = BmpWin_GetBitmap(work->topWindow);
        y = line->wait;
        work->fadeScreens |= SCREEN_TOP;
    } else {
        bitmap = BmpWin_GetBitmap(work->bottomWindow);
        y = line->wait - 256;
        work->fadeScreens |= SCREEN_BOTTOM;
    }
    StaffRoll_DrawLine(work, line, bitmap, y);
}

// Fades the titles in, and returns FALSE once done
static BOOL StaffRoll_FadeIn(StaffRollWork *work) {
    int eva, evb;

    switch (work->fadeState) {
    case 0:
        if (work->fadeScreens & SCREEN_TOP) {
            gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BD,
                                0, 16);
        }
        if (work->fadeScreens & SCREEN_BOTTOM) {
            gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1,
                                GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BD, 0, 16);
        }
        work->flushScreens = work->fadeScreens;
        work->fadeState++;
        break;
    case 1:
        if (work->fastForward == TRUE) {
            work->fadeAlpha += 16;
        } else {
            work->fadeAlpha += 4;
        }
        if (work->fadeAlpha > 16) {
            eva = 16;
            evb = 0;
        } else {
            evb = 16 - work->fadeAlpha;
            eva = work->fadeAlpha;
        }
        if (work->fadeScreens & SCREEN_TOP) {
            G2_ChangeBlendAlpha(eva, evb);
        }
        if (work->fadeScreens & SCREEN_BOTTOM) {
            G2S_ChangeBlendAlpha(eva, evb);
        }
        if (work->fadeAlpha >= 16) {
            if (work->fadeScreens & SCREEN_TOP) {
                G2_BlendNone();
            }
            if (work->fadeScreens & SCREEN_BOTTOM) {
                G2S_BlendNone();
            }
            work->fadeAlpha = 0;
            work->fadeState = 0;
            return FALSE;
        }
        break;
    }
    return TRUE;
}

// Fades the titles out and clears them, and returns FALSE once done
static BOOL StaffRoll_FadeOut(StaffRollWork *work) {
    int eva, evb;

    switch (work->fadeState) {
    case 0:
        if (work->fadeScreens & SCREEN_TOP) {
            gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BD,
                                16, 0);
        }
        if (work->fadeScreens & SCREEN_BOTTOM) {
            gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1,
                                GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BD, 16, 0);
        }
        work->fadeState++;
    case 1:
        if (work->fastForward == TRUE) {
            work->fadeAlpha += 16;
        } else {
            work->fadeAlpha += 4;
        }
        if (work->fadeAlpha > 16) {
            eva = 0;
            evb = 16;
        } else {
            eva = 16 - work->fadeAlpha;
            evb = work->fadeAlpha;
        }
        if (work->fadeScreens & SCREEN_TOP) {
            G2_ChangeBlendAlpha(eva, evb);
        }
        if (work->fadeScreens & SCREEN_BOTTOM) {
            G2S_ChangeBlendAlpha(eva, evb);
        }
        if (work->fadeAlpha >= 16) {
            if (work->fadeScreens & SCREEN_TOP) {
                GFL_BitmapFill(BmpWin_GetBitmap(work->topWindow), 0);
            }
            if (work->fadeScreens & SCREEN_BOTTOM) {
                GFL_BitmapFill(BmpWin_GetBitmap(work->bottomWindow), 0);
            }
            work->flushScreens = work->fadeScreens;
            work->fadeAlpha = 0;
            work->fadeState++;
        }
        break;
    case 2:
        if (work->fadeScreens & SCREEN_TOP) {
            G2_BlendNone();
        }
        if (work->fadeScreens & SCREEN_BOTTOM) {
            G2S_BlendNone();
        }
        work->fadeScreens = 0;
        work->fadeState = 0;
        return FALSE;
    }
    return TRUE;
}

// Fades the logo in, waits and fades it out, and returns FALSE once done
static BOOL StaffRoll_UpdateLogo(StaffRollWork *work) {
    int alpha;
    s16 eva, evb;

    switch (work->fadeState) {
    case 0:
        gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG3, GX_BLEND_PLANEMASK_BD, 0, 16);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, TRUE);
        GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, 0);
        GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, 0);
        work->logoFrame = 0;
        work->fadeState++;
        break;
    case 1:
        if (work->fastForward == TRUE) {
            alpha = work->logoFrame * 16 / 34;
            eva = alpha;
            evb = 16 - alpha;
        } else {
            alpha = work->logoFrame * 16 / 138;
            eva = alpha;
            evb = 16 - alpha;
        }
        work->logoFrame++;
        if (eva >= 16) {
            G2_ChangeBlendAlpha(16, 0);
            work->logoFrame = 0;
            work->waitFrames = 200;
            work->fadeState++;
        } else {
            G2_ChangeBlendAlpha(eva, evb);
        }
        break;
    case 2:
        if (work->fastForward == TRUE) {
            work->waitFrames -= 4;
        } else {
            work->waitFrames--;
        }
        if (work->waitFrames <= 0) {
            work->waitFrames = 0;
            work->fadeState++;
        }
        break;
    case 3:
        if (work->fastForward == TRUE) {
            alpha = work->logoFrame * 16 / 34;
            eva = 16 - alpha;
            evb = alpha;
        } else {
            alpha = work->logoFrame * 16 / 138;
            eva = 16 - alpha;
            evb = alpha;
        }
        work->logoFrame++;
        if (evb >= 16) {
            G2_BlendNone();
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, FALSE);
            work->logoFrame = 0;
            work->fadeState = 0;
            return FALSE;
        }
        G2_ChangeBlendAlpha(eva, evb);
        break;
    }
    return TRUE;
}

static void StaffRollPicture_Update(StaffRollWork *work, StaffRollPicture *picture) {
    ArcTool *arc = work->arc;
    u32 gender = getTrainerGender(work->param->playerInfo);
    u32 fileId;
    u32 size;

    switch (picture->state) {
    case PICTURE_NONE:
        return;
    case PICTURE_START:
        if (picture->screens & SCREEN_TOP) {
            gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0,
                                GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |
                                    GX_BLEND_PLANEMASK_BD,
                                0, 16);
        }
        if (picture->screens & SCREEN_BOTTOM) {
            gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0,
                                GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |
                                    GX_BLEND_PLANEMASK_BD,
                                0, 16);
        }
        picture->state = PICTURE_LOAD_CHAR;
        return;
    case PICTURE_LAST_START:
        gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_OBJ, FADE_BRIGHTNESS(16));
        func_0204c124(work->clact.actor, TRUE);
        func_0204c56c(work->clact.actor);
        picture->state = PICTURE_LOAD_CHAR;
        return;
    case PICTURE_LOAD_CHAR:
        // An eighth of the characters a frame
        fileId = StaffRollPicture_GetFileID(picture, PICTURE_FILE_CHAR, gender);
        size = GFL_ArcSysGetDataLength(ARCID_THE_END, fileId) / 8;
        GFL_ArcToolReadRange(work->arc, fileId, size * picture->part, size,
                             (u8 *)picture->charFile + size * picture->part);
        picture->part++;
        if (picture->part >= 8) {
            NNS_G2dGetUnpackedBGCharacterData(picture->charFile, &picture->charData);
            picture->state = PICTURE_LOAD_SCREEN;
            return;
        }
        break;
    case PICTURE_LOAD_SCREEN:
        GFL_ArcToolRead(arc, StaffRollPicture_GetFileID(picture, PICTURE_FILE_SCREEN, gender), picture->screenFile);
        NNS_G2dGetUnpackedScreenData(picture->screenFile, &picture->screenData);
        picture->state = PICTURE_SHOW;
        return;
    case PICTURE_SHOW:
        fileId = StaffRollPicture_GetFileID(picture, PICTURE_FILE_PALETTE, gender);
        if (picture->screens & SCREEN_TOP) {
            GFL_BGSysLoadScrCore(0, picture->screenData->rawData, picture->screenData->size, 0);
            GFL_BGSysLoadChar(0, picture->charData->rawData, picture->charData->size, 0);
            GFL_G2DIOLoadArcNCLRDefault(arc, fileId, PALTYPE_MAIN_BG_EX, 0, 0, HEAPID_STAFF_ROLL);
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
        }
        if (picture->screens & SCREEN_BOTTOM) {
            GFL_BGSysLoadScrCore(4, picture->screenData->rawData, picture->screenData->size, 0);
            GFL_BGSysLoadChar(4, picture->charData->rawData, picture->charData->size, 0);
            GFL_G2DIOLoadArcNCLRDefault(arc, fileId, PALTYPE_SUB_BG_EX, 0x4000, 0, HEAPID_STAFF_ROLL);
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, TRUE);
        }
        picture->state = PICTURE_FADE_IN;
        return;
    case PICTURE_FADE_IN:
        if (StaffRollPicture_Fade(picture, FALSE, work->fastForward) == TRUE) {
            picture->state = PICTURE_WAIT;
            return;
        }
        break;
    case PICTURE_WAIT:
        if (work->fastForward == TRUE) {
            picture->wait -= 4;
        } else {
            picture->wait--;
        }
        if (picture->wait <= 0) {
            picture->state = PICTURE_FADE_OUT;
            return;
        }
        break;
    case PICTURE_FADE_OUT:
        if (StaffRollPicture_Fade(picture, TRUE, work->fastForward) == TRUE) {
            picture->state = PICTURE_END;
            if (picture->isLast == TRUE) {
                func_0204c520(work->clact.actor, FALSE);
                func_0204c550(work->clact.actor);
                func_0204c124(work->clact.actor, FALSE);
            }
        }
        break;
    case PICTURE_END:
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, FALSE);
        G2_BlendNone();
        G2S_BlendNone();
        picture->state = PICTURE_NONE;
        break;
    }
}

// Advances a fade of the picture, and returns TRUE once it is over
static BOOL StaffRollPicture_Fade(StaffRollPicture *picture, BOOL out, u8 fastForward) {
    int eva, evb;

    if (fastForward == TRUE) {
        picture->frame += 4;
    } else {
        picture->frame++;
    }
    if (!out) {
        eva = picture->frame * 16 / picture->fadeFrames;
        evb = 16 - eva;
    } else {
        evb = picture->frame * 16 / picture->fadeFrames;
        eva = 16 - evb;
    }
    if (eva < 0) {
        eva = 0;
    }
    if (evb < 0) {
        evb = 0;
    }
    if (eva > 16) {
        eva = 16;
    }
    if (evb > 16) {
        evb = 16;
    }
    if (picture->isLast == TRUE) {
        gfxRegAdjustBrightnessBlend(REG_BLDCNT_ADDR, FADE_BRIGHTNESS(evb));
    } else {
        if (picture->screens & SCREEN_TOP) {
            G2_ChangeBlendAlpha(eva, evb);
        }
        if (picture->screens & SCREEN_BOTTOM) {
            G2S_ChangeBlendAlpha(eva, evb);
        }
    }
    if (picture->frame >= picture->fadeFrames) {
        picture->frame = 0;
        return TRUE;
    }
    return FALSE;
}

// The files of each picture in the end archive, from 21: the palette, characters and screen of the male player's
// version, then the female's, 12 files apart
static u32 StaffRollPicture_GetFileID(StaffRollPicture *picture, u32 kind, u32 gender) {
    u32 palette = 21;
    u32 chars = 22;
    u32 screen = 23;
    u32 offset = picture->index * 12 + PICTURE_VERSION_OFFSET;

    if (gender == 1) {
        offset += 3;
    }
    palette += offset;
    chars += offset;
    screen += offset;
    if (picture->isLast == TRUE) {
        palette = LAST_PICTURE_FILE;
        chars = LAST_PICTURE_FILE + 1;
        screen = LAST_PICTURE_FILE + 2;
    }
    switch (kind) {
    case PICTURE_FILE_PALETTE:
        return palette;
    case PICTURE_FILE_CHAR:
        return chars;
    case PICTURE_FILE_SCREEN:
        return screen;
    }
    return 0;
}

// A line that scrolls alone, or the end of a block when its message is 0xffff
static BOOL StaffRollCmd_Line(StaffRollWork *work, StaffRollLine *line) {
    if (line->msgId == 0xffff) {
        work->waitFrames = line->wait;
        work->state = ROLL_WAIT;
        return TRUE;
    }
    if (work->waitFrames == 0) {
        work->waitFrames = line->wait;
    }
    StaffRoll_DrawLineBelow(work, line);
    if (line[1].command != CMD_LINE || line[1].wait != 0) {
        work->state = ROLL_WAIT;
        return TRUE;
    }
    return FALSE;
}

// A title, which fades in with the titles after it
static BOOL StaffRollCmd_FadeIn(StaffRollWork *work, StaffRollLine *line) {
    if (work->state == ROLL_READ) {
        work->unk231 = line->arg;
        work->state = ROLL_FADE_IN;
    }
    StaffRoll_DrawTitle(work, line);
    if (line[1].command != CMD_FADE_IN) {
        return TRUE;
    }
    return FALSE;
}

static BOOL StaffRollCmd_FadeOut(StaffRollWork *work, StaffRollLine *line) {
    work->unk231 = line->arg;
    work->state = ROLL_FADE_OUT;
    return TRUE;
}

static BOOL StaffRollCmd_ScrollStart(StaffRollWork *work, StaffRollLine *line) {
    work->scrolling = TRUE;
    return FALSE;
}

static BOOL StaffRollCmd_ScrollStop(StaffRollWork *work, StaffRollLine *line) {
    work->scrolling = FALSE;
    return FALSE;
}

static BOOL StaffRollCmd_End(StaffRollWork *work, StaffRollLine *line) {
    work->state = ROLL_END;
    return TRUE;
}

static BOOL StaffRollCmd_Yield(StaffRollWork *work, StaffRollLine *line) {
    return TRUE;
}

static BOOL StaffRollCmd_LineAt(StaffRollWork *work, StaffRollLine *line) {
#ifdef BLACK2
    line += line->arg;
#else
    line += line->arg + 1;
#endif
    StaffRoll_DrawLineBelow(work, line);
    work->waitFrames = line->wait;
    work->state = ROLL_WAIT;
    return TRUE;
}

static BOOL StaffRollCmd_Picture(StaffRollWork *work, StaffRollLine *line) {
    work->picture.index = line->arg;
    work->picture.part = 0;
    work->picture.screens = 0;
    work->picture.frame = 0;
    work->picture.fadeFrames = line->x;
    work->picture.wait = line->wait;
    work->picture.isLast = FALSE;
    if (line->align == ALIGN_PICTURE_TOP) {
        work->picture.screens = SCREEN_TOP;
    } else if (line->align == ALIGN_PICTURE_BOTTOM) {
        work->picture.screens = SCREEN_BOTTOM;
    }
    work->picture.state = PICTURE_START;
    return FALSE;
}

static BOOL StaffRollCmd_LastPicture(StaffRollWork *work, StaffRollLine *line) {
    work->picture.index = 0;
    work->picture.part = 0;
    work->picture.screens = 0;
    work->picture.frame = 0;
    work->picture.fadeFrames = line->x;
    // The original clears the screens a second time before setting them
    work->picture.screens = 0;
    work->picture.wait = line->wait;
    work->picture.isLast = TRUE;
    work->picture.screens = SCREEN_TOP;
    work->picture.state = PICTURE_LAST_START;
    return FALSE;
}
