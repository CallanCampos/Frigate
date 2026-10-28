#include "types.h"
#include "app/pokedex_common.h"
#include "app/pokedex_habitat.h"
#include "app/pokedex_habitat_graphic.h"
#include "constants/arc.h"
#include "constants/flags.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/zones.h"
#include "field/zone.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/medal_box.h"
#include "save/pokedex_habitat_savedata.h"
#include "save/pokedex_savedata.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/pokedex_habitat.h"

// The Pokédex's Habitat List: its process, its sequences and what they show. The ROM has no name for this file;
// pokedex_habitat.c is a guess after pokedex.c, which calls the screen the habitat list.
//
// An area has three kinds of habitat, the tall grass, the water and fishing, each with a tab, and four seasons. The
// screen lists the Pokémon of the area's kind of habitat and season that the player has seen, with the map of the
// region and the area on it on the other screen. Once all the Pokémon of a kind of habitat are seen or caught, and
// again once all those of the area are caught, it stamps the area

// How far the two screens slide in
#define SLIDE_DISTANCE 48
#define SLIDE_SPEED 3

// The Pokémon that the cursor's screen can touch: the rows that fit above its buttons
#define TOUCH_ICON_COUNT 20

#define SEASON_COUNT POKEDEX_SEASON_COUNT

// A habitat's kinds
typedef enum {
    TYPE_LAND,
    TYPE_WATER,
    TYPE_FISHING,
    TYPE_COUNT,
} PokedexHabitatType;

// A species' status in PokedexHabitatSpecies
enum {
    STATUS_NONE,
    STATUS_UNSEEN,
    STATUS_SEEN,
    STATUS_CAUGHT,
};

// The stamps: for the Pokémon of a kind of habitat all seen or all caught, and for all those of the area caught
enum {
    AWARD_NONE,
    AWARD_SEEN,
    AWARD_CAUGHT,
    AWARD_AREA,
};

enum {
    SEQ_INIT,
    SEQ_FADE_IN,
    SEQ_FADE_OUT,
    SEQ_LIST,
    SEQ_LEAVE_CURSOR,
    SEQ_CHANGE_SEASON,
    SEQ_CHANGE_AREA,
    SEQ_CHANGE_TYPE,
    SEQ_CURSOR,
    SEQ_MOVE_CURSOR,
    SEQ_AWARD,
    SEQ_BUTTON,
    SEQ_ALL_COMPLETE,
    SEQ_COUNT,
};

// What a sequence is started with
enum {
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_UP,
    ACTION_DOWN,
    ACTION_DECIDE,
    ACTION_RETURN,
    ACTION_EXIT,
    ACTION_TOUCH,
    ACTION_TOUCH_DECIDE,
    ACTION_INFO,
    ACTION_NONE,
    ACTION_TAB,
};

// What the list's screen touches
enum {
    LIST_TOUCH_SHORTCUT,
    LIST_TOUCH_EXIT,
    LIST_TOUCH_RETURN,
    LIST_TOUCH_TAB,
    LIST_TOUCH_AREA_UP = LIST_TOUCH_TAB + TYPE_COUNT,
    LIST_TOUCH_AREA_DOWN,
    LIST_TOUCH_SEASON_LEFT,
    LIST_TOUCH_SEASON_RIGHT,
    LIST_TOUCH_ICON,
    LIST_TOUCH_COUNT = LIST_TOUCH_ICON + TOUCH_ICON_COUNT,
};

// And the cursor's
enum {
    CURSOR_TOUCH_RETURN,
    CURSOR_TOUCH_INFO,
    CURSOR_TOUCH_ICON,
    CURSOR_TOUCH_COUNT = CURSOR_TOUCH_ICON + TOUCH_ICON_COUNT,
};

typedef struct {
    BOOL finished;
    u32 seq;
    u32 step;
    PokedexHabitatListParam *param;
    PokedexHabitatGraphic graphic;
    MsgData *msgData;
    TCB *vblankTask;
    // Whether the cursor is on a Pokémon
    BOOL cursorShown;
    int cursor;
    // The Pokémon that was touched
    int touched;
    int areaCount;
    int speciesCount;
    int area;
    int season;
    int type;
    WordSet *wordSet;
    // The stamp that the area is being given, an AWARD_*
    u32 award;
    // Whether each kind of habitat has Pokémon in the area, and how many of them the player has, as an AWARD_*
    BOOL typeShown[TYPE_COUNT];
    u32 typeState[TYPE_COUNT];
    u32 species[POKEDEX_HABITAT_ICON_COUNT];
    // An ACTION_*
    u32 action;
    PokeDexSave *pokedex;
    PokedexHabitatSave *habitat;
    int slideMain;
    int slideSub;
    // The kind of habitat that the player last chose
    u32 chosenType;
    u32 timer;
    // Whether the kind of habitat has no Pokémon in this season
    BOOL empty;
} PokedexHabitatWork; // size: 0x94c

typedef void (*PokedexHabitatSeqFunc)(PokedexHabitatWork *wk);

static BOOL PokedexHabitat_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexHabitat_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexHabitat_ProcEnd(GameProc *proc, u32 *state, void *param, void *work);
static void PokedexHabitat_AddVBlank(PokedexHabitatWork *wk);
static void PokedexHabitat_RemoveVBlank(PokedexHabitatWork *wk);
static void PokedexHabitat_VBlank(TCB *tcb, void *data);
static PokedexHabitatWork *PokedexHabitat_CreateWork(GameProc *proc, PokedexHabitatListParam *param);
static void PokedexHabitat_FreeWork(GameProc *proc, PokedexHabitatWork *wk);
static BOOL PokedexHabitat_RunSequence(PokedexHabitatWork *wk);
static void PokedexHabitat_SetSequence(PokedexHabitatWork *wk, u32 seq, u32 action);
static void PokedexHabitat_SeqFadeIn(PokedexHabitatWork *wk);
static void PokedexHabitat_SeqFadeOut(PokedexHabitatWork *wk);
static void PokedexHabitat_SeqList(PokedexHabitatWork *wk);
static void PokedexHabitat_SelectType(PokedexHabitatWork *wk, int type);
static void PokedexHabitat_ListKeys(PokedexHabitatWork *wk);
static void PokedexHabitat_SeqLeaveCursor(PokedexHabitatWork *wk);
static void PokedexHabitat_SeqCursor(PokedexHabitatWork *wk);
static void PokedexHabitat_CursorKeys(PokedexHabitatWork *wk);
static void PokedexHabitat_SeqInit(PokedexHabitatWork *wk);
static void PokedexHabitat_SeqButton(PokedexHabitatWork *wk);
static void PokedexHabitat_SeqChangeSeason(PokedexHabitatWork *wk);
static void PokedexHabitat_SeqChangeArea(PokedexHabitatWork *wk);
static void PokedexHabitat_UpdateStamp(PokedexHabitatWork *wk);
static void PokedexHabitat_SeqChangeType(PokedexHabitatWork *wk);
static void PokedexHabitat_SkipType(PokedexHabitatWork *wk, int forward);
static void PokedexHabitat_SeqMoveCursor(PokedexHabitatWork *wk);
static void PokedexHabitat_SeqAward(PokedexHabitatWork *wk);
static void PokedexHabitat_SeqAllComplete(PokedexHabitatWork *wk);
static void PokedexHabitat_PrintHabitat(PokedexHabitatWork *wk);
static u32 PokedexHabitat_GetHabitatMessage(u32 type, u32 land);
static u32 PokedexHabitat_ChooseMessage(u32 type, u32 land, const u32 *landMessages, u32 water, u32 fishing);
static void PokedexHabitat_PrintInfoLabel(PokedexHabitatWork *wk, u16 color);
static void PokedexHabitat_ClearInfoLabel(PokedexHabitatWork *wk);
static void PokedexHabitat_PrintAreaName(PokedexHabitatWork *wk);
static PokedexHabitatArea *PokedexHabitat_GetArea(PokedexHabitatWork *wk);
static void PokedexHabitat_LoadSpecies(PokedexHabitatWork *wk);
static void PokedexHabitat_UpdateTabs(PokedexHabitatWork *wk);
static void PokedexHabitat_UpdateNewMarks(PokedexHabitatWork *wk);
static void PokedexHabitat_UpdateNewMark(PokedexHabitatWork *wk, u32 type, ClActor *actor);
static void PokedexHabitat_UpdateList(PokedexHabitatWork *wk);
static void PokedexHabitat_SetCursor(PokedexHabitatWork *wk, int cell, BOOL shown);
static void PokedexHabitat_CheckTypes(PokedexHabitatWork *wk);
static void PokedexHabitat_HideListButtons(PokedexHabitatWork *wk);
static void PokedexHabitat_ShowListButtons(PokedexHabitatWork *wk);
static void PokedexHabitat_UpdateMap(PokedexHabitatWork *wk);
static void PokedexHabitat_SetResult(PokedexHabitatWork *wk, u32 result);
static void PokedexHabitat_LoadSave(PokedexHabitatWork *wk);
static int PokedexHabitat_GetSpecies(PokedexHabitatWork *wk, u8 cell);
static u32 PokedexHabitat_GetAward(PokedexHabitatWork *wk);
static u32 PokedexHabitat_GetTypeAward(PokedexHabitatWork *wk, u32 type);
static void PokedexHabitat_SetAward(PokedexHabitatWork *wk, u32 award);
static int PokedexHabitat_GetHabitatIndex(u16 zone);
static void PokedexHabitat_ToggleShortcut(PokedexHabitatWork *wk);
static void PokedexHabitat_UpdateShortcut(PokedexHabitatWork *wk);
static BOOL PokedexHabitat_HasSeveralTypes(PokedexHabitatWork *wk);
static void PokedexHabitat_ShowInfoButton(PokedexHabitatWork *wk, BOOL known, BOOL shown);

// clang-format off
static const PokedexHabitatSeqFunc sPokedexHabitatSequences[SEQ_COUNT + 1] = {
    PokedexHabitat_SeqInit,         PokedexHabitat_SeqFadeIn,     PokedexHabitat_SeqFadeOut,     PokedexHabitat_SeqList,
    PokedexHabitat_SeqLeaveCursor,  PokedexHabitat_SeqChangeSeason, PokedexHabitat_SeqChangeArea, PokedexHabitat_SeqChangeType,
    PokedexHabitat_SeqCursor,       PokedexHabitat_SeqMoveCursor, PokedexHabitat_SeqAward,       PokedexHabitat_SeqButton,
    PokedexHabitat_SeqAllComplete,  NULL,
};

// clang-format on

// BUG: neither table ends with TOUCH_RECT_END, so the search runs on into the data that follows them
#if defined(PLATFORM_PC) || defined(BUGFIX)
#define POKEDEX_HABITAT_TOUCH_CAPACITY(count) ((count) + 1)
#else
#define POKEDEX_HABITAT_TOUCH_CAPACITY(count) count
#endif

// clang-format off
static const TouchRect sPokedexHabitatCursorTouchRects[POKEDEX_HABITAT_TOUCH_CAPACITY(CURSOR_TOUCH_COUNT)] = {
    { 168, 191, 232, 255 }, { 168, 191, 96, 212 },
    { 48, 80, 32, 72 },     { 48, 80, 72, 112 },    { 48, 80, 112, 152 },   { 48, 80, 152, 192 },   { 48, 80, 192, 232 },
    { 80, 112, 32, 72 },    { 80, 112, 72, 112 },   { 80, 112, 112, 152 },  { 80, 112, 152, 192 },  { 80, 112, 192, 232 },
    { 112, 144, 32, 72 },   { 112, 144, 72, 112 },  { 112, 144, 112, 152 }, { 112, 144, 152, 192 }, { 112, 144, 192, 232 },
    { 144, 176, 32, 72 },   { 144, 176, 72, 112 },  { 144, 176, 112, 152 }, { 144, 176, 152, 192 }, { 144, 176, 192, 232 },
#if defined(PLATFORM_PC) || defined(BUGFIX)
    { TOUCH_RECT_END, 0, 0, 0 },
#endif
};

// clang-format on

// clang-format off
static const TouchRect sPokedexHabitatListTouchRects[POKEDEX_HABITAT_TOUCH_CAPACITY(LIST_TOUCH_COUNT)] = {
    { 168, 191, 184, 207 }, { 168, 191, 208, 231 }, { 168, 191, 232, 255 }, { 168, 191, 68, 92 },   { 168, 191, 108, 132 },
    { 168, 191, 148, 172 }, { 168, 191, 0, 24 },    { 168, 191, 24, 48 },   { 52, 76, 12, 36 },     { 52, 76, 228, 244 },
    { 48, 80, 32, 72 },     { 48, 80, 72, 112 },    { 48, 80, 112, 152 },   { 48, 80, 152, 192 },   { 48, 80, 192, 232 },
    { 80, 112, 32, 72 },    { 80, 112, 72, 112 },   { 80, 112, 112, 152 },  { 80, 112, 152, 192 },  { 80, 112, 192, 232 },
    { 112, 144, 32, 72 },   { 112, 144, 72, 112 },  { 112, 144, 112, 152 }, { 112, 144, 152, 192 }, { 112, 144, 192, 232 },
    { 144, 176, 32, 72 },   { 144, 176, 72, 112 },  { 144, 176, 112, 152 }, { 144, 176, 152, 192 }, { 144, 176, 192, 232 },
#if defined(PLATFORM_PC) || defined(BUGFIX)
    { TOUCH_RECT_END, 0, 0, 0 },
#endif
};

// clang-format on

// The areas of the habitat list, as in pokedex_common.c. An area's place in it is its habitat in the save
static const PokedexHabitatTableEntry sPokedexHabitatAreas[] = {
    { EVENT_FLAG_ARRIVED_ASPERTIA_CITY, ZONE_ASPERTIA_CITY, 24, 0 },
    { EVENT_FLAG_0x09c0, ZONE_ROUTE_19, 23, 0 },
    { EVENT_FLAG_0x09c1, ZONE_ROUTE_20, 28, 0 },
    { EVENT_FLAG_0x09c2, ZONE_FLOCCESY_RANCH, 26, 0 },
    { EVENT_FLAG_ARRIVED_VIRBANK_CITY, ZONE_VIRBANK_CITY, 29, 0 },
    { EVENT_FLAG_0x09c3, ZONE_VIRBANK_COMPLEX, 32, 0 },
    { EVENT_FLAG_ARRIVED_CASTELIA_CITY, ZONE_CASTELIA_CITY, 34, 0 },
    { EVENT_FLAG_0x09c4, ZONE_CASTELIA_SEWERS, 30, 0 },
    { EVENT_FLAG_0x09c5, ZONE_ROUTE_4, 0, 0 },
    { EVENT_FLAG_0x09c6, ZONE_DESERT_RESORT, 41, 0 },
    { EVENT_FLAG_0x09c7, ZONE_RELIC_CASTLE, 21, 0 },
    { EVENT_FLAG_0x09c8, ZONE_ROUTE_5, 9, 0 },
    { EVENT_FLAG_0x09c9, ZONE_ROUTE_16, 8, 0 },
    { EVENT_FLAG_0x09ca, ZONE_LOSTLORN_FOREST, 55, 0 },
    { EVENT_FLAG_0x09cb, ZONE_DRIFTVEIL_DRAWBRIDGE, 53, 0 },
    { EVENT_FLAG_0x09cc, ZONE_ROUTE_6, 13, 0 },
    { EVENT_FLAG_0x09cd, ZONE_RELIC_PASSAGE, 7, 0 },
    { EVENT_FLAG_0x09f0, ZONE_CLAY_TUNNEL, 3, 0 },
    { EVENT_FLAG_0x09ce, ZONE_MISTRALTON_CAVE, 49, 0 },
    { EVENT_FLAG_0x09cf, ZONE_CHARGESTONE_CAVE, 57, 0 },
    { EVENT_FLAG_0x09d0, ZONE_ROUTE_7, 19, 0 },
    { EVENT_FLAG_0x09d1, ZONE_CELESTIAL_TOWER_2, 39, 0 },
    { EVENT_FLAG_0x09d2, ZONE_REVERSAL_MOUNTAIN, 37, 0 },
    { EVENT_FLAG_0x09d3, ZONE_STRANGE_HOUSE, 27, 0 },
    { EVENT_FLAG_ARRIVED_UNDELLA_TOWN, ZONE_UNDELLA_TOWN, 12, 0 },
    { EVENT_FLAG_0x09d4, ZONE_ROUTE_13, 50, 0 },
    { EVENT_FLAG_0x09d5, ZONE_UNDELLA_BAY, 14, 0 },
    { EVENT_FLAG_0x09d6, ZONE_ROUTE_14, 56, 0 },
    { EVENT_FLAG_0x09d7, ZONE_ABUNDANT_SHRINE, 11, 0 },
    { EVENT_FLAG_0x09d8, ZONE_ROUTE_12, 47, 0 },
    { EVENT_FLAG_0x09d9, ZONE_VILLAGE_BRIDGE, 5, 0 },
    { EVENT_FLAG_0x09da, ZONE_ROUTE_11, 42, 0 },
    { EVENT_FLAG_0x09db, ZONE_ROUTE_9, 31, 0 },
    { EVENT_FLAG_0x09dc, ZONE_SEASIDE_CAVE, 54, 0 },
    { EVENT_FLAG_0x09dd, ZONE_ROUTE_21, 2, 0 },
    { EVENT_FLAG_0x09de, ZONE_ROUTE_15, 1, 0 },
    { EVENT_FLAG_0x09bf, ZONE_MARVELOUS_BRIDGE, 10, 0 },
    { EVENT_FLAG_ARRIVED_HUMILAU_CITY, ZONE_HUMILAU_CITY, 36, 0 },
    { EVENT_FLAG_0x09df, ZONE_ROUTE_22, 40, 0 },
    { EVENT_FLAG_0x09e0, ZONE_GIANT_CHASM, 18, 0 },
    { EVENT_FLAG_0x09e1, ZONE_ROUTE_23, 45, 0 },
    { EVENT_FLAG_ARRIVED_VICTORY_ROAD, ZONE_VICTORY_ROAD, 20, 0 },
    { EVENT_FLAG_0x09e3, ZONE_ROUTE_8, 25, 0 },
    { EVENT_FLAG_0x09e4, ZONE_MOOR_OF_ICIRRUS, 46, 0 },
    { EVENT_FLAG_ARRIVED_ICIRRUS_CITY, ZONE_ICIRRUS_CITY, 52, 0 },
    { EVENT_FLAG_0x09e5, ZONE_DRAGONSPIRAL_TOWER, 6, 0 },
    { EVENT_FLAG_0x09e6, ZONE_TWIST_MOUNTAIN, 4, 0 },
    { EVENT_FLAG_0x09e7, ZONE_PINWHEEL_FOREST, 35, 0 },
    { EVENT_FLAG_0x09e8, ZONE_ROUTE_3, 51, 0 },
    { EVENT_FLAG_0x09e9, ZONE_WELLSPRING_CAVE, 15, 0 },
    { EVENT_FLAG_ARRIVED_STRIATON_CITY, ZONE_STRIATON_CITY, 22, 0 },
    { EVENT_FLAG_0x09ea, ZONE_DREAMYARD, 38, 0 },
    { EVENT_FLAG_0x09ec, ZONE_ROUTE_2, 48, 0 },
    { EVENT_FLAG_0x09eb, ZONE_ROUTE_1, 44, 0 },
    { EVENT_FLAG_0x09ed, ZONE_ROUTE_17, 43, 0 },
    { EVENT_FLAG_0x09ee, ZONE_ROUTE_18, 17, 0 },
    { EVENT_FLAG_0x09ef, ZONE_P2_LABORATORY, 33, 0 },
};

static BOOL PokedexHabitat_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexHabitatWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_POKEDEX_HABITAT, 0x60000);
    wk = PokedexHabitat_CreateWork(proc, param);
    PokedexHabitatGraphic_Init(&wk->graphic, wk->param->gameData);
    PokedexHabitat_AddVBlank(wk);
    return TRUE;
}

static BOOL PokedexHabitat_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexHabitatWork *wk = work;

    if (PokedexHabitat_RunSequence(wk)) {
        PokedexHabitatGraphic_Main(&wk->graphic);
        return FALSE;
    }
    return TRUE;
}

static BOOL PokedexHabitat_ProcEnd(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexHabitatWork *wk = work;

    PokedexHabitat_RemoveVBlank(wk);
    PokedexHabitatGraphic_Exit(&wk->graphic);
    PokedexHabitat_FreeWork(proc, wk);
    GFL_HeapDelete(HEAPID_POKEDEX_HABITAT);
    return TRUE;
}

static void PokedexHabitat_AddVBlank(PokedexHabitatWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(PokedexHabitat_VBlank, wk, 0);
}

static void PokedexHabitat_RemoveVBlank(PokedexHabitatWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static void PokedexHabitat_VBlank(TCB *tcb, void *data) {
    PokedexHabitatWork *wk = data;

    PokedexHabitatGraphic_VBlank(&wk->graphic);
}

static PokedexHabitatWork *PokedexHabitat_CreateWork(GameProc *proc, PokedexHabitatListParam *param) {
    PokedexHabitatWork *wk = GFL_ProcInitSubsystem(proc, sizeof(PokedexHabitatWork), HEAPID_POKEDEX_HABITAT);

    MI_CpuFill8(wk, 0, sizeof(PokedexHabitatWork));
    wk->param = param;
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_POKEDEX_HABITAT, HEAPID_POKEDEX_HABITAT);
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_POKEDEX_HABITAT);
    return wk;
}

static void PokedexHabitat_FreeWork(GameProc *proc, PokedexHabitatWork *wk) {
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_MsgDataFree(wk->msgData);
    GFL_ProcReleaseSubsystem(proc);
}

// Returns whether the screen goes on
static BOOL PokedexHabitat_RunSequence(PokedexHabitatWork *wk) {
    if (sPokedexHabitatSequences[wk->seq] != NULL) {
        sPokedexHabitatSequences[wk->seq](wk);
    }
    if (wk->finished) {
        return FALSE;
    }
    return TRUE;
}

static void PokedexHabitat_SetSequence(PokedexHabitatWork *wk, u32 seq, u32 action) {
    wk->seq = seq;
    wk->action = action;
    wk->step = 0;
}

static void PokedexHabitat_SeqFadeIn(PokedexHabitatWork *wk) {
    switch (wk->step) {
    case 0:
        if (PokedexHabitatGraphic_IsPrinted(&wk->graphic)) {
            wk->slideMain = SLIDE_DISTANCE;
            wk->slideSub = -SLIDE_DISTANCE;
            PokedexHabitatGraphic_StartFade(&wk->graphic, 0, 0);
            PokedexHabitatGraphic_Scroll(&wk->graphic, wk->slideMain, TRUE);
            PokedexHabitatGraphic_Scroll(&wk->graphic, wk->slideSub, FALSE);
            wk->step++;
        }
        break;
    case 1:
        if (!PokedexHabitatGraphic_IsFading(&wk->graphic)) {
            GX_SetMasterBrightness(0);
            GXS_SetMasterBrightness(0);
            wk->step++;
        }
        break;
    case 2:
        if (!PokedexHabitatGraphic_IsFading(&wk->graphic)) {
            GFL_BGSysSetBGEnabled(6, TRUE);
            GFL_BGSysSetBGEnabled(7, TRUE);
        }
        if (wk->slideMain <= 0) {
            u32 award = PokedexHabitat_GetAward(wk);

            if (award != AWARD_NONE) {
                PokedexHabitat_SetAward(wk, award);
                PokedexHabitat_SetSequence(wk, SEQ_AWARD, ACTION_NONE);
            } else {
                PokedexHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
            }
        } else {
            PokedexHabitatGraphic_Scroll(&wk->graphic, -SLIDE_SPEED, TRUE);
            PokedexHabitatGraphic_Scroll(&wk->graphic, SLIDE_SPEED, FALSE);
            wk->slideMain -= SLIDE_SPEED;
            wk->slideSub += SLIDE_SPEED;
        }
        break;
    }
}

static void PokedexHabitat_SeqFadeOut(PokedexHabitatWork *wk) {
    switch (wk->step) {
    case 0:
        if (PokedexHabitatGraphic_IsPrinted(&wk->graphic)) {
            u16 backdrop;

            PokedexHabitatGraphic_SetBlinkStopped(&wk->graphic, TRUE);
            wk->slideMain = 0;
            wk->slideSub = 0;
            PokedexHabitatGraphic_StartFade(&wk->graphic, 16, 16);
            PokedexHabitat_ClearInfoLabel(wk);
            ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_INFO), FALSE);
            PokedexHabitatGraphic_SetNewMarksVisible(&wk->graphic, FALSE);
            backdrop = 0;
            GXS_LoadBGPltt(&backdrop, 0, sizeof(backdrop));
            GFL_BGSysSetBGEnabled(6, FALSE);
            GFL_BGSysSetBGEnabled(7, FALSE);
            wk->step++;
        }
        break;
    case 1:
        if (!PokedexHabitatGraphic_IsFading(&wk->graphic)) {
            if (wk->slideMain >= SLIDE_DISTANCE) {
                GX_SetMasterBrightness(-16);
                GXS_SetMasterBrightness(-16);
                wk->finished = TRUE;
            } else {
                PokedexHabitatGraphic_Scroll(&wk->graphic, SLIDE_SPEED, TRUE);
                PokedexHabitatGraphic_Scroll(&wk->graphic, -SLIDE_SPEED, FALSE);
                wk->slideMain += SLIDE_SPEED;
                wk->slideSub -= SLIDE_SPEED;
            }
        }
        break;
    }
}

static void PokedexHabitat_SeqList(PokedexHabitatWork *wk) {
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);
    int hit = GFL_HIDGetTouchPressedRect(sPokedexHabitatListTouchRects);

    switch (hit) {
    case LIST_TOUCH_SHORTCUT:
        GFL_SndSEPlay(SEQ_SE_SYS_07);
        PokedexHabitat_ToggleShortcut(wk);
        break;
    case LIST_TOUCH_EXIT:
        PokedexHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_EXIT);
        break;
    case LIST_TOUCH_RETURN:
        PokedexHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_RETURN);
        break;
    case LIST_TOUCH_TAB + TYPE_LAND:
        PokedexHabitat_SelectType(wk, TYPE_LAND);
        break;
    case LIST_TOUCH_TAB + TYPE_WATER:
        PokedexHabitat_SelectType(wk, TYPE_WATER);
        break;
    case LIST_TOUCH_TAB + TYPE_FISHING:
        PokedexHabitat_SelectType(wk, TYPE_FISHING);
        break;
    case LIST_TOUCH_AREA_UP:
        if (wk->areaCount > 1) {
            PokedexHabitat_SetSequence(wk, SEQ_CHANGE_AREA, ACTION_UP);
        }
        break;
    case LIST_TOUCH_AREA_DOWN:
        if (wk->areaCount > 1) {
            PokedexHabitat_SetSequence(wk, SEQ_CHANGE_AREA, ACTION_DOWN);
        }
        break;
    case LIST_TOUCH_SEASON_LEFT:
        if (area->values[wk->type] == 1) {
            PokedexHabitat_SetSequence(wk, SEQ_CHANGE_SEASON, ACTION_LEFT);
        }
        break;
    case LIST_TOUCH_SEASON_RIGHT:
        if (area->values[wk->type] == 1) {
            PokedexHabitat_SetSequence(wk, SEQ_CHANGE_SEASON, ACTION_RIGHT);
        }
        break;
    case LIST_TOUCH_ICON + 0:
    case LIST_TOUCH_ICON + 1:
    case LIST_TOUCH_ICON + 2:
    case LIST_TOUCH_ICON + 3:
    case LIST_TOUCH_ICON + 4:
    case LIST_TOUCH_ICON + 5:
    case LIST_TOUCH_ICON + 6:
    case LIST_TOUCH_ICON + 7:
    case LIST_TOUCH_ICON + 8:
    case LIST_TOUCH_ICON + 9:
    case LIST_TOUCH_ICON + 10:
    case LIST_TOUCH_ICON + 11:
    case LIST_TOUCH_ICON + 12:
    case LIST_TOUCH_ICON + 13:
    case LIST_TOUCH_ICON + 14:
    case LIST_TOUCH_ICON + 15:
    case LIST_TOUCH_ICON + 16:
    case LIST_TOUCH_ICON + 17:
    case LIST_TOUCH_ICON + 18:
    case LIST_TOUCH_ICON + 19:
        if (wk->speciesCount > 0) {
            int cell = hit - LIST_TOUCH_ICON;

            if (cell < wk->speciesCount && PokedexHabitat_GetSpecies(wk, cell) >= 0) {
                wk->touched = cell;
                PokedexHabitat_SetSequence(wk, SEQ_CURSOR, ACTION_TOUCH);
            }
        }
        break;
    case TOUCH_RECT_NONE:
        PokedexHabitat_ListKeys(wk);
        break;
    }
}

static void PokedexHabitat_SelectType(PokedexHabitatWork *wk, int type) {
    if (wk->type != type && PokedexHabitat_HasSeveralTypes(wk) && wk->typeShown[type] == TRUE) {
        wk->type = type;
        PokedexHabitat_SetSequence(wk, SEQ_CHANGE_TYPE, ACTION_TAB);
    }
}

static void PokedexHabitat_ListKeys(PokedexHabitatWork *wk) {
    u32 repeat = GCTX_HIDGetTypedKeys();
    u32 pressed = GCTX_HIDGetPressedKeys();
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);

    if (repeat & PAD_BUTTON_L) {
        if (area->values[wk->type] == 1) {
            PokedexHabitat_SetSequence(wk, SEQ_CHANGE_SEASON, ACTION_LEFT);
        }
    } else if (repeat & PAD_BUTTON_R) {
        if (area->values[wk->type] == 1) {
            PokedexHabitat_SetSequence(wk, SEQ_CHANGE_SEASON, ACTION_RIGHT);
        }
    } else if (repeat & PAD_KEY_LEFT) {
        if (PokedexHabitat_HasSeveralTypes(wk)) {
            PokedexHabitat_SetSequence(wk, SEQ_CHANGE_TYPE, ACTION_LEFT);
        }
    } else if (repeat & PAD_KEY_RIGHT) {
        if (PokedexHabitat_HasSeveralTypes(wk)) {
            PokedexHabitat_SetSequence(wk, SEQ_CHANGE_TYPE, ACTION_RIGHT);
        }
    } else if (repeat & PAD_KEY_UP) {
        if (wk->areaCount > 1) {
            PokedexHabitat_SetSequence(wk, SEQ_CHANGE_AREA, ACTION_UP);
        }
    } else if (repeat & PAD_KEY_DOWN) {
        if (wk->areaCount > 1) {
            PokedexHabitat_SetSequence(wk, SEQ_CHANGE_AREA, ACTION_DOWN);
        }
    } else if (pressed & PAD_BUTTON_A) {
        if (wk->speciesCount > 0) {
            int cell = 0;

            if (PokedexHabitat_GetSpecies(wk, 0) >= 0) {
                wk->cursor = cell;
                PokedexHabitat_SetSequence(wk, SEQ_CURSOR, ACTION_DECIDE);
            }
        }
    } else if (pressed & PAD_BUTTON_B) {
        PokedexHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_RETURN);
    } else if (pressed & PAD_BUTTON_X) {
        PokedexHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_EXIT);
    } else if (pressed & PAD_BUTTON_Y) {
        GFL_SndSEPlay(SEQ_SE_SYS_07);
        PokedexHabitat_ToggleShortcut(wk);
    }
}

// Back from the cursor to the list
static void PokedexHabitat_SeqLeaveCursor(PokedexHabitatWork *wk) {
    ClActor *actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_RETURN);

    switch (wk->step) {
    case 0:
        ClActor_SetAnimTime(actor, 0);
        ClActor_SetSequence(actor, 9);
        ClActor_SetAutoAnim(actor, TRUE);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        wk->step++;
        break;
    case 1:
        if (!GFL_SndPlayerIsActiveAny() && !ClActor_IsAnimActive(actor)) {
            PokedexHabitat_SetCursor(wk, wk->cursor, FALSE);
            PokedexHabitat_ShowListButtons(wk);
            ClActor_SetSequence(actor, 1);
            ClActor_SetAutoAnim(actor, TRUE);
            ClActor_SetAnimTime(actor, 0);
            PokedexHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
        }
        break;
    }
}

// The cursor on the Pokémon, which chooses one to see in the Pokédex
static void PokedexHabitat_SeqCursor(PokedexHabitatWork *wk) {
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);
    int hit = GFL_HIDGetTouchPressedRect(sPokedexHabitatCursorTouchRects);

    switch (wk->step) {
    case 0:
        PokedexHabitat_HideListButtons(wk);
        if (wk->action == ACTION_DECIDE) {
            PokedexHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_NONE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if (wk->action == ACTION_TOUCH) {
            PokedexHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_TOUCH_DECIDE);
        } else {
            wk->step++;
        }
        break;
    case 1:
        if (!GFL_SndPlayerIsActiveAny()) {
            wk->step++;
        }
        break;
    case 2:
        switch (hit) {
        case CURSOR_TOUCH_RETURN:
            PokedexHabitat_SetSequence(wk, SEQ_LEAVE_CURSOR, ACTION_RETURN);
            break;
        case CURSOR_TOUCH_INFO:
            if (PokedexHabitat_GetSpecies(wk, wk->cursor) >= 0) {
                PokedexHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_INFO);
            }
            break;
        case CURSOR_TOUCH_ICON + 0:
        case CURSOR_TOUCH_ICON + 1:
        case CURSOR_TOUCH_ICON + 2:
        case CURSOR_TOUCH_ICON + 3:
        case CURSOR_TOUCH_ICON + 4:
        case CURSOR_TOUCH_ICON + 5:
        case CURSOR_TOUCH_ICON + 6:
        case CURSOR_TOUCH_ICON + 7:
        case CURSOR_TOUCH_ICON + 8:
        case CURSOR_TOUCH_ICON + 9:
        case CURSOR_TOUCH_ICON + 10:
        case CURSOR_TOUCH_ICON + 11:
        case CURSOR_TOUCH_ICON + 12:
        case CURSOR_TOUCH_ICON + 13:
        case CURSOR_TOUCH_ICON + 14:
        case CURSOR_TOUCH_ICON + 15:
        case CURSOR_TOUCH_ICON + 16:
        case CURSOR_TOUCH_ICON + 17:
        case CURSOR_TOUCH_ICON + 18:
        case CURSOR_TOUCH_ICON + 19:
            if (hit - CURSOR_TOUCH_ICON < wk->speciesCount) {
                wk->touched = hit - CURSOR_TOUCH_ICON;
                PokedexHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_TOUCH);
            }
            break;
        case TOUCH_RECT_NONE:
            PokedexHabitat_CursorKeys(wk);
            break;
        }
        break;
    }
}

static void PokedexHabitat_CursorKeys(PokedexHabitatWork *wk) {
    u32 pressed = GCTX_HIDGetPressedKeys();
    u32 repeat = GCTX_HIDGetTypedKeys();

    if (wk->cursorShown == TRUE) {
        if (repeat & PAD_KEY_LEFT) {
            PokedexHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_LEFT);
        } else if (repeat & PAD_KEY_RIGHT) {
            PokedexHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_RIGHT);
        } else if (repeat & PAD_KEY_UP) {
            PokedexHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_UP);
        } else if (repeat & PAD_KEY_DOWN) {
            PokedexHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_DOWN);
        } else if (pressed & PAD_BUTTON_B) {
            PokedexHabitat_SetSequence(wk, SEQ_LEAVE_CURSOR, ACTION_RETURN);
        } else if (pressed & PAD_BUTTON_A) {
            if (PokedexHabitat_GetSpecies(wk, wk->cursor) >= 0) {
                PokedexHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_INFO);
            }
        }
    } else {
        // After a touch, any key brings the cursor back
        if (pressed & PAD_BUTTON_B) {
            PokedexHabitat_SetSequence(wk, SEQ_LEAVE_CURSOR, ACTION_RETURN);
        } else if (pressed) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            PokedexHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_NONE);
        }
    }
}

static void PokedexHabitat_SeqInit(PokedexHabitatWork *wk) {
    switch (wk->step) {
    case 0:
        PokedexHabitatGraphic_FillWindow(POKEDEX_HABITAT_WIN_MAIN, 0, &wk->graphic);
        GFL_NetSetIconScreen(TRUE, HEAPID_POKEDEX_HABITAT);
        PokedexHabitat_LoadSave(wk);
        PokedexHabitat_CheckTypes(wk);
        PokedexHabitat_UpdateList(wk);
        PokedexHabitat_UpdateTabs(wk);
        PokedexHabitat_UpdateMap(wk);
        PokedexHabitat_UpdateStamp(wk);
        PokedexHabitat_UpdateShortcut(wk);
        wk->step++;
        break;
    case 1:
        if (PokedexHabitatGraphic_AreIconsLoaded(&wk->graphic) &&
            !PokedexHabitatGraphic_IsPaletteRequested(&wk->graphic)) {
            PokedexHabitat_SetSequence(wk, SEQ_FADE_IN, ACTION_NONE);
            wk->finished = FALSE;
        }
        break;
    }
}

// A button that leaves the screen: it sets the result, and saves what the list showed
static void PokedexHabitat_SeqButton(PokedexHabitatWork *wk) {
    ClActor *actor;

    if (wk->action == ACTION_EXIT) {
        actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_EXIT);
        PokedexHabitat_SetResult(wk, POKEDEX_HABITAT_RESULT_EXIT);
    } else if (wk->action == ACTION_RETURN) {
        actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_RETURN);
        PokedexHabitat_SetResult(wk, POKEDEX_HABITAT_RESULT_RETURN);
    } else if (wk->action == ACTION_INFO) {
        actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_INFO);
        PokedexHabitat_SetResult(wk, POKEDEX_HABITAT_RESULT_INFO);
        PokedexHabitatSave_SetType(wk->habitat, (u16)wk->type);
        PokedexHabitatSave_SetSeason(wk->habitat, (u16)wk->season);
    }

    switch (wk->step) {
    case 0:
        PokedexHabitatSave_SetZone(wk->habitat, PokedexHabitat_GetArea(wk)->zone);
        if (wk->action == ACTION_EXIT) {
            ClActor_SetSequence(actor, 8);
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
        } else if (wk->action == ACTION_RETURN) {
            ClActor_SetSequence(actor, 9);
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else if (wk->action == ACTION_INFO) {
            int species = PokedexHabitat_GetSpecies(wk, wk->cursor);

            if (species >= 0) {
                PokeDex_SetLastSpecies(wk->pokedex, species);
            }
            ClActor_SetSequence(actor, 29);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        ClActor_SetAnimTime(actor, 0);
        ClActor_SetAutoAnim(actor, TRUE);
        wk->step++;
        break;
    case 1:
        if (!ClActor_IsAnimActive(actor)) {
            PokedexHabitat_SetSequence(wk, SEQ_FADE_OUT, ACTION_NONE);
        }
        break;
    }
}

static void PokedexHabitat_SeqChangeSeason(PokedexHabitatWork *wk) {
    ClActor *pressed;
    ClActor *other;
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);

    if (wk->action == ACTION_LEFT) {
        pressed = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_LEFT);
        other = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_RIGHT);
    } else if (wk->action == ACTION_RIGHT) {
        pressed = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_RIGHT);
        other = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_LEFT);
    }

    switch (wk->step) {
    case 0:
        ClActor_RestartAnim(other);
        ClActor_StopAnim(other);
        if (wk->action == ACTION_LEFT) {
            ClActor_SetSequence(pressed, 25);
            wk->season--;
            if (wk->season < 0) {
                wk->season = SEASON_COUNT - 1;
            }
        } else if (wk->action == ACTION_RIGHT) {
            ClActor_SetSequence(pressed, 23);
            wk->season++;
            if (wk->season >= SEASON_COUNT) {
                wk->season = 0;
            }
        }
        ClActor_SetAnimFrame(pressed, 0);
        ClActor_SetAutoAnim(pressed, TRUE);
        PokedexHabitat_UpdateList(wk);
        PokedexHabitat_UpdateTabs(wk);
        GFL_SndSEPlay(SEQ_SE_SELECT3);
        wk->step++;
        // fallthrough
    case 1:
        if (PokedexHabitatGraphic_AreIconsLoaded(&wk->graphic) &&
            !PokedexHabitatGraphic_IsPaletteRequested(&wk->graphic) && !ClActor_IsAnimActive(pressed)) {
            if (wk->action == ACTION_LEFT) {
                ClActor *right = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_RIGHT);

                ClActor_SetSequence(pressed, 24);
                ClActor_RestartAnim(pressed);
                ClActor_RestartAnim(right);
            } else if (wk->action == ACTION_RIGHT) {
                ClActor *left = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_LEFT);

                ClActor_SetSequence(pressed, 22);
                ClActor_RestartAnim(pressed);
                ClActor_RestartAnim(left);
            }
            ClActor_StartAnim(other);
            PokedexHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
        }
        break;
    }
}

static void PokedexHabitat_SeqChangeArea(PokedexHabitatWork *wk) {
    ClActor *actor;
    int anim;

    if (wk->action == ACTION_UP) {
        actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_AREA_UP);
        anim = 11;
    } else if (wk->action == ACTION_DOWN) {
        actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_AREA_DOWN);
        anim = 10;
    }

    switch (wk->step) {
    case 0:
        wk->season = PokedexHabitatSave_GetSeason(wk->habitat);
        wk->type = wk->chosenType;
        if (wk->action == ACTION_UP) {
            wk->area--;
            if (wk->area < 0) {
                wk->area = wk->areaCount - 1;
            }
        } else if (wk->action == ACTION_DOWN) {
            wk->area++;
            if (wk->area >= wk->areaCount) {
                wk->area = 0;
            }
        }
        ClActor_SetAnimTime(actor, 0);
        ClActor_SetSequence(actor, anim);
        ClActor_SetAutoAnim(actor, TRUE);
        ClActor_StartAnim(actor);
        wk->step++;
        // fallthrough
    case 1:
        PokedexHabitat_CheckTypes(wk);
        PokedexHabitat_UpdateList(wk);
        PokedexHabitat_UpdateTabs(wk);
        PokedexHabitat_UpdateMap(wk);
        PokedexHabitat_UpdateStamp(wk);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        wk->step++;
        // fallthrough
    case 2:
        if (PokedexHabitatGraphic_AreIconsLoaded(&wk->graphic) &&
            !PokedexHabitatGraphic_IsPaletteRequested(&wk->graphic) && PokedexHabitatGraphic_IsPrinted(&wk->graphic)) {
            u32 award = PokedexHabitat_GetAward(wk);

            if (award != AWARD_NONE) {
                PokedexHabitat_SetAward(wk, award);
                PokedexHabitat_SetSequence(wk, SEQ_AWARD, ACTION_NONE);
            } else {
                PokedexHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
            }
        }
        break;
    }
}

// Shows the stamp that the area's kind of habitat has
static void PokedexHabitat_UpdateStamp(PokedexHabitatWork *wk) {
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);
    ClActor *actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_STAMP);

    switch (PokedexHabitatSave_GetHabitatField(wk->habitat, PokedexHabitat_GetHabitatIndex(area->zone), wk->type)) {
    case AWARD_NONE:
        ClActor_SetVisible(actor, FALSE);
        break;
    case AWARD_SEEN:
        PokedexHabitatGraphic_StartActorAnim(&wk->graphic, POKEDEX_HABITAT_OBJ_STAMP, 39);
        ClActor_SetBgPriority(actor, 1);
        break;
    case AWARD_CAUGHT:
        PokedexHabitatGraphic_StartActorAnim(&wk->graphic, POKEDEX_HABITAT_OBJ_STAMP, 38);
        ClActor_SetBgPriority(actor, 1);
        break;
    }
}

static void PokedexHabitat_SeqChangeType(PokedexHabitatWork *wk) {
    u32 anims[TYPE_COUNT] = { 19, 20, 21 };
    ClActor *actor;

    switch (wk->step) {
    case 0:
        if (wk->action == ACTION_LEFT) {
            wk->type--;
            if (wk->type < 0) {
                wk->type = TYPE_COUNT - 1;
            }
            if (wk->typeShown[wk->type] == FALSE) {
                PokedexHabitat_SkipType(wk, FALSE);
            }
        } else if (wk->action == ACTION_RIGHT) {
            wk->type++;
            if (wk->type >= TYPE_COUNT) {
                wk->type = 0;
            }
            if (wk->typeShown[wk->type] == FALSE) {
                PokedexHabitat_SkipType(wk, TRUE);
            }
        }
        PokedexHabitat_UpdateList(wk);
        PokedexHabitat_UpdateTabs(wk);
        PokedexHabitat_UpdateStamp(wk);
        GFL_SndSEPlay(SEQ_SE_SELECT3);
        actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_TAB + wk->type);
        ClActor_SetAnimTime(actor, 0);
        ClActor_SetSequence(actor, anims[wk->type]);
        ClActor_SetAutoAnim(actor, TRUE);
        wk->step++;
        break;
    case 1:
        actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_TAB + wk->type);
        if (!ClActor_IsAnimActive(actor)) {
            ClActor_SetAutoAnim(actor, FALSE);
            ClActor_SetSequence(actor, anims[wk->type]);
            ClActor_SetAnimTime(actor, 0);
            wk->step++;
        }
        break;
    case 2:
        if (PokedexHabitatGraphic_AreIconsLoaded(&wk->graphic) &&
            !PokedexHabitatGraphic_IsPaletteRequested(&wk->graphic) && PokedexHabitatGraphic_IsPrinted(&wk->graphic)) {
            u32 award = PokedexHabitat_GetAward(wk);

            if (award != AWARD_NONE) {
                PokedexHabitat_SetAward(wk, award);
                PokedexHabitat_SetSequence(wk, SEQ_AWARD, ACTION_NONE);
            } else {
                PokedexHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
            }
            wk->chosenType = wk->type;
        }
        break;
    }
}

// Goes on to the next kind of habitat that the area has
static void PokedexHabitat_SkipType(PokedexHabitatWork *wk, int forward) {
    int i;

    if (forward == TRUE) {
        for (i = 0; i < TYPE_COUNT; i++) {
            wk->type++;
            if (wk->type >= TYPE_COUNT) {
                wk->type = 0;
            }
            if (wk->typeShown[wk->type] == TRUE) {
                return;
            }
        }
    } else {
        for (i = 0; i < TYPE_COUNT; i++) {
            wk->type--;
            if (wk->type < 0) {
                wk->type = TYPE_COUNT - 1;
            }
            if (wk->typeShown[wk->type] == TRUE) {
                return;
            }
        }
    }
}

static void PokedexHabitat_SeqMoveCursor(PokedexHabitatWork *wk) {
    switch (wk->step) {
    case 0: {
        int column = 0;
        int row = 0;
        int old = wk->cursor;
        int lastRow;

        if (wk->cursor < wk->speciesCount) {
            PokedexHabitat_SetCursor(wk, wk->cursor, FALSE);
        }
        if (wk->cursor != 0) {
            column = wk->cursor % POKEDEX_HABITAT_COLUMNS;
            row = wk->cursor / POKEDEX_HABITAT_COLUMNS;
        }
        lastRow = (wk->speciesCount - 1) / POKEDEX_HABITAT_COLUMNS;

        if (wk->action == ACTION_LEFT) {
            column--;
            if (column < 0) {
                column = POKEDEX_HABITAT_COLUMNS - 1;
            }
        } else if (wk->action == ACTION_RIGHT) {
            column++;
            if (column > POKEDEX_HABITAT_COLUMNS - 1) {
                column = 0;
            }
        } else if (wk->action == ACTION_UP) {
            row--;
            if (row < 0) {
                row = lastRow;
            }
        } else if (wk->action == ACTION_DOWN) {
            row++;
            if (row > lastRow) {
                row = 0;
            }
        } else if (wk->action == ACTION_NONE) {
        } else if (wk->action == ACTION_TOUCH) {
            column = wk->touched % POKEDEX_HABITAT_COLUMNS;
            row = wk->touched / POKEDEX_HABITAT_COLUMNS;
        } else if (wk->action == ACTION_TOUCH_DECIDE) {
            column = wk->touched % POKEDEX_HABITAT_COLUMNS;
            row = wk->touched / POKEDEX_HABITAT_COLUMNS;
        }

        wk->cursor = column + row * POKEDEX_HABITAT_COLUMNS;
        if (wk->cursor >= wk->speciesCount) {
            // The last row is not full
            if (wk->action == ACTION_RIGHT) {
                wk->cursor = row * POKEDEX_HABITAT_COLUMNS;
            } else if (wk->action == ACTION_LEFT) {
                wk->cursor = wk->speciesCount - 1;
            } else if (wk->action == ACTION_UP) {
                wk->cursor = column + (row - 1) * POKEDEX_HABITAT_COLUMNS;
            } else if (wk->action == ACTION_DOWN) {
                wk->cursor = column;
            } else {
                wk->cursor = wk->speciesCount - 1;
            }
        }

        if (wk->action == ACTION_TOUCH) {
            if (PokedexHabitat_GetSpecies(wk, wk->cursor) >= 0) {
                PokedexHabitatGraphic_AnimateIcon(&wk->graphic, wk->cursor, TRUE);
                if (old != wk->cursor) {
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                }
            }
            wk->cursorShown = FALSE;
        } else if (wk->action == ACTION_TOUCH_DECIDE) {
            if (PokedexHabitat_GetSpecies(wk, wk->cursor) >= 0) {
                PokedexHabitatGraphic_AnimateIcon(&wk->graphic, wk->cursor, TRUE);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
            wk->cursorShown = FALSE;
        } else {
            PokedexHabitat_SetCursor(wk, wk->cursor, TRUE);
            if (wk->action != ACTION_NONE && old != wk->cursor) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        }

        if (PokedexHabitat_GetSpecies(wk, wk->cursor) >= 0) {
            PokedexHabitat_ShowInfoButton(wk, TRUE, TRUE);
        } else {
            PokedexHabitat_ShowInfoButton(wk, FALSE, TRUE);
        }
        wk->step++;
    }
        // fallthrough
    case 1:
        PokedexHabitat_SetSequence(wk, SEQ_CURSOR, ACTION_NONE);
        break;
    }
}

// Stamps the area: with the message and the jingle for the whole area, then the stamp of the kind of habitat
static void PokedexHabitat_SeqAward(PokedexHabitatWork *wk) {
    PokedexHabitatArea *area;
    ClActor *actor;

    switch (wk->step) {
    case 0:
        if (wk->award == AWARD_AREA) {
            StrBuf *strbuf;
            StrBuf *format;

            area = PokedexHabitat_GetArea(wk);
            loadLocationNameToStrbuf(wk->wordSet, 0, ZoneData_GetPlaceNameID(PokedexHabitat_GetArea(wk)->zone));
            strbuf = GFL_StrBufCreate(0x100, HEAPID_POKEDEX_HABITAT);
            format = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexHabitat_Text_YouveRegisteredEveryPokemon);
            GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
            PokedexHabitatSave_SetCaughtMessageShown(wk->habitat);
            PokedexHabitatGraphic_CreateMessageWindow(&wk->graphic, 10, 5);
            PokedexHabitatGraphic_StartMessage(strbuf, &wk->graphic);
            GFL_StrBufFree(format);
            GFL_StrBufFree(strbuf);
            GFL_SndBGMPush();
            GFL_SndBGMPlay(SEQ_ME_ZUKAN_01, 0xffff);
            PokedexHabitatSave_SetHabitatFlag(wk->habitat, PokedexHabitat_GetHabitatIndex(area->zone));
            wk->step++;
        } else {
            wk->step = 3;
        }
        break;
    case 1:
        if (!GFL_SndBGMIsPlaying() && PokedexHabitatGraphic_GetMessageState(&wk->graphic) == PRINT_STREAM_DONE) {
            PokedexHabitatGraphic_WaitMessage(&wk->graphic);
            wk->step++;
        }
        break;
    case 2:
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed() == TRUE) {
            u32 award;
            int habitat;
            PokedexHabitatType type;

            area = PokedexHabitat_GetArea(wk);
            GFL_SndBGMPop();
            PokedexHabitatGraphic_CloseMessage(&wk->graphic);
            award = PokedexHabitat_GetAward(wk);
            habitat = PokedexHabitat_GetHabitatIndex(area->zone);
            for (type = TYPE_LAND; type < TYPE_COUNT; type++) {
                PokedexHabitatSave_SetHabitatField(wk->habitat, habitat, type, AWARD_CAUGHT);
            }
            if (award != AWARD_NONE) {
                wk->step++;
            } else {
                PokedexHabitat_UpdateNewMarks(wk);
                PokedexHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
                wk->step = 6;
            }
        }
        break;
    case 3:
        actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_STAMP);
        if (wk->award == AWARD_CAUGHT || wk->award == AWARD_AREA) {
            PokedexHabitatGraphic_StartActorAnim(&wk->graphic, POKEDEX_HABITAT_OBJ_STAMP, 26);
        } else {
            PokedexHabitatGraphic_StartActorAnim(&wk->graphic, POKEDEX_HABITAT_OBJ_STAMP, 27);
        }
        ClActor_SetBgPriority(actor, 0);
        wk->timer = 0;
        wk->step++;
        break;
    case 4:
        actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_STAMP);
        if (wk->timer == 19) {
            GFL_SndSEPlay(SEQ_SE_SW_ZUKANSTUMP_01);
        }
        wk->timer++;
        if (!ClActor_IsAnimActive(actor)) {
            ClActor_SetBgPriority(actor, 1);
            wk->step++;
        }
        break;
    case 5:
        area = PokedexHabitat_GetArea(wk);
        if (wk->award != AWARD_AREA) {
            int habitat = PokedexHabitat_GetHabitatIndex(area->zone);

            PokedexHabitatSave_SetHabitatField(wk->habitat, habitat, wk->type, wk->award);
        }
        PokedexHabitat_UpdateNewMarks(wk);
        wk->step++;
        if (wk->award == AWARD_CAUGHT && PokedexHabitatSave_IsCaughtMessageShown(wk->habitat) != TRUE) {
            PokedexHabitat_SetSequence(wk, SEQ_ALL_COMPLETE, ACTION_NONE);
        } else if (wk->award == AWARD_SEEN && PokedexHabitatSave_IsSeenMessageShown(wk->habitat) != TRUE) {
            PokedexHabitat_SetSequence(wk, SEQ_ALL_COMPLETE, ACTION_NONE);
        } else {
            PokedexHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
        }
        break;
    }
}

// The message of the first stamp of each kind
static void PokedexHabitat_SeqAllComplete(PokedexHabitatWork *wk) {
    StrBuf *strbuf;

    switch (wk->step) {
    case 0:
        if (wk->award == AWARD_CAUGHT) {
            strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexHabitat_Text_WhenAllPokemonHabitat_2);
            PokedexHabitatSave_SetCaughtMessageShown(wk->habitat);
        } else {
            strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexHabitat_Text_WhenAllPokemonHabitat);
            PokedexHabitatSave_SetSeenMessageShown(wk->habitat);
        }
        PokedexHabitatGraphic_CreateMessageWindow(&wk->graphic, 8, 9);
        PokedexHabitatGraphic_StartMessage(strbuf, &wk->graphic);
        GFL_StrBufFree(strbuf);
        wk->step++;
        break;
    case 1:
        if (PokedexHabitatGraphic_GetMessageState(&wk->graphic) == PRINT_STREAM_DONE) {
            PokedexHabitatGraphic_WaitMessage(&wk->graphic);
            wk->step++;
        }
        break;
    case 2: {
        PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);

        PokedexHabitatGraphic_CloseMessage(&wk->graphic);
        PokedexHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
        wk->step++;
        break;
    }
    }
}

// Prints what lives in the area's kind of habitat, and in which season if they differ
static void PokedexHabitat_PrintHabitat(PokedexHabitatWork *wk) {
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);
    u16 placeName = ZoneData_GetPlaceNameID(area->zone);
    u32 message;
    StrBuf *strbuf;
    StrBuf *format;

    if (wk->empty == FALSE) {
        message = PokedexHabitat_GetHabitatMessage(wk->type, area->unk8);
    } else {
        message = PokedexHabitat_Text_NoPokemonLivesHere;
    }
    if (area->values[wk->type] == 1) {
        message += wk->season + 1;
    }
    loadLocationNameToStrbuf(wk->wordSet, 0, placeName);
    strbuf = GFL_StrBufCreate(0x100, HEAPID_POKEDEX_HABITAT);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData, message);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    PokedexHabitatGraphic_Print(POKEDEX_HABITAT_WIN_MAIN, strbuf, &wk->graphic, 12, 4, 240, 32, PRINT_COLOR(1, 2, 15));
    GFL_StrBufFree(format);
    GFL_StrBufFree(strbuf);
}

static u32 PokedexHabitat_GetHabitatMessage(u32 type, u32 land) {
    u32 landMessages[] = {
        PokedexHabitat_Text_PokemonLivingTallGrass, PokedexHabitat_Text_PokemonLivingSand,
        PokedexHabitat_Text_PokemonLivingSwamp,     PokedexHabitat_Text_PokemonLivingPlace,
        PokedexHabitat_Text_PokemonLivingPlace_2,   PokedexHabitat_Text_PokemonLivingLand,
        PokedexHabitat_Text_NoPokemonLivesHere,
    };

    return PokedexHabitat_ChooseMessage(type, land, landMessages, PokedexHabitat_Text_PokemonCatchWhileSurfing,
                                        PokedexHabitat_Text_PokemonCatchByFishing);
}

static u32 PokedexHabitat_ChooseMessage(u32 type, u32 land, const u32 *landMessages, u32 water, u32 fishing) {
    u32 message = 0;

    if (type == TYPE_LAND) {
        message = landMessages[land];
    } else if (type == TYPE_WATER) {
        message = water;
    } else if (type == TYPE_FISHING) {
        message = fishing;
    }
    return message;
}

static void PokedexHabitat_PrintInfoLabel(PokedexHabitatWork *wk, u16 color) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexHabitat_Text_Info);

    PokedexHabitatGraphic_Print(POKEDEX_HABITAT_WIN_MAIN, strbuf, &wk->graphic, 124, 172, 96, 16, color);
    GFL_StrBufFree(strbuf);
}

static void PokedexHabitat_ClearInfoLabel(PokedexHabitatWork *wk) {
    PokedexHabitatGraphic_ClearArea(POKEDEX_HABITAT_WIN_MAIN, &wk->graphic, 124, 172, 96, 16);
}

static void PokedexHabitat_PrintAreaName(PokedexHabitatWork *wk) {
    StrBuf *strbuf;
    StrBuf *format;

    loadLocationNameToStrbuf(wk->wordSet, 0, ZoneData_GetPlaceNameID(PokedexHabitat_GetArea(wk)->zone));
    strbuf = GFL_StrBufCreate(0x100, HEAPID_POKEDEX_HABITAT);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexHabitat_Text_Empty_6);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    PokedexHabitatGraphic_Print(POKEDEX_HABITAT_WIN_SUB, strbuf, &wk->graphic, 8, 4, 256, 16, PRINT_COLOR(14, 15, 0));
    GFL_StrBufFree(format);
    GFL_StrBufFree(strbuf);
}

static PokedexHabitatArea *PokedexHabitat_GetArea(PokedexHabitatWork *wk) {
    return &wk->param->areas[wk->area];
}

// Lists the Pokémon of the area's kind of habitat and season that the player has seen, and asks for their icons
static void PokedexHabitat_LoadSpecies(PokedexHabitatWork *wk) {
    u8 count = 0;
    int i;
    u8 unseen = 0;
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);
    PokedexHabitatSpecies *list;

    MI_CpuFill8(wk->species, 0, sizeof(wk->species));
    list = area->species;
    for (i = 0; i < POKEDEX_HABITAT_ICON_COUNT; i++) {
        PokedexHabitatSpecies *species = &list[i];

        if (species->status != STATUS_NONE && species->present[wk->season][wk->type] != 0) {
            if (species->status >= STATUS_SEEN) {
                u32 sex;
                u32 form;
                u32 rare;

                PokeDex_GetShownSexRareForm(wk->pokedex, species->species, &sex, &rare, &form, HEAPID_POKEDEX_HABITAT);
                if (PokeDex_HasSeenVariant(wk->pokedex, species->species, sex, rare,
                                           species->other[wk->season][wk->type]) == TRUE) {
                    form = species->other[wk->season][wk->type];
                }
                PokedexHabitatGraphic_RequestIcon(&wk->graphic, species->species, form, sex, count);
                PokedexHabitatGraphic_SetCellPalette(&wk->graphic, count, POKEDEX_HABITAT_CELL_FILLED);
                wk->species[count] = species->species;
                if (species->status == STATUS_SEEN) {
                    PokedexHabitatGraphic_SetMark(&wk->graphic, count, POKEDEX_HABITAT_MARK_SEEN);
                } else if (species->status == STATUS_CAUGHT) {
                    PokedexHabitatGraphic_SetMark(&wk->graphic, count, POKEDEX_HABITAT_MARK_CAUGHT);
                }
                count++;
            } else {
                unseen++;
            }
        }
    }
    if (unseen == 0 && count == 0) {
        wk->empty = TRUE;
    } else {
        wk->empty = FALSE;
    }
    for (i = count; i < POKEDEX_HABITAT_ICON_COUNT; i++) {
        PokedexHabitatGraphic_SetIconVisible(&wk->graphic, i, FALSE);
        PokedexHabitatGraphic_SetCellPalette(&wk->graphic, i, POKEDEX_HABITAT_CELL_EMPTY);
        PokedexHabitatGraphic_SetMark(&wk->graphic, i, POKEDEX_HABITAT_MARK_NONE);
    }
    wk->speciesCount = count;
}

static void PokedexHabitat_UpdateTabs(PokedexHabitatWork *wk) {
    if (wk->type == TYPE_LAND) {
        PokedexHabitatGraphic_SetTab(&wk->graphic, TYPE_LAND, POKEDEX_HABITAT_TAB_CHOSEN);
    } else if (wk->typeShown[TYPE_LAND] == TRUE) {
        PokedexHabitatGraphic_SetTab(&wk->graphic, TYPE_LAND, POKEDEX_HABITAT_TAB_SHOWN);
    } else {
        PokedexHabitatGraphic_SetTab(&wk->graphic, TYPE_LAND, POKEDEX_HABITAT_TAB_HIDDEN);
    }
    if (wk->type == TYPE_WATER) {
        PokedexHabitatGraphic_SetTab(&wk->graphic, TYPE_WATER, POKEDEX_HABITAT_TAB_CHOSEN);
    } else if (wk->typeShown[TYPE_WATER] == TRUE) {
        PokedexHabitatGraphic_SetTab(&wk->graphic, TYPE_WATER, POKEDEX_HABITAT_TAB_SHOWN);
    } else {
        PokedexHabitatGraphic_SetTab(&wk->graphic, TYPE_WATER, POKEDEX_HABITAT_TAB_HIDDEN);
    }
    if (wk->type == TYPE_FISHING) {
        PokedexHabitatGraphic_SetTab(&wk->graphic, TYPE_FISHING, POKEDEX_HABITAT_TAB_CHOSEN);
    } else if (wk->typeShown[TYPE_FISHING] == TRUE) {
        PokedexHabitatGraphic_SetTab(&wk->graphic, TYPE_FISHING, POKEDEX_HABITAT_TAB_SHOWN);
    } else {
        PokedexHabitatGraphic_SetTab(&wk->graphic, TYPE_FISHING, POKEDEX_HABITAT_TAB_HIDDEN);
    }
    PokedexHabitatGraphic_RequestPalette(&wk->graphic, wk->type);
    PokedexHabitat_UpdateNewMarks(wk);
    PokedexHabitat_PrintHabitat(wk);
}

static void PokedexHabitat_UpdateNewMarks(PokedexHabitatWork *wk) {
    PokedexHabitat_UpdateNewMark(wk, TYPE_LAND, PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_NEW));
    PokedexHabitat_UpdateNewMark(wk, TYPE_WATER,
                                 PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_NEW + 1));
    PokedexHabitat_UpdateNewMark(wk, TYPE_FISHING,
                                 PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_NEW + 2));
}

// Marks the tab of a kind of habitat that has a stamp to give
static void PokedexHabitat_UpdateNewMark(PokedexHabitatWork *wk, u32 type, ClActor *actor) {
    if (wk->typeShown[type] == TRUE) {
        if (PokedexHabitat_GetTypeAward(wk, type) != AWARD_NONE) {
            ClActor_SetVisible(actor, TRUE);
        } else {
            ClActor_SetVisible(actor, FALSE);
        }
    } else {
        ClActor_SetVisible(actor, FALSE);
    }
}

static void PokedexHabitat_UpdateList(PokedexHabitatWork *wk) {
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);

    if (wk->typeShown[wk->type] == FALSE) {
        PokedexHabitat_SkipType(wk, TRUE);
    }
    PokedexHabitat_LoadSpecies(wk);
    // The seasons' arrows show only where the seasons differ
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_LEFT),
                       area->values[wk->type]);
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_RIGHT),
                       area->values[wk->type]);
}

// Takes the cursor off its cell, and shows or hides it on another
static void PokedexHabitat_SetCursor(PokedexHabitatWork *wk, int cell, BOOL shown) {
    if (shown == TRUE) {
        PokedexHabitatGraphic_SetCellPalette(&wk->graphic, wk->cursor, POKEDEX_HABITAT_CELL_CURSOR);
    } else {
        PokedexHabitatGraphic_SetCellPalette(&wk->graphic, wk->cursor, POKEDEX_HABITAT_CELL_FILLED);
    }
    if (PokedexHabitat_GetSpecies(wk, cell) >= 0) {
        PokedexHabitatGraphic_AnimateIcon(&wk->graphic, cell, shown);
    }
    wk->cursorShown = shown;
}

// Finds which kinds of habitat the area has, and how many of each one's Pokémon the player has
static void PokedexHabitat_CheckTypes(PokedexHabitatWork *wk) {
    int type;
    int i;
    int season;
    PokedexHabitatSpecies *species = PokedexHabitat_GetArea(wk)->species;

    MI_CpuFill8(wk->typeShown, 0, sizeof(wk->typeShown));
    for (type = 0; type < TYPE_COUNT; type++) {
        BOOL seen = TRUE;
        BOOL caught = TRUE;

        for (i = 0; i < POKEDEX_HABITAT_ICON_COUNT; i++) {
            if (species[i].status >= STATUS_UNSEEN) {
                for (season = 0; season < SEASON_COUNT; season++) {
                    if (species[i].present[season][type] == 1) {
                        wk->typeShown[type] = TRUE;
                        if (species[i].status == STATUS_UNSEEN) {
                            seen = FALSE;
                            caught = FALSE;
                        } else if (species[i].status == STATUS_SEEN) {
                            caught = FALSE;
                        }
                    }
                }
            }
        }
        if (caught == TRUE) {
            wk->typeState[type] = AWARD_CAUGHT;
        } else if (seen == TRUE) {
            wk->typeState[type] = AWARD_SEEN;
        } else {
            wk->typeState[type] = AWARD_NONE;
        }
    }
    if (wk->areaCount <= 1) {
        ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_AREA_UP), FALSE);
        ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_AREA_DOWN), FALSE);
    }
}

static void PokedexHabitat_HideListButtons(PokedexHabitatWork *wk) {
    ClActor *actor;

    actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_TAB + TYPE_LAND);
    ClActor_SetVisible(actor, FALSE);
    actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_TAB + TYPE_WATER);
    ClActor_SetVisible(actor, FALSE);
    actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_TAB + TYPE_FISHING);
    ClActor_SetVisible(actor, FALSE);
    actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_AREA_UP);
    ClActor_SetVisible(actor, FALSE);
    actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_AREA_DOWN);
    ClActor_SetVisible(actor, FALSE);
    actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SHORTCUT);
    ClActor_SetVisible(actor, FALSE);
    actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_EXIT);
    ClActor_SetVisible(actor, FALSE);
    PokedexHabitatGraphic_SetNewMarksVisible(&wk->graphic, FALSE);
    actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_INFO);
    ClActor_SetVisible(actor, TRUE);
    actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_LEFT);
    ClActor_SetVisible(actor, FALSE);
    actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_RIGHT);
    ClActor_SetVisible(actor, FALSE);
}

static void PokedexHabitat_ShowListButtons(PokedexHabitatWork *wk) {
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);

    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_TAB + TYPE_LAND),
                       wk->typeShown[TYPE_LAND]);
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_TAB + TYPE_WATER),
                       wk->typeShown[TYPE_WATER]);
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_TAB + TYPE_FISHING),
                       wk->typeShown[TYPE_FISHING]);
    if (wk->areaCount > 1) {
        ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_AREA_UP), TRUE);
        ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_AREA_DOWN), TRUE);
    } else {
        ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_AREA_UP), FALSE);
        ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_AREA_DOWN), FALSE);
    }
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SHORTCUT), TRUE);
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_EXIT), TRUE);
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_LEFT),
                       area->values[wk->type]);
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_SEASON_RIGHT),
                       area->values[wk->type]);
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_INFO), FALSE);
    PokedexHabitat_UpdateNewMarks(wk);
    PokedexHabitat_ClearInfoLabel(wk);
}

static void PokedexHabitat_UpdateMap(PokedexHabitatWork *wk) {
    PokedexHabitatGraphic_SetArea(&wk->graphic, PokedexHabitat_GetArea(wk)->zone);
    PokedexHabitatGraphic_UpdatePlayerMarker(&wk->graphic);
    PokedexHabitat_PrintAreaName(wk);
}

static void PokedexHabitat_SetResult(PokedexHabitatWork *wk, u32 result) {
    wk->param->result = result;
}

// Starts on the area, the season and the kind of habitat that the save has
static void PokedexHabitat_LoadSave(PokedexHabitatWork *wk) {
    int i;
    GameData *gameData = wk->param->gameData;
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);
    PokedexHabitatSave *habitat = SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(gameData));
    u16 zone = PokedexHabitatSave_GetZone(habitat);

    wk->pokedex = pokedex;
    wk->habitat = habitat;
    wk->areaCount = wk->param->count;
    wk->area = 0;
    for (i = 0; i < wk->areaCount; i++) {
        if (zone == wk->param->areas[i].zone) {
            wk->area = i;
        }
    }
    wk->chosenType = PokedexHabitatSave_GetType(wk->habitat);
    wk->season = PokedexHabitatSave_GetSeason(wk->habitat);
    wk->type = wk->chosenType;
}

// The species in a cell, or -1
static int PokedexHabitat_GetSpecies(PokedexHabitatWork *wk, u8 cell) {
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);
    int species = wk->species[cell];

    if (species == 0) {
        species = -1;
    }
    return species;
}

// The stamp that the area has yet to be given, an AWARD_*
static u32 PokedexHabitat_GetAward(PokedexHabitatWork *wk) {
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);

    if (area->complete == TRUE &&
        PokedexHabitatSave_IsHabitatFlagSet(wk->habitat, PokedexHabitat_GetHabitatIndex(area->zone)) == FALSE) {
        return AWARD_AREA;
    }
    return PokedexHabitat_GetTypeAward(wk, wk->type);
}

static u32 PokedexHabitat_GetTypeAward(PokedexHabitatWork *wk, u32 type) {
    PokedexHabitatArea *area = PokedexHabitat_GetArea(wk);
    int done = PokedexHabitatSave_GetHabitatField(wk->habitat, PokedexHabitat_GetHabitatIndex(area->zone), type);
    u32 *state = area->states;

    state += type;
    if (done < (int)*state) {
        return *state;
    }
    return AWARD_NONE;
}

static void PokedexHabitat_SetAward(PokedexHabitatWork *wk, u32 award) {
    wk->award = award;
}

static int PokedexHabitat_GetHabitatIndex(u16 zone) {
    int i;

    // POKEDEX_HABITAT_COUNT areas; save/pokedex_habitat_savedata.h defines that name again, as the save's 70 slots
    for (i = 0; i < (int)NELEMS(sPokedexHabitatAreas); i++) {
        if (zone == sPokedexHabitatAreas[i].zone) {
            return i;
        }
    }
    return 0;
}

static void PokedexHabitat_ToggleShortcut(PokedexHabitatWork *wk) {
    GameData *gameData = wk->param->gameData;

    if (GameData_IsShortcutRegistered(gameData, SHORTCUT_POKEDEX_HABITAT) == TRUE) {
        GameData_SetKeyItemRegistration(gameData, SHORTCUT_POKEDEX_HABITAT, FALSE);
        PokedexHabitatGraphic_StartActorAnim(&wk->graphic, POKEDEX_HABITAT_OBJ_SHORTCUT, 6);
    } else {
        GameData_SetKeyItemRegistration(gameData, SHORTCUT_POKEDEX_HABITAT, TRUE);
        PokedexHabitatGraphic_StartActorAnim(&wk->graphic, POKEDEX_HABITAT_OBJ_SHORTCUT, 7);
    }
}

static void PokedexHabitat_UpdateShortcut(PokedexHabitatWork *wk) {
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_HABITAT) == TRUE) {
        PokedexHabitatGraphic_StartActorAnim(&wk->graphic, POKEDEX_HABITAT_OBJ_SHORTCUT, 7);
    } else {
        PokedexHabitatGraphic_StartActorAnim(&wk->graphic, POKEDEX_HABITAT_OBJ_SHORTCUT, 6);
    }
}

static BOOL PokedexHabitat_HasSeveralTypes(PokedexHabitatWork *wk) {
    int count = 0;
    int i;

    for (i = 0; i < TYPE_COUNT; i++) {
        if (wk->typeShown[i] == TRUE) {
            count++;
        }
    }
    return count > 1 ? TRUE : FALSE;
}

// Shows the button that opens the Pokédex on the cursor's Pokémon, or hides it
static void PokedexHabitat_ShowInfoButton(PokedexHabitatWork *wk, BOOL known, BOOL shown) {
    ClActor *actor = PokedexHabitatGraphic_GetActor(&wk->graphic, POKEDEX_HABITAT_OBJ_INFO);

    if (shown == FALSE) {
        PokedexHabitat_ClearInfoLabel(wk);
        ClActor_SetVisible(actor, FALSE);
    } else {
        ClActor_SetVisible(actor, TRUE);
        if (known == TRUE) {
            PokedexHabitat_PrintInfoLabel(wk, PRINT_COLOR(15, 1, 2));
            PokedexHabitatGraphic_StartActorAnim(&wk->graphic, POKEDEX_HABITAT_OBJ_INFO, 28);
        } else {
            PokedexHabitat_PrintInfoLabel(wk, PRINT_COLOR(2, 1, 0));
            PokedexHabitatGraphic_StartActorAnim(&wk->graphic, POKEDEX_HABITAT_OBJ_INFO, 41);
        }
    }
}

// Defined after the functions: the ROM has it ahead of PokedexHabitat_SeqChangeType's table of the same size
const GameProcFunctions PokedexHabitat_ProcFunctions = {
    PokedexHabitat_ProcInit,
    PokedexHabitat_ProcMain,
    PokedexHabitat_ProcEnd,
};
