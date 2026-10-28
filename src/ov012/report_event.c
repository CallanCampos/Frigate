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
#include "field/field_msgbg.h"
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
#include "text/system/report.h"

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
    AppTaskMenuRes *menuRes;
    AppTaskMenu *menu;
    WaitIcon *waitIcon;
    FieldSaveAnime *effect;
    u32 timer;
};

static void Report_Exit(EventSaveWork *work);
static void Report_CreateWindow(EventSaveWork *work);
static void Report_FreeWindow(EventSaveWork *work);
static void Report_PrintMessage(EventSaveWork *work, u32 messageId);
static void Report_StartStream(EventSaveWork *work);
static BOOL Report_UpdateStream(EventSaveWork *work);
static BOOL Report_UpdateMessage(EventSaveWork *work);
static void Report_PrintMessageInstant(EventSaveWork *work, u32 messageId);
static BOOL Report_UpdatePrintQueue(EventSaveWork *work);
static void Report_CreateYesNoRes(EventSaveWork *work);
static void Report_FreeYesNoRes(EventSaveWork *work);
static void Report_OpenYesNoMenu(EventSaveWork *work);
static int Report_UpdateYesNoMenu(EventSaveWork *work);
static void Report_StartPlayerSaveSeq(EventSaveWork *work);
static void Report_EndPlayerSaveSeq(EventSaveWork *work);
static BOOL Report_IsPlayerSeqDone(EventSaveWork *work);
static BOOL Report_IsAvatarReady(EventSaveWork *work);
static void Report_StartSaveIcon(EventSaveWork *work);
static void Report_EndSaveIcon(EventSaveWork *work);

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
            FieldSubscreen_Report_IsReady(Field_GetSubscreen(work->field)) == TRUE) {
            work->report = GFL_HeapAllocate(work->heapId, sizeof(ReportWork), FALSE, "report_event.c", 159);
            work->report->msgData =
                GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520, work->heapId);
            work->report->strbuf = GFL_StrBufCreate(0x200, work->heapId);
            work->report->tcbManager = GFL_TCBExMgrCreate(work->heapId, work->heapId, 1, 4);
            work->report->cursor = KeyCursor_Create(0xf, 1, 1, work->heapId);
            work->report->printQueue = PrintQueue_CreateDefault(work->heapId);
            work->report->timer = 0;
            Report_CreateWindow(work);
            Report_CreateYesNoRes(work);
            *state = 2;
        }
        break;
    case 2:
        Report_PrintMessage(work, 2);
        *state = 3;
        break;
    case 3:
        if (Report_UpdateMessage(work) == FALSE) {
            Report_OpenYesNoMenu(work);
            *state = 4;
        }
        break;
    case 4:
        switch (Report_UpdateYesNoMenu(work)) {
        case 0:
            if (SaveControl_IsNewGameOverSave(work->save) == TRUE) {
                Report_PrintMessage(work, 8);
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
        FieldSubscreen_Report_InitSave(Field_GetSubscreen(work->field));
        if (FieldSubscreen_Report_IsLargeSave(Field_GetSubscreen(work->field)) == TRUE) {
            Report_PrintMessageInstant(work, 10);
        } else {
            Report_PrintMessageInstant(work, 4);
        }
        *state = 6;
        break;
    case 6:
        if (Report_UpdatePrintQueue(work) == TRUE) {
            Report_StartPlayerSaveSeq(work);
            *state = 7;
        }
        break;
    case 7:
        if (Report_IsPlayerSeqDone(work) == TRUE) {
            *state = 8;
        }
        break;
    case 8:
        if (Report_IsAvatarReady(work)) {
            work->report->waitIcon =
                WaitIcon_Create(GFL_VBlankGetTCBMgr(), work->report->window.window, 0xf, 0x10, work->heapId);
            FieldSubscreen_Report_StartBar(Field_GetSubscreen(work->field));
            Report_StartSaveIcon(work);
            GameData_StartSave(GSYS_GetGameData(work->gameSystem));
            *state = 9;
        }
        break;
    case 9:
        switch (GameData_UpdateSave(GSYS_GetGameData(work->gameSystem))) {
        case 0:
        case 1:
            break;
        case 2:
            *state = 10;
            break;
        case 3:
            FieldSubscreen_Report_StopBar(Field_GetSubscreen(work->field));
            Report_EndSaveIcon(work);
            Report_EndPlayerSaveSeq(work);
            Report_PrintMessage(work, 7);
            WaitIcon_Free(work->report->waitIcon);
            *state = 13;
            break;
        }
        break;
    case 10:
        if (FieldSubscreen_Report_IsBarFull(Field_GetSubscreen(work->field)) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_SAVE);
            Report_EndSaveIcon(work);
            Report_EndPlayerSaveSeq(work);
            wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
            str = GFL_MsgDataLoadStrbufNew(work->report->msgData, Global10520_Text_SavedGame);
            copyVarForText(wordSet, 0, GetGameDataPlayerInfo(GSYS_GetGameData(work->gameSystem)));
            GFL_WordSetFormatStrbuf(wordSet, work->report->strbuf, str);
            GFL_StrBufFree(str);
            GFL_WordSetSystemFree(wordSet);
            Report_StartStream(work);
            WaitIcon_Free(work->report->waitIcon);
            *state = 11;
        }
        break;
    case 11:
        if (Report_IsPlayerSeqDone(work) == TRUE && Report_IsAvatarReady(work) && Report_UpdateMessage(work) == FALSE) {
            *state = 12;
        }
        break;
    case 12:
        if (!GFL_SndIsPlaying(SEQ_SE_SAVE)) {
            *state = 16;
        }
        break;
    case 13:
        if (Report_IsPlayerSeqDone(work) != TRUE || !Report_IsAvatarReady(work)) {
            break;
        }
        *state = 14;
        // fallthrough
    case 14:
        if (Report_UpdateMessage(work) == FALSE) {
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
        Report_Exit(work);
        FieldSubscreen_ReqChange(Field_GetSubscreen(work->field), 1);
        *state = 18;
        break;
    case 18:
        return 1;
    case 19:
        Report_Exit(work);
        if (work->screenId == 10 || work->screenId == 4) {
            FieldSubscreen *subscreen = Field_GetSubscreen(work->field);
            FieldSubscreen_ReqChange(subscreen, work->screenId);
        } else {
            FieldSubscreen_ReqChange(Field_GetSubscreen(work->field), 0);
        }
        *state = 20;
        break;
    case 20:
        FieldSubscreen_ResumeField(Field_GetSubscreen(work->field), work->heapId);
        return 0;
    }
    return 2;
}

static void Report_Exit(EventSaveWork *work) {
    Report_FreeYesNoRes(work);
    Report_FreeWindow(work);
    PrintQueue_Free(work->report->printQueue);
    KeyCursor_Free(work->report->cursor);
    GFL_TCBExMgrFree(work->report->tcbManager);
    GFL_StrBufFree(work->report->strbuf);
    GFL_MsgDataFree(work->report->msgData);
    GFL_HeapFree(work->report);
}

static void Report_CreateWindow(EventSaveWork *work) {
    work->report->window.window = BmpWin_CreateDynamic(6, 1, 1, 30, 4, 12, 1);
    LoadSysMsgBox(6, 1, 13, 0, work->heapId);
    GFL_BGSysLoadNCLRDefault(0x17, 5, 4, 0x180, 0x20, work->heapId);
}

static void Report_FreeWindow(EventSaveWork *work) {
    BmpWin_Free(work->report->window.window);
}

static void Report_PrintMessage(EventSaveWork *work, u32 messageId) {
    GFL_MsgDataLoadStrbuf(work->report->msgData, messageId, work->report->strbuf);
    Report_StartStream(work);
}

static void Report_StartStream(EventSaveWork *work) {
    ReportWork *report;
    BmpWin *window;

    GFL_BitmapFill(BmpWin_GetBitmap(work->report->window.window), 15);
    BmpWin_DrawFrame(work->report->window.window, 2, 1, 13);
    report = work->report;
    work->report->stream =
        PrintStream_Create(report->window.window, 0, 0, report->strbuf, MsgBGPrinter_GetFont(work->msgBgSys),
                           TextSpeed_GetCurrentWait(), report->tcbManager, 10, work->heapId, 15);
    work->report->continued = FALSE;
    window = work->report->window.window;
    BmpWin_Transfer(window);
}

static BOOL Report_UpdateStream(EventSaveWork *work) {
    switch (PrintStream_GetState(work->report->stream)) {
    case PRINT_STREAM_RUNNING:
        if (GFL_HIDIsTouchPressed() == TRUE || (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            PrintStream_SkipWait(work->report->stream, 0);
        }
        work->report->continued = FALSE;
        break;
    case PRINT_STREAM_PAUSED:
        if (work->report->continued == FALSE &&
            (GFL_HIDIsTouchPressed() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)))) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            PrintStream_Continue(work->report->stream);
            work->report->continued = TRUE;
        }
        break;
    case PRINT_STREAM_DONE:
        PrintStream_Delete(work->report->stream);
        work->report->continued = FALSE;
        return FALSE;
    }
    return TRUE;
}

// Prints at twice the speed
static BOOL Report_UpdateMessage(EventSaveWork *work) {
    GFL_TCBExMgrUpdate(work->report->tcbManager);
    KeyCursor_Update(work->report->cursor, work->report->stream, work->report->window.window);
    if (Report_UpdateStream(work) == FALSE) {
        return FALSE;
    }
    GFL_TCBExMgrUpdate(work->report->tcbManager);
    KeyCursor_Update(work->report->cursor, work->report->stream, work->report->window.window);
    return Report_UpdateStream(work);
}

static void Report_PrintMessageInstant(EventSaveWork *work, u32 messageId) {
    StrBuf *str;
    ReportWindow *reportWindow;
    PrintQueue *queue;
    Font *font;
    BmpWin *window;

    GFL_BitmapFill(BmpWin_GetBitmap(work->report->window.window), 15);
    BmpWin_DrawFrame(work->report->window.window, 2, 1, 13);
    str = GFL_MsgDataLoadStrbufNew(work->report->msgData, messageId);
    font = MsgBGPrinter_GetFont(work->msgBgSys);
    reportWindow = &work->report->window;
    queue = work->report->printQueue;
    PrintQueue_Print(queue, BmpWin_GetBitmap(reportWindow->window), 0, 0, str, font);
    reportWindow->printing = TRUE;
    GFL_StrBufFree(str);
    window = work->report->window.window;
    BmpWin_Transfer(window);
}

static BOOL Report_UpdatePrintQueue(EventSaveWork *work) {
    ReportWindow *reportWindow;
    PrintQueue *queue;

    PrintQueue_Main(work->report->printQueue);
    queue = work->report->printQueue;
    reportWindow = &work->report->window;
    if (reportWindow->printing && !PrintQueue_IsBitmapPending(queue, BmpWin_GetBitmap(reportWindow->window))) {
        BmpWin_FlushChar(reportWindow->window);
        reportWindow->printing = FALSE;
    }
    return PrintQueue_IsEmpty(work->report->printQueue);
}

static void Report_CreateYesNoRes(EventSaveWork *work) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_REPORT, work->heapId);

    work->report->items[0].str = GFL_MsgDataLoadStrbufNew(msgData, Report_Text_Yes);
    work->report->items[0].color = 0x39e3;
    work->report->items[0].type = 0;
    work->report->items[1].str = GFL_MsgDataLoadStrbufNew(msgData, Report_Text_No);
    work->report->items[1].color = 0x39e3;
    work->report->items[1].type = 0;
    GFL_MsgDataFree(msgData);
    work->report->menuRes = AppTaskMenuRes_Create(6, 14, MsgBGPrinter_GetFont(work->msgBgSys),
                                                  MsgBGPrinter_GetPrintQue(work->msgBgSys), work->heapId);
}

static void Report_FreeYesNoRes(EventSaveWork *work) {
    AppTaskMenuRes_Free(work->report->menuRes);
    GFL_StrBufFree(work->report->items[1].str);
    GFL_StrBufFree(work->report->items[0].str);
}

static void Report_OpenYesNoMenu(EventSaveWork *work) {
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
static int Report_UpdateYesNoMenu(EventSaveWork *work) {
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

static void Report_StartPlayerSaveSeq(EventSaveWork *work) {
    FieldPlayer *player = Field_GetPlayer(work->field);
    FieldActor *actor;

    if (FieldPlayer_CheckNotWaterForm(player) == TRUE) {
        actor = FieldPlayer_GetActor(player);
        FieldPlayer_SetSpecialSeq(player, 0x20);
        EnableActorMovement(actor);
        ClearActorFlag(actor, 0x10);
        FieldPlayer_StepFormChange(player);
    }
}

static void Report_EndPlayerSaveSeq(EventSaveWork *work) {
    FieldPlayer *player = Field_GetPlayer(work->field);

    if (FieldPlayer_CheckNotWaterForm(player) == TRUE) {
        FieldPlayer_SetSpecialSeq(player, 8);
        FieldPlayer_StepFormChange(player);
    }
}

static BOOL Report_IsPlayerSeqDone(EventSaveWork *work) {
    FieldPlayer *player = Field_GetPlayer(work->field);

    if (FieldPlayer_CheckNotWaterForm(player) == FALSE) {
        return TRUE;
    }
    if (FieldPlayer_StepFormChange(player) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL Report_IsAvatarReady(EventSaveWork *work) {
    FieldPlayer *player = Field_GetPlayer(work->field);

    if (FieldPlayer_CheckNotWaterForm(player) == TRUE) {
        return FieldPlayer_CheckAnimePaused(player);
    }
    return TRUE;
}

static void Report_StartSaveIcon(EventSaveWork *work) {
    work->report->effect = FieldSaveanime_Create(work->heapId, work->field);
    FieldSaveanime_Start(work->report->effect);
}

static void Report_EndSaveIcon(EventSaveWork *work) {
    FieldSaveanime_Stop(work->report->effect);
    FieldSaveanime_Delete(work->report->effect);
}
