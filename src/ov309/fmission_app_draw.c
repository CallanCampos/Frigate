#include "types.h"
#include "app/fmission_app.h"
#include "app/fmission_app_gra.h"
#include "app/ui/print_msg.h"
#include "constants/arc.h"
#include "field/fest_mission_data.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "pml/item.h"
#include "save/bag.h"
#include "save/player_info.h"
#include "system/app_keycursor.h"
#include "system/app_menu_common.h"
#include "system/app_printsys_common.h"
#include "system/bmp_winframe.h"
#include "system/fest_mission.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wordset.h"
#include "text/system/title_4.h"

// The Funfest mission screen's BGs, windows, actors and messages. The ROM has no name for this file;
// "fmission_app_draw.c" is a guess

#define ACTOR_AUTO_ANIM 0x1
#define ACTOR_FLIP_Y 0x2
#define ACTOR_FLIP_X 0x4

#define DIGIT_COUNT 4

// A palette or characters of the screen's archive, and where they go
typedef struct {
    u8 target;
    u8 file;
    u8 offset;
    u8 count;
} FMissionAppDrawBGFile;

// A screen of the screen's archive, for one of the screen's modes
typedef struct {
    u8 target;
    u8 file;
    u8 offset;
    u8 mode;
} FMissionAppDrawScreenFile;

typedef struct {
    s16 file;
    s16 resource;
    u16 offset;
    u16 count;
} FMissionAppDrawObjFile;

// The resources of a group of actors, how many they are and which they are
typedef struct {
    s16 palette;
    s16 chars;
    s16 cells;
    s16 count;
    s16 type;
} FMissionAppDrawActorGroup;

typedef struct {
    u8 sequence;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 unused4;
    u8 flags;
    u8 pos;
    u8 pad;
} FMissionAppDrawActorSetup;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} FMissionAppDrawWindowSetup;

static void FMissionAppDraw_LoadBG(FMissionApp *work, u32 mode);
static void FMissionAppDraw_LoadTouchBar(FMissionApp *work, u32 bg, u32 palette);
static void FMissionAppDraw_InitWindows(FMissionApp *work, u32 mode);
static void FMissionAppDraw_AddActors(FMissionApp *work, u32 vramType, const FMissionAppDrawActorSetup *setups,
                                      const FMissionAppDrawActorGroup *group);
static void FMissionAppDraw_LoadObjResources(FMissionApp *work, u32 vramType, const FMissionAppDrawObjFile *files);
static void FMissionAppDraw_LoadCommonObjResources(FMissionApp *work, u32 vramType);
static void FMissionAppDraw_InitObj(FMissionApp *work);
static void FMissionAppDraw_FreeObj(FMissionApp *work);
static void FMissionAppDraw_StartMessage(FMissionApp *work, int window, int message);

// The level icon's animations
static const s16 sFMissionAppDrawLevelSequences[] = { 2, 3, 4, 5 };
// The return button
static const TouchRect sFMissionAppDrawTouchRects[] = { { 168, 192, 232, 255 }, { TOUCH_RECT_END, 0, 0, 0 } };
static const FMissionAppDrawActorGroup sFMissionAppDrawUpperActorGroup = { 0, 1, 2, 20, FMISSION_APP_ACTOR_UPPER };
static const FMissionAppDrawActorGroup sFMissionAppDrawRecordActorGroup = { 6, 7, 8, 1, FMISSION_APP_ACTOR_RECORD };
static const FMissionAppDrawActorGroup sFMissionAppDrawReturnActorGroup = { 3, 4, 5, 20, FMISSION_APP_ACTOR_LOWER };

static const FMissionAppDrawBGFile sFMissionAppDrawMissionPalettes[] = {
    { PALTYPE_SUB_BG, 4, 0, 2 },
    { PALTYPE_MAIN_BG, 6, 0, 2 },
    { 0xff },
};

static const FMissionAppDrawBGFile sFMissionAppDrawResultPalettes[] = {
    { PALTYPE_SUB_BG, 4, 2, 2 },
    { PALTYPE_MAIN_BG, 6, 2, 2 },
    { 0xff },
};

static const FMissionAppDrawActorSetup sFMissionAppDrawRecordActorSetups[] = {
    { 8, 64, 1, FALSE, 0, ACTOR_AUTO_ANIM, 12 },
    { 0xff },
};

static const FMissionAppDrawActorSetup sFMissionAppDrawReturnActorSetups[] = {
    { 1, 48, 1, TRUE, 0, ACTOR_AUTO_ANIM, 11 },
    { 0xff },
};

// The result's messages
static const s16 sFMissionAppDrawMessages[] = {
    Title4_Text_MissionAccomplished,
    Title4_Text_MissionFailed,
    Title4_Text_Obtained,
    Title4_Text_TooBadThereNo,
    Title4_Text_DoubleScoreBonusActivated,
    Title4_Text_SFeelingsResonatedEntree,
    Title4_Text_EveryonesFeelingsResonatedEntree,
    Title4_Text_White,
    Title4_Text_Black,
};

static const FMissionAppDrawBGFile sFMissionAppDrawChars[] = {
    { 7, 5 }, { 6, 5 }, { 3, 7 }, { 2, 7 }, { 0xff },
};

static const FMissionAppDrawObjFile sFMissionAppDrawObjFiles[] = {
    { 0, 0, 0, 3 },
    { 1, 1 },
    { 2, 2 },
    { 3, -1 },
};

static const FMissionAppDrawObjFile sFMissionAppDrawRecordObjFiles[] = {
    { 0, 6, 2 * 32, 3 },
    { 1, 7 },
    { 2, 8 },
    { 3, -1 },
};

static const FMissionAppDrawScreenFile sFMissionAppDrawScreens[] = {
    { 7, 10, 0, FMISSION_APP_MODE_MISSION }, { 6, 8, 0, FMISSION_APP_MODE_MISSION },
    { 3, 13, 0, FMISSION_APP_MODE_MISSION }, { 2, 11, 0, FMISSION_APP_MODE_MISSION },
    { 7, 10, 0, FMISSION_APP_MODE_RESULT },  { 6, 9, 0, FMISSION_APP_MODE_RESULT },
    { 3, 13, 0, FMISSION_APP_MODE_RESULT },  { 2, 12, 0, FMISSION_APP_MODE_RESULT },
    { 0xff },
};

static const FMissionAppDrawActorSetup sFMissionAppDrawMissionActorSetups[] = {
    { 6, 64, 1, TRUE, 0, ACTOR_AUTO_ANIM, 0 },  { 7, 64, 1, TRUE, 0, ACTOR_AUTO_ANIM, 1 },
    { 5, 64, 1, TRUE, 0, ACTOR_AUTO_ANIM, 2 },  { 0, 64, 1, TRUE, 0, 0, 3 },
    { 0, 64, 1, TRUE, 0, 0, 3 },                { 1, 64, 1, TRUE, 0, 0, 3 },
    { 0, 64, 1, TRUE, 0, 0, 3 },                { 0, 64, 1, TRUE, 0, 0, 3 },
    { 0, 64, 1, TRUE, 0, 0, 4 },                { 0, 64, 1, TRUE, 0, 0, 4 },
    { 0, 64, 1, TRUE, 0, 0, 4 },                { 0, 64, 1, TRUE, 0, 0, 4 },
    { 0xff },
};

static const FMissionAppDrawActorSetup sFMissionAppDrawResultActorSetups[] = {
    { 7, 64, 1, FALSE, 0, ACTOR_AUTO_ANIM, 5 }, { 5, 64, 1, FALSE, 0, ACTOR_AUTO_ANIM, 6 },
    { 0, 64, 1, FALSE, 0, 0, 8 },               { 0, 64, 1, FALSE, 0, 0, 8 },
    { 1, 64, 1, FALSE, 0, 0, 8 },               { 0, 64, 1, FALSE, 0, 0, 8 },
    { 0, 64, 1, FALSE, 0, 0, 8 },               { 0, 64, 1, FALSE, 0, 0, 7 },
    { 0, 64, 1, FALSE, 0, 0, 7 },               { 0, 64, 1, FALSE, 0, 0, 7 },
    { 0, 64, 1, FALSE, 0, 0, 7 },               { 8, 64, 1, FALSE, 0, ACTOR_AUTO_ANIM, 9 },
    { 8, 64, 1, FALSE, 0, ACTOR_AUTO_ANIM, 10 }, { 0xff },
};

static const FMissionAppDrawWindowSetup sFMissionAppDrawMissionWindows[] = {
    { 4, 1, 0, 24, 3, 8 },  { 4, 5, 7, 12, 2, 8 },  { 4, 5, 10, 12, 2, 8 }, { 4, 3, 13, 20, 2, 8 },
    { 4, 3, 16, 12, 2, 8 }, { 4, 23, 10, 6, 2, 8 }, { 4, 23, 13, 6, 2, 8 }, { 4, 23, 16, 6, 2, 8 },
    { 0, 3, 1, 8, 2, 8 },   { 0, 3, 4, 7, 2, 8 },   { 0, 3, 7, 10, 2, 8 },  { 0, 13, 7, 8, 2, 8 },
    { 0, 3, 13, 28, 8, 8 }, { 0, 21, 1, 8, 2, 8 },  { 0, 10, 4, 19, 2, 8 }, { 0, 21, 7, 8, 2, 8 },
    { 5, 2, 21, 28, 2, 8 }, { 5, 34, 21, 28, 2, 8 }, { 0xff },
};

static const FMissionAppDrawWindowSetup sFMissionAppDrawResultWindows[] = {
    { 4, 1, 0, 24, 3, 8 },  { 4, 5, 5, 12, 2, 8 },  { 4, 3, 8, 20, 2, 8 },   { 4, 3, 11, 12, 2, 8 },
    { 4, 3, 14, 12, 2, 8 }, { 4, 23, 5, 6, 2, 8 },  { 4, 23, 8, 6, 2, 8 },   { 4, 23, 11, 6, 2, 8 },
    { 0, 3, 1, 8, 2, 8 },   { 0, 3, 4, 7, 2, 8 },   { 0, 3, 7, 10, 2, 8 },   { 0, 13, 7, 8, 2, 8 },
    { 0, 3, 12, 8, 2, 8 },  { 0, 3, 15, 12, 2, 8 }, { 0, 3, 18, 10, 2, 8 },  { 0, 21, 1, 8, 2, 8 },
    { 0, 10, 4, 19, 2, 8 }, { 0, 21, 7, 8, 2, 8 },  { 0, 13, 12, 16, 2, 8 }, { 0, 23, 15, 6, 2, 8 },
    { 0, 13, 18, 16, 2, 8 }, { 5, 1, 19, 30, 4, 8 }, { 0xff },
};

static const FMissionAppDrawBGFile *sFMissionAppDrawPalettes[FMISSION_APP_MODE_COUNT] = {
    sFMissionAppDrawMissionPalettes,
    sFMissionAppDrawResultPalettes,
};

static const FMissionAppDrawWindowSetup *sFMissionAppDrawWindows[FMISSION_APP_MODE_COUNT] = {
    sFMissionAppDrawMissionWindows,
    sFMissionAppDrawResultWindows,
};

static const FMissionAppDrawActorSetup *sFMissionAppDrawActorSetups[] = {
    sFMissionAppDrawMissionActorSetups,
    sFMissionAppDrawResultActorSetups,
    sFMissionAppDrawRecordActorSetups,
};

void FMissionAppDraw_Init(FMissionApp *work, u32 mode) {
    FMissionAppDraw_LoadBG(work, mode);
    FMissionAppDraw_LoadTouchBar(work, 1, 6);
    FMissionAppDraw_InitWindows(work, mode);
    FMissionAppDraw_InitObj(work);
}

void FMissionAppDraw_Free(FMissionApp *work) {
    int i;

    for (i = 0; i < FMISSION_APP_WINDOW_COUNT; i++) {
        if (work->windows[i].window != NULL) {
            BmpWin_Free(work->windows[i].window);
            work->windows[i].window = NULL;
        }
    }
    if (work->message != NULL) {
        GFL_StrBufFree(work->message);
        work->message = NULL;
    }
    if (work->stream != NULL) {
        func_020223cc(work->stream);
        work->stream = NULL;
    }
    FMissionAppDraw_FreeObj(work);
}

static void FMissionAppDraw_LoadBG(FMissionApp *work, u32 mode) {
    const FMissionAppDrawBGFile *palette = sFMissionAppDrawPalettes[mode];
    const FMissionAppDrawBGFile *chars = sFMissionAppDrawChars;
    const FMissionAppDrawScreenFile *screen = sFMissionAppDrawScreens;
    int i;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_FMISSION_APP, work->heapId);

    for (i = 0; i < 8; i++) {
        if (palette->target == 0xff) {
            break;
        }
        GFL_G2DIOLoadArcNCLR(arc, palette->file, palette->target, palette->offset * 32, 0, palette->count * 32,
                             work->heapId);
        palette++;
    }
    for (i = 0; i < 8; i++) {
        if (chars->target == 0xff) {
            break;
        }
        GFL_BGSysLoadArcNCGRStatic(arc, chars->file, chars->target, chars->offset, 0, TRUE, work->heapId);
        chars++;
    }
    for (i = 0; i < 8; i++) {
        if (screen->target == 0xff) {
            break;
        }
        if (screen->mode == mode) {
            loadBGScrToVramByFileNoReserveNegAlign(arc, screen->file, screen->target, screen->offset, 0, TRUE,
                                                   work->heapId);
        }
        screen++;
    }
    GFL_ArcToolFree(arc);
}

// Loads the touch bar's BG and gives its screen a palette
static void FMissionAppDraw_LoadTouchBar(FMissionApp *work, u32 bg, u32 palette) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), work->heapId);
    u16 *screen;
    int x;
    int y;

    GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), PALTYPE_MAIN_BG, palette * 32, 32, work->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, func_0202d824(), bg, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, func_0202d828(), bg, 0, 0, FALSE, work->heapId);
    screen = GFL_BGSysIsScrHeapExists(bg);
    for (y = 0; y < 24; y++) {
        for (x = 0; x < 32; x++) {
            screen[y * 32 + x] += (u16)((palette & 0xf) << 12);
        }
    }
    GFL_BGSysLoadScr(bg);
    GFL_ArcToolFree(arc);
}

static void FMissionAppDraw_InitWindows(FMissionApp *work, u32 mode) {
    int i;
    const FMissionAppDrawWindowSetup *setup = sFMissionAppDrawWindows[mode];
    ArcTool *arc;

    for (i = 0; i < FMISSION_APP_WINDOW_COUNT; i++) {
        if (setup->bg == 0xff) {
            break;
        }
        work->windows[i].window =
            BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, TRUE);
        BmpWin_FlushMap(work->windows[i].window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(work->windows[i].window));
        setup++;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, PALTYPE_MAIN_BG, 8 * 32, 32, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, PALTYPE_SUB_BG, 8 * 32, 32, work->heapId);
    GFL_ArcToolFree(arc);
}

void FMissionAppDraw_Print(FMissionApp *work, int window, u32 x, u32 y, StrBuf *strbuf, u32 align, u16 color) {
    PrintStrAligned(&work->windows[window], work->printQueue, x, y, strbuf, work->font, color, align);
}

void FMissionAppDraw_ClearWindow(FMissionApp *work, int window) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[window].window), 0);
    work->windows[window].flushPending = TRUE;
}

s16 FMissionAppDraw_GetPos(FMissionApp *work, u32 index) {
    if (index >= FMISSION_APP_POS_COUNT * 2) {
        return 0;
    }
    return work->positions[index];
}

static void FMissionAppDraw_AddActors(FMissionApp *work, u32 vramType, const FMissionAppDrawActorSetup *setups,
                                      const FMissionAppDrawActorGroup *group) {
    int i;
    ClActUnit *unit = FMissionAppGra_GetClActUnit(work->graphic);
    ClActorSetup setup;
    ClActor **actors;
    s16 count;
    s16 palette;
    s16 chars;
    s16 cells;
    s16 type;

    sys_memset(&setup, 0, sizeof(setup));
    palette = group->palette;
    chars = group->chars;
    cells = group->cells;
    count = group->count;
    type = group->type;
    if (type == FMISSION_APP_ACTOR_UPPER) {
        actors = work->upperActors;
    } else if (type == FMISSION_APP_ACTOR_LOWER) {
        actors = work->lowerActors;
    } else if (type == FMISSION_APP_ACTOR_RECORD) {
        actors = work->recordActors;
    }
    for (i = 0; i < count; i++) {
        u32 pos;
        s16 x;
        s16 y;
        BOOL visible;
        BOOL autoAnim;

        if (setups->sequence == 0xff) {
            break;
        }
        pos = setups->pos * 2;
        x = FMissionAppDraw_GetPos(work, pos);
        y = FMissionAppDraw_GetPos(work, pos + 1);
        setup.x = x;
        setup.y = y;
        setup.sequence = setups->sequence;
        setup.priority = setups->priority;
        setup.bgPriority = setups->bgPriority;
        actors[i] = func_0204c040(unit, work->resources[chars], work->resources[palette], work->resources[cells],
                                  &setup, vramType, work->heapId);
        visible = FALSE;
        if (setups->visible == TRUE) {
            visible = TRUE;
        }
        func_0204c124(actors[i], visible);
        autoAnim = FALSE;
        if (setups->flags & ACTOR_AUTO_ANIM) {
            autoAnim = TRUE;
        }
        func_0204c520(actors[i], autoAnim);
        if (setups->flags & ACTOR_FLIP_Y) {
            func_0204c2b0(actors[i], 1, TRUE);
        }
        if (setups->flags & ACTOR_FLIP_X) {
            func_0204c2b0(actors[i], 0, TRUE);
        }
        setups++;
    }
}

// Loads a palette, characters, and cells and animations of the screen's archive
static void FMissionAppDraw_LoadObjResources(FMissionApp *work, u32 vramType, const FMissionAppDrawObjFile *files) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_FMISSION_APP, work->heapId);

    FMissionAppGra_GetClActUnit(work->graphic);
    work->resources[files[0].resource] =
        func_0204bbb8(arc, files[0].file, vramType, files[0].offset, 0, files[0].count, work->heapId);
    work->resources[files[1].resource] = func_0204b81c(arc, files[1].file, TRUE, vramType, work->heapId);
    work->resources[files[2].resource] = func_0204bde0(arc, files[2].file, files[3].file, work->heapId);
    GFL_ArcToolFree(arc);
}

// Loads the touch bar's icons, for the return button
static void FMissionAppDraw_LoadCommonObjResources(FMissionApp *work, u32 vramType) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), work->heapId);
    u32 palette;
    u32 chars;
    u32 cells;
    u32 anims;

    FMissionAppGra_GetClActUnit(work->graphic);
    palette = func_0202d810();
    chars = func_0202d814();
    cells = func_0202d818(2);
    anims = func_0202d81c(2);
    work->resources[3] = func_0204bbb8(arc, palette, vramType, 0, 0, 4, work->heapId);
    work->resources[4] = func_0204b81c(arc, chars, FALSE, vramType, work->heapId);
    work->resources[5] = func_0204bde0(arc, cells, anims, work->heapId);
    GFL_ArcToolFree(arc);
}

static void FMissionAppDraw_InitObj(FMissionApp *work) {
    ClActUnit *unit = FMissionAppGra_GetClActUnit(work->graphic);
    const FMissionAppDrawActorSetup *setups = sFMissionAppDrawActorSetups[work->mode];

    FMissionAppDraw_LoadObjResources(work, CLACT_VRAM_SUB, sFMissionAppDrawObjFiles);
    FMissionAppDraw_LoadCommonObjResources(work, CLACT_VRAM_MAIN);
    FMissionAppDraw_AddActors(work, CLACT_VRAM_SUB, setups, &sFMissionAppDrawUpperActorGroup);
    FMissionAppDraw_AddActors(work, CLACT_VRAM_MAIN, sFMissionAppDrawReturnActorSetups,
                              &sFMissionAppDrawReturnActorGroup);
    if (work->mode == FMISSION_APP_MODE_RESULT) {
        FMissionAppDraw_LoadObjResources(work, CLACT_VRAM_MAIN, sFMissionAppDrawRecordObjFiles);
        FMissionAppDraw_AddActors(work, CLACT_VRAM_MAIN, sFMissionAppDrawActorSetups[2],
                                  &sFMissionAppDrawRecordActorGroup);
    }
    func_02042ba8(FALSE, work->heapId);
}

static void FMissionAppDraw_FreeObj(FMissionApp *work) {
    int i;

    for (i = 0; i < FMISSION_APP_UPPER_ACTOR_COUNT; i++) {
        if (work->upperActors[i] != NULL) {
            func_0204c108(work->upperActors[i]);
            work->upperActors[i] = NULL;
        }
    }
    for (i = 0; i < FMISSION_APP_LOWER_ACTOR_COUNT; i++) {
        if (work->lowerActors[i] != NULL) {
            func_0204c108(work->lowerActors[i]);
            work->lowerActors[i] = NULL;
        }
    }
    if (work->recordActors[0] != NULL) {
        func_0204c108(work->recordActors[0]);
        work->recordActors[0] = NULL;
    }
    func_0204bcd0(work->resources[0]);
    func_0204b98c(work->resources[1]);
    func_0204be64(work->resources[2]);
    func_0204bcd0(work->resources[3]);
    func_0204b98c(work->resources[4]);
    func_0204be64(work->resources[5]);
    if (work->mode == FMISSION_APP_MODE_RESULT) {
        func_0204bcd0(work->resources[6]);
        func_0204b98c(work->resources[7]);
        func_0204be64(work->resources[8]);
    }
}

void FMissionAppDraw_SetActorAnim(FMissionApp *work, int type, int index, u32 sequence) {
    if (type == FMISSION_APP_ACTOR_UPPER) {
        func_0204c488(work->upperActors[index], (u16)sequence);
        func_0204c520(work->upperActors[index], TRUE);
    } else if (type == FMISSION_APP_ACTOR_LOWER) {
        func_0204c488(work->lowerActors[index], (u16)sequence);
        func_0204c520(work->lowerActors[index], TRUE);
    } else if (type == FMISSION_APP_ACTOR_RECORD) {
        func_0204c488(work->recordActors[index], (u16)sequence);
        func_0204c520(work->recordActors[index], TRUE);
    }
}

void FMissionAppDraw_SetActorVisible(FMissionApp *work, int type, int index, BOOL visible) {
    if (type == FMISSION_APP_ACTOR_UPPER) {
        func_0204c124(work->upperActors[index], visible);
    } else if (type == FMISSION_APP_ACTOR_LOWER) {
        func_0204c124(work->lowerActors[index], visible);
    } else if (type == FMISSION_APP_ACTOR_RECORD) {
        func_0204c124(work->recordActors[index], visible);
    }
}

BOOL FMissionAppDraw_IsActorAnimActive(FMissionApp *work, int type, int index) {
    BOOL active = TRUE;

    if (type == FMISSION_APP_ACTOR_UPPER) {
        if (!func_0204c560(work->upperActors[index])) {
            active = FALSE;
        }
    } else if (type == FMISSION_APP_ACTOR_LOWER) {
        if (!func_0204c560(work->lowerActors[index])) {
            active = FALSE;
        }
    } else if (type == FMISSION_APP_ACTOR_RECORD) {
        if (!func_0204c560(work->recordActors[index])) {
            active = FALSE;
        }
    }
    return active;
}

void FMissionAppDraw_SetActorPalette(FMissionApp *work, int type, int index, u32 palette) {
    if (type == FMISSION_APP_ACTOR_UPPER) {
        func_0204c378(work->upperActors[index], (u8)palette, 1);
    } else if (type == FMISSION_APP_ACTOR_LOWER) {
        func_0204c378(work->lowerActors[index], (u8)palette, 1);
    } else if (type == FMISSION_APP_ACTOR_RECORD) {
        func_0204c378(work->recordActors[index], (u8)palette, 1);
    }
}

void FMissionAppDraw_SetNumber(FMissionApp *work, int type, int first, int number, int digits, BOOL pad, int x, int y) {
    int values[DIGIT_COUNT];
    ClActorPos pos;
    int i;

    for (i = 0; i < DIGIT_COUNT; i++) {
        values[i] = -1;
    }
    pos.x = x;
    pos.y = y;
    for (i = 0; i < digits; i++) {
        values[i] = number % 10;
        number /= 10;
        if (number == 0) {
            break;
        }
    }
    for (i = 0; i < DIGIT_COUNT; i++) {
        BOOL visible = TRUE;

        if (!pad && values[i] == -1) {
            visible = FALSE;
            values[i] = 0;
        }
        func_0204c124(work->upperActors[first + DIGIT_COUNT - 1 - i], visible);
        func_0204c504(work->upperActors[first + DIGIT_COUNT - 1 - i], values[i]);
        pos.x = x + (DIGIT_COUNT - 1 - i) * 8;
        func_0204c140(work->upperActors[first + DIGIT_COUNT - 1 - i], &pos, CLACT_VRAM_SUB);
    }
}

// `first` is a long, the SDK's s32: MWCC then shares one `first * 4` among the position, the digit loops and the
// colon, as the original does
void FMissionAppDraw_SetTime(FMissionApp *work, int type, long first, int minutes, int seconds) {
    int values[4];
    ClActorPos pos;
    int i;
    int x;
    int number;

    for (i = 0; i < 4; i++) {
        values[i] = 0;
    }
    func_0204c178(work->upperActors[first], &pos, CLACT_VRAM_SUB);
    x = pos.x;
    number = minutes;
    for (i = 0; i < 2; i++) {
        values[1 - i] = number % 10;
        number /= 10;
        if (number == 0) {
            break;
        }
    }
    number = seconds;
    for (i = 0; i < 2; i++) {
        values[3 - i] = number % 10;
        number /= 10;
        if (number == 0) {
            break;
        }
    }
    for (i = 0; i < 2; i++) {
        func_0204c124(work->upperActors[first + i], TRUE);
        func_0204c504(work->upperActors[first + i], values[i]);
        pos.x = x + i * 8;
        func_0204c140(work->upperActors[first + i], &pos, CLACT_VRAM_SUB);
    }
    func_0204c124(work->upperActors[first + 2], TRUE);
    pos.x = x + 16;
    func_0204c140(work->upperActors[first + 2], &pos, CLACT_VRAM_SUB);
    for (i = 0; i < 2; i++) {
        func_0204c124(work->upperActors[first + 3 + i], TRUE);
        func_0204c504(work->upperActors[first + 3 + i], values[2 + i]);
        pos.x = x + (i + 3) * 8;
        func_0204c140(work->upperActors[first + 3 + i], &pos, CLACT_VRAM_SUB);
    }
}

void FMissionAppDraw_SetLevelIcon(FMissionApp *work, int index) {
    u32 level = work->param->missionConfig->info.level;

    func_0204c124(work->upperActors[index], TRUE);
    FMissionAppDraw_SetActorAnim(work, FMISSION_APP_ACTOR_UPPER, index, sFMissionAppDrawLevelSequences[level]);
}

int FMissionAppDraw_CheckTouch(FMissionApp *work) {
    return func_0203da0c(sFMissionAppDrawTouchRects);
}

// Makes one of the result's messages, gives the player what it says, and starts printing it
// Sets an item message's count and returns whether the item's name is to be plural
static inline BOOL FMissionAppDraw_SetCountWord(FMissionApp *work, int count) {
    BOOL plural = TRUE;

    WordSetNumber(work->wordSet, 2, count, 3, 0, TRUE);
    if (count == 1) {
        plural = FALSE;
    }
    return plural;
}

static void FMissionAppDraw_StartMessage(FMissionApp *work, int window, int message) {
    if (work->stream != NULL) {
        func_020223cc(work->stream);
        work->stream = NULL;
    }
    if (work->message != NULL) {
        GFL_StrBufFree(work->message);
        work->message = NULL;
    }
    if (message == FMISSION_APP_MESSAGE_ITEM) {
        PlayerInfo *playerInfo = GetGameDataPlayerInfo(work->param->gameData);
        StrBuf *name = GFL_StrBufCreate(64, work->heapId);
        int count;
        int room;
        StrBuf *format;

        textCopy(playerInfo->name, name);
        func_0202437c(work->wordSet, 0, name, 0, 1, 2);
        room = FMissionAppDraw_GetItemRoom(work, work->result.item);
        count = work->result.itemCount + work->result.bonusCount;
        if (count > room) {
            count = room;
        }
        loadItemText(work->wordSet, 1, work->result.item, FMissionAppDraw_SetCountWord(work, count), FALSE);
        work->message = GFL_StrBufCreate(256, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppDrawMessages[message]);
        GFL_WordSetFormatStrbuf(work->wordSet, work->message, format);
        GFL_StrBufFree(name);
        GFL_StrBufFree(format);
        BagSave_AddItem(GameData_GetBag(work->param->gameData), work->result.item, count, work->heapId);
    } else if (message == FMISSION_APP_MESSAGE_ITEM_NO_ROOM) {
        StrBuf *format;

        loadItemsNameToStrbuf(work->wordSet, 0, work->result.item);
        work->message = GFL_StrBufCreate(256, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppDrawMessages[message]);
        GFL_WordSetFormatStrbuf(work->wordSet, work->message, format);
        GFL_StrBufFree(format);
    } else if (message == FMISSION_APP_MESSAGE_ITEM2) {
        PlayerInfo *playerInfo = GetGameDataPlayerInfo(work->param->gameData);
        StrBuf *name = GFL_StrBufCreate(64, work->heapId);
        int count;
        int room;
        StrBuf *format;

        textCopy(playerInfo->name, name);
        func_0202437c(work->wordSet, 0, name, 0, 1, 2);
        room = FMissionAppDraw_GetItemRoom(work, work->result.item2);
        count = work->result.item2Count;
        if (count > room) {
            count = room;
        }
        loadItemText(work->wordSet, 1, work->result.item2, FMissionAppDraw_SetCountWord(work, count), FALSE);
        work->message = GFL_StrBufCreate(256, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Obtained);
        GFL_WordSetFormatStrbuf(work->wordSet, work->message, format);
        GFL_StrBufFree(name);
        GFL_StrBufFree(format);
        BagSave_AddItem(GameData_GetBag(work->param->gameData), work->result.item2, count, work->heapId);
    } else if (message == FMISSION_APP_MESSAGE_ITEM2_NO_ROOM) {
        StrBuf *format;

        loadItemsNameToStrbuf(work->wordSet, 0, work->result.item2);
        work->message = GFL_StrBufCreate(256, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_TooBadThereNo);
        GFL_WordSetFormatStrbuf(work->wordSet, work->message, format);
        GFL_StrBufFree(format);
    } else if (message == FMISSION_APP_MESSAGE_LEVEL_ALONE) {
        PlayerInfo *playerInfo = GetGameDataPlayerInfo(work->param->gameData);
        StrBuf *name = GFL_StrBufCreate(64, work->heapId);
        StrBuf *side;
        StrBuf *format;

        textCopy(playerInfo->name, name);
        func_0202437c(work->wordSet, 0, name, 0, 1, 2);
        GFL_StrBufFree(name);
        side = GFL_MsgDataLoadStrbufNew(work->msgData[0],
                                        sFMissionAppDrawMessages[work->result.isBlack + FMISSION_APP_MESSAGE_WHITE]);
        func_0202437c(work->wordSet, 1, side, 0, 1, 2);
        WordSetNumber(work->wordSet, 2, work->result.levels, 3, 0, TRUE);
        work->message = GFL_StrBufCreate(256, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppDrawMessages[message]);
        GFL_WordSetFormatStrbuf(work->wordSet, work->message, format);
        GFL_StrBufFree(side);
        GFL_StrBufFree(format);
    } else if (message == FMISSION_APP_MESSAGE_LEVEL) {
        StrBuf *side = GFL_MsgDataLoadStrbufNew(
            work->msgData[0], sFMissionAppDrawMessages[work->result.isBlack + FMISSION_APP_MESSAGE_WHITE]);
        StrBuf *format;

        func_0202437c(work->wordSet, 0, side, 0, 1, 2);
        WordSetNumber(work->wordSet, 1, work->result.levels, 3, 0, TRUE);
        work->message = GFL_StrBufCreate(256, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppDrawMessages[message]);
        GFL_WordSetFormatStrbuf(work->wordSet, work->message, format);
        GFL_StrBufFree(side);
        GFL_StrBufFree(format);
    } else {
        work->message = GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppDrawMessages[message]);
    }
    work->stream = func_02022268(work->windows[window].window, 0, 0, work->message, work->font, func_02017bcc(),
                                 work->tcbManager, 4, work->heapId, 15);
}

BOOL FMissionAppDraw_UpdateMessage(FMissionApp *work) {
    if (work->stream == NULL) {
        return FALSE;
    }
    KeyCursor_Update(work->keyCursor, work->stream, work->windows[FMISSION_APP_WINDOW_MESSAGE].window);
    if (AppPrintsysCommon_Update(&work->printCommon, work->stream) != TRUE) {
        return TRUE;
    }
    return FALSE;
}

void FMissionAppDraw_ShowMessage(FMissionApp *work, int window, int message, BOOL stream) {
    LoadSysMsgBox(5, 1, 4, 0, work->heapId);
    BmpWin_DrawFrame(work->windows[window].window, FALSE, 1, 4);
    BmpWin_FlushMap(work->windows[window].window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(work->windows[window].window));
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[window].window), 15);
    BmpWin_FlushChar(work->windows[window].window);
    if (!stream) {
        StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppDrawMessages[message]);

        FMissionAppDraw_Print(work, window, 120, 0, strbuf, PRINT_ALIGN_CENTER, PRINT_COLOR(1, 2, 0));
        GFL_StrBufFree(strbuf);
    } else {
        FMissionAppDraw_StartMessage(work, window, message);
    }
}

int FMissionAppDraw_GetItemRoom(FMissionApp *work, u32 item) {
    if (item == 0) {
        return 0;
    }
    return PML_ItemGetMaxStorageCount(item)
         - BagSave_GetItemCountByID(GameData_GetBag(work->param->gameData), item, work->heapId);
}

void FMissionAppDraw_SetKeyCursor(FMissionApp *work, BOOL shown) {
    if (shown == TRUE) {
        KeyCursor_Draw(work->keyCursor, BmpWin_GetBitmap(work->windows[FMISSION_APP_WINDOW_MESSAGE].window), 15);
        BmpWin_FlushChar(work->windows[FMISSION_APP_WINDOW_MESSAGE].window);
    } else {
        KeyCursor_Erase(work->keyCursor, BmpWin_GetBitmap(work->windows[FMISSION_APP_WINDOW_MESSAGE].window), 15);
        BmpWin_FlushChar(work->windows[FMISSION_APP_WINDOW_MESSAGE].window);
    }
}
