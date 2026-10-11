// The report: the screen that asks whether to save the game, saves it alongside the field and says so. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "constants/text_banks.h"
#include "field/event_save.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_player.h"
#include "field/subscreen.h"
#include "gfl/arc_util.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wordset.h"
#include "text/script/global_10520.h"
#include "text/system/0359.h"

#define SEQ_SE_SAVE 0x558
#define SEQ_SE_MESSAGE 0x547

typedef struct {
    BmpWin *window;
    // Set while the print queue has yet to draw into the window
    u8 printing;
} ReportWindow;

struct ReportWork {
    TCBExManager *tcbManager;
    MsgData *msgData;
    ReportWindow window;
    PrintStream *stream;
    PrintQueue *printQueue;
    StrBuf *strbuf;
    KeyCursor *cursor;
    // Set once the player has continued a paused message
    u32 continued;
    AppTaskMenuItem items[2];
    void *menuRes;
    void *menu;
    WaitIcon *waitIcon;
    void *effect;
    u32 timer;
};

static void func_ov012_02163ea0(EventSaveWork *work);
static void func_ov012_02163ee0(EventSaveWork *work);
static void func_ov012_02163f2c(EventSaveWork *work);
static void func_ov012_02163f38(EventSaveWork *work, u32 messageId);
static void func_ov012_02163f50(EventSaveWork *work);
static BOOL func_ov012_02163fc8(EventSaveWork *work);
static BOOL func_ov012_0216404c(EventSaveWork *work);
static void func_ov012_02164090(EventSaveWork *work, u32 messageId);
static BOOL func_ov012_02164110(EventSaveWork *work);
static void func_ov012_02164150(EventSaveWork *work);
static void func_ov012_021641c0(EventSaveWork *work);
static void func_ov012_021641e0(EventSaveWork *work);
static int func_ov012_0216421c(EventSaveWork *work);
static void func_ov012_02164258(EventSaveWork *work);
static void func_ov012_02164290(EventSaveWork *work);
static BOOL func_ov012_021642b4(EventSaveWork *work);
static BOOL func_ov012_021642dc(EventSaveWork *work);
static void func_ov012_021642fc(EventSaveWork *work);
static void func_ov012_02164318(EventSaveWork *work);

u32 EventSave_Update(EventSaveWork *work, u32 *state) {
    WordSet *wordSet;
    StrBuf *str;

    switch (*state) {
    case 0:
        FieldSubscreen_ReqChange(Field_GetSubscreen(work->field), 7);
        *state = 1;
        break;
    case 1:
        if (FieldSubscreen_IsReady(Field_GetSubscreen(work->field)) == TRUE &&
            func_ov036_02198b04(Field_GetSubscreen(work->field)) == TRUE) {
            work->report = GFL_HeapAllocate(work->heapId, sizeof(ReportWork), FALSE, "report_event.c", 159);
            work->report->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520, work->heapId);
            work->report->strbuf = GFL_StrBufCreate(0x200, work->heapId);
            work->report->tcbManager = GFL_TCBExMgrCreate(work->heapId, work->heapId, 1, 4);
            work->report->cursor = KeyCursor_Create(0xf, 1, 1, work->heapId);
            work->report->printQueue = func_02021998(work->heapId);
            work->report->timer = 0;
            func_ov012_02163ee0(work);
            func_ov012_02164150(work);
            *state = 2;
        }
        break;
    case 2:
        func_ov012_02163f38(work, 2);
        *state = 3;
        break;
    case 3:
        if (func_ov012_0216404c(work) == FALSE) {
            func_ov012_021641e0(work);
            *state = 4;
        }
        break;
    case 4:
        switch (func_ov012_0216421c(work)) {
        case 0:
            if (func_0200746c(work->save) == TRUE) {
                func_ov012_02163f38(work, 8);
                *state = 14;
            } else {
                BmpWin_ClearFrame(work->report->window.window, WINFRAME_TRANSFER_VBLANK);
                *state = 5;
            }
            break;
        case 1:
            *state = 17;
            break;
        }
        break;
    case 5:
        RecordAddOne(GameData_GetRecords(GSYS_GetGameData(work->gameSystem)), 1);
        func_ov036_02198b10(Field_GetSubscreen(work->field));
        if (func_ov036_02198b40(Field_GetSubscreen(work->field)) == TRUE) {
            func_ov012_02164090(work, 10);
        } else {
            func_ov012_02164090(work, 4);
        }
        *state = 6;
        break;
    case 6:
        if (func_ov012_02164110(work) == TRUE) {
            func_ov012_02164258(work);
            *state = 7;
        }
        break;
    case 7:
        if (func_ov012_021642b4(work) == TRUE) {
            *state = 8;
        }
        break;
    case 8:
        if (func_ov012_021642dc(work)) {
            work->report->waitIcon =
                WaitIcon_Create(GFL_VBlankGetTCBMgr(), work->report->window.window, 0xf, 0x10, work->heapId);
            func_ov036_02198b1c(Field_GetSubscreen(work->field));
            func_ov012_021642fc(work);
            func_0201782c(GSYS_GetGameData(work->gameSystem));
            *state = 9;
        }
        break;
    case 9:
        switch (func_02017850(GSYS_GetGameData(work->gameSystem))) {
        case 0:
        case 1:
            break;
        case 2:
            *state = 10;
            break;
        case 3:
            func_ov036_02198b34(Field_GetSubscreen(work->field));
            func_ov012_02164318(work);
            func_ov012_02164290(work);
            func_ov012_02163f38(work, 7);
            WaitIcon_Free(work->report->waitIcon);
            *state = 13;
            break;
        }
        break;
    case 10:
        if (func_ov036_02198b28(Field_GetSubscreen(work->field)) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_SAVE);
            func_ov012_02164318(work);
            func_ov012_02164290(work);
            wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
            str = GFL_MsgDataLoadStrbufNew(work->report->msgData, Global10520_Text_SavedGame);
            copyVarForText(wordSet, 0, GetGameDataPlayerInfo(GSYS_GetGameData(work->gameSystem)));
            GFL_WordSetFormatStrbuf(wordSet, work->report->strbuf, str);
            GFL_StrBufFree(str);
            GFL_WordSetSystemFree(wordSet);
            func_ov012_02163f50(work);
            WaitIcon_Free(work->report->waitIcon);
            *state = 11;
        }
        break;
    case 11:
        if (func_ov012_021642b4(work) == TRUE && func_ov012_021642dc(work) && func_ov012_0216404c(work) == FALSE) {
            *state = 12;
        }
        break;
    case 12:
        if (!GFL_SndIsPlaying(SEQ_SE_SAVE)) {
            *state = 16;
        }
        break;
    case 13:
        if (func_ov012_021642b4(work) != TRUE || !func_ov012_021642dc(work)) {
            break;
        }
        *state = 14;
        // fallthrough
    case 14:
        if (func_ov012_0216404c(work) == FALSE) {
            *state = 15;
        }
        break;
    case 15:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            *state = 19;
        }
        break;
    case 16:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            *state = 19;
        }
        if (work->report->timer >= 150) {
            *state = 19;
        } else {
            work->report->timer++;
        }
        break;
    case 17:
        func_ov012_02163ea0(work);
        FieldSubscreen_ReqChange(Field_GetSubscreen(work->field), 1);
        *state = 18;
        break;
    case 18:
        return 1;
    case 19:
        func_ov012_02163ea0(work);
        if (work->screenId == 10 || work->screenId == 4) {
            FieldSubscreen *subscreen = Field_GetSubscreen(work->field);
            FieldSubscreen_ReqChange(subscreen, work->screenId);
        } else {
            FieldSubscreen_ReqChange(Field_GetSubscreen(work->field), 0);
        }
        *state = 20;
        break;
    case 20:
        func_ov036_021984f0(Field_GetSubscreen(work->field), work->heapId);
        return 0;
    }
    return 2;
}

static void func_ov012_02163ea0(EventSaveWork *work) {
    func_ov012_021641c0(work);
    func_ov012_02163f2c(work);
    func_02021a18(work->report->printQueue);
    KeyCursor_Free(work->report->cursor);
    GFL_TCBExMgrFree(work->report->tcbManager);
    GFL_StrBufFree(work->report->strbuf);
    GFL_MsgDataFree(work->report->msgData);
    GFL_HeapFree(work->report);
}

static void func_ov012_02163ee0(EventSaveWork *work) {
    work->report->window.window = BmpWin_CreateDynamic(6, 1, 1, 30, 4, 12, 1);
    LoadSysMsgBox(6, 1, 13, 0, work->heapId);
    GFL_BGSysLoadNCLRDefault(0x17, 5, 4, 0x180, 0x20, work->heapId);
}

static void func_ov012_02163f2c(EventSaveWork *work) {
    BmpWin_Free(work->report->window.window);
}

static void func_ov012_02163f38(EventSaveWork *work, u32 messageId) {
    GFL_MsgDataLoadStrbuf(work->report->msgData, messageId, work->report->strbuf);
    func_ov012_02163f50(work);
}

static void func_ov012_02163f50(EventSaveWork *work) {
    ReportWork *report;
    BmpWin *window;

    GFL_BitmapFill(BmpWin_GetBitmap(work->report->window.window), 15);
    BmpWin_DrawFrame(work->report->window.window, 2, 1, 13);
    report = work->report;
    work->report->stream = func_02022268(report->window.window, 0, 0, report->strbuf, func_ov036_0218799c(work->msgBgSys),
                                         func_02017bcc(), report->tcbManager, 10, work->heapId, 15);
    work->report->continued = FALSE;
    window = work->report->window.window;
    BmpWin_Transfer(window);
}

static BOOL func_ov012_02163fc8(EventSaveWork *work) {
    switch (func_020223b4(work->report->stream)) {
    case PRINT_STREAM_RUNNING:
        if (func_0203da48() == TRUE || (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            func_020223e0(work->report->stream, 0);
        }
        work->report->continued = FALSE;
        break;
    case PRINT_STREAM_PAUSED:
        if (work->report->continued == FALSE &&
            (func_0203da48() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)))) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            func_020223bc(work->report->stream);
            work->report->continued = TRUE;
        }
        break;
    case PRINT_STREAM_DONE:
        func_020223cc(work->report->stream);
        work->report->continued = FALSE;
        return FALSE;
    }
    return TRUE;
}

// Prints at twice the speed
static BOOL func_ov012_0216404c(EventSaveWork *work) {
    GFL_TCBExMgrUpdate(work->report->tcbManager);
    KeyCursor_Update(work->report->cursor, work->report->stream, work->report->window.window);
    if (func_ov012_02163fc8(work) == FALSE) {
        return FALSE;
    }
    GFL_TCBExMgrUpdate(work->report->tcbManager);
    KeyCursor_Update(work->report->cursor, work->report->stream, work->report->window.window);
    return func_ov012_02163fc8(work);
}

static void func_ov012_02164090(EventSaveWork *work, u32 messageId) {
    StrBuf *str;
    ReportWindow *reportWindow;
    PrintQueue *queue;
    Font *font;
    BmpWin *window;

    GFL_BitmapFill(BmpWin_GetBitmap(work->report->window.window), 15);
    BmpWin_DrawFrame(work->report->window.window, 2, 1, 13);
    str = GFL_MsgDataLoadStrbufNew(work->report->msgData, messageId);
    font = func_ov036_0218799c(work->msgBgSys);
    reportWindow = &work->report->window;
    queue = work->report->printQueue;
    func_02021c54(queue, BmpWin_GetBitmap(reportWindow->window), 0, 0, str, font);
    reportWindow->printing = TRUE;
    GFL_StrBufFree(str);
    window = work->report->window.window;
    BmpWin_Transfer(window);
}

static BOOL func_ov012_02164110(EventSaveWork *work) {
    ReportWindow *reportWindow;
    PrintQueue *queue;

    func_02021a3c(work->report->printQueue);
    queue = work->report->printQueue;
    reportWindow = &work->report->window;
    if (reportWindow->printing && !func_02021c1c(queue, BmpWin_GetBitmap(reportWindow->window))) {
        BmpWin_FlushChar(reportWindow->window);
        reportWindow->printing = FALSE;
    }
    return func_02021c0c(work->report->printQueue);
}

static void func_ov012_02164150(EventSaveWork *work) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0359, work->heapId);

    work->report->items[0].str = GFL_MsgDataLoadStrbufNew(msgData, Bank0359_Text_Yes);
    work->report->items[0].color = 0x39e3;
    work->report->items[0].type = 0;
    work->report->items[1].str = GFL_MsgDataLoadStrbufNew(msgData, Bank0359_Text_No);
    work->report->items[1].color = 0x39e3;
    work->report->items[1].type = 0;
    GFL_MsgDataFree(msgData);
    work->report->menuRes = AppTaskMenuRes_Create(6, 14, func_ov036_0218799c(work->msgBgSys),
                                          func_ov036_02187998(work->msgBgSys), work->heapId);
}

static void func_ov012_021641c0(EventSaveWork *work) {
    AppTaskMenuRes_Free(work->report->menuRes);
    GFL_StrBufFree(work->report->items[1].str);
    GFL_StrBufFree(work->report->items[0].str);
}

static void func_ov012_021641e0(EventSaveWork *work) {
    AppTaskMenuInit init;

    init.heapId = work->heapId;
    init.itemCount = 2;
    init.items = work->report->items;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 12;
    init.width = 8;
    init.height = 3;
    work->report->menu = AppTaskMenu_CreateFastFlash(&init, work->report->menuRes);
}

// 0 for yes, 1 for no, and -1 until one is picked
static int func_ov012_0216421c(EventSaveWork *work) {
    u32 pos;

    AppTaskMenu_Update(work->report->menu);
    if (AppTaskMenu_IsFlashFinished(work->report->menu) == TRUE) {
        pos = AppTaskMenu_GetCursorPos(work->report->menu);
        AppTaskMenu_Free(work->report->menu);
        if (pos == 0) {
            return 0;
        }
        return 1;
    }
    return -1;
}

static void func_ov012_02164258(EventSaveWork *work) {
    FieldPlayer *player = Field_GetPlayer(work->field);
    FieldActor *actor;

    if (func_ov036_0219a834(player) == TRUE) {
        actor = FieldPlayer_GetActor(player);
        FieldPlayer_SetSpecialSeq(player, 0x20);
        EnableActorMovement(actor);
        ClearActorFlag(actor, 0x10);
        func_ov036_0219a580(player);
    }
}

static void func_ov012_02164290(EventSaveWork *work) {
    FieldPlayer *player = Field_GetPlayer(work->field);

    if (func_ov036_0219a834(player) == TRUE) {
        FieldPlayer_SetSpecialSeq(player, 8);
        func_ov036_0219a580(player);
    }
}

static BOOL func_ov012_021642b4(EventSaveWork *work) {
    FieldPlayer *player = Field_GetPlayer(work->field);

    if (func_ov036_0219a834(player) == FALSE) {
        return TRUE;
    }
    if (func_ov036_0219a580(player) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov012_021642dc(EventSaveWork *work) {
    FieldPlayer *player = Field_GetPlayer(work->field);

    if (func_ov036_0219a834(player) == TRUE) {
        return func_ov036_0219a870(player);
    }
    return TRUE;
}

static void func_ov012_021642fc(EventSaveWork *work) {
    work->report->effect = func_ov036_021c6cc8(work->heapId, work->field);
    func_ov036_021c6d14(work->report->effect);
}

static void func_ov012_02164318(EventSaveWork *work) {
    func_ov036_021c6d3c(work->report->effect);
    func_ov036_021c6cf8(work->report->effect);
}
