#include "app/comm_tvt/ctvt_draw.h"
#include "types.h"
#include "app/comm_tvt/comm_tvt_sys.h"
#include "app/comm_tvt/ctvt_camera.h"
#include "app/comm_tvt/ctvt_comm.h"
#include "app/comm_tvt/ctvt_talk.h"
#include "app/comm_tvt/draw_system.h"
#include "constants/sound.h"
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
#include "gfl/net_handle.h"
#include "gfl/sound.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/app_menu_common.h"
#include "system/bmp_winframe.h"
#include "system/printsys.h"
#include "system/wipe.h"

// The Xtransceiver's drawing mode: the members draw on the top screen over the video, with pens of eight shapes and
// a color picked from a palette or from the screen. The strokes are sent to the others through ctvt_comm.c

// The color a pen starts with: black in Black 2, white in White 2
#ifdef BLACK2
#define CTVT_DRAW_DEFAULT_COLOR 0x8000
#else
#define CTVT_DRAW_DEFAULT_COLOR 0xffff
#endif

#define CTVT_DRAW_TOOL_BUTTONS 4
#define CTVT_DRAW_BUTTONS 6
#define CTVT_DRAW_MENU_ITEMS 5
#define CTVT_DRAW_TOUCH_MAX 7

// How far the buttons of the bottom screen slide in, in pixels
#define CTVT_DRAW_SLIDE 24

// The direct color of the top screen's bitmap that is drawn, past the transparent ones
#define CTVT_DRAW_OPAQUE 0x8000

enum {
    DRAW_STATE_FADE_IN,
    DRAW_STATE_WAIT_FADE_IN,
    DRAW_STATE_FADE_OUT,
    DRAW_STATE_WAIT_FADE_OUT,
    DRAW_STATE_MENU,
    DRAW_STATE_DRAW,
    DRAW_STATE_DISCONNECTED,
    DRAW_STATE_WAIT_DISCONNECTED,
    DRAW_STATE_SEND_PARENT_EXIT,
    DRAW_STATE_PARENT_LEFT,
    DRAW_STATE_WAIT_PARENT_LEFT,
    DRAW_STATE_SYNC_EXIT,
    DRAW_STATE_WAIT_SYNC_EXIT,
    DRAW_STATE_ALONE,
    DRAW_STATE_WAIT_ALONE,
};

// The tools of the drawing mode
enum {
    DRAW_TOOL_PEN,
    DRAW_TOOL_PICK,
    DRAW_TOOL_ERASER,
    DRAW_TOOL_STAMP,
};

struct CtvtDraw {
    int state;
    u8 slideTarget;
    u8 slide;
    BOOL inputHandled;
    BOOL stroking;
    BOOL penMenuOpen;
    BOOL stampMenuOpen;
    BOOL drawEnabled;
    BOOL exit;
    BOOL waitSound;
    BOOL stampChosen;
    int tool;
    u32 lastX;
    u32 lastY;
    u16 color;
    u8 pen;
    u8 stamp;
    BOOL menuWindowPending;
    BOOL menuExpanded;
    BOOL titleWindowPending;
    BOOL messageWindowPending;
    BmpWin *menuWindow;
    BmpWin *titleWindow;
    BmpWin *messageWindow;
    u16 waitFrames;
    ClActor *buttons[CTVT_DRAW_BUTTONS];
    ClActor *menuItems[CTVT_DRAW_MENU_ITEMS];
    ClActor *penMenu;
    ClActor *stampMenu;
    ClActor *menuCursor;
    ClActor *selectFrame;
    u16 cursorColor;
    u16 cursorPhase;
    int syncState;
};

static void CtvtDraw_UpdateMenu(CommTvtWork *sys, CtvtDraw *draw);
static void CtvtDraw_UpdateDraw(CommTvtWork *sys, CtvtDraw *draw);
static void CtvtDraw_UpdateStroke(CommTvtWork *sys, CtvtDraw *draw);
static void CtvtDraw_UpdateSlide(CommTvtWork *sys, CtvtDraw *draw);
static void CtvtDraw_RefreshButtons(CommTvtWork *sys, CtvtDraw *draw);
static void CtvtDraw_PrintMenu(CommTvtWork *sys, CtvtDraw *draw, BOOL expanded);
static BOOL CtvtDraw_CheckDisconnect(CommTvtWork *sys, CtvtDraw *draw);
static void CtvtDraw_PrintMessage(CommTvtWork *sys, CtvtDraw *draw, u32 msgId);
static void CtvtDraw_CloseMenus(CommTvtWork *sys, CtvtDraw *draw);
static void CtvtDraw_UpdateCancel(CommTvtWork *sys, CtvtDraw *draw);

static const u16 sUnusedColors[] = { 0x1c13, 0x1313, 0x1c1c, 0x4e73, 0x739c };

CtvtDraw *CtvtDraw_Create(CommTvtWork *sys, HeapID heapId) {
    CtvtDraw *draw = GFL_HeapAllocate(heapId, sizeof(CtvtDraw), TRUE, "ctvt_draw.c", 219);

    draw->pen = DRAW_PEN_DOT;
    draw->stamp = DRAW_PEN_HEART;
    draw->tool = DRAW_TOOL_PEN;
    draw->stampChosen = FALSE;
    draw->color = CTVT_DRAW_DEFAULT_COLOR;
    return draw;
}

void CtvtDraw_Delete(CommTvtWork *sys, CtvtDraw *draw) {
    GFL_HeapFree(draw);
}

// The resources of the actors are the system's: characters, palette and cells
static inline ClActor *CtvtDraw_CreateActor(CommTvtWork *sys, int chars, int palette, int cells,
                                            const ClActorSetup *setup, u16 surface, HeapID heapId) {
    return ClActor_Create(CommTvt_GetClActUnit(sys), CommTvt_GetObjResource(sys, chars),
                          CommTvt_GetObjResource(sys, palette), CommTvt_GetObjResource(sys, cells), setup, surface,
                          heapId);
}

void CtvtDraw_Enter(CommTvtWork *sys, CtvtDraw *draw) {
    HeapID heapId = CommTvt_GetHeapId(sys);
    ArcTool *arc = CommTvt_GetArc(sys);
    ClActorSetup setup;
    Font *font;
    MsgData *msgData;
    PrintQueue *queue;
    StrBuf *str;
    u8 i;

    loadBGScrToVramByFileNoReserveNegAlign(arc, 16, 7, 0, 0, FALSE, heapId);
    GFL_BGSysLoadScr(7);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GX_SetDispSelect(0);
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    GFL_BGSysClearScr(0);
    GFL_G2DIOLoadNSCRSync(arc, AppMenuCommon_GetBarScreenFile(), 0, 0x60, 0, 0x600, FALSE, heapId);
    GFL_BGSysSetScrPaletteNo(0, 0, 21, 32, 3, 0);
    GFL_BGSysLoadScr(0);
    GFL_BGSysMoveBG(0, BG_MOVE_SET_Y, 0);
    GFL_ArcToolFree(arc);

    setup.x = 16;
    setup.y = 204;
    setup.sequence = 1;
    setup.priority = 8;
    setup.bgPriority = 0;
    draw->buttons[0] = CtvtDraw_CreateActor(sys, 4, 0, 8, &setup, 0, heapId);
    setup.x = 56;
    setup.sequence = 3;
    draw->buttons[1] = CtvtDraw_CreateActor(sys, 4, 0, 8, &setup, 0, heapId);
    setup.x = 96;
    setup.sequence = 5;
    draw->buttons[2] = CtvtDraw_CreateActor(sys, 4, 0, 8, &setup, 0, heapId);
    setup.x = 136;
    setup.sequence = 7;
    draw->buttons[3] = CtvtDraw_CreateActor(sys, 4, 0, 8, &setup, 0, heapId);
    setup.x = 200;
    if (CommTvt_GetViewSwapped(sys) == TRUE) {
        setup.sequence = 12;
    } else {
        setup.sequence = 10;
    }
    draw->buttons[4] = CtvtDraw_CreateActor(sys, 4, 0, 8, &setup, 0, heapId);
    setup.x = 224;
    setup.y = 192;
    setup.sequence = 1;
    draw->buttons[5] = CtvtDraw_CreateActor(sys, 7, 3, 10, &setup, 0, heapId);
    ClActor_SetAutoAnim(draw->buttons[5], TRUE);
    for (i = 0; i < CTVT_DRAW_BUTTONS; i++) {
        ClActor_SetVisible(draw->buttons[i], TRUE);
    }

    setup.x = 56;
    setup.y = 72;
    setup.sequence = 14;
    draw->menuItems[0] = CtvtDraw_CreateActor(sys, 5, 1, 9, &setup, 1, heapId);
    setup.y = 92;
    setup.sequence = 15;
    draw->menuItems[1] = CtvtDraw_CreateActor(sys, 5, 1, 9, &setup, 1, heapId);
    setup.y = 112;
    setup.sequence = 16;
    draw->menuItems[2] = CtvtDraw_CreateActor(sys, 5, 1, 9, &setup, 1, heapId);
    setup.y = 132;
    setup.sequence = 17;
    draw->menuItems[3] = CtvtDraw_CreateActor(sys, 5, 1, 9, &setup, 1, heapId);
    setup.x = 44;
    setup.y = 140;
    setup.sequence = 1;
    draw->menuItems[4] = CtvtDraw_CreateActor(sys, 6, 2, 10, &setup, 1, heapId);
    for (i = 0; i < CTVT_DRAW_MENU_ITEMS; i++) {
        ClActor_SetVisible(draw->menuItems[i], FALSE);
    }

    setup.x = 16;
    setup.y = 160;
    setup.sequence = 15;
    draw->penMenu = CtvtDraw_CreateActor(sys, 4, 0, 8, &setup, 0, heapId);
    ClActor_SetVisible(draw->penMenu, FALSE);
    setup.x = 136;
    setup.y = 160;
    setup.sequence = 16;
    draw->stampMenu = CtvtDraw_CreateActor(sys, 4, 0, 8, &setup, 0, heapId);
    ClActor_SetVisible(draw->stampMenu, FALSE);
    setup.priority = 0;
    draw->menuCursor = CtvtDraw_CreateActor(sys, 4, 0, 8, &setup, 0, heapId);
    ClActor_SetVisible(draw->menuCursor, FALSE);
    setup.sequence = 25;
    setup.priority = 2;
    draw->selectFrame = CtvtDraw_CreateActor(sys, 4, 0, 8, &setup, 0, heapId);
    ClActor_SetVisible(draw->selectFrame, FALSE);
    CtvtDraw_RefreshButtons(sys, draw);
    draw->menuWindow = NULL;
    draw->messageWindow = NULL;

    font = CommTvt_GetFont(sys);
    msgData = CommTvt_GetMsgData(sys);
    queue = CommTvt_GetPrintQueue(sys);
    draw->titleWindow = BmpWin_CreateDynamic(4, 3, 1, 28, 2, 10, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(draw->titleWindow), 0);
    str = GFL_MsgDataLoadStrbufNew(msgData, 11);
    PrintQueue_PrintColor(queue, BmpWin_GetBitmap(draw->titleWindow), 1, 0, str, font, 0x440);
    GFL_StrBufFree(str);
    draw->titleWindowPending = TRUE;
    draw->state = DRAW_STATE_FADE_IN;
    draw->slideTarget = CTVT_DRAW_SLIDE;
    draw->slide = CTVT_DRAW_SLIDE;
    draw->cursorPhase = 0;
    draw->syncState = 0;
    draw->stroking = FALSE;
    draw->inputHandled = FALSE;
    draw->penMenuOpen = FALSE;
    draw->stampMenuOpen = FALSE;
    draw->menuWindowPending = FALSE;
    draw->messageWindowPending = FALSE;
    draw->drawEnabled = FALSE;
    draw->exit = FALSE;
    draw->waitSound = FALSE;
    *(vu16 *)(HW_BG_PLTT + 0x21a) = draw->color & 0x7fff;
    CtvtDraw_PrintMenu(sys, draw, FALSE);
    GFL_NetSetIconScreen(TRUE, heapId);
    CommTvt_HideSlotMarker(sys);
}

static inline void CtvtDraw_ClearWindow(BmpWin *window) {
    BmpWin_ClearScreen(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
}

void CtvtDraw_Leave(CommTvtWork *sys, CtvtDraw *draw) {
    HeapID heapId = CommTvt_GetHeapId(sys);
    u8 i;

    if (draw->tool == DRAW_TOOL_PICK) {
        if (draw->stampChosen == FALSE) {
            draw->tool = DRAW_TOOL_PEN;
        } else {
            draw->tool = DRAW_TOOL_STAMP;
        }
    }
    GFL_NetSetIconScreen(FALSE, heapId);
    BmpWin_ClearFrame(draw->menuWindow, 0);
    CtvtDraw_ClearWindow(draw->menuWindow);
    BmpWin_Free(draw->menuWindow);
    CtvtDraw_ClearWindow(draw->titleWindow);
    BmpWin_Free(draw->titleWindow);
    if (draw->messageWindow != NULL) {
        CtvtDraw_ClearWindow(draw->messageWindow);
        BmpWin_Free(draw->messageWindow);
        draw->messageWindow = NULL;
    }
    for (i = 0; i < CTVT_DRAW_MENU_ITEMS; i++) {
        ClActor_Delete(draw->menuItems[i]);
    }
    for (i = 0; i < CTVT_DRAW_BUTTONS; i++) {
        ClActor_Delete(draw->buttons[i]);
    }
    ClActor_Delete(draw->selectFrame);
    ClActor_Delete(draw->menuCursor);
    ClActor_SetVisible(draw->stampMenu, FALSE);
    ClActor_Delete(draw->stampMenu);
    ClActor_SetVisible(draw->penMenu, FALSE);
    ClActor_Delete(draw->penMenu);
    GFL_BGSysClearScr(0);
    GFL_BGSysLoadScr(0);
    GFL_BGSysMoveBG(0, BG_MOVE_SET_Y, 0);
    GFL_BGSysClearScr(6);
    GFL_BGSysLoadScr(6);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GX_SetDispSelect(1);
    CommTvt_ClearNameWindows(sys);
}

int CtvtDraw_Main(CommTvtWork *sys, CtvtDraw *draw) {
    HeapID heapId = CommTvt_GetHeapId(sys);
    BOOL done;
    u8 i;

    draw->inputHandled = FALSE;
    switch (draw->state) {
    case DRAW_STATE_FADE_IN:
        if (func_ov257_021aaa74(sys) == TRUE) {
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, heapId);
        } else {
            GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, heapId);
        }
        draw->state = DRAW_STATE_WAIT_FADE_IN;
        break;
    case DRAW_STATE_WAIT_FADE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            draw->drawEnabled = TRUE;
            draw->state = DRAW_STATE_MENU;
        }
        break;
    case DRAW_STATE_FADE_OUT: {
        CtvtCamera *camera = CommTvt_GetCamera(sys);

        if (draw->waitSound && CtvtCamera_IsSoundDone(sys, camera) != TRUE) {
            break;
        }
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, heapId);
        draw->state = DRAW_STATE_WAIT_FADE_OUT;
        draw->drawEnabled = FALSE;
        break;
    }
    case DRAW_STATE_WAIT_FADE_OUT:
        if (GFL_WipeIsFinished() == TRUE) {
            if (draw->exit == FALSE) {
                if (draw->syncState != 0) {
                    CtvtDraw_UpdateCancel(sys, draw);
                    break;
                }
                return COMM_TVT_MODE_TALK;
            }
            return COMM_TVT_MODE_EXIT;
        }
        break;
    case DRAW_STATE_MENU:
        if (CtvtDraw_CheckDisconnect(sys, draw)) {
            break;
        }
        CtvtDraw_UpdateCancel(sys, draw);
        CtvtDraw_UpdateMenu(sys, draw);
        break;
    case DRAW_STATE_DRAW:
        if (CtvtDraw_CheckDisconnect(sys, draw)) {
            break;
        }
        CtvtDraw_UpdateCancel(sys, draw);
        CtvtDraw_UpdateDraw(sys, draw);
        break;
    case DRAW_STATE_DISCONNECTED:
        CtvtComm_Disconnect(sys, CommTvt_GetComm(sys));
        CtvtDraw_PrintMessage(sys, draw, 36);
        CommTvt_SetMessageActive(sys, TRUE);
        draw->state = DRAW_STATE_WAIT_DISCONNECTED;
        CtvtCamera_EndRecording(sys, CommTvt_GetCamera(sys));
        draw->waitSound = TRUE;
        break;
    case DRAW_STATE_WAIT_DISCONNECTED:
        if (GFL_HIDIsTouchPressed() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            draw->exit = TRUE;
            draw->state = DRAW_STATE_FADE_OUT;
            CommTvt_SetMessageActive(sys, TRUE);
            CtvtCamera_StopCamera(sys, CommTvt_GetCamera(sys));
            draw->waitSound = TRUE;
        }
        break;
    case DRAW_STATE_SEND_PARENT_EXIT:
        if (CtvtComm_SendPacket(sys, CommTvt_GetComm(sys), CTVT_PACKET_UNK_A, 0) == TRUE) {
            draw->state = DRAW_STATE_PARENT_LEFT;
        }
        break;
    case DRAW_STATE_PARENT_LEFT:
        CtvtComm_Disconnect(sys, CommTvt_GetComm(sys));
        CtvtDraw_PrintMessage(sys, draw, 37);
        CommTvt_SetMessageActive(sys, TRUE);
        draw->state = DRAW_STATE_WAIT_PARENT_LEFT;
        draw->waitFrames = 0;
        CtvtCamera_EndRecording(sys, CommTvt_GetCamera(sys));
        draw->waitSound = TRUE;
        break;
    case DRAW_STATE_WAIT_PARENT_LEFT:
        draw->waitFrames++;
        if (GFL_HIDIsTouchPressed() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) ||
            draw->waitFrames >= 300) {
            draw->state = DRAW_STATE_SYNC_EXIT;
        }
        break;
    case DRAW_STATE_SYNC_EXIT:
        CtvtComm_StartSync(sys, CommTvt_GetComm(sys), 47);
        draw->state = DRAW_STATE_WAIT_SYNC_EXIT;
        break;
    case DRAW_STATE_WAIT_SYNC_EXIT:
        if (CtvtComm_IsSynced(sys, CommTvt_GetComm(sys), 47) == TRUE) {
            draw->exit = TRUE;
            draw->state = DRAW_STATE_FADE_OUT;
            CommTvt_SetMessageActive(sys, TRUE);
            CtvtCamera_StopCamera(sys, CommTvt_GetCamera(sys));
            draw->waitSound = TRUE;
        }
        break;
    case DRAW_STATE_ALONE:
        CtvtComm_Disconnect(sys, CommTvt_GetComm(sys));
        CtvtDraw_PrintMessage(sys, draw, 38);
        CommTvt_SetMessageActive(sys, TRUE);
        draw->state = DRAW_STATE_WAIT_ALONE;
        CtvtCamera_EndRecording(sys, CommTvt_GetCamera(sys));
        draw->waitSound = TRUE;
        break;
    case DRAW_STATE_WAIT_ALONE:
        if (GFL_HIDIsTouchPressed() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            draw->exit = TRUE;
            draw->state = DRAW_STATE_FADE_OUT;
            CommTvt_SetMessageActive(sys, TRUE);
            CtvtCamera_StopCamera(sys, CommTvt_GetCamera(sys));
            draw->waitSound = TRUE;
        }
        break;
    }

    CtvtDraw_UpdateSlide(sys, draw);
    if (draw->drawEnabled == TRUE) {
        CtvtDraw_UpdateStroke(sys, draw);
    }
    if (draw->menuWindowPending == TRUE) {
        PrintQueue *queue = CommTvt_GetPrintQueue(sys);

        if (!PrintQueue_IsBitmapPending(queue, BmpWin_GetBitmap(draw->menuWindow))) {
            BmpWin_DrawFrame(draw->menuWindow, 2, 0x140, 9);
            BmpWin_FlushChar(draw->menuWindow);
            BmpWin_FlushMap(draw->menuWindow);
            GFL_BGSysQueueScrLoad(4);
            draw->menuWindowPending = FALSE;
            if (draw->menuExpanded == TRUE) {
                for (i = 0; i < CTVT_DRAW_MENU_ITEMS; i++) {
                    ClActor_SetVisible(draw->menuItems[i], TRUE);
                }
            } else {
                for (i = 0; i < CTVT_DRAW_MENU_ITEMS; i++) {
                    ClActor_SetVisible(draw->menuItems[i], FALSE);
                }
            }
        }
    }
    if (draw->titleWindowPending == TRUE) {
        PrintQueue *queue = CommTvt_GetPrintQueue(sys);

        if (!PrintQueue_IsBitmapPending(queue, BmpWin_GetBitmap(draw->titleWindow))) {
            BmpWin_FlushChar(draw->titleWindow);
            BmpWin_FlushMap(draw->titleWindow);
            GFL_BGSysLoadScr(4);
            draw->titleWindowPending = FALSE;
        }
    }
    if (draw->messageWindowPending == TRUE && draw->messageWindow != NULL) {
        PrintQueue *queue = CommTvt_GetPrintQueue(sys);

        if (!PrintQueue_IsBitmapPending(queue, BmpWin_GetBitmap(draw->messageWindow))) {
            BmpWin_FlushChar(draw->messageWindow);
            BmpWin_FlushMap(draw->messageWindow);
            GFL_BGSysLoadScr(0);
            draw->messageWindowPending = FALSE;
        }
    }
    if (draw->state <= DRAW_STATE_DRAW && CommTvt_GetViewSwapped(sys) == FALSE) {
        CtvtCamera *camera = CommTvt_GetCamera(sys);

        if (GCTX_HIDGetPressedKeys() & PAD_KEY_UP) {
            CtvtCamera_SetEffect(sys, camera, CAMERA_EFFECT_NONE);
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
            CtvtCamera_SetEffect(sys, camera, CAMERA_EFFECT_SEPIA01);
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_DOWN) {
            CtvtCamera_SetEffect(sys, camera, CAMERA_EFFECT_MONO);
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
            CtvtCamera_SetEffect(sys, camera, CAMERA_EFFECT_NEGATIVE);
        }
    }
    if (CommTvt_GetSelfIndex(sys) == 0 && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_L) &&
        CtvtCamera_IsRedrawing(sys, CommTvt_GetCamera(sys)) == FALSE) {
        CommTvt_ToggleZoom(sys);
        GFL_SndSEPlay(SEQ_SE_SYS_45);
    }

    // The cursor of the color palette pulses between dark and light
    if (draw->cursorPhase + 0x400 >= 0x10000) {
        draw->cursorPhase = draw->cursorPhase + 0x400 - 0x10000;
    } else {
        draw->cursorPhase += 0x400;
    }
    {
        u8 level = ((s16)((FX_CosIdx(draw->cursorPhase) + FX32_ONE) / 2) * -9 >> FX32_SHIFT) + 28;

        draw->cursorColor = GX_RGB(level, level, level);
    }
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_MAIN, 0xde, &draw->cursorColor,
                                       sizeof(draw->cursorColor));
    return COMM_TVT_MODE_DRAW;
}

static void CtvtDraw_UpdateMenu(CommTvtWork *sys, CtvtDraw *draw) {
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) && draw->menuWindowPending == FALSE) {
        if (ClActor_IsAnimActive(draw->buttons[5]) == FALSE || ClActor_GetSequence(draw->buttons[5]) == 1) {
            draw->state = DRAW_STATE_DRAW;
            draw->slideTarget = 0;
            CtvtDraw_PrintMenu(sys, draw, TRUE);
            ClActor_SetSequence(draw->buttons[5], 1);
            GFL_SndSEPlay(SEQ_SE_SYS_45);
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        draw->state = DRAW_STATE_FADE_OUT;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_SELECT | PAD_BUTTON_START)) {
        CommTvt_ToggleViewSwapped(sys);
        CtvtDraw_RefreshButtons(sys, draw);
        GFL_SndSEPlay(SEQ_SE_SYS_48);
    }
}

// The animations of each tool's button, when not chosen and when chosen
static const u8 sPenButtonAnims[] = { 1, 2 };
static const u8 sPickButtonAnims[] = { 3, 4 };
static const u8 sEraserButtonAnims[] = { 5, 6 };
static const u8 sStampButtonAnims[] = { 7, 8 };

static const int sPens[] = { DRAW_PEN_ROUND, DRAW_PEN_SMALL, DRAW_PEN_DOT };

static const int sStamps[] = { DRAW_PEN_HEART, DRAW_PEN_POKE_BALL, DRAW_PEN_FACE, DRAW_PEN_STAR, DRAW_PEN_DROP };

static const TouchRect sStampButtons[CTVT_DRAW_TOUCH_MAX] = {
    { 0x98, 0xa8, 0x60, 0x70 }, { 0x98, 0xa8, 0x70, 0x80 }, { 0x98, 0xa8, 0x80, 0x90 },
    { 0x98, 0xa8, 0x90, 0xa0 }, { 0x98, 0xa8, 0xa0, 0xb0 }, { TOUCH_RECT_END },
};

static const TouchRect sToolButtons[CTVT_DRAW_TOUCH_MAX] = {
    { 0xa8, 0xc0, 0x04, 0x1c }, { 0xa8, 0xc0, 0x2c, 0x44 }, { 0xa8, 0xc0, 0x54, 0x6c }, { 0xa8, 0xc0, 0x7c, 0x94 },
    { 0xa8, 0xc0, 0xbc, 0xd4 }, { 0xa8, 0xc0, 0xe0, 0xf8 }, { TOUCH_RECT_END },
};

static const TouchRect sPenButtons[CTVT_DRAW_TOUCH_MAX] = {
    { 0x98, 0xa8, 0x08, 0x18 },
    { 0x98, 0xa8, 0x18, 0x28 },
    { 0x98, 0xa8, 0x28, 0x38 },
    { TOUCH_RECT_END },
};

static void CtvtDraw_UpdateDraw(CommTvtWork *sys, CtvtDraw *draw) {
    s32 hit = GFL_HIDGetTouchPressedRect(sToolButtons);

    if (hit == DRAW_TOOL_PEN && ClActor_IsVisible(draw->menuCursor) == FALSE) {
        draw->tool = DRAW_TOOL_PEN;
        draw->stampChosen = FALSE;
        CtvtDraw_RefreshButtons(sys, draw);
        draw->inputHandled = TRUE;
        draw->penMenuOpen = TRUE;
        ClActor_SetVisible(draw->penMenu, TRUE);
        ClActor_SetAutoAnim(draw->penMenu, TRUE);
        GFL_SndSEPlay(SEQ_SE_SYS_45);
    }
    if (hit == DRAW_TOOL_PICK) {
        draw->tool = DRAW_TOOL_PICK;
        CtvtDraw_RefreshButtons(sys, draw);
        draw->inputHandled = TRUE;
        GFL_SndSEPlay(SEQ_SE_SYS_45);
    }
    if (hit == DRAW_TOOL_ERASER) {
        draw->tool = DRAW_TOOL_ERASER;
        CtvtDraw_RefreshButtons(sys, draw);
        draw->inputHandled = TRUE;
        GFL_SndSEPlay(SEQ_SE_SYS_45);
    }
    if (hit == DRAW_TOOL_STAMP && ClActor_IsVisible(draw->menuCursor) == FALSE) {
        draw->tool = DRAW_TOOL_STAMP;
        draw->stampChosen = TRUE;
        CtvtDraw_RefreshButtons(sys, draw);
        draw->inputHandled = TRUE;
        draw->stampMenuOpen = TRUE;
        ClActor_SetVisible(draw->stampMenu, TRUE);
        ClActor_SetAutoAnim(draw->stampMenu, TRUE);
        GFL_SndSEPlay(SEQ_SE_SYS_45);
    }
    if (hit == 4) {
        CommTvt_ToggleViewSwapped(sys);
        CtvtDraw_RefreshButtons(sys, draw);
        draw->inputHandled = TRUE;
        GFL_SndSEPlay(SEQ_SE_SYS_48);
    }
    if (draw->penMenuOpen == TRUE) {
        s32 pen = GFL_HIDGetTouchPressedRect(sPenButtons);

        if (pen != TOUCH_RECT_NONE) {
            draw->pen = sPens[pen];
            draw->inputHandled = TRUE;
            {
                ClActorPos pos = { 0x10, 0xa0 };

                ClActor_SetVisible(draw->menuCursor, TRUE);
                ClActor_RestartAnim(draw->menuCursor);
                ClActor_SetAutoAnim(draw->menuCursor, TRUE);
                ClActor_SetSequence(draw->menuCursor, pen + 17);
                ClActor_SetSurfacePos(draw->menuCursor, &pos, 0);
            }
            ClActor_SetVisible(draw->selectFrame, FALSE);
            GFL_SndSEPlay(SEQ_SE_SYS_45);
        }
    }
    if (draw->stampMenuOpen == TRUE) {
        s32 stamp = GFL_HIDGetTouchPressedRect(sStampButtons);

        if (stamp != TOUCH_RECT_NONE) {
            draw->stamp = sStamps[stamp];
            draw->inputHandled = TRUE;
            GFL_SndSEPlay(SEQ_SE_SYS_45);
            {
                ClActorPos pos = { 0x88, 0xa0 };

                ClActor_SetVisible(draw->menuCursor, TRUE);
                ClActor_RestartAnim(draw->menuCursor);
                ClActor_SetAutoAnim(draw->menuCursor, TRUE);
                ClActor_SetSequence(draw->menuCursor, stamp + 20);
                ClActor_SetSurfacePos(draw->menuCursor, &pos, 0);
            }
            ClActor_SetVisible(draw->selectFrame, FALSE);
        }
    }
    if (((GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) || hit == 5) &&
        draw->menuWindowPending == FALSE) {
        draw->state = DRAW_STATE_MENU;
        draw->slideTarget = CTVT_DRAW_SLIDE;
        draw->inputHandled = TRUE;
        if (GFL_HIDIsTouchHeld() == TRUE) {
            u32 x, y;

            GFL_HIDGetTouchHeldPoint(&x, &y);
            draw->stroking = TRUE;
            draw->lastX = x;
            draw->lastY = y;
        }
        CtvtDraw_PrintMenu(sys, draw, FALSE);
        CtvtDraw_CloseMenus(sys, draw);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        ClActor_SetSequence(draw->buttons[5], 9);
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_SELECT | PAD_BUTTON_START)) {
        CommTvt_ToggleViewSwapped(sys);
        CtvtDraw_RefreshButtons(sys, draw);
        GFL_SndSEPlay(SEQ_SE_SYS_48);
    }
    if (ClActor_IsVisible(draw->menuCursor) == TRUE && ClActor_IsAnimActive(draw->menuCursor) == FALSE) {
        CtvtDraw_CloseMenus(sys, draw);
    }
    if (draw->penMenuOpen == TRUE && ClActor_IsAnimActive(draw->penMenu) == FALSE &&
        ClActor_IsVisible(draw->selectFrame) == FALSE) {
        ClActorPos pos = { 0x10, 0xa0 };

        switch (draw->pen) {
        case DRAW_PEN_SMALL:
            pos.x += 16;
            break;
        case DRAW_PEN_DOT:
            pos.x += 32;
            break;
        case DRAW_PEN_ROUND:
            break;
        }
        ClActor_SetSurfacePos(draw->selectFrame, &pos, 0);
        ClActor_SetVisible(draw->selectFrame, TRUE);
    }
    if (draw->stampMenuOpen == TRUE && ClActor_IsAnimActive(draw->stampMenu) == FALSE &&
        ClActor_IsVisible(draw->selectFrame) == FALSE) {
        ClActorPos pos = { 0x88, 0xa0 };

        switch (draw->stamp) {
        case DRAW_PEN_HEART:
            pos.x -= 32;
            break;
        case DRAW_PEN_POKE_BALL:
            pos.x -= 16;
            break;
        case DRAW_PEN_STAR:
            pos.x += 16;
            break;
        case DRAW_PEN_DROP:
            pos.x += 32;
            break;
        }
        ClActor_SetSurfacePos(draw->selectFrame, &pos, 0);
        ClActor_SetVisible(draw->selectFrame, TRUE);
    }
}

static void CtvtDraw_UpdateStroke(CommTvtWork *sys, CtvtDraw *draw) {
    u32 x, y;
    u8 memberCount;
    CtvtComm *comm;
    DrawSystem *drawSystem;
    DrawCommand *cmd;

    GFL_HIDGetTouchPressedPoint(&x, &y);
    if (CommTvt_GetInputLocked(sys) == TRUE) {
        return;
    }
    if (GFL_HIDIsTouchHeld() == FALSE) {
        draw->stroking = FALSE;
    }
    if (GFL_HIDIsTouchPressed() == TRUE && draw->inputHandled == FALSE && draw->state == DRAW_STATE_DRAW && y >= 168) {
        if (draw->tool == DRAW_TOOL_PICK) {
            if (draw->stampChosen == FALSE) {
                draw->tool = DRAW_TOOL_PEN;
            } else {
                draw->tool = DRAW_TOOL_STAMP;
            }
            CtvtDraw_RefreshButtons(sys, draw);
        }
        CtvtDraw_CloseMenus(sys, draw);
        return;
    }
    if ((draw->state == DRAW_STATE_MENU || draw->state == DRAW_STATE_DRAW) && GFL_HIDIsTouchPressed() == TRUE &&
        draw->inputHandled == FALSE && draw->tool == DRAW_TOOL_STAMP) {
        BOOL full;

        memberCount = CommTvt_GetMemberCount(sys);
        comm = CommTvt_GetComm(sys);
        drawSystem = CommTvt_GetDrawSystem(sys);
        cmd = CtvtComm_GetDrawSlot(sys, comm, &full);
        if (full == FALSE) {
            cmd->x0 = x;
            cmd->y0 = y;
            cmd->x1 = x;
            cmd->y1 = y;
            cmd->pen = draw->stamp;
            cmd->color = draw->color;
            if (memberCount == 1) {
                DrawSystem_AddCommand(drawSystem, cmd);
            } else {
                CtvtComm_CommitDrawSlot(sys, comm);
            }
        }
    }
    if (GFL_HIDIsTouchPressed() == TRUE && draw->inputHandled == FALSE) {
        if (draw->tool == DRAW_TOOL_PICK) {
            u16 *screen = (u16 *)((u32)G2_GetBG2ScrPtr() + x * 2 + y * 512);
            u16 *camera;

            G2_GetBG3ScrPtr();
            if (CommTvt_IsZoomed(sys) == TRUE) {
                camera = (u16 *)((u32)G2_GetBG3ScrPtr() + (x / 2) * 2 + (y / 2) * 512);
            } else {
                camera = (u16 *)((u32)G2_GetBG3ScrPtr() + x * 2 + y * 512);
            }
            if (*screen & CTVT_DRAW_OPAQUE) {
                draw->color = *screen;
            } else if (*camera & CTVT_DRAW_OPAQUE) {
                draw->color = *camera;
            } else {
                draw->color = CTVT_DRAW_OPAQUE;
            }
            *(vu16 *)(HW_BG_PLTT + 0x21a) = draw->color & 0x7fff;
            if (draw->stampChosen == FALSE) {
                draw->tool = DRAW_TOOL_PEN;
            } else {
                draw->tool = DRAW_TOOL_STAMP;
            }
            CtvtDraw_RefreshButtons(sys, draw);
            GFL_SndSEPlay(SEQ_SE_SYS_46);
        } else {
            draw->stroking = TRUE;
            draw->lastX = 0xffff;
            draw->lastY = 0xffff;
            CtvtDraw_CloseMenus(sys, draw);
        }
    }
    if ((draw->tool == DRAW_TOOL_PEN || draw->tool == DRAW_TOOL_ERASER) && draw->stroking == TRUE) {
        u32 touchX, touchY;

        GFL_HIDGetTouchHeldPoint(&touchX, &touchY);
        if (draw->stroking == TRUE) {
            if (draw->lastX != 0xffff && draw->lastY != 0xffff) {
                BOOL full;

                memberCount = CommTvt_GetMemberCount(sys);
                comm = CommTvt_GetComm(sys);
                drawSystem = CommTvt_GetDrawSystem(sys);
                cmd = CtvtComm_GetDrawSlot(sys, comm, &full);
                if (full == FALSE) {
                    cmd->x0 = draw->lastX;
                    cmd->y0 = draw->lastY;
                    cmd->x1 = touchX;
                    cmd->y1 = touchY;
                    cmd->pen = draw->pen;
                    if (draw->tool == DRAW_TOOL_ERASER) {
                        cmd->color = 0;
                        cmd->pen = DRAW_PEN_ROUND;
                    } else {
                        cmd->color = draw->color;
                    }
                    if (memberCount == 1) {
                        DrawSystem_AddCommand(drawSystem, cmd);
                    } else {
                        CtvtComm_CommitDrawSlot(sys, comm);
                    }
                } else {
                    draw->stroking = FALSE;
                }
            }
            draw->lastX = touchX;
            draw->lastY = touchY;
        }
    }
}

static void CtvtDraw_UpdateSlide(CommTvtWork *sys, CtvtDraw *draw) {
    BOOL moved = FALSE;
    u8 i;

    if (ClActor_IsAnimActive(draw->buttons[5]) == FALSE || ClActor_GetSequence(draw->buttons[5]) == 1) {
        if (draw->slideTarget < draw->slide) {
            draw->slide -= CTVT_DRAW_SLIDE;
            moved = TRUE;
        } else if (draw->slideTarget > draw->slide) {
            draw->slide += CTVT_DRAW_SLIDE;
            moved = TRUE;
        }
    }
    if (moved == TRUE) {
        GFL_BGSysMoveBGReq(0, BG_MOVE_SET_Y, CTVT_DRAW_SLIDE - draw->slide);
        for (i = 0; i < CTVT_DRAW_BUTTONS; i++) {
            ClActorPos pos;

            ClActor_GetSurfacePos(draw->buttons[i], &pos, 0);
            pos.y = draw->slide + 180;
            if (i == 5) {
                pos.y -= 12;
            }
            ClActor_SetSurfacePos(draw->buttons[i], &pos, 0);
        }
    }
}

static void CtvtDraw_RefreshButtons(CommTvtWork *sys, CtvtDraw *draw) {
    ClActor_SetSequence(draw->buttons[0], sPenButtonAnims[draw->tool == DRAW_TOOL_PEN ? 1 : 0]);
    ClActor_SetSequence(draw->buttons[1], sPickButtonAnims[draw->tool == DRAW_TOOL_PICK ? 1 : 0]);
    ClActor_SetSequence(draw->buttons[2], sEraserButtonAnims[draw->tool == DRAW_TOOL_ERASER ? 1 : 0]);
    ClActor_SetSequence(draw->buttons[3], sStampButtonAnims[draw->tool == DRAW_TOOL_STAMP ? 1 : 0]);
    if (CommTvt_GetViewSwapped(sys) == TRUE) {
        ClActor_SetSequence(draw->buttons[4], 12);
    } else {
        ClActor_SetSequence(draw->buttons[4], 10);
    }
    if (draw->tool != DRAW_TOOL_PEN && draw->penMenuOpen == TRUE) {
        draw->penMenuOpen = FALSE;
        ClActor_SetVisible(draw->penMenu, FALSE);
        ClActor_RestartAnim(draw->penMenu);
        ClActor_SetAnimTime(draw->penMenu, 0);
        ClActor_SetAutoAnim(draw->penMenu, FALSE);
        ClActor_SetVisible(draw->selectFrame, FALSE);
    }
    if (draw->tool != DRAW_TOOL_STAMP && draw->stampMenuOpen == TRUE) {
        draw->stampMenuOpen = FALSE;
        ClActor_SetVisible(draw->stampMenu, FALSE);
        ClActor_RestartAnim(draw->stampMenu);
        ClActor_SetAnimTime(draw->stampMenu, 0);
        ClActor_SetAutoAnim(draw->stampMenu, FALSE);
        ClActor_SetVisible(draw->selectFrame, FALSE);
    }
}

static inline void CtvtDraw_PrintMenuItem(PrintQueue *queue, CtvtDraw *draw, MsgData *msgData, u32 msgId, int x, int y,
                                          Font *font) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgData, msgId);

    PrintQueue_PrintColor(queue, BmpWin_GetBitmap(draw->menuWindow), x, y, str, font, 0x440);
    GFL_StrBufFree(str);
}

static void CtvtDraw_PrintMenu(CommTvtWork *sys, CtvtDraw *draw, BOOL expanded) {
    Font *font = CommTvt_GetFont(sys);
    MsgData *msgData = CommTvt_GetMsgData(sys);
    PrintQueue *queue = CommTvt_GetPrintQueue(sys);
    BmpWin *oldWindow = draw->menuWindow;

    if (oldWindow != NULL) {
        PrintQueue_Clear(queue);
        BmpWin_ClearFrame(draw->menuWindow, 2);
        BmpWin_ClearScreen(draw->menuWindow);
    }
    draw->menuExpanded = expanded;
    if (expanded == TRUE) {
        draw->menuWindow = BmpWin_CreateDynamic(4, 2, 8, 28, 13, 10, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(draw->menuWindow), 15);
        BmpWin_FlushChar(draw->menuWindow);
        CtvtDraw_PrintMenuItem(queue, draw, msgData, 6, 64, 4, font);
        CtvtDraw_PrintMenuItem(queue, draw, msgData, 7, 64, 24, font);
        CtvtDraw_PrintMenuItem(queue, draw, msgData, 8, 64, 44, font);
        CtvtDraw_PrintMenuItem(queue, draw, msgData, 10, 64, 64, font);
        CtvtDraw_PrintMenuItem(queue, draw, msgData, 9, 64, 84, font);
    } else {
        draw->menuWindow = BmpWin_CreateDynamic(4, 2, 16, 28, 6, 10, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(draw->menuWindow), 15);
        BmpWin_FlushChar(draw->menuWindow);
        CtvtDraw_PrintMenuItem(queue, draw, msgData, 0, 8, 0, font);
        CtvtDraw_PrintMenuItem(queue, draw, msgData, 1, 8, 16, font);
        CtvtDraw_PrintMenuItem(queue, draw, msgData, 2, 8, 32, font);
        CtvtDraw_PrintMenuItem(queue, draw, msgData, 3, 80, 0, font);
        CtvtDraw_PrintMenuItem(queue, draw, msgData, 4, 80, 16, font);
        CtvtDraw_PrintMenuItem(queue, draw, msgData, 5, 80, 32, font);
    }
    draw->menuWindowPending = TRUE;
    if (oldWindow != NULL) {
        BmpWin_Free(oldWindow);
    }
}

static BOOL CtvtDraw_CheckDisconnect(CommTvtWork *sys, CtvtDraw *draw) {
    if (CommTvt_GetEndRequested(sys) == TRUE) {
        if (CtvtComm_GetConnectType(sys, CommTvt_GetComm(sys)) == CTVT_CONNECT_EXISTING) {
            draw->state = DRAW_STATE_PARENT_LEFT;
        } else {
            draw->state = DRAW_STATE_DISCONNECTED;
        }
        return TRUE;
    }
    if (CommTvt_GetMemberCount(sys) <= 1) {
        draw->state = DRAW_STATE_ALONE;
        return TRUE;
    }
    return FALSE;
}

static void CtvtDraw_PrintMessage(CommTvtWork *sys, CtvtDraw *draw, u32 msgId) {
    Font *font = CommTvt_GetFont(sys);
    MsgData *msgData = CommTvt_GetMsgData(sys);
    PrintQueue *queue = CommTvt_GetPrintQueue(sys);
    StrBuf *str;
    u8 y;

    if (draw->messageWindow != NULL) {
        BmpWin_Free(draw->messageWindow);
    }
    y = 4;
    if (draw->slideTarget != 0) {
        y = 1;
    }
    draw->messageWindow = BmpWin_CreateDynamic(0, 1, y, 30, 4, 10, TRUE);
    PrintQueue_Clear(queue);
    GFL_BitmapFill(BmpWin_GetBitmap(draw->messageWindow), 15);
    str = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    PrintQueue_PrintColor(queue, BmpWin_GetBitmap(draw->messageWindow), 0, 0, str, font, 0x440);
    GFL_StrBufFree(str);
    BmpWin_FlushChar(draw->messageWindow);
    BmpWin_FlushMap(draw->messageWindow);
    BmpWin_DrawFrame(draw->messageWindow, 1, 0x200, 9);
    draw->messageWindowPending = TRUE;
}

static void CtvtDraw_CloseMenus(CommTvtWork *sys, CtvtDraw *draw) {
    draw->penMenuOpen = FALSE;
    draw->stampMenuOpen = FALSE;
    ClActor_RestartAnim(draw->penMenu);
    ClActor_RestartAnim(draw->stampMenu);
    ClActor_RestartAnim(draw->menuCursor);
    ClActor_SetAnimTime(draw->penMenu, 0);
    ClActor_SetAnimTime(draw->stampMenu, 0);
    ClActor_SetAutoAnim(draw->penMenu, FALSE);
    ClActor_SetAutoAnim(draw->stampMenu, FALSE);
    ClActor_SetVisible(draw->penMenu, FALSE);
    ClActor_SetVisible(draw->stampMenu, FALSE);
    ClActor_SetVisible(draw->menuCursor, FALSE);
    ClActor_SetVisible(draw->selectFrame, FALSE);
}

// A parent that leaves the drawing mode while a minigame is being cancelled waits for the others
static void CtvtDraw_UpdateCancel(CommTvtWork *sys, CtvtDraw *draw) {
    CtvtTalk *talk = CommTvt_GetTalk(sys);

    if (CtvtTalk_IsGameCancelled(talk) == TRUE && draw->syncState == 0) {
        draw->syncState = 1;
    }
    switch (draw->syncState) {
    case 0:
        break;
    case 1:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 10, 32);
        draw->syncState = 2;
        break;
    case 2:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 10, 32) == TRUE) {
            CtvtTalk_SetGameCancelled(talk, FALSE);
            draw->syncState = 0;
        }
        break;
    }
}
