#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "field/beacon_view_common.h"
#include "field/fest_mission_data.h"
#include "field/festival.h"
#include "field/field_disp_control.h"
#include "field/fmission_log_accessor_fv.h"
#include "field/fmission_view.h"
#include "field/fmission_view_local.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "save/high_link.h"
#include "system/beacon_status.h"
#include "system/bmp_oam.h"
#include "system/fest_mission.h"
#include "system/fest_mission_beacon.h"
#include "system/nsbtx_to_clwk.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/fmission_view_setup.h"

// The touch screen during a Funfest mission: its sequences, the log's panels and the tasks of its messages. The file's
// name is a guess, after the ROM's "fmission_view_setup.c". The names are ours

enum {
    SEQ_MAIN,
    SEQ_NEW_ENTRY,
    SEQ_WAIT_TASKS,
    SEQ_SCROLL,
    SEQ_QUIT,
    SEQ_5,
    SEQ_6,
    SEQ_7,
    SEQ_8,
};

// The frames between two new entries, and those a message or a mark stays
#define ENTRY_WAIT 90
#define MESSAGE_WAIT 90

#define BUTTON_SIZE 24
// The sequences of a button's states
#define BUTTON_ANIM(button, state) ((u8)(((button) + 1) * FMISSION_VIEW_BUTTON_STATE_COUNT + (state)))
// The arrows' sequences with nothing further to scroll to
#define ANIM_UP_END 17
#define ANIM_DOWN_END 20
// The sequences of a panel's frame, by the kind of its entry, and those of a mark
#define ANIM_FRAME 0
#define ANIM_MARK 20

// Which way the list scrolls: to older entries, or to newer ones. The first is also how a new entry comes in
enum {
    SCROLL_OLDER,
    SCROLL_NEWER,
    // Only takes a free panel
    SCROLL_NONE = 3,
};

// Where a panel's parts are, from its corner
#define PANEL_MARK_X 16
#define PANEL_MARK_Y 0
#define PANEL_ICON_X 16
#define PANEL_ICON_Y 16
#define PANEL_NAME_X 48
#define PANEL_NAME_Y 4
#define PANEL_TEXT_X 48
#define PANEL_TEXT_Y 20
#define PANEL_SCORE_X 128
#define PANEL_SCORE_Y 4

// The first palette of the panels' icons
#define PANEL_ICON_PALETTE 9

// The buttons of the Pass Power's menu
enum {
    PASS_POWER_MENU_LIST,
    PASS_POWER_MENU_USE,
    PASS_POWER_MENU_CANCEL,
};

typedef struct {
    u32 x;
    u32 y;
} FMissionViewTouch;

// Where a button is, and when it can be touched: always, or only while the list can scroll that way
typedef struct {
    u8 x;
    u8 y;
    u8 scroll;
} FMissionViewButton;

typedef struct {
    u8 timer;
    FMissionViewPanel *panel;
    FMissionView *view;
} FMissionViewMarkTask;

typedef struct {
    u8 seq;
    u8 dir;
    u32 unk4;
    FMissionViewPanel *panel;
    FMissionView *view;
    u32 unk10;
    int *pending;
} FMissionViewPanelTask;

typedef struct {
    u8 seq;
    u8 timer;
    int wait;
    FMissionView *view;
    int *pending;
} FMissionViewMessageTask;

typedef struct {
    u8 seq;
    u8 unk1;
    u8 timer;
    u8 unk3;
    u8 unk4;
    u8 button;
    int wait;
    u32 passPower;
    int received;
    FMissionView *view;
    int *pending;
} FMissionViewPassPowerTask;

typedef struct {
    u8 seq;
    u8 unk1;
    u8 timer;
    u8 unk3;
    u8 unk4;
    u8 button;
    int wait;
    u32 unkC[4];
    FMissionView *view;
    int *pending;
} FMissionViewQuitTask;

static int FMissionView_SeqMain(FMissionView *view);
static int FMissionView_SeqNewEntry(FMissionView *view);
static int FMissionView_SeqWaitTasks(FMissionView *view);
static int FMissionView_SeqScroll(FMissionView *view);
static int FMissionView_SeqQuit(FMissionView *view);
static int FMissionView_Seq5(FMissionView *view);
static void FMissionView_SetDimmed(FMissionView *view, BOOL dimmed);
static void FMissionView_DrawAll(FMissionView *view);
static int FMissionView_CheckTouch(FMissionView *view);
static int FMissionView_GetHeldScroll(FMissionView *view);
static BOOL FMissionView_CheckGreeting(FMissionView *view);
static BOOL FMissionView_CheckPassPower(FMissionView *view);
static BOOL FMissionView_CheckNewEntry(FMissionView *view);
static void FMissionView_UpdateMemberCount(FMissionView *view);
static void FMissionView_ScrollOn(FMissionView *view);
static BOOL FMissionView_AddNewEntry(FMissionView *view);
static BOOL FMissionView_Quit(FMissionView *view);
static int FMissionView_GetTouchedButton(FMissionView *view, FMissionViewTouch *touch);
static void FMissionView_LoadMessage(FMissionView *view, u8 messageId);
static void FMissionView_PrintMessage(FMissionView *view, StrBuf *strbuf, int *pending);
static void FMissionView_DrawMission(FMissionView *view);
static void FMissionView_DrawMemberCount(FMissionView *view, int *pending);
static void FMissionView_SetButtonsVisible(FMissionView *view, BOOL visible);
static void FMissionView_UpdateArrows(FMissionView *view);
static void FMissionViewPanel_SetVisible(FMissionViewPanel *panel, BOOL visible);
static void FMissionViewPanel_SetMarkVisible(FMissionViewPanel *panel, BOOL visible);
static void FMissionViewPanel_Clear(FMissionViewPanel *panel);
static void FMissionViewPanel_Set(FMissionViewPanel *panel, u8 logIndex, u8 slot);
static FMissionViewPanel *FMissionView_GetFreePanel(FMissionView *view, int dir);
static void FMissionView_LoadPanelIcon(FMissionView *view, FMissionViewPanel *panel);
static int FMissionView_GetEntryFrame(FMissionView *view, u32 kind, u32 value);
static void FMissionView_TransferPanel(FMissionView *view, FMissionViewPanel *panel);
static void FMissionView_PrintPanel(FMissionView *view, FMissionViewPanel *panel, FestMissionLogEntry *entry);
static void FMissionView_ShowPanel(FMissionView *view, FMissionViewPanel *panel);
static BOOL FMissionView_ShowPanelWhenPrinted(FMissionView *view, FMissionViewPanel *panel);
static void FMissionView_DrawPanel(FMissionView *view, FMissionViewPanel *panel, FestMissionLogEntry *entry);
static void FMissionViewPanel_SetPos(FMissionViewPanel *panel, s16 x, s16 y);
static void FMissionView_StartPanelMark(FMissionView *view, FMissionViewPanel *panel);
static void FMissionView_MarkTask(TCBEx *task, void *data);
static void FMissionView_Scroll(FMissionView *view, int dir);
static void FMissionView_StartScroll(FMissionView *view, int dir, u8 nextSeq);
static void FMissionView_AddPanel(FMissionView *view, FestMissionLogEntry *entry, u8 logIndex, int dir, BOOL mark);
static void FMissionView_PanelTask(TCBEx *task, void *data);
static void FMissionView_ShowMessage(FMissionView *view, u8 messageId);
static void FMissionView_StartMessage(FMissionView *view, StrBuf *strbuf, int *pending);
static void FMissionView_MessageTask(TCBEx *task, void *data);
static BOOL FMissionView_OfferEntryPassPower(FMissionView *view, FestMissionLogEntry *entry);
static BOOL FMissionView_OfferPassPower(FMissionView *view, u32 passPower, StrBuf *name, int received);
static BOOL FMissionView_StartPassPower(FMissionView *view, u32 passPower, int received, int *pending);
static void FMissionView_PassPowerTask(TCBEx *task, void *data);
static void FMissionView_StartQuit(FMissionView *view);
static void FMissionView_StartQuitTask(FMissionView *view, int *pending);
static void FMissionView_QuitTask(TCBEx *task, void *data);

// The player's own icon: the models of the boy and the girl, whose textures are loaded
static const u32 sFMissionViewPlayerModels[2] = { 209, 218 };
// The sounds of a player's words, by their mark
static const u16 sFMissionViewWordsSE[4] = { SEQ_SE_SW_FES_02, SEQ_SE_SW_FES_08, SEQ_SE_SW_FES_09, SEQ_SE_SW_FES_10 };
// The palette of each look's icon
static const u8 sFMissionViewLookPalettes[FMISSION_VIEW_LOOK_COUNT] = {
    2, 3, 6, 5, 4, 5, 2, 0, 1, 3, 6, 5, 4, 7, 7, 0
};

static const FMissionViewButton sFMissionViewButtons[FMISSION_VIEW_BUTTON_COUNT] = {
    { 164, 172, 0 }, { 196, 172, 0 }, { 228, 172, 0 }, { 16, 172, 0 }, { 88, 172, 1 }, { 112, 172, 2 },
};

FMissionView *FMissionView_Create(GameSystem *gsys, FieldSubscreen *subscreen) {
    FMissionView *view = FMissionViewSetup_Create(gsys, subscreen);

    view->printNow = TRUE;
    FMissionView_DrawAll(view);
    view->printNow = FALSE;
    view->active = FALSE;
    FMissionView_SetDimmed(view, TRUE);
    return view;
}

void FMissionView_Free(FMissionView *view) {
    FMissionViewSetup_Free(view);
}

void FMissionView_Update(FMissionView *view, BOOL active) {
    if (view->active != active) {
        BOOL locked;

        if (view->eventRunning && !active) {
            locked = TRUE;
        } else {
            locked = FALSE;
        }
        if (!locked) {
            view->active = active;
            FMissionView_SetDimmed(view, active ? FALSE : TRUE);
        }
    }
    if (view->eventRequest != FMISSION_VIEW_EVENT_NONE) {
        if (!active) {
            FMissionViewSetup_ClearEventRequest(view);
        }
        return;
    }
    if (!active) {
        return;
    }
    GFL_TCBExMgrUpdate(view->tcbManager);
    switch (view->seq) {
    case SEQ_MAIN:
        view->seq = FMissionView_SeqMain(view);
        break;
    case SEQ_NEW_ENTRY:
        view->seq = FMissionView_SeqNewEntry(view);
        break;
    case SEQ_WAIT_TASKS:
        view->seq = FMissionView_SeqWaitTasks(view);
        break;
    case SEQ_SCROLL:
        view->seq = FMissionView_SeqScroll(view);
        break;
    case SEQ_QUIT:
        view->seq = FMissionView_SeqQuit(view);
        break;
    case SEQ_5:
        view->seq = FMissionView_Seq5(view);
        break;
    case SEQ_6:
    case SEQ_7:
    case SEQ_8:
        break;
    }
}

BOOL FMissionView_IsPrinting(FMissionView *view) {
    return PrintQueue_Main(view->printQueue);
}

GameEvent *FMissionView_CheckEvent(FMissionView *view, BOOL enabled) {
    return FMissionViewSetup_CheckEvent(view, enabled);
}

static int FMissionView_SeqMain(FMissionView *view) {
    int next;

    FMissionView_UpdateMemberCount(view);
    if (FMissionView_CheckGreeting(view)) {
        view->wait = ENTRY_WAIT;
        return SEQ_WAIT_TASKS;
    }
    WihDwc_GetStatus(view->wifiList);
    next = FMissionView_CheckTouch(view);
    if (next != SEQ_MAIN) {
        return next;
    }
    if (view->wait != 0) {
        view->wait--;
        return SEQ_MAIN;
    }
    if (FMissionView_CheckPassPower(view) == TRUE) {
        view->wait = ENTRY_WAIT;
        return SEQ_WAIT_TASKS;
    }
    if (!FMissionView_CheckNewEntry(view)) {
        return SEQ_MAIN;
    }
    view->wait = ENTRY_WAIT;
    return SEQ_NEW_ENTRY;
}

static int FMissionView_SeqNewEntry(FMissionView *view) {
    if (!FMissionView_AddNewEntry(view)) {
        return SEQ_NEW_ENTRY;
    }
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_ALL, FMISSION_VIEW_BUTTON_STATE_NORMAL);
    view->subSeq = 0;
    return SEQ_MAIN;
}

static int FMissionView_SeqWaitTasks(FMissionView *view) {
    if (view->pending != 0) {
        return SEQ_WAIT_TASKS;
    }
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_ALL, FMISSION_VIEW_BUTTON_STATE_NORMAL);
    return SEQ_MAIN;
}

// Scrolls on for as long as the arrow is held
static int FMissionView_SeqScroll(FMissionView *view) {
    if (!view->scrollEnd) {
        int dir = FMissionView_GetHeldScroll(view);

        if (dir != view->scrollDir || dir == -1) {
            view->scrollEnd = TRUE;
        }
        view->scrollFrames++;
    }
    if (view->scrollEnd) {
        if (view->pending > 0) {
            return SEQ_SCROLL;
        }
        view->scrollEnd = FALSE;
        view->scrollFrames = 0;
        view->scrolling = FALSE;
        FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_ALL, FMISSION_VIEW_BUTTON_STATE_NORMAL);
        BeaconView_SetPrintTimeLimit(view->printQueue, FALSE);
        return view->scrollNextSeq;
    }
    if (view->pending == 0) {
        FMissionView_ScrollOn(view);
    }
    return SEQ_SCROLL;
}

static int FMissionView_SeqQuit(FMissionView *view) {
    if (!FMissionView_Quit(view)) {
        return SEQ_QUIT;
    }
    view->subSeq = 0;
    return SEQ_MAIN;
}

static int FMissionView_Seq5(FMissionView *view) {
    view->subSeq = 0;
    return SEQ_MAIN;
}

// Darkens the screen's palettes and blends the field's screen
static void FMissionView_SetDimmed(FMissionView *view, BOOL dimmed) {
    if (dimmed) {
        PaletteFade_BlendBuffer(view->fade, PALFADE_BUFFER_SUB_BG, 0, 0xe0, 6, 0);
        PaletteFade_BlendBuffer(view->fade, PALFADE_BUFFER_SUB_OBJ, 0, 0xe0, 6, 0);
        FieldDispControl_ReqSetAlphaB(view->dispControl, 5, 30, 10, 6);
    } else {
        PaletteFade_BlendBuffer(view->fade, PALFADE_BUFFER_SUB_BG, 0, 0xe0, 0, 0);
        PaletteFade_BlendBuffer(view->fade, PALFADE_BUFFER_SUB_OBJ, 0, 0xe0, 0, 0);
        FieldDispControl_ReqSetAlphaB(view->dispControl, 4, 30, 13, 3);
    }
}

static void FMissionView_DrawAll(FMissionView *view) {
    int i;

    view->memberCount = LinkFestival_GetMemberCount(view->festival);
    PaletteFade_SetTransferAll(view->fade, TRUE);
    for (i = 0; i < view->logVisible; i++) {
        u8 top = view->logTop;
        int logIndex;
        FMissionViewPanel *panel;

        FMissionLogFV_GetEntry(view->log, view->entry, top + i);
        logIndex = FMissionLogFV_GetIndex(view->log, top + i);
        panel = FMissionView_GetFreePanel(view, SCROLL_NONE);
        FMissionViewPanel_Set(panel, logIndex, i);
        FMissionView_DrawPanel(view, panel, view->entry);
        FMissionViewPanel_SetVisible(panel, TRUE);
    }
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_ALL, FMISSION_VIEW_BUTTON_STATE_NORMAL);
    if (view->passPower == PASS_POWER_NONE) {
        BeaconView_SetActorAnim(view->buttons[FMISSION_VIEW_BUTTON_PASS_POWER],
                                BUTTON_ANIM(FMISSION_VIEW_BUTTON_PASS_POWER, FMISSION_VIEW_BUTTON_STATE_DISABLED));
    }
    GFL_NetSetIconScreen(FALSE, view->tmpHeapId);
    FMissionView_DrawMission(view);
    FMissionView_DrawMemberCount(view, &view->memberPending);
}

// Starts what the button touched this frame does, and returns the sequence to go on with
static int FMissionView_CheckTouch(FMissionView *view) {
    FMissionViewTouch touch;
    int button;

    if (!GFL_HIDGetTouchPressedPoint(&touch.x, &touch.y)) {
        return SEQ_MAIN;
    }
    button = FMissionView_GetTouchedButton(view, &touch);
    if (button != -1) {
        switch (button) {
        case FMISSION_VIEW_BUTTON_PASS_POWER:
            FMissionViewSetup_RequestEvent(view, FMISSION_VIEW_EVENT_PASS_POWER);
            break;
        case FMISSION_VIEW_BUTTON_GREETING:
            FMissionViewSetup_RequestEvent(view, FMISSION_VIEW_EVENT_GREETING);
            break;
        case FMISSION_VIEW_BUTTON_QUIT:
            return SEQ_QUIT;
        case FMISSION_VIEW_BUTTON_INFO:
            FMissionViewSetup_RequestEvent(view, FMISSION_VIEW_EVENT_INFO);
            break;
        case FMISSION_VIEW_BUTTON_UP:
        case FMISSION_VIEW_BUTTON_DOWN:
            FMissionView_StartScroll(view, (button - FMISSION_VIEW_BUTTON_UP) ^ 1, SEQ_MAIN);
            return SEQ_SCROLL;
        }
        return SEQ_MAIN;
    }
    return SEQ_MAIN;
}

// The way of the arrow that is held, or -1
static int FMissionView_GetHeldScroll(FMissionView *view) {
    FMissionViewTouch touch;
    u32 arrow;

    if (!GFL_HIDGetTouchHeldPoint(&touch.x, &touch.y)) {
        return -1;
    }
    arrow = FMissionView_GetTouchedButton(view, &touch) - FMISSION_VIEW_BUTTON_UP;
    if (arrow <= FMISSION_VIEW_BUTTON_DOWN - FMISSION_VIEW_BUTTON_UP) {
        return arrow ^ 1;
    }
    return -1;
}

// Shows the player's new greeting, once it was changed
static BOOL FMissionView_CheckGreeting(FMissionView *view) {
    u16 *changed = (u16 *)BeaconStatus_GetGreetingChanged(view->beaconStatus);

    if (*changed == 0) {
        return FALSE;
    }
    *changed = 0;
    GFL_WordSetSetStrbuf(view->wordSet, 0, BeaconStatus_GetGreeting(view->beaconStatus), view->gender, 1, 2);
    FMissionView_ShowMessage(view, FmissionViewSetup_Text_Said);
    return TRUE;
}

static BOOL FMissionView_CheckPassPower(FMissionView *view) {
    int received;
    u32 passPower = FMissionLogFV_GetPassPower(view->passPowers, view->tmpStr, &received);

    if (passPower == PASS_POWER_NONE) {
        return FALSE;
    }
    return FMissionView_OfferPassPower(view, passPower, view->tmpStr, received);
}

// Moves the next new entry into the view's log
static BOOL FMissionView_CheckNewEntry(FMissionView *view) {
    if (!FMissionLogFV_PopEntry(view->newLog, view->newEntry)) {
        return FALSE;
    }
    view->newOffset = FMissionLogFV_PushEntry(view->log, view->newEntry);
    view->newIndex = FMissionLogFV_GetIndex(view->log, view->newOffset);
    view->unk5C = 0;
    view->prevCount = view->logCount;
    if (view->logCount < FEST_MISSION_LOG_MAX) {
        view->logCount++;
        if (view->logCount <= FMISSION_VIEW_PANEL_COUNT) {
            view->logVisible = view->logCount;
        }
    }
    return TRUE;
}

static void FMissionView_UpdateMemberCount(FMissionView *view) {
    u8 count = LinkFestival_GetMemberCount(view->festival);

    if (count > view->memberCount && view->memberPending == 0) {
        view->memberCount = count;
        FMissionView_DrawMemberCount(view, &view->memberPending);
    }
}

static void FMissionView_ScrollOn(FMissionView *view) {
    FMissionView_Scroll(view, view->scrollDir);
}

void FMissionView_SetButtonState(FMissionView *view, u32 button, u32 state) {
    u8 mask;

    if (button == FMISSION_VIEW_BUTTON_ALL) {
        mask = 0xff;
    } else {
        mask = 1 << button;
    }
    if ((mask & (1 << FMISSION_VIEW_BUTTON_PASS_POWER)) && view->passPower != PASS_POWER_NONE) {
        BeaconView_SetActorAnim(view->buttons[FMISSION_VIEW_BUTTON_PASS_POWER],
                                BUTTON_ANIM(FMISSION_VIEW_BUTTON_PASS_POWER, state));
    }
    if (mask & (1 << FMISSION_VIEW_BUTTON_GREETING)) {
        BeaconView_SetActorAnim(view->buttons[FMISSION_VIEW_BUTTON_GREETING],
                                BUTTON_ANIM(FMISSION_VIEW_BUTTON_GREETING, state));
    }
    if (mask & (1 << FMISSION_VIEW_BUTTON_QUIT)) {
        BeaconView_SetActorAnim(view->buttons[FMISSION_VIEW_BUTTON_QUIT],
                                BUTTON_ANIM(FMISSION_VIEW_BUTTON_QUIT, state));
    }
    if (mask & (1 << FMISSION_VIEW_BUTTON_INFO)) {
        BeaconView_SetActorAnim(view->buttons[FMISSION_VIEW_BUTTON_INFO],
                                BUTTON_ANIM(FMISSION_VIEW_BUTTON_INFO, state));
    }
    if (mask & ((1 << FMISSION_VIEW_BUTTON_UP) | (1 << FMISSION_VIEW_BUTTON_DOWN))) {
        if (state == FMISSION_VIEW_BUTTON_STATE_NORMAL) {
            FMissionView_UpdateArrows(view);
            return;
        }
        BeaconView_SetActorAnim(view->buttons[FMISSION_VIEW_BUTTON_UP], BUTTON_ANIM(FMISSION_VIEW_BUTTON_UP, state));
        BeaconView_SetActorAnim(view->buttons[FMISSION_VIEW_BUTTON_DOWN],
                                BUTTON_ANIM(FMISSION_VIEW_BUTTON_DOWN, state));
    }
}

BOOL FMissionView_IsButtonAnimating(FMissionView *view, int button) {
    return ClActor_IsAnimActive(view->buttons[button]);
}

void FMissionView_TransferPanels(FMissionView *view) {
    int i;

    for (i = 0; i < FMISSION_VIEW_PANEL_COUNT; i++) {
        FMissionViewPanel *panel = &view->panels[i];

        if (panel->logIndex != FMISSION_VIEW_PANEL_NONE && panel->dirty) {
            FMissionView_TransferPanel(view, panel);
        }
    }
}

// Brings the new entry in at the top of the list, and offers the Pass Power of one that another player used
static BOOL FMissionView_AddNewEntry(FMissionView *view) {
    if (view->pending != 0) {
        return FALSE;
    }
    switch (view->subSeq) {
    case 0: {
        u32 kind;
        u32 se;

        FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_ALL, FMISSION_VIEW_BUTTON_STATE_DISABLED);
        kind = FMissionLogEntryFV_GetKind(view->newEntry);
        switch (kind) {
        case LOG_KIND_TEXT:
            se = sFMissionViewWordsSE[FMissionLogEntryFV_GetValue(view->newEntry) % NELEMS(sFMissionViewWordsSE)];
            break;
        case LOG_KIND_JOIN:
            if (view->memberCount < 10) {
                se = SEQ_SE_SW_FES_11;
            } else {
                se = SEQ_SE_SW_FES_12;
            }
            break;
        default:
            if (FestMissionConfig_IsTarget(view->config, kind, FMissionLogEntryFV_GetValue(view->newEntry))) {
                se = SEQ_SE_SW_FES_03;
            } else {
                se = SEQ_SE_SYS_11;
            }
            break;
        }
        BeaconView_PlaySE(se);
        if (view->logTop != 0) {
            view->newOffset = view->logTop;
            FMissionLogFV_GetEntry(view->log, view->newEntry, view->newOffset);
            view->newIndex = FMissionLogFV_GetIndex(view->log, view->newOffset);
        }
        FMissionView_AddPanel(view, view->newEntry, view->newIndex, SCROLL_NEWER, view->logTop == 0);
        view->logTop = view->newOffset;
        view->subSeq++;
        break;
    }
    case 1:
        if (FMissionLogEntryFV_IsOwn(view->newEntry) ||
            FMissionLogEntryFV_GetKind(view->newEntry) != LOG_KIND_PASS_POWER) {
            return TRUE;
        }
        if (!FMissionView_OfferEntryPassPower(view, view->newEntry)) {
            return TRUE;
        }
        view->subSeq++;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

// Asks whether to quit the mission, and waits for the answer's tasks
static BOOL FMissionView_Quit(FMissionView *view) {
    switch (view->subSeq) {
    case 0:
        BeaconView_SetPrintTimeLimit(view->printQueue, TRUE);
        FMissionView_StartQuit(view);
        view->subSeq++;
        break;
    case 1:
        if (view->pending == 0) {
            BeaconView_SetPrintTimeLimit(view->printQueue, FALSE);
            view->subSeq = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The button at a point that can be touched now, or -1
static int FMissionView_GetTouchedButton(FMissionView *view, FMissionViewTouch *touch) {
    TouchRect rects[2];
    u8 scroll = 0;
    int i;

    rects[1].top = TOUCH_RECT_END;
    if (view->logTop != 0) {
        scroll |= 1;
    }
    if (view->logTop + view->logVisible < view->logCount) {
        scroll |= 2;
    }
    for (i = 0; i < FMISSION_VIEW_BUTTON_COUNT; i++) {
        if ((u8)(ClActor_GetSequence(view->buttons[i]) - (i + 1) * FMISSION_VIEW_BUTTON_STATE_COUNT) %
                FMISSION_VIEW_BUTTON_STATE_COUNT >=
            FMISSION_VIEW_BUTTON_STATE_DISABLED) {
            continue;
        }
        if (sFMissionViewButtons[i].scroll != 0 && !(sFMissionViewButtons[i].scroll & scroll)) {
            continue;
        }
        rects[0].top = sFMissionViewButtons[i].y;
        rects[0].bottom = rects[0].top + BUTTON_SIZE;
        rects[0].left = sFMissionViewButtons[i].x;
        rects[0].right = rects[0].left + BUTTON_SIZE;
        if (GFL_HIDGetTouchRectAt(rects, touch->x, touch->y) == TOUCH_RECT_NONE) {
            continue;
        }
        if (sFMissionViewButtons[i].scroll == 0) {
            BeaconView_PlaySE(SEQ_SE_SYS_79);
        }
        return i;
    }
    return -1;
}

// Loads a message with the word set's words into view->message
static void FMissionView_LoadMessage(FMissionView *view, u8 messageId) {
    GFL_MsgDataLoadStrbuf(view->msgData, messageId, view->tmpStr);
    GFL_WordSetFormatStrbuf(view->wordSet, view->message, view->tmpStr);
}

static void FMissionView_PrintMessage(FMissionView *view, StrBuf *strbuf, int *pending) {
    BeaconViewWin_Print(&view->wins[FMISSION_VIEW_WIN_MESSAGE].win, strbuf, 0, 0, 9, PRINT_COLOR(1, 2, 9), pending,
                        view->printNow);
}

// Draws the mission's name, centered, and what it wants
static void FMissionView_DrawMission(FMissionView *view) {
    int x;

    FestMission_GetMessage(view->festText, view->message, view->config, view->tmpHeapId);
    x = (184 - (int)GFL_FontGetBlockWidth(view->message, view->font, 0)) / 2;
    BeaconViewWin_Print(&view->wins[FMISSION_VIEW_WIN_MISSION].win, view->message, (u8)x, 4, 13, PRINT_COLOR(1, 2, 13),
                        NULL, view->printNow);
    WordSetNumber(view->wordSet, 0, view->config->info.count, 4, 1, TRUE);
    FMissionView_LoadMessage(view, view->config->info.targetKind);
    BeaconViewWin_Print(&view->wins[FMISSION_VIEW_WIN_GOAL].win, view->message, 0, 0, 13, PRINT_COLOR(1, 2, 13), NULL,
                        view->printNow);
}

static void FMissionView_DrawMemberCount(FMissionView *view, int *pending) {
    GFL_BitmapFill(view->wins[FMISSION_VIEW_WIN_MEMBERS].win.bitmap, 13);
    WordSetNumber(view->wordSet, 0, view->memberCount, 3, 1, TRUE);
    FMissionView_LoadMessage(view, FmissionViewSetup_Text_Participants);
    BeaconViewWin_PrintEx(&view->wins[FMISSION_VIEW_WIN_MEMBERS].win, view->message, 0, 0, 13, PRINT_COLOR(1, 2, 13),
                          pending, view->printNow, FALSE);
}

// Shows or hides the buttons that a menu covers
static void FMissionView_SetButtonsVisible(FMissionView *view, BOOL visible) {
    ClActor_SetVisible(view->buttons[FMISSION_VIEW_BUTTON_PASS_POWER], visible);
    ClActor_SetVisible(view->buttons[FMISSION_VIEW_BUTTON_GREETING], visible);
    ClActor_SetVisible(view->buttons[FMISSION_VIEW_BUTTON_QUIT], visible);
    ClActor_SetVisible(view->buttons[FMISSION_VIEW_BUTTON_UP], visible);
    ClActor_SetVisible(view->buttons[FMISSION_VIEW_BUTTON_DOWN], visible);
}

// Shows each arrow as usable or not, by whether the list can scroll its way
static void FMissionView_UpdateArrows(FMissionView *view) {
    if (view->logTop == 0) {
        ClActor_SetSequence(view->buttons[FMISSION_VIEW_BUTTON_UP], ANIM_UP_END);
    } else {
        ClActor_SetSequence(view->buttons[FMISSION_VIEW_BUTTON_UP],
                            BUTTON_ANIM(FMISSION_VIEW_BUTTON_UP, FMISSION_VIEW_BUTTON_STATE_NORMAL));
    }
    if (view->logTop + view->logVisible < view->logCount) {
        ClActor_SetSequence(view->buttons[FMISSION_VIEW_BUTTON_DOWN],
                            BUTTON_ANIM(FMISSION_VIEW_BUTTON_DOWN, FMISSION_VIEW_BUTTON_STATE_NORMAL));
    } else {
        ClActor_SetSequence(view->buttons[FMISSION_VIEW_BUTTON_DOWN], ANIM_DOWN_END);
    }
}

static void FMissionViewPanel_SetVisible(FMissionViewPanel *panel, BOOL visible) {
    int i;

    for (i = 0; i < FMISSION_VIEW_PANEL_LINE_COUNT; i++) {
        BmpOam_ActorSetDrawEnable(panel->lines[i].actor, visible);
    }
    if (!visible) {
        FMissionViewPanel_SetMarkVisible(panel, visible);
    }
    ClActor_SetVisible(panel->icon, visible);
    ClActor_SetVisible(panel->frame, visible);
}

static void FMissionViewPanel_SetMarkVisible(FMissionViewPanel *panel, BOOL visible) {
    ClActor_SetVisible(panel->markActor, visible);
}

static void FMissionViewPanel_Clear(FMissionViewPanel *panel) {
    int i;

    if (panel->markTask != NULL) {
        GFL_TCBExRequestEnd(panel->markTask);
        panel->markTask = NULL;
        panel->mark = 0;
    }
    for (i = 0; i < FMISSION_VIEW_PANEL_LINE_COUNT; i++) {
        GFL_BitmapFill(panel->lines[i].bitmap, 0);
        GFL_StrBufClear(panel->strs[i]);
    }
    panel->logIndex = FMISSION_VIEW_PANEL_NONE;
    panel->slot = 0;
    panel->look = 0;
    panel->mark = 0;
    panel->kind = 0;
}

static void FMissionViewPanel_Set(FMissionViewPanel *panel, u8 logIndex, u8 slot) {
    FMissionViewPanel_Clear(panel);
    panel->logIndex = logIndex;
    panel->slot = slot;
}

// A panel that shows nothing, or else the one that scrolls out: the first when the list scrolls to older entries,
// the last when it scrolls to newer ones
static FMissionViewPanel *FMissionView_GetFreePanel(FMissionView *view, int dir) {
    int i;
    u32 slot;

    for (i = 0; i < FMISSION_VIEW_PANEL_COUNT; i++) {
        if (view->panels[i].logIndex == FMISSION_VIEW_PANEL_NONE) {
            return &view->panels[i];
        }
    }
    if (dir == SCROLL_NONE) {
        return NULL;
    }
    slot = (dir == SCROLL_OLDER) ? 0 : FMISSION_VIEW_PANEL_COUNT - 1;
    for (i = 0; i < FMISSION_VIEW_PANEL_COUNT; i++) {
        if (view->panels[i].slot == slot) {
            return &view->panels[i];
        }
    }
    return NULL;
}

// Gives the panel's icon the look of its player: the icon of the look, or the player's own model for an own entry
static void FMissionView_LoadPanelIcon(FMissionView *view, FMissionViewPanel *panel) {
    if (!panel->own) {
        u8 color;

        ClActSys_ReplaceCharData(view->res[FMISSION_VIEW_RES_ICON].lists[FMISSION_VIEW_RES_CHARS].ids[panel->index],
                                 view->iconChars[panel->look].data);
        color = sFMissionViewLookPalettes[panel->look] * 16;
        PaletteFade_LoadData(view->fade, &view->iconPalettes[color], PALFADE_BUFFER_SUB_OBJ,
                             (panel->index + PANEL_ICON_PALETTE) * 16, 0x20);
    } else {
        NsbtxToClwk_LoadTextureEx(panel->icon, ARCID_MMODEL_GRA, sFMissionViewPlayerModels[panel->gender], 3, 4, 4, 0,
                                  CLACT_VRAM_SUB, view->fade, panel->index + PANEL_ICON_PALETTE, view->tmpHeapId);
    }
}

// The animation of the frame of an entry: 2 for a player's words, 1 for what counts, 0 for the rest
static int FMissionView_GetEntryFrame(FMissionView *view, u32 kind, u32 value) {
    switch (kind) {
    case LOG_KIND_TEXT:
        return 2;
    case LOG_KIND_START:
    case LOG_KIND_JOIN:
        return 1;
    }
    if (FestMissionConfig_IsTarget(view->config, kind, value)) {
        return 1;
    }
    return 0;
}

static void FMissionView_TransferPanel(FMissionView *view, FMissionViewPanel *panel) {
    int i;

    for (i = 0; i < FMISSION_VIEW_PANEL_LINE_COUNT; i++) {
        BmpOam_ActorBmpTrans(panel->lines[i].actor);
    }
    panel->dirty = FALSE;
}

// Takes what the panel shows from an entry, and prints its lines: the name, what was done and the score
static void FMissionView_PrintPanel(FMissionView *view, FMissionViewPanel *panel, FestMissionLogEntry *entry) {
    u32 kind = FMissionLogEntryFV_GetKind(entry);
    int i;

    panel->gender = FMissionLogEntryFV_GetGender(entry);
    panel->own = FMissionLogEntryFV_IsOwn(entry);
    panel->value = FMissionLogEntryFV_GetValue(entry);
    panel->kind = FMissionView_GetEntryFrame(view, kind, panel->value);
    panel->look = FMissionLogEntryFV_GetLook(entry);
    FestMissionLogEntry_GetName(entry, panel->strs[0]);
    WordSetNumber(view->wordSet, 0, FMissionLogEntryFV_GetScore(entry), 4, 1, TRUE);
    GFL_MsgDataLoadStrbuf(view->msgData, FmissionViewSetup_Text_Score, view->tmpStr);
    GFL_WordSetFormatStrbuf(view->wordSet, panel->strs[2], view->tmpStr);
    FMissionLogEntryFV_SetWords(entry, view->wordSet, view->heapId);
    GFL_MsgDataLoadStrbuf(view->msgData, kind + FmissionViewSetup_Text_Score, view->tmpStr);
    GFL_WordSetFormatStrbuf(view->wordSet, panel->strs[1], view->tmpStr);
    {
        u16 colors[2] = { PRINT_COLOR(15, 3, 0), PRINT_COLOR(6, 5, 0) };
        int textX = 0;

        if (panel->kind == 2) {
            colors[0] = PRINT_COLOR(2, 3, 0);
            colors[1] = PRINT_COLOR(6, 4, 0);
            textX = 6;
        }
        if (view->printNow) {
            for (i = 0; i < FMISSION_VIEW_PANEL_LINE_COUNT; i++) {
                GFL_TextRendererDrawToBitmapEx(panel->lines[i].bitmap, (i == 1) ? textX : 0, 0, panel->strs[i],
                                               view->font, colors[i == 2]);
            }
        } else {
            for (i = 0; i < FMISSION_VIEW_PANEL_LINE_COUNT; i++) {
                PrintQueue_PrintColor(view->printQueue, panel->lines[i].bitmap, (i == 1) ? textX : 0, 0, panel->strs[i],
                                      view->font, colors[i == 2]);
            }
        }
    }
}

static void FMissionView_ShowPanel(FMissionView *view, FMissionViewPanel *panel) {
    FMissionView_LoadPanelIcon(view, panel);
    BeaconView_SetActorAnim(panel->frame, (u8)(ANIM_FRAME + panel->kind));
    FMissionViewPanel_SetVisible(panel, TRUE);
    panel->dirty = TRUE;
}

static BOOL FMissionView_ShowPanelWhenPrinted(FMissionView *view, FMissionViewPanel *panel) {
    int i;

    for (i = 0; i < FMISSION_VIEW_PANEL_LINE_COUNT; i++) {
        if (PrintQueue_IsBitmapPending(view->printQueue, panel->lines[i].bitmap)) {
            return FALSE;
        }
    }
    FMissionView_ShowPanel(view, panel);
    return TRUE;
}

static void FMissionView_DrawPanel(FMissionView *view, FMissionViewPanel *panel, FestMissionLogEntry *entry) {
    FMissionView_PrintPanel(view, panel, entry);
    FMissionView_ShowPanel(view, panel);
    FMissionView_TransferPanel(view, panel);
}

static void FMissionViewPanel_SetPos(FMissionViewPanel *panel, s16 x, s16 y) {
    panel->x = x;
    panel->y = y;
    BeaconView_SetActorPos(panel->frame, x, y);
    BeaconView_SetActorPos(panel->markActor, x + PANEL_MARK_X, y + PANEL_MARK_Y);
    BeaconView_SetActorPos(panel->icon, x + PANEL_ICON_X, y + PANEL_ICON_Y);
    BmpOam_ActorSetPos(panel->lines[0].actor, x + PANEL_NAME_X, y + PANEL_NAME_Y);
    BmpOam_ActorSetPos(panel->lines[1].actor, x + PANEL_TEXT_X, y + PANEL_TEXT_Y);
    BmpOam_ActorSetPos(panel->lines[2].actor, x + PANEL_SCORE_X, y + PANEL_SCORE_Y);
}

// Pops up the mark of a player's words for a while
static void FMissionView_StartPanelMark(FMissionView *view, FMissionViewPanel *panel) {
    TCBEx *task;
    FMissionViewMarkTask *data;

    if (panel->mark == 0) {
        FMissionViewPanel_SetMarkVisible(panel, FALSE);
        return;
    }
    task = GFL_TCBExMgrAddTask(view->tcbManager, FMissionView_MarkTask, sizeof(FMissionViewMarkTask), 0);
    data = GFL_TCBExGetData(task);
    MI_CpuFill8(data, 0, sizeof(FMissionViewMarkTask));
    data->view = view;
    data->panel = panel;
    data->timer = MESSAGE_WAIT;
    data->panel->markTask = task;
    BeaconView_SetActorAnim(panel->markActor, (u8)(panel->mark + ANIM_MARK));
    FMissionViewPanel_SetMarkVisible(panel, TRUE);
}

static void FMissionView_MarkTask(TCBEx *task, void *data) {
    FMissionViewMarkTask *mark = data;

    if (mark->timer-- == 0) {
        mark->panel->markTask = NULL;
        mark->panel->mark = 0;
        FMissionViewPanel_SetMarkVisible(mark->panel, FALSE);
        GFL_TCBExRequestEnd(task);
    }
}

// Scrolls the list by one entry, bringing in the next one that way
static void FMissionView_Scroll(FMissionView *view, int dir) {
    u8 offset;
    u8 logIndex;

    BeaconView_PlaySE(SEQ_SE_SELECT4);
    if (dir == SCROLL_OLDER) {
        offset = view->logTop + view->logVisible;
        view->logTop++;
    } else {
        offset = view->logTop - 1;
        view->logTop--;
    }
    FMissionLogFV_SetTop(view->log, view->logTop);
    FMissionLogFV_GetEntry(view->log, view->entry, offset);
    logIndex = FMissionLogFV_GetIndex(view->log, offset);
    FMissionView_AddPanel(view, view->entry, logIndex, dir, FALSE);
}

static void FMissionView_StartScroll(FMissionView *view, int dir, u8 nextSeq) {
    view->scrolling = TRUE;
    view->scrollEnd = FALSE;
    view->scrollDir = dir;
    view->scrollNextSeq = nextSeq;
    view->scrollFrames = 0;
    BeaconView_SetPrintTimeLimit(view->printQueue, TRUE);
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_ALL, FMISSION_VIEW_BUTTON_STATE_DISABLED);
    if (view->scrollDir == SCROLL_OLDER) {
        BeaconView_SetActorAnim(view->buttons[FMISSION_VIEW_BUTTON_DOWN],
                                BUTTON_ANIM(FMISSION_VIEW_BUTTON_DOWN, FMISSION_VIEW_BUTTON_STATE_PRESSED));
    } else {
        BeaconView_SetActorAnim(view->buttons[FMISSION_VIEW_BUTTON_UP],
                                BUTTON_ANIM(FMISSION_VIEW_BUTTON_UP, FMISSION_VIEW_BUTTON_STATE_PRESSED));
    }
    FMissionView_Scroll(view, dir);
}

// Prints an entry into the panel that scrolls out, and starts the task that moves the panels once it is printed
static void FMissionView_AddPanel(FMissionView *view, FestMissionLogEntry *entry, u8 logIndex, int dir, BOOL mark) {
    FMissionViewPanel *panel = FMissionView_GetFreePanel(view, dir);
    FMissionViewPanelTask *data;

    if (panel == NULL) {
        return;
    }
    data = GFL_TCBExGetData(
        GFL_TCBExMgrAddTask(view->tcbManager, FMissionView_PanelTask, sizeof(FMissionViewPanelTask), 0));
    MI_CpuFill8(data, 0, sizeof(FMissionViewPanelTask));
    data->view = view;
    data->dir = dir;
    data->panel = panel;
    data->pending = &view->pending;
    view->pending++;
    FMissionViewPanel_Set(panel, logIndex, 0);
    FMissionView_PrintPanel(view, panel, entry);
    if (mark && panel->kind == 2) {
        panel->mark = (u8)panel->value;
    } else {
        panel->mark = 0;
    }
}

static void FMissionView_PanelTask(TCBEx *task, void *data) {
    FMissionViewPanelTask *slide = data;
    FMissionView *view = slide->view;
    int i;

    switch (slide->seq) {
    case 0:
        if (!FMissionView_ShowPanelWhenPrinted(view, slide->panel)) {
            return;
        }
        for (i = 0; i < FMISSION_VIEW_PANEL_COUNT; i++) {
            FMissionViewPanel *panel = &view->panels[i];

            if (panel->logIndex == FMISSION_VIEW_PANEL_NONE) {
                continue;
            }
            if (panel == slide->panel) {
                if (slide->dir == SCROLL_OLDER) {
                    panel->slot = FMISSION_VIEW_PANEL_COUNT - 1;
                } else {
                    panel->slot = 0;
                }
            } else {
                if (slide->dir == SCROLL_OLDER) {
                    panel->slot--;
                } else {
                    panel->slot++;
                }
            }
            FMissionViewPanel_SetPos(panel, FMISSION_VIEW_PANEL_X,
                                     panel->slot * FMISSION_VIEW_PANEL_HEIGHT + FMISSION_VIEW_PANEL_Y);
            if (panel == slide->panel) {
                FMissionView_StartPanelMark(view, slide->panel);
            }
        }
        slide->seq++;
        break;
    case 1:
    default:
        if (slide->pending != NULL) {
            (*slide->pending)--;
        }
        GFL_TCBExRequestEnd(task);
        break;
    }
}

// Slides a message of the view's bank in over the buttons
static void FMissionView_ShowMessage(FMissionView *view, u8 messageId) {
    FMissionView_LoadMessage(view, messageId);
    FMissionView_StartMessage(view, view->message, &view->pending);
}

static void FMissionView_StartMessage(FMissionView *view, StrBuf *strbuf, int *pending) {
    FMissionViewMessageTask *data = GFL_TCBExGetData(
        GFL_TCBExMgrAddTask(view->tcbManager, FMissionView_MessageTask, sizeof(FMissionViewMessageTask), 0));

    MI_CpuFill8(data, 0, sizeof(FMissionViewMessageTask));
    GFL_StrBufCopy(view->messageCopy, strbuf);
    data->view = view;
    data->timer = MESSAGE_WAIT;
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_ALL, FMISSION_VIEW_BUTTON_STATE_DISABLED);
    data->pending = pending;
    (*pending)++;
}

static void FMissionView_MessageTask(TCBEx *task, void *data) {
    FMissionViewMessageTask *message = data;

    if (message->wait != 0) {
        return;
    }
    switch (message->seq) {
    case 0:
        FMissionView_PrintMessage(message->view, message->view->messageCopy, &message->wait);
        message->seq++;
        break;
    case 1:
        BeaconView_StartBGSlide(message->view->tcbManager, 6, FALSE, &message->wait);
        message->seq++;
        break;
    case 2:
        if (BeaconView_WaitTimerOrTouch(&message->timer, 6)) {
            BeaconView_StartBGSlide(message->view->tcbManager, 6, TRUE, &message->wait);
            message->seq++;
        }
        break;
    default:
        (*message->pending)--;
        GFL_TCBExRequestEnd(task);
        break;
    }
}

// Offers the Pass Power that the entry's player used
static BOOL FMissionView_OfferEntryPassPower(FMissionView *view, FestMissionLogEntry *entry) {
    u16 passPower = FestMissionPassPower_FromLogEntry(&view->passPowers->own, entry);

    FMissionLogEntryFV_SetWords(entry, view->wordSet, view->heapId);
    return FMissionView_StartPassPower(view, passPower, FALSE, &view->pending);
}

static BOOL FMissionView_OfferPassPower(FMissionView *view, u32 passPower, StrBuf *name, int received) {
    GFL_WordSetSetStrbuf(view->wordSet, 0, name, 2, 1, 2);
    return FMissionView_StartPassPower(view, passPower, received, &view->pending);
}

// Starts the task that asks whether to use a Pass Power, unless its effect already runs
static BOOL FMissionView_StartPassPower(FMissionView *view, u32 passPower, int received, int *pending) {
    FMissionViewPassPowerTask *data;

    if (!PassPower_IsIDValid(passPower)) {
        FMissionLogFV_RemovePassPower(view->passPowers, received);
        return FALSE;
    }
    if (passPower == PassPower_GetUsedIDByPassPower(passPower, view->passPowerData)) {
        return FALSE;
    }
    data = GFL_TCBExGetData(
        GFL_TCBExMgrAddTask(view->tcbManager, FMissionView_PassPowerTask, sizeof(FMissionViewPassPowerTask), 0));
    MI_CpuFill8(data, 0, sizeof(FMissionViewPassPowerTask));
    view->usePassPower = PASS_POWER_NONE;
    view->unk89 = 0;
    data->view = view;
    data->timer = MESSAGE_WAIT;
    data->passPower = passPower;
    data->received = received;
    loadPassPowerToStrbuf(view->wordSet, 1, data->passPower);
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_PASS_POWER, FMISSION_VIEW_BUTTON_STATE_DISABLED);
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_GREETING, FMISSION_VIEW_BUTTON_STATE_DISABLED);
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_QUIT, FMISSION_VIEW_BUTTON_STATE_DISABLED);
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_COUNT, FMISSION_VIEW_BUTTON_STATE_DISABLED);
    data->pending = pending;
    (*pending)++;
    return TRUE;
}

static void FMissionView_PassPowerTask(TCBEx *task, void *data) {
    FMissionViewPassPowerTask *offer = data;
    FMissionView *view = offer->view;

    if (offer->wait > 0) {
        return;
    }
    switch (offer->seq) {
    case 0:
        FMissionView_LoadMessage(view, FmissionViewSetup_Text_UseActivated);
        FMissionView_PrintMessage(view, view->message, &offer->wait);
        offer->seq++;
        break;
    case 1:
        BeaconView_StartBGSlide(view->tcbManager, 6, FALSE, &offer->wait);
        offer->seq++;
        break;
    case 2:
        BeaconView_SetPrintTimeLimit(view->printQueue, TRUE);
        BeaconViewMenu_Open(view->menu, 0);
        FMissionView_SetButtonsVisible(view, FALSE);
        offer->seq++;
        break;
    case 3: {
        int button = BeaconViewMenu_Update(view->menu);

        if (button < 0) {
            break;
        }
        offer->button = button;
        switch (offer->button) {
        case PASS_POWER_MENU_LIST:
            FMissionViewSetup_RequestEvent(view, FMISSION_VIEW_EVENT_PASS_POWER_LIST);
            offer->seq = 3;
            break;
        case PASS_POWER_MENU_USE:
            view->usePassPower = offer->passPower;
            FMissionViewSetup_RequestEvent(view, FMISSION_VIEW_EVENT_USE_PASS_POWER);
            offer->seq = 4;
            break;
        case PASS_POWER_MENU_CANCEL:
            FMissionLogFV_RemovePassPower(view->passPowers, offer->received);
            offer->seq = 5;
            break;
        }
        break;
    }
    case 4:
        // The event sets usePassPower back when the player didn't use the Pass Power after all
        if (view->usePassPower != PASS_POWER_NONE) {
            view->usePassPower = PASS_POWER_NONE;
            offer->seq = 3;
        } else {
            FMissionLogFV_RemovePassPower(view->passPowers, offer->received);
            BeaconViewMenu_Close(view->menu);
            offer->seq = 6;
        }
        break;
    case 5:
        if (BeaconViewMenu_WaitFlash(view->menu, offer->button)) {
            offer->seq++;
        }
        break;
    case 6:
        FMissionView_SetButtonsVisible(view, TRUE);
        BeaconView_StartBGSlide(view->tcbManager, 6, TRUE, &offer->wait);
        offer->seq++;
        break;
    default:
        FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_ALL, FMISSION_VIEW_BUTTON_STATE_NORMAL);
        BeaconView_SetPrintTimeLimit(view->printQueue, FALSE);
        (*offer->pending)--;
        GFL_TCBExRequestEnd(task);
        break;
    }
}

static void FMissionView_StartQuit(FMissionView *view) {
    FMissionView_StartQuitTask(view, &view->pending);
}

static void FMissionView_StartQuitTask(FMissionView *view, int *pending) {
    FMissionViewQuitTask *data =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(view->tcbManager, FMissionView_QuitTask, sizeof(FMissionViewQuitTask), 0));

    MI_CpuFill8(data, 0, sizeof(FMissionViewQuitTask));
    data->view = view;
    data->timer = MESSAGE_WAIT;
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_QUIT, FMISSION_VIEW_BUTTON_STATE_PRESSED);
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_PASS_POWER, FMISSION_VIEW_BUTTON_STATE_DISABLED);
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_GREETING, FMISSION_VIEW_BUTTON_STATE_DISABLED);
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_INFO, FMISSION_VIEW_BUTTON_STATE_DISABLED);
    FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_COUNT, FMISSION_VIEW_BUTTON_STATE_DISABLED);
    data->pending = pending;
    (*pending)++;
}

static void FMissionView_QuitTask(TCBEx *task, void *data) {
    FMissionViewQuitTask *quit = data;
    FMissionView *view = quit->view;

    if (quit->wait > 0) {
        return;
    }
    switch (quit->seq) {
    case 0:
        FMissionView_LoadMessage(view, FmissionViewSetup_Text_QuitFunfestMission);
        FMissionView_PrintMessage(view, view->message, &quit->wait);
        quit->seq++;
        break;
    case 1:
        BeaconView_StartBGSlide(view->tcbManager, 6, FALSE, &quit->wait);
        quit->seq++;
        break;
    case 2:
        BeaconView_SetPrintTimeLimit(view->printQueue, TRUE);
        BeaconViewMenu_Open(view->menu, 1);
        FMissionView_SetButtonsVisible(view, FALSE);
        quit->seq++;
        break;
    case 3: {
        int button = BeaconViewMenu_Update(view->menu);

        if (button < 0) {
            break;
        }
        quit->button = button;
        if (quit->button == 0) {
            FMissionViewSetup_RequestEvent(view, FMISSION_VIEW_EVENT_QUIT);
            quit->seq = 4;
        } else {
            quit->seq = 5;
        }
        break;
    }
    case 4:
        if (!view->eventDone) {
            quit->seq = 3;
        } else {
            BeaconViewMenu_Close(view->menu);
            quit->seq = 6;
        }
        break;
    case 5:
        if (BeaconViewMenu_WaitFlash(view->menu, quit->button)) {
            quit->seq++;
        }
        break;
    case 6:
        FMissionView_SetButtonsVisible(view, TRUE);
        BeaconView_StartBGSlide(view->tcbManager, 6, TRUE, &quit->wait);
        quit->seq++;
        break;
    default:
        FMissionView_SetButtonState(view, FMISSION_VIEW_BUTTON_ALL, FMISSION_VIEW_BUTTON_STATE_NORMAL);
        BeaconView_SetPrintTimeLimit(view->printQueue, FALSE);
        (*quit->pending)--;
        GFL_TCBExRequestEnd(task);
        break;
    }
}
