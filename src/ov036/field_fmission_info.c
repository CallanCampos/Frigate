// The Funfest mission display on the field: the sprites for a mission's time left and score, which slide in and out
// as the mission runs. The ROM names the file in its heap allocation. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0). The code was first written by mariano54 on their
// parallel-decomp branch.
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "field/fest_mission_gimmick.h"
#include "field/festival.h"
#include "field/field.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "system/fest_mission.h"
#include "system/game_system.h"

// The archive's files
enum {
    FMISSION_FILE_PALETTE,
    FMISSION_FILE_CHARS,
    FMISSION_FILE_CELLS,
    FMISSION_FILE_ANIMS,
};

// The cells' animation sequences. The mission icons follow the last, one for each of the mission's target kinds
enum {
    FMISSION_SEQ_GROUP_DIGIT,
    FMISSION_SEQ_DIGIT,
    FMISSION_SEQ_COLON,
    FMISSION_SEQ_GROUP_LABEL,
    FMISSION_SEQ_SCORE_LABEL,
    FMISSION_SEQ_TIME_PANEL,
    FMISSION_SEQ_SCORE_GAIN,
    FMISSION_SEQ_TIME_GAIN,
    FMISSION_SEQ_FLASH,
    FMISSION_SEQ_ICONS,
};

// The digits of a count or a score, and of the minutes or the seconds
#define FMISSION_COUNT_DIGITS 4
#define FMISSION_TIME_DIGITS 2

// The actors. At the bottom, slid up into place: with more than one player in the mission, a label and the digits of
// LinkFestival_GetShownScore's count; the score's label and digits; the mission's icon; and the highlight of a rising
// score, a flash over the digits and a sign beside the icon. At the top, slid down: the time's panel, its colon, the
// minutes, the seconds, and the highlight of time gained
enum {
    FMISSION_ACTOR_GROUP_LABEL,
    FMISSION_ACTOR_GROUP_DIGITS,
    FMISSION_ACTOR_SCORE_LABEL = FMISSION_ACTOR_GROUP_DIGITS + FMISSION_COUNT_DIGITS,
    FMISSION_ACTOR_SCORE_DIGITS,
    FMISSION_ACTOR_ICON = FMISSION_ACTOR_SCORE_DIGITS + FMISSION_COUNT_DIGITS,
    FMISSION_ACTOR_SCORE_FLASH,
    FMISSION_ACTOR_SCORE_GAIN,
    FMISSION_ACTOR_TIME_PANEL,
    FMISSION_ACTOR_COLON,
    FMISSION_ACTOR_MINUTES,
    FMISSION_ACTOR_SECONDS = FMISSION_ACTOR_MINUTES + FMISSION_TIME_DIGITS,
    FMISSION_ACTOR_TIME_FLASH = FMISSION_ACTOR_SECONDS + FMISSION_TIME_DIGITS,
    FMISSION_ACTOR_TIME_GAIN,
    FMISSION_ACTOR_COUNT,
};

// The frames that the actors take to slide into place, and that a highlight stays up
#define FMISSION_SLIDE_FRAMES 8
#define FMISSION_HIGHLIGHT_FRAMES 60

struct FieldFMissionInfo {
    // This file never uses unk0, unk4 or unk24, which the allocation zeroes
    u32 unk0;
    u32 unk4;
    Field *field;
    // Open only while the display is created
    ArcTool *arc;
    HeapID heapId;
    LinkFestival *festival;
    FestMissionConfig *config;
    u8 shown : 1;
    // Whether the group's label and digits are shown, with more than one player in the mission
    u8 groupVisible : 1;
    // Whether the highlight of a rising score, or of time gained, is up, and the frames it has left
    u8 scoreAnim : 1;
    u8 timeAnim : 1;
    u8 scoreTimer;
    u8 timeTimer;
    // The seconds left that the digits show, 0xffff before the first
    u16 seconds;
    u32 unk24;
    u32 chars;
    u32 cellAnims;
    u32 palette;
    ClActUnit *unit;
    ClActRenderer *renderer;
    ClActor *actors[FMISSION_ACTOR_COUNT];
    // Where each actor rests
    ClActorPos positions[FMISSION_ACTOR_COUNT];
    TCBExManager *tasks;
};

// A task that slides count actors from first into place, from yOffset away, over timer frames
typedef struct {
    FieldFMissionInfo *info;
    u8 first;
    u8 count;
    int timer;
    fx32 step;
} FieldFMissionSlide;

// A task that hides a highlight when the score's or the time's timer runs out
typedef struct {
    FieldFMissionInfo *info;
} FieldFMissionBlink;

static void FieldFMissionInfo_LoadResources(FieldFMissionInfo *info, ArcTool *arc);
static void FieldFMissionInfo_FreeResources(FieldFMissionInfo *info);
static ClActor *FieldFMissionInfo_CreateActor(FieldFMissionInfo *info, s16 x, s16 y, u16 sequence, u8 priority);
static void FieldFMissionInfo_CreateActors(FieldFMissionInfo *info);
static void FieldFMissionInfo_DeleteActors(FieldFMissionInfo *info);
static void FieldFMissionInfo_SetActorsVisible(FieldFMissionInfo *info, BOOL visible);
static void FieldFMissionInfo_Hide(FieldFMissionInfo *info);
static BOOL FieldFMissionInfo_IsGroupVisible(FieldFMissionInfo *info);
static u32 FieldFMissionInfo_GetDigit(u32 value, u8 digit);
static void FieldFMissionInfo_SetNumber(FieldFMissionInfo *info, u32 value, int digits, int firstActor, BOOL showZeros);
static void FieldFMissionInfo_UpdateTime(FieldFMissionInfo *info, BOOL force);
static void FieldFMissionInfo_UpdateGroupCount(FieldFMissionInfo *info);
static void FieldFMissionInfo_SetActorX(FieldFMissionInfo *info, u8 actor, u16 x);
static void FieldFMissionInfo_GetScoreLayout(int targetKind, int score, u16 *iconX, u16 *scoreX);
static void FieldFMissionInfo_UpdateScore(FieldFMissionInfo *info);
static void FieldFMissionInfo_Show(FieldFMissionInfo *info);
static void FieldFMissionInfo_StartSlide(FieldFMissionInfo *info, u8 first, u8 count, int yOffset);
static void FieldFMissionInfo_SlideTask(TCBEx *task, void *data);
static void FieldFMissionInfo_UpdateScoreHighlight(FieldFMissionInfo *info);
static void FieldFMissionInfo_ScoreHighlightTask(TCBEx *task, void *data);
static void FieldFMissionInfo_UpdateTimeHighlight(FieldFMissionInfo *info);
static void FieldFMissionInfo_TimeHighlightTask(TCBEx *task, void *data);

FieldFMissionInfo *FieldFMissionInfo_Create(GameSystem *gsys, HeapID heapId) {
    FieldFMissionInfo *info = GFL_HeapAllocate(heapId, sizeof(FieldFMissionInfo), TRUE, "field_fmission_info.c", 192);

    info->arc = GFL_ArcSysCreateFileHandle(ARCID_FIELD_FMISSION_INFO, heapId);
    info->heapId = heapId;
    info->field = GSYS_GetField(gsys);
    info->festival = GSYS_GetLinkFestival(gsys);
    info->config = GetFestMissionCfg(info->festival);
    info->seconds = 0xffff;
    FieldFMissionInfo_LoadResources(info, info->arc);
    FieldFMissionInfo_CreateActors(info);
    info->tasks = GFL_TCBExMgrCreate(info->heapId, info->heapId, 16, 128);
    GFL_ArcToolFree(info->arc);
    info->arc = NULL;
    FieldFMissionInfo_Hide(info);
    return info;
}

void FieldFMissionInfo_Free(FieldFMissionInfo *info) {
    GFL_TCBExMgrFreeTasks(info->tasks);
    GFL_TCBExMgrFree(info->tasks);
    FieldFMissionInfo_DeleteActors(info);
    FieldFMissionInfo_FreeResources(info);
    GFL_HeapFree(info);
}

void FieldFMissionInfo_Update(FieldFMissionInfo *info) {
    if (isFesMissionAvailable(info->config)) {
        BOOL running = Field_IsEventRunning(info->field);

        // The ROM reads shown again for the second test
        if (info->shown) {
            if (running) {
                FieldFMissionInfo_Hide(info);
            } else {
                FieldFMissionInfo_UpdateScoreHighlight(info);
                FieldFMissionInfo_UpdateTimeHighlight(info);
            }
        } else if (!info->shown && !running) {
            FieldFMissionInfo_Show(info);
        }
        GFL_TCBExMgrUpdate(info->tasks);
    }
}

void FieldFMissionInfo_UpdateDisplay(FieldFMissionInfo *info) {
    if (isFesMissionAvailable(info->config) && info->shown) {
        FieldFMissionInfo_UpdateTime(info, FALSE);
        info->groupVisible = FieldFMissionInfo_IsGroupVisible(info);
        FieldFMissionInfo_UpdateGroupCount(info);
        FieldFMissionInfo_UpdateScore(info);
    }
}

static void FieldFMissionInfo_LoadResources(FieldFMissionInfo *info, ArcTool *arc) {
    info->unit = ClActUnit_Create(FMISSION_ACTOR_COUNT, 0, info->heapId);
    {
        ClActSurfaceSetup surface = { 0, 0, 256, 192, 0, 2 };

        info->renderer = ClActRenderer_Create(&surface, 1, info->heapId);
    }
    ClActUnit_SetRenderer(info->unit, info->renderer);
    ClActUnit_SetVisibleAnimOnly(info->unit, TRUE);
    ClActUnit_SetSharedParams(info->unit, TRUE);
    info->palette = ClActSys_LoadPlttRange(arc, FMISSION_FILE_PALETTE, CLACT_VRAM_MAIN, 160, 0, 1, info->heapId);
    info->chars = ClActSys_LoadChars(arc, FMISSION_FILE_CHARS, TRUE, CLACT_VRAM_MAIN, info->heapId);
    info->cellAnims = ClActSys_LoadCellAnims(arc, FMISSION_FILE_CELLS, FMISSION_FILE_ANIMS, info->heapId);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
}

static void FieldFMissionInfo_FreeResources(FieldFMissionInfo *info) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, FALSE);
    ClActSys_FreeCellAnims(info->cellAnims);
    ClActSys_FreeChars(info->chars);
    ClActSys_FreePltt(info->palette);
    ClActRenderer_Delete(info->renderer);
    ClActUnit_Delete(info->unit);
}

static ClActor *FieldFMissionInfo_CreateActor(FieldFMissionInfo *info, s16 x, s16 y, u16 sequence, u8 priority) {
    ClActorSetup setup;
    ClActor *actor;

    setup.x = x;
    setup.y = y;
    setup.sequence = sequence;
    setup.bgPriority = 1;
    setup.priority = priority;
    actor = ClActor_Create(info->unit, info->chars, info->palette, info->cellAnims, &setup, CLACT_SURFACE_MAIN,
                           info->heapId);
    ClActor_SetAutoAnim(actor, FALSE);
    ClActor_SetDrawAnim(actor, FALSE);
    ClActor_SetVisible(actor, FALSE);
    return actor;
}

static void FieldFMissionInfo_CreateActors(FieldFMissionInfo *info) {
    int i;

    info->actors[FMISSION_ACTOR_GROUP_LABEL] = FieldFMissionInfo_CreateActor(info, 0, 144, FMISSION_SEQ_GROUP_LABEL, 2);
    info->actors[FMISSION_ACTOR_SCORE_LABEL] = FieldFMissionInfo_CreateActor(info, 0, 168, FMISSION_SEQ_SCORE_LABEL, 2);
    info->actors[FMISSION_ACTOR_TIME_PANEL] = FieldFMissionInfo_CreateActor(info, 176, 0, FMISSION_SEQ_TIME_PANEL, 2);
    for (i = 0; i < FMISSION_COUNT_DIGITS; i++) {
        info->actors[i + FMISSION_ACTOR_GROUP_DIGITS] =
            FieldFMissionInfo_CreateActor(info, i * 8 + 46, 144, FMISSION_SEQ_GROUP_DIGIT, 1);
        info->actors[i + FMISSION_ACTOR_SCORE_DIGITS] =
            FieldFMissionInfo_CreateActor(info, i * 8 + 44, 172, FMISSION_SEQ_DIGIT, 1);
    }
    info->actors[FMISSION_ACTOR_ICON] = FieldFMissionInfo_CreateActor(info, 80, 180, FMISSION_SEQ_ICONS, 1);
    for (i = 0; i < FMISSION_TIME_DIGITS; i++) {
        info->actors[i + FMISSION_ACTOR_MINUTES] =
            FieldFMissionInfo_CreateActor(info, i * 8 + 204, 4, FMISSION_SEQ_DIGIT, 1);
        // The seconds follow the minutes' columns, moved past the colon
        info->actors[i + FMISSION_ACTOR_SECONDS] =
            FieldFMissionInfo_CreateActor(info, (i + FMISSION_TIME_DIGITS) * 8 + 209, 4, FMISSION_SEQ_DIGIT, 1);
    }
    info->actors[FMISSION_ACTOR_COLON] = FieldFMissionInfo_CreateActor(info, 219, 4, FMISSION_SEQ_COLON, 1);
    ClActor_SetAutoAnim(info->actors[FMISSION_ACTOR_COLON], TRUE);
    info->actors[FMISSION_ACTOR_SCORE_FLASH] = FieldFMissionInfo_CreateActor(info, 40, 172, FMISSION_SEQ_FLASH, 0);
    info->actors[FMISSION_ACTOR_TIME_FLASH] = FieldFMissionInfo_CreateActor(info, 208, 4, FMISSION_SEQ_FLASH, 0);
    ClActor_SetAutoAnim(info->actors[FMISSION_ACTOR_SCORE_FLASH], TRUE);
    ClActor_SetAutoAnim(info->actors[FMISSION_ACTOR_TIME_FLASH], TRUE);
    info->actors[FMISSION_ACTOR_SCORE_GAIN] = FieldFMissionInfo_CreateActor(info, 104, 180, FMISSION_SEQ_SCORE_GAIN, 2);
    info->actors[FMISSION_ACTOR_TIME_GAIN] = FieldFMissionInfo_CreateActor(info, 104, 4, FMISSION_SEQ_TIME_GAIN, 2);
    for (i = 0; i < FMISSION_ACTOR_COUNT; i++) {
        ClActor_GetSurfacePos(info->actors[i], &info->positions[i], CLACT_SURFACE_MAIN);
    }
}

static void FieldFMissionInfo_DeleteActors(FieldFMissionInfo *info) {
    int i;

    for (i = 0; i < FMISSION_ACTOR_COUNT; i++) {
        ClActor_SetVisible(info->actors[i], FALSE);
        ClActor_Delete(info->actors[i]);
    }
}

static void FieldFMissionInfo_SetActorsVisible(FieldFMissionInfo *info, BOOL visible) {
    int i;

    for (i = 0; i < FMISSION_ACTOR_COUNT; i++) {
        ClActor_SetVisible(info->actors[i], visible);
    }
}

static void FieldFMissionInfo_Hide(FieldFMissionInfo *info) {
    FieldFMissionInfo_SetActorsVisible(info, FALSE);
    info->shown = FALSE;
    ClActUnit_SetActive(info->unit, FALSE);
    info->scoreTimer = 0;
}

static BOOL FieldFMissionInfo_IsGroupVisible(FieldFMissionInfo *info) {
    return LinkFestival_GetMemberCount(info->festival) > 1;
}

static u32 FieldFMissionInfo_GetDigit(u32 value, u8 digit) {
    u32 powers[8] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000 };

    return value % powers[digit + 1] / powers[digit];
}

static void FieldFMissionInfo_SetNumber(FieldFMissionInfo *info, u32 value, int digits, int firstActor,
                                        BOOL showZeros) {
    int i;
    BOOL started = FALSE;

    for (i = digits - 1; i >= 0; i--) {
        int digit = FieldFMissionInfo_GetDigit(value, i);

        if (i > 0 && !showZeros) {
            BOOL visible = FALSE;

            if (digit > 0 || started) {
                visible = TRUE;
                started = TRUE;
            }
            ClActor_SetVisible(info->actors[firstActor], visible);
        } else {
            ClActor_SetVisible(info->actors[firstActor], TRUE);
        }
        ClActor_SetAnimFrame(info->actors[firstActor], digit);
        firstActor++;
    }
}

static void FieldFMissionInfo_UpdateTime(FieldFMissionInfo *info, BOOL force) {
    int time = LinkFestival_GetSecondsLeft(info->festival);
    int minutes = time / 60;

    if (time != info->seconds || force) {
        FieldFMissionInfo_SetNumber(info, minutes % 60, FMISSION_TIME_DIGITS, FMISSION_ACTOR_MINUTES, TRUE);
        FieldFMissionInfo_SetNumber(info, time % 60, FMISSION_TIME_DIGITS, FMISSION_ACTOR_SECONDS, TRUE);
        info->seconds = time;
        if (!force && time > 0 && time <= 10) {
            GFL_SndSEPlay(SEQ_SE_DECIDE5);
        }
    }
}

static void FieldFMissionInfo_UpdateGroupCount(FieldFMissionInfo *info) {
    if (info->groupVisible) {
        ClActor_SetVisible(info->actors[FMISSION_ACTOR_GROUP_LABEL], TRUE);
        FieldFMissionInfo_SetNumber(info, LinkFestival_GetShownScore(info->festival), FMISSION_COUNT_DIGITS,
                                    FMISSION_ACTOR_GROUP_DIGITS, FALSE);
    }
}

static void FieldFMissionInfo_SetActorX(FieldFMissionInfo *info, u8 actor, u16 x) {
    info->positions[actor].x = x;
    ClActor_SetSurfacePos(info->actors[actor], &info->positions[actor], CLACT_SURFACE_MAIN);
}

// Where the mission's icon and the player's score go, the same whatever the mission and the score
static void FieldFMissionInfo_GetScoreLayout(int targetKind, int score, u16 *iconX, u16 *scoreX) {
    *iconX = 80;
    *scoreX = 44;
}

static void FieldFMissionInfo_UpdateScore(FieldFMissionInfo *info) {
    int i;
    int targetKind = info->config->info.targetKind;
    int score = LinkFestival_GetShownTotal(info->festival);
    u16 iconX;
    u16 scoreX;

    FieldFMissionInfo_SetNumber(info, score, FMISSION_COUNT_DIGITS, FMISSION_ACTOR_SCORE_DIGITS, FALSE);
    ClActor_SetSequence(info->actors[FMISSION_ACTOR_ICON], targetKind + FMISSION_SEQ_ICONS);
    ClActor_RestartAnim(info->actors[FMISSION_ACTOR_ICON]);
    FieldFMissionInfo_GetScoreLayout(targetKind, score, &iconX, &scoreX);
    FieldFMissionInfo_SetActorX(info, FMISSION_ACTOR_ICON, iconX);
    for (i = 0; i < FMISSION_COUNT_DIGITS; i++) {
        FieldFMissionInfo_SetActorX(info, i + FMISSION_ACTOR_SCORE_DIGITS, scoreX + i * 8);
    }
}

static void FieldFMissionInfo_Show(FieldFMissionInfo *info) {
    info->groupVisible = FieldFMissionInfo_IsGroupVisible(info);
    // The bottom's actors, from the group's label or the score's, slide up, and the top's slide down
    if (info->groupVisible) {
        FieldFMissionInfo_StartSlide(info, FMISSION_ACTOR_GROUP_LABEL,
                                     FMISSION_ACTOR_TIME_PANEL - FMISSION_ACTOR_GROUP_LABEL, 48);
    } else {
        FieldFMissionInfo_StartSlide(info, FMISSION_ACTOR_SCORE_LABEL,
                                     FMISSION_ACTOR_TIME_PANEL - FMISSION_ACTOR_SCORE_LABEL, 48);
    }
    FieldFMissionInfo_StartSlide(info, FMISSION_ACTOR_TIME_PANEL, FMISSION_ACTOR_COUNT - FMISSION_ACTOR_TIME_PANEL,
                                 -24);
    FieldFMissionInfo_UpdateScoreHighlight(info);
    FieldFMissionInfo_UpdateTimeHighlight(info);
    FieldFMissionInfo_UpdateTime(info, TRUE);
    FieldFMissionInfo_UpdateGroupCount(info);
    FieldFMissionInfo_UpdateScore(info);
    info->shown = TRUE;
    ClActUnit_SetActive(info->unit, TRUE);
}

static void FieldFMissionInfo_StartSlide(FieldFMissionInfo *info, u8 first, u8 count, int yOffset) {
    FieldFMissionSlide *slide;
    int i;
    TCBEx *task = GFL_TCBExMgrAddTask(info->tasks, FieldFMissionInfo_SlideTask, sizeof(FieldFMissionSlide), 0);

    slide = GFL_TCBExGetData(task);
    MI_CpuFill8(slide, 0, sizeof(FieldFMissionSlide));
    slide->info = info;
    slide->first = first;
    slide->count = count;
    slide->timer = FMISSION_SLIDE_FRAMES;
    for (i = 0; i < count; i++) {
        int actor = first + i;
        ClActorPos pos;

        pos.x = info->positions[actor].x;
        pos.y = yOffset + info->positions[actor].y;
        ClActor_SetSurfacePos(info->actors[actor], &pos, CLACT_SURFACE_MAIN);
        // The last two, a highlight, stay hidden
        if (i < count - 2) {
            ClActor_SetVisible(info->actors[actor], TRUE);
        }
    }
    slide->step = FX_Div(FX32_CONST(yOffset), FX32_CONST(slide->timer));
}

static void FieldFMissionInfo_SlideTask(TCBEx *task, void *data) {
    FieldFMissionSlide *slide = data;
    FieldFMissionInfo *info = slide->info;
    int i;
    ClActorPos pos;

    if (!info->shown) {
        GFL_TCBExRequestEnd(task);
        return;
    }
    if (slide->timer-- > 0) {
        for (i = slide->first; i < slide->first + slide->count; i++) {
            pos.x = info->positions[i].x;
            pos.y = info->positions[i].y + FX_Whole(slide->step * slide->timer);
            ClActor_SetSurfacePos(info->actors[i], &pos, CLACT_SURFACE_MAIN);
        }
    } else {
        for (i = slide->first; i < slide->first + slide->count; i++) {
            ClActor_SetSurfacePos(info->actors[i], &info->positions[i], CLACT_SURFACE_MAIN);
        }
        GFL_TCBExRequestEnd(task);
    }
}

static void FieldFMissionInfo_UpdateScoreHighlight(FieldFMissionInfo *info) {
    u32 score = LinkFestival_GetTotalScore(info->festival);

    if (LinkFestival_GetShownTotal(info->festival) < score) {
        info->scoreTimer = FMISSION_HIGHLIGHT_FRAMES;
        LinkFestival_SetShownTotal(info->festival, score);
        GFL_SndSEPlay(SEQ_SE_SW_FES_04);
        if (!info->scoreAnim) {
            TCBEx *task;
            FieldFMissionBlink *blink;

            info->scoreAnim = TRUE;
            task =
                GFL_TCBExMgrAddTask(info->tasks, FieldFMissionInfo_ScoreHighlightTask, sizeof(FieldFMissionBlink), 0);
            blink = GFL_TCBExGetData(task);
            MI_CpuFill8(blink, 0, sizeof(FieldFMissionBlink));
            blink->info = info;
            ClActor_SetVisible(info->actors[FMISSION_ACTOR_SCORE_GAIN], TRUE);
            ClActor_SetVisible(info->actors[FMISSION_ACTOR_SCORE_FLASH], TRUE);
        }
    }
}

static void FieldFMissionInfo_ScoreHighlightTask(TCBEx *task, void *data) {
    FieldFMissionBlink *blink = data;
    FieldFMissionInfo *info = blink->info;

    if (info->scoreTimer != 0) {
        info->scoreTimer--;
        return;
    }
    ClActor_SetVisible(info->actors[FMISSION_ACTOR_SCORE_GAIN], FALSE);
    ClActor_SetVisible(info->actors[FMISSION_ACTOR_SCORE_FLASH], FALSE);
    info->scoreAnim = FALSE;
    GFL_TCBExRequestEnd(task);
}

static void FieldFMissionInfo_UpdateTimeHighlight(FieldFMissionInfo *info) {
    if (LinkFestival_GetSecondsGained(info->festival)) {
        info->timeTimer = FMISSION_HIGHLIGHT_FRAMES;
        LinkFestival_ClearSecondsGained(info->festival);
        if (!info->timeAnim) {
            TCBEx *task;
            FieldFMissionBlink *blink;

            info->timeAnim = TRUE;
            task = GFL_TCBExMgrAddTask(info->tasks, FieldFMissionInfo_TimeHighlightTask, sizeof(FieldFMissionBlink), 0);
            blink = GFL_TCBExGetData(task);
            MI_CpuFill8(blink, 0, sizeof(FieldFMissionBlink));
            blink->info = info;
            ClActor_SetVisible(info->actors[FMISSION_ACTOR_TIME_GAIN], TRUE);
            ClActor_SetVisible(info->actors[FMISSION_ACTOR_TIME_FLASH], TRUE);
        }
    }
}

static void FieldFMissionInfo_TimeHighlightTask(TCBEx *task, void *data) {
    FieldFMissionBlink *blink = data;
    FieldFMissionInfo *info = blink->info;

    if (info->timeTimer != 0) {
        info->timeTimer--;
        return;
    }
    ClActor_SetVisible(info->actors[FMISSION_ACTOR_TIME_GAIN], FALSE);
    ClActor_SetVisible(info->actors[FMISSION_ACTOR_TIME_FLASH], FALSE);
    info->timeAnim = FALSE;
    GFL_TCBExRequestEnd(task);
}
