#include "types.h"
#include "app/staff_roll.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/calctool.h"
#include "gfl/clact.h"
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

// The staff roll. The whole overlay is one file, the only one the ROM names. A list of the staff roll's archive
// (ARCID_THE_END) drives it: each entry is a command, most of which put a message of the staff roll's text bank on the
// screens. The lines of the roll enter at the bottom of the lower screen, the sub engine's, and scroll up through both
// screens; fixed texts and pictures fade in and out meanwhile. The names are ours

// Black 2 fades through black and White 2 through white, and each has its own pictures
#ifdef BLACK2
#define VERSION_INDEX 0
#define FADE_COLOR WIPE_COLOR_BLACK
#define FADE_BRIGHTNESS (-16)
#define BACKDROP_COLOR 0x0000
#define COPYRIGHT_PALETTE 1
#else
#define VERSION_INDEX 1
#define FADE_COLOR WIPE_COLOR_WHITE
#define FADE_BRIGHTNESS 16
#define BACKDROP_COLOR 0xffff
#define COPYRIGHT_PALETTE 2
#endif

#define BG_MAIN_PICTURE 0
#define BG_MAIN_TEXT 1
#define BG_MAIN_TITLE 3
#define BG_SUB_PICTURE 4
#define BG_SUB_TEXT 5

// The rows of the two text windows
#define MAIN_WINDOW_HEIGHT 25
#define SUB_WINDOW_HEIGHT 28
// The bytes of a tile and of a row of tiles in a window's bitmap
#define TILE_SIZE 0x20
#define ROW_SIZE (32 * TILE_SIZE)
// Where a line of the roll is put in the sub screen's window, below the visible rows
#define LINE_Y 192

// A screen of a StaffRollWork's flags
#define SCREEN_MAIN 1
#define SCREEN_SUB 2

// The lines the roll draws ahead of time
#define LINE_BITMAP_MAX 64

// No message, in a StaffRollEntry
#define MESSAGE_NONE 0xffff

// Members of ARCID_THE_END
enum {
    FILE_LIST = 1,
    // The actor's files and the files of the picture it is shown with: seven files, Black 2's and then White 2's
    FILE_OBJ_NCLR = 6 + VERSION_INDEX * 7,
    FILE_OBJ_NCGR,
    FILE_OBJ_NCER,
    FILE_OBJ_NANR,
    FILE_OBJ_PICTURE_NCLR,
    FILE_OBJ_PICTURE_NCGR,
    FILE_OBJ_PICTURE_NSCR,
    FILE_TEXT_NCLR = 20,
    // The first picture's. Each picture has four sets of these three: Black 2's for the male and for the female
    // player, and then White 2's
    FILE_PICTURE_NCLR,
    FILE_PICTURE_NCGR,
    FILE_PICTURE_NSCR,
};

// The files of a picture, of a version of a picture and of a player's gender of a version
#define PICTURE_FILES 12
#define PICTURE_VERSION_FILES 6
#define PICTURE_GENDER_FILES 3

// The parts a picture's characters are read in, one a frame
#define PICTURE_PARTS 8

// The sequences of the process
enum {
    SEQ_START,
    SEQ_WAIT_FADE,
    SEQ_FINISH,
    SEQ_OPENING,
    SEQ_ROLL,
    SEQ_ENDING,
    SEQ_END,
};

// What the roll waits for before it reads the list on
enum {
    ROLL_READ,
    ROLL_WAIT,
    ROLL_SHOW_TEXT,
    ROLL_HIDE_TEXT,
    ROLL_UNUSED,
    ROLL_END,
};

// The alignments of a message, and the screens of a picture, in a StaffRollEntry
enum {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER,
    ALIGN_PICTURE_MAIN,
    ALIGN_PICTURE_SUB,
};

// The kinds of file of a picture
enum {
    PICTURE_FILE_NCLR,
    PICTURE_FILE_NCGR,
    PICTURE_FILE_NSCR,
};

// The steps of a picture
enum {
    PICTURE_NONE,
    PICTURE_START,
    PICTURE_LOAD_CHARS,
    PICTURE_LOAD_SCREEN,
    PICTURE_SHOW,
    PICTURE_FADE_IN,
    PICTURE_WAIT,
    PICTURE_FADE_OUT,
    PICTURE_START_OBJ,
    PICTURE_END,
};

// An entry of the list
typedef struct {
    u32 messageId;
    // The frames to wait after a line, the Y of a fixed text, or the frames a picture stays
    u16 wait;
    u8 command;
    u8 arg;
    u8 color;
    u8 font;
    u16 align;
    // The X of a message, or the frames a picture fades in
    s16 x;
    s16 offsetX;
} StaffRollEntry;

// A pair of lines that follow each other with no wait, drawn ahead of time
typedef struct {
    GFLBitmap *bitmap;
    // The entry of the first line
    u32 entry;
} StaffRollLineBitmap;

typedef struct {
    BmpWin *window;
    u32 unk04;
} StaffRollWindow;

typedef struct {
    u8 step;
    u8 screens;
    u8 loadedParts;
    u8 picture;
    s32 frame;
    s32 fadeFrames;
    s32 wait;
    // Whether it is the picture with the actor, which fades with the brightness
    BOOL withObj;
    NNSG2dCharacterData *chars;
    NNSG2dScreenData *screen;
    u32 unk1c;
    void *charFile;
    void *screenFile;
} StaffRollPicture;

typedef struct {
    ClActUnit *unit;
    ClActor *actor;
    u32 palette;
    u32 chars;
    u32 cellAnims;
} StaffRollObj;

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
    u8 usedLineBitmaps;
    // The main screen's and the sub screen's
    StaffRollWindow windows[2];
    // The screens whose window the VBlank copies
    u8 flushScreens;
    u8 textScreens;
    u8 blendStep;
    // The screens with a fixed text
    u8 blendScreens;
    s8 blend;
    u16 titleFrame;
    u8 unk238;
    // Never set: everything runs four times as fast
    u8 fast;
    StaffRollEntry *entries;
    u16 position;
    s16 wait;
    fx32 scroll;
    BOOL scrolling;
    u32 timer;
    int seq;
    u32 subSeq;
    int nextSeq;
    StaffRollPicture picture;
    StaffRollObj obj;
} StaffRollWork;

typedef int (*StaffRollSeqFunc)(StaffRollWork *work);
typedef BOOL (*StaffRollCommandFunc)(StaffRollWork *work, StaffRollEntry *entry);

static BOOL StaffRoll_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL StaffRoll_Update(GameProc *proc, u32 *state, void *param, void *work);
static BOOL StaffRoll_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL StaffRoll_Main(StaffRollWork *work);
static int StaffRoll_SeqStart(StaffRollWork *work);
static int StaffRoll_SeqWaitFade(StaffRollWork *work);
static int StaffRoll_SeqFinish(StaffRollWork *work);
static int StaffRoll_SeqOpening(StaffRollWork *work);
static int StaffRoll_SeqRoll(StaffRollWork *work);
static int StaffRoll_SeqEnding(StaffRollWork *work);
static void StaffRoll_InitVRAM(void);
static void StaffRoll_InitBG(void);
static void StaffRoll_FreeBG(void);
static void StaffRoll_LoadBG(ArcTool *arc);
static void StaffRoll_InitObj(ArcTool *arc, StaffRollObj *obj);
static void StaffRoll_FreeObj(StaffRollObj *obj);
static void StaffRoll_AllocPictureFiles(StaffRollPicture *picture);
static void StaffRoll_FreePictureFiles(StaffRollPicture *picture);
static void StaffRoll_VBlank(TCB *tcb, void *data);
static void StaffRoll_StartVBlank(StaffRollWork *work);
static void StaffRoll_StopVBlank(StaffRollWork *work);
static void StaffRoll_InitMessage(StaffRollWork *work);
static void StaffRoll_FreeMessage(StaffRollWork *work);
static void StaffRoll_CreateLineBitmaps(StaffRollWork *work);
static void StaffRoll_FreeLineBitmaps(StaffRollWork *work);
static void StaffRoll_DrawLine(StaffRollWork *work, GFLBitmap *bitmap, u32 index);
static void StaffRoll_DrawNextLineBitmap(StaffRollWork *work);
static void StaffRoll_PutLineBitmap(StaffRollWork *work);
static void StaffRoll_LoadList(StaffRollWork *work);
static void StaffRoll_FreeList(StaffRollWork *work);
static void StaffRoll_InitWindows(StaffRollWork *work);
static void StaffRoll_FreeWindows(StaffRollWork *work);
static void StaffRoll_RequestFlush(StaffRollWork *work, u32 screens);
static void StaffRoll_FlushWindows(StaffRollWork *work);
static int StaffRoll_FadeIn(StaffRollWork *work, int nextSeq);
static int StaffRoll_FadeOut(StaffRollWork *work, int nextSeq);
static void StaffRoll_ScrollTile(u32 *tile, u32 *above, u32 lines);
static void StaffRoll_Scroll(StaffRollWork *work);
static void StaffRoll_Print(StaffRollWork *work, StaffRollEntry *entry, GFLBitmap *bitmap, int y);
static void StaffRoll_PutLine(StaffRollWork *work, StaffRollEntry *entry);
static void StaffRoll_PutText(StaffRollWork *work, StaffRollEntry *entry);
static BOOL StaffRoll_ShowText(StaffRollWork *work);
static BOOL StaffRoll_HideText(StaffRollWork *work);
static BOOL StaffRoll_ShowTitle(StaffRollWork *work);
static void StaffRoll_UpdatePicture(StaffRollWork *work, StaffRollPicture *picture);
static BOOL StaffRoll_FadePicture(StaffRollPicture *picture, BOOL out, u8 fast);
static u32 StaffRoll_GetPictureFile(StaffRollPicture *picture, u32 kind, u32 gender);
static BOOL StaffRoll_CommandLine(StaffRollWork *work, StaffRollEntry *entry);
static BOOL StaffRoll_CommandText(StaffRollWork *work, StaffRollEntry *entry);
static BOOL StaffRoll_CommandHideText(StaffRollWork *work, StaffRollEntry *entry);
static BOOL StaffRoll_CommandStartScroll(StaffRollWork *work, StaffRollEntry *entry);
static BOOL StaffRoll_CommandStopScroll(StaffRollWork *work, StaffRollEntry *entry);
static BOOL StaffRoll_CommandEnd(StaffRollWork *work, StaffRollEntry *entry);
static BOOL StaffRoll_CommandNone(StaffRollWork *work, StaffRollEntry *entry);
static BOOL StaffRoll_CommandVersionLine(StaffRollWork *work, StaffRollEntry *entry);
static BOOL StaffRoll_CommandPicture(StaffRollWork *work, StaffRollEntry *entry);
static BOOL StaffRoll_CommandObjPicture(StaffRollWork *work, StaffRollEntry *entry);

const GameProcFunctions STAFF_ROLL_PROC_FUNCTIONS = { StaffRoll_Init, StaffRoll_Update, StaffRoll_Exit };

static const BGSysVRAMConfig sStaffRollVRAMConfig = { 1, 0x10, 4, 0x80, 2, 0, 8, 0, 0, 0, 0x10, 0x10 };

static const StaffRollCommandFunc sStaffRollCommands[] = {
    StaffRoll_CommandLine,       StaffRoll_CommandText,       StaffRoll_CommandHideText, StaffRoll_CommandStartScroll,
    StaffRoll_CommandStopScroll, StaffRoll_CommandEnd,        StaffRoll_CommandNone,     StaffRoll_CommandVersionLine,
    StaffRoll_CommandPicture,    StaffRoll_CommandObjPicture,
};

static const StaffRollSeqFunc sSequences[] = {
    StaffRoll_SeqStart,   StaffRoll_SeqWaitFade, StaffRoll_SeqFinish,
    StaffRoll_SeqOpening, StaffRoll_SeqRoll,     StaffRoll_SeqEnding,
};

// The colors of a StaffRollEntry. White 2's first two are the dark ones, for its white screens
static const u16 sColors[] = {
#ifdef BLACK2
    PRINT_COLOR(1, 2, 0), PRINT_COLOR(3, 4, 0),
#else
    PRINT_COLOR(5, 6, 0), PRINT_COLOR(7, 8, 0),
#endif
    PRINT_COLOR(5, 6, 0), PRINT_COLOR(7, 8, 0), PRINT_COLOR(9, 10, 0), PRINT_COLOR(11, 12, 0), PRINT_COLOR(13, 14, 0),
};

static BOOL StaffRoll_Init(GameProc *proc, u32 *state, void *param, void *work) {
    StaffRollWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_STAFF_ROLL, 0x125000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(StaffRollWork), HEAPID_STAFF_ROLL);
    sys_memset(wk, 0, sizeof(StaffRollWork));
    wk->param = param;
    return TRUE;
}

static BOOL StaffRoll_Update(GameProc *proc, u32 *state, void *param, void *work) {
    if (StaffRoll_Main(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL StaffRoll_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_STAFF_ROLL);
    return TRUE;
}

static BOOL StaffRoll_Main(StaffRollWork *work) {
    work->seq = sSequences[work->seq](work);
    if (work->seq == SEQ_END) {
        return FALSE;
    }
    StaffRoll_DrawNextLineBitmap(work);
    return TRUE;
}

static int StaffRoll_SeqStart(StaffRollWork *work) {
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_MAIN_SUB);
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, FADE_BRIGHTNESS);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, FADE_BRIGHTNESS);
    work->arc = GFL_ArcSysCreateFileHandle(ARCID_THE_END, HEAPID_STAFF_ROLL);
    StaffRoll_InitVRAM();
    StaffRoll_InitBG();
    StaffRoll_InitObj(work->arc, &work->obj);
    StaffRoll_AllocPictureFiles(&work->picture);
    StaffRoll_LoadBG(work->arc);
    StaffRoll_InitMessage(work);
    StaffRoll_InitWindows(work);
    StaffRoll_LoadList(work);
    StaffRoll_StartVBlank(work);
    func_020051d8(TRUE);
    return SEQ_OPENING;
}

static int StaffRoll_SeqWaitFade(StaffRollWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        return work->nextSeq;
    }
    return SEQ_WAIT_FADE;
}

static int StaffRoll_SeqFinish(StaffRollWork *work) {
    func_020051d8(FALSE);
    StaffRoll_StopVBlank(work);
    StaffRoll_FreeList(work);
    if (work->arc != NULL) {
        GFL_ArcToolFree(work->arc);
        work->arc = NULL;
    }
    StaffRoll_FreeWindows(work);
    StaffRoll_FreeMessage(work);
    StaffRoll_FreePictureFiles(&work->picture);
    StaffRoll_FreeObj(&work->obj);
    StaffRoll_FreeBG();
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, FADE_BRIGHTNESS);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, FADE_BRIGHTNESS);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return SEQ_END;
}

// Waits, shows the title, and waits again
static int StaffRoll_SeqOpening(StaffRollWork *work) {
    switch (work->subSeq) {
    case 0:
        if (work->timer >= 20) {
            work->timer = 0;
            work->subSeq++;
        } else if (work->fast == TRUE) {
            work->timer += 4;
        } else {
            work->timer++;
        }
        break;
    case 1:
        if (StaffRoll_ShowTitle(work) == FALSE) {
            work->subSeq++;
        }
        break;
    case 2:
        if (work->timer >= 130) {
            work->timer = 0;
            work->subSeq = 0;
            return SEQ_ROLL;
        }
        if (work->fast == TRUE) {
            work->timer += 4;
        } else {
            work->timer++;
        }
        break;
    }
    return SEQ_OPENING;
}

static int StaffRoll_SeqRoll(StaffRollWork *work) {
    if (work->param->skippable == TRUE && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START)) {
        return StaffRoll_FadeOut(work, SEQ_FINISH);
    }
    if (work->subSeq == ROLL_READ) {
        while (TRUE) {
            StaffRollEntry *entry = &work->entries[work->position];

            if (work->position == work->lineBitmaps[work->usedLineBitmaps].entry) {
                StaffRoll_PutLineBitmap(work);
                work->usedLineBitmaps++;
                work->wait = entry->wait;
                work->subSeq = ROLL_WAIT;
                work->position += 2;
                break;
            }
            if (sStaffRollCommands[entry->command](work, entry) == TRUE) {
                work->position++;
                break;
            }
            work->position++;
        }
    }
    switch (work->subSeq) {
    case ROLL_READ:
        break;
    case ROLL_WAIT:
        if (work->wait <= 0) {
            work->wait = 0;
            work->subSeq = ROLL_READ;
        } else if (work->fast == TRUE) {
            work->wait -= 4;
        } else {
            work->wait--;
        }
        break;
    case ROLL_SHOW_TEXT:
        if (StaffRoll_ShowText(work) == FALSE) {
            work->subSeq = ROLL_READ;
        }
        break;
    case ROLL_HIDE_TEXT:
        if (StaffRoll_HideText(work) == FALSE) {
            work->subSeq = ROLL_READ;
        }
        break;
    case ROLL_END:
        work->timer = 0;
        work->subSeq = 0;
        return StaffRoll_FadeOut(work, SEQ_ENDING);
    }
    StaffRoll_Scroll(work);
    if (work->param->skippable != TRUE) {
        StaffRoll_UpdatePicture(work, &work->picture);
    }
    func_0204b794();
    return SEQ_ROLL;
}

// Shows the two copyright screens
static int StaffRoll_SeqEnding(StaffRollWork *work) {
    ArcTool *arc;

    switch (work->subSeq) {
    case 0:
        arc = GFL_ArcSysCreateFileHandle(ARCID_COPYRIGHT, HEAPID_TAIL(HEAPID_STAFF_ROLL));
        GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 0, HEAPID_STAFF_ROLL);
        GFL_BGSysLoadArcNCGRStatic(arc, 7, BG_MAIN_TEXT, 0, 0x8000, FALSE, HEAPID_STAFF_ROLL);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 8, BG_MAIN_TEXT, 0, 0, FALSE, HEAPID_STAFF_ROLL);
        GFL_ArcToolFree(arc);
        GFL_BGSysSetScrPaletteNo(BG_MAIN_TEXT, 0, 0, 32, 24, COPYRIGHT_PALETTE);
        GFL_BGSysResetStdPalette(BG_MAIN_TEXT, BACKDROP_COLOR);
        GFL_BGSysLoadScr(BG_MAIN_TEXT);
        work->subSeq = 1;
        return StaffRoll_FadeIn(work, SEQ_ENDING);
    case 1:
        if (work->timer >= 200) {
            work->subSeq = 2;
            work->timer = 0;
            return StaffRoll_FadeOut(work, SEQ_ENDING);
        }
        if (work->fast == TRUE) {
            work->timer += 4;
        } else {
            work->timer++;
        }
        break;
    case 2:
        arc = GFL_ArcSysCreateFileHandle(ARCID_COPYRIGHT, HEAPID_TAIL(HEAPID_STAFF_ROLL));
        GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 0, HEAPID_STAFF_ROLL);
        GFL_BGSysLoadArcNCGRStatic(arc, 1, BG_MAIN_TEXT, 0, 0x8000, FALSE, HEAPID_STAFF_ROLL);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 2, BG_MAIN_TEXT, 0, 0, FALSE, HEAPID_STAFF_ROLL);
        GFL_ArcToolFree(arc);
        GFL_BGSysSetScrPaletteNo(BG_MAIN_TEXT, 0, 0, 32, 24, COPYRIGHT_PALETTE);
        GFL_BGSysResetStdPalette(BG_MAIN_TEXT, BACKDROP_COLOR);
        GFL_BGSysLoadScr(BG_MAIN_TEXT);
        work->subSeq = 3;
        return StaffRoll_FadeIn(work, SEQ_ENDING);
    case 3:
        if (work->timer >= 200) {
            work->timer = 0;
            work->subSeq = 0;
            return StaffRoll_FadeOut(work, SEQ_FINISH);
        }
        if (work->fast == TRUE) {
            work->timer += 4;
        } else {
            work->timer++;
        }
        break;
    }
    return SEQ_ENDING;
}

static void StaffRoll_InitVRAM(void) {
    GFL_BGSysInitVRAM(0);
    GFL_BGSysSetVRAMBanks(&sStaffRollVRAMConfig);
}

static void StaffRoll_InitBG(void) {
    GFL_BGSysCreate(HEAPID_STAFF_ROLL);
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_3, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&config);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0x00000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(BG_MAIN_TEXT, &setup, BGMODE_TEXT);
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

        GFL_BGSysCreateBG(BG_MAIN_TITLE, &setup, BGMODE_EXTENDED);
        MAT2_SetScaleRot(&mtx, 0, FX32_ONE, FX32_ONE, 0);
        GFL_BGSysSetBGTransform(BG_MAIN_TITLE, &mtx, 128, 96);
    }
    {
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

        GFL_BGSysCreateBG(BG_MAIN_PICTURE, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0x00000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(BG_SUB_TEXT, &setup, BGMODE_TEXT);
    }
    {
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

        GFL_BGSysCreateBG(BG_SUB_PICTURE, &setup, BGMODE_TEXT);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, FALSE);
}

static void StaffRoll_FreeBG(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, FALSE);
    GFL_BGSysReleaseBG(BG_SUB_TEXT);
    GFL_BGSysReleaseBG(BG_MAIN_TITLE);
    GFL_BGSysReleaseBG(BG_MAIN_TEXT);
    GFL_BGSysReleaseBG(BG_MAIN_PICTURE);
    GFL_BGSysReleaseBG(BG_SUB_PICTURE);
    GFL_BGSysFree();
}

// Loads the title's picture, from the title screen's archive, and the palette of the text
static void StaffRoll_LoadBG(ArcTool *arc) {
    ArcTool *titleArc = GFL_ArcSysCreateFileHandle(ARCID_TITLE, HEAPID_TAIL(HEAPID_STAFF_ROLL));

    GFL_G2DIOLoadArcNCLRDefault(titleArc, 2, PALTYPE_MAIN_BG_EX, 0x6000, 0, HEAPID_STAFF_ROLL);
    GFL_BGSysLoadArcNCGRStatic(titleArc, 0, BG_MAIN_TITLE, 0, 0, TRUE, HEAPID_STAFF_ROLL);
    loadBGScrToVramByFileNoReserveNegAlign(titleArc, 1, BG_MAIN_TITLE, 0, 0, TRUE, HEAPID_STAFF_ROLL);
    GFL_ArcToolFree(titleArc);
    GFL_G2DIOLoadArcNCLRDefault(arc, FILE_TEXT_NCLR, PALTYPE_MAIN_BG, 15 * 0x20, 0x20, HEAPID_STAFF_ROLL);
    GFL_G2DIOLoadArcNCLRDefault(arc, FILE_TEXT_NCLR, PALTYPE_SUB_BG, 15 * 0x20, 0x20, HEAPID_STAFF_ROLL);
    GFL_BGSysResetStdPalette(BG_MAIN_TEXT, BACKDROP_COLOR);
    GFL_BGSysResetStdPalette(BG_SUB_TEXT, BACKDROP_COLOR);
}

static void StaffRoll_InitObj(ArcTool *arc, StaffRollObj *obj) {
    ClActorSetup setup;

    ClActSys_Create(&data_02093f08, &sStaffRollVRAMConfig, HEAPID_STAFF_ROLL);
    obj->unit = func_0204bf1c(1, 0, HEAPID_TAIL(HEAPID_STAFF_ROLL));
    func_0204c028(obj->unit);
    obj->chars = func_0204b81c(arc, FILE_OBJ_NCGR, FALSE, 0, HEAPID_TAIL(HEAPID_STAFF_ROLL));
    obj->palette = func_0204bba0(arc, FILE_OBJ_NCLR, 0, 0, HEAPID_TAIL(HEAPID_STAFF_ROLL));
    obj->cellAnims = func_0204bde0(arc, FILE_OBJ_NCER, FILE_OBJ_NANR, HEAPID_TAIL(HEAPID_STAFF_ROLL));
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 0;
    setup.bgPriority = 1;
    setup.priority = 0;
    obj->actor =
        func_0204c040(obj->unit, obj->chars, obj->palette, obj->cellAnims, &setup, 0, HEAPID_TAIL(HEAPID_STAFF_ROLL));
    func_0204c520(obj->actor, TRUE);
    func_0204c124(obj->actor, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
}

static void StaffRoll_FreeObj(StaffRollObj *obj) {
    func_0204b98c(obj->chars);
    func_0204bcd0(obj->palette);
    func_0204be64(obj->cellAnims);
    func_0204bf98(obj->unit);
    func_0204b758();
}

// Allocates the buffers that the pictures' files are read into, which are all as large as the first's
static void StaffRoll_AllocPictureFiles(StaffRollPicture *picture) {
    u32 charSize = GFL_ArcSysGetDataLength(ARCID_THE_END, FILE_PICTURE_NCGR);
    u32 screenSize = GFL_ArcSysGetDataLength(ARCID_THE_END, FILE_PICTURE_NSCR);

    picture->charFile = GFL_HeapAllocate(HEAPID_STAFF_ROLL, charSize, FALSE, "staff_roll_main.c", 1074);
    picture->screenFile = GFL_HeapAllocate(HEAPID_STAFF_ROLL, screenSize, FALSE, "staff_roll_main.c", 1075);
}

static void StaffRoll_FreePictureFiles(StaffRollPicture *picture) {
    GFL_HeapFree(picture->charFile);
    GFL_HeapFree(picture->screenFile);
}

static void StaffRoll_VBlank(TCB *tcb, void *data) {
    StaffRoll_FlushWindows(data);
    GFL_BGSysUpdate();
    func_0204b7c8();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

static void StaffRoll_StartVBlank(StaffRollWork *work) {
    work->vblankTask = GFL_VBlankTCBAdd(StaffRoll_VBlank, work, 0);
}

static void StaffRoll_StopVBlank(StaffRollWork *work) {
    GFL_TCBRemove(work->vblankTask);
}

static void StaffRoll_InitMessage(StaffRollWork *work) {
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_15, HEAPID_STAFF_ROLL);
    work->strbuf = GFL_StrBufCreate(0x80, HEAPID_STAFF_ROLL);
    work->fonts[0] = GFL_FontCreate(ARCID_FONT, 0, 1, FALSE, HEAPID_STAFF_ROLL);
    work->fonts[1] = GFL_FontCreate(ARCID_FONT, 2, 1, FALSE, HEAPID_STAFF_ROLL);
}

static void StaffRoll_FreeMessage(StaffRollWork *work) {
    GFL_FontFree(work->fonts[1]);
    GFL_FontFree(work->fonts[0]);
    GFL_StrBufFree(work->strbuf);
    GFL_MsgDataFree(work->msgData);
}

// Creates a bitmap for each pair of lines that follow each other with no wait
static void StaffRoll_CreateLineBitmaps(StaffRollWork *work) {
    u16 i = 0;

    while (TRUE) {
        StaffRollEntry *entry = &work->entries[i];

        if (entry->command == 5) {
            break;
        }
        if (entry->command == 0 && entry->messageId != MESSAGE_NONE) {
            StaffRollEntry *next = &work->entries[i + 1];

            if (next->wait == 0 && next->command == 0 && next->messageId != MESSAGE_NONE) {
                work->lineBitmaps[work->lineBitmapCount].bitmap = GFL_BitmapCreate(32, 2, 0x20, HEAPID_STAFF_ROLL);
                work->lineBitmaps[work->lineBitmapCount].entry = i;
                work->lineBitmapCount++;
                if (work->lineBitmapCount == LINE_BITMAP_MAX) {
                    break;
                }
                i++;
            }
        }
        i++;
    }
}

static void StaffRoll_FreeLineBitmaps(StaffRollWork *work) {
    u32 i;

    for (i = 0; i < work->lineBitmapCount; i++) {
        GFL_BitmapFree(work->lineBitmaps[i].bitmap);
    }
}

static void StaffRoll_DrawLine(StaffRollWork *work, GFLBitmap *bitmap, u32 index) {
    StaffRollEntry *entry = &work->entries[index];

    if (entry->font == 0) {
        StaffRoll_Print(work, entry, bitmap, 0);
    } else {
        StaffRoll_Print(work, entry, bitmap, 2);
    }
}

// Draws the lines of a bitmap, one bitmap a frame
static void StaffRoll_DrawNextLineBitmap(StaffRollWork *work) {
    if (work->drawnLineBitmaps < work->lineBitmapCount) {
        StaffRoll_DrawLine(work, work->lineBitmaps[work->drawnLineBitmaps].bitmap,
                           work->lineBitmaps[work->drawnLineBitmaps].entry);
        StaffRoll_DrawLine(work, work->lineBitmaps[work->drawnLineBitmaps].bitmap,
                           work->lineBitmaps[work->drawnLineBitmaps].entry + 1);
        work->drawnLineBitmaps++;
    }
}

static void StaffRoll_PutLineBitmap(StaffRollWork *work) {
    GFL_BitmapCopyArea(work->lineBitmaps[work->usedLineBitmaps].bitmap, BmpWin_GetBitmap(work->windows[1].window), 0, 0,
                       0, LINE_Y, 256, 16, 0);
}

static void StaffRoll_LoadList(StaffRollWork *work) {
    work->entries = GFL_ArcSysReadHeapNew(ARCID_THE_END, FILE_LIST, HEAPID_STAFF_ROLL);
    StaffRoll_CreateLineBitmaps(work);
}

static void StaffRoll_FreeList(StaffRollWork *work) {
    StaffRoll_FreeLineBitmaps(work);
    GFL_HeapFree(work->entries);
}

static void StaffRoll_InitWindows(StaffRollWork *work) {
    BmpWin_InitAllocator(HEAPID_STAFF_ROLL);
    work->windows[0].window = BmpWin_CreateDynamic(BG_MAIN_TEXT, 0, 0, 32, MAIN_WINDOW_HEIGHT, 15, TRUE);
    work->windows[1].window = BmpWin_CreateDynamic(BG_SUB_TEXT, 0, 0, 32, SUB_WINDOW_HEIGHT, 15, TRUE);
    BmpWin_TransferNow(work->windows[0].window);
    BmpWin_TransferNow(work->windows[1].window);
}

static void StaffRoll_FreeWindows(StaffRollWork *work) {
    BmpWin_Free(work->windows[0].window);
    BmpWin_Free(work->windows[1].window);
    BmpWin_FreeAllocator();
}

static void StaffRoll_RequestFlush(StaffRollWork *work, u32 screens) {
    work->flushScreens |= (u8)screens;
}

static void StaffRoll_FlushWindows(StaffRollWork *work) {
    if (work->flushScreens & SCREEN_MAIN) {
        BmpWin_FlushChar(work->windows[0].window);
    }
    if (work->flushScreens & SCREEN_SUB) {
        BmpWin_FlushChar(work->windows[1].window);
    }
    work->flushScreens = 0;
}

static int StaffRoll_FadeIn(StaffRollWork *work, int nextSeq) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, FADE_COLOR, 6, 1, HEAPID_STAFF_ROLL);
    work->nextSeq = nextSeq;
    return SEQ_WAIT_FADE;
}

static int StaffRoll_FadeOut(StaffRollWork *work, int nextSeq) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, FADE_COLOR, 6, 1, HEAPID_STAFF_ROLL);
    work->nextSeq = nextSeq;
    return SEQ_WAIT_FADE;
}

// Moves a tile of a window up by lines, its top lines into the bottom of the tile above
static void StaffRoll_ScrollTile(u32 *tile, u32 *above, u32 lines) {
    u32 i;

    if (above != NULL) {
        for (i = 0; i < lines; i++) {
            above[8 - lines + i] = tile[i];
        }
    }
    for (i = 0; i < 8 - lines; i++) {
        tile[i] = tile[lines + i];
    }
}

// Scrolls both windows up, the sub screen's top into the main screen's bottom
static void StaffRoll_Scroll(StaffRollWork *work) {
    u8 *mainPixels;
    u8 *subPixels;
    u32 lines;
    u32 tile;
    u32 above;
    u16 x;
    u16 y;

    if (!work->scrolling) {
        return;
    }
    work->scroll += FX32_ONE;
    if (work->scroll < FX32_ONE) {
        return;
    }
    work->scroll -= FX32_ONE;
    lines = 1;
    if (work->fast) {
        lines = 4;
    }
    mainPixels = GFL_BitmapGetPixelData(BmpWin_GetBitmap(work->windows[0].window));
    subPixels = GFL_BitmapGetPixelData(BmpWin_GetBitmap(work->windows[1].window));
    tile = 0;
    for (x = 0; x < 32; x++) {
        StaffRoll_ScrollTile((u32 *)&mainPixels[tile], NULL, lines);
        tile += TILE_SIZE;
    }
    for (y = 1; y < MAIN_WINDOW_HEIGHT; y++) {
        tile = y * ROW_SIZE;
        above = (y - 1) * ROW_SIZE;
        for (x = 0; x < 32; x++) {
            StaffRoll_ScrollTile((u32 *)&mainPixels[tile], (u32 *)&mainPixels[above], lines);
            tile += TILE_SIZE;
            above += TILE_SIZE;
        }
    }
    tile = 0;
    above = (MAIN_WINDOW_HEIGHT - 1) * ROW_SIZE;
    for (x = 0; x < 32; x++) {
        StaffRoll_ScrollTile((u32 *)&subPixels[tile], (u32 *)&mainPixels[above], lines);
        tile += TILE_SIZE;
        above += TILE_SIZE;
    }
    for (y = 1; y < SUB_WINDOW_HEIGHT; y++) {
        tile = y * ROW_SIZE;
        above = (y - 1) * ROW_SIZE;
        for (x = 0; x < 32; x++) {
            StaffRoll_ScrollTile((u32 *)&subPixels[tile], (u32 *)&subPixels[above], lines);
            tile += TILE_SIZE;
            above += TILE_SIZE;
        }
    }
    StaffRoll_RequestFlush(work, SCREEN_MAIN | SCREEN_SUB);
}

// Draws an entry's message into a bitmap
static void StaffRoll_Print(StaffRollWork *work, StaffRollEntry *entry, GFLBitmap *bitmap, int y) {
    int x;

    GFL_MsgDataLoadStrbuf(work->msgData, entry->messageId, work->strbuf);
    if (entry->align == ALIGN_LEFT) {
        x = entry->x + entry->offsetX;
    } else if (entry->align == ALIGN_RIGHT) {
        x = entry->x + entry->offsetX - GFL_FontGetBlockWidth(work->strbuf, work->fonts[entry->font], 0);
    } else if (entry->align == ALIGN_CENTER) {
        x = entry->x + entry->offsetX - GFL_FontGetBlockWidth(work->strbuf, work->fonts[entry->font], 0) / 2;
    }
    GFL_TextRendererDrawToBitmapEx(bitmap, x, y, work->strbuf, work->fonts[entry->font], sColors[entry->color]);
}

// Puts a line of the roll below the sub screen
static void StaffRoll_PutLine(StaffRollWork *work, StaffRollEntry *entry) {
    if (entry->font == 0) {
        StaffRoll_Print(work, entry, BmpWin_GetBitmap(work->windows[1].window), LINE_Y);
    } else {
        StaffRoll_Print(work, entry, BmpWin_GetBitmap(work->windows[1].window), LINE_Y + 2);
    }
}

// Puts a fixed text on the main screen, or on the sub screen from Y 256 on
static void StaffRoll_PutText(StaffRollWork *work, StaffRollEntry *entry) {
    GFLBitmap *bitmap;
    int y;

    if (entry->wait < 256) {
        bitmap = BmpWin_GetBitmap(work->windows[0].window);
        y = entry->wait;
        work->blendScreens |= SCREEN_MAIN;
    } else {
        bitmap = BmpWin_GetBitmap(work->windows[1].window);
        y = entry->wait - 256;
        work->blendScreens |= SCREEN_SUB;
    }
    StaffRoll_Print(work, entry, bitmap, y);
}

// Fades the fixed texts in. FALSE once it is over
static BOOL StaffRoll_ShowText(StaffRollWork *work) {
    int eva;
    int evb;

    switch (work->blendStep) {
    case 0:
        if (work->blendScreens & SCREEN_MAIN) {
            gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BD,
                                0, 16);
        }
        if (work->blendScreens & SCREEN_SUB) {
            gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1,
                                GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BD, 0, 16);
        }
        work->flushScreens = work->blendScreens;
        work->blendStep++;
        break;
    case 1:
        if (work->fast == TRUE) {
            work->blend += 16;
        } else {
            work->blend += 4;
        }
        if (work->blend > 16) {
            eva = 16;
            evb = 0;
        } else {
            eva = work->blend;
            evb = 16 - eva;
        }
        if (work->blendScreens & SCREEN_MAIN) {
            G2_ChangeBlendAlpha(eva, evb);
        }
        if (work->blendScreens & SCREEN_SUB) {
            G2S_ChangeBlendAlpha(eva, evb);
        }
        if (work->blend >= 16) {
            if (work->blendScreens & SCREEN_MAIN) {
                G2_BlendNone();
            }
            if (work->blendScreens & SCREEN_SUB) {
                G2S_BlendNone();
            }
            work->blend = 0;
            work->blendStep = 0;
            return FALSE;
        }
        break;
    }
    return TRUE;
}

// Fades the fixed texts out and clears their windows. FALSE once it is over
static BOOL StaffRoll_HideText(StaffRollWork *work) {
    int eva;
    int evb;

    switch (work->blendStep) {
    case 0:
        if (work->blendScreens & SCREEN_MAIN) {
            gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BD,
                                16, 0);
        }
        if (work->blendScreens & SCREEN_SUB) {
            gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1,
                                GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BD, 16, 0);
        }
        work->blendStep++;
        // fallthrough
    case 1:
        if (work->fast == TRUE) {
            work->blend += 16;
        } else {
            work->blend += 4;
        }
        if (work->blend > 16) {
            eva = 0;
            evb = 16;
        } else {
            evb = work->blend;
            eva = 16 - evb;
        }
        if (work->blendScreens & SCREEN_MAIN) {
            G2_ChangeBlendAlpha(eva, evb);
        }
        if (work->blendScreens & SCREEN_SUB) {
            G2S_ChangeBlendAlpha(eva, evb);
        }
        if (work->blend >= 16) {
            if (work->blendScreens & SCREEN_MAIN) {
                GFL_BitmapFill(BmpWin_GetBitmap(work->windows[0].window), 0);
            }
            if (work->blendScreens & SCREEN_SUB) {
                GFL_BitmapFill(BmpWin_GetBitmap(work->windows[1].window), 0);
            }
            work->flushScreens = work->blendScreens;
            work->blend = 0;
            work->blendStep++;
        }
        break;
    case 2:
        if (work->blendScreens & SCREEN_MAIN) {
            G2_BlendNone();
        }
        if (work->blendScreens & SCREEN_SUB) {
            G2S_BlendNone();
        }
        work->blendScreens = 0;
        work->blendStep = 0;
        return FALSE;
    }
    return TRUE;
}

// Fades the title in, keeps it for 200 frames and fades it out. FALSE once it is over
static BOOL StaffRoll_ShowTitle(StaffRollWork *work) {
    int alpha;

    switch (work->blendStep) {
    case 0:
        gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG3, GX_BLEND_PLANEMASK_BD, 0, 16);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, TRUE);
        GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, 0);
        GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, 0);
        work->titleFrame = 0;
        work->blendStep++;
        break;
    case 1:
        if (work->fast == TRUE) {
            alpha = (work->titleFrame << 4) / 34;
        } else {
            alpha = (work->titleFrame << 4) / 138;
        }
        {
            s16 eva = alpha;
            s16 evb = 16 - alpha;

            work->titleFrame++;
            if (eva >= 16) {
                G2_ChangeBlendAlpha(16, 0);
                work->titleFrame = 0;
                work->wait = 200;
                work->blendStep++;
            } else {
                G2_ChangeBlendAlpha(eva, evb);
            }
        }
        break;
    case 2:
        if (work->fast == TRUE) {
            work->wait -= 4;
        } else {
            work->wait--;
        }
        if (work->wait <= 0) {
            work->wait = 0;
            work->blendStep++;
        }
        break;
    case 3:
        if (work->fast == TRUE) {
            alpha = (work->titleFrame << 4) / 34;
        } else {
            alpha = (work->titleFrame << 4) / 138;
        }
        {
            s16 eva = 16 - alpha;
            s16 evb = alpha;

            work->titleFrame++;
            if (evb >= 16) {
                G2_BlendNone();
                GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, FALSE);
                work->titleFrame = 0;
                work->blendStep = 0;
                return FALSE;
            }
            G2_ChangeBlendAlpha(eva, evb);
        }
        break;
    }
    return TRUE;
}

// Loads the picture a command asked for over several frames, fades it in, keeps it and fades it out
static void StaffRoll_UpdatePicture(StaffRollWork *work, StaffRollPicture *picture) {
    ArcTool *arc = work->arc;
    u32 gender = getTrainerGender(work->param->playerInfo);
    u32 fileId;
    u32 partSize;

    switch (picture->step) {
    case PICTURE_NONE:
        return;
    case PICTURE_START:
        if (picture->screens & SCREEN_MAIN) {
            gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0,
                                GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |
                                    GX_BLEND_PLANEMASK_BD,
                                0, 16);
        }
        if (picture->screens & SCREEN_SUB) {
            gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0,
                                GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |
                                    GX_BLEND_PLANEMASK_BD,
                                0, 16);
        }
        picture->step = PICTURE_LOAD_CHARS;
        break;
    case PICTURE_START_OBJ:
        gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_OBJ, FADE_BRIGHTNESS);
        func_0204c124(work->obj.actor, TRUE);
        func_0204c56c(work->obj.actor);
        picture->step = PICTURE_LOAD_CHARS;
        break;
    case PICTURE_LOAD_CHARS:
        fileId = StaffRoll_GetPictureFile(picture, PICTURE_FILE_NCGR, gender);
        partSize = GFL_ArcSysGetDataLength(ARCID_THE_END, fileId) / PICTURE_PARTS;
        GFL_ArcToolReadRange(work->arc, fileId, partSize * picture->loadedParts, partSize,
                             (u8 *)picture->charFile + partSize * picture->loadedParts);
        picture->loadedParts++;
        if (picture->loadedParts >= PICTURE_PARTS) {
            NNS_G2dGetUnpackedBGCharacterData(picture->charFile, &picture->chars);
            picture->step = PICTURE_LOAD_SCREEN;
        }
        break;
    case PICTURE_LOAD_SCREEN:
        GFL_ArcToolRead(arc, StaffRoll_GetPictureFile(picture, PICTURE_FILE_NSCR, gender), picture->screenFile);
        NNS_G2dGetUnpackedScreenData(picture->screenFile, &picture->screen);
        picture->step = PICTURE_SHOW;
        break;
    case PICTURE_SHOW:
        fileId = StaffRoll_GetPictureFile(picture, PICTURE_FILE_NCLR, gender);
        if (picture->screens & SCREEN_MAIN) {
            GFL_BGSysLoadScrCore(BG_MAIN_PICTURE, picture->screen->rawData, picture->screen->size, 0);
            GFL_BGSysLoadChar(BG_MAIN_PICTURE, picture->chars->rawData, picture->chars->size, 0);
            GFL_G2DIOLoadArcNCLRDefault(arc, fileId, PALTYPE_MAIN_BG_EX, 0, 0, HEAPID_STAFF_ROLL);
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
        }
        if (picture->screens & SCREEN_SUB) {
            GFL_BGSysLoadScrCore(BG_SUB_PICTURE, picture->screen->rawData, picture->screen->size, 0);
            GFL_BGSysLoadChar(BG_SUB_PICTURE, picture->chars->rawData, picture->chars->size, 0);
            GFL_G2DIOLoadArcNCLRDefault(arc, fileId, PALTYPE_SUB_BG_EX, 0x4000, 0, HEAPID_STAFF_ROLL);
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, TRUE);
        }
        picture->step = PICTURE_FADE_IN;
        break;
    case PICTURE_FADE_IN:
        if (StaffRoll_FadePicture(picture, FALSE, work->fast) == TRUE) {
            picture->step = PICTURE_WAIT;
        }
        break;
    case PICTURE_WAIT:
        if (work->fast == TRUE) {
            picture->wait -= 4;
        } else {
            picture->wait--;
        }
        if (picture->wait <= 0) {
            picture->step = PICTURE_FADE_OUT;
        }
        break;
    case PICTURE_FADE_OUT:
        if (StaffRoll_FadePicture(picture, TRUE, work->fast) == TRUE) {
            picture->step = PICTURE_END;
            if (picture->withObj == TRUE) {
                func_0204c520(work->obj.actor, FALSE);
                func_0204c550(work->obj.actor);
                func_0204c124(work->obj.actor, FALSE);
            }
        }
        break;
    case PICTURE_END:
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, FALSE);
        G2_BlendNone();
        G2S_BlendNone();
        picture->step = PICTURE_NONE;
        break;
    }
}

// Advances the fade of the picture, in or out. TRUE once it is over
static BOOL StaffRoll_FadePicture(StaffRollPicture *picture, BOOL out, u8 fast) {
    int eva;
    int evb;

    if (fast == TRUE) {
        picture->frame += 4;
    } else {
        picture->frame++;
    }
    if (!out) {
        eva = (picture->frame << 4) / picture->fadeFrames;
        evb = 16 - eva;
    } else {
        evb = (picture->frame << 4) / picture->fadeFrames;
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
    if (picture->withObj == TRUE) {
#ifdef BLACK2
        gfxRegAdjustBrightnessBlend(REG_BLDCNT_ADDR, -evb);
#else
        gfxRegAdjustBrightnessBlend(REG_BLDCNT_ADDR, evb);
#endif
    } else {
        if (picture->screens & SCREEN_MAIN) {
            G2_ChangeBlendAlpha(eva, evb);
        }
        if (picture->screens & SCREEN_SUB) {
            G2S_ChangeBlendAlpha(eva, evb);
        }
    }
    if (picture->frame >= picture->fadeFrames) {
        picture->frame = 0;
        return TRUE;
    }
    return FALSE;
}

// The file of a kind of the picture, this version's, and the female player's when gender is 1
static u32 StaffRoll_GetPictureFile(StaffRollPicture *picture, u32 kind, u32 gender) {
    u32 palette = FILE_PICTURE_NCLR;
    u32 chars = FILE_PICTURE_NCGR;
    u32 screen = FILE_PICTURE_NSCR;
    u32 offset = picture->picture * PICTURE_FILES + VERSION_INDEX * PICTURE_VERSION_FILES;

    if (gender == 1) {
        offset += PICTURE_GENDER_FILES;
    }
    palette += offset;
    chars += offset;
    screen += offset;
    if (picture->withObj == TRUE) {
        palette = FILE_OBJ_PICTURE_NCLR;
        chars = FILE_OBJ_PICTURE_NCGR;
        screen = FILE_OBJ_PICTURE_NSCR;
    }
    switch (kind) {
    case PICTURE_FILE_NCLR:
        return palette;
    case PICTURE_FILE_NCGR:
        return chars;
    case PICTURE_FILE_NSCR:
        return screen;
    }
    return 0;
}

// The commands of the list. TRUE when the roll stops reading the list for this frame

// A line of the roll, or with no message only a wait. The lines that follow with no wait are put in the same frame
static BOOL StaffRoll_CommandLine(StaffRollWork *work, StaffRollEntry *entry) {
    if (entry->messageId == MESSAGE_NONE) {
        work->wait = entry->wait;
        work->subSeq = ROLL_WAIT;
        return TRUE;
    }
    if (work->wait == 0) {
        work->wait = entry->wait;
    }
    StaffRoll_PutLine(work, entry);
    if (entry[1].command != 0 || entry[1].wait != 0) {
        work->subSeq = ROLL_WAIT;
        return TRUE;
    }
    return FALSE;
}

// A fixed text, which fades in with the texts that follow it
static BOOL StaffRoll_CommandText(StaffRollWork *work, StaffRollEntry *entry) {
    if (work->subSeq == ROLL_READ) {
        work->textScreens = entry->arg;
        work->subSeq = ROLL_SHOW_TEXT;
    }
    StaffRoll_PutText(work, entry);
    if (entry[1].command != 1) {
        return TRUE;
    }
    return FALSE;
}

static BOOL StaffRoll_CommandHideText(StaffRollWork *work, StaffRollEntry *entry) {
    work->textScreens = entry->arg;
    work->subSeq = ROLL_HIDE_TEXT;
    return TRUE;
}

static BOOL StaffRoll_CommandStartScroll(StaffRollWork *work, StaffRollEntry *entry) {
    work->scrolling = TRUE;
    return FALSE;
}

static BOOL StaffRoll_CommandStopScroll(StaffRollWork *work, StaffRollEntry *entry) {
    work->scrolling = FALSE;
    return FALSE;
}

static BOOL StaffRoll_CommandEnd(StaffRollWork *work, StaffRollEntry *entry) {
    work->subSeq = ROLL_END;
    return TRUE;
}

static BOOL StaffRoll_CommandNone(StaffRollWork *work, StaffRollEntry *entry) {
    return TRUE;
}

// A line that each version has its own entry for, after this one
static BOOL StaffRoll_CommandVersionLine(StaffRollWork *work, StaffRollEntry *entry) {
    entry = &entry[entry->arg + VERSION_INDEX];
    StaffRoll_PutLine(work, entry);
    work->wait = entry->wait;
    work->subSeq = ROLL_WAIT;
    return TRUE;
}

static BOOL StaffRoll_CommandPicture(StaffRollWork *work, StaffRollEntry *entry) {
    work->picture.picture = entry->arg;
    work->picture.loadedParts = 0;
    work->picture.screens = 0;
    work->picture.frame = 0;
    work->picture.fadeFrames = entry->x;
    work->picture.wait = entry->wait;
    work->picture.withObj = FALSE;
    if (entry->align == ALIGN_PICTURE_MAIN) {
        work->picture.screens = SCREEN_MAIN;
    } else if (entry->align == ALIGN_PICTURE_SUB) {
        work->picture.screens = SCREEN_SUB;
    }
    work->picture.step = PICTURE_START;
    return FALSE;
}

static BOOL StaffRoll_CommandObjPicture(StaffRollWork *work, StaffRollEntry *entry) {
    work->picture.picture = 0;
    work->picture.loadedParts = 0;
    work->picture.screens = 0;
    work->picture.frame = 0;
    work->picture.fadeFrames = entry->x;
    work->picture.screens = 0;
    work->picture.wait = entry->wait;
    work->picture.withObj = TRUE;
    work->picture.screens = SCREEN_MAIN;
    work->picture.step = PICTURE_START_OBJ;
    return FALSE;
}
