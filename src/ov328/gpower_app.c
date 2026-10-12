#include "types.h"
#include "app/pass_power.h"
#include "app/gpower_app_graphic.h"
#include "app/ui/print_msg.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "save/high_link.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_oam.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/str_tool.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/btl_main_title.h"

// The Pass Power screen: the running Pass Powers on the top screen, and on the touch screen the Pass Powers the
// player can use for Pass Orbs and the way to the Entralink. The ROM has no name for this file; "gpower_app.c" is a
// guess from its graphic file's, "gpower_app_graphic.c"

#define HEAPID_GPOWER_APP 0xa0
// The screen's graphics and its table of positions
#define ARCID_GPOWER_APP 285

#define WINDOW_COUNT 22
#define BUTTON_COUNT 4
#define POWER_BUTTON_COUNT 3
#define ACTIVE_POWER_MAX 10
#define POS_COUNT 9

// The windows: the title, the running Pass Powers' names and times, and the message
#define WINDOW_TITLE 0
#define WINDOW_ACTIVE_NAME 1
#define WINDOW_ACTIVE_TIME 11
#define WINDOW_MESSAGE 21

// The buttons, and what a touch gives beyond them
#define BUTTON_ENTRALINK 3
#define TOUCH_RETURN 4

#define ACTOR_RETURN 0
#define ACTOR_BUTTON 1

#define RES_COMMON_PALETTE 0
#define RES_COMMON_CHARS 1
#define RES_COMMON_CELLS 2
#define RES_PALETTE 3
#define RES_CHARS 4
#define RES_CELLS 5
#define RES_FONT_PALETTE 6
#define RES_COUNT 7

// The flags of the work
#define FLAG_REDRAW 0x1
#define FLAG_KEYS 0x2
#define FLAG_NO_ENTRALINK 0x4
#define FLAG_ABORTED 0x8
#define FLAG_PALETTE_ANIM 0x10

#define ACTOR_AUTO_ANIM 0x1
#define ACTOR_FLIP_Y 0x2
#define ACTOR_FLIP_X 0x4

#define MESSAGE_HIDDEN_Y (-64)

#define COLOR_TEXT PRINT_COLOR(15, 2, 0)

enum {
    SEQ_START = 0,
    SEQ_SELECT = 100,
    SEQ_RETURN = 200,
    SEQ_DECIDE = 300,
    SEQ_SHORT_MESSAGE_IN = 310,
    SEQ_SHORT_MESSAGE_WAIT = 320,
    SEQ_SHORT_MESSAGE_OUT = 330,
    SEQ_CONFIRM_MESSAGE_IN = 400,
    SEQ_CONFIRM = 410,
    SEQ_CONFIRM_FLASH = 420,
    SEQ_CONFIRM_MESSAGE_OUT = 430,
    SEQ_END = 10000,
};

typedef struct {
    HeapID heapId;
    u32 resources[RES_COUNT];
    ClActor *returnButton[1];
    ClActor *buttons[BUTTON_COUNT];
    GPowerAppGraphic *graphic;
    PassPowerParam *param;
    Font *font;
    PrintQueue *printQueue;
    MsgData *msgData[2];
    PrintWindow windows[WINDOW_COUNT];
    WordSet *wordSet;
    BmpOamSys *bmpOam;
    GFLBitmap *labelBitmaps[BUTTON_COUNT];
    BmpOamActor *labels[BUTTON_COUNT];
    AppTaskMenuWin *yesNoWins[2];
    AppTaskMenuItem yesNoItems[2];
    AppTaskMenuRes *taskMenuRes;
    int seq;
    u32 flags;
    int cursor;
    int powerCount;
    int result;
    int decideFrame;
    int cursorPalette;
    int yesNoCursor;
    // A bit for each Pass Power that costs more Pass Orbs than the player has
    u32 shortMask;
    int prices[POWER_BUTTON_COUNT];
    int messageY;
    s16 *positions;
    int paletteAngle;
    u16 paletteFrom[16];
    u16 paletteTo[16];
    u16 palette[16];
} GPowerApp;

// A palette, characters or screen of the screen's archive, and where it goes
typedef struct {
    u8 target;
    u8 file;
    u8 offset;
    u8 count;
} GPowerAppBGFile;

typedef struct {
    s16 file;
    s16 resource;
    u16 offset;
    u16 count;
} GPowerAppObjFile;

// The resources of a group of actors, how many they are and which they are
typedef struct {
    s16 palette;
    s16 chars;
    s16 cells;
    s16 count;
    s16 type;
} GPowerAppActorGroup;

typedef struct {
    u8 sequence;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 unused4;
    u8 flags;
    u8 pos;
    u8 pad;
} GPowerAppActorSetup;

typedef struct {
    u8 width;
    u8 height;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 paletteOffset;
    u8 pos;
    u8 pad;
} GPowerAppLabelSetup;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} GPowerAppWindowSetup;

typedef struct {
    s16 frame;
    s16 palette;
} GPowerAppDecideFrame;

static BOOL GPowerApp_Init(GameProc *proc, u32 *state, void *param, void *data);
static BOOL GPowerApp_Exit(GameProc *proc, u32 *state, void *param, void *data);
static BOOL GPowerApp_Main(GameProc *proc, u32 *state, void *param, void *data);
static void GPowerApp_InitWork(GPowerApp *work);
static void GPowerApp_Update(GPowerApp *work);
static void GPowerApp_FreeWork(GPowerApp *work);
static void GPowerApp_UpdateFrame(GPowerApp *work);
static void GPowerApp_UpdateSeq(GPowerApp *work);
static void GPowerApp_UpdatePrint(GPowerApp *work);
static s16 GPowerApp_GetPos(GPowerApp *work, u32 index);
static void GPowerApp_LoadBG(GPowerApp *work);
static void GPowerApp_LoadTouchBar(GPowerApp *work, u32 bg, u32 paletteType, u32 palette);
static void GPowerApp_AddActors(GPowerApp *work, u32 vramType, const GPowerAppActorSetup *setups,
                                const GPowerAppActorGroup *group);
static void GPowerApp_LoadObjResources(GPowerApp *work, u32 vramType, const GPowerAppObjFile *files);
static void GPowerApp_LoadCommonObjResources(GPowerApp *work, u32 vramType);
static void GPowerApp_InitObj(GPowerApp *work);
static void GPowerApp_FreeObj(GPowerApp *work);
static void GPowerApp_SetActorAnim(GPowerApp *work, int type, int index, u32 sequence);
static void GPowerApp_SetActorVisible(GPowerApp *work, int type, int index, BOOL visible);
static void GPowerApp_SetActorPalette(GPowerApp *work, int type, int index, u32 palette);
static BOOL GPowerApp_IsActorAnimActive(GPowerApp *work, int type, int index);
static void GPowerApp_AddLabels(GPowerApp *work, u32 vramType, const GPowerAppLabelSetup *setups, u32 paletteRes);
static void GPowerApp_PrintLabel(GPowerApp *work, int index, StrBuf *str, int x, int y);
static void GPowerApp_SetLabelVisible(GPowerApp *work, int index, BOOL visible);
static void GPowerApp_InitWindows(GPowerApp *work);
static void GPowerApp_PrintWindow(GPowerApp *work, int index, u32 x, u32 y, StrBuf *str, u32 align, u16 color);
static void GPowerApp_FreeWindows(GPowerApp *work);
static void GPowerApp_ClearWindow(GPowerApp *work, int index, u32 color);
static void GPowerApp_UpdateSelect(GPowerApp *work);
static void GPowerApp_Draw(GPowerApp *work, int mode);
static void GPowerApp_DrawActivePowers(GPowerApp *work);
static void GPowerApp_DrawPowerButtons(GPowerApp *work);
static void GPowerApp_DrawEntralinkButton(GPowerApp *work);
static int GPowerApp_CheckTouch(GPowerApp *work);
static void GPowerApp_PrintMessage(GPowerApp *work, BOOL isShort);
static void GPowerApp_SetMessageVisible(GPowerApp *work, BOOL visible);
static BOOL GPowerApp_IsAborted(GPowerApp *work);
static void GPowerApp_InitPowers(GPowerApp *work);
static void GPowerApp_InitPaletteAnim(GPowerApp *work);
static void GPowerApp_UpdatePaletteAnim(GPowerApp *work);
static BOOL GPowerApp_UpdateDecideAnim(GPowerApp *work);
static void GPowerApp_CreateYesNo(GPowerApp *work);
static void GPowerApp_FreeYesNo(GPowerApp *work);
static void GPowerApp_UpdateYesNo(GPowerApp *work);
static BOOL GPowerApp_UpdateYesNoInput(GPowerApp *work);
static BOOL GPowerApp_IsYesNoFlashFinished(GPowerApp *work);
static BOOL GPowerApp_CheckAnyInput(GPowerApp *work);
static void GPowerApp_LoadPrices(GPowerApp *work);
static BOOL GPowerApp_ScrollMessage(GPowerApp *work, BOOL out);
static void GPowerApp_SetMessagePriority(GPowerApp *work, BOOL front);

const GameProcFunctions GPOWER_APP_PROC_FUNCTIONS = { GPowerApp_Init, GPowerApp_Main, GPowerApp_Exit };

static const GPowerAppBGFile sGPowerAppPalettes[] = {
    { PALTYPE_SUB_BG, 0, 0, 3 },
    { PALTYPE_MAIN_BG, 0, 0, 3 },
    { 0xff, 0, 0, 0 },
};

static const GPowerAppBGFile sGPowerAppChars[] = {
    { 7, 2, 0, 0 },
    { 3, 2, 0, 0 },
    { 1, 2, 0, 0 },
    { 0xff, 0, 0, 0 },
};

// The screen's messages
enum {
    MSG_ACTIVE_POWERS,
    MSG_NO_ACTIVE_POWER,
    MSG_ENTRALINK,
    MSG_TIME,
    MSG_CONFIRM,
    MSG_SHORT,
    MSG_YES,
    MSG_NO,
};

static const u16 sGPowerAppMessages[] = {
    BtlMainTitle_Text_ActivePassPower, BtlMainTitle_Text_None,              BtlMainTitle_Text_Entralink,
    BtlMainTitle_Text_Empty_19,        BtlMainTitle_Text_WillUseNeededHeld, BtlMainTitle_Text_NotHaveEnoughPass,
    BtlMainTitle_Text_Yes,             BtlMainTitle_Text_No,
};

static const GPowerAppBGFile sGPowerAppScreens[] = {
    { 7, 6, 0, 0 },
    { 3, 5, 0, 0 },
    { 1, 7, 0, 0 },
    { 0xff, 0, 0, 0 },
};

static const GPowerAppLabelSetup sGPowerAppLabelSetups[] = {
    { 20, 2, 40, 3, TRUE, 0, 5 },
    { 20, 2, 40, 3, TRUE, 0, 6 },
    { 20, 2, 40, 3, TRUE, 0, 7 },
    { 20, 2, 40, 3, TRUE, 0, 8 },
    { 0xff },
};

static const GPowerAppActorSetup sGPowerAppReturnActorSetups[] = {
    { 1, 48, 2, TRUE, 0, ACTOR_AUTO_ANIM, 4 },
    { 0xff },
};

static const GPowerAppActorGroup sGPowerAppReturnActorGroup = { RES_COMMON_PALETTE, RES_COMMON_CHARS, RES_COMMON_CELLS, 1,
                                                                ACTOR_RETURN };

static const GPowerAppObjFile sGPowerAppObjFiles[] = {
    { 1, RES_PALETTE, 0, 3 },
    { 3, RES_CHARS, 0, 0 },
    { 4, RES_CELLS, 0, 0 },
    { 8, -1, 0, 0 },
};

static const GPowerAppActorGroup sGPowerAppButtonActorGroup = { RES_PALETTE, RES_CHARS, RES_CELLS, BUTTON_COUNT,
                                                                ACTOR_BUTTON };

static const GPowerAppActorSetup sGPowerAppButtonActorSetups[] = {
    { 0, 48, 3, TRUE, 0, ACTOR_AUTO_ANIM, 0 },
    { 0, 48, 3, TRUE, 0, ACTOR_AUTO_ANIM, 1 },
    { 0, 48, 3, TRUE, 0, ACTOR_AUTO_ANIM, 2 },
    { 1, 48, 3, TRUE, 0, ACTOR_AUTO_ANIM, 3 },
    { 0xff },
};

static const GPowerAppWindowSetup sGPowerAppWindowSetups[] = {
    { 6, 1, 0, 30, 3, 8 },
    { 6, 2, 3, 20, 2, 8 },
    { 6, 2, 5, 20, 2, 8 },
    { 6, 2, 7, 20, 2, 8 },
    { 6, 2, 9, 20, 2, 8 },
    { 6, 2, 11, 20, 2, 8 },
    { 6, 2, 13, 20, 2, 8 },
    { 6, 2, 15, 20, 2, 8 },
    { 6, 2, 17, 20, 2, 8 },
    { 6, 2, 19, 20, 2, 8 },
    { 6, 2, 21, 20, 2, 8 },
    { 6, 22, 3, 8, 2, 8 },
    { 6, 22, 5, 8, 2, 8 },
    { 6, 22, 7, 8, 2, 8 },
    { 6, 22, 9, 8, 2, 8 },
    { 6, 22, 11, 8, 2, 8 },
    { 6, 22, 13, 8, 2, 8 },
    { 6, 22, 15, 8, 2, 8 },
    { 6, 22, 17, 8, 2, 8 },
    { 6, 22, 19, 8, 2, 8 },
    { 6, 22, 21, 8, 2, 8 },
    { 0, 2, 14, 28, 6, 8 },
    { 0xff },
};

// The cursor's palette on the frames after a Pass Power is decided
static const GPowerAppDecideFrame sGPowerAppDecideFrames[] = {
    { 1, 0 }, { 4, 1 }, { 8, 0 }, { 12, 1 }, { 16, -1 }, { -1, -1 },
};

static const TouchRect sGPowerAppSelectTouchRects[] = {
    { 28, 52, 56, 248 },   { 60, 84, 32, 224 },    { 92, 116, 8, 200 },      { 132, 156, 128, 232 },
    { 96, 132, 216, 240 }, { 168, 192, 232, 255 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sGPowerAppYesNoTouchRects[] = {
    { 168, 192, 128, 192 },
    { 168, 192, 192, 255 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static BOOL GPowerApp_Init(GameProc *proc, u32 *state, void *param, void *data) {
    GPowerApp *work;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_GPOWER_APP, 0x30000);
    work = GFL_ProcInitSubsystem(proc, sizeof(GPowerApp), HEAPID_GPOWER_APP);
    sys_memset(work, 0, sizeof(GPowerApp));
    work->heapId = HEAPID_GPOWER_APP;
    work->param = param;
    work->graphic = GPowerAppGraphic_Create(0, work->heapId);
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, work->heapId);
    work->msgData[0] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_TITLE, work->heapId);
    work->msgData[1] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PASS_POWERS, work->heapId);
    work->wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    work->printQueue = func_02021998(work->heapId);
    work->yesNoWins[0] = NULL;
    work->yesNoWins[1] = NULL;
    work->taskMenuRes = AppTaskMenuRes_Create(1, 10, work->font, work->printQueue, work->heapId);
    GPowerApp_InitWork(work);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, work->heapId);
    return TRUE;
}

static BOOL GPowerApp_Exit(GameProc *proc, u32 *state, void *param, void *data) {
    GPowerApp *work = data;
    HeapID heapId;
    int i;

    GPowerApp_FreeWork(work);
    for (i = 0; i < 2; i++) {
        if (work->msgData[i] != NULL) {
            GFL_MsgDataFree(work->msgData[i]);
            work->msgData[i] = NULL;
        }
    }
    GFL_WordSetSystemFree(work->wordSet);
    func_02021a18(work->printQueue);
    GFL_FontFree(work->font);
    GPowerApp_FreeYesNo(work);
    AppTaskMenuRes_Free(work->taskMenuRes);
    GPowerAppGraphic_Free(work->graphic);
    heapId = work->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL GPowerApp_Main(GameProc *proc, u32 *state, void *param, void *data) {
    GPowerApp *work = data;

    switch (*state) {
    case 0:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 1:
        if (work->seq == SEQ_END && func_02021c0c(work->printQueue) == TRUE) {
            (*state)++;
        }
        break;
    case 2:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, work->heapId);
        (*state)++;
        break;
    case 3:
        if (GFL_WipeIsFinished()) {
            return TRUE;
        }
        break;
    }
    GPowerApp_Update(work);
    GPowerApp_UpdatePrint(work);
    GPowerAppGraphic_Update(work->graphic);
    GPowerAppGraphic_Begin3D(work->graphic);
    GPowerAppGraphic_End3D(work->graphic);
    return FALSE;
}

static void GPowerApp_InitWork(GPowerApp *work) {
    int i;

    work->seq = SEQ_START;
    work->flags = 0;
    work->cursor = 0;
    work->powerCount = work->param->highLinkCount;
    if (work->powerCount <= 0) {
        work->cursor = BUTTON_ENTRALINK;
    }
    work->result = -1;
    work->decideFrame = 0;
    work->cursorPalette = 0;
    work->yesNoCursor = 0;
    work->shortMask = 0;
    work->messageY = 0;
    work->paletteAngle = 0;
    for (i = 0; i < POWER_BUTTON_COUNT; i++) {
        work->prices[i] = 0;
    }
    for (i = 0; i < WINDOW_COUNT; i++) {
        work->windows[i].window = NULL;
    }
    work->positions = GFL_ArcSysReadHeapNew(ARCID_GPOWER_APP, 9, work->heapId);
    GPowerApp_InitPowers(work);
    GPowerApp_LoadBG(work);
    GPowerApp_LoadTouchBar(work, 2, PALTYPE_MAIN_BG, 6);
    GPowerApp_InitWindows(work);
    GPowerApp_InitObj(work);
    GPowerApp_InitPaletteAnim(work);
    if (!func_0203d554()) {
        work->flags |= FLAG_KEYS;
        GPowerApp_SetActorPalette(work, ACTOR_BUTTON, work->cursor, 2);
    }
    GPowerApp_Draw(work, 0);
    work->flags |= FLAG_REDRAW;
    GPowerApp_SetMessageVisible(work, FALSE);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 3, 0x3f, 14, 2);
}

static void GPowerApp_Update(GPowerApp *work) {
    GPowerApp_UpdateFrame(work);
}

// Gives the result to the caller and frees what GPowerApp_InitWork made
static void GPowerApp_FreeWork(GPowerApp *work) {
    if (work->flags & FLAG_ABORTED) {
        work->param->result = PASS_POWER_APP_ABORTED;
    } else {
        switch (work->result) {
        case BUTTON_ENTRALINK:
            work->param->result = PASS_POWER_APP_ENTRALINK;
            break;
        case 0:
        case 1:
        case 2:
            work->param->result = PASS_POWER_APP_USE;
            work->param->passPower = work->param->highLinkIds[work->result];
            break;
        default:
            work->param->result = PASS_POWER_APP_NONE;
            break;
        }
    }
    GFL_HeapFree(work->positions);
    GPowerApp_FreeWindows(work);
    GPowerApp_FreeObj(work);
}

static void GPowerApp_UpdateFrame(GPowerApp *work) {
    GPowerApp_UpdateSeq(work);
    GPowerApp_UpdatePaletteAnim(work);
}

static void GPowerApp_UpdateSeq(GPowerApp *work) {
    switch (work->seq) {
    case SEQ_START:
        work->seq = SEQ_SELECT;
        break;
    case SEQ_SELECT:
        if (GPowerApp_IsAborted(work) == TRUE) {
            work->flags |= FLAG_ABORTED;
            work->seq = SEQ_END;
        } else {
            GPowerApp_UpdateSelect(work);
        }
        break;
    case SEQ_RETURN:
        if (!GPowerApp_IsActorAnimActive(work, ACTOR_RETURN, 0)) {
            work->seq = SEQ_END;
            work->result = -1;
        }
        break;
    case SEQ_DECIDE: {
        BOOL finished = GPowerApp_UpdateDecideAnim(work);

        GPowerApp_SetActorPalette(work, ACTOR_BUTTON, work->cursor, work->cursorPalette);
        if (finished == TRUE) {
            if (work->cursor == BUTTON_ENTRALINK) {
                work->seq = SEQ_END;
                work->result = work->cursor;
            } else {
                if (work->shortMask & (1 << work->cursor)) {
                    work->messageY = MESSAGE_HIDDEN_Y;
                    GPowerApp_ScrollMessage(work, FALSE);
                    GPowerApp_SetMessagePriority(work, FALSE);
                    GPowerApp_PrintMessage(work, TRUE);
                    GPowerApp_SetMessageVisible(work, TRUE);
                    work->seq = SEQ_SHORT_MESSAGE_IN;
                } else {
                    work->messageY = MESSAGE_HIDDEN_Y;
                    GPowerApp_ScrollMessage(work, FALSE);
                    GPowerApp_SetMessagePriority(work, FALSE);
                    GPowerApp_PrintMessage(work, FALSE);
                    GPowerApp_SetMessageVisible(work, TRUE);
                    work->seq = SEQ_CONFIRM_MESSAGE_IN;
                }
                GPowerApp_SetActorVisible(work, ACTOR_RETURN, 0, FALSE);
            }
        }
        break;
    }
    case SEQ_SHORT_MESSAGE_IN:
        if (GPowerApp_ScrollMessage(work, FALSE) == TRUE) {
            work->seq = SEQ_SHORT_MESSAGE_WAIT;
            GPowerApp_SetMessagePriority(work, TRUE);
        }
        break;
    case SEQ_SHORT_MESSAGE_WAIT:
        if (GPowerApp_IsAborted(work) == TRUE) {
            work->flags |= FLAG_ABORTED;
            work->seq = SEQ_END;
        } else if (GPowerApp_CheckAnyInput(work) == TRUE) {
            GPowerApp_ScrollMessage(work, TRUE);
            GPowerApp_SetMessagePriority(work, FALSE);
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            work->seq = SEQ_SHORT_MESSAGE_OUT;
        }
        break;
    case SEQ_SHORT_MESSAGE_OUT:
        if (GPowerApp_ScrollMessage(work, TRUE) == TRUE) {
            GPowerApp_SetMessagePriority(work, TRUE);
            GPowerApp_SetMessageVisible(work, FALSE);
            work->seq = SEQ_SELECT;
            work->flags |= FLAG_REDRAW;
            GPowerApp_SetActorVisible(work, ACTOR_RETURN, 0, TRUE);
            if (func_0203d554() == TRUE) {
                work->flags &= ~FLAG_KEYS;
                GPowerApp_SetActorPalette(work, ACTOR_BUTTON, work->cursor, 0);
            }
        }
        break;
    case SEQ_CONFIRM_MESSAGE_IN:
        if (GPowerApp_ScrollMessage(work, FALSE) == TRUE) {
            GPowerApp_SetMessagePriority(work, TRUE);
            GPowerApp_CreateYesNo(work);
            work->seq = SEQ_CONFIRM;
        }
        break;
    case SEQ_CONFIRM:
        if (GPowerApp_IsAborted(work) == TRUE) {
            work->flags |= FLAG_ABORTED;
            work->seq = SEQ_END;
        } else {
            GPowerApp_UpdateYesNo(work);
            if (GPowerApp_UpdateYesNoInput(work) == TRUE) {
                work->seq = SEQ_CONFIRM_FLASH;
            }
        }
        break;
    case SEQ_CONFIRM_FLASH:
        GPowerApp_UpdateYesNo(work);
        if (GPowerApp_IsYesNoFlashFinished(work) == TRUE) {
            GPowerApp_ScrollMessage(work, TRUE);
            GPowerApp_SetMessagePriority(work, FALSE);
            GPowerApp_FreeYesNo(work);
            work->seq = SEQ_CONFIRM_MESSAGE_OUT;
        }
        break;
    case SEQ_CONFIRM_MESSAGE_OUT:
        if (GPowerApp_ScrollMessage(work, TRUE) == TRUE) {
            if (work->yesNoCursor == 0) {
                work->seq = SEQ_END;
                work->result = work->cursor;
            } else {
                work->seq = SEQ_SELECT;
                work->flags |= FLAG_REDRAW;
                if (func_0203d554() == TRUE) {
                    work->flags &= ~FLAG_KEYS;
                    GPowerApp_SetActorPalette(work, ACTOR_BUTTON, work->cursor, 0);
                }
            }
            GPowerApp_SetMessagePriority(work, TRUE);
            GPowerApp_SetMessageVisible(work, FALSE);
            GPowerApp_SetActorVisible(work, ACTOR_RETURN, 0, TRUE);
        }
        break;
    case SEQ_END:
        break;
    }
}

static void GPowerApp_UpdatePrint(GPowerApp *work) {
    int i;

    func_02021a3c(work->printQueue);
    for (i = 0; i < WINDOW_COUNT; i++) {
        PrintWindow_Flush(&work->windows[i], work->printQueue);
    }
}

// A value of the positions' table, which holds an x and a y for each position
static s16 GPowerApp_GetPos(GPowerApp *work, u32 index) {
    if (index >= POS_COUNT * 2) {
        return 0;
    }
    return work->positions[index];
}

static void GPowerApp_LoadBG(GPowerApp *work) {
    const GPowerAppBGFile *palette = sGPowerAppPalettes;
    const GPowerAppBGFile *chars = sGPowerAppChars;
    const GPowerAppBGFile *screen = sGPowerAppScreens;
    int i;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_GPOWER_APP, work->heapId);

    for (i = 0; i < 8; i++) {
        if (palette->target == 0xff) {
            break;
        }
        GFL_G2DIOLoadArcNCLRDefault(arc, palette->file, palette->target, palette->offset, palette->count * 32,
                                    work->heapId);
        palette++;
    }
    for (i = 0; i < 8; i++) {
        if (chars->target == 0xff) {
            break;
        }
        GFL_BGSysLoadArcNCGRStatic(arc, chars->file, chars->target, chars->offset, 0, FALSE, work->heapId);
        chars++;
    }
    for (i = 0; i < 8; i++) {
        if (screen->target == 0xff) {
            break;
        }
        loadBGScrToVramByFileNoReserveNegAlign(arc, screen->file, screen->target, screen->offset, 0, FALSE,
                                               work->heapId);
        screen++;
    }
    GFL_ArcToolFree(arc);
}

// Loads the touch bar's BG and gives its screen a palette
static void GPowerApp_LoadTouchBar(GPowerApp *work, u32 bg, u32 paletteType, u32 palette) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), work->heapId);
    u16 *screen;
    int x;
    int y;

    GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), paletteType, palette * 32, 32, work->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, func_0202d824(), bg, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, func_0202d828(), bg, 0, 0, FALSE, work->heapId);
    screen = GFL_BGSysIsScrHeapExists(bg);
    for (y = 0; y < 24; y++) {
        for (x = 0; x < 32; x++) {
            screen[y * 32 + x] += (u16)((palette & 0xf) << 12);
        }
    }
    GFL_BGSysLoadScr(bg);
    GFL_ArcToolFree(arc);
}

static void GPowerApp_AddActors(GPowerApp *work, u32 vramType, const GPowerAppActorSetup *setups,
                                const GPowerAppActorGroup *group) {
    int i;
    ClActUnit *unit = GPowerAppGraphic_GetClActUnit(work->graphic);
    ClActorSetup setup;
    ClActor **actors;
    s16 count;
    s16 palette;
    s16 chars;
    s16 cells;
    s16 type;

    sys_memset(&setup, 0, sizeof(setup));
    palette = group->palette;
    chars = group->chars;
    cells = group->cells;
    count = group->count;
    type = group->type;
    if (type == ACTOR_BUTTON) {
        actors = work->buttons;
    } else if (type == ACTOR_RETURN) {
        actors = work->returnButton;
    }
    for (i = 0; i < count; i++) {
        u32 pos;
        s16 x;
        s16 y;
        BOOL visible;
        BOOL autoAnim;

        if (setups->sequence == 0xff) {
            break;
        }
        pos = setups->pos * 2;
        x = GPowerApp_GetPos(work, pos);
        y = GPowerApp_GetPos(work, pos + 1);
        setup.x = x;
        setup.y = y;
        setup.sequence = setups->sequence;
        setup.priority = setups->priority;
        setup.bgPriority = setups->bgPriority;
        actors[i] = func_0204c040(unit, work->resources[chars], work->resources[palette], work->resources[cells], &setup,
                                  vramType, work->heapId);
        visible = FALSE;
        if (setups->visible == TRUE) {
            visible = TRUE;
        }
        func_0204c124(actors[i], visible);
        autoAnim = FALSE;
        if (setups->flags & ACTOR_AUTO_ANIM) {
            autoAnim = TRUE;
        }
        func_0204c520(actors[i], autoAnim);
        if (setups->flags & ACTOR_FLIP_Y) {
            func_0204c2b0(actors[i], 1, TRUE);
        }
        if (setups->flags & ACTOR_FLIP_X) {
            func_0204c2b0(actors[i], 0, TRUE);
        }
        setups++;
    }
}

// Loads the screen's own palette, characters, and cells and animations
static void GPowerApp_LoadObjResources(GPowerApp *work, u32 vramType, const GPowerAppObjFile *files) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_GPOWER_APP, work->heapId);

    GPowerAppGraphic_GetClActUnit(work->graphic);
    work->resources[files[0].resource] =
        func_0204bbb8(arc, files[0].file, vramType, files[0].offset, 0, files[0].count, work->heapId);
    work->resources[files[1].resource] = func_0204b81c(arc, files[1].file, FALSE, vramType, work->heapId);
    work->resources[files[2].resource] = func_0204bde0(arc, files[2].file, files[3].file, work->heapId);
    GFL_ArcToolFree(arc);
}

// Loads the touch bar's icons, for the return button
static void GPowerApp_LoadCommonObjResources(GPowerApp *work, u32 vramType) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), work->heapId);
    u32 palette;
    u32 chars;
    u32 cells;
    u32 anims;

    GPowerAppGraphic_GetClActUnit(work->graphic);
    palette = func_0202d810();
    chars = func_0202d814();
    cells = func_0202d818(2);
    anims = func_0202d81c(2);
    work->resources[RES_COMMON_PALETTE] = func_0204bbb8(arc, palette, vramType, 8 * 32, 0, 4, work->heapId);
    work->resources[RES_COMMON_CHARS] = func_0204b81c(arc, chars, FALSE, vramType, work->heapId);
    work->resources[RES_COMMON_CELLS] = func_0204bde0(arc, cells, anims, work->heapId);
    GFL_ArcToolFree(arc);
}

static void GPowerApp_InitObj(GPowerApp *work) {
    ClActUnit *unit = GPowerAppGraphic_GetClActUnit(work->graphic);
    ArcTool *arc;

    GPowerApp_LoadObjResources(work, CLACT_VRAM_MAIN, sGPowerAppObjFiles);
    GPowerApp_LoadCommonObjResources(work, CLACT_VRAM_MAIN);
    GPowerApp_AddActors(work, CLACT_VRAM_MAIN, sGPowerAppButtonActorSetups, &sGPowerAppButtonActorGroup);
    GPowerApp_AddActors(work, CLACT_VRAM_MAIN, sGPowerAppReturnActorSetups, &sGPowerAppReturnActorGroup);
    func_02042ba8(FALSE, work->heapId);
    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, work->heapId);
    work->resources[RES_FONT_PALETTE] = func_0204bc48(arc, 6, CLACT_VRAM_MAIN, 4 * 32, work->heapId);
    GFL_ArcToolFree(arc);
    work->bmpOam = BmpOam_Init(work->heapId, unit);
    GPowerApp_AddLabels(work, CLACT_VRAM_MAIN, sGPowerAppLabelSetups, RES_FONT_PALETTE);
}

static void GPowerApp_FreeObj(GPowerApp *work) {
    int i;

    for (i = 0; i < BUTTON_COUNT; i++) {
        func_0204c108(work->buttons[i]);
    }
    func_0204c108(work->returnButton[0]);
    func_0204bcd0(work->resources[RES_PALETTE]);
    func_0204b98c(work->resources[RES_CHARS]);
    func_0204be64(work->resources[RES_CELLS]);
    func_0204bcd0(work->resources[RES_COMMON_PALETTE]);
    func_0204b98c(work->resources[RES_COMMON_CHARS]);
    func_0204be64(work->resources[RES_COMMON_CELLS]);
    func_0204bcd0(work->resources[RES_FONT_PALETTE]);
    for (i = 0; i < BUTTON_COUNT; i++) {
        BmpOam_ActorDel(work->labels[i]);
        GFL_BitmapFree(work->labelBitmaps[i]);
    }
    BmpOam_Exit(work->bmpOam);
}

static void GPowerApp_SetActorAnim(GPowerApp *work, int type, int index, u32 sequence) {
    if (type == ACTOR_BUTTON) {
        if (index < BUTTON_COUNT) {
            func_0204c488(work->buttons[index], (u16)sequence);
            func_0204c520(work->buttons[index], TRUE);
        }
    } else if (type == ACTOR_RETURN) {
        if (index < 1) {
            func_0204c488(work->returnButton[index], (u16)sequence);
            func_0204c520(work->returnButton[index], TRUE);
        }
    }
}

static void GPowerApp_SetActorVisible(GPowerApp *work, int type, int index, BOOL visible) {
    if (type == ACTOR_BUTTON) {
        if (index < BUTTON_COUNT) {
            func_0204c124(work->buttons[index], visible);
        }
    } else if (type == ACTOR_RETURN) {
        if (index < 1) {
            func_0204c124(work->returnButton[index], visible);
        }
    }
}

static void GPowerApp_SetActorPalette(GPowerApp *work, int type, int index, u32 palette) {
    if (type == ACTOR_BUTTON) {
        if (index < BUTTON_COUNT) {
            func_0204c378(work->buttons[index], (u8)palette, 1);
        }
    } else if (type == ACTOR_RETURN) {
        if (index < 1) {
            func_0204c378(work->returnButton[index], (u8)palette, 1);
        }
    }
}

static BOOL GPowerApp_IsActorAnimActive(GPowerApp *work, int type, int index) {
    BOOL active = TRUE;

    if (type == ACTOR_BUTTON) {
        if (index >= BUTTON_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(work->buttons[index])) {
            active = FALSE;
        }
    } else if (type == ACTOR_RETURN) {
        if (index >= 1) {
            return FALSE;
        }
        if (!func_0204c560(work->returnButton[index])) {
            active = FALSE;
        }
    }
    return active;
}

// Creates the buttons' labels. Every label gets the first setup, since `setups` never advances; GPowerApp_PrintLabel
// puts each where it belongs
static void GPowerApp_AddLabels(GPowerApp *work, u32 vramType, const GPowerAppLabelSetup *setups, u32 paletteRes) {
    int i;

    for (i = 0; i < BUTTON_COUNT; i++) {
        BmpOamActorSetup setup;
        u32 pos;
        s16 x;
        s16 y;

        if (setups->width == 0xff) {
            break;
        }
        work->labelBitmaps[i] = GFL_BitmapCreate(setups->width, setups->height, 0x20, work->heapId);
        pos = setups->pos;
        x = GPowerApp_GetPos(work, pos * 2);
        y = GPowerApp_GetPos(work, pos * 2 + 1);
        sys_memset(&setup, 0, sizeof(setup));
        setup.bitmap = work->labelBitmaps[i];
        setup.x = x;
        setup.y = y;
        setup.palette = work->resources[paletteRes];
        setup.priority = setups->priority;
        setup.bgPriority = setups->bgPriority;
        setup.paletteOffset = setups->paletteOffset;
        setup.surface = vramType;
        setup.vramType = vramType;
        work->labels[i] = BmpOam_ActorAdd(work->bmpOam, &setup);
        if (!setups->visible) {
            BmpOam_ActorSetDrawEnable(work->labels[i], FALSE);
        }
    }
}

static void GPowerApp_PrintLabel(GPowerApp *work, int index, StrBuf *str, int x, int y) {
    if (index < BUTTON_COUNT) {
        const GPowerAppLabelSetup *setup;
        u32 pos;
        s16 posX;
        s16 posY;

        GFL_BitmapFill(work->labelBitmaps[index], 0);
        GFL_TextRendererDrawToBitmapEx(work->labelBitmaps[index], 0, 0, str, work->font, COLOR_TEXT);
        BmpOam_ActorBmpTrans(work->labels[index]);
        setup = &sGPowerAppLabelSetups[index];
        pos = setup->pos * 2;
        posX = GPowerApp_GetPos(work, pos);
        posY = GPowerApp_GetPos(work, pos + 1);
        posX += x;
        posY += y;
        BmpOam_ActorSetPos(work->labels[index], posX, posY);
        BmpOam_ActorSetDrawEnable(work->labels[index], TRUE);
    }
}

static void GPowerApp_SetLabelVisible(GPowerApp *work, int index, BOOL visible) {
    BmpOam_ActorSetDrawEnable(work->labels[index], visible);
}

static void GPowerApp_InitWindows(GPowerApp *work) {
    int i;
    const GPowerAppWindowSetup *setup = sGPowerAppWindowSetups;
    ArcTool *arc;

    for (i = 0; i < WINDOW_COUNT; i++) {
        if (setup->bg == 0xff) {
            break;
        }
        work->windows[i].window =
            BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, TRUE);
        BmpWin_FlushMap(work->windows[i].window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(work->windows[i].window));
        setup++;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, PALTYPE_MAIN_BG, 8 * 32, 32, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, PALTYPE_SUB_BG, 8 * 32, 32, work->heapId);
    GFL_ArcToolFree(arc);
}

static void GPowerApp_PrintWindow(GPowerApp *work, int index, u32 x, u32 y, StrBuf *str, u32 align, u16 color) {
    GPowerApp_ClearWindow(work, index, 0);
    PrintStrAligned(&work->windows[index], work->printQueue, x, y, str, work->font, color, align);
}

static void GPowerApp_FreeWindows(GPowerApp *work) {
    int i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        if (work->windows[i].window != NULL) {
            BmpWin_Free(work->windows[i].window);
            work->windows[i].window = NULL;
        }
    }
}

static void GPowerApp_ClearWindow(GPowerApp *work, int index, u32 color) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[index].window), color);
    work->windows[index].flushPending = TRUE;
}

// The choice of a Pass Power, the Entralink or the way back, by touch or keys
static void GPowerApp_UpdateSelect(GPowerApp *work) {
    BOOL decided = FALSE;
    int changed = 0;
    int cursor = work->cursor;
    int touch;

    if (work->flags & FLAG_REDRAW) {
        changed = 1;
        work->flags &= ~FLAG_REDRAW;
    }
    touch = GPowerApp_CheckTouch(work);
    if (touch >= 0 && touch <= BUTTON_ENTRALINK) {
        work->cursor = touch;
        decided = TRUE;
        work->flags |= FLAG_KEYS;
        changed = 1;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_0203d564(TRUE);
    } else if (touch == TOUCH_RETURN) {
        changed = 2;
        GPowerApp_SetActorAnim(work, ACTOR_RETURN, 0, 9);
        work->seq = SEQ_RETURN;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        func_0203d564(TRUE);
    }
    if (changed == 0) {
        if (!(work->flags & FLAG_KEYS) && GCTX_HIDGetPressedKeys()) {
            work->flags |= FLAG_KEYS;
            changed = 1;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_0203d564(FALSE);
        }
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            if ((work->flags & FLAG_KEYS) && changed == 0) {
                if (work->cursor == BUTTON_ENTRALINK) {
                    if (work->powerCount != 0) {
                        work->cursor = work->powerCount - 1;
                    }
                } else {
                    work->cursor--;
                    if (work->cursor < 0) {
                        if (work->flags & FLAG_NO_ENTRALINK) {
                            work->cursor = work->powerCount - 1;
                        } else {
                            work->cursor = BUTTON_ENTRALINK;
                        }
                    }
                }
                changed = 1;
                if (work->cursor != cursor) {
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                }
                func_0203d564(FALSE);
            }
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            if ((work->flags & FLAG_KEYS) && changed == 0) {
                if (work->cursor == BUTTON_ENTRALINK) {
                    if (work->powerCount != 0) {
                        work->cursor = 0;
                    }
                } else {
                    work->cursor++;
                    if (work->cursor > work->powerCount - 1) {
                        if (work->flags & FLAG_NO_ENTRALINK) {
                            work->cursor = 0;
                        } else {
                            work->cursor = BUTTON_ENTRALINK;
                        }
                    }
                }
                changed = 1;
                if (work->cursor != cursor) {
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                }
                func_0203d564(FALSE);
            }
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            changed = 2;
            GPowerApp_SetActorAnim(work, ACTOR_RETURN, 0, 9);
            work->seq = SEQ_RETURN;
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            func_0203d564(FALSE);
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            if ((work->flags & FLAG_KEYS) && changed == 0) {
                changed = 2;
                decided = TRUE;
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
            func_0203d564(FALSE);
        }
    }
    if (decided == TRUE) {
        work->decideFrame = 0;
        work->seq = SEQ_DECIDE;
    }
    if (changed == 1) {
        GPowerApp_Draw(work, 1);
        GPowerApp_SetActorPalette(work, ACTOR_BUTTON, cursor, 0);
        if (work->flags & FLAG_KEYS) {
            GPowerApp_SetActorPalette(work, ACTOR_BUTTON, work->cursor, 2);
        }
    }
}

// Draws everything for mode 0; nothing changes after that
static void GPowerApp_Draw(GPowerApp *work, int mode) {
    if (mode == 0) {
        StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData[0], sGPowerAppMessages[MSG_ACTIVE_POWERS]);

        GPowerApp_PrintWindow(work, WINDOW_TITLE, 0, 4, str, PRINT_ALIGN_LEFT, COLOR_TEXT);
        GFL_StrBufFree(str);
        GPowerApp_DrawActivePowers(work);
        GPowerApp_DrawPowerButtons(work);
        GPowerApp_DrawEntralinkButton(work);
    }
}

// The running Pass Powers with their minutes and seconds left
static void GPowerApp_DrawActivePowers(GPowerApp *work) {
    int count;
    int i;

    for (i = 0; i < ACTIVE_POWER_MAX; i++) {
        GPowerApp_ClearWindow(work, WINDOW_ACTIVE_NAME + i, 0);
        GPowerApp_ClearWindow(work, WINDOW_ACTIVE_TIME + i, 0);
    }
    count = work->param->powerCount;
    if (count == 0) {
        StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData[0], sGPowerAppMessages[MSG_NO_ACTIVE_POWER]);

        GPowerApp_PrintWindow(work, WINDOW_ACTIVE_NAME, 0, 0, str, PRINT_ALIGN_LEFT, COLOR_TEXT);
        GFL_StrBufFree(str);
        return;
    }
    for (i = 0; i < count; i++) {
        StrBuf *name = GFL_MsgDataLoadStrbufNew(work->msgData[1], work->param->powers[i].id);
        StrBuf *format;
        StrBuf *number;
        StrBuf *time;
        int seconds;

        GPowerApp_PrintWindow(work, WINDOW_ACTIVE_NAME + i, 0, 0, name, PRINT_ALIGN_LEFT, COLOR_TEXT);
        GFL_StrBufFree(name);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], sGPowerAppMessages[MSG_TIME]);
        seconds = work->param->powers[i].seconds;
        number = GFL_StrBufCreate(0x40, work->heapId);
        time = GFL_StrBufCreate(0x40, work->heapId);
        GFL_WordSetFormatNumber(number, seconds / 60, 2, 2, TRUE);
        func_0202437c(work->wordSet, 0, number, 0, 1, 2);
        GFL_WordSetFormatNumber(number, seconds % 60, 2, 2, TRUE);
        func_0202437c(work->wordSet, 1, number, 0, 1, 2);
        GFL_WordSetFormatStrbuf(work->wordSet, time, format);
        GPowerApp_PrintWindow(work, WINDOW_ACTIVE_TIME + i, 0, 0, time, PRINT_ALIGN_LEFT, COLOR_TEXT);
        GFL_StrBufFree(format);
        GFL_StrBufFree(number);
        GFL_StrBufFree(time);
    }
}

// The buttons of the Pass Powers the player can use
static void GPowerApp_DrawPowerButtons(GPowerApp *work) {
    int count;
    int i;

    for (i = 0; i < POWER_BUTTON_COUNT; i++) {
        GPowerApp_SetLabelVisible(work, i, FALSE);
    }
    count = work->powerCount;
    if (count != 0) {
        for (i = 0; i < count; i++) {
            StrBuf *name = GFL_MsgDataLoadStrbufNew(work->msgData[1], work->param->highLinkIds[i]);

            GPowerApp_PrintLabel(work, i, name, 0, 0);
            GPowerApp_SetLabelVisible(work, i, TRUE);
            GFL_StrBufFree(name);
        }
    }
    for (i = 0; i < POWER_BUTTON_COUNT; i++) {
        GPowerApp_SetActorAnim(work, ACTOR_BUTTON, i, count > i ? 0 : 2);
    }
}

static void GPowerApp_DrawEntralinkButton(GPowerApp *work) {
    BOOL enabled = TRUE;

    if (work->flags & FLAG_NO_ENTRALINK) {
        enabled = FALSE;
    }
    GPowerApp_SetLabelVisible(work, BUTTON_ENTRALINK, FALSE);
    if (enabled == TRUE) {
        StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData[0], sGPowerAppMessages[MSG_ENTRALINK]);

        GPowerApp_PrintLabel(work, BUTTON_ENTRALINK, str, 0, 0);
        GFL_StrBufFree(str);
    }
    GPowerApp_SetActorAnim(work, ACTOR_BUTTON, BUTTON_ENTRALINK, enabled ? 1 : 3);
}

// The button touched, TOUCH_RETURN, or -1. A touch on a button that is off shows the cursor nowhere
static int GPowerApp_CheckTouch(GPowerApp *work) {
    int result = -1;
    int hit = func_0203da0c(sGPowerAppSelectTouchRects);

    if (hit >= 0 && hit <= 2) {
        if (work->powerCount > hit) {
            result = hit;
        } else {
            work->flags &= ~FLAG_KEYS;
            work->flags |= FLAG_REDRAW;
        }
    } else if (hit == 3 || hit == 4) {
        if (!(work->flags & FLAG_NO_ENTRALINK)) {
            result = BUTTON_ENTRALINK;
        } else {
            work->flags = (work->flags & ~FLAG_KEYS) | FLAG_REDRAW;
        }
    } else if (hit == 5) {
        result = TOUCH_RETURN;
    }
    return result;
}

// Prints the question whether to use the Pass Power, or that the player is short of Pass Orbs
static void GPowerApp_PrintMessage(GPowerApp *work, BOOL isShort) {
    GPowerApp_ClearWindow(work, WINDOW_MESSAGE, 0);
    if (!isShort) {
        StrBuf *format = GFL_MsgDataLoadStrbufNew(work->msgData[0], sGPowerAppMessages[MSG_CONFIRM]);
        StrBuf *message = GFL_StrBufCreate(0x100, work->heapId);
        StrBuf *name = GFL_MsgDataLoadStrbufNew(work->msgData[1], work->param->highLinkIds[work->cursor]);
        StrBuf *number;

        func_0202437c(work->wordSet, 0, name, 0, 1, 2);
        number = GFL_StrBufCreate(0x40, work->heapId);
        GFL_WordSetFormatNumber(number, work->prices[work->cursor], 4, 1, TRUE);
        func_0202437c(work->wordSet, 1, number, 0, 1, 2);
        GFL_WordSetFormatNumber(number, work->param->itemCount, 4, 1, TRUE);
        func_0202437c(work->wordSet, 2, number, 0, 1, 2);
        GFL_WordSetFormatStrbuf(work->wordSet, message, format);
        GPowerApp_PrintWindow(work, WINDOW_MESSAGE, 0, 0, message, PRINT_ALIGN_LEFT, COLOR_TEXT);
        GFL_StrBufFree(format);
        GFL_StrBufFree(name);
        GFL_StrBufFree(message);
        GFL_StrBufFree(number);
    } else {
        StrBuf *format = GFL_MsgDataLoadStrbufNew(work->msgData[0], sGPowerAppMessages[MSG_SHORT]);
        StrBuf *message = GFL_StrBufCreate(0x100, work->heapId);
        StrBuf *number = GFL_StrBufCreate(0x40, work->heapId);

        GFL_WordSetFormatNumber(number, work->prices[work->cursor], 4, 1, TRUE);
        func_0202437c(work->wordSet, 0, number, 0, 1, 2);
        GFL_WordSetFormatNumber(number, work->param->itemCount, 4, 1, TRUE);
        func_0202437c(work->wordSet, 1, number, 0, 1, 2);
        GFL_WordSetFormatStrbuf(work->wordSet, message, format);
        GPowerApp_PrintWindow(work, WINDOW_MESSAGE, 0, 0, message, PRINT_ALIGN_LEFT, COLOR_TEXT);
        GFL_StrBufFree(format);
        GFL_StrBufFree(message);
        GFL_StrBufFree(number);
    }
}

static void GPowerApp_SetMessageVisible(GPowerApp *work, BOOL visible) {
    if (visible == TRUE) {
        GFL_BGSysSetBGEnabled(0, TRUE);
        GFL_BGSysSetBGEnabled(1, TRUE);
    } else {
        GFL_BGSysSetBGEnabled(0, FALSE);
        GFL_BGSysSetBGEnabled(1, FALSE);
    }
}

static BOOL GPowerApp_IsAborted(GPowerApp *work) {
    return func_02016bec(work->param->gsys);
}

static void GPowerApp_InitPowers(GPowerApp *work) {
    if (work->param->argument == 1) {
        work->flags |= FLAG_NO_ENTRALINK;
    }
    GPowerApp_LoadPrices(work);
}

// The cursor's palette fades between two palettes of the screen's
static void GPowerApp_InitPaletteAnim(GPowerApp *work) {
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLR(ARCID_GPOWER_APP, 1, &palette, work->heapId);
    u16 *colors = palette->rawData;

    sys_memcpy(&colors[1 * 16], work->paletteFrom, sizeof(work->paletteFrom));
    sys_memcpy(&colors[2 * 16], work->paletteTo, sizeof(work->paletteTo));
    GFL_HeapFree(file);
    work->paletteAngle = 0;
    work->flags |= FLAG_PALETTE_ANIM;
}

static void GPowerApp_UpdatePaletteAnim(GPowerApp *work) {
    fx16 ratio;
    int i;

    if (!(work->flags & FLAG_PALETTE_ANIM)) {
        return;
    }
    work->paletteAngle += 0x400;
    if (work->paletteAngle >= 0x10000) {
        work->paletteAngle -= 0x10000;
    }
    ratio = (FX_CosIdx(work->paletteAngle) + FX32_ONE) / 2;
    for (i = 0; i < 16; i++) {
        u16 from = work->paletteFrom[i];
        u16 to = work->paletteTo[i];
        u8 r0 = from & 0x1f;
        u8 g0 = (from & 0x3e0) >> 5;
        u8 b0 = (from & 0x7c00) >> 10;
        u8 r1 = to & 0x1f;
        u8 g1 = (to & 0x3e0) >> 5;
        u8 b1 = (to & 0x7c00) >> 10;
        u8 r = r0 + (((r1 - r0) * ratio) >> FX32_SHIFT);
        u8 g = g0 + (((g1 - g0) * ratio) >> FX32_SHIFT);
        u8 b = b0 + (((b1 - b0) * ratio) >> FX32_SHIFT);

        work->palette[i] = r | (g << 5) | (b << 10);
    }
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_MAIN, 2 * 32, work->palette, 32);
}

// Flashes the decided button's palette, and returns TRUE when it is over
static BOOL GPowerApp_UpdateDecideAnim(GPowerApp *work) {
    int i;

    work->decideFrame++;
    for (i = 0; i < 10; i++) {
        s16 frame = sGPowerAppDecideFrames[i].frame;
        s16 palette = sGPowerAppDecideFrames[i].palette;

        if (frame == -1) {
            break;
        }
        if (frame == work->decideFrame) {
            if (palette == -1) {
                return TRUE;
            }
            work->cursorPalette = palette;
            break;
        }
    }
    return FALSE;
}

static void GPowerApp_CreateYesNo(GPowerApp *work) {
    GPowerApp_FreeYesNo(work);
    work->yesNoItems[0].str = GFL_MsgDataLoadStrbufNew(work->msgData[0], sGPowerAppMessages[MSG_YES]);
    work->yesNoItems[0].color = PRINT_COLOR(14, 15, 3);
    work->yesNoItems[0].type = 0;
    work->yesNoItems[1].str = GFL_MsgDataLoadStrbufNew(work->msgData[0], sGPowerAppMessages[MSG_NO]);
    work->yesNoItems[1].color = PRINT_COLOR(14, 15, 3);
    work->yesNoItems[1].type = 0;
    work->yesNoWins[0] = AppTaskMenuWin_Create(work->taskMenuRes, &work->yesNoItems[0], 16, 21, 8, work->heapId);
    work->yesNoWins[1] = AppTaskMenuWin_Create(work->taskMenuRes, &work->yesNoItems[1], 24, 21, 8, work->heapId);
    GFL_StrBufFree(work->yesNoItems[0].str);
    GFL_StrBufFree(work->yesNoItems[1].str);
    AppTaskMenuWin_SetActive(work->yesNoWins[0], TRUE);
    AppTaskMenuWin_SetActive(work->yesNoWins[1], FALSE);
    work->yesNoCursor = 0;
}

static void GPowerApp_FreeYesNo(GPowerApp *work) {
    int i;

    for (i = 0; i < 2; i++) {
        if (work->yesNoWins[i] != NULL) {
            AppTaskMenuWin_Free(work->yesNoWins[i]);
            work->yesNoWins[i] = NULL;
        }
    }
}

static void GPowerApp_UpdateYesNo(GPowerApp *work) {
    int i;

    for (i = 0; i < 2; i++) {
        if (work->yesNoWins[i] != NULL) {
            AppTaskMenuWin_Update(work->yesNoWins[i]);
        }
    }
}

// Returns TRUE once Yes or No is picked
static BOOL GPowerApp_UpdateYesNoInput(GPowerApp *work) {
    BOOL touched = FALSE;
    BOOL decided = FALSE;
    int hit;

    if (work->yesNoWins[0] == NULL || work->yesNoWins[1] == NULL) {
        return FALSE;
    }
    hit = func_0203da0c(sGPowerAppYesNoTouchRects);
    if (hit == 0) {
        AppTaskMenuWin_SetFlashing(work->yesNoWins[0], TRUE);
        AppTaskMenuWin_SetActive(work->yesNoWins[1], FALSE);
        decided = TRUE;
        work->yesNoCursor = 0;
        touched = TRUE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_0203d564(TRUE);
    } else if (hit == 1) {
        AppTaskMenuWin_SetActive(work->yesNoWins[0], FALSE);
        AppTaskMenuWin_SetFlashing(work->yesNoWins[1], TRUE);
        decided = TRUE;
        work->yesNoCursor = 1;
        touched = TRUE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_0203d564(TRUE);
    }
    if (!touched) {
        if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
            if (work->yesNoCursor != 0) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            AppTaskMenuWin_SetActive(work->yesNoWins[0], TRUE);
            AppTaskMenuWin_SetActive(work->yesNoWins[1], FALSE);
            work->yesNoCursor = 0;
            func_0203d564(FALSE);
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
            if (work->yesNoCursor != 1) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            AppTaskMenuWin_SetActive(work->yesNoWins[0], FALSE);
            AppTaskMenuWin_SetActive(work->yesNoWins[1], TRUE);
            work->yesNoCursor = 1;
            func_0203d564(FALSE);
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            if (work->yesNoCursor == 0) {
                AppTaskMenuWin_SetFlashing(work->yesNoWins[0], TRUE);
                AppTaskMenuWin_SetActive(work->yesNoWins[1], FALSE);
            } else {
                AppTaskMenuWin_SetActive(work->yesNoWins[0], FALSE);
                AppTaskMenuWin_SetFlashing(work->yesNoWins[1], TRUE);
            }
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            func_0203d564(FALSE);
            decided = TRUE;
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            AppTaskMenuWin_SetActive(work->yesNoWins[0], FALSE);
            AppTaskMenuWin_SetFlashing(work->yesNoWins[1], TRUE);
            decided = TRUE;
            work->yesNoCursor = 1;
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            func_0203d564(FALSE);
        }
    }
    return decided;
}

static BOOL GPowerApp_IsYesNoFlashFinished(GPowerApp *work) {
    if (work->yesNoWins[work->yesNoCursor] == NULL) {
        return TRUE;
    }
    if (AppTaskMenuWin_IsFlashFinished(work->yesNoWins[work->yesNoCursor])) {
        return TRUE;
    }
    return FALSE;
}

// Whether A or B was pressed or the screen touched
static BOOL GPowerApp_CheckAnyInput(GPowerApp *work) {
    BOOL result = FALSE;

    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        func_0203d564(FALSE);
        result = TRUE;
    }
    if (func_0203da48() == TRUE) {
        func_0203d564(TRUE);
        result = TRUE;
    }
    return result;
}

static void GPowerApp_LoadPrices(GPowerApp *work) {
    void *data = PassPowerData_Create(work->heapId);
    int i;

    for (i = 0; i < work->param->highLinkCount; i++) {
        work->prices[i] = PassPowerData_GetPrice(data, work->param->highLinkIds[i]);
        if (work->param->itemCount < work->prices[i]) {
            work->shortMask |= 1 << i;
        }
    }
    PassPowerData_Free(data);
}

// Slides the message's BGs in from the top or back out, and returns TRUE when they are there
static BOOL GPowerApp_ScrollMessage(GPowerApp *work, BOOL out) {
    BOOL finished = FALSE;

    if (!out) {
        work->messageY += 8;
        if (work->messageY >= 0) {
            work->messageY = 0;
            finished = TRUE;
        }
    } else {
        work->messageY -= 8;
        if (work->messageY <= MESSAGE_HIDDEN_Y) {
            work->messageY = MESSAGE_HIDDEN_Y;
            finished = TRUE;
        }
    }
    GFL_BGSysMoveBG(1, BG_MOVE_SET_Y, work->messageY);
    GFL_BGSysMoveBG(0, BG_MOVE_SET_Y, work->messageY);
    return finished;
}

// Puts the message's BGs behind the buttons' while they slide, or in front
static void GPowerApp_SetMessagePriority(GPowerApp *work, BOOL front) {
    if (!front) {
        GFL_BGSysSetBGPriority(0, 1);
        GFL_BGSysSetBGPriority(1, 2);
        GFL_BGSysSetBGPriority(2, 0);
    } else {
        GFL_BGSysSetBGPriority(0, 0);
        GFL_BGSysSetBGPriority(1, 1);
        GFL_BGSysSetBGPriority(2, 2);
    }
}
