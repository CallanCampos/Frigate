#include "types.h"
#include "app/beacon_detail_local.h"
#include "app/townmap/townmap_data.h"
#include "app/ui/touchbar.h"
#include "constants/sound.h"
#include "constants/version.h"
#include "constants/zones.h"
#include "field/zone.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "pml/poke_graphic.h"
#include "system/game_beacon.h"
#include "system/game_system.h"
#include "system/palanm.h"
#include "system/pms_data.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "system/union_view.h"
#include "system/wordset.h"
#include "text/system/startmenu.h"

// The drawing, the input and the tasks of the screen of a beacon's details. The ROM has no name for this file;
// "beacon_detail_sub.c" is a guess from its graphic file's, "beacon_detail_gra.c", which the linker placed between
// this file and the screen's own

// The zones are below this
#define ZONE_COUNT 615

#define SCREEN_HEIGHT 192

// How fast and for how long a panel slides when the page turns
#define PANEL_SLIDE_SPEED 8
#define PANEL_SLIDE_FRAMES (SCREEN_HEIGHT / PANEL_SLIDE_SPEED)

// How far and how fast the map slides up for the pop-up, and how long the pop-up stays
#define POPUP_SLIDE_Y 64
#define POPUP_SLIDE_SPEED 8
#define POPUP_SLIDE_FRAMES (POPUP_SLIDE_Y / POPUP_SLIDE_SPEED)
#define POPUP_FRAMES 150

// Where a panel that slides in starts from
enum {
    PANEL_SLIDE_FROM_ABOVE,
    PANEL_SLIDE_OUT,
    PANEL_SLIDE_FROM_BELOW,
};

// The directions of turning the page
enum {
    PAGE_NEXT,
    PAGE_PREV,
};

// Where LoadPalette loads colors
enum {
    PALETTE_BG_MAIN,
    PALETTE_BG_SUB,
    PALETTE_OBJ_MAIN,
    PALETTE_OBJ_SUB,
};

// The two looks of a panel's rows
typedef struct {
    u16 color;
    u8 background;
} BeaconDetailRowStyle;

// How the screen shows a kind of recent event: its message, and the animation of its icon on the map plus 1, or 0
// for no icon
typedef struct {
    u8 kind;
    u8 message;
    u8 icon;
} BeaconDetailRecent;

typedef struct {
    u8 out;
    u8 frames;
    u8 frame;
    s8 y;
    s8 speed;
    int *taskCount;
} BeaconDetailMapSlideTask;

typedef struct {
    u8 seq;
    u8 timer;
    int taskCount;
    BeaconDetailWork *work;
    int *parentTaskCount;
} BeaconDetailPopupTask;

typedef struct {
    u8 page;
    u8 frames;
    u16 unk2;
    s16 y;
    s16 speed;
    u32 unk8;
    BeaconDetailPanel *panel;
    BeaconDetailWork *work;
    int *taskCount;
} BeaconDetailPanelSlideTask;

typedef struct {
    u8 seq;
    u8 unk1;
    u8 unk2;
    u8 index;
    u8 direction;
    u8 decided;
    int taskCount;
    u32 unkC;
    BeaconDetailWork *work;
    int *parentTaskCount;
} BeaconDetailPageTask;

typedef struct {
    BeaconDetailWindow *window;
    BeaconDetailWork *work;
    int *taskCount;
} BeaconDetailFlushTask;

static BOOL BeaconDetailSub_IsMarkerTouched(BeaconDetailWork *work);
static void BeaconDetailSub_PlaySE(u32 se);
static void BeaconDetailSub_LoadPalette(const void *src, u32 dest, u16 offset, u32 count);
static void BeaconDetailSub_UpdateTouchBar(BeaconDetailWork *work);
static u32 BeaconDetailSub_GetVersionPalette(const GameBeacon *beacon);
static const BeaconDetailRecent *BeaconDetailSub_FindRecent(u32 kind);
static void BeaconDetailSub_SetRecentWords(const GameBeacon *beacon, WordSet *wordSet);
static void BeaconDetailSub_SetActorAnim(ClActor *actor, u8 anim);
static void BeaconDetailSub_SetActorPos(ClActor *actor, s16 x, s16 y, u16 surface);
static void BeaconDetailSub_FormatMessage(BeaconDetailWork *work, u8 messageId);
static void BeaconDetailSub_SetMapTrainer(BeaconDetailWork *work, u8 view);
static void BeaconDetailSub_DrawLabels(BeaconDetailWork *work);
static void BeaconDetailSub_DrawRow(BeaconDetailWork *work, BeaconDetailWindow *window, StrBuf *strbuf, u8 clearX, u8 x,
                                    u8 width, u8 height, u8 style);
static void BeaconDetailSub_SetPanelVisible(BeaconDetailWork *work, u8 page, BOOL visible);
static void BeaconDetailSub_ScrollPanel(BeaconDetailPanel *panel, s16 y);
static void BeaconDetailSub_DrawPanel(BeaconDetailWork *work, GameBeacon *beacon, u16 time, u8 page);
static void BeaconDetailSub_DrawMap(BeaconDetailWork *work);
static void BeaconDetailSub_StartMapSlide(BeaconDetailWork *work, u8 out, int *taskCount);
static void BeaconDetailSub_MapSlideTask(TCBEx *task, void *data);
static void BeaconDetailSub_StartPopup(BeaconDetailWork *work);
static void BeaconDetailSub_StartPopupTask(BeaconDetailWork *work, int *taskCount);
static BOOL BeaconDetailSub_WaitPopup(BeaconDetailPopupTask *popup);
static void BeaconDetailSub_PopupTask(TCBEx *task, void *data);
static void BeaconDetailSub_StartPanelSlide(BeaconDetailWork *work, u8 page, int from, u8 direction, int *taskCount);
static void BeaconDetailSub_PanelSlideTask(TCBEx *task, void *data);
static void BeaconDetailSub_TurnPage(BeaconDetailWork *work, u8 direction);
static void BeaconDetailSub_StartPageTask(BeaconDetailWork *work, u8 direction, int *taskCount);
static void BeaconDetailSub_PageTask(TCBEx *task, void *data);
static void BeaconDetailSub_Print(BeaconDetailWork *work, BeaconDetailWindow *window, s16 x, s16 y, StrBuf *strbuf,
                                  u16 color);
static void BeaconDetailSub_StartFlushTask(BeaconDetailWork *work, BeaconDetailWindow *window, int *taskCount);
static void BeaconDetailSub_FlushTask(TCBEx *task, void *data);
static BOOL BeaconDetailSub_IsOffMap(const GameBeacon *beacon);

// Where the panel that slides in comes from, by the direction of turning the page
static const u8 sBeaconDetailSlideFrom[] = { PANEL_SLIDE_FROM_BELOW, PANEL_SLIDE_FROM_ABOVE };

// The zones of Black and White that Black 2 and White 2's map has no place for
static const u16 sBeaconDetailOffMapZones[] = { 191, 264 };

static const BeaconDetailRowStyle sBeaconDetailRowStyles[] = {
    { PRINT_COLOR(15, 14, 3), 3 },
    { PRINT_COLOR(15, 14, 4), 4 },
};

// The palette of the Union Room's trainers that each view's is
static const u8 sBeaconDetailUnionPalettes[BEACON_DETAIL_UNION_VIEW_COUNT] = {
    2, 3, 6, 5, 4, 5, 2, 0, 1, 3, 6, 5, 4, 7, 7, 0,
};

static const BeaconDetailRecent sBeaconDetailRecents[] = {
    { 0, Startmenu_Text_BattlingWild, 1 },
    { 1, Startmenu_Text_BattlingSpecialPokemon, 1 },
    { 2, Startmenu_Text_Battling, 2 },
    { 3, Startmenu_Text_BattlingGymLeader, 2 },
    { 4, Startmenu_Text_BattlingSpecialTrainer, 2 },
    { GAME_BEACON_RECENT_NONE, Startmenu_Text_MovingThrough, 0 },
};

void BeaconDetailSub_Start(BeaconDetailWork *work) {
    BeaconDetailSub_DrawLabels(work);
    GameBeaconInfoTable_RingGet(work->infoTable, work->beacon, &work->time, work->logIndices[work->index]);
    BeaconDetailSub_DrawPanel(work, work->beacon, work->time, work->page);
    BeaconDetailSub_SetPanelVisible(work, work->page, TRUE);
    BeaconDetailSub_SetPanelVisible(work, work->page ^ 1, FALSE);
    BeaconDetailSub_DrawMap(work);
    BeaconDetailSub_UpdateTouchBar(work);
    func_02042ba8(FALSE, work->tailHeapId);
}

u32 BeaconDetailSub_Input(BeaconDetailWork *work) {
    u32 touched = TouchBar_GetTouched(work->touchBar);
    u32 keys = GCTX_HIDGetPressedKeys();

    if (func_02016bec(work->param->gsys)) {
        work->param->unk0A = TRUE;
        return BEACON_DETAIL_SEQ_FADE_OUT;
    }
    if (touched == BEACON_DETAIL_ICON_RETURN) {
        work->param->unk0A = FALSE;
        return BEACON_DETAIL_SEQ_FADE_OUT;
    }
    if (touched == BEACON_DETAIL_ICON_PREV && work->index != 0) {
        BeaconDetailSub_TurnPage(work, PAGE_PREV);
        return BEACON_DETAIL_SEQ_WAIT;
    }
    if (touched == BEACON_DETAIL_ICON_NEXT && work->index < work->count - 1) {
        BeaconDetailSub_TurnPage(work, PAGE_NEXT);
        return BEACON_DETAIL_SEQ_WAIT;
    }
    if (!work->hasPlace) {
        return BEACON_DETAIL_SEQ_INPUT;
    }
    if (BeaconDetailSub_IsMarkerTouched(work) || (keys & PAD_BUTTON_A)) {
        BeaconDetailSub_PlaySE(SEQ_SE_DECIDE1);
        BeaconDetailSub_StartPopup(work);
        return BEACON_DETAIL_SEQ_WAIT;
    }
    return BEACON_DETAIL_SEQ_INPUT;
}

void BeaconDetailSub_EndWait(BeaconDetailWork *work) {
    BeaconDetailSub_UpdateTouchBar(work);
}

// Whether the map's trainer was touched this frame
static BOOL BeaconDetailSub_IsMarkerTouched(BeaconDetailWork *work) {
    u32 x;
    u32 y;

    if (func_0203dac8(&x, &y)) {
        if (x - (work->markerX - 12) <= 24 && y - (work->markerY - 16) <= 32) {
            return TRUE;
        }
    }
    return FALSE;
}

static void BeaconDetailSub_PlaySE(u32 se) {
    GFL_SndSEPlay(se);
}

static void BeaconDetailSub_LoadPalette(const void *src, u32 dest, u16 offset, u32 count) {
    u32 destOffset = offset * 2;

    cp15_flushDC(src, count * 2);
    switch (dest) {
    case PALETTE_BG_MAIN:
        gfxUploadStdPaletteBGA(src, destOffset, count * 2);
        break;
    case PALETTE_BG_SUB:
        gfxUploadStdPaletteBGB(src, destOffset, count * 2);
        break;
    case PALETTE_OBJ_MAIN:
        gfxUploadStdPaletteObjA(src, destOffset, count * 2);
        break;
    case PALETTE_OBJ_SUB:
        gfxUploadStdPaletteObjB(src, destOffset, count * 2);
        break;
    }
}

// The arrows are active when the log has a beacon that way
static void BeaconDetailSub_UpdateTouchBar(BeaconDetailWork *work) {
    TouchBar_SetIconActive(work->touchBar, BEACON_DETAIL_ICON_RETURN, TRUE);
    if (work->index != 0) {
        TouchBar_SetIconActive(work->touchBar, BEACON_DETAIL_ICON_PREV, TRUE);
    } else {
        TouchBar_SetIconActive(work->touchBar, BEACON_DETAIL_ICON_PREV, FALSE);
    }
    if (work->index < work->count - 1) {
        TouchBar_SetIconActive(work->touchBar, BEACON_DETAIL_ICON_NEXT, TRUE);
    } else {
        TouchBar_SetIconActive(work->touchBar, BEACON_DETAIL_ICON_NEXT, FALSE);
    }
}

// The palette of the panel's frame: Black's, White's, or the one of Black 2 and White 2
static u32 BeaconDetailSub_GetVersionPalette(const GameBeacon *beacon) {
    switch (GameBeacon_GetVersion(beacon)) {
    case VERSION_BLACK:
        return 0;
    case VERSION_WHITE:
        return 1;
    default:
        return 2;
    }
}

static const BeaconDetailRecent *BeaconDetailSub_FindRecent(u32 kind) {
    u32 i;

    for (i = 0; i < NELEMS(sBeaconDetailRecents); i++) {
        if (kind == sBeaconDetailRecents[i].kind) {
            return &sBeaconDetailRecents[i];
        }
    }
    return &sBeaconDetailRecents[0];
}

// The words of the message of the beacon's recent event
static void BeaconDetailSub_SetRecentWords(const GameBeacon *beacon, WordSet *wordSet) {
    switch (GameBeacon_GetRecentKind(beacon)) {
    case 0:
    case 1:
        WordSet_LoadSpeciesName(wordSet, 0, GameBeacon_GetRecentSpecies(beacon));
        break;
    case 2: {
        u16 trainer = GameBeacon_GetRecentTrainer(beacon);

        loadTrainerTypeToStrbuf(wordSet, 0, trainer);
        loadTrainerNamesToStrbuf(wordSet, 1, trainer);
        break;
    }
    case 3:
    case 4:
        break;
    default: {
        u16 version = GameBeacon_GetVersion(beacon);
        u16 zone = GameBeacon_GetZone(beacon);

        if (zone >= ZONE_COUNT) {
            zone = ZONE_ROUTE_1;
        }
        loadLocationNameToStrbuf(wordSet, 0, GetZoneLocationNameIdx(zone, version));
        break;
    }
    }
}

static void BeaconDetailSub_SetActorAnim(ClActor *actor, u8 anim) {
    func_0204c488(actor, anim);
    func_0204c56c(actor);
}

static void BeaconDetailSub_SetActorPos(ClActor *actor, s16 x, s16 y, u16 surface) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c140(actor, &pos, surface);
}

// Loads a message into the text buffer, with the words set
static void BeaconDetailSub_FormatMessage(BeaconDetailWork *work, u8 messageId) {
    GFL_MsgDataLoadStrbuf(work->msgData, messageId, work->formatBuf);
    GFL_WordSetFormatStrbuf(work->wordSet, work->textBuf, work->formatBuf);
}

// Gives the map's trainer the characters and the palette of a view
static void BeaconDetailSub_SetMapTrainer(BeaconDetailWork *work, u8 view) {
    u8 color;

    func_0204ba40(work->unionRes.chars, work->unionChars[view].data);
    color = sBeaconDetailUnionPalettes[view] * 16;
    BeaconDetailSub_LoadPalette(&work->unionPalette.raw[color], PALETTE_OBJ_MAIN, 10 * 16, 16);
}

static void BeaconDetailSub_DrawLabels(BeaconDetailWork *work) {
    int i;
    int j;

    for (i = 0; i < 4; i++) {
        GFL_MsgDataLoadStrbuf(work->msgData, Startmenu_Text_Name + i, work->formatBuf);
        for (j = 0; j < BEACON_DETAIL_PANEL_COUNT; j++) {
            BeaconDetailPanel *panel = &work->panels[j];

            GFL_BitmapFill(panel->windows[i].bitmap, sBeaconDetailRowStyles[i % 2].background);
            BeaconDetailSub_Print(work, &panel->windows[i], 0, 0, work->formatBuf, sBeaconDetailRowStyles[i % 2].color);
            BmpWin_TransferNow(panel->windows[i].window);
        }
    }
    GFL_MsgDataLoadStrbuf(work->msgData, Startmenu_Text_From, work->formatBuf);
    for (i = 0; i < BEACON_DETAIL_PANEL_COUNT; i++) {
        BeaconDetailPanel *panel = &work->panels[i];

        GFL_BitmapFill(panel->windows[BEACON_DETAIL_WINDOW_FROM].bitmap, 3);
        BeaconDetailSub_Print(work, &panel->windows[BEACON_DETAIL_WINDOW_FROM], 0, 0, work->formatBuf,
                              PRINT_COLOR(15, 14, 3));
        BmpWin_TransferNow(panel->windows[BEACON_DETAIL_WINDOW_FROM].window);
    }
    GFL_MsgDataLoadStrbuf(work->msgData, Startmenu_Text_NumberTimesThankedNumber, work->formatBuf);
    for (i = 0; i < BEACON_DETAIL_PANEL_COUNT; i++) {
        BeaconDetailPanel *panel = &work->panels[i];

        GFL_BitmapFill(panel->windows[BEACON_DETAIL_WINDOW_COUNTS].bitmap, 3);
        BeaconDetailSub_Print(work, &panel->windows[BEACON_DETAIL_WINDOW_COUNTS], 0, 0, work->formatBuf,
                              PRINT_COLOR(15, 14, 3));
        BmpWin_TransferNow(panel->windows[BEACON_DETAIL_WINDOW_COUNTS].window);
    }
}

// Clears a window from a column on, width by height characters, and prints a string there
static void BeaconDetailSub_DrawRow(BeaconDetailWork *work, BeaconDetailWindow *window, StrBuf *strbuf, u8 clearX, u8 x,
                                    u8 width, u8 height, u8 style) {
    GFL_BitmapFillArea(window->bitmap, clearX * 8, 0, width * 8, height * 8, sBeaconDetailRowStyles[style].background);
    BeaconDetailSub_Print(work, window, x, 0, strbuf, sBeaconDetailRowStyles[style].color);
    BmpWin_TransferNow(window->window);
}

static void BeaconDetailSub_SetPanelVisible(BeaconDetailWork *work, u8 page, BOOL visible) {
    BeaconDetailPanel *panel = &work->panels[page];

    func_0204c124(panel->trainerActor, visible);
    if (panel->rankShown) {
        func_0204c124(panel->rankActor, visible);
    }
    if (PMSDraw_IsDrawn(work->pmsDraw, page)) {
        PMSDraw_SetVisible(work->pmsDraw, page, visible);
    }
    GFL_BGSysSetBGEnabled(panel->bg, visible);
}

static void BeaconDetailSub_ScrollPanel(BeaconDetailPanel *panel, s16 y) {
    GFL_BGSysMoveBG(panel->bg, BG_MOVE_SET_Y, -y);
    BeaconDetailSub_SetActorPos(panel->trainerActor, 220, y + 44, CLACT_SURFACE_SUB);
    BeaconDetailSub_SetActorPos(panel->rankActor, 200, y + 80, CLACT_SURFACE_SUB);
}

// Draws a beacon's profile into a panel
static void BeaconDetailSub_DrawPanel(BeaconDetailWork *work, GameBeacon *beacon, u16 time, u8 page) {
    BeaconDetailPanel *panel = &work->panels[page];
    u16 colors[2];
    PMSData record;
    u16 hours;
    u16 minutes;
    int palette;
    u16 country;
    u8 answer;
    u8 rank;

    // The sender's favorite color and a darker shade of it
    func_02013d38(&colors[0], beacon);
    BlendColors(&colors[0], &colors[1], 1, 3, 0);
    palette = page * 16;
    BeaconDetailSub_LoadPalette(colors, PALETTE_BG_SUB, palette + 1, 2);
    BeaconDetailSub_LoadPalette(&work->versionPalette.raw[BeaconDetailSub_GetVersionPalette(beacon) * 16],
                                PALETTE_BG_SUB, palette + 10, 2);

    GameBeacon_GetName(beacon, work->formatBuf);
    BeaconDetailSub_DrawRow(work, &panel->windows[BEACON_DETAIL_WINDOW_NAME], work->formatBuf, 6, 48, 13, 2, 0);

    answer = func_02013cd8(beacon);
    if (answer == 0xff) {
        GFL_StrBufClear(work->textBuf);
    } else {
        loadJobAnswerToStrbuf(work->wordSet, 0, answer);
        BeaconDetailSub_FormatMessage(work, Startmenu_Text_Empty_2);
    }
    BeaconDetailSub_DrawRow(work, &panel->windows[BEACON_DETAIL_WINDOW_JOB], work->textBuf, 6, 48, 13, 2, 1);

    answer = func_02013cf0(beacon);
    if (answer == 0xff) {
        GFL_StrBufClear(work->textBuf);
    } else {
        loadHobbyNameToStrbuf(work->wordSet, 0, answer);
        BeaconDetailSub_FormatMessage(work, Startmenu_Text_Empty);
    }
    BeaconDetailSub_DrawRow(work, &panel->windows[BEACON_DETAIL_WINDOW_HOBBY], work->textBuf, 6, 48, 13, 2, 0);

    GameBeacon_GetPlayTime(beacon, &hours, &minutes);
    WordSetNumber(work->wordSet, 2, hours, 3, 0, TRUE);
    WordSetNumber(work->wordSet, 3, minutes, 2, 2, TRUE);
    BeaconDetailSub_FormatMessage(work, Startmenu_Text_Empty_3);
    BeaconDetailSub_DrawRow(work, &panel->windows[BEACON_DETAIL_WINDOW_TIME], work->textBuf, 6, 0, 5, 2, 1);

    country = GameBeacon_GetCountry(beacon);
    loadCountryToStrbuf(work->wordSet, 0, country);
    loadCountryAreaToStrbuf(work->wordSet, 1, country, GameBeacon_GetRegion(beacon));
    BeaconDetailSub_FormatMessage(work, Startmenu_Text_Empty_4);
    BeaconDetailSub_DrawRow(work, &panel->windows[BEACON_DETAIL_WINDOW_HOME], work->textBuf, 0, 0, 28, 4, 0);

    WordSetNumber(work->wordSet, 0, func_02013db8(beacon), 5, 1, TRUE);
    WordSetNumber(work->wordSet, 1, func_02013dc0(beacon), 5, 1, TRUE);
    BeaconDetailSub_FormatMessage(work, Startmenu_Text_Empty_5);
    BeaconDetailSub_DrawRow(work, &panel->windows[BEACON_DETAIL_WINDOW_COUNTS], work->textBuf, 21, 0, 6, 4, 0);

    if (PMSDraw_IsDrawn(work->pmsDraw, page)) {
        PMSDraw_Clear(work->pmsDraw, page, TRUE);
    }
    GameBeacon_GetCGearRecord(beacon, &record);
    PMSDraw_Print(work->pmsDraw, panel->recordWindow, &record, page);

    panel->trainerView = GameBeacon_GetTrainerViewWithGender(beacon);
    TrGra_ReplaceClActCharsAndPalette(work->trainerArc, func_0202b5e8(panel->trainerView), work->trainerRes[page].chars,
                                      work->trainerRes[page].palette, work->tailHeapId);

    rank = GameBeacon_GetSurveyRank(beacon);
    if (rank == 0) {
        func_0204c124(panel->rankActor, FALSE);
    } else {
        BeaconDetailSub_SetActorAnim(panel->rankActor, rank + 1);
        func_0204c124(panel->rankActor, TRUE);
    }
}

// Puts the sender on the map, when the map has the place and shows it, and prints when the beacon came and what the
// sender was doing
static void BeaconDetailSub_DrawMap(BeaconDetailWork *work) {
    const BeaconDetailRecent *recent;
    u32 zone;
    u16 place;
    BOOL offMap;
    u8 hour;

    GameBeaconInfoTable_RingGet(work->infoTable, work->beacon, &work->time, work->logIndices[work->index]);
    recent = BeaconDetailSub_FindRecent(GameBeacon_GetRecentKind(work->beacon));
    BeaconDetailSub_SetMapTrainer(work, GameBeacon_GetTrainerViewWithGender(work->beacon));
    zone = GameBeacon_GetZone(work->beacon);
    place = TownMapData_GetPlaceByZone(work->townMapData, GameBeacon_GetParentZone(work->beacon));
    offMap = BeaconDetailSub_IsOffMap(work->beacon);
    if (place == TOWNMAP_PLACE_NONE || offMap ||
        TownMapData_GetParam(work->townMapData, place, TOWNMAP_PARAM_FLAG) != TOWNMAP_NO_FLAG || zone >= ZONE_COUNT) {
        work->hasPlace = FALSE;
        work->markerY = 0;
        work->markerX = work->markerY;
        func_0204c124(work->actors[BEACON_DETAIL_ACTOR_TRAINER], FALSE);
        func_0204c124(work->actors[BEACON_DETAIL_ACTOR_RECENT], FALSE);
    } else {
        work->hasPlace = TRUE;
        work->markerX = TownMapData_GetParam(work->townMapData, place, TOWNMAP_PARAM_X);
        work->markerY = TownMapData_GetParam(work->townMapData, place, TOWNMAP_PARAM_Y) - 16;
        if (work->markerY < 16) {
            work->markerY = 16;
        } else if (work->markerY > 152) {
            work->markerY = 152;
        }
        if (work->markerX < 16) {
            work->markerX = 16;
        } else if (work->markerX > 240) {
            work->markerX = 240;
        }
        BeaconDetailSub_SetActorPos(work->actors[BEACON_DETAIL_ACTOR_TRAINER], work->markerX, work->markerY,
                                    CLACT_SURFACE_MAIN);
        func_0204c124(work->actors[BEACON_DETAIL_ACTOR_TRAINER], TRUE);
        if (recent->icon) {
            // To the trainer's left, or to the right at the map's left edge
            int x = work->markerX - 24;

            if (x < 16) {
                x = work->markerX + 24;
            }
            BeaconDetailSub_SetActorPos(work->actors[BEACON_DETAIL_ACTOR_RECENT], x, work->markerY, CLACT_SURFACE_MAIN);
            BeaconDetailSub_SetActorAnim(work->actors[BEACON_DETAIL_ACTOR_RECENT], recent->icon - 1);
        }
        func_0204c124(work->actors[BEACON_DETAIL_ACTOR_RECENT], recent->icon != 0);
    }

    GFL_BitmapFill(work->infoWindow.bitmap, 15);
    hour = work->time >> 8;
    WordSetNumber(work->wordSet, 0, hour, 2, 0, TRUE);
    WordSetNumber(work->wordSet, 1, (u8)work->time, 2, 2, TRUE);
    // The message of the morning or of the afternoon
    BeaconDetailSub_FormatMessage(work, Startmenu_Text_PasserbyTime + hour / 12);
    BeaconDetailSub_Print(work, &work->infoWindow, 0, 0, work->textBuf, PRINT_COLOR(7, 8, 15));
    BeaconDetailSub_SetRecentWords(work->beacon, work->wordSet);
    BeaconDetailSub_FormatMessage(work, recent->message);
    BeaconDetailSub_Print(work, &work->infoWindow, 0, 16, work->textBuf, PRINT_COLOR(7, 8, 15));
    BmpWin_TransferNow(work->infoWindow.window);
}

// Slides the touch screen's info window up over the map, or back down
static void BeaconDetailSub_StartMapSlide(BeaconDetailWork *work, u8 out, int *taskCount) {
    BeaconDetailMapSlideTask *slide =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(work->tcbManager, BeaconDetailSub_MapSlideTask, sizeof(*slide), 0));

    sys_memset(slide, 0, sizeof(*slide));
    slide->out = out;
    if (!slide->out) {
        slide->y = 0;
        slide->speed = POPUP_SLIDE_SPEED;
    } else {
        slide->y = POPUP_SLIDE_Y;
        slide->speed = -POPUP_SLIDE_SPEED;
    }
    slide->frames = POPUP_SLIDE_FRAMES;
    BeaconDetailSub_PlaySE(SEQ_SE_DECIDE2);
    slide->taskCount = taskCount;
    (*taskCount)++;
}

static void BeaconDetailSub_MapSlideTask(TCBEx *task, void *data) {
    BeaconDetailMapSlideTask *slide = data;

    if (slide->frame++ < slide->frames) {
        slide->y += slide->speed;
        GFL_BGSysMoveBG(1, BG_MOVE_SET_Y, slide->y);
    } else {
        (*slide->taskCount)--;
        GFL_TCBExRequestEnd(task);
    }
}

static void BeaconDetailSub_StartPopup(BeaconDetailWork *work) {
    BeaconDetailSub_StartPopupTask(work, &work->taskCount);
}

static void BeaconDetailSub_StartPopupTask(BeaconDetailWork *work, int *taskCount) {
    BeaconDetailPopupTask *popup =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(work->tcbManager, BeaconDetailSub_PopupTask, sizeof(*popup), 0));

    sys_memset(popup, 0, sizeof(*popup));
    popup->work = work;
    popup->timer = POPUP_FRAMES;
    TouchBar_SetActive(work->touchBar, FALSE);
    popup->parentTaskCount = taskCount;
    (*taskCount)++;
}

// Whether the pop-up stays: until its time is up, a touch or the A Button
static BOOL BeaconDetailSub_WaitPopup(BeaconDetailPopupTask *popup) {
    if (func_0203da48() || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A)) {
        popup->timer = 0;
        return FALSE;
    }
    if (popup->timer-- == 0) {
        return FALSE;
    }
    return TRUE;
}

static void BeaconDetailSub_PopupTask(TCBEx *task, void *data) {
    BeaconDetailPopupTask *popup = data;

    switch (popup->seq) {
    case 0:
        BeaconDetailSub_StartMapSlide(popup->work, FALSE, &popup->taskCount);
        popup->seq++;
        break;
    case 1:
        if (popup->taskCount == 0) {
            popup->seq++;
        }
        break;
    case 2:
        if (!BeaconDetailSub_WaitPopup(popup)) {
            BeaconDetailSub_StartMapSlide(popup->work, TRUE, &popup->taskCount);
            popup->seq++;
        }
        break;
    case 3:
        if (popup->taskCount) {
            break;
        }
    default:
        (*popup->parentTaskCount)--;
        GFL_TCBExRequestEnd(task);
        break;
    }
}

static void BeaconDetailSub_StartPanelSlide(BeaconDetailWork *work, u8 page, int from, u8 direction, int *taskCount) {
    BeaconDetailPanelSlideTask *slide =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(work->tcbManager, BeaconDetailSub_PanelSlideTask, sizeof(*slide), 0));

    sys_memset(slide, 0, sizeof(*slide));
    slide->panel = &work->panels[page];
    slide->page = page;
    slide->y = (PANEL_SLIDE_OUT - from) * -SCREEN_HEIGHT;
    if (direction == PAGE_NEXT) {
        slide->speed -= PANEL_SLIDE_SPEED;
    } else {
        slide->speed += PANEL_SLIDE_SPEED;
    }
    slide->frames = PANEL_SLIDE_FRAMES;
    BeaconDetailSub_ScrollPanel(slide->panel, slide->y);
    if (from != PANEL_SLIDE_OUT) {
        BeaconDetailSub_SetPanelVisible(work, slide->page, TRUE);
    }
    slide->work = work;
    slide->taskCount = taskCount;
    (*taskCount)++;
}

static void BeaconDetailSub_PanelSlideTask(TCBEx *task, void *data) {
    BeaconDetailPanelSlideTask *slide = data;

    if (slide->frames-- != 0) {
        slide->y += slide->speed;
        BeaconDetailSub_ScrollPanel(slide->panel, slide->y);
        return;
    }
    // The panel that slid off the screen
    if (slide->y != 0) {
        BeaconDetailSub_SetPanelVisible(slide->work, slide->page, FALSE);
    }
    (*slide->taskCount)--;
    GFL_TCBExRequestEnd(task);
}

static void BeaconDetailSub_TurnPage(BeaconDetailWork *work, u8 direction) {
    BeaconDetailSub_StartPageTask(work, direction, &work->taskCount);
}

// Draws the next or the previous beacon into the hidden panel, to slide it in once it is printed
static void BeaconDetailSub_StartPageTask(BeaconDetailWork *work, u8 direction, int *taskCount) {
    BeaconDetailPageTask *page =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(work->tcbManager, BeaconDetailSub_PageTask, sizeof(*page), 0));

    sys_memset(page, 0, sizeof(*page));
    page->direction = direction;
    if (direction == PAGE_NEXT) {
        page->index = work->index + 1;
        TouchBar_SetIconActive(work->touchBar, BEACON_DETAIL_ICON_PREV, FALSE);
    } else {
        page->index = work->index - 1;
        TouchBar_SetIconActive(work->touchBar, BEACON_DETAIL_ICON_NEXT, FALSE);
    }
    TouchBar_SetIconActive(work->touchBar, BEACON_DETAIL_ICON_RETURN, FALSE);
    GameBeaconInfoTable_RingGet(work->infoTable, work->beacon, &work->time, work->logIndices[page->index]);
    BeaconDetailSub_DrawPanel(work, work->beacon, work->time, work->page ^ 1);
    BeaconDetailSub_SetPanelVisible(work, work->page ^ 1, FALSE);
    page->work = work;
    page->parentTaskCount = taskCount;
    (*taskCount)++;
}

static void BeaconDetailSub_PageTask(TCBEx *task, void *data) {
    BeaconDetailPageTask *page = data;

    // The arrow's animation ends before the panels slide
    if (!page->decided && TouchBar_GetDecided(page->work->touchBar) != -1) {
        page->decided = TRUE;
    }
    if (!func_02021c0c(page->work->printQueue)) {
        return;
    }
    switch (page->seq) {
    case 0:
        if (!page->decided) {
            return;
        }
        TouchBar_SetActive(page->work->touchBar, FALSE);
        page->seq++;
        break;
    case 1:
        BeaconDetailSub_StartPanelSlide(page->work, page->work->page, PANEL_SLIDE_OUT, page->direction,
                                        &page->taskCount);
        BeaconDetailSub_StartPanelSlide(page->work, page->work->page ^ 1, sBeaconDetailSlideFrom[page->direction],
                                        page->direction, &page->taskCount);
        func_0204c124(page->work->actors[BEACON_DETAIL_ACTOR_TRAINER], FALSE);
        func_0204c124(page->work->actors[BEACON_DETAIL_ACTOR_RECENT], FALSE);
        page->seq++;
        break;
    case 2:
        if (page->taskCount) {
            return;
        }
        page->work->page ^= 1;
        page->work->index = page->index;
        BeaconDetailSub_DrawMap(page->work);
        page->seq++;
        break;
    case 3:
    default:
        (*page->parentTaskCount)--;
        GFL_TCBExRequestEnd(task);
        break;
    }
}

// Prints into a window through the queue, with a task that sends the window to VRAM once it is printed
static void BeaconDetailSub_Print(BeaconDetailWork *work, BeaconDetailWindow *window, s16 x, s16 y, StrBuf *strbuf,
                                  u16 color) {
    PrintWindow_Print(&window->print, work->printQueue, x, y, strbuf, work->font, color);
    BeaconDetailSub_StartFlushTask(work, window, &work->taskCount);
}

static void BeaconDetailSub_StartFlushTask(BeaconDetailWork *work, BeaconDetailWindow *window, int *taskCount) {
    BeaconDetailFlushTask *flush =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(work->tcbManager, BeaconDetailSub_FlushTask, sizeof(*flush), 0));

    sys_memset(flush, 0, sizeof(*flush));
    flush->work = work;
    flush->window = window;
    if (taskCount != NULL) {
        flush->taskCount = taskCount;
        (*taskCount)++;
    }
}

static void BeaconDetailSub_FlushTask(TCBEx *task, void *data) {
    BeaconDetailFlushTask *flush = data;
    PrintWindow *print = &flush->window->print;
    PrintQueue *queue = flush->work->printQueue;

    PrintWindow_Flush(print, queue);
    if (!print->flushPending) {
        if (flush->taskCount != NULL) {
            (*flush->taskCount)--;
        }
        GFL_TCBExRequestEnd(task);
    }
}

// Whether a beacon of Black or White comes from a zone that this game's map has no place for
static BOOL BeaconDetailSub_IsOffMap(const GameBeacon *beacon) {
    u32 version = GameBeacon_GetVersion(beacon);

    if (version == VERSION_WHITE || version == VERSION_BLACK) {
        u16 zone = GetZoneParentZone(GameBeacon_GetZone(beacon));
        u32 i;

        for (i = 0; i < NELEMS(sBeaconDetailOffMapZones); i++) {
            if (zone == sBeaconDetailOffMapZones[i]) {
                return TRUE;
            }
        }
    }
    return FALSE;
}
