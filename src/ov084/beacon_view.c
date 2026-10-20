#include "types.h"
#include "constants/sound.h"
#include "field/beacon_view.h"
#include "field/beacon_view_common.h"
#include "field/fest_mission_data.h"
#include "field/field_disp_control.h"
#include "field/game_beacon_set.h"
#include "field/intrude_work.h"
#include "gfl/clact.h"
#include "gfl/net.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "save/high_link.h"
#include "save/trainer_game_info.h"
#include "system/beacon_status.h"
#include "system/game_beacon.h"
#include "system/game_system.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/game_beacon.h"

// The beacon view's sequences: what the field's touch screen calls, and what the view does from frame to frame. The
// ROM has no name for this file; "beacon_view.c" is a guess from "beacon_view_setup.c"

// The view's sequences
#define SEQ_MAIN 0
#define SEQ_NEW_BEACON 1
#define SEQ_WAIT_TASKS 2
#define SEQ_SCROLL 3
#define SEQ_BAR 4
#define SEQ_THANKS 5

// What SEQ_THANKS does
#define THANKS_START 0
#define THANKS_WAIT_BUTTON 1
#define THANKS_INPUT 2
#define THANKS_SCROLL_END 3
#define THANKS_SEND 4
#define THANKS_SENT 5
#define THANKS_END 6
#define THANKS_EXIT 7
// The bar is touched: not a state, but what the input gives
#define THANKS_BAR 8

// How long the view leaves a message alone
#define MESSAGE_WAIT 90

static int BeaconView_SeqMain(BeaconViewWork *work);
static int BeaconView_SeqNewBeacon(BeaconViewWork *work);
static int BeaconView_SeqWaitTasks(BeaconViewWork *work);
static int BeaconView_SeqScroll(BeaconViewWork *work);
static int BeaconView_SeqBar(BeaconViewWork *work);
static int BeaconView_SeqThanks(BeaconViewWork *work);
static void BeaconView_SetDimmed(BeaconViewWork *work, BOOL dimmed);
static void BeaconView_DrawAll(BeaconViewWork *work);
static int BeaconView_CheckTouch(BeaconViewWork *work);
static int BeaconView_GetHeldArrow(BeaconViewWork *work);
static int BeaconView_UpdateBarDrag(BeaconViewWork *work);
static BOOL BeaconView_CheckLeave(BeaconViewWork *work);
static BOOL BeaconView_CheckGreeting(BeaconViewWork *work);
static BOOL BeaconView_CheckPassPower(BeaconViewWork *work);
static BOOL BeaconView_CheckMission(BeaconViewWork *work);
static BOOL BeaconView_TakeNewBeacon(BeaconViewWork *work);
static void BeaconView_ScrollOn(BeaconViewWork *work);
static BOOL BeaconView_ShowNewBeacon(BeaconViewWork *work);
static int BeaconView_UpdateThanks(BeaconViewWork *work);
static int BeaconView_CheckThanksTouch(BeaconViewWork *work);
static void BeaconView_ResetArrows(BeaconViewWork *work);

BeaconViewWork *BeaconView_Init(GameSystem *gsys, void *parent) {
    BeaconViewWork *work = BeaconView_Create(gsys, parent);

    work->printNow = TRUE;
    BeaconView_DrawAll(work);
    work->printNow = FALSE;
    work->active = FALSE;
    BeaconView_SetDimmed(work, TRUE);
    return work;
}

void BeaconView_Exit(BeaconViewWork *work) {
    BeaconView_Free(work);
}

void BeaconView_Update(BeaconViewWork *work, BOOL active) {
    if (work->active != active) {
        BOOL busy;

        if (work->eventRunning && !active) {
            busy = TRUE;
        } else {
            busy = FALSE;
        }
        if (!busy) {
            work->active = active;
            BeaconView_SetDimmed(work, active ? FALSE : TRUE);
        }
    }
    if (work->eventRequest != BEACON_VIEW_EVENT_NONE) {
        if (!active) {
            BeaconView_ClearEventRequest(work);
        }
        return;
    }
    if (!active) {
        return;
    }
    GFL_TCBExMgrUpdate(work->tcbManager);
    switch (work->seq) {
    case SEQ_MAIN:
        work->seq = BeaconView_SeqMain(work);
        break;
    case SEQ_NEW_BEACON:
        work->seq = BeaconView_SeqNewBeacon(work);
        break;
    case SEQ_WAIT_TASKS:
        work->seq = BeaconView_SeqWaitTasks(work);
        break;
    case SEQ_SCROLL:
        work->seq = BeaconView_SeqScroll(work);
        break;
    case SEQ_BAR:
        work->seq = BeaconView_SeqBar(work);
        break;
    case SEQ_THANKS:
        work->seq = BeaconView_SeqThanks(work);
        break;
    case 6:
    case 7:
        break;
    }
    GameBeaconInfoTable_TakeNew(work->newTable);
}

BOOL BeaconView_Draw(BeaconViewWork *work) {
    return PrintQueue_Main(work->printQueue);
}

GameEvent *BeaconView_CheckEvent(BeaconViewWork *work, BOOL enabled) {
    return BeaconView_CreateEvent(work, enabled);
}

static int BeaconView_SeqMain(BeaconViewWork *work) {
    int seq;

    if (BeaconView_CheckGreeting(work)) {
        work->wait = MESSAGE_WAIT;
        return SEQ_WAIT_TASKS;
    }
    if (BeaconView_CheckLeave(work)) {
        return SEQ_MAIN;
    }
    if (BeaconView_CheckMission(work)) {
        work->wait = MESSAGE_WAIT;
        return SEQ_WAIT_TASKS;
    }
    seq = BeaconView_CheckTouch(work);
    if (seq != SEQ_MAIN) {
        return seq;
    }
    if (work->wait != 0) {
        work->wait--;
        return SEQ_MAIN;
    }
    if (BeaconView_CheckPassPower(work) == TRUE) {
        work->wait = MESSAGE_WAIT;
        return SEQ_WAIT_TASKS;
    }
    if (!BeaconView_TakeNewBeacon(work)) {
        return SEQ_MAIN;
    }
    work->wait = MESSAGE_WAIT;
    return SEQ_NEW_BEACON;
}

static int BeaconView_SeqNewBeacon(BeaconViewWork *work) {
    if (!BeaconView_ShowNewBeacon(work)) {
        return SEQ_NEW_BEACON;
    }
    work->subSeq = 0;
    return SEQ_WAIT_TASKS;
}

static int BeaconView_SeqWaitTasks(BeaconViewWork *work) {
    if (work->pending != 0) {
        return SEQ_WAIT_TASKS;
    }
    BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 0);
    BeaconView_ResetArrows(work);
    return SEQ_MAIN;
}

// Scrolls on, a line at a time, for as long as the arrow is held
static int BeaconView_SeqScroll(BeaconViewWork *work) {
    if (!work->scrollEnd) {
        int dir = BeaconView_GetHeldArrow(work);

        if (dir != work->scrollDir || dir == TOUCH_RECT_NONE) {
            work->scrollEnd = TRUE;
        }
        work->scrollFrames++;
    }
    if (work->scrollEnd) {
        if (work->pending > 0) {
            return SEQ_SCROLL;
        }
        work->scrollEnd = FALSE;
        work->scrollFrames = 0;
        work->scrolling = FALSE;
        if (work->scrollReturnSeq == SEQ_MAIN) {
            BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 0);
        }
        BeaconView_ResetArrows(work);
        BeaconView_SetPrintTimeLimit(work->printQueue, FALSE);
        return work->scrollReturnSeq;
    }
    if (work->pending == 0) {
        BeaconView_ScrollOn(work);
    }
    return SEQ_SCROLL;
}

static int BeaconView_SeqBar(BeaconViewWork *work) {
    return BeaconView_UpdateBarDrag(work);
}

static int BeaconView_SeqThanks(BeaconViewWork *work) {
    return BeaconView_UpdateThanks(work);
}

// Fades the view when the touch screen shows it without it being in use
static void BeaconView_SetDimmed(BeaconViewWork *work, BOOL dimmed) {
    if (dimmed) {
        PaletteFade_BlendBuffer(work->paletteFade, 1, 0, 0xe0, 6, 0);
        PaletteFade_BlendBuffer(work->paletteFade, 3, 0, 0xe0, 6, 0);
        FieldDispControl_ReqSetAlphaB(work->dispControl, 5, 0x1e, 10, 6);
    } else {
        PaletteFade_BlendBuffer(work->paletteFade, 1, 0, 0xe0, 0, 0);
        PaletteFade_BlendBuffer(work->paletteFade, 3, 0, 0xe0, 0, 0);
        FieldDispControl_ReqSetAlphaB(work->dispControl, 4, 0x1e, 13, 3);
    }
}

static void BeaconView_DrawAll(BeaconViewWork *work) {
    int i;

    PaletteFade_SetTransferAll(work->paletteFade, TRUE);
    BeaconView_PrintCount(work);
    BeaconView_PrintTotal(work, GameBeacon_Text_Total, NULL);
    for (i = 0; i < work->viewCount; i++) {
        u8 top = work->top;
        int dataIndex;
        BeaconViewPanel *panel;

        GameBeaconInfoTable_RingGet(work->log, work->beacon, &work->time, top + i);
        dataIndex = GameBeaconInfoTable_RingIndex(work->log, top + i);
        panel = BeaconView_FindPanel(work, 0xff);
        BeaconViewPanel_Assign(panel, dataIndex, i + 1);
        BeaconViewPanel_SetBeacon(work, panel, work->beacon, 2, FALSE, NULL);
        BeaconViewPanel_SetVisible(panel, TRUE);
    }
    BeaconView_UpdateArrows(work);
    BeaconView_UpdateThanksButton(work);
    BeaconView_UpdateBar(work);
    ClActor_SetVisible(work->actors[BEACON_VIEW_ACTOR_BAR], work->logCount >= BEACON_VIEW_LINE_MAX + 1 ? TRUE : FALSE);
    if (work->passPower == PASS_POWER_NONE) {
        BeaconView_SetActorAnim(work->actors[BEACON_VIEW_ACTOR_PASS_POWER], 3);
    }
    GFL_NetSetIconScreen(FALSE, work->tmpHeapId);
}

// What a touch does, and the sequence it gives
static int BeaconView_CheckTouch(BeaconViewWork *work) {
    BeaconViewTouch touch;
    int hit;

    if (!GFL_HIDGetTouchPressedPoint((u32 *)&touch.x, (u32 *)&touch.y)) {
        return SEQ_MAIN;
    }
    hit = BeaconView_HitTestButton(work, &touch);
    if (hit != TOUCH_RECT_NONE) {
        switch (hit) {
        case BEACON_VIEW_ACTOR_PASS_POWER:
            BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_PASS_POWER);
            break;
        case BEACON_VIEW_ACTOR_GREETING:
            BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_GREETING);
            break;
        case BEACON_VIEW_ACTOR_THANKS:
            return SEQ_THANKS;
        case BEACON_VIEW_ACTOR_RETURN:
            BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_MENU);
            break;
        }
        return SEQ_MAIN;
    }
    hit = BeaconView_HitTestMedal(work, &touch);
    if (hit != TOUCH_RECT_NONE) {
        u8 dataIndex = GameBeaconInfoTable_RingIndex(work->log, work->top + hit);
        BeaconViewPanel *panel;

        GameBeaconInfoTable_RingGet(work->log, work->beacon, &work->time, work->top + hit);
        if (GameBeacon_IsGiftType(work->beacon)) {
            return SEQ_MAIN;
        }
        panel = BeaconView_FindPanel(work, dataIndex);
        if (BeaconViewPanel_HasNoMedals(panel)) {
            return SEQ_MAIN;
        }
        work->selected = hit;
        work->selectedPanel = panel->id;
        BeaconView_PlaySE(SEQ_SE_DECIDE1);
        BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_MEDAL);
        return SEQ_MAIN;
    }
    hit = BeaconView_HitTestPanel(work, &touch);
    if (hit != TOUCH_RECT_NONE) {
        GameBeaconInfoTable_RingGet(work->log, work->beacon, &work->time, work->top + hit);
        if (GameBeacon_IsGiftType(work->beacon)) {
            return SEQ_MAIN;
        }
        work->selected = hit;
        BeaconView_FlashSelected(work, hit);
        BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_DETAIL);
        return SEQ_MAIN;
    }
    hit = BeaconView_HitTestArrow(work, &touch);
    if (hit != TOUCH_RECT_NONE) {
        BeaconView_StartScroll(work, hit, SEQ_MAIN);
        return SEQ_SCROLL;
    }
    if (BeaconView_HitTestBar(work, &touch) != TOUCH_RECT_NONE) {
        work->barReturnSeq = SEQ_MAIN;
        return SEQ_BAR;
    }
    return SEQ_MAIN;
}

static int BeaconView_GetHeldArrow(BeaconViewWork *work) {
    BeaconViewTouch touch;

    if (GFL_HIDGetTouchHeldPoint((u32 *)&touch.x, (u32 *)&touch.y)) {
        return BeaconView_HitTestArrow(work, &touch);
    }
    return TOUCH_RECT_NONE;
}

static int BeaconView_UpdateBarDrag(BeaconViewWork *work) {
    BeaconViewTouch touch;

    if (work->barDragging) {
        if (!GFL_HIDGetTouchHeldPoint((u32 *)&touch.x, (u32 *)&touch.y)) {
            work->barDragging = FALSE;
            return SEQ_BAR;
        }
        if (touch.x >= 232 && touch.x <= 248) {
            BeaconView_DragBar(work, touch.y);
        }
    }
    if (work->pending != 0) {
        return SEQ_BAR;
    }
    if (work->barTarget == work->top) {
        if (!work->barDragging) {
            if (work->barReturnSeq == SEQ_MAIN) {
                BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 0);
            } else {
                BeaconView_SetButtonAnim(work, BEACON_VIEW_ACTOR_THANKS, 0);
            }
            return work->barReturnSeq;
        }
    } else {
        BeaconView_StartJump(work, work->barTarget);
    }
    return SEQ_BAR;
}

// Leaves the view for the menu when something else needs the touch screen
static BOOL BeaconView_CheckLeave(BeaconViewWork *work) {
    u32 state = func_ov012_02153650(work->commSys);

    if (GSYS_IsLowBatteryCGearOff(work->gsys)) {
        BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_MENU);
        return TRUE;
    }
    if (state == 2) {
        BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_MENU_COMM);
        return TRUE;
    }
    if (WihDwc_GetStatus(work->wifiList) & 8) {
        BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_MENU_WIFI);
        return TRUE;
    }
    return FALSE;
}

// Shows the player's new greeting once it has been entered
static BOOL BeaconView_CheckGreeting(BeaconViewWork *work) {
    u16 *changed = (u16 *)BeaconStatus_GetGreetingChanged(work->status);

    if (*changed == FALSE) {
        return FALSE;
    }
    *changed = FALSE;
    func_0202437c(work->wordSet, 0, BeaconStatus_GetGreeting(work->status), work->gender, 1, 2);
    BeaconView_ShowMessage(work, GameBeacon_Text_Said);
    return TRUE;
}

// Offers a Pass Power that a beacon gave, or that a record gives
static BOOL BeaconView_CheckPassPower(BeaconViewWork *work) {
    u32 passPower = BeaconStatus_GetGivenPassPower(work->status, work->strTemplate);

    if (passPower != PASS_POWER_NONE) {
        if (passPower == PassPower_GetUsedIDByPassPower(passPower, work->passPowerData)) {
            BeaconStatus_ClearGivenPassPower(work->status);
            return FALSE;
        }
        func_0202437c(work->wordSet, 0, work->strTemplate, 0, 1, 2);
        BeaconView_OfferPassPower(work, passPower, FALSE, &work->pending);
        return TRUE;
    }
    passPower = BeaconView_GetRecordPassPower(work);
    if (passPower == PASS_POWER_NONE) {
        return FALSE;
    }
    if (passPower == PassPower_GetUsedIDByPassPower(passPower, work->passPowerData)) {
        BeaconStatus_PopRecord(work->status);
        return FALSE;
    }
    BeaconView_OfferRecordPassPower(work, passPower);
    return TRUE;
}

// Offers a Funfest mission that another player is running
static BOOL BeaconView_CheckMission(BeaconViewWork *work) {
    FestMissionConfig config;

    if (getStatusOfFesMission(work->festival)) {
        return FALSE;
    }
    if (!BeaconView_GetMissionOffer(work->festival, &config, work->strTemplate)) {
        return FALSE;
    }
    func_0202437c(work->wordSet, 0, work->strTemplate, 0, 1, 2);
    FestMission_GetMessage(work->festText, work->strTemplate, (const FestMission *)&config, work->tmpHeapId);
    func_0202437c(work->wordSet, 1, work->strTemplate, 0, 1, 2);
    BeaconView_OfferMission(work, &config, &work->pending);
    return TRUE;
}

// Takes the next beacon received into the log
static BOOL BeaconView_TakeNewBeacon(BeaconViewWork *work) {
    if (!GameBeaconInfoTable_PopFront(work->newTable, work->newBeacon, &work->newTime)) {
        BeaconView_UpdateNameColors(work);
        return FALSE;
    }
    if (GameBeacon_IsInvalid(work->newBeacon)) {
        return FALSE;
    }
    work->newOffset = GameBeaconInfoTable_RingAdd(work->log, work->newBeacon, work->newTime, &work->isNew);
    work->newIndex = GameBeaconInfoTable_RingIndex(work->log, work->newOffset);
    if (!work->isNew) {
        work->newPanel = BeaconView_FindPanel(work, work->newIndex);
    } else {
        work->newPanel = NULL;
        work->passerbyCount = TrainerGameInfo_IncPasserbyCount((TrainerGameInfoSave *)work->trainerCard);
    }
    if (work->newPanel != NULL && work->newPanel->printTask != NULL) {
        work->subSeq = 0;
    } else {
        work->subSeq = 1;
    }
    work->oldCount = work->logCount;
    if (work->logCount < GAME_BEACON_LOG_MAX) {
        work->logCount += (u8)work->isNew;
        if (work->isNew && work->logCount >= BEACON_VIEW_LINE_MAX + 1) {
            ClActor_SetVisible(work->actors[BEACON_VIEW_ACTOR_BAR], TRUE);
        }
        if (work->logCount <= BEACON_VIEW_LINE_MAX) {
            work->viewCount = work->logCount;
        }
    }
    return TRUE;
}

static void BeaconView_ScrollOn(BeaconViewWork *work) {
    BeaconView_ScrollLine(work, work->scrollDir, 4);
    BeaconView_UpdateBar(work);
}

void BeaconView_SetButtonAnim(BeaconViewWork *work, int button, int state) {
    u8 buttons;

    if (button == BEACON_VIEW_BUTTON_ALL) {
        buttons = 0xf;
    } else {
        buttons = 1 << button;
    }
    if ((buttons & (1 << BEACON_VIEW_ACTOR_PASS_POWER)) && work->passPower != PASS_POWER_NONE) {
        BeaconView_SetActorAnim(work->actors[BEACON_VIEW_ACTOR_PASS_POWER], (u8)(state + 1));
    }
    if (buttons & (1 << BEACON_VIEW_ACTOR_GREETING)) {
        BeaconView_SetActorAnim(work->actors[BEACON_VIEW_ACTOR_GREETING], (u8)(state + 4));
    }
    if ((buttons & (1 << BEACON_VIEW_ACTOR_THANKS)) && work->logCount != 0) {
        BeaconView_SetActorAnim(work->actors[BEACON_VIEW_ACTOR_THANKS], (u8)(state + 7));
    }
    if (buttons & (1 << BEACON_VIEW_ACTOR_RETURN)) {
        BeaconView_SetActorAnim(work->actors[BEACON_VIEW_ACTOR_RETURN], (u8)(state + 10));
    }
}

BOOL BeaconView_IsButtonAnimating(BeaconViewWork *work, int button) {
    return ClActor_IsAnimActive(work->actors[button]);
}

// Shows a beacon just taken: its message, then its line, which pushes the others down when the player is new
static BOOL BeaconView_ShowNewBeacon(BeaconViewWork *work) {
    switch (work->subSeq) {
    case 0:
        if (work->newPanel->printTask != NULL) {
            break;
        }
        work->subSeq++;
        break;
    case 1: {
        BOOL message = BeaconView_ShowBeaconMessage(work, work->newBeacon, work->isNew);

        if (!work->isNew) {
            if (message) {
                BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 2);
                BeaconView_PlaySE(SEQ_SE_SELECT2);
            }
            if (work->newPanel != NULL) {
                BeaconViewPanel_SetBeacon(work, work->newPanel, work->newBeacon, 0, message ? FALSE : TRUE, NULL);
                BeaconViewPanel_SetVisible(work->newPanel, TRUE);
            }
            return TRUE;
        }
        BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 2);
        work->subSeq++;
        break;
    }
    case 2:
        BeaconView_PrintCount(work);
        BeaconView_PrintTotal(work, GameBeacon_Text_Total, &work->pending);
        BeaconView_CheckRecords(work, work->newBeacon, work->oldCount);
        work->subSeq++;
        break;
    case 3: {
        BOOL atTop = TRUE;

        if (work->top != 0) {
            work->newOffset = work->top;
            GameBeaconInfoTable_RingGet(work->log, work->newBeacon, &work->newTime, work->newOffset);
            work->newIndex = GameBeaconInfoTable_RingIndex(work->log, work->newOffset);
            atTop = FALSE;
        }
        work->top = work->newOffset;
        BeaconView_AddLine(work, work->newBeacon, work->newIndex, 1, 8, (atTop ? 0 : 1) + 1);
        BeaconView_UpdateBar(work);
        BeaconView_PlaySE(SEQ_SE_SYS_11);
        return TRUE;
    }
    }
    return FALSE;
}

// The player thanks one of the players of the log by touching its line
static int BeaconView_UpdateThanks(BeaconViewWork *work) {
    if (work->pending != 0) {
        return SEQ_THANKS;
    }
    switch (work->subSeq) {
    case THANKS_START:
        BeaconView_SetButtonAnim(work, BEACON_VIEW_ACTOR_THANKS, 1);
        BeaconView_PrintTotal(work, GameBeacon_Text_WhoWillThank, &work->pending);
        work->subSeq++;
        break;
    case THANKS_WAIT_BUTTON:
        if (BeaconView_IsButtonAnimating(work, BEACON_VIEW_ACTOR_THANKS)) {
            break;
        }
        BeaconView_SetButtonAnim(work, BEACON_VIEW_ACTOR_PASS_POWER, 2);
        BeaconView_SetButtonAnim(work, BEACON_VIEW_ACTOR_GREETING, 2);
        BeaconView_SetButtonAnim(work, BEACON_VIEW_ACTOR_RETURN, 2);
        work->subSeq++;
        break;
    case THANKS_INPUT:
        work->subSeq = BeaconView_CheckThanksTouch(work);
        if (work->subSeq == THANKS_SCROLL_END) {
            return SEQ_SCROLL;
        }
        if (work->subSeq == THANKS_BAR) {
            work->subSeq = THANKS_INPUT;
            return SEQ_BAR;
        }
        break;
    case THANKS_SEND: {
        u8 dataIndex = GameBeaconInfoTable_RingIndex(work->log, work->selected + work->top);
        BeaconViewPanel *panel = BeaconView_FindPanel(work, dataIndex);

        func_0202437c(work->wordSet, 0, panel->name, panel->gender, 1, 2);
        BeaconView_SetPrintTimeLimit(work->printQueue, TRUE);
        BeaconView_ShowMessage(work, GameBeacon_Text_Thanked);
        GameBeacon_BroadcastThanks(work->gameData, panel->trainerId);
        work->subSeq = THANKS_SENT;
        break;
    }
    case THANKS_SENT:
        BeaconView_SetPrintTimeLimit(work->printQueue, FALSE);
        work->subSeq = THANKS_END;
        break;
    case THANKS_SCROLL_END:
        BeaconView_SetButtonAnim(work, BEACON_VIEW_ACTOR_THANKS, 0);
        work->subSeq = THANKS_INPUT;
        break;
    case THANKS_END:
        BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 0);
        BeaconView_PrintTotal(work, GameBeacon_Text_Total, &work->pending);
        work->subSeq = THANKS_EXIT;
        break;
    case THANKS_EXIT:
        work->subSeq = 0;
        return SEQ_MAIN;
    }
    return SEQ_THANKS;
}

static int BeaconView_CheckThanksTouch(BeaconViewWork *work) {
    BeaconViewTouch touch;
    int hit;

    if (GSYS_IsLowBatteryCGearOff(work->gsys)) {
        BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_MENU);
        return THANKS_END;
    }
    if (!GFL_HIDGetTouchPressedPoint((u32 *)&touch.x, (u32 *)&touch.y)) {
        return THANKS_INPUT;
    }
    if (BeaconView_HitTestButton(work, &touch) == BEACON_VIEW_ACTOR_THANKS) {
        return THANKS_END;
    }
    hit = BeaconView_HitTestPanel(work, &touch);
    if (hit != TOUCH_RECT_NONE) {
        work->selected = hit;
        BeaconView_FlashSelected(work, hit);
        return THANKS_SEND;
    }
    hit = BeaconView_HitTestArrow(work, &touch);
    if (hit != TOUCH_RECT_NONE) {
        BeaconView_StartScroll(work, hit, SEQ_THANKS);
        return THANKS_SCROLL_END;
    }
    if (BeaconView_HitTestBar(work, &touch) != TOUCH_RECT_NONE) {
        work->barReturnSeq = SEQ_THANKS;
        return THANKS_BAR;
    }
    return THANKS_INPUT;
}

static void BeaconView_ResetArrows(BeaconViewWork *work) {
    BeaconView_UpdateArrows(work);
}
