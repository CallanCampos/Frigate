#include "types.h"
#include "app/musical/mus_shot_info.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/musical.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "save/save_control.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wipe.h"
#include "text/system/btl_pokeparam_2.h"

// Overlay 209's mus_shot_info.c: the touch screen under a musical's photo. Right after a musical it asks whether to
// keep the photo, replacing the one in the save; otherwise it waits for the return button

enum {
    MUS_SHOT_INFO_STATE_START,
    MUS_SHOT_INFO_STATE_WAIT_WIPE,
    MUS_SHOT_INFO_STATE_WAIT_MESSAGE,
    MUS_SHOT_INFO_STATE_MENU,
    // Waiting for the other players before leaving
    MUS_SHOT_INFO_STATE_SYNC,
    MUS_SHOT_INFO_STATE_WAIT_RETURN,
    MUS_SHOT_INFO_STATE_RETURN,
    MUS_SHOT_INFO_STATE_END,
};

// The message of the musical's system messages that the communication waits with
#define MUS_SHOT_INFO_SYNC_ID 71

struct MusShotInfo {
    HeapID heapId;
    u32 state;
    // Frames until the background scrolls again
    u8 scrollTimer;
    // Whether it asks to keep the photo, rather than showing the return button
    BOOL askSave;
    MusicalShot *shot;
    MusicalSave *save;
    TCBExManager *tcbManager;
    BmpWin *msgWin;
    Font *font;
    PrintStream *stream;
    MsgData *msgData;
    StrBuf *msgStr;
    // Whether a message is in the print queue, to show the wait icon once it is printed
    BOOL printing;
    WaitIcon *waitIcon;
    PrintQueue *printQueue;
    AppTaskMenu *menu;
    AppTaskMenuRes *menuRes;
    // Overlay 211's communication work, or NULL
    void *comm;
    ClActUnit *clactUnit;
    u32 returnPalette;
    u32 returnChars;
    u32 returnCellAnims;
    ClActor *returnButton;
};

static void MusShotInfo_InitGraphics(MusShotInfo *info);
static void MusShotInfo_InitBG(const BGSetup *setup, u8 bg, u8 mode);
static void MusShotInfo_InitMessage(MusShotInfo *info);
static void MusShotInfo_ExitMessage(MusShotInfo *info);
static void MusShotInfo_PrintMessage(MusShotInfo *info, u32 msgId);
static void MusShotInfo_PrintMessageNow(MusShotInfo *info, u32 msgId);
static void MusShotInfo_CreateMenu(MusShotInfo *info);

static const TouchRect MUS_SHOT_INFO_RETURN_RECT[] = {
    { 168, 192, 232, 255 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const BGSetup MUS_SHOT_INFO_BG4_SETUP = {
    0,
    0,
    0x1000,
    0,
    BGRES_512x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x6000),
    GX_BG_CHARBASE(0x04000),
    0x2000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup MUS_SHOT_INFO_BG6_SETUP = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x7000),
    GX_BG_CHARBASE(0x00000),
    0x4000,
    GX_BG_EXTPLTT_01,
    2,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup MUS_SHOT_INFO_BG7_SETUP = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x7800),
    GX_BG_CHARBASE(0x00000),
    0,
    GX_BG_EXTPLTT_01,
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

MusShotInfo *MusShotInfo_Create(MusicalShot *shot, MusicalSave *save, BOOL askSave, void *comm, HeapID heapId) {
    MusShotInfo *info = GFL_HeapAllocate(heapId, sizeof(MusShotInfo), TRUE, "mus_shot_info.c", 141);

    info->heapId = heapId;
    info->askSave = askSave;
    info->shot = shot;
    info->save = save;
    info->comm = comm;
    info->msgStr = NULL;
    info->scrollTimer = 0;
    if (askSave == TRUE) {
        info->state = MUS_SHOT_INFO_STATE_START;
    } else {
        info->state = MUS_SHOT_INFO_STATE_WAIT_WIPE;
    }
    MusShotInfo_InitGraphics(info);
    MusShotInfo_InitMessage(info);
    info->menuRes = AppTaskMenuRes_Create(4, 12, info->font, info->printQueue, info->heapId);
    func_02042ba8(FALSE, heapId);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    return info;
}

void MusShotInfo_Delete(MusShotInfo *info) {
    AppTaskMenuRes_Free(info->menuRes);
    if (info->askSave == FALSE) {
        func_0204bcd0(info->returnPalette);
        func_0204b98c(info->returnChars);
        func_0204be64(info->returnCellAnims);
        func_0204c108(info->returnButton);
    }
    func_0204bf98(info->clactUnit);
    MusShotInfo_ExitMessage(info);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(7);
    GFL_HeapFree(info);
}

void MusShotInfo_Main(MusShotInfo *info) {
    u8 pos;

    switch (info->state) {
    case MUS_SHOT_INFO_STATE_START:
        info->state = MUS_SHOT_INFO_STATE_WAIT_WIPE;
        GFL_SndSEPlay(SEQ_SE_MSCL_14);
        // fallthrough
    case MUS_SHOT_INFO_STATE_WAIT_WIPE:
        if (GFL_WipeIsFinished() == TRUE) {
            if (info->askSave == TRUE) {
                MusShotInfo_PrintMessage(info, 3);
                info->state = MUS_SHOT_INFO_STATE_WAIT_MESSAGE;
            } else {
                info->state = MUS_SHOT_INFO_STATE_WAIT_RETURN;
            }
        }
        break;
    case MUS_SHOT_INFO_STATE_WAIT_MESSAGE:
        if (info->stream == NULL) {
            MusShotInfo_CreateMenu(info);
            info->state = MUS_SHOT_INFO_STATE_MENU;
        }
        break;
    case MUS_SHOT_INFO_STATE_MENU:
        AppTaskMenu_Update(info->menu);
        if (AppTaskMenu_IsFlashFinished(info->menu) == TRUE) {
            pos = AppTaskMenu_GetCursorPos(info->menu);
            AppTaskMenu_Free(info->menu);
            if (pos == 0) {
                sys_memcpy(info->shot, func_0200ad5c(info->save), sizeof(MusicalShot));
            }
            if (info->comm != NULL) {
                info->state = MUS_SHOT_INFO_STATE_SYNC;
                func_ov211_021ef988(info->comm, MUS_SHOT_INFO_SYNC_ID);
                MusShotInfo_PrintMessageNow(info, 6);
            } else {
                info->state = MUS_SHOT_INFO_STATE_END;
            }
        } else if (GFL_NetErrCheck()) {
            AppTaskMenu_Free(info->menu);
            info->state = MUS_SHOT_INFO_STATE_END;
        }
        break;
    case MUS_SHOT_INFO_STATE_SYNC:
        if (func_ov211_021ef99c(info->comm, MUS_SHOT_INFO_SYNC_ID) == TRUE) {
            info->state = MUS_SHOT_INFO_STATE_END;
        } else if (GFL_NetErrCheck()) {
            info->state = MUS_SHOT_INFO_STATE_END;
        }
        break;
    case MUS_SHOT_INFO_STATE_WAIT_RETURN:
        if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) || func_0203da0c(MUS_SHOT_INFO_RETURN_RECT) == 0) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            info->state = MUS_SHOT_INFO_STATE_RETURN;
            func_0204c488(info->returnButton, 9);
            func_0204c520(info->returnButton, TRUE);
        }
        break;
    case MUS_SHOT_INFO_STATE_RETURN:
        if (func_0204c560(info->returnButton) == FALSE) {
            info->state = MUS_SHOT_INFO_STATE_END;
        }
        break;
    case MUS_SHOT_INFO_STATE_END:
        break;
    }

    if (info->stream != NULL && func_020223b4(info->stream) == PRINT_STREAM_DONE) {
        func_020223cc(info->stream);
        info->stream = NULL;
    }
    func_02021a3c(info->printQueue);
    if (info->printing == TRUE && func_02021c0c(info->printQueue) == TRUE) {
        info->printing = FALSE;
        info->waitIcon = WaitIcon_CreateTCBEx(info->tcbManager, info->msgWin, 15, 16, info->heapId);
        BmpWin_FlushChar(info->msgWin);
    }
    GFL_TCBExMgrUpdate(info->tcbManager);
    info->scrollTimer++;
    if (info->scrollTimer > 1) {
        info->scrollTimer = 0;
        GFL_BGSysMoveBGReq(7, BG_MOVE_RIGHT, 1);
        GFL_BGSysMoveBGReq(7, BG_MOVE_UP, 1);
    }
}

BOOL MusShotInfo_IsFinished(MusShotInfo *info) {
    if (info->state == MUS_SHOT_INFO_STATE_END) {
        return TRUE;
    }
    return FALSE;
}

static void MusShotInfo_InitGraphics(MusShotInfo *info) {
    ArcTool *arc;

    MusShotInfo_InitBG(&MUS_SHOT_INFO_BG4_SETUP, 4, 0);
    MusShotInfo_InitBG(&MUS_SHOT_INFO_BG6_SETUP, 6, 0);
    MusShotInfo_InitBG(&MUS_SHOT_INFO_BG7_SETUP, 7, 0);
    info->clactUnit = func_0204bf1c(8, 0, info->heapId);
    func_0204c028(info->clactUnit);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);

    arc = GFL_ArcSysCreateFileHandle(ARCID_MUSICAL_SHOT, info->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 4, 4, 0, 0, info->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 7, 0, 0, FALSE, info->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 12, 7, 0, 0, FALSE, info->heapId);
    GFL_ArcToolFree(arc);

    if (info->askSave == FALSE) {
        ClActorSetup setup;

        arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), info->heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), 4, 0x120, 0x20, info->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, func_0202d824(), 6, 0, 0, FALSE, info->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, func_0202d828(), 6, 0, 0, FALSE, info->heapId);
        GFL_BGSysSetScrPaletteNo(6, 0, 21, 32, 3, 9);
        GFL_BGSysLoadScr(6);
        info->returnPalette = func_0204bbb8(arc, func_0202d810(), CLACT_VRAM_SUB, 0, 0, 3, info->heapId);
        info->returnChars = func_0204b81c(arc, func_0202d814(), FALSE, CLACT_VRAM_SUB, info->heapId);
        info->returnCellAnims = func_0204bde0(arc, func_0202d818(0), func_0202d81c(0), info->heapId);
        GFL_ArcToolFree(arc);

        setup.x = 232;
        setup.y = 168;
        setup.sequence = 1;
        setup.bgPriority = 0;
        setup.priority = 0;
        info->returnButton = func_0204c040(info->clactUnit, info->returnChars, info->returnPalette,
                                           info->returnCellAnims, &setup, CLACT_SURFACE_SUB, info->heapId);
    }
}

static void MusShotInfo_InitBG(const BGSetup *setup, u8 bg, u8 mode) {
    GFL_BGSysCreateBG(bg, setup, mode);
    GFL_BGSysSetBGEnabled(bg, TRUE);
    GFL_BGSysClearBG(bg);
    GFL_BGSysLoadScr(bg);
}

static void MusShotInfo_InitMessage(MusShotInfo *info) {
    info->msgWin = BmpWin_CreateDynamic(4, 1, 1, 30, 4, 10, TRUE);
    BmpWin_FlushChar(info->msgWin);
    BmpWin_FlushMap(info->msgWin);
    GFL_BGSysLoadScr(4);
    info->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, info->heapId);
    info->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_POKEPARAM_2, info->heapId);
    LoadSysMsgBox(4, 1, 11, 0, info->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x140, 0x20, info->heapId);
    func_020232d8();
    info->tcbManager = GFL_TCBExMgrCreate(info->heapId, info->heapId, 3, 0x100);
    info->stream = NULL;
    info->msgStr = NULL;
    info->printing = FALSE;
    info->waitIcon = NULL;
    info->printQueue = func_02021998(info->heapId);
}

static void MusShotInfo_ExitMessage(MusShotInfo *info) {
    func_02021c44(info->printQueue);
    func_02021a18(info->printQueue);
    if (info->waitIcon != NULL) {
        WaitIcon_Free(info->waitIcon);
        info->waitIcon = NULL;
    }
    if (info->stream != NULL) {
        func_020223cc(info->stream);
    }
    if (info->msgStr != NULL) {
        GFL_StrBufFree(info->msgStr);
    }
    GFL_MsgDataFree(info->msgData);
    BmpWin_Free(info->msgWin);
    GFL_FontFree(info->font);
    GFL_TCBExMgrFree(info->tcbManager);
}

static void MusShotInfo_PrintMessage(MusShotInfo *info, u32 msgId) {
    if (info->stream != NULL) {
        func_020223cc(info->stream);
        info->stream = NULL;
    }
    if (info->msgStr != NULL) {
        GFL_StrBufFree(info->msgStr);
        info->msgStr = NULL;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(info->msgWin), 15);
    info->msgStr = GFL_MsgDataLoadStrbufNew(info->msgData, msgId);
    info->stream = func_02022268(info->msgWin, 0, 0, info->msgStr, info->font, func_02017bcc(), info->tcbManager, 2,
                                 info->heapId, 0);
    BmpWin_DrawFrame(info->msgWin, TRUE, 1, 11);
}

static void MusShotInfo_PrintMessageNow(MusShotInfo *info, u32 msgId) {
    if (info->msgStr != NULL) {
        GFL_StrBufFree(info->msgStr);
        info->msgStr = NULL;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(info->msgWin), 15);
    info->msgStr = GFL_MsgDataLoadStrbufNew(info->msgData, msgId);
    func_02021c54(info->printQueue, BmpWin_GetBitmap(info->msgWin), 0, 0, info->msgStr, info->font);
    info->printing = TRUE;
    BmpWin_DrawFrame(info->msgWin, TRUE, 1, 11);
}

static void MusShotInfo_CreateMenu(MusShotInfo *info) {
    AppTaskMenuItem items[2];
    AppTaskMenuInit init;

    items[0].str = GFL_MsgDataLoadStrbufNew(info->msgData, BtlPokeparam2_Text_Yes);
    items[1].str = GFL_MsgDataLoadStrbufNew(info->msgData, BtlPokeparam2_Text_No);
    items[0].color = PRINT_COLOR(14, 15, 0);
    items[1].color = PRINT_COLOR(14, 15, 0);
    items[0].type = 0;
    items[1].type = 0;
    init.heapId = info->heapId;
    init.itemCount = 2;
    init.items = items;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 24;
    init.y = 6;
    init.width = 8;
    init.height = 3;
    info->menu = AppTaskMenu_Create(&init, info->menuRes);
    GFL_StrBufFree(items[0].str);
    GFL_StrBufFree(items[1].str);
}
