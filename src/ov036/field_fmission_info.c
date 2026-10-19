// The Funfest mission display on the field: the sprites for a mission's time left and score, which slide in and out
// as the mission runs. The ROM names the file in its heap allocation. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0). The code was first written by mariano54 on their
// parallel-decomp branch.
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

// The cells' animation sequences. The mission icons follow the last, one for each value of the mission's unk10_6
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
// func_0201482c's count; the score's label and digits; the mission's icon; and the highlight of a rising score, a flash
// over the digits and a sign beside the icon. At the top, slid down: the time's panel, its colon, the minutes, the
// seconds, and the highlight of time gained
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

typedef struct {
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
} FieldFMissionInfo;

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

static void func_ov036_021b5cbc(FieldFMissionInfo *info, ArcTool *arc);
static void func_ov036_021b5d4c(FieldFMissionInfo *info);
static ClActor *func_ov036_021b5d78(FieldFMissionInfo *info, s16 x, s16 y, u16 sequence, u8 priority);
static void func_ov036_021b5dc8(FieldFMissionInfo *info);
static void func_ov036_021b5f24(FieldFMissionInfo *info);
static void func_ov036_021b5f48(FieldFMissionInfo *info, BOOL visible);
static void func_ov036_021b5f64(FieldFMissionInfo *info);
static BOOL func_ov036_021b5f84(FieldFMissionInfo *info);
static u32 func_ov036_021b5f98(u32 value, u8 digit);
static void func_ov036_021b5fd4(FieldFMissionInfo *info, u32 value, int digits, int firstActor, BOOL showZeros);
static void func_ov036_021b6040(FieldFMissionInfo *info, BOOL force);
static void func_ov036_021b60a0(FieldFMissionInfo *info);
static void func_ov036_021b60d0(FieldFMissionInfo *info, u8 actor, u16 x);
static void func_ov036_021b60ec(int mode, int score, u16 *iconX, u16 *scoreX);
static void func_ov036_021b60f8(FieldFMissionInfo *info);
static void func_ov036_021b616c(FieldFMissionInfo *info);
static void func_ov036_021b61e8(FieldFMissionInfo *info, u8 first, u8 count, int yOffset);
static void func_ov036_021b62d4(TCBEx *task, void *data);
static void func_ov036_021b6368(FieldFMissionInfo *info);
static void func_ov036_021b63dc(TCBEx *task, void *data);
static void func_ov036_021b6410(FieldFMissionInfo *info);
static void func_ov036_021b6474(TCBEx *task, void *data);

void *func_ov036_021b5b7c(GameSystem *gsys, HeapID heapId) {
    FieldFMissionInfo *info = GFL_HeapAllocate(heapId, sizeof(FieldFMissionInfo), TRUE, "field_fmission_info.c", 192);

    info->arc = GFL_ArcSysCreateFileHandle(ARCID_FIELD_FMISSION_INFO, heapId);
    info->heapId = heapId;
    info->field = GSYS_GetField(gsys);
    info->festival = GSYS_GetLinkFestival(gsys);
    info->config = GetFestMissionCfg(info->festival);
    info->seconds = 0xffff;
    func_ov036_021b5cbc(info, info->arc);
    func_ov036_021b5dc8(info);
    info->tasks = GFL_TCBExMgrCreate(info->heapId, info->heapId, 16, 128);
    GFL_ArcToolFree(info->arc);
    info->arc = NULL;
    func_ov036_021b5f64(info);
    return info;
}

void func_ov036_021b5bfc(void *work) {
    FieldFMissionInfo *info = work;

    GFL_TCBExMgrFreeTasks(info->tasks);
    GFL_TCBExMgrFree(info->tasks);
    func_ov036_021b5f24(info);
    func_ov036_021b5d4c(info);
    GFL_HeapFree(info);
}

void func_ov036_021b5c28(void *work) {
    FieldFMissionInfo *info = work;

    if (isFesMissionAvailable(info->config)) {
        BOOL running = Field_IsEventRunning(info->field);

        // The ROM reads shown again for the second test
        if (info->shown) {
            if (running) {
                func_ov036_021b5f64(info);
            } else {
                func_ov036_021b6368(info);
                func_ov036_021b6410(info);
            }
        } else if (!info->shown && !running) {
            func_ov036_021b616c(info);
        }
        GFL_TCBExMgrUpdate(info->tasks);
    }
}

void func_ov036_021b5c78(void *work) {
    FieldFMissionInfo *info = work;

    if (isFesMissionAvailable(info->config) && info->shown) {
        func_ov036_021b6040(info, FALSE);
        info->groupVisible = func_ov036_021b5f84(info);
        func_ov036_021b60a0(info);
        func_ov036_021b60f8(info);
    }
}

static void func_ov036_021b5cbc(FieldFMissionInfo *info, ArcTool *arc) {
    info->unit = func_0204bf1c(FMISSION_ACTOR_COUNT, 0, info->heapId);
    {
        ClActSurfaceSetup surface = { 0, 0, 256, 192, 0, 2 };

        info->renderer = func_0204be9c(&surface, 1, info->heapId);
    }
    func_0204c018(info->unit, info->renderer);
    func_0204bff0(info->unit, TRUE);
    func_0204c004(info->unit, TRUE);
    info->palette = func_0204bbb8(arc, FMISSION_FILE_PALETTE, CLACT_VRAM_MAIN, 160, 0, 1, info->heapId);
    info->chars = func_0204b81c(arc, FMISSION_FILE_CHARS, TRUE, CLACT_VRAM_MAIN, info->heapId);
    info->cellAnims = func_0204bde0(arc, FMISSION_FILE_CELLS, FMISSION_FILE_ANIMS, info->heapId);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
}

static void func_ov036_021b5d4c(FieldFMissionInfo *info) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, FALSE);
    func_0204be64(info->cellAnims);
    func_0204b98c(info->chars);
    func_0204bcd0(info->palette);
    func_0204becc(info->renderer);
    func_0204bf98(info->unit);
}

static ClActor *func_ov036_021b5d78(FieldFMissionInfo *info, s16 x, s16 y, u16 sequence, u8 priority) {
    ClActorSetup setup;
    ClActor *actor;

    setup.x = x;
    setup.y = y;
    setup.sequence = sequence;
    setup.bgPriority = 1;
    setup.priority = priority;
    actor = func_0204c040(info->unit, info->chars, info->palette, info->cellAnims, &setup, CLACT_SURFACE_MAIN,
                          info->heapId);
    func_0204c520(actor, FALSE);
    func_0204c5c8(actor, FALSE);
    func_0204c124(actor, FALSE);
    return actor;
}

static void func_ov036_021b5dc8(FieldFMissionInfo *info) {
    int i;

    info->actors[FMISSION_ACTOR_GROUP_LABEL] = func_ov036_021b5d78(info, 0, 144, FMISSION_SEQ_GROUP_LABEL, 2);
    info->actors[FMISSION_ACTOR_SCORE_LABEL] = func_ov036_021b5d78(info, 0, 168, FMISSION_SEQ_SCORE_LABEL, 2);
    info->actors[FMISSION_ACTOR_TIME_PANEL] = func_ov036_021b5d78(info, 176, 0, FMISSION_SEQ_TIME_PANEL, 2);
    for (i = 0; i < FMISSION_COUNT_DIGITS; i++) {
        info->actors[i + FMISSION_ACTOR_GROUP_DIGITS] =
            func_ov036_021b5d78(info, i * 8 + 46, 144, FMISSION_SEQ_GROUP_DIGIT, 1);
        info->actors[i + FMISSION_ACTOR_SCORE_DIGITS] =
            func_ov036_021b5d78(info, i * 8 + 44, 172, FMISSION_SEQ_DIGIT, 1);
    }
    info->actors[FMISSION_ACTOR_ICON] = func_ov036_021b5d78(info, 80, 180, FMISSION_SEQ_ICONS, 1);
    for (i = 0; i < FMISSION_TIME_DIGITS; i++) {
        info->actors[i + FMISSION_ACTOR_MINUTES] = func_ov036_021b5d78(info, i * 8 + 204, 4, FMISSION_SEQ_DIGIT, 1);
        // The seconds follow the minutes' columns, moved past the colon
        info->actors[i + FMISSION_ACTOR_SECONDS] =
            func_ov036_021b5d78(info, (i + FMISSION_TIME_DIGITS) * 8 + 209, 4, FMISSION_SEQ_DIGIT, 1);
    }
    info->actors[FMISSION_ACTOR_COLON] = func_ov036_021b5d78(info, 219, 4, FMISSION_SEQ_COLON, 1);
    func_0204c520(info->actors[FMISSION_ACTOR_COLON], TRUE);
    info->actors[FMISSION_ACTOR_SCORE_FLASH] = func_ov036_021b5d78(info, 40, 172, FMISSION_SEQ_FLASH, 0);
    info->actors[FMISSION_ACTOR_TIME_FLASH] = func_ov036_021b5d78(info, 208, 4, FMISSION_SEQ_FLASH, 0);
    func_0204c520(info->actors[FMISSION_ACTOR_SCORE_FLASH], TRUE);
    func_0204c520(info->actors[FMISSION_ACTOR_TIME_FLASH], TRUE);
    info->actors[FMISSION_ACTOR_SCORE_GAIN] = func_ov036_021b5d78(info, 104, 180, FMISSION_SEQ_SCORE_GAIN, 2);
    info->actors[FMISSION_ACTOR_TIME_GAIN] = func_ov036_021b5d78(info, 104, 4, FMISSION_SEQ_TIME_GAIN, 2);
    for (i = 0; i < FMISSION_ACTOR_COUNT; i++) {
        func_0204c178(info->actors[i], &info->positions[i], CLACT_SURFACE_MAIN);
    }
}

static void func_ov036_021b5f24(FieldFMissionInfo *info) {
    int i;

    for (i = 0; i < FMISSION_ACTOR_COUNT; i++) {
        func_0204c124(info->actors[i], FALSE);
        func_0204c108(info->actors[i]);
    }
}

static void func_ov036_021b5f48(FieldFMissionInfo *info, BOOL visible) {
    int i;

    for (i = 0; i < FMISSION_ACTOR_COUNT; i++) {
        func_0204c124(info->actors[i], visible);
    }
}

static void func_ov036_021b5f64(FieldFMissionInfo *info) {
    func_ov036_021b5f48(info, FALSE);
    info->shown = FALSE;
    func_0204bfd4(info->unit, FALSE);
    info->scoreTimer = 0;
}

static BOOL func_ov036_021b5f84(FieldFMissionInfo *info) {
    return func_0201494c(info->festival) > 1;
}

static u32 func_ov036_021b5f98(u32 value, u8 digit) {
    u32 powers[8] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000 };

    return value % powers[digit + 1] / powers[digit];
}

static void func_ov036_021b5fd4(FieldFMissionInfo *info, u32 value, int digits, int firstActor, BOOL showZeros) {
    int i;
    BOOL started = FALSE;

    for (i = digits - 1; i >= 0; i--) {
        int digit = func_ov036_021b5f98(value, i);

        if (i > 0 && !showZeros) {
            BOOL visible = FALSE;

            if (digit > 0 || started) {
                visible = TRUE;
                started = TRUE;
            }
            func_0204c124(info->actors[firstActor], visible);
        } else {
            func_0204c124(info->actors[firstActor], TRUE);
        }
        func_0204c504(info->actors[firstActor], digit);
        firstActor++;
    }
}

static void func_ov036_021b6040(FieldFMissionInfo *info, BOOL force) {
    int time = func_020145dc(info->festival);
    int minutes = time / 60;

    if (time != info->seconds || force) {
        func_ov036_021b5fd4(info, minutes % 60, FMISSION_TIME_DIGITS, FMISSION_ACTOR_MINUTES, TRUE);
        func_ov036_021b5fd4(info, time % 60, FMISSION_TIME_DIGITS, FMISSION_ACTOR_SECONDS, TRUE);
        info->seconds = time;
        if (!force && time > 0 && time <= 10) {
            GFL_SndSEPlay(SEQ_SE_DECIDE5);
        }
    }
}

static void func_ov036_021b60a0(FieldFMissionInfo *info) {
    if (info->groupVisible) {
        func_0204c124(info->actors[FMISSION_ACTOR_GROUP_LABEL], TRUE);
        func_ov036_021b5fd4(info, func_0201482c(info->festival), FMISSION_COUNT_DIGITS, FMISSION_ACTOR_GROUP_DIGITS,
                            FALSE);
    }
}

static void func_ov036_021b60d0(FieldFMissionInfo *info, u8 actor, u16 x) {
    info->positions[actor].x = x;
    func_0204c140(info->actors[actor], &info->positions[actor], CLACT_SURFACE_MAIN);
}

// Where the mission's icon and the player's score go, the same whatever the mission and the score
static void func_ov036_021b60ec(int mode, int score, u16 *iconX, u16 *scoreX) {
    *iconX = 80;
    *scoreX = 44;
}

static void func_ov036_021b60f8(FieldFMissionInfo *info) {
    int i;
    int mode = info->config->info.unk10_6;
    int score = func_02014828(info->festival);
    u16 iconX;
    u16 scoreX;

    func_ov036_021b5fd4(info, score, FMISSION_COUNT_DIGITS, FMISSION_ACTOR_SCORE_DIGITS, FALSE);
    func_0204c488(info->actors[FMISSION_ACTOR_ICON], mode + FMISSION_SEQ_ICONS);
    func_0204c56c(info->actors[FMISSION_ACTOR_ICON]);
    func_ov036_021b60ec(mode, score, &iconX, &scoreX);
    func_ov036_021b60d0(info, FMISSION_ACTOR_ICON, iconX);
    for (i = 0; i < FMISSION_COUNT_DIGITS; i++) {
        func_ov036_021b60d0(info, i + FMISSION_ACTOR_SCORE_DIGITS, scoreX + i * 8);
    }
}

static void func_ov036_021b616c(FieldFMissionInfo *info) {
    info->groupVisible = func_ov036_021b5f84(info);
    // The bottom's actors, from the group's label or the score's, slide up, and the top's slide down
    if (info->groupVisible) {
        func_ov036_021b61e8(info, FMISSION_ACTOR_GROUP_LABEL, FMISSION_ACTOR_TIME_PANEL - FMISSION_ACTOR_GROUP_LABEL,
                            48);
    } else {
        func_ov036_021b61e8(info, FMISSION_ACTOR_SCORE_LABEL, FMISSION_ACTOR_TIME_PANEL - FMISSION_ACTOR_SCORE_LABEL,
                            48);
    }
    func_ov036_021b61e8(info, FMISSION_ACTOR_TIME_PANEL, FMISSION_ACTOR_COUNT - FMISSION_ACTOR_TIME_PANEL, -24);
    func_ov036_021b6368(info);
    func_ov036_021b6410(info);
    func_ov036_021b6040(info, TRUE);
    func_ov036_021b60a0(info);
    func_ov036_021b60f8(info);
    info->shown = TRUE;
    func_0204bfd4(info->unit, TRUE);
}

static void func_ov036_021b61e8(FieldFMissionInfo *info, u8 first, u8 count, int yOffset) {
    FieldFMissionSlide *slide;
    int i;
    TCBEx *task = GFL_TCBExMgrAddTask(info->tasks, func_ov036_021b62d4, sizeof(FieldFMissionSlide), 0);

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
        func_0204c140(info->actors[actor], &pos, CLACT_SURFACE_MAIN);
        // The last two, a highlight, stay hidden
        if (i < count - 2) {
            func_0204c124(info->actors[actor], TRUE);
        }
    }
    slide->step = FX_Div(FX32_CONST(yOffset), FX32_CONST(slide->timer));
}

static void func_ov036_021b62d4(TCBEx *task, void *data) {
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
            func_0204c140(info->actors[i], &pos, CLACT_SURFACE_MAIN);
        }
    } else {
        for (i = slide->first; i < slide->first + slide->count; i++) {
            func_0204c140(info->actors[i], &info->positions[i], CLACT_SURFACE_MAIN);
        }
        GFL_TCBExRequestEnd(task);
    }
}

static void func_ov036_021b6368(FieldFMissionInfo *info) {
    u32 score = LinkFestival_GetTotalScore(info->festival);

    if (func_02014828(info->festival) < score) {
        info->scoreTimer = FMISSION_HIGHLIGHT_FRAMES;
        func_02014830(info->festival, score);
        GFL_SndSEPlay(SEQ_SE_SW_FES_04);
        if (!info->scoreAnim) {
            TCBEx *task;
            FieldFMissionBlink *blink;

            info->scoreAnim = TRUE;
            task = GFL_TCBExMgrAddTask(info->tasks, func_ov036_021b63dc, sizeof(FieldFMissionBlink), 0);
            blink = GFL_TCBExGetData(task);
            MI_CpuFill8(blink, 0, sizeof(FieldFMissionBlink));
            blink->info = info;
            func_0204c124(info->actors[FMISSION_ACTOR_SCORE_GAIN], TRUE);
            func_0204c124(info->actors[FMISSION_ACTOR_SCORE_FLASH], TRUE);
        }
    }
}

static void func_ov036_021b63dc(TCBEx *task, void *data) {
    FieldFMissionBlink *blink = data;
    FieldFMissionInfo *info = blink->info;

    if (info->scoreTimer != 0) {
        info->scoreTimer--;
        return;
    }
    func_0204c124(info->actors[FMISSION_ACTOR_SCORE_GAIN], FALSE);
    func_0204c124(info->actors[FMISSION_ACTOR_SCORE_FLASH], FALSE);
    info->scoreAnim = FALSE;
    GFL_TCBExRequestEnd(task);
}

static void func_ov036_021b6410(FieldFMissionInfo *info) {
    if (func_02014838(info->festival)) {
        info->timeTimer = FMISSION_HIGHLIGHT_FRAMES;
        func_0201483c(info->festival);
        if (!info->timeAnim) {
            TCBEx *task;
            FieldFMissionBlink *blink;

            info->timeAnim = TRUE;
            task = GFL_TCBExMgrAddTask(info->tasks, func_ov036_021b6474, sizeof(FieldFMissionBlink), 0);
            blink = GFL_TCBExGetData(task);
            MI_CpuFill8(blink, 0, sizeof(FieldFMissionBlink));
            blink->info = info;
            func_0204c124(info->actors[FMISSION_ACTOR_TIME_GAIN], TRUE);
            func_0204c124(info->actors[FMISSION_ACTOR_TIME_FLASH], TRUE);
        }
    }
}

static void func_ov036_021b6474(TCBEx *task, void *data) {
    FieldFMissionBlink *blink = data;
    FieldFMissionInfo *info = blink->info;

    if (info->timeTimer != 0) {
        info->timeTimer--;
        return;
    }
    func_0204c124(info->actors[FMISSION_ACTOR_TIME_GAIN], FALSE);
    func_0204c124(info->actors[FMISSION_ACTOR_TIME_FLASH], FALSE);
    info->timeAnim = FALSE;
    GFL_TCBExRequestEnd(task);
}
