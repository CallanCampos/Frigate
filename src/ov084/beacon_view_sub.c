#include "types.h"
#include "constants/sound.h"
#include "constants/version.h"
#include "field/beacon_view.h"
#include "field/beacon_view_common.h"
#include "field/festival.h"
#include "field/game_beacon_set.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/mi.h"
#include "save/event_work.h"
#include "save/high_link.h"
#include "save/trainer_game_info.h"
#include "system/aeabi.h"
#include "system/beacon_status.h"
#include "system/bmp_oam.h"
#include "system/fest_mission.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/scroll_bar.h"
#include "system/wordset.h"
#include "text/system/game_beacon.h"

// The beacon view's parts: where the screen is touched, its text, the log's lines and how they move, and the tasks
// that show a message, a Pass Power or a Funfest mission over the log. The ROM has no name for this file;
// "beacon_view_sub.c" is a guess

#define PANEL_X 8
#define PANEL_HEIGHT 40
// A line's y: line 1 is the top of the screen
#define PANEL_LINE_Y(line) (-PANEL_HEIGHT * (1 - (line)))

#define BAR_X 232
#define BAR_TOP 44
#define BAR_BOTTOM 132

// How long a beacon's name stays in the fresh color, in seconds
#define BEACON_FRESH_SECONDS 10800

typedef struct {
    u8 seq;
    u8 dir;
    u8 frames;
    int mode;
    BeaconViewPanel *panel;
    BeaconViewWork *work;
    int printing;
    int *pending;
} BeaconViewAddLineTask;

typedef struct {
    int seq;
    BeaconViewWork *work;
    BeaconViewPanel *panel;
    u8 dir;
    u8 delay;
    u8 frames;
    u8 frame;
    s16 destX;
    s16 destY;
    fx32 x;
    fx32 y;
    fx32 stepX;
    fx32 stepY;
    int *pending;
} BeaconViewMoveTask;

typedef struct {
    BeaconViewWork *work;
    BeaconViewPanel *panels[BEACON_VIEW_LINE_MAX];
    int seq;
    u8 top;
    u8 count;
    int printing;
    int *pending;
} BeaconViewJumpTask;

typedef struct {
    u8 seq;
    u8 timer;
    int printing;
    BeaconViewWork *work;
    int *pending;
} BeaconViewMessageTask;

typedef struct {
    u8 seq;
    u8 unk1;
    u8 timer;
    u8 record;
    u8 unk4;
    u8 button;
    int printing;
    u32 passPower;
    BeaconViewWork *work;
    int *pending;
} BeaconViewPassPowerTask;

typedef struct {
    u8 seq;
    u8 unk1;
    u8 timer;
    u8 unk3[2];
    u8 button;
    int printing;
    FestMissionConfig config;
    u32 unk38[4];
    BeaconViewWork *work;
    int *pending;
} BeaconViewMissionTask;

typedef struct {
    u8 seq;
    u8 timer;
    u32 unk4;
    BeaconViewPanel *panel;
    int type;
    BeaconViewWork *work;
    u32 unk14;
} BeaconViewIconTask;

typedef struct {
    u8 seq;
    u8 frame;
    u8 frames;
    u8 offset;
    u32 unk4;
    BeaconViewPanel *panel;
    BeaconViewWork *work;
    int *pending;
} BeaconViewFlashTask;

typedef struct {
    BeaconViewPanel *panel;
    BeaconViewOam *oam;
    BeaconViewWork *work;
    int *pending;
} BeaconViewPrintTask;

static BOOL BeaconView_IsRepdigit(u32 number);
static void BeaconView_ExpandMessage(BeaconViewWork *work, u8 messageId);
static void BeaconView_PrintMessage(BeaconViewWork *work, const StrBuf *strbuf, int *pending);
static void BeaconView_ClearTotal(BeaconViewWork *work);
static void BeaconView_SetButtonsVisible(BeaconViewWork *work, BOOL visible);
static void BeaconView_SetBarPos(BeaconViewWork *work, int y);
static void BeaconViewPanel_SetIconVisible(BeaconViewPanel *panel, BOOL visible);
static void BeaconViewPanel_SetMedalVisible(BeaconViewPanel *panel, BOOL visible);
static void BeaconViewPanel_Clear(BeaconViewPanel *panel);
static void BeaconViewPanel_Reassign(BeaconViewPanel *panel, u8 dataIndex, u8 line);
static void BeaconViewPanel_SetTrainer(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon);
static void BeaconViewPanel_SetIcon(BeaconViewWork *work, BeaconViewPanel *panel, u8 icon);
static u8 BeaconViewPanel_GetColorSet(BeaconViewPanel *panel);
static void BeaconViewPanel_SetColors(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon);
static void BeaconViewPanel_PrintText(BeaconViewWork *work, BeaconViewPanel *panel, const StrBuf *strbuf, int *pending);
static void BeaconViewPanel_PrintName(BeaconViewWork *work, BeaconViewPanel *panel, BOOL force, int *pending);
static void BeaconViewPanel_SetRank(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon);
static void BeaconViewPanel_SetMedals(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon);
static void BeaconViewPanel_SetPos(BeaconViewPanel *panel, s16 x, s16 y);
static void BeaconView_AddLineTask(TCBEx *task, void *data);
static void BeaconView_MoveLines(BeaconViewWork *work, u8 dir, u8 frames, BeaconViewPanel *newPanel,
                                 BeaconViewPanel *skip, int *pending);
static void BeaconView_SlideInLine(BeaconViewWork *work, BeaconViewPanel *panel, int *pending);
static void BeaconViewPanel_StartMove(BeaconViewWork *work, BeaconViewPanel *panel, u8 dir, u8 delay, u8 frames,
                                      int *pending);
static void BeaconViewPanel_MoveTask(TCBEx *task, void *data);
static void BeaconView_JumpTask(TCBEx *task, void *data);
static void BeaconView_ShowTypeMessage(BeaconViewWork *work, const GameBeacon *beacon);
static void BeaconView_StartMessage(BeaconViewWork *work, const StrBuf *strbuf, int *pending);
static void BeaconView_MessageTask(TCBEx *task, void *data);
static BOOL BeaconView_TakePassPower(BeaconViewWork *work, const GameBeacon *beacon, BOOL gift);
static void BeaconView_PassPowerTask(TCBEx *task, void *data);
static void BeaconView_MissionTask(TCBEx *task, void *data);
static u32 BeaconView_CheckMissionRefusal(BeaconViewWork *work, FestMissionConfig *config, int button);
static void BeaconViewPanel_StartIcon(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon,
                                      BOOL greeting);
static void BeaconViewPanel_IconTask(TCBEx *task, void *data);
static void BeaconViewPanel_StartFlash(BeaconViewWork *work, BeaconViewPanel *panel, int *pending);
static void BeaconViewPanel_FlashTask(TCBEx *task, void *data);
static void BeaconViewPanel_Print(BeaconViewWork *work, BeaconViewPanel *panel, const StrBuf *strbuf, u16 color,
                                  int *pending);
static void BeaconViewPanel_StartPrintTask(BeaconViewWork *work, BeaconViewPanel *panel, int *pending);
static void BeaconViewPanel_PrintTask(TCBEx *task, void *data);

// The message of the offer of a Pass Power, by whether a record gives it
static const u8 sBeaconViewPassPowerMessages[] = {
    GameBeacon_Text_UseActivated,
    GameBeacon_Text_SpecialPassPowerActivated,
};

static const u16 sBeaconViewButtonSounds[] = {
    SEQ_SE_SYS_79,
    SEQ_SE_SYS_79,
    SEQ_SE_SYS_79,
    SEQ_SE_CANCEL1,
};

// The colors of a line's text by its color set, and after them those of an old beacon's name
static const u16 sBeaconViewTextColors[] = { 0x10a0, 0x0440, 0x14c0, 0x0860 };

static const u16 sBeaconViewDigitDivisors[] = { 1, 10, 100, 1000 };

static const TouchRect sBeaconViewArrowRects[] = {
    { 24, 40, 232, 248 },
    { 144, 160, 232, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The palette of each trainer sprite
static const u8 sBeaconViewTrainerPalettes[BEACON_VIEW_TRAINER_CHARS] = {
    2, 3, 6, 5, 4, 5, 2, 0, 1, 3, 6, 5, 4, 7, 7, 0,
};

// The Pass Power that each record gives
static const u32 sBeaconViewRecordPassPowers[] = { 0x21, 0x23, 0x22, 0x24, 0x25, 0x26 };

// Counts the players of the log of the other gender, and of another version
void BeaconView_CountOthers(BeaconViewWork *work, u8 *genderCount, u8 *versionCount) {
    int i;
    u16 time;

    *genderCount = 0;
    *versionCount = 0;
    for (i = 0; i < work->logCount; i++) {
        GameBeaconInfoTable_RingGet(work->log, work->countBeacon, &time, i);
        if (work->gender != GameBeacon_GetGender(work->countBeacon)) {
            (*genderCount)++;
        }
        if (work->version != GameBeacon_GetVersion(work->countBeacon)) {
            (*versionCount)++;
        }
    }
}

// Notes the records that a new player's beacon made the player reach
void BeaconView_CheckRecords(BeaconViewWork *work, const GameBeacon *beacon, u8 oldCount) {
    u8 versionCount = 0;
    u8 genderCount = 0;

    BeaconView_CountOthers(work, &genderCount, &versionCount);
    if (GameBeacon_GetLanguage(beacon) != 2) {
        BeaconStatus_AddRecord(work->status, 1);
    }
    if (work->otherVersionCount != versionCount && versionCount == 20) {
        BeaconStatus_AddRecord(work->status, 2);
    }
    if (work->passerbyCount % 100 == 0) {
        BeaconStatus_AddRecord(work->status, 3);
    }
    if (oldCount == GAME_BEACON_LOG_MAX - 1 && work->logCount == GAME_BEACON_LOG_MAX) {
        BeaconStatus_AddRecord(work->status, 4);
    }
    if (work->otherGenderCount != genderCount && genderCount == 20) {
        BeaconStatus_AddRecord(work->status, 5);
    }
    if (BeaconView_IsRepdigit(work->passerbyCount)) {
        BeaconStatus_AddRecord(work->status, 6);
    }
    work->otherGenderCount = genderCount;
    work->otherVersionCount = versionCount;
}

u32 BeaconView_GetRecordPassPower(BeaconViewWork *work) {
    u8 record = BeaconStatus_GetRecord(work->status);

    if (record == 0) {
        return PASS_POWER_NONE;
    }
    return sBeaconViewRecordPassPowers[record - 1];
}

// Whether a number of three digits or more has the same digit all through
static BOOL BeaconView_IsRepdigit(u32 number) {
    u32 digit;

    if (number / 100 == 0) {
        return FALSE;
    }
    digit = number % 10;
    number /= 10;
    do {
        u8 next = number % 10;

        if (next != digit) {
            return FALSE;
        }
        number /= 10;
    } while (number != 0);
    return TRUE;
}

// The line touched, or TOUCH_RECT_NONE
int BeaconView_HitTestPanel(BeaconViewWork *work, BeaconViewTouch *touch) {
    int i;
    TouchRect rects[BEACON_VIEW_LINE_MAX + 2];
    int hit;

    rects[0].top = 152;
    rects[0].bottom = 168;
    rects[0].left = 0;
    rects[0].right = 56;
    for (i = 0; i < work->viewCount; i++) {
        rects[i + 1].top = i * PANEL_HEIGHT;
        rects[i + 1].bottom = rects[i + 1].top + PANEL_HEIGHT;
        rects[i + 1].left = 8;
        rects[i + 1].right = 209;
    }
    rects[work->viewCount + 1].top = TOUCH_RECT_END;
    hit = GFL_HIDGetTouchRectAt(rects, touch->x, touch->y);
    if (hit == TOUCH_RECT_NONE || hit == 0) {
        return TOUCH_RECT_NONE;
    }
    return hit - 1;
}

// The line whose medal is touched, or TOUCH_RECT_NONE
int BeaconView_HitTestMedal(BeaconViewWork *work, BeaconViewTouch *touch) {
    int i;
    TouchRect rects[BEACON_VIEW_LINE_MAX + 1];

    // Circles: their x, y and radius
    for (i = 0; i < work->viewCount; i++) {
        rects[i].top = TOUCH_RECT_CIRCLE;
        rects[i].bottom = 208;
        rects[i].left = i * PANEL_HEIGHT + 16;
        rects[i].right = 16;
    }
    rects[work->viewCount].top = TOUCH_RECT_END;
    return GFL_HIDGetTouchRectAt(rects, touch->x, touch->y);
}

// 1 for the up arrow and 0 for the down arrow when the log can scroll that way, or TOUCH_RECT_NONE
int BeaconView_HitTestArrow(BeaconViewWork *work, BeaconViewTouch *touch) {
    int hit = GFL_HIDGetTouchRectAt(sBeaconViewArrowRects, touch->x, touch->y);

    if (hit == 0 && work->top != 0) {
        return 1;
    }
    if (hit == 1 && work->top + work->viewCount < work->logCount) {
        return 0;
    }
    return TOUCH_RECT_NONE;
}

// Starts dragging the scroll bar when it is touched, and returns 0, or TOUCH_RECT_NONE
int BeaconView_HitTestBar(BeaconViewWork *work, BeaconViewTouch *touch) {
    if (work->logCount < BEACON_VIEW_LINE_MAX + 1) {
        return TOUCH_RECT_NONE;
    }
    if (touch->x < BAR_X || touch->x > BAR_X + 16 || touch->y < BAR_TOP || touch->y > BAR_BOTTOM + 8) {
        return TOUCH_RECT_NONE;
    }
    work->barTarget = work->top;
    work->barDragging = TRUE;
    if (BeaconView_DragBar(work, touch->y) == TRUE) {
        BeaconView_StartJump(work, work->barTarget);
    }
    BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 2);
    return 0;
}

// The button touched among those that are on, or TOUCH_RECT_NONE
int BeaconView_HitTestButton(BeaconViewWork *work, BeaconViewTouch *touch) {
    int i;
    TouchRect rects[2];

    rects[1].top = TOUCH_RECT_END;
    for (i = 0; i < BEACON_VIEW_ACTOR_UP; i++) {
        u8 state = ClActor_GetSequence(work->actors[i]) - (i * 3 + 1);

        if (state % 3 < 2) {
            rects[0].top = 172;
            rects[0].bottom = rects[0].top + 24;
            rects[0].left = i * 32 + 132;
            rects[0].right = rects[0].left + 24;
            if (GFL_HIDGetTouchRectAt(rects, touch->x, touch->y) != TOUCH_RECT_NONE) {
                BeaconView_PlaySE(sBeaconViewButtonSounds[i]);
                return i;
            }
        }
    }
    return TOUCH_RECT_NONE;
}

static void BeaconView_ExpandMessage(BeaconViewWork *work, u8 messageId) {
    GFL_MsgDataLoadStrbuf(work->msgData, messageId, work->strTemplate);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strExpand, work->strTemplate);
}

static void BeaconView_PrintMessage(BeaconViewWork *work, const StrBuf *strbuf, int *pending) {
    BeaconViewWin_Print(&work->wins[BEACON_VIEW_WIN_MESSAGE].win, strbuf, 0, 0, 9, 0x449, pending, work->printNow);
}

void BeaconView_PrintCount(BeaconViewWork *work) {
    WordSetNumber(work->wordSet, 0, work->logCount, 2, 1, TRUE);
    BeaconView_ExpandMessage(work, GameBeacon_Text_Ppl);
    BeaconViewOam_Print(&work->countOam, work->strExpand, 0x10a0, NULL, work->printNow);
}

void BeaconView_PrintTotal(BeaconViewWork *work, u8 messageId, int *pending) {
    WordSetNumber(work->wordSet, 0, work->passerbyCount, 5, 0, TRUE);
    BeaconView_ExpandMessage(work, messageId);
    BeaconViewWin_Print(&work->wins[BEACON_VIEW_WIN_TOTAL].win, work->strExpand, 0, 4, 0, 0x3c40, pending,
                        work->printNow);
}

static void BeaconView_ClearTotal(BeaconViewWork *work) {
    GFL_BitmapFill(work->wins[BEACON_VIEW_WIN_TOTAL].win.bitmap, 0x40);
    BmpWin_FlushChar(work->wins[BEACON_VIEW_WIN_TOTAL].win.window);
}

static void BeaconView_SetButtonsVisible(BeaconViewWork *work, BOOL visible) {
    ClActor_SetVisible(work->actors[BEACON_VIEW_ACTOR_PASS_POWER], visible);
    ClActor_SetVisible(work->actors[BEACON_VIEW_ACTOR_GREETING], visible);
    ClActor_SetVisible(work->actors[BEACON_VIEW_ACTOR_RETURN], visible);
    ClActor_SetVisible(work->actors[BEACON_VIEW_ACTOR_THANKS], visible);
}

void BeaconView_UpdateArrows(BeaconViewWork *work) {
    if (work->top == 0) {
        ClActor_SetSequence(work->actors[BEACON_VIEW_ACTOR_UP], 16);
    } else {
        ClActor_SetSequence(work->actors[BEACON_VIEW_ACTOR_UP], 13);
    }
    if (work->top + work->viewCount < work->logCount) {
        ClActor_SetSequence(work->actors[BEACON_VIEW_ACTOR_DOWN], 17);
    } else {
        ClActor_SetSequence(work->actors[BEACON_VIEW_ACTOR_DOWN], 20);
    }
}

static void BeaconView_SetBarPos(BeaconViewWork *work, int y) {
    if (y < BAR_TOP) {
        y = BAR_TOP;
    } else if (y > BAR_BOTTOM) {
        y = BAR_BOTTOM;
    }
    BeaconView_SetActorPos(work->actors[BEACON_VIEW_ACTOR_BAR], BAR_X, y);
}

void BeaconView_UpdateBar(BeaconViewWork *work) {
    BeaconView_SetBarPos(work,
                         ScrollBar_GetPos(work->logCount - BEACON_VIEW_LINE_MAX, work->top, BAR_TOP, BAR_BOTTOM, 0));
}

void BeaconView_UpdateThanksButton(BeaconViewWork *work) {
    if (work->logCount == 0) {
        ClActor_SetSequence(work->actors[BEACON_VIEW_ACTOR_THANKS], 9);
    } else {
        ClActor_SetSequence(work->actors[BEACON_VIEW_ACTOR_THANKS], 7);
    }
}

void BeaconViewPanel_SetVisible(BeaconViewPanel *panel, BOOL visible) {
    BmpOam_ActorSetDrawEnable(panel->oam.actor, visible);
    ClActor_SetVisible(panel->trainer, visible);
    ClActor_SetVisible(panel->base, visible);
    if (!visible || panel->unkA != 0xffff) {
        BeaconViewPanel_SetMedalVisible(panel, visible);
    }
    if (!visible) {
        BeaconViewPanel_SetIconVisible(panel, visible);
    }
}

static void BeaconViewPanel_SetIconVisible(BeaconViewPanel *panel, BOOL visible) {
    ClActor_SetVisible(panel->icon, visible);
}

static void BeaconViewPanel_SetMedalVisible(BeaconViewPanel *panel, BOOL visible) {
    int i;

    ClActor_SetVisible(panel->medal, visible);
    for (i = 0; i < BEACON_VIEW_PANEL_DIGITS; i++) {
        ClActor_SetVisible(panel->digits[i], visible);
    }
}

static void BeaconViewPanel_Clear(BeaconViewPanel *panel) {
    BeaconViewPanel_SetVisible(panel, FALSE);
    BeaconViewPanel_SetIconVisible(panel, FALSE);
    GFL_BitmapFill(panel->oam.bitmap, 0);
    BmpOam_ActorBmpTrans(panel->oam.actor);
    GFL_StrBufClear(panel->name);
    GFL_StrBufClear(panel->greeting);
    panel->dataIndex = 0xff;
    panel->line = 0;
    panel->rank = 0;
    if (panel->printTask != NULL) {
        GFL_TCBExRequestEnd(panel->printTask);
        panel->printTask = NULL;
    }
    if (panel->iconTask != NULL) {
        GFL_TCBExRequestEnd(panel->iconTask);
        panel->iconTask = NULL;
    }
}

void BeaconViewPanel_Assign(BeaconViewPanel *panel, u8 dataIndex, u8 line) {
    BeaconViewPanel_Clear(panel);
    panel->dataIndex = dataIndex;
    panel->line = line;
}

// Gives a line that stays on the screen another log entry
static void BeaconViewPanel_Reassign(BeaconViewPanel *panel, u8 dataIndex, u8 line) {
    if (panel->printTask != NULL) {
        GFL_TCBExRequestEnd(panel->printTask);
        panel->printTask = NULL;
    }
    if (panel->iconTask != NULL) {
        BeaconViewPanel_SetIconVisible(panel, FALSE);
        GFL_TCBExRequestEnd(panel->iconTask);
        panel->iconTask = NULL;
    }
    panel->dataIndex = dataIndex;
    panel->line = line;
}

BeaconViewPanel *BeaconView_FindPanel(BeaconViewWork *work, u8 dataIndex) {
    int i;

    for (i = 0; i < BEACON_VIEW_PANEL_MAX; i++) {
        if (work->panels[i].dataIndex == dataIndex) {
            return &work->panels[i];
        }
    }
    return NULL;
}

static void BeaconViewPanel_SetTrainer(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon) {
    u8 view = GameBeacon_GetTrainerViewWithGender(beacon);

    ClActSys_ReplaceCharData(work->res[BEACON_VIEW_RES_TRAINER].lists[BEACON_VIEW_RES_CHARS].ids[panel->id],
                             work->trainerChars[view].data);
    PaletteFade_LoadData(work->paletteFade, &work->trainerPalette.colors[(u8)(sBeaconViewTrainerPalettes[view] * 16)],
                         3, (panel->id + 9) * 16, 0x20);
}

static void BeaconViewPanel_SetIcon(BeaconViewWork *work, BeaconViewPanel *panel, u8 icon) {
    ClActSys_ReplaceCharData(work->res[BEACON_VIEW_RES_ICON].lists[BEACON_VIEW_RES_CHARS].ids[panel->id],
                             work->iconChars[icon].data);
}

static u8 BeaconViewPanel_GetColorSet(BeaconViewPanel *panel) {
    switch (panel->version) {
    case VERSION_BLACK:
    case VERSION_BLACK2:
        return 0;
    case VERSION_WHITE:
    case VERSION_WHITE2:
        return 1;
    }
    return 0;
}

// Sets the colors of the plate: its color set's, and the player's favorite color with a darker shade
static void BeaconViewPanel_SetColors(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon) {
    u16 colors[7];

    MI_CpuCopy16(&work->commonPalette.colors[panel->colorSet * 16], colors, 5 * sizeof(u16));
    GameBeacon_GetFavoriteColorRGB(&colors[5], beacon);
    BlendColors(&colors[5], &colors[6], 1, 3, 0);
    PaletteFade_LoadData(work->paletteFade, colors, 3, (panel->id + 4) * 16 + 1, sizeof(colors));
}

static void BeaconViewPanel_PrintText(BeaconViewWork *work, BeaconViewPanel *panel, const StrBuf *strbuf,
                                      int *pending) {
    BeaconViewPanel_Print(work, panel, strbuf, sBeaconViewTextColors[panel->colorSet], pending);
}

// Prints the player's name, in the color of an old beacon once it is one
static void BeaconViewPanel_PrintName(BeaconViewWork *work, BeaconViewPanel *panel, BOOL force, int *pending) {
    u8 old;

    if (GameBeaconSys_GetAge(panel->trainerId) >= BEACON_FRESH_SECONDS) {
        old = TRUE;
    } else {
        old = FALSE;
    }
    if (old == panel->old && !force) {
        return;
    }
    panel->old = old;
    if (old) {
        BeaconViewPanel_Print(work, panel, panel->name, sBeaconViewTextColors[panel->colorSet + 2], pending);
    } else {
        BeaconViewPanel_Print(work, panel, panel->name, sBeaconViewTextColors[panel->colorSet], pending);
    }
}

// Sets the colors of the plate's stars by the player's survey rank
static void BeaconViewPanel_SetRank(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon) {
    u8 rank;
    u16 offset;
    u16 litOffset;

    panel->rank = GameBeacon_GetSurveyRank(beacon);
    rank = panel->rank;
    offset = panel->colorSet * 16 + 32;
    if (rank == 0) {
        litOffset = offset;
        rank = 5;
    } else {
        litOffset = offset + 5;
    }
    PaletteFade_LoadData(work->paletteFade, &work->commonPalette.colors[offset], 3, (panel->id + 4) * 16 + 8,
                         5 * sizeof(u16));
    PaletteFade_LoadData(work->paletteFade, &work->commonPalette.colors[litOffset], 3, (panel->id + 4) * 16 + 8,
                         rank * sizeof(u16));
}

BOOL BeaconViewPanel_HasNoMedals(BeaconViewPanel *panel) {
    if (panel->medalCount == 0 || panel->unkA >= 0xff) {
        return TRUE;
    }
    return FALSE;
}

static void BeaconViewPanel_SetMedals(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon) {
    int i;

    // Black and White have no medals
    if (panel->version == VERSION_WHITE || panel->version == VERSION_BLACK) {
        panel->unkA = 0xffff;
        panel->medalCount = 0;
        BeaconViewPanel_SetMedalVisible(panel, FALSE);
        return;
    }
    panel->medalCount = GameBeacon_GetMedalCount(beacon);
    panel->unkA = func_02013dc8(beacon);
    if (BeaconViewPanel_HasNoMedals(panel)) {
        BeaconView_SetActorAnim(panel->medal, 24);
    } else {
        BeaconView_SetActorAnim(panel->medal, 22);
    }
    for (i = 0; i < BEACON_VIEW_PANEL_DIGITS; i++) {
        ClActor_SetAnimFrame(panel->digits[i],
                             panel->medalCount % sBeaconViewDigitDivisors[i + 1] / sBeaconViewDigitDivisors[i]);
    }
}

// Makes the line show a beacon. Unless quiet, it then prints by mode: 0 the beacon's own text with its icon, 1 the
// player's greeting, 2 the player's name
void BeaconViewPanel_SetBeacon(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon, int mode,
                               BOOL quiet, int *pending) {
    panel->trainerId = GameBeacon_GetTrainerId(beacon);
    panel->gender = GameBeacon_GetGender(beacon);
    panel->version = GameBeacon_GetVersion(beacon);
    panel->colorSet = BeaconViewPanel_GetColorSet(panel);
    GameBeacon_GetName(beacon, panel->name);
    GFL_StrBufClear(panel->greeting);
    GameBeacon_GetGreeting(beacon, panel->greeting);
    BeaconViewPanel_SetRank(work, panel, beacon);
    BeaconViewPanel_SetMedals(work, panel, beacon);
    BeaconViewPanel_SetTrainer(work, panel, beacon);
    BeaconViewPanel_SetColors(work, panel, beacon);
    if (quiet) {
        return;
    }
    switch (mode) {
    case 0:
        switch (GameBeacon_GetType(beacon)) {
        case 24:
            GameBeacon_GetMessage(beacon, work->strTemplate);
            BeaconViewPanel_PrintText(work, panel, work->strTemplate, pending);
            break;
        case 49:
            GameBeacon_GetTextMessage(beacon, work->strTemplate);
            BeaconViewPanel_PrintText(work, panel, work->strTemplate, pending);
            break;
        }
        BeaconViewPanel_StartIcon(work, panel, beacon, FALSE);
        break;
    case 1:
        BeaconViewPanel_PrintText(work, panel, panel->greeting, pending);
        BeaconViewPanel_StartIcon(work, panel, beacon, TRUE);
        break;
    case 2:
        BeaconViewPanel_PrintName(work, panel, TRUE, pending);
        break;
    }
}

static void BeaconViewPanel_SetPos(BeaconViewPanel *panel, s16 x, s16 y) {
    int i;

    panel->x = x;
    panel->y = y;
    BeaconView_SetActorPos(panel->base, x, y);
    BeaconView_SetActorPos(panel->trainer, x + 24, y + 24);
    BeaconView_SetActorPos(panel->icon, x + 52, y + 24);
    BeaconView_SetActorPos(panel->medal, x + 188, y + 12);
    for (i = 0; i < BEACON_VIEW_PANEL_DIGITS; i++) {
        BeaconView_SetActorPos(panel->digits[i], x + 200 - i * 8, y + 28);
    }
    x += 72;
    y += 16;
    BmpOam_ActorSetPos(panel->oam.actor, x, y);
}

// Prints the names of the lines again where their beacons have become old
void BeaconView_UpdateNameColors(BeaconViewWork *work) {
    int i;

    for (i = 0; i < work->viewCount; i++) {
        BeaconViewPanel *panel =
            BeaconView_FindPanel(work, (u8)GameBeaconInfoTable_RingIndex(work->log, work->top + i));

        if (panel->iconTask == NULL || panel->printTask != NULL) {
            BeaconViewPanel_PrintName(work, panel, FALSE, NULL);
        }
    }
}

// Scrolls the log by a line: down (to older entries) for 0, up otherwise
void BeaconView_ScrollLine(BeaconViewWork *work, int dir, u8 frames) {
    u8 offset;
    u8 dataIndex;

    BeaconView_PlaySE(SEQ_SE_SELECT4);
    if (dir == 0) {
        offset = work->top + work->viewCount;
        work->top++;
    } else {
        offset = work->top - 1;
        work->top--;
    }
    func_0202d7b8(work->status, work->top);
    GameBeaconInfoTable_RingGet(work->log, work->beacon, &work->time, offset);
    dataIndex = GameBeaconInfoTable_RingIndex(work->log, offset);
    BeaconView_AddLine(work, work->beacon, dataIndex, dir, frames, 2);
}

void BeaconView_StartScroll(BeaconViewWork *work, int dir, u8 returnSeq) {
    work->scrolling = TRUE;
    work->scrollEnd = FALSE;
    work->scrollDir = dir;
    work->scrollReturnSeq = returnSeq;
    work->scrollFrames = 0;
    BeaconView_SetPrintTimeLimit(work->printQueue, TRUE);
    if (work->scrollDir == 0) {
        BeaconView_SetActorAnim(work->actors[BEACON_VIEW_ACTOR_DOWN], 19);
    } else {
        BeaconView_SetActorAnim(work->actors[BEACON_VIEW_ACTOR_UP], 15);
    }
    BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 2);
    BeaconView_ScrollLine(work, dir, 8);
    BeaconView_UpdateBar(work);
}

// Moves the bar to where it is dragged, and returns whether that changes the line the log is to start at
BOOL BeaconView_DragBar(BeaconViewWork *work, int y) {
    int top = ScrollBar_GetValue(work->logCount - BEACON_VIEW_LINE_MAX, y, BAR_TOP, BAR_BOTTOM, 0);

    BeaconView_SetBarPos(work, y);
    if (work->barTarget == top) {
        return FALSE;
    }
    work->barTarget = top;
    work->barDistance = top - work->top < 0 ? -(top - work->top) : top - work->top;
    work->barUp = top < work->top ? TRUE : FALSE;
    return TRUE;
}

// Brings a log entry in on the free line: mode 1 slides it in from the left as the newest, pushing the others down;
// otherwise it comes in from below for dir 0 and from above for dir 1, as the others move by a line
void BeaconView_AddLine(BeaconViewWork *work, const GameBeacon *beacon, u8 dataIndex, u8 dir, u8 frames, int mode) {
    BeaconViewPanel *panel = BeaconView_FindPanel(work, 0xff);
    BeaconViewAddLineTask *data;

    if (panel == NULL) {
        return;
    }
    data = GFL_TCBExGetData(
        GFL_TCBExMgrAddTask(work->tcbManager, BeaconView_AddLineTask, sizeof(BeaconViewAddLineTask), 0));
    MI_CpuFill8(data, 0, sizeof(BeaconViewAddLineTask));
    data->work = work;
    data->dir = dir;
    data->frames = frames;
    data->panel = panel;
    data->mode = mode;
    data->pending = &work->pending;
    work->pending++;
    BeaconViewPanel_Assign(panel, dataIndex, 0);
    BeaconViewPanel_SetBeacon(work, panel, beacon, mode, FALSE, &data->printing);
}

static void BeaconView_AddLineTask(TCBEx *task, void *data) {
    BeaconViewAddLineTask *add = data;
    BeaconViewWork *work = add->work;

    if (add->printing > 0) {
        return;
    }
    if (add->seq == 0) {
        BeaconViewPanel_SetVisible(add->panel, TRUE);
        if (add->mode == 1) {
            BeaconView_SlideInLine(work, add->panel, &add->printing);
            BeaconViewPanel_SetIconVisible(add->panel, TRUE);
        } else {
            BeaconView_MoveLines(work, add->dir, add->frames, add->panel, NULL, &add->printing);
        }
        add->seq++;
        return;
    }
    if (add->pending != NULL) {
        (*add->pending)--;
    }
    GFL_TCBExRequestEnd(task);
}

// Moves every line in use by a line, but for skip, with newPanel coming in from outside the screen
static void BeaconView_MoveLines(BeaconViewWork *work, u8 dir, u8 frames, BeaconViewPanel *newPanel,
                                 BeaconViewPanel *skip, int *pending) {
    int i;

    if (newPanel != NULL) {
        if (dir == 0) {
            newPanel->line = BEACON_VIEW_LINE_MAX + 1;
        } else {
            newPanel->line = 0;
        }
        BeaconViewPanel_SetPos(newPanel, PANEL_X, PANEL_LINE_Y(newPanel->line));
        BeaconViewPanel_SetVisible(newPanel, TRUE);
    }
    for (i = 0; i < BEACON_VIEW_PANEL_MAX; i++) {
        BeaconViewPanel *panel = &work->panels[i];

        if (panel->dataIndex == 0xff) {
            continue;
        }
        if (skip != NULL && panel->id == skip->id) {
            continue;
        }
        BeaconViewPanel_StartMove(work, panel, dir, 0, frames, pending);
    }
}

static void BeaconView_SlideInLine(BeaconViewWork *work, BeaconViewPanel *panel, int *pending) {
    int delay = 0;

    panel->line = 1;
    BeaconViewPanel_SetPos(panel, PANEL_X - 224, 0);
    BeaconViewPanel_SetVisible(panel, TRUE);
    if (work->logCount != 0) {
        delay = 8;
    }
    BeaconViewPanel_StartMove(work, panel, 2, delay, 8, pending);
    BeaconView_MoveLines(work, 1, 8, NULL, panel, pending);
}

// Moves a line to where it belongs over some frames, after going up a line for dir 0 or down a line for dir 1. A line
// that leaves the screen is freed
static void BeaconViewPanel_StartMove(BeaconViewWork *work, BeaconViewPanel *panel, u8 dir, u8 delay, u8 frames,
                                      int *pending) {
    BeaconViewMoveTask *data = GFL_TCBExGetData(
        GFL_TCBExMgrAddTask(work->tcbManager, BeaconViewPanel_MoveTask, sizeof(BeaconViewMoveTask), 0));

    MI_CpuFill8(data, 0, sizeof(BeaconViewMoveTask));
    data->dir = dir;
    if (data->dir == 0) {
        panel->line--;
    } else if (data->dir == 1) {
        panel->line++;
    }
    data->work = work;
    data->panel = panel;
    data->destX = PANEL_X;
    data->destY = PANEL_LINE_Y(panel->line);
    data->x = FX32_CONST(panel->x);
    data->y = FX32_CONST(panel->y);
    data->frames = frames;
    data->delay = delay;
    data->stepX = FX_Div(FX32_CONST(data->destX - panel->x), FX32_CONST(data->frames));
    data->stepY = FX_Div(FX32_CONST(data->destY - panel->y), FX32_CONST(data->frames));
    data->pending = pending;
    (*data->pending)++;
}

static void BeaconViewPanel_MoveTask(TCBEx *task, void *data) {
    BeaconViewMoveTask *move = data;

    switch (move->seq) {
    case 0:
        if (move->delay != 0) {
            move->delay--;
            return;
        }
        move->frame++;
        if (move->frame < move->frames) {
            move->x += move->stepX;
            move->y += move->stepY;
            BeaconViewPanel_SetPos(move->panel, move->x >> FX32_SHIFT, move->y >> FX32_SHIFT);
            return;
        }
        BeaconViewPanel_SetPos(move->panel, move->destX, move->destY);
        if (move->panel->line == 0 || move->panel->line == BEACON_VIEW_LINE_MAX + 1) {
            move->seq++;
            return;
        }
        break;
    case 1:
    default:
        if (move->panel->printTask != NULL) {
            return;
        }
        BeaconViewPanel_Clear(move->panel);
        break;
    }
    (*move->pending)--;
    GFL_TCBExRequestEnd(task);
}

// Makes the log start at another line, with the lines on the screen showing its entries at once
void BeaconView_StartJump(BeaconViewWork *work, u8 top) {
    int i;
    BeaconViewJumpTask *data =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(work->tcbManager, BeaconView_JumpTask, sizeof(BeaconViewJumpTask), 0));

    MI_CpuFill8(data, 0, sizeof(BeaconViewJumpTask));
    data->work = work;
    data->pending = &work->pending;
    data->top = top;
    data->count = work->viewCount;
    for (i = 0; i < work->viewCount; i++) {
        data->panels[i] = BeaconView_FindPanel(work, (u8)GameBeaconInfoTable_RingIndex(work->log, work->top + i));
    }
    work->top = top;
    func_0202d7b8(work->status, work->top);
    BeaconView_SetPrintTimeLimit(work->printQueue, TRUE);
    BeaconView_UpdateArrows(work);
    BeaconView_PlaySE(SEQ_SE_SELECT4);
    work->pending++;
}

static void BeaconView_JumpTask(TCBEx *task, void *data) {
    int i;
    BeaconViewJumpTask *jump = data;
    BeaconViewWork *work = jump->work;

    switch (jump->seq) {
    case 0:
        for (i = 0; i < jump->count; i++) {
            if (jump->panels[i]->printTask != NULL) {
                return;
            }
        }
        jump->seq++;
        return;
    case 1:
        for (i = 0; i < jump->count; i++) {
            u8 dataIndex = GameBeaconInfoTable_RingIndex(work->log, jump->top + i);

            GameBeaconInfoTable_RingGet(work->log, work->beacon, &work->time, jump->top + i);
            BeaconViewPanel_Reassign(jump->panels[i], dataIndex, i + 1);
            BeaconViewPanel_SetBeacon(work, jump->panels[i], work->beacon, 2, FALSE, &jump->printing);
            BeaconViewPanel_SetVisible(jump->panels[i], TRUE);
        }
        jump->seq++;
        return;
    case 2:
        if (jump->printing > 0) {
            return;
        }
        break;
    }
    BeaconView_SetPrintTimeLimit(work->printQueue, FALSE);
    if (jump->pending != NULL) {
        (*jump->pending)--;
    }
    GFL_TCBExRequestEnd(task);
}

// Shows the message of a beacon just received, or offers its Pass Power, and returns whether it shows anything
BOOL BeaconView_ShowBeaconMessage(BeaconViewWork *work, const GameBeacon *beacon, BOOL isNew) {
    int type = GameBeacon_GetType(beacon);

    if (isNew) {
        GameBeacon_SetMessageWords(beacon, work->wordSet, work->tmpHeapId);
        BeaconView_ShowMessage(work, GameBeacon_Text_Nearby);
        return TRUE;
    }
    if (GameBeacon_IsMissionType(beacon)) {
        return FALSE;
    }
    switch (type) {
    case 1:
        return FALSE;
    case 18:
        return BeaconView_TakePassPower(work, beacon, FALSE);
    case 50:
        return BeaconView_TakePassPower(work, beacon, TRUE);
    case 24:
        GameBeacon_BroadcastThankedCount(func_0200c910((TrainerGameInfoSave *)work->trainerCard));
        break;
    }
    BeaconView_ShowTypeMessage(work, beacon);
    return TRUE;
}

static void BeaconView_ShowTypeMessage(BeaconViewWork *work, const GameBeacon *beacon) {
    GameBeacon_SetMessageWords(beacon, work->wordSet, work->tmpHeapId);
    BeaconView_ExpandMessage(work, BeaconView_GetTypeMsgId(beacon));
    BeaconView_StartMessage(work, work->strExpand, &work->pending);
}

void BeaconView_ShowMessage(BeaconViewWork *work, u8 messageId) {
    BeaconView_ExpandMessage(work, messageId);
    BeaconView_StartMessage(work, work->strExpand, &work->pending);
}

// Slides the message window in with a string, and out again after a while or a touch
static void BeaconView_StartMessage(BeaconViewWork *work, const StrBuf *strbuf, int *pending) {
    BeaconViewMessageTask *data = GFL_TCBExGetData(
        GFL_TCBExMgrAddTask(work->tcbManager, BeaconView_MessageTask, sizeof(BeaconViewMessageTask), 0));

    MI_CpuFill8(data, 0, sizeof(BeaconViewMessageTask));
    GFL_StrBufCopy(work->strMessage, strbuf);
    data->work = work;
    data->timer = 90;
    BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 2);
    data->pending = pending;
    (*data->pending)++;
}

static void BeaconView_MessageTask(TCBEx *task, void *data) {
    BeaconViewMessageTask *message = data;

    if (message->printing != 0) {
        return;
    }
    switch (message->seq) {
    case 0:
        BeaconView_PrintMessage(message->work, message->work->strMessage, &message->printing);
        message->seq++;
        return;
    case 1:
        BeaconView_StartBGSlide(message->work->tcbManager, 6, FALSE, &message->printing);
        message->seq++;
        return;
    case 2:
        if (!BeaconView_WaitTimerOrTouch(&message->timer, 6)) {
            return;
        }
        BeaconView_StartBGSlide(message->work->tcbManager, 6, TRUE, &message->printing);
        message->seq++;
        return;
    }
    (*message->pending)--;
    GFL_TCBExRequestEnd(task);
}

// Offers the Pass Power a beacon gives, unless it is already running
static BOOL BeaconView_TakePassPower(BeaconViewWork *work, const GameBeacon *beacon, BOOL gift) {
    u32 passPower;

    if (gift) {
        passPower = GameBeacon_GetGiftPassPower(beacon);
    } else {
        passPower = GameBeacon_GetPassPower(beacon);
    }
    if (!PassPower_IsIDValid(passPower)) {
        return FALSE;
    }
    if (passPower == PassPower_GetUsedIDByPassPower(passPower, work->passPowerData)) {
        return FALSE;
    }
    GameBeacon_GetName(beacon, work->strTemplate);
    BeaconStatus_SetGivenPassPower(work->status, passPower, work->strTemplate);
    GameBeacon_SetMessageWords(beacon, work->wordSet, work->tmpHeapId);
    BeaconView_OfferPassPower(work, passPower, FALSE, &work->pending);
    return TRUE;
}

void BeaconView_OfferRecordPassPower(BeaconViewWork *work, u32 passPower) {
    BeaconView_OfferPassPower(work, passPower, TRUE, &work->pending);
}

// Asks whether to use a Pass Power, with buttons to see the running ones, to use it and to decline
void BeaconView_OfferPassPower(BeaconViewWork *work, u32 passPower, u8 record, int *pending) {
    BeaconViewPassPowerTask *data = GFL_TCBExGetData(
        GFL_TCBExMgrAddTask(work->tcbManager, BeaconView_PassPowerTask, sizeof(BeaconViewPassPowerTask), 0));

    MI_CpuFill8(data, 0, sizeof(BeaconViewPassPowerTask));
    work->usePassPower = PASS_POWER_NONE;
    data->work = work;
    data->timer = 90;
    data->passPower = passPower;
    data->record = record;
    loadPassPowerToStrbuf(work->wordSet, 1, passPower);
    BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 2);
    data->pending = pending;
    (*data->pending)++;
}

static void BeaconView_PassPowerTask(TCBEx *task, void *data) {
    BeaconViewPassPowerTask *offer = data;
    BeaconViewWork *work = offer->work;
    int button;

    if (offer->printing > 0) {
        return;
    }
    switch (offer->seq) {
    case 0:
        BeaconView_ExpandMessage(work, sBeaconViewPassPowerMessages[offer->record]);
        BeaconView_PrintMessage(work, work->strExpand, &offer->printing);
        offer->seq++;
        return;
    case 1:
        BeaconView_StartBGSlide(work->tcbManager, 6, FALSE, &offer->printing);
        offer->seq++;
        return;
    case 2:
        BeaconView_SetPrintTimeLimit(work->printQueue, TRUE);
        BeaconViewMenu_Open(work->menu, 0);
        BeaconView_ClearTotal(work);
        BeaconView_SetButtonsVisible(work, FALSE);
        offer->seq++;
        return;
    case 3:
        button = BeaconViewMenu_Update(work->menu);
        if (button < 0) {
            return;
        }
        offer->button = button;
        switch (offer->button) {
        case 0:
            BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_PASS_POWER_LIST);
            offer->seq = 3;
            return;
        case 1:
            work->usePassPower = offer->passPower;
            BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_PASS_POWER_USE);
            offer->seq = 4;
            return;
        case 2:
            offer->seq = 5;
            return;
        }
        return;
    case 4:
        // The event was called off: ask again
        if (work->usePassPower != PASS_POWER_NONE) {
            work->usePassPower = PASS_POWER_NONE;
            offer->seq = 3;
            return;
        }
        BeaconViewMenu_Close(work->menu);
        offer->seq = 6;
        return;
    case 5:
        if (!BeaconViewMenu_WaitFlash(work->menu, offer->button)) {
            return;
        }
        offer->seq++;
        return;
    case 6:
        if (offer->record == TRUE) {
            BeaconStatus_PopRecord(work->status);
        } else {
            BeaconStatus_ClearGivenPassPower(work->status);
        }
        BeaconView_PrintTotal(work, GameBeacon_Text_Total, &offer->printing);
        BeaconView_SetButtonsVisible(work, TRUE);
        BeaconView_StartBGSlide(work->tcbManager, 6, TRUE, &offer->printing);
        offer->seq++;
        return;
    default:
        BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 0);
        BeaconView_SetPrintTimeLimit(work->printQueue, FALSE);
        (*offer->pending)--;
        GFL_TCBExRequestEnd(task);
        return;
    }
}

// Asks whether to join a Funfest mission, with buttons to see it, to join and to decline
void BeaconView_OfferMission(BeaconViewWork *work, const FestMissionConfig *config, int *pending) {
    BeaconViewMissionTask *data = GFL_TCBExGetData(
        GFL_TCBExMgrAddTask(work->tcbManager, BeaconView_MissionTask, sizeof(BeaconViewMissionTask), 0));

    MI_CpuFill8(data, 0, sizeof(BeaconViewMissionTask));
    ClearFesMissionCfg(&work->mission);
    work->missionRefusal = 0;
    data->work = work;
    data->timer = 90;
    data->config = *config;
    BeaconView_SetButtonAnim(work, BEACON_VIEW_ACTOR_PASS_POWER, 2);
    BeaconView_SetButtonAnim(work, BEACON_VIEW_ACTOR_GREETING, 2);
    BeaconView_SetButtonAnim(work, BEACON_VIEW_ACTOR_THANKS, 2);
    BeaconView_SetButtonAnim(work, BEACON_VIEW_ACTOR_RETURN, 2);
    data->pending = pending;
    (*data->pending)++;
}

static void BeaconView_MissionTask(TCBEx *task, void *data) {
    BeaconViewMissionTask *offer = data;
    BeaconViewWork *work = offer->work;
    int button;
    u32 refusal;

    if (offer->printing > 0) {
        return;
    }
    switch (offer->seq) {
    case 0:
        BeaconView_ExpandMessage(work, offer->config.info.unk0_30 + GameBeacon_Text_WantJoinFunfestMission);
        BeaconView_PrintMessage(work, work->strExpand, &offer->printing);
        offer->seq++;
        return;
    case 1:
        BeaconView_StartBGSlide(work->tcbManager, 6, FALSE, &offer->printing);
        offer->seq++;
        return;
    case 2:
        BeaconView_SetPrintTimeLimit(work->printQueue, TRUE);
        BeaconViewMenu_Open(work->menu, 0);
        BeaconView_ClearTotal(work);
        BeaconView_SetButtonsVisible(work, FALSE);
        offer->seq++;
        return;
    case 3:
        button = BeaconViewMenu_Update(work->menu);
        if (button < 0) {
            return;
        }
        offer->button = button;
        switch (offer->button) {
        case 0:
            refusal = BeaconView_CheckMissionRefusal(work, &offer->config, offer->button);
            work->mission = offer->config;
            work->missionButton = offer->button;
            if (refusal == 0 || refusal == 3) {
                BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_MISSION_INFO);
                offer->seq = 3;
                return;
            }
            work->missionRefusal = refusal;
            BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_MISSION_MESSAGE);
            offer->seq = 4;
            return;
        case 1:
            refusal = BeaconView_CheckMissionRefusal(work, &offer->config, offer->button);
            work->mission = offer->config;
            work->missionButton = offer->button;
            if (refusal == 0) {
                BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_MISSION_START);
            } else {
                work->missionRefusal = refusal;
                BeaconView_RequestEvent(work, BEACON_VIEW_EVENT_MISSION_MESSAGE);
            }
            offer->seq = 4;
            return;
        case 2:
            LinkFestival_AddRecord(work->festival, &offer->config);
            offer->seq = 5;
            return;
        }
        return;
    case 4:
        // The event was called off: ask again
        if (work->mission.info.index != FEST_MISSION_NONE) {
            ClearFesMissionCfg(&work->mission);
            offer->seq = 3;
            return;
        }
        BeaconViewMenu_Close(work->menu);
        offer->seq = 6;
        return;
    case 5:
        if (!BeaconViewMenu_WaitFlash(work->menu, offer->button)) {
            return;
        }
        offer->seq++;
        return;
    case 6:
        BeaconView_PrintTotal(work, GameBeacon_Text_Total, &offer->printing);
        BeaconView_SetButtonsVisible(work, TRUE);
        BeaconView_StartBGSlide(work->tcbManager, 6, TRUE, &offer->printing);
        offer->seq++;
        return;
    default:
        BeaconView_SetButtonAnim(work, BEACON_VIEW_BUTTON_ALL, 0);
        BeaconView_SetPrintTimeLimit(work->printQueue, FALSE);
        (*offer->pending)--;
        GFL_TCBExRequestEnd(task);
        return;
    }
}

// Why the mission can't be joined: 0 when it can, 1 or 2 when it is out of time, 3 when its flag is not set
static u32 BeaconView_CheckMissionRefusal(BeaconViewWork *work, FestMissionConfig *config, int button) {
    EventWork *eventWork = GameData_GetEventWork(work->gameData);

    if (!FestMissionConfig_HasTimeLeft(config)) {
        if (button == 0) {
            return 2;
        }
        return 1;
    }
    if (config->info.unk8_18 != 0 && !EventWork_FlagGet(eventWork, config->info.unk8_18)) {
        return 3;
    }
    return 0;
}

// Shows the icon of the beacon's type on the line for a while, after which the line shows the name again
static void BeaconViewPanel_StartIcon(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon,
                                      BOOL greeting) {
    TCBEx *task = GFL_TCBExMgrAddTask(work->tcbManager, BeaconViewPanel_IconTask, sizeof(BeaconViewIconTask), 0);
    BeaconViewIconTask *data = GFL_TCBExGetData(task);

    MI_CpuFill8(data, 0, sizeof(BeaconViewIconTask));
    data->work = work;
    data->panel = panel;
    data->timer = 90;
    panel->iconTask = task;
    if (greeting) {
        data->type = 1;
    } else {
        data->type = GameBeacon_GetType(beacon);
    }
    BeaconViewPanel_SetIcon(work, panel, func_020140cc(data->type));
    BeaconViewPanel_SetIconVisible(panel, greeting ? FALSE : TRUE);
}

static void BeaconViewPanel_IconTask(TCBEx *task, void *data) {
    BeaconViewIconTask *icon = data;

    if (icon->seq == 0) {
        if (icon->timer-- != 0) {
            return;
        }
        if (icon->type == 1 || icon->type == 24 || icon->type == 49) {
            BeaconViewPanel_PrintName(icon->work, icon->panel, TRUE, NULL);
            icon->seq++;
            return;
        }
    } else if (icon->panel->printTask != NULL) {
        return;
    }
    icon->panel->iconTask = NULL;
    BeaconViewPanel_SetIconVisible(icon->panel, FALSE);
    GFL_TCBExRequestEnd(task);
}

// Flashes the touched line white
void BeaconView_FlashSelected(BeaconViewWork *work, u8 offset) {
    u8 dataIndex = GameBeaconInfoTable_RingIndex(work->log, offset + work->top);
    BeaconViewPanel *panel = BeaconView_FindPanel(work, dataIndex);

    BeaconViewPanel_StartFlash(work, panel, &work->pending);
    BeaconView_PlaySE(SEQ_SE_DECIDE1);
}

static void BeaconViewPanel_StartFlash(BeaconViewWork *work, BeaconViewPanel *panel, int *pending) {
    BeaconViewFlashTask *data = GFL_TCBExGetData(
        GFL_TCBExMgrAddTask(work->tcbManager, BeaconViewPanel_FlashTask, sizeof(BeaconViewFlashTask), 0));

    MI_CpuFill8(data, 0, sizeof(BeaconViewFlashTask));
    data->work = work;
    data->panel = panel;
    data->frames = 2;
    data->offset = (panel->id + 4) * 16;
    PaletteFade_BlendBuffer(data->work->paletteFade, 3, data->offset, 16, 4, 0x7fff);
    if (pending != NULL) {
        data->pending = pending;
        (*data->pending)++;
    }
}

static void BeaconViewPanel_FlashTask(TCBEx *task, void *data) {
    BeaconViewFlashTask *flash = data;

    if (flash->seq < 2) {
        if (flash->frame++ < flash->frames) {
            return;
        }
        PaletteFade_BlendBuffer(flash->work->paletteFade, 3, flash->offset, 16, (flash->seq % 2) * 4, 0x7fff);
        flash->seq++;
        flash->frame = 0;
        return;
    }
    PaletteFade_BlendBuffer(flash->work->paletteFade, 3, flash->offset, 16, 0, 0x7fff);
    if (flash->pending != NULL) {
        (*flash->pending)--;
    }
    GFL_TCBExRequestEnd(task);
}

// Prints a string on the line's plate, at once or through the queue with a task that waits for it
static void BeaconViewPanel_Print(BeaconViewWork *work, BeaconViewPanel *panel, const StrBuf *strbuf, u16 color,
                                  int *pending) {
    BeaconViewOam *oam = &panel->oam;

    GFL_BitmapFill(oam->bitmap, 0);
    if (work->printNow) {
        GFL_TextRendererDrawToBitmapEx(oam->bitmap, 0, 0, strbuf, work->font, color);
        BmpOam_ActorBmpTrans(oam->actor);
    } else {
        PrintQueue_PrintColor(work->printQueue, oam->bitmap, 0, 0, strbuf, work->font, color);
        BeaconViewPanel_StartPrintTask(work, panel, pending);
    }
}

static void BeaconViewPanel_StartPrintTask(BeaconViewWork *work, BeaconViewPanel *panel, int *pending) {
    TCBEx *task = GFL_TCBExMgrAddTask(work->tcbManager, BeaconViewPanel_PrintTask, sizeof(BeaconViewPrintTask), 0);
    BeaconViewPrintTask *data = GFL_TCBExGetData(task);

    MI_CpuFill8(data, 0, sizeof(BeaconViewPrintTask));
    data->work = work;
    data->panel = panel;
    data->oam = &panel->oam;
    panel->printTask = task;
    if (pending != NULL) {
        data->pending = pending;
        (*data->pending)++;
    }
}

static void BeaconViewPanel_PrintTask(TCBEx *task, void *data) {
    BeaconViewPrintTask *print = data;

    if (!PrintQueue_IsBitmapPending(print->work->printQueue, print->oam->bitmap)) {
        BmpOam_ActorBmpTrans(print->oam->actor);
        if (print->pending != NULL) {
            (*print->pending)--;
        }
        print->panel->printTask = NULL;
        GFL_TCBExRequestEnd(task);
    }
}
