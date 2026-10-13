#include "types.h"
#include "battle/b_choice_main.h"
#include "battle/btlv_effect.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wipe.h"

// The battle's choice screen: the choices' buttons on the lower screen, which the touch screen or the keys choose
// from, the chosen button flashing before the screen fades out

#define WINDOW_MAX 14
#define ACTOR_COUNT 4

// The battle's graphics archive, and the one of the actors of the cursor
#define ARCID_BATTLE_CHOICE 101
#define ARCID_BATTLE_CURSOR 11
// The files of the cursor's actors
#define FILE_CURSOR_PALETTE 0x1a5
#define FILE_CURSOR_CHARS 0x1a8
#define FILE_CURSOR_CELLS 0x1a9
#define FILE_CURSOR_ANIMS 0x1aa
// The file of the positions of the cursor and the buttons, in pairs of x and y
#define FILE_POSITIONS 7
#define POSITION_COUNT 0x48

// The BGs of the lower screen
#define BG_FRAME 4
#define BG_BUTTONS 5
#define BG_FLASH 6
#define BG_TEXT 7

// The choice that flashes or flashes not, and no palette change
#define PALETTE_FLASH 6
#define PALETTE_DEFAULT (-1)
#define PALETTE_KEEP (-2)

#define TEXT_X 0x70
#define TEXT_COLOR 0x440
#define ALIGN_CENTER 2
#define ALIGN_RIGHT 1

// The states of the choice screen
enum {
    STEP_INIT = 0,
    STEP_WAIT_WIPE = 10,
    STEP_CHOOSE = 100,
    STEP_FLASH = 110,
    STEP_FADE_OUT = 200,
    STEP_END = 10000,
};

// BChoiceWork's flags
#define FLAG_REFRESH 0x1
// The keys are used, so the cursor shows
#define FLAG_KEYS 0x2
// The choice was made with the keys
#define FLAG_KEY_CHOICE 0x4
#define FLAG_FLASHING 0x8

typedef struct {
    BmpWin *window;
    u8 dirty;
} BChoiceWindow;

// The work of the choice screen, 0xc8 bytes
typedef struct {
    BChoiceParam *param;
    TCBExManager *tcbManager;
    PaletteFade *paletteFade;
    PrintQueue *printQueue;
    BChoiceWindow windows[WINDOW_MAX];
    u32 unk80;
    ClActUnit *unit;
    ClActor *actors[ACTOR_COUNT];
    // The cursor's palette, characters and cells
    u32 resources[3];
    int step;
    u32 flags;
    // The number of choices less 2, which picks the layout
    int layout;
    // The choice made, or -1
    int choice;
    // The choice the cursor is on
    int cursor;
    int flashStep;
    s16 flashFrame[2];
    s16 *positions;
    u16 blend;
} BChoiceWork;

typedef struct {
    u8 type;
    u8 fileId;
    u8 offset;
    u8 count;
} BChoicePaletteLoad;

typedef struct {
    u8 bg;
    u8 fileId;
    u8 offset;
    u8 unused;
} BChoiceBGLoad;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} BChoiceWindowSetup;

typedef struct {
    s16 frame;
    s16 palette;
} BChoiceFlashStep;

typedef struct {
    s16 x;
    s16 y;
    s16 width;
    s16 height;
} BChoiceRect;

typedef struct {
    u8 sequence;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 unused4;
    u8 flags;
    u8 position;
    u8 unused7;
} BChoiceActorSetup;

static void BChoiceMain_Task(TCB *tcb, void *data);
static BOOL BChoiceMain_Finish(TCB *tcb, BChoiceWork *work);
static void BChoiceMain_Step(BChoiceWork *work);
static void BChoiceMain_Init(BChoiceWork *work);
static void BChoiceMain_StartWipe(BChoiceWork *work);
static BOOL BChoiceMain_IsWiping(void);
static void BChoiceMain_StartFadeOut(BChoiceWork *work);
static BOOL BChoiceMain_IsFading(BChoiceWork *work);
static void BChoiceMain_InitBG(BChoiceWork *work);
static void BChoiceMain_FreeBG(BChoiceWork *work);
static void BChoiceMain_InitPalettes(BChoiceWork *work);
static BOOL BChoiceMain_UpdateInput(BChoiceWork *work);
static BOOL BChoiceMain_ReadInput(BChoiceWork *work);
static void BChoiceMain_LoadGraphics(BChoiceWork *work, int layout);
static void BChoiceMain_CreateWindows(BChoiceWork *work, int layout);
static void BChoiceMain_FreeWindows(BChoiceWork *work);
static void BChoiceMain_PrintText(BChoiceWork *work, u32 window, int x, int y, StrBuf *str, u32 align, u16 color);
static u16 BChoiceMain_GetAlignOffset(StrBuf *str, Font *font, u32 align);
static void BChoiceMain_UpdatePrint(BChoiceWork *work);
static s32 BChoiceMain_GetTouched(BChoiceWork *work, int layout);
static void BChoiceMain_PrintChoices(BChoiceWork *work);
static void BChoiceMain_InitActors(BChoiceWork *work);
static void BChoiceMain_FreeActors(BChoiceWork *work);
static void BChoiceMain_LoadActorResources(BChoiceWork *work);
static void BChoiceMain_CreateActors(BChoiceWork *work, u32 surface, const BChoiceActorSetup *setups,
                                     const s16 *resources);
static s16 BChoiceMain_GetPosition(BChoiceWork *work, u32 index);
static void BChoiceMain_SetCursorVisible(BChoiceWork *work, u32 unused, int index, BOOL visible);
static void BChoiceMain_UpdateCursor(BChoiceWork *work, BOOL show, int layout, int choice);
static void BChoiceMain_SetChoicePalette(BChoiceWork *work, int layout, int choice, int palette);
static void BChoiceMain_StartFlash(BChoiceWork *work);
static void BChoiceMain_UpdateFlash(BChoiceWork *work);
static BOOL BChoiceMain_IsFlashing(BChoiceWork *work);

static const BChoiceRect sBChoiceChoiceRects2[2] = {
    { 0, 5, 32, 4 },
    { 0, 12, 32, 4 },
};

static const BChoiceRect sBChoiceChoiceRects3[3] = {
    { 0, 2, 32, 4 },
    { 0, 8, 32, 4 },
    { 0, 14, 32, 4 },
};

static const BChoiceRect sBChoiceChoiceRects4[4] = {
    { 0, 1, 32, 4 },
    { 0, 6, 32, 4 },
    { 0, 11, 32, 4 },
    { 0, 16, 32, 4 },
};

static const BChoiceRect *sBChoiceChoiceRectTables[3] = { sBChoiceChoiceRects2, sBChoiceChoiceRects3,
                                                          sBChoiceChoiceRects4 };

static const s16 sBChoiceChoicePalettes[4] = { 0, 2, 3, 4 };

// The frames of the flash of the choice made, and its palette
static const BChoiceFlashStep sBChoiceFlashSteps[4] = {
    { 1, 5 },
    { 5, PALETTE_DEFAULT },
    { 20, PALETTE_KEEP },
    { -1, -1 },
};

static const TouchRect sBChoiceTouchRects2[3] = {
    { 40, 72, 8, 248 },
    { 96, 128, 8, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sBChoiceTouchRects3[4] = {
    { 16, 48, 8, 248 },
    { 64, 96, 8, 248 },
    { 112, 144, 8, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sBChoiceTouchRects4[5] = {
    { 8, 40, 8, 248 }, { 48, 80, 8, 248 }, { 88, 120, 8, 248 }, { 128, 160, 8, 248 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect *sBChoiceTouchRectTables[3] = { sBChoiceTouchRects2, sBChoiceTouchRects3, sBChoiceTouchRects4 };

static const BChoiceBGLoad sBChoiceScreenLoads2[3] = {
    { 7, 3, 0, 0 },
    { 6, 4, 0, 0 },
    { 0xff, 0, 0, 0 },
};

static const BChoiceBGLoad sBChoiceScreenLoads4[3] = {
    { 7, 3, 0, 0 },
    { 6, 6, 0, 0 },
    { 0xff, 0, 0, 0 },
};

static const BChoiceBGLoad sBChoiceCharLoads[3] = {
    { 7, 2, 0, 0 },
    { 6, 2, 0, 0 },
    { 0xff, 0, 0, 0 },
};

static const BChoiceBGLoad sBChoiceScreenLoads3[3] = {
    { 7, 3, 0, 0 },
    { 6, 5, 0, 0 },
    { 0xff, 0, 0, 0 },
};

// The palettes of the choices' buttons without the flash, by choice
static const BChoicePaletteLoad sBChoicePaletteLoads[2] = {
    { 4, 0, 0, 7 },
    { 0xff, 0, 0, 0 },
};

static const BChoiceBGLoad *sBChoiceScreenLoadTables[3] = { sBChoiceScreenLoads2, sBChoiceScreenLoads3,
                                                            sBChoiceScreenLoads4 };

static const BChoiceWindowSetup sBChoiceWindowSetups3[4] = {
    { 5, 2, 3, 28, 2, 8 },
    { 5, 2, 9, 28, 2, 8 },
    { 5, 2, 15, 28, 2, 8 },
    { 0xff, 0, 0, 0, 0, 0 },
};

static const BChoiceWindowSetup sBChoiceWindowSetups2[3] = {
    { 5, 2, 6, 28, 2, 8 },
    { 5, 2, 13, 28, 2, 8 },
    { 0xff, 0, 0, 0, 0, 0 },
};

static const BChoiceWindowSetup sBChoiceWindowSetups4[5] = {
    { 5, 2, 2, 28, 2, 8 },  { 5, 2, 7, 28, 2, 8 },   { 5, 2, 12, 28, 2, 8 },
    { 5, 2, 17, 28, 2, 8 }, { 0xff, 0, 0, 0, 0, 0 },
};

static const BChoiceWindowSetup *sBChoiceWindowSetupTables[3] = { sBChoiceWindowSetups2, sBChoiceWindowSetups3,
                                                                  sBChoiceWindowSetups4 };

static const BChoiceActorSetup sBChoiceActorSetups[5] = {
    { 0, 48, 1, 0, 0, 1, 0, 0 }, { 1, 48, 1, 0, 0, 1, 0, 0 },   { 2, 48, 1, 0, 0, 1, 0, 0 },
    { 3, 48, 1, 0, 0, 1, 0, 0 }, { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

// The cursor's palette, characters and cells in BChoiceWork's resources, and the number of its actors
static const s16 sBChoiceActorResources[5] = { 0, 1, 2, 4, 0 };

static const s16 sBChoiceCursorPositions4[16] = { 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35 };

static const s16 sBChoiceCursorPositions3[12] = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };

static const s16 sBChoiceCursorPositions2[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

static const s16 *sBChoiceCursorPositionTables[3] = { sBChoiceCursorPositions2, sBChoiceCursorPositions3,
                                                      sBChoiceCursorPositions4 };

void BChoiceMain_Start(BChoiceParam *param) {
    BChoiceWork *work = GFL_HeapAllocate(param->heapId, sizeof(BChoiceWork), TRUE, "b_choice_main.c", 430);
    int i;

    GFL_TCBMgrAddTask(BtlvEffect_GetTCBManager(), BChoiceMain_Task, work, 100);
    work->param = param;
    work->paletteFade = BtlvEffect_GetPaletteFade();
    work->unk80 = 0;
    work->step = STEP_INIT;
    work->flags = 0;
    work->layout = work->param->count - 2;
    work->choice = -1;
    work->cursor = 0;
    for (i = 0; i < 2; i++) {
        work->flashFrame[i] = 0;
    }
    for (i = 0; i < WINDOW_MAX; i++) {
        work->windows[i].window = NULL;
    }
    BChoiceMain_Step(work);
}

static void BChoiceMain_Task(TCB *tcb, void *data) {
    BChoiceWork *work = data;

    if (work->param->skip == 1) {
        work->choice = 0;
        work->step = STEP_END;
    }
    BChoiceMain_Step(work);
    if (work->step == STEP_END) {
        if (BChoiceMain_Finish(tcb, work) == TRUE) {
            return;
        }
    }
    GFL_TCBExMgrUpdate(work->tcbManager);
    BChoiceMain_UpdatePrint(work);
}

static BOOL BChoiceMain_Finish(TCB *tcb, BChoiceWork *work) {
    if (!func_02021c0c(work->printQueue)) {
        if (work->param->skip == 1) {
            func_02021c44(work->printQueue);
        } else {
            return FALSE;
        }
    }
    if (!GFL_WipeIsFinished()) {
        if (work->param->skip == 1) {
            GFL_WipeForceEnd();
        } else {
            return FALSE;
        }
    }
    BChoiceMain_FreeActors(work);
    BChoiceMain_FreeBG(work);
    GFL_TCBExMgrFree(work->tcbManager);
    GFL_HeapFree(work->positions);
    BChoiceMain_FreeWindows(work);
    func_02021a18(work->printQueue);
    work->param->done = TRUE;
    func_0203d564(!(work->flags & FLAG_KEY_CHOICE));
    if (work->param->result != NULL) {
        *work->param->result = work->choice;
    }
    GFL_TCBRemove(tcb);
    GFL_HeapFree(work);
    return TRUE;
}

static void BChoiceMain_Step(BChoiceWork *work) {
    switch (work->step) {
    case STEP_INIT:
        BChoiceMain_Init(work);
        work->step = STEP_WAIT_WIPE;
        break;
    case STEP_WAIT_WIPE:
        if (!BChoiceMain_IsWiping()) {
            work->step = STEP_CHOOSE;
        }
        break;
    case STEP_CHOOSE:
        if (!BChoiceMain_UpdateInput(work)) {
            BChoiceMain_StartFlash(work);
            work->step = STEP_FLASH;
        }
        break;
    case STEP_FLASH:
        BChoiceMain_UpdateFlash(work);
        if (!BChoiceMain_IsFlashing(work)) {
            work->step = STEP_FADE_OUT;
            BChoiceMain_StartFadeOut(work);
        }
        break;
    case STEP_FADE_OUT:
        if (!BChoiceMain_IsFading(work)) {
            work->step = STEP_END;
            G2S_BlendNone();
            GXS_SetVisibleWnd(GX_WNDMASK_NONE);
        }
        break;
    case STEP_END:
        break;
    }
}

static void BChoiceMain_Init(BChoiceWork *work) {
    G2S_BlendNone();
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    work->tcbManager = GFL_TCBExMgrCreate(work->param->heapId, work->param->heapId, 1, 4);
    work->printQueue = func_02021998(work->param->heapId);
    work->positions = GFL_ArcSysReadHeapNew(ARCID_BATTLE_CHOICE, FILE_POSITIONS, work->param->heapId);
    BChoiceMain_InitBG(work);
    BChoiceMain_LoadGraphics(work, work->layout);
    BChoiceMain_CreateWindows(work, work->layout);
    BChoiceMain_PrintChoices(work);
    BChoiceMain_InitActors(work);
    BChoiceMain_StartWipe(work);
    BChoiceMain_InitPalettes(work);
    work->unk80 = 1;
    if (!func_0203d554()) {
        work->flags |= FLAG_KEYS;
        work->flags |= FLAG_REFRESH;
    }
}

static void BChoiceMain_StartWipe(BChoiceWork *work) {
    GFL_WipeSet(4, 1, 1, 0, 6, 1, work->param->heapId);
}

static BOOL BChoiceMain_IsWiping(void) {
    BOOL wiping = TRUE;

    if (GFL_WipeIsFinished() == TRUE) {
        wiping = FALSE;
    }
    return wiping;
}

static void BChoiceMain_StartFadeOut(BChoiceWork *work) {
    PaletteFade_StartFade(work->paletteFade, 10, 0xffff, 0, 0, 16, 0, BtlvEffect_GetTCBManager());
}

static BOOL BChoiceMain_IsFading(BChoiceWork *work) {
    BOOL fading = TRUE;

    if (PaletteFade_GetActiveMask(work->paletteFade) == 0) {
        fading = FALSE;
    }
    return fading;
}

static void BChoiceMain_InitBG(BChoiceWork *work) {
    BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

    GFL_BGSysSetLCDConfigForEngine(&lcdConfig, BGSYS_ENGINE_SUB);
    BGSetup bgText = { 0,
                       0,
                       0x800,
                       0,
                       BGRES_256x256,
                       GX_BG_COLORMODE_16,
                       GX_BG_SCRBASE(0xc800),
                       GX_BG_CHARBASE(0x0000),
                       0x8000,
                       GX_BG_EXTPLTT_23,
                       3,
                       GX_BG_AREAOVER_XLU,
                       FALSE };
    GFL_BGSysCreateBG(BG_TEXT, &bgText, BGMODE_TEXT);
    BGSetup bgFlash = { 0,
                        0,
                        0x800,
                        0,
                        BGRES_256x256,
                        GX_BG_COLORMODE_16,
                        GX_BG_SCRBASE(0xe800),
                        GX_BG_CHARBASE(0x18000),
                        0x8000,
                        GX_BG_EXTPLTT_23,
                        2,
                        GX_BG_AREAOVER_XLU,
                        FALSE };
    GFL_BGSysCreateBG(BG_FLASH, &bgFlash, BGMODE_TEXT);
    BGSetup bgButtons = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0x10000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          1,
                          GX_BG_AREAOVER_XLU,
                          FALSE };
    GFL_BGSysCreateBG(BG_BUTTONS, &bgButtons, BGMODE_TEXT);
    GFL_BGSysClearScr(BG_BUTTONS);
    BGSetup bgFrame = { 0,
                        0,
                        0x800,
                        0,
                        BGRES_256x256,
                        GX_BG_COLORMODE_16,
                        GX_BG_SCRBASE(0xf000),
                        GX_BG_CHARBASE(0x8000),
                        0x4000,
                        GX_BG_EXTPLTT_01,
                        0,
                        GX_BG_AREAOVER_XLU,
                        FALSE };
    GFL_BGSysCreateBG(BG_FRAME, &bgFrame, BGMODE_TEXT);
    GFL_BGSysClearScr(BG_FRAME);
    GFL_BGSysClearCharCore(BG_BUTTONS, 0x20, 0, work->param->heapId);
    GFL_BGSysClearCharCore(BG_FRAME, 0x20, 0, work->param->heapId);
    GFL_BGSysQueueScrLoad(BG_BUTTONS);
    GFL_BGSysQueueScrLoad(BG_FRAME);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_ALL, TRUE);
}

static void BChoiceMain_FreeBG(BChoiceWork *work) {
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_ALL, FALSE);
    GFL_BGSysReleaseBG(BG_FRAME);
    GFL_BGSysReleaseBG(BG_BUTTONS);
    GFL_BGSysReleaseBG(BG_FLASH);
    GFL_BGSysReleaseBG(BG_TEXT);
}

static void BChoiceMain_InitPalettes(BChoiceWork *work) {
    PaletteFade_LoadFromVRAM(work->paletteFade, 1, 0, 0xe0);
    PaletteFade_LoadNCLR(work->paletteFade, ARCID_FONT, 6, work->param->heapId, 1, 0x20, 0x80);
    func_02042ba8(FALSE, work->param->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, 3, 0, 0x20);
    PaletteFade_LoadFromVRAM(work->paletteFade, 3, 0xe0, 0x20);
    PaletteFade_StartFade(work->paletteFade, 10, 0xffff, 0, 16, 0, 0, BtlvEffect_GetTCBManager());
}

static BOOL BChoiceMain_UpdateInput(BChoiceWork *work) {
    return BChoiceMain_ReadInput(work);
}

// Returns whether the player is still choosing
static BOOL BChoiceMain_ReadInput(BChoiceWork *work) {
    s32 touchIndex = 0;
    BOOL touched = FALSE;
    BOOL choosing = TRUE;
    u32 input = 0;
    BOOL refresh = FALSE;

    if (work->flags & FLAG_REFRESH) {
        refresh = TRUE;
        work->flags &= ~FLAG_REFRESH;
    }
    if (!refresh) {
        touchIndex = BChoiceMain_GetTouched(work, work->layout);
        if (touchIndex >= 0 && touchIndex < work->layout + 2) {
            input = 1;
            touched = TRUE;
        }
        if (input == 0) {
            if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
                input = 1;
            } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
                input = 2;
            } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
                input = 3;
            }
        }
    }
    if (input != 0) {
        if (!touched) {
            if (work->flags & FLAG_KEYS) {
                if (input == 1) {
                    if (work->cursor != 0) {
                        GFL_SndSEPlay(SEQ_SE_SELECT1);
                    }
                    work->cursor--;
                    if (work->cursor < 0) {
                        work->cursor = 0;
                    }
                    refresh = TRUE;
                } else if (input == 2) {
                    if (work->cursor != work->layout + 1) {
                        GFL_SndSEPlay(SEQ_SE_SELECT1);
                    }
                    work->cursor++;
                    if (work->cursor > work->layout + 1) {
                        work->cursor = work->layout + 1;
                    }
                    refresh = TRUE;
                } else if (input == 3) {
                    work->choice = work->cursor;
                    choosing = FALSE;
                    work->flags |= FLAG_KEY_CHOICE;
                    GFL_SndSEPlay(SEQ_SE_DECIDE2);
                    refresh = TRUE;
                }
            } else {
                work->flags |= FLAG_KEYS;
                refresh = TRUE;
            }
        } else {
            choosing = FALSE;
            work->choice = touchIndex;
            GFL_SndSEPlay(SEQ_SE_DECIDE2);
            refresh = TRUE;
        }
    }
    if (refresh == TRUE) {
        BChoiceMain_UpdateCursor(work, choosing, work->layout, work->cursor);
    }
    return choosing;
}

static void BChoiceMain_LoadGraphics(BChoiceWork *work, int layout) {
    int i;
    const BChoicePaletteLoad *paletteLoad = sBChoicePaletteLoads;
    const BChoiceBGLoad *charLoad = sBChoiceCharLoads;
    const BChoiceBGLoad *screenLoad = sBChoiceScreenLoadTables[layout];
    u32 heapId = work->param->heapId;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_BATTLE_CHOICE, HEAPID_TAIL(heapId));

    for (i = 0; i < 8; i++) {
        if (paletteLoad->type == 0xff) {
            break;
        }
        GFL_G2DIOLoadArcNCLRDefault(arc, paletteLoad->fileId, paletteLoad->type, paletteLoad->offset,
                                    paletteLoad->count * 0x20, heapId);
        paletteLoad++;
    }
    for (i = 0; i < 8; i++) {
        if (charLoad->bg == 0xff) {
            break;
        }
        GFL_BGSysLoadArcNCGRStatic(arc, charLoad->fileId, charLoad->bg, charLoad->offset, 0, FALSE, heapId);
        charLoad++;
    }
    for (i = 0; i < 8; i++) {
        if (screenLoad->bg == 0xff) {
            break;
        }
        loadBGScrToVramByFileNoReserveNegAlign(arc, screenLoad->fileId, screenLoad->bg, screenLoad->offset, 0, FALSE,
                                               heapId);
        screenLoad++;
    }
    GFL_ArcToolFree(arc);
}

static void BChoiceMain_CreateWindows(BChoiceWork *work, int layout) {
    int i;
    const BChoiceWindowSetup *setup = sBChoiceWindowSetupTables[layout];

    for (i = 0; i < WINDOW_MAX; i++) {
        if (setup->bg == 0xff) {
            break;
        }
        work->windows[i].window =
            BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, TRUE);
        BmpWin_FlushMap(work->windows[i].window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(work->windows[i].window));
        setup++;
    }
}

static void BChoiceMain_FreeWindows(BChoiceWork *work) {
    int i;

    for (i = 0; i < WINDOW_MAX; i++) {
        if (work->windows[i].window != NULL) {
            BmpWin_Free(work->windows[i].window);
            work->windows[i].window = NULL;
        }
    }
}

static void BChoiceMain_PrintText(BChoiceWork *work, u32 window, int x, int y, StrBuf *str, u32 align, u16 color) {
    u16 offset = BChoiceMain_GetAlignOffset(str, work->param->font, align);
    PrintQueue *printQueue = work->printQueue;

    func_02021c7c(printQueue, BmpWin_GetBitmap(work->windows[window].window), x - offset, y, str, work->param->font,
                  color);
    work->windows[window].dirty = TRUE;
}

static u16 BChoiceMain_GetAlignOffset(StrBuf *str, Font *font, u32 align) {
    if (align == ALIGN_RIGHT) {
        return GFL_FontGetBlockWidth(str, font, 0);
    }
    if (align == ALIGN_CENTER) {
        return GFL_FontGetBlockWidth(str, font, 0) >> 1;
    }
    return 0;
}

static void BChoiceMain_UpdatePrint(BChoiceWork *work) {
    int i;

    func_02021a3c(work->printQueue);
    for (i = 0; i < WINDOW_MAX; i++) {
        PrintQueue *printQueue = work->printQueue;

        if (work->windows[i].dirty) {
            if (!func_02021c1c(printQueue, BmpWin_GetBitmap(work->windows[i].window))) {
                BmpWin_FlushChar(work->windows[i].window);
                work->windows[i].dirty = FALSE;
            }
        }
    }
}

static s32 BChoiceMain_GetTouched(BChoiceWork *work, int layout) {
    return func_0203da0c(sBChoiceTouchRectTables[layout]);
}

static void BChoiceMain_PrintChoices(BChoiceWork *work) {
    int i;

    for (i = 0; i < work->param->count; i++) {
        if (work->param->strs[i] != NULL) {
            BChoiceMain_PrintText(work, i, TEXT_X, 0, work->param->strs[i], ALIGN_CENTER, TEXT_COLOR);
        }
    }
}

static void BChoiceMain_InitActors(BChoiceWork *work) {
    int i;

    work->unit = func_0204bf1c(ACTOR_COUNT, 0, HEAPID_TAIL(work->param->heapId));
    BChoiceMain_LoadActorResources(work);
    BChoiceMain_CreateActors(work, CLACT_SURFACE_SUB, sBChoiceActorSetups, sBChoiceActorResources);
    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c378(work->actors[i], 0, TRUE);
    }
}

static void BChoiceMain_FreeActors(BChoiceWork *work) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c108(work->actors[i]);
    }
    func_0204bcd0(work->resources[0]);
    func_0204b98c(work->resources[1]);
    func_0204be64(work->resources[2]);
    func_0204bf98(work->unit);
}

static void BChoiceMain_LoadActorResources(BChoiceWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_BATTLE_CURSOR, HEAPID_TAIL(work->param->heapId));

    work->resources[0] = func_0204bbb8(arc, FILE_CURSOR_PALETTE, 1, 0, 4, 1, work->param->heapId);
    work->resources[2] = func_0204bde0(arc, FILE_CURSOR_CELLS, FILE_CURSOR_ANIMS, work->param->heapId);
    work->resources[1] = func_0204b81c(arc, FILE_CURSOR_CHARS, FALSE, 1, work->param->heapId);
    GFL_ArcToolFree(arc);
}

static void BChoiceMain_CreateActors(BChoiceWork *work, u32 surface, const BChoiceActorSetup *setups,
                                     const s16 *resources) {
    int i;
    ClActUnit *unit = work->unit;
    ClActorSetup setup;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    ClActor **actors = work->actors;
    s16 palette = resources[0];
    s16 chars = resources[1];
    s16 cells = resources[2];
    s16 count = resources[3];
    for (i = 0; i < count; i++) {
        if (setups->sequence == 0xff) {
            break;
        }
        u32 position = setups->position * 2;
        s16 x = BChoiceMain_GetPosition(work, position);
        s16 y = BChoiceMain_GetPosition(work, position + 1);
        BOOL visible;
        BOOL animated;

        setup.x = x;
        setup.y = y;
        setup.sequence = setups->sequence;
        setup.priority = setups->priority;
        setup.bgPriority = setups->bgPriority;
        actors[i] = func_0204c040(unit, work->resources[chars], work->resources[palette], work->resources[cells],
                                  &setup, surface, work->param->heapId);
        visible = FALSE;
        if (setups->visible == 1) {
            visible = TRUE;
        }
        func_0204c124(actors[i], visible);
        animated = FALSE;
        if (setups->flags & 1) {
            animated = TRUE;
        }
        func_0204c520(actors[i], animated);
        if (setups->flags & 2) {
            func_0204c2b0(actors[i], 1, TRUE);
        }
        if (setups->flags & 4) {
            func_0204c2b0(actors[i], 0, TRUE);
        }
        setups++;
    }
}

static s16 BChoiceMain_GetPosition(BChoiceWork *work, u32 index) {
    if (index >= POSITION_COUNT) {
        return 0;
    }
    return work->positions[index];
}

static void BChoiceMain_SetCursorVisible(BChoiceWork *work, u32 unused, int index, BOOL visible) {
    if (index < ACTOR_COUNT) {
        func_0204c124(work->actors[index], visible);
    }
}

static void BChoiceMain_UpdateCursor(BChoiceWork *work, BOOL show, int layout, int choice) {
    const s16 *positions = sBChoiceCursorPositionTables[layout];
    ClActorPos pos;
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        BChoiceMain_SetCursorVisible(work, 0, i, show);
    }
    if (show == TRUE) {
        const s16 *indices = positions + choice * 4;

        for (i = 0; i < ACTOR_COUNT; i++) {
            s16 index = indices[i];

            pos.x = BChoiceMain_GetPosition(work, (u32)index * 2);
            pos.y = BChoiceMain_GetPosition(work, index * 2 + 1);
            func_0204c140(work->actors[i], &pos, CLACT_SURFACE_SUB);
        }
    }
}

static void BChoiceMain_SetChoicePalette(BChoiceWork *work, int layout, int choice, int palette) {
    s16 pal;
    const BChoiceRect *rects = sBChoiceChoiceRectTables[layout];
    s16 x = rects[choice].x;
    s16 y = rects[choice].y;
    s16 width = rects[choice].width;
    s16 height = rects[choice].height;

    pal = palette;
    if (pal == PALETTE_DEFAULT) {
        pal = sBChoiceChoicePalettes[choice];
    }
    GFL_BGSysSetScrPaletteNo(BG_FLASH, x, y, width, height, pal);
    GFL_BGSysLoadScr(BG_FLASH);
}

static void BChoiceMain_StartFlash(BChoiceWork *work) {
    int i = 0;

    work->flags |= FLAG_FLASHING;
    work->flashFrame[0] = 0;
    work->flashStep = 0;
    if (work->layout + 2 > 0) {
        do {
            if (i != work->choice) {
                BChoiceMain_SetChoicePalette(work, work->layout, i, PALETTE_FLASH);
            }
            i++;
        } while (i < work->layout + 2);
    }
    BChoiceMain_UpdateFlash(work);
}

static void BChoiceMain_UpdateFlash(BChoiceWork *work) {
    const TouchRect *rects;
    u16 blend;

    if (!(work->flags & FLAG_FLASHING)) {
        return;
    }
    work->flashFrame[0]++;
    if (work->flashFrame[0] == sBChoiceFlashSteps[work->flashStep].frame) {
        if (sBChoiceFlashSteps[work->flashStep].palette != PALETTE_KEEP) {
            BChoiceMain_SetChoicePalette(work, work->layout, work->choice, sBChoiceFlashSteps[work->flashStep].palette);
        }
        work->flashStep++;
        if (sBChoiceFlashSteps[work->flashStep].frame == -1) {
            work->flags &= ~FLAG_FLASHING;
            work->flashFrame[0] = 0;
        }
    }
    rects = sBChoiceTouchRectTables[work->layout];
    if (work->blend < 16) {
        work->blend++;
    }
    blend = work->blend;
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, 6, 0x39, 16 - blend, blend + 15);
    G2S_SetWnd0InsidePlane(0x1f, FALSE);
    if (work->blend > 10) {
        G2S_SetWndOutsidePlane(0x1d, TRUE);
    } else {
        G2S_SetWndOutsidePlane(0x1f, TRUE);
    }
    GXS_SetVisibleWnd(GX_WNDMASK_W0);
    G2S_SetWnd0Position(0, rects[work->choice].top, 255, rects[work->choice].bottom);
}

static BOOL BChoiceMain_IsFlashing(BChoiceWork *work) {
    BOOL flashing = FALSE;

    if (work->flags & FLAG_FLASHING) {
        flashing = TRUE;
    }
    return flashing;
}
