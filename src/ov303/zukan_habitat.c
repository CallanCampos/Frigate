#include "types.h"
#include "app/zukan_common.h"
#include "app/zukan_habitat.h"
#include "app/zukan_habitat_graphic.h"
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
#include "save/pokedex.h"
#include "save/zukan_habitat_savedata.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/0362.h"

// The Pokédex's Habitat List: its process, its sequences and what they show. The ROM has no name for this file;
// zukan_habitat.c is a guess after zukan.c, which calls the screen the habitat list.
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

#define SEASON_COUNT ZUKAN_SEASON_COUNT

// A habitat's kinds
typedef enum {
    TYPE_LAND,
    TYPE_WATER,
    TYPE_FISHING,
    TYPE_COUNT,
} ZukanHabitatType;

// A species' status in ZukanHabitatSpecies
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

// An area of the habitat list: the flag that shows it, its zone and its file
typedef struct {
    u16 flag;
    u16 zone;
    u8 file;
    u8 padding;
} ZukanHabitatTableEntry;

typedef struct {
    BOOL finished;
    u32 seq;
    u32 step;
    ZukanHabitatListParam *param;
    ZukanHabitatGraphic graphic;
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
    u32 species[ZUKAN_HABITAT_ICON_COUNT];
    // An ACTION_*
    u32 action;
    PokeDexSave *pokedex;
    ZukanHabitatSave *habitat;
    int slideMain;
    int slideSub;
    // The kind of habitat that the player last chose
    u32 chosenType;
    u32 timer;
    // Whether the kind of habitat has no Pokémon in this season
    BOOL empty;
} ZukanHabitatWork; // size: 0x94c

typedef void (*ZukanHabitatSeqFunc)(ZukanHabitatWork *wk);

static BOOL ZukanHabitat_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanHabitat_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanHabitat_ProcEnd(GameProc *proc, u32 *state, void *param, void *work);
static void ZukanHabitat_AddVBlank(ZukanHabitatWork *wk);
static void ZukanHabitat_RemoveVBlank(ZukanHabitatWork *wk);
static void ZukanHabitat_VBlank(TCB *tcb, void *data);
static ZukanHabitatWork *ZukanHabitat_CreateWork(GameProc *proc, ZukanHabitatListParam *param);
static void ZukanHabitat_FreeWork(GameProc *proc, ZukanHabitatWork *wk);
static BOOL ZukanHabitat_RunSequence(ZukanHabitatWork *wk);
static void ZukanHabitat_SetSequence(ZukanHabitatWork *wk, u32 seq, u32 action);
static void ZukanHabitat_SeqFadeIn(ZukanHabitatWork *wk);
static void ZukanHabitat_SeqFadeOut(ZukanHabitatWork *wk);
static void ZukanHabitat_SeqList(ZukanHabitatWork *wk);
static void ZukanHabitat_SelectType(ZukanHabitatWork *wk, int type);
static void ZukanHabitat_ListKeys(ZukanHabitatWork *wk);
static void ZukanHabitat_SeqLeaveCursor(ZukanHabitatWork *wk);
static void ZukanHabitat_SeqCursor(ZukanHabitatWork *wk);
static void ZukanHabitat_CursorKeys(ZukanHabitatWork *wk);
static void ZukanHabitat_SeqInit(ZukanHabitatWork *wk);
static void ZukanHabitat_SeqButton(ZukanHabitatWork *wk);
static void ZukanHabitat_SeqChangeSeason(ZukanHabitatWork *wk);
static void ZukanHabitat_SeqChangeArea(ZukanHabitatWork *wk);
static void ZukanHabitat_UpdateStamp(ZukanHabitatWork *wk);
static void ZukanHabitat_SeqChangeType(ZukanHabitatWork *wk);
static void ZukanHabitat_SkipType(ZukanHabitatWork *wk, int forward);
static void ZukanHabitat_SeqMoveCursor(ZukanHabitatWork *wk);
static void ZukanHabitat_SeqAward(ZukanHabitatWork *wk);
static void ZukanHabitat_SeqAllComplete(ZukanHabitatWork *wk);
static void ZukanHabitat_PrintHabitat(ZukanHabitatWork *wk);
static u32 ZukanHabitat_GetHabitatMessage(u32 type, u32 land);
static u32 ZukanHabitat_ChooseMessage(u32 type, u32 land, const u32 *landMessages, u32 water, u32 fishing);
static void ZukanHabitat_PrintInfoLabel(ZukanHabitatWork *wk, u16 color);
static void ZukanHabitat_ClearInfoLabel(ZukanHabitatWork *wk);
static void ZukanHabitat_PrintAreaName(ZukanHabitatWork *wk);
static ZukanHabitatArea *ZukanHabitat_GetArea(ZukanHabitatWork *wk);
static void ZukanHabitat_LoadSpecies(ZukanHabitatWork *wk);
static void ZukanHabitat_UpdateTabs(ZukanHabitatWork *wk);
static void ZukanHabitat_UpdateNewMarks(ZukanHabitatWork *wk);
static void ZukanHabitat_UpdateNewMark(ZukanHabitatWork *wk, u32 type, ClActor *actor);
static void ZukanHabitat_UpdateList(ZukanHabitatWork *wk);
static void ZukanHabitat_SetCursor(ZukanHabitatWork *wk, int cell, BOOL shown);
static void ZukanHabitat_CheckTypes(ZukanHabitatWork *wk);
static void ZukanHabitat_HideListButtons(ZukanHabitatWork *wk);
static void ZukanHabitat_ShowListButtons(ZukanHabitatWork *wk);
static void ZukanHabitat_UpdateMap(ZukanHabitatWork *wk);
static void ZukanHabitat_SetResult(ZukanHabitatWork *wk, u32 result);
static void ZukanHabitat_LoadSave(ZukanHabitatWork *wk);
static int ZukanHabitat_GetSpecies(ZukanHabitatWork *wk, u8 cell);
static u32 ZukanHabitat_GetAward(ZukanHabitatWork *wk);
static u32 ZukanHabitat_GetTypeAward(ZukanHabitatWork *wk, u32 type);
static void ZukanHabitat_SetAward(ZukanHabitatWork *wk, u32 award);
static int ZukanHabitat_GetHabitatIndex(u16 zone);
static void ZukanHabitat_ToggleShortcut(ZukanHabitatWork *wk);
static void ZukanHabitat_UpdateShortcut(ZukanHabitatWork *wk);
static BOOL ZukanHabitat_HasSeveralTypes(ZukanHabitatWork *wk);
static void ZukanHabitat_ShowInfoButton(ZukanHabitatWork *wk, BOOL known, BOOL shown);

static const ZukanHabitatSeqFunc sZukanHabitatSequences[SEQ_COUNT + 1] = {
    ZukanHabitat_SeqInit,         ZukanHabitat_SeqFadeIn,     ZukanHabitat_SeqFadeOut,     ZukanHabitat_SeqList,
    ZukanHabitat_SeqLeaveCursor,  ZukanHabitat_SeqChangeSeason, ZukanHabitat_SeqChangeArea, ZukanHabitat_SeqChangeType,
    ZukanHabitat_SeqCursor,       ZukanHabitat_SeqMoveCursor, ZukanHabitat_SeqAward,       ZukanHabitat_SeqButton,
    ZukanHabitat_SeqAllComplete,  NULL,
};

// BUG: neither table ends with TOUCH_RECT_END, so the search runs on into the data that follows them
static const TouchRect sZukanHabitatCursorTouchRects[CURSOR_TOUCH_COUNT] = {
    { 168, 191, 232, 255 }, { 168, 191, 96, 212 },
    { 48, 80, 32, 72 },     { 48, 80, 72, 112 },    { 48, 80, 112, 152 },   { 48, 80, 152, 192 },   { 48, 80, 192, 232 },
    { 80, 112, 32, 72 },    { 80, 112, 72, 112 },   { 80, 112, 112, 152 },  { 80, 112, 152, 192 },  { 80, 112, 192, 232 },
    { 112, 144, 32, 72 },   { 112, 144, 72, 112 },  { 112, 144, 112, 152 }, { 112, 144, 152, 192 }, { 112, 144, 192, 232 },
    { 144, 176, 32, 72 },   { 144, 176, 72, 112 },  { 144, 176, 112, 152 }, { 144, 176, 152, 192 }, { 144, 176, 192, 232 },
};

static const TouchRect sZukanHabitatListTouchRects[LIST_TOUCH_COUNT] = {
    { 168, 191, 184, 207 }, { 168, 191, 208, 231 }, { 168, 191, 232, 255 }, { 168, 191, 68, 92 },   { 168, 191, 108, 132 },
    { 168, 191, 148, 172 }, { 168, 191, 0, 24 },    { 168, 191, 24, 48 },   { 52, 76, 12, 36 },     { 52, 76, 228, 244 },
    { 48, 80, 32, 72 },     { 48, 80, 72, 112 },    { 48, 80, 112, 152 },   { 48, 80, 152, 192 },   { 48, 80, 192, 232 },
    { 80, 112, 32, 72 },    { 80, 112, 72, 112 },   { 80, 112, 112, 152 },  { 80, 112, 152, 192 },  { 80, 112, 192, 232 },
    { 112, 144, 32, 72 },   { 112, 144, 72, 112 },  { 112, 144, 112, 152 }, { 112, 144, 152, 192 }, { 112, 144, 192, 232 },
    { 144, 176, 32, 72 },   { 144, 176, 72, 112 },  { 144, 176, 112, 152 }, { 144, 176, 152, 192 }, { 144, 176, 192, 232 },
};

// The areas of the habitat list, as in zukan_common.c. An area's place in it is its habitat in the save
static const ZukanHabitatTableEntry sZukanHabitatAreas[] = {
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

static BOOL ZukanHabitat_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanHabitatWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_ZUKAN_HABITAT, 0x60000);
    wk = ZukanHabitat_CreateWork(proc, param);
    ZukanHabitatGraphic_Init(&wk->graphic, wk->param->gameData);
    ZukanHabitat_AddVBlank(wk);
    return TRUE;
}

static BOOL ZukanHabitat_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanHabitatWork *wk = work;

    if (ZukanHabitat_RunSequence(wk)) {
        ZukanHabitatGraphic_Main(&wk->graphic);
        return FALSE;
    }
    return TRUE;
}

static BOOL ZukanHabitat_ProcEnd(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanHabitatWork *wk = work;

    ZukanHabitat_RemoveVBlank(wk);
    ZukanHabitatGraphic_Exit(&wk->graphic);
    ZukanHabitat_FreeWork(proc, wk);
    GFL_HeapDelete(HEAPID_ZUKAN_HABITAT);
    return TRUE;
}

static void ZukanHabitat_AddVBlank(ZukanHabitatWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(ZukanHabitat_VBlank, wk, 0);
}

static void ZukanHabitat_RemoveVBlank(ZukanHabitatWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static void ZukanHabitat_VBlank(TCB *tcb, void *data) {
    ZukanHabitatWork *wk = data;

    ZukanHabitatGraphic_VBlank(&wk->graphic);
}

static ZukanHabitatWork *ZukanHabitat_CreateWork(GameProc *proc, ZukanHabitatListParam *param) {
    ZukanHabitatWork *wk = GFL_ProcInitSubsystem(proc, sizeof(ZukanHabitatWork), HEAPID_ZUKAN_HABITAT);

    sys_memset(wk, 0, sizeof(ZukanHabitatWork));
    wk->param = param;
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0362, HEAPID_ZUKAN_HABITAT);
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_ZUKAN_HABITAT);
    return wk;
}

static void ZukanHabitat_FreeWork(GameProc *proc, ZukanHabitatWork *wk) {
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_MsgDataFree(wk->msgData);
    GFL_ProcReleaseSubsystem(proc);
}

// Returns whether the screen goes on
static BOOL ZukanHabitat_RunSequence(ZukanHabitatWork *wk) {
    if (sZukanHabitatSequences[wk->seq] != NULL) {
        sZukanHabitatSequences[wk->seq](wk);
    }
    if (wk->finished) {
        return FALSE;
    }
    return TRUE;
}

static void ZukanHabitat_SetSequence(ZukanHabitatWork *wk, u32 seq, u32 action) {
    wk->seq = seq;
    wk->action = action;
    wk->step = 0;
}

static void ZukanHabitat_SeqFadeIn(ZukanHabitatWork *wk) {
    switch (wk->step) {
    case 0:
        if (ZukanHabitatGraphic_IsPrinted(&wk->graphic)) {
            wk->slideMain = SLIDE_DISTANCE;
            wk->slideSub = -SLIDE_DISTANCE;
            ZukanHabitatGraphic_StartFade(&wk->graphic, 0, 0);
            ZukanHabitatGraphic_Scroll(&wk->graphic, wk->slideMain, TRUE);
            ZukanHabitatGraphic_Scroll(&wk->graphic, wk->slideSub, FALSE);
            wk->step++;
        }
        break;
    case 1:
        if (!ZukanHabitatGraphic_IsFading(&wk->graphic)) {
            GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, 0);
            GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, 0);
            wk->step++;
        }
        break;
    case 2:
        if (!ZukanHabitatGraphic_IsFading(&wk->graphic)) {
            GFL_BGSysSetBGEnabled(6, TRUE);
            GFL_BGSysSetBGEnabled(7, TRUE);
        }
        if (wk->slideMain <= 0) {
            u32 award = ZukanHabitat_GetAward(wk);

            if (award != AWARD_NONE) {
                ZukanHabitat_SetAward(wk, award);
                ZukanHabitat_SetSequence(wk, SEQ_AWARD, ACTION_NONE);
            } else {
                ZukanHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
            }
        } else {
            ZukanHabitatGraphic_Scroll(&wk->graphic, -SLIDE_SPEED, TRUE);
            ZukanHabitatGraphic_Scroll(&wk->graphic, SLIDE_SPEED, FALSE);
            wk->slideMain -= SLIDE_SPEED;
            wk->slideSub += SLIDE_SPEED;
        }
        break;
    }
}

static void ZukanHabitat_SeqFadeOut(ZukanHabitatWork *wk) {
    switch (wk->step) {
    case 0:
        if (ZukanHabitatGraphic_IsPrinted(&wk->graphic)) {
            u16 backdrop;

            ZukanHabitatGraphic_SetBlinkStopped(&wk->graphic, TRUE);
            wk->slideMain = 0;
            wk->slideSub = 0;
            ZukanHabitatGraphic_StartFade(&wk->graphic, 16, 16);
            ZukanHabitat_ClearInfoLabel(wk);
            func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_INFO), FALSE);
            ZukanHabitatGraphic_SetNewMarksVisible(&wk->graphic, FALSE);
            backdrop = 0;
            gfxUploadStdPaletteBGB(&backdrop, 0, sizeof(backdrop));
            GFL_BGSysSetBGEnabled(6, FALSE);
            GFL_BGSysSetBGEnabled(7, FALSE);
            wk->step++;
        }
        break;
    case 1:
        if (!ZukanHabitatGraphic_IsFading(&wk->graphic)) {
            if (wk->slideMain >= SLIDE_DISTANCE) {
                GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
                GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
                wk->finished = TRUE;
            } else {
                ZukanHabitatGraphic_Scroll(&wk->graphic, SLIDE_SPEED, TRUE);
                ZukanHabitatGraphic_Scroll(&wk->graphic, -SLIDE_SPEED, FALSE);
                wk->slideMain += SLIDE_SPEED;
                wk->slideSub -= SLIDE_SPEED;
            }
        }
        break;
    }
}

static void ZukanHabitat_SeqList(ZukanHabitatWork *wk) {
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);
    int hit = func_0203da0c(sZukanHabitatListTouchRects);

    switch (hit) {
    case LIST_TOUCH_SHORTCUT:
        GFL_SndSEPlay(SEQ_SE_SYS_07);
        ZukanHabitat_ToggleShortcut(wk);
        break;
    case LIST_TOUCH_EXIT:
        ZukanHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_EXIT);
        break;
    case LIST_TOUCH_RETURN:
        ZukanHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_RETURN);
        break;
    case LIST_TOUCH_TAB + TYPE_LAND:
        ZukanHabitat_SelectType(wk, TYPE_LAND);
        break;
    case LIST_TOUCH_TAB + TYPE_WATER:
        ZukanHabitat_SelectType(wk, TYPE_WATER);
        break;
    case LIST_TOUCH_TAB + TYPE_FISHING:
        ZukanHabitat_SelectType(wk, TYPE_FISHING);
        break;
    case LIST_TOUCH_AREA_UP:
        if (wk->areaCount > 1) {
            ZukanHabitat_SetSequence(wk, SEQ_CHANGE_AREA, ACTION_UP);
        }
        break;
    case LIST_TOUCH_AREA_DOWN:
        if (wk->areaCount > 1) {
            ZukanHabitat_SetSequence(wk, SEQ_CHANGE_AREA, ACTION_DOWN);
        }
        break;
    case LIST_TOUCH_SEASON_LEFT:
        if (area->values[wk->type] == 1) {
            ZukanHabitat_SetSequence(wk, SEQ_CHANGE_SEASON, ACTION_LEFT);
        }
        break;
    case LIST_TOUCH_SEASON_RIGHT:
        if (area->values[wk->type] == 1) {
            ZukanHabitat_SetSequence(wk, SEQ_CHANGE_SEASON, ACTION_RIGHT);
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

            if (cell < wk->speciesCount && ZukanHabitat_GetSpecies(wk, cell) >= 0) {
                wk->touched = cell;
                ZukanHabitat_SetSequence(wk, SEQ_CURSOR, ACTION_TOUCH);
            }
        }
        break;
    case TOUCH_RECT_NONE:
        ZukanHabitat_ListKeys(wk);
        break;
    }
}

static void ZukanHabitat_SelectType(ZukanHabitatWork *wk, int type) {
    if (wk->type != type && ZukanHabitat_HasSeveralTypes(wk) && wk->typeShown[type] == TRUE) {
        wk->type = type;
        ZukanHabitat_SetSequence(wk, SEQ_CHANGE_TYPE, ACTION_TAB);
    }
}

static void ZukanHabitat_ListKeys(ZukanHabitatWork *wk) {
    u32 repeat = GCTX_HIDGetTypedKeys();
    u32 pressed = GCTX_HIDGetPressedKeys();
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);

    if (repeat & PAD_BUTTON_L) {
        if (area->values[wk->type] == 1) {
            ZukanHabitat_SetSequence(wk, SEQ_CHANGE_SEASON, ACTION_LEFT);
        }
    } else if (repeat & PAD_BUTTON_R) {
        if (area->values[wk->type] == 1) {
            ZukanHabitat_SetSequence(wk, SEQ_CHANGE_SEASON, ACTION_RIGHT);
        }
    } else if (repeat & PAD_KEY_LEFT) {
        if (ZukanHabitat_HasSeveralTypes(wk)) {
            ZukanHabitat_SetSequence(wk, SEQ_CHANGE_TYPE, ACTION_LEFT);
        }
    } else if (repeat & PAD_KEY_RIGHT) {
        if (ZukanHabitat_HasSeveralTypes(wk)) {
            ZukanHabitat_SetSequence(wk, SEQ_CHANGE_TYPE, ACTION_RIGHT);
        }
    } else if (repeat & PAD_KEY_UP) {
        if (wk->areaCount > 1) {
            ZukanHabitat_SetSequence(wk, SEQ_CHANGE_AREA, ACTION_UP);
        }
    } else if (repeat & PAD_KEY_DOWN) {
        if (wk->areaCount > 1) {
            ZukanHabitat_SetSequence(wk, SEQ_CHANGE_AREA, ACTION_DOWN);
        }
    } else if (pressed & PAD_BUTTON_A) {
        if (wk->speciesCount > 0) {
            int cell = 0;

            if (ZukanHabitat_GetSpecies(wk, 0) >= 0) {
                wk->cursor = cell;
                ZukanHabitat_SetSequence(wk, SEQ_CURSOR, ACTION_DECIDE);
            }
        }
    } else if (pressed & PAD_BUTTON_B) {
        ZukanHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_RETURN);
    } else if (pressed & PAD_BUTTON_X) {
        ZukanHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_EXIT);
    } else if (pressed & PAD_BUTTON_Y) {
        GFL_SndSEPlay(SEQ_SE_SYS_07);
        ZukanHabitat_ToggleShortcut(wk);
    }
}

// Back from the cursor to the list
static void ZukanHabitat_SeqLeaveCursor(ZukanHabitatWork *wk) {
    ClActor *actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_RETURN);

    switch (wk->step) {
    case 0:
        func_0204c4d4(actor, 0);
        func_0204c488(actor, 9);
        func_0204c520(actor, TRUE);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        wk->step++;
        break;
    case 1:
        if (!GFL_SndPlayerIsActiveAny() && !func_0204c560(actor)) {
            ZukanHabitat_SetCursor(wk, wk->cursor, FALSE);
            ZukanHabitat_ShowListButtons(wk);
            func_0204c488(actor, 1);
            func_0204c520(actor, TRUE);
            func_0204c4d4(actor, 0);
            ZukanHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
        }
        break;
    }
}

// The cursor on the Pokémon, which chooses one to see in the Pokédex
static void ZukanHabitat_SeqCursor(ZukanHabitatWork *wk) {
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);
    int hit = func_0203da0c(sZukanHabitatCursorTouchRects);

    switch (wk->step) {
    case 0:
        ZukanHabitat_HideListButtons(wk);
        if (wk->action == ACTION_DECIDE) {
            ZukanHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_NONE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if (wk->action == ACTION_TOUCH) {
            ZukanHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_TOUCH_DECIDE);
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
            ZukanHabitat_SetSequence(wk, SEQ_LEAVE_CURSOR, ACTION_RETURN);
            break;
        case CURSOR_TOUCH_INFO:
            if (ZukanHabitat_GetSpecies(wk, wk->cursor) >= 0) {
                ZukanHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_INFO);
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
                ZukanHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_TOUCH);
            }
            break;
        case TOUCH_RECT_NONE:
            ZukanHabitat_CursorKeys(wk);
            break;
        }
        break;
    }
}

static void ZukanHabitat_CursorKeys(ZukanHabitatWork *wk) {
    u32 pressed = GCTX_HIDGetPressedKeys();
    u32 repeat = GCTX_HIDGetTypedKeys();

    if (wk->cursorShown == TRUE) {
        if (repeat & PAD_KEY_LEFT) {
            ZukanHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_LEFT);
        } else if (repeat & PAD_KEY_RIGHT) {
            ZukanHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_RIGHT);
        } else if (repeat & PAD_KEY_UP) {
            ZukanHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_UP);
        } else if (repeat & PAD_KEY_DOWN) {
            ZukanHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_DOWN);
        } else if (pressed & PAD_BUTTON_B) {
            ZukanHabitat_SetSequence(wk, SEQ_LEAVE_CURSOR, ACTION_RETURN);
        } else if (pressed & PAD_BUTTON_A) {
            if (ZukanHabitat_GetSpecies(wk, wk->cursor) >= 0) {
                ZukanHabitat_SetSequence(wk, SEQ_BUTTON, ACTION_INFO);
            }
        }
    } else {
        // After a touch, any key brings the cursor back
        if (pressed & PAD_BUTTON_B) {
            ZukanHabitat_SetSequence(wk, SEQ_LEAVE_CURSOR, ACTION_RETURN);
        } else if (pressed) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            ZukanHabitat_SetSequence(wk, SEQ_MOVE_CURSOR, ACTION_NONE);
        }
    }
}

static void ZukanHabitat_SeqInit(ZukanHabitatWork *wk) {
    switch (wk->step) {
    case 0:
        ZukanHabitatGraphic_FillWindow(ZUKAN_HABITAT_WIN_MAIN, 0, &wk->graphic);
        func_02042ba8(TRUE, HEAPID_ZUKAN_HABITAT);
        ZukanHabitat_LoadSave(wk);
        ZukanHabitat_CheckTypes(wk);
        ZukanHabitat_UpdateList(wk);
        ZukanHabitat_UpdateTabs(wk);
        ZukanHabitat_UpdateMap(wk);
        ZukanHabitat_UpdateStamp(wk);
        ZukanHabitat_UpdateShortcut(wk);
        wk->step++;
        break;
    case 1:
        if (ZukanHabitatGraphic_AreIconsLoaded(&wk->graphic) &&
            !ZukanHabitatGraphic_IsPaletteRequested(&wk->graphic)) {
            ZukanHabitat_SetSequence(wk, SEQ_FADE_IN, ACTION_NONE);
            wk->finished = FALSE;
        }
        break;
    }
}

// A button that leaves the screen: it sets the result, and saves what the list showed
static void ZukanHabitat_SeqButton(ZukanHabitatWork *wk) {
    ClActor *actor;

    if (wk->action == ACTION_EXIT) {
        actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_EXIT);
        ZukanHabitat_SetResult(wk, ZUKAN_HABITAT_RESULT_EXIT);
    } else if (wk->action == ACTION_RETURN) {
        actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_RETURN);
        ZukanHabitat_SetResult(wk, ZUKAN_HABITAT_RESULT_RETURN);
    } else if (wk->action == ACTION_INFO) {
        actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_INFO);
        ZukanHabitat_SetResult(wk, ZUKAN_HABITAT_RESULT_INFO);
        func_02010d90(wk->habitat, (u16)wk->type);
        func_02010d80(wk->habitat, (u16)wk->season);
    }

    switch (wk->step) {
    case 0:
        func_02010d70(wk->habitat, ZukanHabitat_GetArea(wk)->zone);
        if (wk->action == ACTION_EXIT) {
            func_0204c488(actor, 8);
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
        } else if (wk->action == ACTION_RETURN) {
            func_0204c488(actor, 9);
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else if (wk->action == ACTION_INFO) {
            int species = ZukanHabitat_GetSpecies(wk, wk->cursor);

            if (species >= 0) {
                func_0200d200(wk->pokedex, species);
            }
            func_0204c488(actor, 29);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        func_0204c4d4(actor, 0);
        func_0204c520(actor, TRUE);
        wk->step++;
        break;
    case 1:
        if (!func_0204c560(actor)) {
            ZukanHabitat_SetSequence(wk, SEQ_FADE_OUT, ACTION_NONE);
        }
        break;
    }
}

static void ZukanHabitat_SeqChangeSeason(ZukanHabitatWork *wk) {
    ClActor *pressed;
    ClActor *other;
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);

    if (wk->action == ACTION_LEFT) {
        pressed = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_LEFT);
        other = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_RIGHT);
    } else if (wk->action == ACTION_RIGHT) {
        pressed = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_RIGHT);
        other = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_LEFT);
    }

    switch (wk->step) {
    case 0:
        func_0204c56c(other);
        func_0204c550(other);
        if (wk->action == ACTION_LEFT) {
            func_0204c488(pressed, 25);
            wk->season--;
            if (wk->season < 0) {
                wk->season = SEASON_COUNT - 1;
            }
        } else if (wk->action == ACTION_RIGHT) {
            func_0204c488(pressed, 23);
            wk->season++;
            if (wk->season >= SEASON_COUNT) {
                wk->season = 0;
            }
        }
        func_0204c504(pressed, 0);
        func_0204c520(pressed, TRUE);
        ZukanHabitat_UpdateList(wk);
        ZukanHabitat_UpdateTabs(wk);
        GFL_SndSEPlay(SEQ_SE_SELECT3);
        wk->step++;
        // fallthrough
    case 1:
        if (ZukanHabitatGraphic_AreIconsLoaded(&wk->graphic) && !ZukanHabitatGraphic_IsPaletteRequested(&wk->graphic) &&
            !func_0204c560(pressed)) {
            if (wk->action == ACTION_LEFT) {
                ClActor *right = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_RIGHT);

                func_0204c488(pressed, 24);
                func_0204c56c(pressed);
                func_0204c56c(right);
            } else if (wk->action == ACTION_RIGHT) {
                ClActor *left = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_LEFT);

                func_0204c488(pressed, 22);
                func_0204c56c(pressed);
                func_0204c56c(left);
            }
            func_0204c540(other);
            ZukanHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
        }
        break;
    }
}

static void ZukanHabitat_SeqChangeArea(ZukanHabitatWork *wk) {
    ClActor *actor;
    int anim;

    if (wk->action == ACTION_UP) {
        actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_AREA_UP);
        anim = 11;
    } else if (wk->action == ACTION_DOWN) {
        actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_AREA_DOWN);
        anim = 10;
    }

    switch (wk->step) {
    case 0:
        wk->season = func_02010d88(wk->habitat);
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
        func_0204c4d4(actor, 0);
        func_0204c488(actor, anim);
        func_0204c520(actor, TRUE);
        func_0204c540(actor);
        wk->step++;
        // fallthrough
    case 1:
        ZukanHabitat_CheckTypes(wk);
        ZukanHabitat_UpdateList(wk);
        ZukanHabitat_UpdateTabs(wk);
        ZukanHabitat_UpdateMap(wk);
        ZukanHabitat_UpdateStamp(wk);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        wk->step++;
        // fallthrough
    case 2:
        if (ZukanHabitatGraphic_AreIconsLoaded(&wk->graphic) && !ZukanHabitatGraphic_IsPaletteRequested(&wk->graphic) &&
            ZukanHabitatGraphic_IsPrinted(&wk->graphic)) {
            u32 award = ZukanHabitat_GetAward(wk);

            if (award != AWARD_NONE) {
                ZukanHabitat_SetAward(wk, award);
                ZukanHabitat_SetSequence(wk, SEQ_AWARD, ACTION_NONE);
            } else {
                ZukanHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
            }
        }
        break;
    }
}

// Shows the stamp that the area's kind of habitat has
static void ZukanHabitat_UpdateStamp(ZukanHabitatWork *wk) {
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);
    ClActor *actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_STAMP);

    switch (func_02010cc4(wk->habitat, ZukanHabitat_GetHabitatIndex(area->zone), wk->type)) {
    case AWARD_NONE:
        func_0204c124(actor, FALSE);
        break;
    case AWARD_SEEN:
        ZukanHabitatGraphic_StartActorAnim(&wk->graphic, ZUKAN_HABITAT_OBJ_STAMP, 39);
        func_0204c468(actor, 1);
        break;
    case AWARD_CAUGHT:
        ZukanHabitatGraphic_StartActorAnim(&wk->graphic, ZUKAN_HABITAT_OBJ_STAMP, 38);
        func_0204c468(actor, 1);
        break;
    }
}

static void ZukanHabitat_SeqChangeType(ZukanHabitatWork *wk) {
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
                ZukanHabitat_SkipType(wk, FALSE);
            }
        } else if (wk->action == ACTION_RIGHT) {
            wk->type++;
            if (wk->type >= TYPE_COUNT) {
                wk->type = 0;
            }
            if (wk->typeShown[wk->type] == FALSE) {
                ZukanHabitat_SkipType(wk, TRUE);
            }
        }
        ZukanHabitat_UpdateList(wk);
        ZukanHabitat_UpdateTabs(wk);
        ZukanHabitat_UpdateStamp(wk);
        GFL_SndSEPlay(SEQ_SE_SELECT3);
        actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_TAB + wk->type);
        func_0204c4d4(actor, 0);
        func_0204c488(actor, anims[wk->type]);
        func_0204c520(actor, TRUE);
        wk->step++;
        break;
    case 1:
        actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_TAB + wk->type);
        if (!func_0204c560(actor)) {
            func_0204c520(actor, FALSE);
            func_0204c488(actor, anims[wk->type]);
            func_0204c4d4(actor, 0);
            wk->step++;
        }
        break;
    case 2:
        if (ZukanHabitatGraphic_AreIconsLoaded(&wk->graphic) && !ZukanHabitatGraphic_IsPaletteRequested(&wk->graphic) &&
            ZukanHabitatGraphic_IsPrinted(&wk->graphic)) {
            u32 award = ZukanHabitat_GetAward(wk);

            if (award != AWARD_NONE) {
                ZukanHabitat_SetAward(wk, award);
                ZukanHabitat_SetSequence(wk, SEQ_AWARD, ACTION_NONE);
            } else {
                ZukanHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
            }
            wk->chosenType = wk->type;
        }
        break;
    }
}

// Goes on to the next kind of habitat that the area has
static void ZukanHabitat_SkipType(ZukanHabitatWork *wk, int forward) {
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

static void ZukanHabitat_SeqMoveCursor(ZukanHabitatWork *wk) {
    switch (wk->step) {
    case 0: {
        int column = 0;
        int row = 0;
        int old = wk->cursor;
        int lastRow;

        if (wk->cursor < wk->speciesCount) {
            ZukanHabitat_SetCursor(wk, wk->cursor, FALSE);
        }
        if (wk->cursor != 0) {
            column = wk->cursor % ZUKAN_HABITAT_COLUMNS;
            row = wk->cursor / ZUKAN_HABITAT_COLUMNS;
        }
        lastRow = (wk->speciesCount - 1) / ZUKAN_HABITAT_COLUMNS;

        if (wk->action == ACTION_LEFT) {
            column--;
            if (column < 0) {
                column = ZUKAN_HABITAT_COLUMNS - 1;
            }
        } else if (wk->action == ACTION_RIGHT) {
            column++;
            if (column > ZUKAN_HABITAT_COLUMNS - 1) {
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
            column = wk->touched % ZUKAN_HABITAT_COLUMNS;
            row = wk->touched / ZUKAN_HABITAT_COLUMNS;
        } else if (wk->action == ACTION_TOUCH_DECIDE) {
            column = wk->touched % ZUKAN_HABITAT_COLUMNS;
            row = wk->touched / ZUKAN_HABITAT_COLUMNS;
        }

        wk->cursor = column + row * ZUKAN_HABITAT_COLUMNS;
        if (wk->cursor >= wk->speciesCount) {
            // The last row is not full
            if (wk->action == ACTION_RIGHT) {
                wk->cursor = row * ZUKAN_HABITAT_COLUMNS;
            } else if (wk->action == ACTION_LEFT) {
                wk->cursor = wk->speciesCount - 1;
            } else if (wk->action == ACTION_UP) {
                wk->cursor = column + (row - 1) * ZUKAN_HABITAT_COLUMNS;
            } else if (wk->action == ACTION_DOWN) {
                wk->cursor = column;
            } else {
                wk->cursor = wk->speciesCount - 1;
            }
        }

        if (wk->action == ACTION_TOUCH) {
            if (ZukanHabitat_GetSpecies(wk, wk->cursor) >= 0) {
                ZukanHabitatGraphic_AnimateIcon(&wk->graphic, wk->cursor, TRUE);
                if (old != wk->cursor) {
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                }
            }
            wk->cursorShown = FALSE;
        } else if (wk->action == ACTION_TOUCH_DECIDE) {
            if (ZukanHabitat_GetSpecies(wk, wk->cursor) >= 0) {
                ZukanHabitatGraphic_AnimateIcon(&wk->graphic, wk->cursor, TRUE);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
            wk->cursorShown = FALSE;
        } else {
            ZukanHabitat_SetCursor(wk, wk->cursor, TRUE);
            if (wk->action != ACTION_NONE && old != wk->cursor) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        }

        if (ZukanHabitat_GetSpecies(wk, wk->cursor) >= 0) {
            ZukanHabitat_ShowInfoButton(wk, TRUE, TRUE);
        } else {
            ZukanHabitat_ShowInfoButton(wk, FALSE, TRUE);
        }
        wk->step++;
    }
        // fallthrough
    case 1:
        ZukanHabitat_SetSequence(wk, SEQ_CURSOR, ACTION_NONE);
        break;
    }
}

// Stamps the area: with the message and the jingle for the whole area, then the stamp of the kind of habitat
static void ZukanHabitat_SeqAward(ZukanHabitatWork *wk) {
    ZukanHabitatArea *area;
    ClActor *actor;

    switch (wk->step) {
    case 0:
        if (wk->award == AWARD_AREA) {
            StrBuf *strbuf;
            StrBuf *format;

            area = ZukanHabitat_GetArea(wk);
            loadLocationNameToStrbuf(wk->wordSet, 0, ZoneData_GetPlaceNameID(ZukanHabitat_GetArea(wk)->zone));
            strbuf = GFL_StrBufCreate(0x100, HEAPID_ZUKAN_HABITAT);
            format = GFL_MsgDataLoadStrbufNew(wk->msgData, Bank0362_Text_YouveRegisteredEveryPokemon);
            GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
            func_02010db8(wk->habitat);
            ZukanHabitatGraphic_CreateMessageWindow(&wk->graphic, 10, 5);
            ZukanHabitatGraphic_StartMessage(strbuf, &wk->graphic);
            GFL_StrBufFree(format);
            GFL_StrBufFree(strbuf);
            GFL_SndBGMPush();
            GFL_SndBGMPlay(SEQ_ME_ZUKAN_01, 0xffff);
            func_02010d08(wk->habitat, ZukanHabitat_GetHabitatIndex(area->zone));
            wk->step++;
        } else {
            wk->step = 3;
        }
        break;
    case 1:
        if (!GFL_SndBGMIsPlaying() && ZukanHabitatGraphic_GetMessageState(&wk->graphic) == PRINT_STREAM_DONE) {
            ZukanHabitatGraphic_WaitMessage(&wk->graphic);
            wk->step++;
        }
        break;
    case 2:
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48() == TRUE) {
            u32 award;
            int habitat;
            ZukanHabitatType type;

            area = ZukanHabitat_GetArea(wk);
            GFL_SndBGMPop();
            ZukanHabitatGraphic_CloseMessage(&wk->graphic);
            award = ZukanHabitat_GetAward(wk);
            habitat = ZukanHabitat_GetHabitatIndex(area->zone);
            for (type = TYPE_LAND; type < TYPE_COUNT; type++) {
                func_02010d14(wk->habitat, habitat, type, AWARD_CAUGHT);
            }
            if (award != AWARD_NONE) {
                wk->step++;
            } else {
                ZukanHabitat_UpdateNewMarks(wk);
                ZukanHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
                wk->step = 6;
            }
        }
        break;
    case 3:
        actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_STAMP);
        if (wk->award == AWARD_CAUGHT || wk->award == AWARD_AREA) {
            ZukanHabitatGraphic_StartActorAnim(&wk->graphic, ZUKAN_HABITAT_OBJ_STAMP, 26);
        } else {
            ZukanHabitatGraphic_StartActorAnim(&wk->graphic, ZUKAN_HABITAT_OBJ_STAMP, 27);
        }
        func_0204c468(actor, 0);
        wk->timer = 0;
        wk->step++;
        break;
    case 4:
        actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_STAMP);
        if (wk->timer == 19) {
            GFL_SndSEPlay(SEQ_SE_SW_ZUKANSTUMP_01);
        }
        wk->timer++;
        if (!func_0204c560(actor)) {
            func_0204c468(actor, 1);
            wk->step++;
        }
        break;
    case 5:
        area = ZukanHabitat_GetArea(wk);
        if (wk->award != AWARD_AREA) {
            int habitat = ZukanHabitat_GetHabitatIndex(area->zone);

            func_02010d14(wk->habitat, habitat, wk->type, wk->award);
        }
        ZukanHabitat_UpdateNewMarks(wk);
        wk->step++;
        if (wk->award == AWARD_CAUGHT && func_02010da8(wk->habitat) != TRUE) {
            ZukanHabitat_SetSequence(wk, SEQ_ALL_COMPLETE, ACTION_NONE);
        } else if (wk->award == AWARD_SEEN && func_02010da0(wk->habitat) != TRUE) {
            ZukanHabitat_SetSequence(wk, SEQ_ALL_COMPLETE, ACTION_NONE);
        } else {
            ZukanHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
        }
        break;
    }
}

// The message of the first stamp of each kind
static void ZukanHabitat_SeqAllComplete(ZukanHabitatWork *wk) {
    StrBuf *strbuf;

    switch (wk->step) {
    case 0:
        if (wk->award == AWARD_CAUGHT) {
            strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, Bank0362_Text_WhenAllPokemonHabitat_2);
            func_02010db8(wk->habitat);
        } else {
            strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, Bank0362_Text_WhenAllPokemonHabitat);
            func_02010db0(wk->habitat);
        }
        ZukanHabitatGraphic_CreateMessageWindow(&wk->graphic, 8, 9);
        ZukanHabitatGraphic_StartMessage(strbuf, &wk->graphic);
        GFL_StrBufFree(strbuf);
        wk->step++;
        break;
    case 1:
        if (ZukanHabitatGraphic_GetMessageState(&wk->graphic) == PRINT_STREAM_DONE) {
            ZukanHabitatGraphic_WaitMessage(&wk->graphic);
            wk->step++;
        }
        break;
    case 2: {
        ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);

        ZukanHabitatGraphic_CloseMessage(&wk->graphic);
        ZukanHabitat_SetSequence(wk, SEQ_LIST, ACTION_NONE);
        wk->step++;
        break;
    }
    }
}

// Prints what lives in the area's kind of habitat, and in which season if they differ
static void ZukanHabitat_PrintHabitat(ZukanHabitatWork *wk) {
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);
    u16 placeName = ZoneData_GetPlaceNameID(area->zone);
    u32 message;
    StrBuf *strbuf;
    StrBuf *format;

    if (wk->empty == FALSE) {
        message = ZukanHabitat_GetHabitatMessage(wk->type, area->unk8);
    } else {
        message = Bank0362_Text_NoPokemonLivesHere;
    }
    if (area->values[wk->type] == 1) {
        message += wk->season + 1;
    }
    loadLocationNameToStrbuf(wk->wordSet, 0, placeName);
    strbuf = GFL_StrBufCreate(0x100, HEAPID_ZUKAN_HABITAT);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData, message);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    ZukanHabitatGraphic_Print(ZUKAN_HABITAT_WIN_MAIN, strbuf, &wk->graphic, 12, 4, 240, 32, PRINT_COLOR(1, 2, 15));
    GFL_StrBufFree(format);
    GFL_StrBufFree(strbuf);
}

static u32 ZukanHabitat_GetHabitatMessage(u32 type, u32 land) {
    u32 landMessages[] = {
        Bank0362_Text_PokemonLivingTallGrass, Bank0362_Text_PokemonLivingSand,    Bank0362_Text_PokemonLivingSwamp,
        Bank0362_Text_PokemonLivingPlace,     Bank0362_Text_PokemonLivingPlace_2, Bank0362_Text_PokemonLivingLand,
        Bank0362_Text_NoPokemonLivesHere,
    };

    return ZukanHabitat_ChooseMessage(type, land, landMessages, Bank0362_Text_PokemonCatchWhileSurfing,
                                      Bank0362_Text_PokemonCatchByFishing);
}

static u32 ZukanHabitat_ChooseMessage(u32 type, u32 land, const u32 *landMessages, u32 water, u32 fishing) {
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

static void ZukanHabitat_PrintInfoLabel(ZukanHabitatWork *wk, u16 color) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, Bank0362_Text_Info);

    ZukanHabitatGraphic_Print(ZUKAN_HABITAT_WIN_MAIN, strbuf, &wk->graphic, 124, 172, 96, 16, color);
    GFL_StrBufFree(strbuf);
}

static void ZukanHabitat_ClearInfoLabel(ZukanHabitatWork *wk) {
    ZukanHabitatGraphic_ClearArea(ZUKAN_HABITAT_WIN_MAIN, &wk->graphic, 124, 172, 96, 16);
}

static void ZukanHabitat_PrintAreaName(ZukanHabitatWork *wk) {
    StrBuf *strbuf;
    StrBuf *format;

    loadLocationNameToStrbuf(wk->wordSet, 0, ZoneData_GetPlaceNameID(ZukanHabitat_GetArea(wk)->zone));
    strbuf = GFL_StrBufCreate(0x100, HEAPID_ZUKAN_HABITAT);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData, Bank0362_Text_Empty_6);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    ZukanHabitatGraphic_Print(ZUKAN_HABITAT_WIN_SUB, strbuf, &wk->graphic, 8, 4, 256, 16, PRINT_COLOR(14, 15, 0));
    GFL_StrBufFree(format);
    GFL_StrBufFree(strbuf);
}

static ZukanHabitatArea *ZukanHabitat_GetArea(ZukanHabitatWork *wk) {
    return &wk->param->areas[wk->area];
}

// Lists the Pokémon of the area's kind of habitat and season that the player has seen, and asks for their icons
static void ZukanHabitat_LoadSpecies(ZukanHabitatWork *wk) {
    u8 count = 0;
    int i;
    u8 unseen = 0;
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);
    ZukanHabitatSpecies *list;

    sys_memset(wk->species, 0, sizeof(wk->species));
    list = area->species;
    for (i = 0; i < ZUKAN_HABITAT_ICON_COUNT; i++) {
        ZukanHabitatSpecies *species = &list[i];

        if (species->status != STATUS_NONE && species->present[wk->season][wk->type] != 0) {
            if (species->status >= STATUS_SEEN) {
                u32 sex;
                u32 form;
                u32 rare;

                func_0200d3c8(wk->pokedex, species->species, &sex, &rare, &form, HEAPID_ZUKAN_HABITAT);
                if (func_0200d8d4(wk->pokedex, species->species, sex, rare, species->other[wk->season][wk->type]) ==
                    TRUE) {
                    form = species->other[wk->season][wk->type];
                }
                ZukanHabitatGraphic_RequestIcon(&wk->graphic, species->species, form, sex, count);
                ZukanHabitatGraphic_SetCellPalette(&wk->graphic, count, ZUKAN_HABITAT_CELL_FILLED);
                wk->species[count] = species->species;
                if (species->status == STATUS_SEEN) {
                    ZukanHabitatGraphic_SetMark(&wk->graphic, count, ZUKAN_HABITAT_MARK_SEEN);
                } else if (species->status == STATUS_CAUGHT) {
                    ZukanHabitatGraphic_SetMark(&wk->graphic, count, ZUKAN_HABITAT_MARK_CAUGHT);
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
    for (i = count; i < ZUKAN_HABITAT_ICON_COUNT; i++) {
        ZukanHabitatGraphic_SetIconVisible(&wk->graphic, i, FALSE);
        ZukanHabitatGraphic_SetCellPalette(&wk->graphic, i, ZUKAN_HABITAT_CELL_EMPTY);
        ZukanHabitatGraphic_SetMark(&wk->graphic, i, ZUKAN_HABITAT_MARK_NONE);
    }
    wk->speciesCount = count;
}

static void ZukanHabitat_UpdateTabs(ZukanHabitatWork *wk) {
    if (wk->type == TYPE_LAND) {
        ZukanHabitatGraphic_SetTab(&wk->graphic, TYPE_LAND, ZUKAN_HABITAT_TAB_CHOSEN);
    } else if (wk->typeShown[TYPE_LAND] == TRUE) {
        ZukanHabitatGraphic_SetTab(&wk->graphic, TYPE_LAND, ZUKAN_HABITAT_TAB_SHOWN);
    } else {
        ZukanHabitatGraphic_SetTab(&wk->graphic, TYPE_LAND, ZUKAN_HABITAT_TAB_HIDDEN);
    }
    if (wk->type == TYPE_WATER) {
        ZukanHabitatGraphic_SetTab(&wk->graphic, TYPE_WATER, ZUKAN_HABITAT_TAB_CHOSEN);
    } else if (wk->typeShown[TYPE_WATER] == TRUE) {
        ZukanHabitatGraphic_SetTab(&wk->graphic, TYPE_WATER, ZUKAN_HABITAT_TAB_SHOWN);
    } else {
        ZukanHabitatGraphic_SetTab(&wk->graphic, TYPE_WATER, ZUKAN_HABITAT_TAB_HIDDEN);
    }
    if (wk->type == TYPE_FISHING) {
        ZukanHabitatGraphic_SetTab(&wk->graphic, TYPE_FISHING, ZUKAN_HABITAT_TAB_CHOSEN);
    } else if (wk->typeShown[TYPE_FISHING] == TRUE) {
        ZukanHabitatGraphic_SetTab(&wk->graphic, TYPE_FISHING, ZUKAN_HABITAT_TAB_SHOWN);
    } else {
        ZukanHabitatGraphic_SetTab(&wk->graphic, TYPE_FISHING, ZUKAN_HABITAT_TAB_HIDDEN);
    }
    ZukanHabitatGraphic_RequestPalette(&wk->graphic, wk->type);
    ZukanHabitat_UpdateNewMarks(wk);
    ZukanHabitat_PrintHabitat(wk);
}

static void ZukanHabitat_UpdateNewMarks(ZukanHabitatWork *wk) {
    ZukanHabitat_UpdateNewMark(wk, TYPE_LAND, ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_NEW));
    ZukanHabitat_UpdateNewMark(wk, TYPE_WATER, ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_NEW + 1));
    ZukanHabitat_UpdateNewMark(wk, TYPE_FISHING,
                               ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_NEW + 2));
}

// Marks the tab of a kind of habitat that has a stamp to give
static void ZukanHabitat_UpdateNewMark(ZukanHabitatWork *wk, u32 type, ClActor *actor) {
    if (wk->typeShown[type] == TRUE) {
        if (ZukanHabitat_GetTypeAward(wk, type) != AWARD_NONE) {
            func_0204c124(actor, TRUE);
        } else {
            func_0204c124(actor, FALSE);
        }
    } else {
        func_0204c124(actor, FALSE);
    }
}

static void ZukanHabitat_UpdateList(ZukanHabitatWork *wk) {
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);

    if (wk->typeShown[wk->type] == FALSE) {
        ZukanHabitat_SkipType(wk, TRUE);
    }
    ZukanHabitat_LoadSpecies(wk);
    // The seasons' arrows show only where the seasons differ
    func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_LEFT), area->values[wk->type]);
    func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_RIGHT), area->values[wk->type]);
}

// Takes the cursor off its cell, and shows or hides it on another
static void ZukanHabitat_SetCursor(ZukanHabitatWork *wk, int cell, BOOL shown) {
    if (shown == TRUE) {
        ZukanHabitatGraphic_SetCellPalette(&wk->graphic, wk->cursor, ZUKAN_HABITAT_CELL_CURSOR);
    } else {
        ZukanHabitatGraphic_SetCellPalette(&wk->graphic, wk->cursor, ZUKAN_HABITAT_CELL_FILLED);
    }
    if (ZukanHabitat_GetSpecies(wk, cell) >= 0) {
        ZukanHabitatGraphic_AnimateIcon(&wk->graphic, cell, shown);
    }
    wk->cursorShown = shown;
}

// Finds which kinds of habitat the area has, and how many of each one's Pokémon the player has
static void ZukanHabitat_CheckTypes(ZukanHabitatWork *wk) {
    int type;
    int i;
    int season;
    ZukanHabitatSpecies *species = ZukanHabitat_GetArea(wk)->species;

    sys_memset(wk->typeShown, 0, sizeof(wk->typeShown));
    for (type = 0; type < TYPE_COUNT; type++) {
        BOOL seen = TRUE;
        BOOL caught = TRUE;

        for (i = 0; i < ZUKAN_HABITAT_ICON_COUNT; i++) {
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
        func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_AREA_UP), FALSE);
        func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_AREA_DOWN), FALSE);
    }
}

static void ZukanHabitat_HideListButtons(ZukanHabitatWork *wk) {
    ClActor *actor;

    actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_TAB + TYPE_LAND);
    func_0204c124(actor, FALSE);
    actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_TAB + TYPE_WATER);
    func_0204c124(actor, FALSE);
    actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_TAB + TYPE_FISHING);
    func_0204c124(actor, FALSE);
    actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_AREA_UP);
    func_0204c124(actor, FALSE);
    actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_AREA_DOWN);
    func_0204c124(actor, FALSE);
    actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SHORTCUT);
    func_0204c124(actor, FALSE);
    actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_EXIT);
    func_0204c124(actor, FALSE);
    ZukanHabitatGraphic_SetNewMarksVisible(&wk->graphic, FALSE);
    actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_INFO);
    func_0204c124(actor, TRUE);
    actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_LEFT);
    func_0204c124(actor, FALSE);
    actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_RIGHT);
    func_0204c124(actor, FALSE);
}

static void ZukanHabitat_ShowListButtons(ZukanHabitatWork *wk) {
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);

    func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_TAB + TYPE_LAND),
                  wk->typeShown[TYPE_LAND]);
    func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_TAB + TYPE_WATER),
                  wk->typeShown[TYPE_WATER]);
    func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_TAB + TYPE_FISHING),
                  wk->typeShown[TYPE_FISHING]);
    if (wk->areaCount > 1) {
        func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_AREA_UP), TRUE);
        func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_AREA_DOWN), TRUE);
    } else {
        func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_AREA_UP), FALSE);
        func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_AREA_DOWN), FALSE);
    }
    func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SHORTCUT), TRUE);
    func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_EXIT), TRUE);
    func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_LEFT), area->values[wk->type]);
    func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_SEASON_RIGHT), area->values[wk->type]);
    func_0204c124(ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_INFO), FALSE);
    ZukanHabitat_UpdateNewMarks(wk);
    ZukanHabitat_ClearInfoLabel(wk);
}

static void ZukanHabitat_UpdateMap(ZukanHabitatWork *wk) {
    ZukanHabitatGraphic_SetArea(&wk->graphic, ZukanHabitat_GetArea(wk)->zone);
    ZukanHabitatGraphic_UpdatePlayerMarker(&wk->graphic);
    ZukanHabitat_PrintAreaName(wk);
}

static void ZukanHabitat_SetResult(ZukanHabitatWork *wk, u32 result) {
    wk->param->result = result;
}

// Starts on the area, the season and the kind of habitat that the save has
static void ZukanHabitat_LoadSave(ZukanHabitatWork *wk) {
    int i;
    GameData *gameData = wk->param->gameData;
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);
    ZukanHabitatSave *habitat = func_02010cb8(GameData_GetSaveControl(gameData));
    u16 zone = func_02010d78(habitat);

    wk->pokedex = pokedex;
    wk->habitat = habitat;
    wk->areaCount = wk->param->count;
    wk->area = 0;
    for (i = 0; i < wk->areaCount; i++) {
        if (zone == wk->param->areas[i].zone) {
            wk->area = i;
        }
    }
    wk->chosenType = func_02010d98(wk->habitat);
    wk->season = func_02010d88(wk->habitat);
    wk->type = wk->chosenType;
}

// The species in a cell, or -1
static int ZukanHabitat_GetSpecies(ZukanHabitatWork *wk, u8 cell) {
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);
    int species = wk->species[cell];

    if (species == 0) {
        species = -1;
    }
    return species;
}

// The stamp that the area has yet to be given, an AWARD_*
static u32 ZukanHabitat_GetAward(ZukanHabitatWork *wk) {
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);

    if (area->complete == TRUE &&
        func_02010cfc(wk->habitat, ZukanHabitat_GetHabitatIndex(area->zone)) == FALSE) {
        return AWARD_AREA;
    }
    return ZukanHabitat_GetTypeAward(wk, wk->type);
}

static u32 ZukanHabitat_GetTypeAward(ZukanHabitatWork *wk, u32 type) {
    ZukanHabitatArea *area = ZukanHabitat_GetArea(wk);
    int done = func_02010cc4(wk->habitat, ZukanHabitat_GetHabitatIndex(area->zone), type);
    u32 *state = area->states;

    state += type;
    if (done < (int)*state) {
        return *state;
    }
    return AWARD_NONE;
}

static void ZukanHabitat_SetAward(ZukanHabitatWork *wk, u32 award) {
    wk->award = award;
}

static int ZukanHabitat_GetHabitatIndex(u16 zone) {
    int i;

    // ZUKAN_HABITAT_COUNT areas; save/zukan_habitat_savedata.h defines that name again, as the save's 70 slots
    for (i = 0; i < (int)NELEMS(sZukanHabitatAreas); i++) {
        if (zone == sZukanHabitatAreas[i].zone) {
            return i;
        }
    }
    return 0;
}

static void ZukanHabitat_ToggleShortcut(ZukanHabitatWork *wk) {
    GameData *gameData = wk->param->gameData;

    if (GameData_IsShortcutRegistered(gameData, SHORTCUT_POKEDEX_HABITAT) == TRUE) {
        GameData_SetKeyItemRegistration(gameData, SHORTCUT_POKEDEX_HABITAT, FALSE);
        ZukanHabitatGraphic_StartActorAnim(&wk->graphic, ZUKAN_HABITAT_OBJ_SHORTCUT, 6);
    } else {
        GameData_SetKeyItemRegistration(gameData, SHORTCUT_POKEDEX_HABITAT, TRUE);
        ZukanHabitatGraphic_StartActorAnim(&wk->graphic, ZUKAN_HABITAT_OBJ_SHORTCUT, 7);
    }
}

static void ZukanHabitat_UpdateShortcut(ZukanHabitatWork *wk) {
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_HABITAT) == TRUE) {
        ZukanHabitatGraphic_StartActorAnim(&wk->graphic, ZUKAN_HABITAT_OBJ_SHORTCUT, 7);
    } else {
        ZukanHabitatGraphic_StartActorAnim(&wk->graphic, ZUKAN_HABITAT_OBJ_SHORTCUT, 6);
    }
}

static BOOL ZukanHabitat_HasSeveralTypes(ZukanHabitatWork *wk) {
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
static void ZukanHabitat_ShowInfoButton(ZukanHabitatWork *wk, BOOL known, BOOL shown) {
    ClActor *actor = ZukanHabitatGraphic_GetActor(&wk->graphic, ZUKAN_HABITAT_OBJ_INFO);

    if (shown == FALSE) {
        ZukanHabitat_ClearInfoLabel(wk);
        func_0204c124(actor, FALSE);
    } else {
        func_0204c124(actor, TRUE);
        if (known == TRUE) {
            ZukanHabitat_PrintInfoLabel(wk, PRINT_COLOR(15, 1, 2));
            ZukanHabitatGraphic_StartActorAnim(&wk->graphic, ZUKAN_HABITAT_OBJ_INFO, 28);
        } else {
            ZukanHabitat_PrintInfoLabel(wk, PRINT_COLOR(2, 1, 0));
            ZukanHabitatGraphic_StartActorAnim(&wk->graphic, ZUKAN_HABITAT_OBJ_INFO, 41);
        }
    }
}

// Defined after the functions: the ROM has it ahead of ZukanHabitat_SeqChangeType's table of the same size
const GameProcFunctions ZukanHabitat_ProcFunctions = {
    ZukanHabitat_ProcInit,
    ZukanHabitat_ProcMain,
    ZukanHabitat_ProcEnd,
};
