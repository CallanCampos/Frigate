// The Black Tower and White Treehollow's retire menu: the touch screen asks whether to leave the dungeon, with a yes
// and a no button. Named after the ROM's "inf_dungeon_retire.c"
#include "field/inf_dungeon_retire.h"
#include "types.h"
#include "field/field.h"
#include "field/subscreen.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "constants/arc.h"
#include "constants/text_banks.h"

struct InfDungeonRetire {
    HeapID heapId; // never set, so 0
    GameSystem *gsys;
    GameCommSys *comm;
    FieldSubscreen *subscreen;
    u32 charOffset;
    u32 cursorImage;
    BmpWin *window;
    Font *font;
    MsgData *msgData;
    PrintQueue *printQueue;
    AppTaskMenuRes *menuRes;
    AppTaskMenu *menu;
    u16 state;
};

// The event's work
typedef struct {
    Field *field;
    u32 *result;
    u32 screenId;
} RetireEvent;

#define RETIRE_SUBSCREEN_MENU 0xc
#define RETIRE_SUBSCREEN_RETURN 0xf

static GameEventReturnCode InfDungeonRetire_EventMain(GameEvent *event, u32 *state, void *data);
static void InfDungeonRetire_Init(InfDungeonRetire *work, HeapID heapId);
static void InfDungeonRetire_InitBGs(InfDungeonRetire *work, HeapID heapId);
static void InfDungeonRetire_InitWindow(InfDungeonRetire *work, HeapID heapId);
static void InfDungeonRetire_InitMenuRes(InfDungeonRetire *work, HeapID heapId);
static void InfDungeonRetire_PrintMessage(InfDungeonRetire *work, BmpWin *window, u32 messageId);
static void InfDungeonRetire_CreateMenu(InfDungeonRetire *work, u32 heapId);
static void InfDungeonRetire_UpdateMenu(InfDungeonRetire *work);
static u32 InfDungeonRetire_GetCursorPos(InfDungeonRetire *work);
static BOOL InfDungeonRetire_IsFlashFinished(InfDungeonRetire *work);
static void InfDungeonRetire_Exit(InfDungeonRetire *work);
static void InfDungeonRetire_FreeWindow(InfDungeonRetire *work);
static void InfDungeonRetire_FreeMenu(InfDungeonRetire *work);
static void InfDungeonRetire_FreeMenuRes(InfDungeonRetire *work);
static void InfDungeonRetire_FreeBGs(InfDungeonRetire *work);

static AppTaskMenuItem sMenuItems[2] = {
    { NULL, 0x39e3, 0 },
    { NULL, 0x39e3, 0 },
};

static AppTaskMenuInit sMenuInit = { 0, 2, sMenuItems, APP_TASKMENU_POS_TOP_LEFT, 23, 9, 0, 0 };

GameEvent *InfDungeonRetire_CreateEvent(GameSystem *gsys, Field *field, u32 screenId, u32 *result) {
    GameEvent *event = GameEvent_Create(gsys, NULL, InfDungeonRetire_EventMain, sizeof(RetireEvent));
    RetireEvent *retire = GameEvent_GetData(event);

    sys_memset(retire, 0, sizeof(RetireEvent));
    retire->field = field;
    retire->screenId = screenId;
    retire->result = result;
    return event;
}

static GameEventReturnCode InfDungeonRetire_EventMain(GameEvent *event, u32 *state, void *data) {
    RetireEvent *retire = data;
    u32 returnScreen;
    u32 result;

    switch (*state) {
    case 0:
        FieldSubscreen_ReqChange(Field_GetSubscreen(retire->field), RETIRE_SUBSCREEN_MENU);
        (*state)++;
        break;
    case 1:
        returnScreen = FieldSubscreen_GetReturnSubscreen(Field_GetSubscreen(retire->field));
        if (returnScreen != 0) {
            func_ov036_021984e4(Field_GetSubscreen(retire->field));
            if (returnScreen == RETIRE_SUBSCREEN_RETURN) {
                if (retire->screenId == 10 || retire->screenId == 4) {
                    FieldSubscreen *subscreen = Field_GetSubscreen(retire->field);
                    FieldSubscreen_ReqChange(subscreen, retire->screenId);
                } else {
                    FieldSubscreen_ReqChange(Field_GetSubscreen(retire->field), 0);
                }
                result = 5;
            } else {
                FieldSubscreen_ReqChange(Field_GetSubscreen(retire->field), 1);
                result = 0;
            }
            *retire->result = result;
            (*state)++;
        }
        break;
    case 2:
        if (FieldSubscreen_IsReady(Field_GetSubscreen(retire->field)) == TRUE) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

InfDungeonRetire *InfDungeonRetire_Create(FieldSubscreen *subscreen, GameSystem *gsys, HeapID heapId) {
    InfDungeonRetire *work = GFL_HeapAllocate(heapId, sizeof(InfDungeonRetire), TRUE, "inf_dungeon_retire.c", 202);

    work->gsys = gsys;
    work->comm = GSYS_GetGameCommSystem(gsys);
    work->subscreen = subscreen;
    InfDungeonRetire_Init(work, heapId);
    InfDungeonRetire_Update(work);
    return work;
}

void InfDungeonRetire_Update(InfDungeonRetire *work) {
    switch (work->state) {
    case 0:
        InfDungeonRetire_PrintMessage(work, work->window, 0);
        InfDungeonRetire_CreateMenu(work, work->heapId);
        work->state++;
        break;
    case 1:
        if (GFL_WipeIsFinished() == TRUE) {
            work->state++;
        }
        break;
    case 2:
        if (InfDungeonRetire_IsFlashFinished(work)) {
            work->state++;
        }
        break;
    case 3:
        if (InfDungeonRetire_GetCursorPos(work) == 0) {
            func_ov036_021984f0(work->subscreen, work->heapId);
            FieldSubscreen_SaveReturnSubscreen(work->subscreen, 0xf);
        } else {
            FieldSubscreen_SaveReturnSubscreen(work->subscreen, 0x10);
        }
        InfDungeonRetire_FreeMenu(work);
        break;
    }
    InfDungeonRetire_UpdateMenu(work);
    func_02021a3c(work->printQueue);
}

void InfDungeonRetire_Free(InfDungeonRetire *work) {
    InfDungeonRetire_Exit(work);
    GFL_HeapFree(work);
}

static void InfDungeonRetire_Init(InfDungeonRetire *work, HeapID heapId) {
    work->font = GFL_FontCreate(0x17, 0, 0, FALSE, heapId);
    func_020232d8();
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_INF_DUNGEON_RETIRE, heapId);
    work->printQueue = func_02021998(heapId);
    InfDungeonRetire_InitBGs(work, heapId);
    InfDungeonRetire_InitMenuRes(work, heapId);
    InfDungeonRetire_InitWindow(work, heapId);
    func_02042ba8(FALSE, heapId);
    func_ov036_02198884(work->subscreen, heapId);
}

static void InfDungeonRetire_InitBGs(InfDungeonRetire *work, HeapID heapId) {
    int bg;
    ArcTool *arc;
    u32 gender;
    static const u32 sPlayerPaletteFiles[2] = { 0, 1 };
    static const BGSetup sMessageBGSetup = {
        0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7800), GX_BG_CHARBASE(0), 0x6800,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE,
    };
    static const BGSetup sMenuBGSetup = {
        0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7000), GX_BG_CHARBASE(0), 0x6800,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE,
    };
    static const BGSetup sFrameBGSetup = {
        0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6800), GX_BG_CHARBASE(0), 0x6800,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE,
    };

    for (bg = 4; bg <= 7; bg++) {
        GFL_BGSysSetBGEnabled(bg, FALSE);
    }
    GFL_BGSysCreateBG(6, &sMessageBGSetup, BGMODE_TEXT);
    GFL_BGSysClearScr(6);
    GFL_BGSysLoadScr(6);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysCreateBG(5, &sMenuBGSetup, BGMODE_TEXT);
    GFL_BGSysClearScr(5);
    GFL_BGSysLoadScr(5);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysCreateBG(4, &sFrameBGSetup, BGMODE_TEXT);
    GFL_BGSysClearScr(4);
    GFL_BGSysLoadScr(4);
    GFL_BGSysSetBGEnabled(4, TRUE);

    arc = GFL_ArcSysCreateFileHandle(0x11f, heapId);
    gender = getTrainerGender(GetGameDataPlayerInfo(GSYS_GetGameData(work->gsys)));
    GFL_G2DIOLoadArcNCLRDefault(arc, sPlayerPaletteFiles[gender & 1], 4, 0, 0x20, heapId);
    work->charOffset = GFL_BGSysLoadArcNCGRDynamic(arc, 2, 6, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 3, 6, 0, (u16)work->charOffset, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
}

static void InfDungeonRetire_InitWindow(InfDungeonRetire *work, HeapID heapId) {
    GFL_BGSysLoadNCLRDefault(0x17, 5, 4, 0x20, 0x20, heapId);
    work->cursorImage = LoadCursorImageEndOfHeap(5, 2, 0, heapId);
    work->window = BmpWin_CreateDynamic(5, 1, 1, 30, 7, 1, 1);
    GFL_BitmapFill(BmpWin_GetBitmap(work->window), 15);
    BmpWin_FlushChar(work->window);
    BmpWin_FlushMap(work->window);
}

static void InfDungeonRetire_InitMenuRes(InfDungeonRetire *work, HeapID heapId) {
    work->menuRes = AppTaskMenuRes_Create(4, 3, work->font, work->printQueue, heapId);
}

static void InfDungeonRetire_PrintMessage(InfDungeonRetire *work, BmpWin *window, u32 messageId) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, messageId);

    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, str, work->font);
    BmpWin_DrawFrameEndOfHeap(window, 0, work->cursorImage, 2);
    BmpWin_FlushChar(window);
    GFL_StrBufFree(str);
}

static void InfDungeonRetire_CreateMenu(InfDungeonRetire *work, u32 heapId) {
    sMenuInit.heapId = heapId;
    sMenuItems[0].str = GFL_MsgDataLoadStrbufNew(work->msgData, 1);
    sMenuItems[1].str = GFL_MsgDataLoadStrbufNew(work->msgData, 2);
    work->menu = AppTaskMenu_CreateFastFlash(&sMenuInit, work->menuRes);
    AppTaskMenu_SetLocked(work->menu, FALSE);
    GFL_StrBufFree(sMenuItems[0].str);
    GFL_StrBufFree(sMenuItems[1].str);
}

static void InfDungeonRetire_UpdateMenu(InfDungeonRetire *work) {
    if (work->menu != NULL) {
        AppTaskMenu_Update(work->menu);
    }
}

static u32 InfDungeonRetire_GetCursorPos(InfDungeonRetire *work) {
    if (work->menu != NULL) {
        return AppTaskMenu_GetCursorPos(work->menu);
    }
    return 0;
}

static BOOL InfDungeonRetire_IsFlashFinished(InfDungeonRetire *work) {
    return AppTaskMenu_IsFlashFinished(work->menu);
}

static void InfDungeonRetire_Exit(InfDungeonRetire *work) {
    InfDungeonRetire_FreeWindow(work);
    InfDungeonRetire_FreeMenuRes(work);
    InfDungeonRetire_FreeBGs(work);
    func_02021c44(work->printQueue);
    func_02021a18(work->printQueue);
    GFL_MsgDataFree(work->msgData);
    GFL_FontFree(work->font);
}

static void InfDungeonRetire_FreeWindow(InfDungeonRetire *work) {
    BmpWin_Free(work->window);
    FreeCursorImageEndOfHeap(5, work->cursorImage);
}

static void InfDungeonRetire_FreeMenu(InfDungeonRetire *work) {
    if (work->menu != NULL) {
        AppTaskMenu_Free(work->menu);
        work->menu = NULL;
    }
}

static void InfDungeonRetire_FreeMenuRes(InfDungeonRetire *work) {
    AppTaskMenuRes_Free(work->menuRes);
}

static void InfDungeonRetire_FreeBGs(InfDungeonRetire *work) {
    GFL_BGSysFreeCharMemory(6, (u16)work->charOffset, (u16)(work->charOffset >> 16));
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysSetBGEnabled(4, FALSE);
}
