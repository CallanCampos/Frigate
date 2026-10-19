#include "types.h"
#include "app/medal_info.h"
#include "app/medal_info_viewer.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/version.h"
#include "system/wordset.h"
#include "text/system/medal_info_viewer.h"

// The viewer of the medal screens, named after the ROM's "medal_info_viewer.c". The names are ours

// The archives of the medal screens. Our names; the archives hold, in order, each medal's picture data, the screen's BG
// graphics and its cell actors' graphics
#define ARCID_MEDAL_INFO_MEDAL_DATA 235

// The BGs, in the order of sMedalInfoBGSetups
enum {
    MEDAL_INFO_BG_MEDAL,
    MEDAL_INFO_BG_PANEL,
    MEDAL_INFO_BG_BUTTONS,
    MEDAL_INFO_BG_TEXT,
    MEDAL_INFO_BG_SUB,
    MEDAL_INFO_BG_COUNT,
};

// The cell actors, in the order of MedalInfoViewer.actors
enum {
    MEDAL_INFO_ACTOR_MEDAL,
    MEDAL_INFO_ACTOR_EXIT,
    MEDAL_INFO_ACTOR_BACK,
    MEDAL_INFO_ACTOR_PREV,
    MEDAL_INFO_ACTOR_NEXT,
    MEDAL_INFO_ACTOR_COUNT,
};

// The sequences of the buttons' cell actors
enum {
    MEDAL_INFO_SEQ_BACK = 1,
    MEDAL_INFO_SEQ_NEXT,
    MEDAL_INFO_SEQ_PREV,
    MEDAL_INFO_SEQ_EXIT_PRESSED = 8,
    MEDAL_INFO_SEQ_BACK_PRESSED,
    MEDAL_INFO_SEQ_NEXT_PRESSED,
    MEDAL_INFO_SEQ_PREV_PRESSED,
    MEDAL_INFO_SEQ_NEXT_OFF = 16,
    MEDAL_INFO_SEQ_PREV_OFF,
};

// The states of the viewer
enum {
    MEDAL_INFO_STATE_WAIT_PRINT,
    MEDAL_INFO_STATE_FADE_IN,
    MEDAL_INFO_STATE_INPUT,
    MEDAL_INFO_STATE_WAIT_BUTTON,
    MEDAL_INFO_STATE_FADE_OUT,
    MEDAL_INFO_STATE_DONE,
    MEDAL_INFO_STATE_CHANGE_ENTRY,
};

// The result of the screen: the button that closed it
enum {
    MEDAL_INFO_RESULT_BACK,
    MEDAL_INFO_RESULT_FIELD,
};

// The touch rectangles
enum {
    MEDAL_INFO_TOUCH_BACK,
    MEDAL_INFO_TOUCH_FIELD,
    MEDAL_INFO_TOUCH_PREV,
};

// The colors of the text
#define MEDAL_INFO_COLOR_TEXT PRINT_COLOR(1, 2, 0)
#define MEDAL_INFO_COLOR_LABEL PRINT_COLOR(3, 4, 0)

typedef struct {
    u32 cellAnims;
    u32 palette;
    u32 chars;
} MedalInfoClActRes;

// What a medal's file of ARCID_MEDAL_INFO_MEDAL_DATA holds, as far as the viewer reads it
typedef struct {
    u8 unk0[9];
    // The sequence of the medal's cell actor
    u8 sequence;
    u8 unkA[2];
} MedalInfoMedalData;

struct MedalInfoViewer {
    HeapID heapId;
    TCB *vblankTask;
    ClActUnit *clActUnit;
    ClActor *actors[MEDAL_INFO_ACTOR_COUNT];
    // The graphics of the medal, and of the touch buttons
    MedalInfoClActRes res[2];
    // Whether the shown entry has a medal
    BOOL hasMedal;
    u16 state;
    u16 subState;
    PrintQueue *printQueue;
    BmpWin *window;
    PrintWindow printWindow;
    Font *fontText;
    Font *fontLabel;
    MsgData *msgMain;
    MsgData *msgMedalNames;
    MsgData *msgDescriptions;
    // Whether the window's text is still being printed
    BOOL printing;
    // The scroll of the background, in fixed point
    s32 scroll;
    // How the viewer was closed
    int result;
    MedalInfoViewerParam *param;
    MedalInfoEntry *entry;
    // The place of the shown entry in the param's entries
    s32 index;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
} MedalInfoBGSetup;

static void MedalInfoViewer_HandleInput(MedalInfoViewer *viewer);
static void MedalInfoViewer_ExitToField(MedalInfoViewer *viewer);
static void MedalInfoViewer_Back(MedalInfoViewer *viewer);
static void MedalInfoViewer_Prev(MedalInfoViewer *viewer);
static void MedalInfoViewer_Next(MedalInfoViewer *viewer);
static void MedalInfoViewer_ShowNext(MedalInfoViewer *viewer);
static void MedalInfoViewer_ShowPrev(MedalInfoViewer *viewer);
static void MedalInfoViewer_ChangeEntry(MedalInfoViewer *viewer);
static void MedalInfoViewer_SetEntry(MedalInfoViewer *viewer, u32 index);
static void MedalInfoViewer_SetState(MedalInfoViewer *viewer, u16 state);
static void MedalInfoViewer_ScrollBG(MedalInfoViewer *viewer);
static void MedalInfoViewer_Init(MedalInfoViewer *viewer, MedalInfoViewerParam *param);
static void MedalInfoViewer_FreeText(MedalInfoViewer *viewer);
static void MedalInfoViewer_GraphicInit(MedalInfoViewer *viewer);
static void MedalInfoViewer_BGInit(MedalInfoViewer *viewer);
static u32 MedalInfoViewer_GetBG(int index);
static void MedalInfoViewer_GraphicExit(MedalInfoViewer *viewer);
static void MedalInfoViewer_BGExit(MedalInfoViewer *viewer);
static void MedalInfoViewer_WindowInit(MedalInfoViewer *viewer);
static void MedalInfoViewer_WindowExit(MedalInfoViewer *viewer);
static void MedalInfoViewer_PrintQueueInit(MedalInfoViewer *viewer);
static void MedalInfoViewer_PrintQueueExit(MedalInfoViewer *viewer);
static void MedalInfoViewer_PrintMedal(MedalInfoViewer *viewer);
static void MedalInfoViewer_PrintNoMedal(MedalInfoViewer *viewer);
static void MedalInfoViewer_ClActInit(MedalInfoViewer *viewer);
static void MedalInfoViewer_ClActExit(MedalInfoViewer *viewer);
static void MedalInfoViewer_VBlank(TCB *tcb, void *data);

static const TouchRect sMedalInfoTouchRects[] = {
    { 168, 192, 232, 0 },
    { 168, 192, 200, 224 },
    { 168, 192, 168, 192 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const BGSysLCDConfig sMedalInfoLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const BGSysVRAMConfig sMedalInfoVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const MedalInfoBGSetup sMedalInfoBGSetups[MEDAL_INFO_BG_COUNT] = {
    { 0,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x4000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 1,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x04000), 0x4000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 2,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x08000), 0x2000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 3,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 4,
      { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
};

MedalInfoViewer *MedalInfoViewer_Create(MedalInfoViewerParam *param) {
    MedalInfoViewer *viewer;
    HeapID heapId = param->heapId;

    viewer = GFL_HeapAllocate(heapId, sizeof(MedalInfoViewer), FALSE, "medal_info_viewer.c", 148);

    MI_CpuFill8(viewer, 0, sizeof(MedalInfoViewer));
    viewer->heapId = heapId;
    MedalInfoViewer_Init(viewer, param);
    MedalInfoViewer_GraphicInit(viewer);
    MedalInfoViewer_PrintQueueInit(viewer);
    func_02042ba8(TRUE, viewer->heapId);
    if (viewer->hasMedal == TRUE) {
        MedalInfoViewer_PrintMedal(viewer);
    } else {
        MedalInfoViewer_PrintNoMedal(viewer);
    }
    return viewer;
}

BOOL MedalInfoViewer_Main(MedalInfoViewer *viewer) {
    BOOL done = FALSE;

    switch (viewer->state) {
    case MEDAL_INFO_STATE_WAIT_PRINT:
        if (!viewer->printing) {
            BmpWin_FlushChar(viewer->window);
            GFL_BGSysQueueScrLoad(MedalInfoViewer_GetBG(MEDAL_INFO_BG_TEXT));
            GFL_FadeSet(3, 16, 0, 0);
            MedalInfoViewer_SetState(viewer, MEDAL_INFO_STATE_FADE_IN);
        }
        break;
    case MEDAL_INFO_STATE_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            MedalInfoViewer_SetState(viewer, MEDAL_INFO_STATE_INPUT);
        }
        break;
    case MEDAL_INFO_STATE_INPUT:
        MedalInfoViewer_ScrollBG(viewer);
        MedalInfoViewer_HandleInput(viewer);
        break;
    case MEDAL_INFO_STATE_CHANGE_ENTRY:
        MedalInfoViewer_ScrollBG(viewer);
        MedalInfoViewer_ChangeEntry(viewer);
        break;
    case MEDAL_INFO_STATE_WAIT_BUTTON:
        if (func_0204c560(viewer->actors[MEDAL_INFO_ACTOR_BACK]) &&
            func_0204c560(viewer->actors[MEDAL_INFO_ACTOR_EXIT])) {
            break;
        }
        GFL_FadeSet(3, 0, 16, 0);
        MedalInfoViewer_SetState(viewer, MEDAL_INFO_STATE_FADE_OUT);
        break;
    case MEDAL_INFO_STATE_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            MedalInfoViewer_SetState(viewer, MEDAL_INFO_STATE_DONE);
        }
        break;
    case MEDAL_INFO_STATE_DONE:
        viewer->param->selected = viewer->index;
        done = TRUE;
        break;
    }
    viewer->printing = FALSE;
    func_02021a3c(viewer->printQueue);
    PrintWindow_Flush(&viewer->printWindow, viewer->printQueue);
    if (PrintWindow_IsPrinted(&viewer->printWindow) == FALSE) {
        viewer->printing = TRUE;
    }
    func_0204b794();
    return done;
}

u32 MedalInfoViewer_Delete(MedalInfoViewer *viewer) {
    int result = viewer->result;

    MedalInfoViewer_PrintQueueExit(viewer);
    MedalInfoViewer_GraphicExit(viewer);
    MedalInfoViewer_FreeText(viewer);
    GFL_HeapFree(viewer);
    return result;
}

static void MedalInfoViewer_HandleInput(MedalInfoViewer *viewer) {
    u32 keys = GCTX_HIDGetPressedKeys();
    s32 touch;

    if (viewer->param->mode == MEDAL_INFO_VIEWER_MODE_LIST) {
        if (keys & PAD_KEY_UP) {
            MedalInfoViewer_Prev(viewer);
            return;
        } else if (keys & PAD_KEY_DOWN) {
            MedalInfoViewer_Next(viewer);
            return;
        }
    } else if (viewer->param->mode == MEDAL_INFO_VIEWER_MODE_BACK_OR_FIELD) {
        if (keys & PAD_BUTTON_X) {
            MedalInfoViewer_ExitToField(viewer);
            func_0203d564(FALSE);
            return;
        }
    }
    if (keys & PAD_BUTTON_B) {
        MedalInfoViewer_Back(viewer);
        func_0203d564(FALSE);
        return;
    }
    touch = func_0203da0c(sMedalInfoTouchRects);
    if (viewer->param->mode == MEDAL_INFO_VIEWER_MODE_BACK_OR_FIELD) {
        switch (touch) {
        case MEDAL_INFO_TOUCH_BACK:
            MedalInfoViewer_Back(viewer);
            func_0203d564(FALSE);
            break;
        case MEDAL_INFO_TOUCH_FIELD:
            MedalInfoViewer_ExitToField(viewer);
            func_0203d564(TRUE);
            break;
        }
    } else if (viewer->param->mode == MEDAL_INFO_VIEWER_MODE_BACK) {
        if (touch == MEDAL_INFO_TOUCH_BACK) {
            MedalInfoViewer_Back(viewer);
            func_0203d564(FALSE);
        }
    } else if (viewer->param->mode == MEDAL_INFO_VIEWER_MODE_LIST) {
        switch (touch) {
        case MEDAL_INFO_TOUCH_BACK:
            MedalInfoViewer_Back(viewer);
            func_0203d564(FALSE);
            break;
        case MEDAL_INFO_TOUCH_PREV:
            MedalInfoViewer_Prev(viewer);
            break;
        case MEDAL_INFO_TOUCH_FIELD:
            MedalInfoViewer_Next(viewer);
            break;
        }
    }
}

static void MedalInfoViewer_ExitToField(MedalInfoViewer *viewer) {
    func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_EXIT], MEDAL_INFO_SEQ_EXIT_PRESSED);
    func_0204c520(viewer->actors[MEDAL_INFO_ACTOR_EXIT], TRUE);
    MedalInfoViewer_SetState(viewer, MEDAL_INFO_STATE_WAIT_BUTTON);
    GFL_SndSEPlay(SEQ_SE_CLOSE1);
    viewer->result = MEDAL_INFO_RESULT_FIELD;
}

static void MedalInfoViewer_Back(MedalInfoViewer *viewer) {
    func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_BACK], MEDAL_INFO_SEQ_BACK_PRESSED);
    func_0204c520(viewer->actors[MEDAL_INFO_ACTOR_BACK], TRUE);
    MedalInfoViewer_SetState(viewer, MEDAL_INFO_STATE_WAIT_BUTTON);
    GFL_SndSEPlay(SEQ_SE_CANCEL1);
    viewer->result = MEDAL_INFO_RESULT_BACK;
}

static void MedalInfoViewer_Prev(MedalInfoViewer *viewer) {
    if (viewer->index > 0 && viewer->param->count > 1) {
        func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_PREV], MEDAL_INFO_SEQ_PREV_PRESSED);
        func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_NEXT], MEDAL_INFO_SEQ_NEXT_OFF);
        func_0204c520(viewer->actors[MEDAL_INFO_ACTOR_PREV], TRUE);
        func_0204c4d4(viewer->actors[MEDAL_INFO_ACTOR_PREV], 0);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        MedalInfoViewer_ShowPrev(viewer);
    }
}

static void MedalInfoViewer_Next(MedalInfoViewer *viewer) {
    if (viewer->param->count > viewer->index + 1) {
        func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_NEXT], MEDAL_INFO_SEQ_NEXT_PRESSED);
        func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_PREV], MEDAL_INFO_SEQ_PREV_OFF);
        func_0204c520(viewer->actors[MEDAL_INFO_ACTOR_NEXT], TRUE);
        func_0204c4d4(viewer->actors[MEDAL_INFO_ACTOR_NEXT], 0);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        MedalInfoViewer_ShowNext(viewer);
    }
}

static void MedalInfoViewer_ShowNext(MedalInfoViewer *viewer) {
    viewer->index++;
    if (viewer->index >= viewer->param->count) {
        viewer->index = 0;
    }
    viewer->entry = &viewer->param->entries[viewer->index];
    MedalInfoViewer_SetState(viewer, MEDAL_INFO_STATE_CHANGE_ENTRY);
}

static void MedalInfoViewer_ShowPrev(MedalInfoViewer *viewer) {
    viewer->index--;
    if (viewer->index < 0) {
        viewer->index = viewer->param->count - 1;
    }
    viewer->entry = &viewer->param->entries[viewer->index];
    MedalInfoViewer_SetState(viewer, MEDAL_INFO_STATE_CHANGE_ENTRY);
}

static void MedalInfoViewer_ChangeEntry(MedalInfoViewer *viewer) {
    MedalInfoMedalData data;
    ArcTool *arc;

    switch (viewer->subState) {
    case 0:
        if (func_0204c560(viewer->actors[MEDAL_INFO_ACTOR_PREV]) &&
            func_0204c560(viewer->actors[MEDAL_INFO_ACTOR_NEXT])) {
            return;
        }
        if (viewer->index <= 0) {
            func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_PREV], MEDAL_INFO_SEQ_PREV_OFF);
        } else {
            func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_PREV], MEDAL_INFO_SEQ_PREV);
        }
        if (viewer->param->count == viewer->index + 1) {
            func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_NEXT], MEDAL_INFO_SEQ_NEXT_OFF);
        } else {
            func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_NEXT], MEDAL_INFO_SEQ_NEXT);
        }
        GFL_BitmapFill(BmpWin_GetBitmap(viewer->window), 0);
        viewer->subState++;
        // fallthrough
    case 1:
        if (viewer->entry->medal >= MEDAL_INFO_NO_MEDAL) {
            viewer->hasMedal = FALSE;
        } else {
            viewer->hasMedal = TRUE;
        }
        arc = GFL_ArcSysCreateFileHandle(ARCID_MEDAL_INFO_MEDAL_DATA, viewer->heapId);
        if (viewer->hasMedal == TRUE) {
            GFL_ArcToolRead(arc, viewer->entry->medal, &data);
            func_0204c124(viewer->actors[MEDAL_INFO_ACTOR_MEDAL], TRUE);
        } else {
            func_0204c124(viewer->actors[MEDAL_INFO_ACTOR_MEDAL], FALSE);
            GFL_ArcToolRead(arc, 0, &data);
        }
        func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_MEDAL], data.sequence);
        GFL_ArcToolFree(arc);
        if (viewer->hasMedal == TRUE) {
            MedalInfoViewer_PrintMedal(viewer);
        } else {
            MedalInfoViewer_PrintNoMedal(viewer);
        }
        arc = GFL_ArcSysCreateFileHandle(ARCID_MEDAL_INFO_VIEWER, viewer->heapId);
        if (viewer->hasMedal == TRUE) {
            loadBGScrToVramByFileNoReserveNegAlign(arc, 5, MedalInfoViewer_GetBG(MEDAL_INFO_BG_PANEL), 0, 0, FALSE,
                                                   viewer->heapId);
        } else {
            loadBGScrToVramByFileNoReserveNegAlign(arc, 4, MedalInfoViewer_GetBG(MEDAL_INFO_BG_PANEL), 0, 0, FALSE,
                                                   viewer->heapId);
        }
        GFL_ArcToolFree(arc);
        viewer->printing = TRUE;
        viewer->subState++;
        // fallthrough
    case 2:
        if (!viewer->printing) {
            BmpWin_FlushChar(viewer->window);
            GFL_BGSysQueueScrLoad(MedalInfoViewer_GetBG(MEDAL_INFO_BG_TEXT));
            MedalInfoViewer_SetState(viewer, MEDAL_INFO_STATE_INPUT);
        }
        break;
    }
}

static void MedalInfoViewer_SetEntry(MedalInfoViewer *viewer, u32 index) {
    viewer->index = index;
    viewer->entry = &viewer->param->entries[index];
}

static void MedalInfoViewer_SetState(MedalInfoViewer *viewer, u16 state) {
    viewer->state = state;
    viewer->subState = 0;
}

static void MedalInfoViewer_ScrollBG(MedalInfoViewer *viewer) {
    viewer->scroll += FX32_HALF;
    if (viewer->scroll >> FX32_SHIFT > 256) {
        viewer->scroll = 0;
    }
    GFL_BGSysMoveBGReq(MedalInfoViewer_GetBG(MEDAL_INFO_BG_MEDAL), BG_MOVE_SET_X, -(viewer->scroll >> FX32_SHIFT));
    GFL_BGSysMoveBGReq(MedalInfoViewer_GetBG(MEDAL_INFO_BG_SUB), BG_MOVE_SET_X, -(viewer->scroll >> FX32_SHIFT));
}

static void MedalInfoViewer_Init(MedalInfoViewer *viewer, MedalInfoViewerParam *param) {
    HeapID heapId;

    viewer->param = param;
    heapId = viewer->heapId;
    MedalInfoViewer_SetEntry(viewer, param->startIndex);
    if (viewer->entry->medal >= MEDAL_INFO_NO_MEDAL) {
        viewer->hasMedal = FALSE;
    } else {
        viewer->hasMedal = TRUE;
    }
    viewer->fontText = GFL_FontCreate(ARCID_FONT, 0, 0, 0, heapId);
    viewer->fontLabel = GFL_FontCreate(ARCID_FONT, 2, 0, 0, heapId);
    viewer->msgMain = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MEDAL_INFO_VIEWER, heapId);
    viewer->msgMedalNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MEDAL_NAMES, heapId);
    viewer->msgDescriptions = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_11, heapId);
    viewer->printing = TRUE;
    viewer->scroll = 0;
}

static void MedalInfoViewer_FreeText(MedalInfoViewer *viewer) {
    GFL_MsgDataFree(viewer->msgMain);
    GFL_MsgDataFree(viewer->msgDescriptions);
    GFL_MsgDataFree(viewer->msgMedalNames);
    GFL_FontFree(viewer->fontLabel);
    GFL_FontFree(viewer->fontText);
}

static void MedalInfoViewer_GraphicInit(MedalInfoViewer *viewer) {
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sMedalInfoVRAMConfig);
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    func_020232d0();
    MedalInfoViewer_BGInit(viewer);
    MedalInfoViewer_ClActInit(viewer);
    MedalInfoViewer_WindowInit(viewer);
    viewer->vblankTask = GFL_VBlankTCBAdd(MedalInfoViewer_VBlank, viewer, 0);
}

static void MedalInfoViewer_BGInit(MedalInfoViewer *viewer) {
    HeapID heapId = viewer->heapId;
    ArcTool *bgArc;
    ArcTool *commonArc;
    u8 rank;
    int i;

    GFL_BGSysCreate(heapId);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG0, 16, 5);
    GFL_BGSysSetLCDConfig(&sMedalInfoLCDConfig);
    for (i = 0; i < MEDAL_INFO_BG_COUNT; i++) {
        GFL_BGSysCreateBG(sMedalInfoBGSetups[i].bg, &sMedalInfoBGSetups[i].setup, sMedalInfoBGSetups[i].mode);
        GFL_BGSysSetBGEnabled(sMedalInfoBGSetups[i].bg, TRUE);
    }
    bgArc = GFL_ArcSysCreateFileHandle(ARCID_MEDAL_INFO_VIEWER, heapId);
    commonArc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, heapId);
    if (viewer->param->mode != MEDAL_INFO_VIEWER_MODE_LIST) {
        rank = viewer->entry->rank;
        if (rank == 4 && getGameVersion() == VERSION_BLACK2) {
            rank++;
        }
    } else {
        rank = 7;
    }
    GFL_G2DIOLoadArcNCLR(bgArc, 0, 0, rank * 32, 0, 0x20, heapId);
    GFL_G2DIOLoadArcNCLRDefault(commonArc, 27, 0, 0x20, 0x20, heapId);
    GFL_G2DIOLoadArcNCLR(bgArc, 0, 0, 0xc0, 0x40, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(bgArc, 1, MedalInfoViewer_GetBG(MEDAL_INFO_BG_MEDAL), 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(commonArc, 28, MedalInfoViewer_GetBG(MEDAL_INFO_BG_BUTTONS), 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(bgArc, 2, MedalInfoViewer_GetBG(MEDAL_INFO_BG_MEDAL), 0, 0, FALSE, heapId);
    if (viewer->hasMedal == TRUE) {
        if (viewer->param->mode == MEDAL_INFO_VIEWER_MODE_LIST) {
            loadBGScrToVramByFileNoReserveNegAlign(bgArc, 5, MedalInfoViewer_GetBG(MEDAL_INFO_BG_PANEL), 0, 0, FALSE,
                                                   heapId);
        } else {
            loadBGScrToVramByFileNoReserveNegAlign(bgArc, 3, MedalInfoViewer_GetBG(MEDAL_INFO_BG_PANEL), 0, 0, FALSE,
                                                   heapId);
        }
    } else {
        loadBGScrToVramByFileNoReserveNegAlign(bgArc, 4, MedalInfoViewer_GetBG(MEDAL_INFO_BG_PANEL), 0, 0, FALSE,
                                               heapId);
    }
    loadBGScrToVramByFileNoReserveNegAlign(commonArc, 29, MedalInfoViewer_GetBG(MEDAL_INFO_BG_BUTTONS), 0, 0, FALSE,
                                           heapId);
    GFL_G2DIOLoadArcNCLR(bgArc, 0, 4, rank * 32, 0, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(bgArc, 1, MedalInfoViewer_GetBG(MEDAL_INFO_BG_SUB), 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(bgArc, 6, MedalInfoViewer_GetBG(MEDAL_INFO_BG_SUB), 0, 0, FALSE, heapId);
    GFL_ArcToolFree(bgArc);
    GFL_ArcToolFree(commonArc);
    GFL_BGSysSetScrPaletteNo(MedalInfoViewer_GetBG(MEDAL_INFO_BG_BUTTONS), 0, 0, 32, 24, 1);
    GFL_BGSysLoadScr(MedalInfoViewer_GetBG(MEDAL_INFO_BG_BUTTONS));
}

static u32 MedalInfoViewer_GetBG(int index) {
    return sMedalInfoBGSetups[index].bg;
}

static void MedalInfoViewer_GraphicExit(MedalInfoViewer *viewer) {
    GFL_TCBRemove(viewer->vblankTask);
    MedalInfoViewer_WindowExit(viewer);
    MedalInfoViewer_ClActExit(viewer);
    MedalInfoViewer_BGExit(viewer);
}

static void MedalInfoViewer_BGExit(MedalInfoViewer *viewer) {
    int i;

    for (i = 0; i < MEDAL_INFO_BG_COUNT; i++) {
        GFL_BGSysClearBG(sMedalInfoBGSetups[i].bg);
        GFL_BGSysReleaseBG(sMedalInfoBGSetups[i].bg);
    }
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, 0, 0, 31, 0);
    GFL_BGSysFree();
}

static void MedalInfoViewer_WindowInit(MedalInfoViewer *viewer) {
    BmpWin_InitAllocator(viewer->heapId);
    viewer->window = BmpWin_CreateDynamic(MedalInfoViewer_GetBG(MEDAL_INFO_BG_TEXT), 0, 0, 32, 24, 2, TRUE);
    BmpWin_FlushMap(viewer->window);
    PrintWindow_Init(&viewer->printWindow, viewer->window);
}

static void MedalInfoViewer_WindowExit(MedalInfoViewer *viewer) {
    BmpWin_Free(viewer->window);
    BmpWin_FreeAllocator();
}

static void MedalInfoViewer_PrintQueueInit(MedalInfoViewer *viewer) {
    viewer->printQueue = func_02021998(viewer->heapId);
}

static void MedalInfoViewer_PrintQueueExit(MedalInfoViewer *viewer) {
    func_02021c44(viewer->printQueue);
    func_02021a18(viewer->printQueue);
}

// Prints the title, the medal count, the medal's name and description, and for a Join Avenue visitor the date
static void MedalInfoViewer_PrintMedal(MedalInfoViewer *viewer) {
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(viewer->heapId);
    StrBuf *buf = GFL_StrBufCreate(49, viewer->heapId);
    StrBuf *fmt = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_SMedalRally);

    func_0202437c(wordSet, 0, viewer->entry->name, 0, 1, 2);
    GFL_WordSetFormatStrbuf(wordSet, buf, fmt);
    PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 8, 8, buf, viewer->fontText, MEDAL_INFO_COLOR_TEXT);
    GFL_StrBufFree(buf);
    GFL_StrBufFree(fmt);

    buf = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_MedalsReceived);
    PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 128, 29, buf, viewer->fontLabel,
                      MEDAL_INFO_COLOR_LABEL);
    GFL_StrBufFree(buf);

    buf = GFL_StrBufCreate(49, viewer->heapId);
    fmt = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_Empty_2);
    WordSetNumber(wordSet, 0, viewer->entry->count, 3, NUM_PAD_SPACE, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, buf, fmt);
    PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 216, 28, buf, viewer->fontText, MEDAL_INFO_COLOR_TEXT);
    GFL_StrBufFree(buf);
    GFL_StrBufFree(fmt);

    buf = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_FavoriteMedal);
    PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 48, 49, buf, viewer->fontLabel, MEDAL_INFO_COLOR_LABEL);
    GFL_StrBufFree(buf);

    buf = GFL_StrBufCreate(49, viewer->heapId);
    fmt = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_Empty);
    loadMedalNameToStrbuf(wordSet, 0, viewer->entry->medal);
    GFL_WordSetFormatStrbuf(wordSet, buf, fmt);
    PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 48, 64, buf, viewer->fontText, MEDAL_INFO_COLOR_TEXT);
    GFL_StrBufFree(buf);
    GFL_StrBufFree(fmt);

    if (viewer->param->mode != MEDAL_INFO_VIEWER_MODE_LIST) {
        buf = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_Received);
        PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 48, 85, buf, viewer->fontLabel,
                          MEDAL_INFO_COLOR_LABEL);
        GFL_StrBufFree(buf);

        buf = GFL_StrBufCreate(49, viewer->heapId);
        fmt = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_20);
        WordSetNumber(wordSet, 0, viewer->entry->year, 2, NUM_PAD_ZERO, TRUE);
        WordSetNumber(wordSet, 1, viewer->entry->month, 2, NUM_PAD_NONE, TRUE);
        WordSetNumber(wordSet, 2, viewer->entry->day, 2, NUM_PAD_NONE, TRUE);
        GFL_WordSetFormatStrbuf(wordSet, buf, fmt);
        PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 152, 84, buf, viewer->fontText,
                          MEDAL_INFO_COLOR_TEXT);
        GFL_StrBufFree(buf);
        GFL_StrBufFree(fmt);
    }
    buf = GFL_MsgDataLoadStrbufNew(viewer->msgDescriptions, viewer->entry->medal);
    PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 16, 108, buf, viewer->fontText, MEDAL_INFO_COLOR_TEXT);
    GFL_StrBufFree(buf);
    GFL_WordSetSystemFree(wordSet);
}

// Prints the title and the medal count of an entry with no medal
static void MedalInfoViewer_PrintNoMedal(MedalInfoViewer *viewer) {
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(viewer->heapId);
    StrBuf *buf = GFL_StrBufCreate(49, viewer->heapId);
    StrBuf *fmt = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_SMedalRally);

    func_0202437c(wordSet, 0, viewer->entry->name, 0, 1, 2);
    GFL_WordSetFormatStrbuf(wordSet, buf, fmt);
    PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 8, 8, buf, viewer->fontText, MEDAL_INFO_COLOR_TEXT);
    GFL_StrBufFree(buf);
    GFL_StrBufFree(fmt);

    buf = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_MedalsReceived);
    PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 128, 57, buf, viewer->fontLabel,
                      MEDAL_INFO_COLOR_LABEL);
    GFL_StrBufFree(buf);

    buf = GFL_StrBufCreate(49, viewer->heapId);
    fmt = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_Empty_2);
    WordSetNumber(wordSet, 0, viewer->entry->count, 3, NUM_PAD_SPACE, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, buf, fmt);
    PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 216, 56, buf, viewer->fontText, MEDAL_INFO_COLOR_TEXT);
    GFL_StrBufFree(buf);
    GFL_StrBufFree(fmt);

    buf = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_FavoriteMedal);
    PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 8, 77, buf, viewer->fontLabel, MEDAL_INFO_COLOR_LABEL);
    GFL_StrBufFree(buf);

    buf = GFL_MsgDataLoadStrbufNew(viewer->msgMain, MedalInfoViewer_Text_None);
    PrintWindow_Print(&viewer->printWindow, viewer->printQueue, 8, 92, buf, viewer->fontText, MEDAL_INFO_COLOR_TEXT);
    GFL_StrBufFree(buf);
    GFL_WordSetSystemFree(wordSet);
}

static void MedalInfoViewer_ClActInit(MedalInfoViewer *viewer) {
    HeapID heapId = viewer->heapId;
    ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };
    MedalInfoClActRes *res;
    ArcTool *arcs[2];
    ArcTool *medalArc;
    ClActorSetup actorSetup;
    MedalInfoMedalData data;
    int i;

    ClActSys_Create(&setup, &sMedalInfoVRAMConfig, heapId);
    viewer->clActUnit = func_0204bf1c(128, 1, heapId);
    func_0204c028(viewer->clActUnit);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);

    res = viewer->res;
    arcs[1] = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, heapId);
    arcs[0] = GFL_ArcSysCreateFileHandle(ARCID_MEDAL, heapId);
    res[0].cellAnims = func_0204bde0(arcs[0], 2, 3, heapId);
    res[0].palette = func_0204bbb8(arcs[0], 0, CLACT_VRAM_BOTH, 0, 0, 6, heapId);
    res[0].chars = func_0204b81c(arcs[0], 1, FALSE, CLACT_VRAM_BOTH, heapId);
    res[1].cellAnims = func_0204bde0(arcs[1], 23, 26, heapId);
    res[1].palette = func_0204bbb8(arcs[1], 19, CLACT_VRAM_MAIN, 0xc0, 0, 4, heapId);
    res[1].chars = func_0204b81c(arcs[1], 20, FALSE, CLACT_VRAM_MAIN, heapId);
    for (i = 0; i < 2; i++) {
        GFL_ArcToolFree(arcs[i]);
    }

    MI_CpuFill8(&actorSetup, 0, sizeof(ClActorSetup));
    medalArc = GFL_ArcSysCreateFileHandle(ARCID_MEDAL_INFO_MEDAL_DATA, heapId);
    if (viewer->hasMedal == TRUE) {
        GFL_ArcToolRead(medalArc, viewer->entry->medal, &data);
    } else {
        GFL_ArcToolRead(medalArc, 0, &data);
    }
    actorSetup.x = 24;
    actorSetup.y = 64;
    actorSetup.bgPriority = 1;
    actorSetup.priority = 0;
    viewer->actors[MEDAL_INFO_ACTOR_MEDAL] =
        func_0204c040(viewer->clActUnit, viewer->res[0].chars, viewer->res[0].palette, viewer->res[0].cellAnims,
                      &actorSetup, CLACT_SURFACE_MAIN, heapId);
    func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_MEDAL], data.sequence);
    GFL_ArcToolFree(medalArc);
    if (viewer->hasMedal == TRUE) {
        func_0204c124(viewer->actors[MEDAL_INFO_ACTOR_MEDAL], TRUE);
    } else {
        func_0204c124(viewer->actors[MEDAL_INFO_ACTOR_MEDAL], FALSE);
    }

    MI_CpuFill8(&actorSetup, 0, sizeof(ClActorSetup));
    actorSetup.bgPriority = 0;
    actorSetup.priority = 0;
    actorSetup.x = 232;
    actorSetup.y = 168;
    viewer->actors[MEDAL_INFO_ACTOR_BACK] =
        func_0204c040(viewer->clActUnit, viewer->res[1].chars, viewer->res[1].palette, viewer->res[1].cellAnims,
                      &actorSetup, CLACT_SURFACE_MAIN, heapId);
    func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_BACK], MEDAL_INFO_SEQ_BACK);

    MI_CpuFill8(&actorSetup, 0, sizeof(ClActorSetup));
    actorSetup.bgPriority = 0;
    actorSetup.priority = 0;
    actorSetup.x = 200;
    actorSetup.y = 168;
    viewer->actors[MEDAL_INFO_ACTOR_EXIT] =
        func_0204c040(viewer->clActUnit, viewer->res[1].chars, viewer->res[1].palette, viewer->res[1].cellAnims,
                      &actorSetup, CLACT_SURFACE_MAIN, heapId);
    func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_EXIT], 0);
    func_0204c124(viewer->actors[MEDAL_INFO_ACTOR_EXIT], TRUE);
    if (viewer->param->mode == MEDAL_INFO_VIEWER_MODE_BACK_OR_FIELD) {
        func_0204c124(viewer->actors[MEDAL_INFO_ACTOR_EXIT], TRUE);
    } else {
        func_0204c124(viewer->actors[MEDAL_INFO_ACTOR_EXIT], FALSE);
    }

    if (viewer->param->mode == MEDAL_INFO_VIEWER_MODE_LIST) {
        MI_CpuFill8(&actorSetup, 0, sizeof(ClActorSetup));
        actorSetup.bgPriority = 0;
        actorSetup.priority = 0;
        actorSetup.x = 168;
        actorSetup.y = 168;
        viewer->actors[MEDAL_INFO_ACTOR_PREV] =
            func_0204c040(viewer->clActUnit, viewer->res[1].chars, viewer->res[1].palette, viewer->res[1].cellAnims,
                          &actorSetup, CLACT_SURFACE_MAIN, heapId);
        if (viewer->index == 0) {
            func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_PREV], MEDAL_INFO_SEQ_PREV_OFF);
        } else {
            func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_PREV], MEDAL_INFO_SEQ_PREV);
        }
        func_0204c124(viewer->actors[MEDAL_INFO_ACTOR_PREV], TRUE);

        MI_CpuFill8(&actorSetup, 0, sizeof(ClActorSetup));
        actorSetup.bgPriority = 0;
        actorSetup.priority = 0;
        actorSetup.x = 200;
        actorSetup.y = 168;
        viewer->actors[MEDAL_INFO_ACTOR_NEXT] =
            func_0204c040(viewer->clActUnit, viewer->res[1].chars, viewer->res[1].palette, viewer->res[1].cellAnims,
                          &actorSetup, CLACT_SURFACE_MAIN, heapId);
        if (viewer->param->count == viewer->index + 1) {
            func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_NEXT], MEDAL_INFO_SEQ_NEXT_OFF);
        } else {
            func_0204c488(viewer->actors[MEDAL_INFO_ACTOR_NEXT], MEDAL_INFO_SEQ_NEXT);
        }
        func_0204c124(viewer->actors[MEDAL_INFO_ACTOR_NEXT], TRUE);
    }
}

static void MedalInfoViewer_ClActExit(MedalInfoViewer *viewer) {
    int i;

    for (i = 0; i < MEDAL_INFO_ACTOR_COUNT; i++) {
        if (viewer->actors[i] != NULL) {
            func_0204c108(viewer->actors[i]);
        }
    }
    func_0204be64(viewer->res[0].cellAnims);
    func_0204b98c(viewer->res[0].chars);
    func_0204bcd0(viewer->res[0].palette);
    func_0204be64(viewer->res[1].cellAnims);
    func_0204b98c(viewer->res[1].chars);
    func_0204bcd0(viewer->res[1].palette);
    if (viewer->clActUnit != NULL) {
        func_0204bf98(viewer->clActUnit);
    }
    func_0204b758();
}

static void MedalInfoViewer_VBlank(TCB *tcb, void *data) {
    func_0204b7c8();
}
