#include "types.h"
#include "app/townmap.h"
#include "app/townmap/townmap_data.h"
#include "app/zukan_detail.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/player_state.h"
#include "field/townmap_util.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/calctool.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/poke_party.h"
#include "save/event_work.h"
#include "save/medal_box.h"
#include "save/pokedex.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/btl_server_flow_title.h"

// The detail screen's habitat map page, which the ROM does not name: the town map on the main screen with the areas
// where the Pokémon lives in each season, and a cursor to pick a place for the Pokédex's habitat list

// The town map's OBJ graphics, the Pokémon icons, and the habitats, a file per species
#define ARCID_TOWNMAP_GRA 84
#define ARCID_POKEICON 7
#define ARCID_ZUKAN_AREA 176

// The Pokédex's shortcut for the Y button, which the touch bar's check box registers
#define SHORTCUT_POKEDEX_MAP 21

// The event flags that show the water and fishing habitats
#define FLAG_HABITAT_WATER 0x987
#define FLAG_HABITAT_FISHING 0x985

#define SEASON_COUNT 4

#define NO_PLACE 0xff

// The BG of the panel that slides over the map, with the place's name and the habitat icons
#define PANEL_BG 7

#define ALL_PLANES                                                                                                     \
    (GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |               \
     GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD)

enum {
    MAP_SEQ_INIT,
    MAP_SEQ_FADE_IN_SUB,
    MAP_SEQ_FADE_IN,
    MAP_SEQ_WAIT_FADE_IN,
    MAP_SEQ_WAIT_STEP,
    MAP_SEQ_MAIN,
    MAP_SEQ_WAIT_EXIT,
    MAP_SEQ_WAIT_FADE_OUT,
    MAP_SEQ_FADE_OUT_SUB,
    MAP_SEQ_EXIT,
};

// How the page ends
enum {
    MAP_EXIT_NONE,
    MAP_EXIT_PAGE,
    MAP_EXIT_SCREEN,
};

// What the page is doing: showing the map, opening the panel to pick a place, picking one, closing the panel, or
// changing the habitat shown in either mode
enum {
    MAP_STATE_MAP,
    MAP_STATE_OPEN_START,
    MAP_STATE_OPEN,
    MAP_STATE_CHANGE,
    MAP_STATE_EXIT,
    MAP_STATE_PLACE,
    MAP_STATE_CLOSE_START,
    MAP_STATE_CLOSE,
    MAP_STATE_PLACE_CHANGE,
};

// How the panel and the Pokémon's icon slide
enum {
    SLIDE_NONE,
    SLIDE_IN,
    SLIDE_DONE,
    SLIDE_OUT,
};

// A place on the map: none, hidden as the player has not been there, or shown
enum {
    PLACE_NONE,
    PLACE_HIDDEN,
    PLACE_SHOWN,
};

// The cursor actors: the season's label and its arrows, the box saying the habitat is unknown, the button the touch
// bar's place button replaces, the cursor, the picked place's marker and the player's
enum {
    ACTOR_SEASON,
    ACTOR_SEASON_PREV,
    ACTOR_SEASON_NEXT,
    ACTOR_UNKNOWN,
    ACTOR_BUTTON,
    ACTOR_CURSOR,
    ACTOR_PLACE,
    ACTOR_PLAYER,
    ACTOR_COUNT,
};

// No season arrow is playing its animation
#define ACTOR_NONE ACTOR_COUNT

// The windows: the Pokémon's name, the place's name, the season, the unknown habitat's message and the place button
enum {
    WINDOW_NAME,
    WINDOW_PLACE,
    WINDOW_SEASON,
    WINDOW_UNKNOWN,
    WINDOW_BUTTON,
    WINDOW_COUNT,
};

// What a VBlank changes
enum {
    VBLANK_NONE,
    VBLANK_DIM,
    VBLANK_HIGHLIGHT,
    VBLANK_RESET,
};

// The kinds of habitat of a place in a season
#define HABITAT_LAND 0x07
#define HABITAT_WATER 0x18
#define HABITAT_FISHING 0x60

#define HABITAT_KIND_COUNT 3

// A Pokémon's habitat
typedef struct {
    // Whether it is the same all year, as the first season
    u8 allYear;
    struct {
        // Whether it lives nowhere in the season
        u8 none;
        // The HABITAT_* of each place, in the order of sZukanDetailMapZones
        u8 places[TOWNMAP_PLACE_COUNT];
    } seasons[SEASON_COUNT];
} ZukanHabitat;

typedef struct {
    int state;
    ClActor *actor;
} MapPlace;

typedef struct {
    ClActUnit *unit;
    Font *font;
    ZukanDetailBackground *background;
    MsgData *msgData[2];
    BmpWin *windows[WINDOW_COUNT];
    BOOL transferPending[WINDOW_COUNT];
    PrintQueue *printQueues[WINDOW_COUNT];
    BmpWin *bgWindows[2];
    // The Pokémon's icons, two to slide one in as the other leaves
    u32 iconChars[2];
    u32 iconPalette;
    u32 iconCellAnims;
    ClActor *icons[2];
    int icon;
    u32 panelChars;
    int state;
    int panelSlide;
    int iconSlide;
    u8 season;
    u8 place;
    // Whether the panel was opened by touching the map, and where
    BOOL touched;
    u8 touchX;
    u8 touchY;
    // The step of a state's change, done over frames
    int step;
    BOOL active;
    // Whether the habitat changes with the seasons, and whether the Pokémon lives anywhere in the season shown
    BOOL bySeason;
    BOOL found;
    ZukanHabitat *habitat;
    u32 mapChars;
    u32 mapPalette;
    u32 mapCellAnims;
    u32 seasonChars;
    u32 seasonPalette;
    u32 seasonCellAnims;
    u32 areaChars;
    u32 areaPalette;
    u32 areaCellAnims;
    ClActor *actors[ACTOR_COUNT];
    // The season arrow playing its animation
    u8 arrow;
    // Where the Pokémon lives
    ClActor *areas[TOWNMAP_PLACE_COUNT];
    ClActUnit *unit2;
    ClActRenderer *renderer;
    int glowEv;
    u16 glowPhase;
    MapPlace places[TOWNMAP_PLACE_COUNT];
    void *townmap;
    // Each place's index in sZukanDetailMapZones
    u8 placeOrder[TOWNMAP_PLACE_COUNT];
    ArcTool *habitatArc;
    TCB *vblankTcb;
    u8 vblankRequest;
    ZukanDetailBlend *blendMain;
    ZukanDetailBlend *blendSub;
    ZukanDetailPalFade *palFade;
    int exit;
    BOOL inputEnabled;
    // An arrow of the touch bar was touched, the Pokémon it moved to is shown, and the arrow's animation has played
    BOOL arrowPushed;
    BOOL arrowLoaded;
    BOOL arrowDone;
} ZukanDetailMapWork;

// The event flag that the habitat list needs for a zone
#define PLACE_FLAG_COUNT 57

typedef struct {
    u16 flag;
    u16 zone;
    u16 id;
} MapPlaceFlag;

static BOOL ZukanDetailMap_Init(ZukanDetailProcSys *sys, int *seq, void *param, void *work, ZukanDetailCommon *common);
static BOOL ZukanDetailMap_Exit(ZukanDetailProcSys *sys, int *seq, void *param, void *work, ZukanDetailCommon *common);
static BOOL ZukanDetailMap_Main(ZukanDetailProcSys *sys, int *seq, void *param, void *work, ZukanDetailCommon *common);
static void ZukanDetailMap_Command(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                   ZukanDetailCommon *common, int command);
static void ZukanDetailMap_VBlank(TCB *tcb, void *data);
static void ZukanDetailMap_InitPlaceOrder(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                          ZukanDetailCommon *common);
static void ZukanDetailMap_LoadMapBG(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_FreeMapBG(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_CreateActors(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_FreeActors(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_CreateAreas(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_FreeAreas(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_CreatePlaces(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_FreePlaces(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_CreatePrintQueues(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                             ZukanDetailCommon *common);
static void ZukanDetailMap_FreePrintQueues(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                           ZukanDetailCommon *common);
static void ZukanDetailMap_UpdatePrint(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_TransferWindow(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                          u8 window);
static void ZukanDetailMap_CreateWindows(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_FreeWindows(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_LoadPanelBG(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_FreePanelBG(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_UpdateSlide(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_UpdateMainSlide(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                           ZukanDetailCommon *common);
static void ZukanDetailMap_UpdateSubSlide(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                          ZukanDetailCommon *common);
static void ZukanDetailMap_UpdateArrow(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_LoadIconResources(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                             ZukanDetailCommon *common);
static void ZukanDetailMap_FreeIconResources(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                             ZukanDetailCommon *common);
static ClActor *ZukanDetailMap_CreateIcon(u32 *chars, u32 palette, u32 cellAnims, ClActUnit *unit, HeapID heapId,
                                          u32 species, u32 form, u32 sex, BOOL egg, u8 x, u8 y);
static void ZukanDetailMap_FreeIcon(u32 chars, ClActor *actor);
static u8 ZukanDetailMap_GetPlayerPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_InitPlayerMarker(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                            ZukanDetailCommon *common);
static void ZukanDetailMap_SetPlayerMarkerVisible(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                                  ZukanDetailCommon *common, BOOL visible);
static BOOL ZukanDetailMap_IsPlayerMarkerVisible(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                                 ZukanDetailCommon *common);
static void ZukanDetailMap_ResetCursor(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_SelectPlayerPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                             ZukanDetailCommon *common);
static u8 ZukanDetailMap_FindTouchedPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                          u8 x, u8 y);
static BOOL ZukanDetailMap_IsNearPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                       u8 place, u8 x, u8 y, u32 *distSq);
static u8 ZukanDetailMap_FindNearestPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                          u8 x, u8 y);
static void ZukanDetailMap_HandleInput(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_MoveCursor(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static BOOL ZukanDetailMap_SetHabitatKind(ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_UpdateState(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_SetState(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                    int state);
static void ZukanDetailMap_LoadPokemon(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_ShowHabitat(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_StartExit(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_PrintPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_ChangeSeason(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                        BOOL next, BOOL touch);
static ZukanHabitat *ZukanDetailMap_LoadHabitat(u16 species, HeapID heapId, ArcTool *arc);
static void ZukanDetailMap_FreeHabitat(ZukanHabitat *habitat);
static void ZukanDetailMap_PrintSeason(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_StartChange(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_HideHabitat(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_RequestHabitatBlend(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                               ZukanDetailCommon *common);
static void ZukanDetailMap_ShowAreas(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_PrintButton(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                       BOOL active);
static void ZukanDetailMap_ClearButton(ZukanDetailMapWork *wk);
static void ZukanDetailMap_HideAreas(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_RequestReset(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_FinishExit(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_UpdateKinds(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static void ZukanDetailMap_SetDimBlend(ZukanDetailMapWork *wk);
static void ZukanDetailMap_ResetBlend(ZukanDetailMapWork *wk);
static void ZukanDetailMap_SetHighlight(ZukanDetailMapWork *wk);
static void ZukanDetailMap_ClearHighlight(ZukanDetailMapWork *wk);
static void ZukanDetailMap_UpdateGlow(ZukanDetailMapWork *wk);
static void ZukanDetailMap_StartGlow(ZukanDetailMapWork *wk);
static void ZukanDetailMap_SetActorsOpaque(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                           ZukanDetailCommon *common);
static void ZukanDetailMap_SetActorsBlended(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                            ZukanDetailCommon *common);
static void ZukanDetailMap_SetIconsOpaque(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                          ZukanDetailCommon *common);
static void ZukanDetailMap_SetIconsBlended(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                           ZukanDetailCommon *common);
static BOOL ZukanDetailMap_IsPlaceListed(ZukanDetailMapWork *wk, ZukanDetailCommon *common);
static int ZukanDetailMap_GetPlaceFlag(u16 zone);
static void ZukanDetailMap_UpdateButton(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                        BOOL show);

const ZukanDetailProcFuncs ZUKAN_DETAIL_MAP_PROC_FUNCS = {
    ZukanDetailMap_Init, ZukanDetailMap_Main, ZukanDetailMap_Exit, ZukanDetailMap_Command, NULL,
};

// The zones of the places, in the order of the habitat files, and an unused 0
static const u16 sZukanDetailMapZones[TOWNMAP_PLACE_COUNT + 1] = {
    0x185, 0x18d, 0x006, 0x010, 0x01c, 0x03e, 0x060, 0x06b, 0x071, 0x078, 0x088, 0x196, 0x19c, 0x1a2, 0x000,
    0x1a8, 0x093, 0x117, 0x17d, 0x098, 0x09a, 0x09d, 0x0a0, 0x0bf, 0x0c2, 0x0c6, 0x0cd, 0x0e6, 0x0eb, 0x0ee,
    0x144, 0x14d, 0x152, 0x15a, 0x178, 0x0f0, 0x181, 0x13d, 0x13f, 0x141, 0x146, 0x149, 0x14b, 0x151, 0x159,
    0x15c, 0x16d, 0x170, 0x172, 0x176, 0x17a, 0x17f, 0x1a7, 0x183, 0x0f9, 0x0fd, 0x0fe, 0x0ff, 0x107, 0x1ab,
    0x1b7, 0x1c0, 0x1ca, 0x1d1, 0x236, 0x1ea, 0x1b5, 0x1be, 0x1cf, 0x1da, 0x1ef, 0x1bc, 0x1c8, 0x1cd, 0x1ce,
    0x1db, 0x23d, 0x1f7, 0x203, 0x1fa, 0x205, 0x1d0, 0x108, 0x228, 0x0f1, 0x000,
};

// The map's BGs 2 and 3, 256-color bitmaps, and the BGs they replace
static const ZukanDetailBGSetup sZukanDetailMapBGSetups[2] = {
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x4000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_EXTENDED,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_EXTENDED,
      TRUE },
};

static const ZukanDetailBGSetup sZukanDetailMapDefaultBGSetups[2] = {
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x10000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x18000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

static const MapPlaceFlag sZukanDetailMapPlaceFlags[PLACE_FLAG_COUNT] = {
    { 0x9b8, 0x1ab, 24 }, { 0x9c0, 0x1b5, 23 }, { 0x9c1, 0x1be, 28 }, { 0x9c2, 0x1bc, 26 }, { 0x9b9, 0x1c0, 29 },
    { 0x9c3, 0x1c8, 32 }, { 0x9ac, 0x01c, 34 }, { 0x9c4, 0x1ef, 30 }, { 0x9c5, 0x146, 0 },  { 0x9c6, 0x09d, 41 },
    { 0x9c7, 0x0a0, 21 }, { 0x9c8, 0x149, 9 },  { 0x9c9, 0x17f, 8 },  { 0x9ca, 0x181, 55 }, { 0x9cb, 0x0fd, 53 },
    { 0x9cc, 0x14b, 13 }, { 0x9cd, 0x1f7, 7 },  { 0x9f0, 0x1fa, 3 },  { 0x9ce, 0x14d, 49 }, { 0x9cf, 0x0c2, 57 },
    { 0x9d0, 0x151, 19 }, { 0x9d1, 0x152, 39 }, { 0x9d2, 0x1cd, 37 }, { 0x9d3, 0x1ce, 27 }, { 0x9b5, 0x19c, 12 },
    { 0x9d4, 0x172, 50 }, { 0x9d5, 0x0f0, 14 }, { 0x9d6, 0x176, 56 }, { 0x9d7, 0x178, 11 }, { 0x9d8, 0x170, 47 },
    { 0x9d9, 0x0ff, 5 },  { 0x9da, 0x16d, 42 }, { 0x9db, 0x15c, 31 }, { 0x9dc, 0x203, 54 }, { 0x9dd, 0x1cf, 2 },
    { 0x9de, 0x17a, 1 },  { 0x9bf, 0x107, 10 }, { 0x9ba, 0x1d1, 36 }, { 0x9df, 0x1da, 40 }, { 0x9e0, 0x0e6, 18 },
    { 0x9e1, 0x1db, 45 }, { 0x9e2, 0x23d, 20 }, { 0x9e3, 0x159, 25 }, { 0x9e4, 0x15a, 46 }, { 0x9b0, 0x071, 52 },
    { 0x9e5, 0x0cd, 6 },  { 0x9e6, 0x0c6, 4 },  { 0x9e7, 0x09a, 35 }, { 0x9e8, 0x141, 51 }, { 0x9e9, 0x144, 15 },
    { 0x9aa, 0x006, 22 }, { 0x9ea, 0x098, 38 }, { 0x9ec, 0x13f, 48 }, { 0x9eb, 0x13d, 44 }, { 0x9ed, 0x1a7, 43 },
    { 0x9ee, 0x183, 17 }, { 0x9ef, 0x0ee, 33 },
};

// The palettes of the habitat kinds' icons on the panel, without and with the kind, and the icons' screen areas
static u8 sZukanDetailMapKindPalettes[HABITAT_KIND_COUNT][2] = {
    { 5, 2 },
    { 6, 0 },
    { 7, 4 },
};

static u8 sZukanDetailMapKindRects[HABITAT_KIND_COUNT][4] = {
    { 7, 28, 4, 3 },
    { 14, 28, 4, 3 },
    { 21, 28, 4, 3 },
};

void ZukanDetailMap_InitParam(ZukanDetailMapParam *param, HeapID heapId) {
    param->heapId = heapId;
    param->place = ZUKAN_DETAIL_MAP_NO_PLACE;
}

static BOOL ZukanDetailMap_Init(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                ZukanDetailCommon *common) {
    ZukanDetailMapParam *param = param_;
    GameData *gameData = ZukanDetailCommon_GetGameData(common);
    ZukanDetailMapWork *wk;
    u8 i;

    GFL_OvlLoad(OVERLAY_TOWNMAP);
    wk = ZukanDetailProcSys_AllocWork(sys, sizeof(ZukanDetailMapWork), param->heapId);
    sys_memset(wk, 0, sizeof(ZukanDetailMapWork));
    wk->unit = ZukanDetailGraphic_GetClActUnit(ZukanDetailCommon_GetGraphic(common));
    wk->font = ZukanDetailCommon_GetFont(common);
    for (i = 0; i < 2; i++) {
        wk->icons[i] = NULL;
    }
    wk->icon = 0;
    wk->bySeason = FALSE;
    wk->found = TRUE;
    wk->habitat = NULL;
    wk->state = MAP_STATE_MAP;
    wk->panelSlide = SLIDE_NONE;
    wk->iconSlide = SLIDE_NONE;
    wk->season = GameData_GetSeason(gameData);
    wk->place = NO_PLACE;
    wk->step = 0;
    wk->active = TRUE;
    wk->townmap = TownMapData_Load(param->heapId);
    ZukanDetailMap_InitPlaceOrder(param, wk, common);
    wk->habitatArc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_AREA, param->heapId);
    wk->vblankTcb = GFL_VBlankTCBAdd(ZukanDetailMap_VBlank, wk, 1);
    wk->vblankRequest = VBLANK_NONE;
    ZukanDetailMap_CreatePrintQueues(param, wk, common);
    wk->blendMain = ZukanDetailBlend_Create(param->heapId);
    wk->blendSub = ZukanDetailBlend_Create(param->heapId);
    ZukanDetailBlend_InitPlanes(wk->blendMain);
    ZukanDetailBlend_InitPlanes(wk->blendSub);
    ZukanDetailBlend_SetOut(0, wk->blendMain);
    ZukanDetailBlend_SetOut(1, wk->blendSub);
    wk->palFade =
        ZukanDetailPalFade_CreateEx(param->heapId, ZUKAN_DETAIL_PALFADE_SUB_BG | ZUKAN_DETAIL_PALFADE_SUB_OBJ);
    wk->exit = MAP_EXIT_NONE;
    wk->inputEnabled = TRUE;
    wk->arrowPushed = FALSE;
    wk->arrowLoaded = TRUE;
    wk->arrowDone = TRUE;
    return TRUE;
}

static BOOL ZukanDetailMap_Exit(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                ZukanDetailCommon *common) {
    ZukanDetailMapParam *param = param_;
    ZukanDetailMapWork *wk = work;
    u8 i;

    ZukanDetailMap_FreeHabitat(wk->habitat);
    for (i = 0; i < 2; i++) {
        if (wk->icons[i] != NULL) {
            ZukanDetailMap_FreeIcon(wk->iconChars[i], wk->icons[i]);
        }
        wk->icons[i] = NULL;
    }
    ZukanDetailMap_FreeIconResources(param, wk, common);
    ZukanDetailMap_FreePanelBG(param, wk, common);
    ZukanDetailMap_FreeWindows(param, wk, common);
    ZukanDetailBackground_Free(wk->background);
    ZukanDetailMap_FreePlaces(param, wk, common);
    ZukanDetailMap_FreeAreas(param, wk, common);
    ZukanDetailMap_FreeActors(param, wk, common);
    ZukanDetailPalFade_Free(wk->palFade);
    ZukanDetailBlend_Free(wk->blendSub);
    ZukanDetailBlend_Free(wk->blendMain);
    ZukanDetailMap_FreePrintQueues(param, wk, common);
    GFL_TCBRemove(wk->vblankTcb);
    GFL_ArcToolFree(wk->habitatArc);
    TownMapData_Free(wk->townmap);
    ZukanDetailProcSys_FreeWork(sys);
    GFL_OvlUnload(OVERLAY_TOWNMAP);
    return TRUE;
}

static BOOL ZukanDetailMap_Main(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                ZukanDetailCommon *common) {
    ZukanDetailMapParam *param = param_;
    ZukanDetailMapWork *wk = work;
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    ZukanDetailHeadbar *headbar = ZukanDetailCommon_GetHeadbar(common);

    switch (*seq) {
    case MAP_SEQ_INIT: {
        u32 i;
        u8 bg;

        *seq = MAP_SEQ_FADE_IN_SUB;
        gfxSetEngineModeA(GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BG0_AS_2D);
        for (i = 0; i < NELEMS(sZukanDetailMapBGSetups); i++) {
            GFL_BGSysReleaseBG(sZukanDetailMapBGSetups[i].bg);
            GFL_BGSysCreateBG(sZukanDetailMapBGSetups[i].bg, &sZukanDetailMapBGSetups[i].setup,
                              sZukanDetailMapBGSetups[i].mode);
            GFL_BGSysClearBG(sZukanDetailMapBGSetups[i].bg);
            GFL_BGSysSetBGEnabled(sZukanDetailMapBGSetups[i].bg, sZukanDetailMapBGSetups[i].enabled);
        }
        for (bg = 0; bg <= 7; bg++) {
            if (bg != 1 && bg != 5) {
                GFL_BGSysMoveBG(bg, BG_MOVE_SET_X, 0);
                GFL_BGSysMoveBG(bg, BG_MOVE_SET_Y, 0);
                GFL_BGSysClearBG(bg);
            }
        }
        GFL_BGSysSetBGPriority(2, 2);
        GFL_BGSysSetBGPriority(3, 3);
        GFL_BGSysSetBGPriority(0, 0);
        GFL_BGSysSetBGPriority(6, 1);
        GFL_BGSysSetBGPriority(PANEL_BG, 2);
        GFL_BGSysSetBGPriority(4, 3);
        ZukanDetailTouchbar_SetBGPriority(touchbar, 1);
        ZukanDetailMap_LoadMapBG(param, wk, common);
        ZukanDetailMap_CreateActors(param, wk, common);
        ZukanDetailMap_CreateAreas(param, wk, common);
        ZukanDetailMap_CreatePlaces(param, wk, common);
        wk->background = ZukanDetailBackground_Create(param->heapId, 1, 4, 9, 10);
        ZukanDetailMap_CreateWindows(param, wk, common);
        ZukanDetailMap_LoadPanelBG(param, wk, common);
        ZukanDetailMap_LoadIconResources(param, wk, common);
        ZukanDetailMap_LoadPokemon(param, wk, common);
        func_0204c318(wk->icons[wk->icon], 1);
        ZukanDetailMap_InitPlayerMarker(param, wk, common);
        if (wk->found == TRUE) {
            ZukanDetailMap_SetPlayerMarkerVisible(param, wk, common, TRUE);
        }
        ZukanDetailPalFade_ReadPalettes(wk->palFade);
        ZukanDetailPalFade_SetHidden(wk->palFade);
        break;
    }
    case MAP_SEQ_FADE_IN_SUB:
        *seq = MAP_SEQ_FADE_IN;
        ZukanDetailBlend_SetIn(1, wk->blendSub);
        ZukanDetailMap_SetIconsOpaque(param, wk, common);
        break;
    case MAP_SEQ_FADE_IN:
        *seq = MAP_SEQ_WAIT_FADE_IN;
        ZukanDetailBlend_StartIn(wk->blendMain);
        ZukanDetailPalFade_StartIn(wk->palFade);
        if (ZukanDetailTouchbar_GetState(touchbar) != ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
            ZukanDetailTouchbar_SetType(touchbar, ZUKAN_DETAIL_TOUCHBAR_GENERAL, ZUKAN_DETAIL_PAGE_MAP - 1,
                                        ZukanDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
            ZukanDetailTouchbar_Appear(touchbar, 0);
        } else {
            ZukanDetailTouchbar_SetPage(touchbar, ZUKAN_DETAIL_PAGE_MAP - 1);
        }
        ZukanDetailTouchbar_SetActive(touchbar, FALSE);
        wk->active = FALSE;
        ZukanDetailTouchbar_SetCheck(
            touchbar, GameData_IsShortcutRegistered(ZukanDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_MAP));
        if (ZukanDetailHeadbar_GetState(headbar) != ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
            ZukanDetailHeadbar_SetTitle(headbar, 1);
            ZukanDetailHeadbar_Appear(headbar);
        }
        break;
    case MAP_SEQ_WAIT_FADE_IN:
        if (!ZukanDetailBlend_IsActive(wk->blendMain) && !ZukanDetailPalFade_IsFading(wk->palFade) &&
            ZukanDetailTouchbar_GetState(touchbar) == ZUKAN_DETAIL_TOUCHBAR_SHOWN &&
            ZukanDetailHeadbar_GetState(headbar) == ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
            ZukanDetailTouchbar_Unlock(touchbar);
            ZukanDetailMap_SetActorsOpaque(param, wk, common);
            ZukanDetailMap_ShowHabitat(param, wk, common);
            *seq = MAP_SEQ_WAIT_STEP;
        }
        break;
    case MAP_SEQ_WAIT_STEP:
        if (wk->step == 0) {
            ZukanDetailTouchbar_SetActive(touchbar, TRUE);
            wk->active = TRUE;
            *seq = MAP_SEQ_MAIN;
        }
        break;
    case MAP_SEQ_MAIN:
        if (wk->exit != MAP_EXIT_NONE) {
            ZukanDetailMap_StartExit(param, wk, common);
            *seq = MAP_SEQ_WAIT_EXIT;
        } else {
            if (wk->found == TRUE) {
                ZukanDetailMap_UpdateGlow(wk);
            }
            ZukanDetailMap_HandleInput(param, wk, common);
        }
        break;
    case MAP_SEQ_WAIT_EXIT:
        if (wk->step == 0) {
            ZukanDetailMap_SetActorsBlended(param, wk, common);
            *seq = MAP_SEQ_WAIT_FADE_OUT;
            ZukanDetailBlend_StartOut(wk->blendMain);
            ZukanDetailPalFade_StartOut(wk->palFade);
            ZukanDetailHeadbar_Disappear(headbar);
            if (wk->exit == MAP_EXIT_SCREEN) {
                ZukanDetailTouchbar_Disappear(touchbar, 0);
            }
        }
        break;
    case MAP_SEQ_WAIT_FADE_OUT: {
        BOOL done = FALSE;

        if (!ZukanDetailBlend_IsActive(wk->blendMain) && !ZukanDetailPalFade_IsFading(wk->palFade) &&
            ZukanDetailHeadbar_GetState(headbar) == ZUKAN_DETAIL_TOUCHBAR_HIDDEN) {
            if (wk->exit == MAP_EXIT_SCREEN) {
                if (ZukanDetailTouchbar_GetState(touchbar) == ZUKAN_DETAIL_TOUCHBAR_HIDDEN) {
                    done = TRUE;
                }
            } else {
                done = TRUE;
            }
        }
        if (done) {
            *seq = MAP_SEQ_FADE_OUT_SUB;
        }
        break;
    }
    case MAP_SEQ_FADE_OUT_SUB:
        *seq = MAP_SEQ_EXIT;
        ZukanDetailMap_SetIconsBlended(param, wk, common);
        ZukanDetailBlend_SetOut(1, wk->blendSub);
        break;
    case MAP_SEQ_EXIT: {
        u32 i;

        ZukanDetailMap_FreeMapBG(param, wk, common);
        gfxSetEngineModeA(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_2D);
        for (i = 0; i < NELEMS(sZukanDetailMapDefaultBGSetups); i++) {
            GFL_BGSysReleaseBG(sZukanDetailMapDefaultBGSetups[i].bg);
            GFL_BGSysCreateBG(sZukanDetailMapDefaultBGSetups[i].bg, &sZukanDetailMapDefaultBGSetups[i].setup,
                              sZukanDetailMapDefaultBGSetups[i].mode);
            GFL_BGSysClearBG(sZukanDetailMapDefaultBGSetups[i].bg);
            GFL_BGSysSetBGEnabled(sZukanDetailMapDefaultBGSetups[i].bg, sZukanDetailMapDefaultBGSetups[i].enabled);
        }
        return TRUE;
    }
    }

    if (*seq >= MAP_SEQ_FADE_IN) {
        ZukanDetailBackground_Update(wk->background);
        ZukanDetailMap_UpdateSlide(param, wk, common);
        ZukanDetailMap_UpdateArrow(param, wk, common);
    }
    ZukanDetailMap_UpdateState(param, wk, common);
    ZukanDetailBlend_Update(wk->blendMain, wk->blendSub);
    ZukanDetailPalFade_Update(wk->palFade);
    ZukanDetailMap_UpdatePrint(param, wk, common);
    return FALSE;
}

static void ZukanDetailMap_Command(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                   ZukanDetailCommon *common, int command) {
    ZukanDetailMapWork *wk = work;

    if (wk != NULL) {
        ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
        BOOL unlock = FALSE;

        // Input waits while the bar's icons play their animations
        switch (command) {
        case ZUKAN_DETAIL_CMD_CLOSE_TOUCH:
        case ZUKAN_DETAIL_CMD_RETURN_TOUCH:
        case ZUKAN_DETAIL_CMD_CUR_D_TOUCH:
        case ZUKAN_DETAIL_CMD_CUR_U_TOUCH:
        case ZUKAN_DETAIL_CMD_CHECK_TOUCH:
        case ZUKAN_DETAIL_CMD_INFO_TOUCH:
        case ZUKAN_DETAIL_CMD_VOICE_TOUCH:
        case ZUKAN_DETAIL_CMD_FORM_TOUCH:
        case ZUKAN_DETAIL_CMD_MAP_RETURN_TOUCH:
        case ZUKAN_DETAIL_CMD_MAP_PLACE_TOUCH:
            wk->inputEnabled = FALSE;
            break;
        case ZUKAN_DETAIL_CMD_MAP_TOUCH:
            break;
        }
        switch (command) {
        case ZUKAN_DETAIL_CMD_CLOSE:
        case ZUKAN_DETAIL_CMD_RETURN:
        case ZUKAN_DETAIL_CMD_CUR_D:
        case ZUKAN_DETAIL_CMD_CUR_U:
        case ZUKAN_DETAIL_CMD_CHECK:
        case ZUKAN_DETAIL_CMD_INFO:
        case ZUKAN_DETAIL_CMD_VOICE:
        case ZUKAN_DETAIL_CMD_FORM:
        case ZUKAN_DETAIL_CMD_MAP_RETURN:
            unlock = TRUE;
            wk->inputEnabled = TRUE;
            break;
        }

        switch (command) {
        case ZUKAN_DETAIL_CMD_CUR_D_TOUCH: {
            u16 species = ZukanDetailCommon_GetSpecies(common);

            ZukanDetailCommon_GoNext(common);
            if (species != ZukanDetailCommon_GetSpecies(common)) {
                ZukanDetailMap_LoadPokemon(param, wk, common);
                ZukanDetailMap_ShowHabitat(param, wk, common);
                wk->arrowPushed = TRUE;
                wk->arrowLoaded = FALSE;
                wk->arrowDone = FALSE;
            } else {
                wk->arrowPushed = TRUE;
                wk->arrowLoaded = TRUE;
                wk->arrowDone = FALSE;
            }
            break;
        }
        case ZUKAN_DETAIL_CMD_CUR_U_TOUCH: {
            u16 species = ZukanDetailCommon_GetSpecies(common);

            ZukanDetailCommon_GoPrev(common);
            if (species != ZukanDetailCommon_GetSpecies(common)) {
                ZukanDetailMap_LoadPokemon(param, wk, common);
                ZukanDetailMap_ShowHabitat(param, wk, common);
                wk->arrowPushed = TRUE;
                wk->arrowLoaded = FALSE;
                wk->arrowDone = FALSE;
            } else {
                wk->arrowPushed = TRUE;
                wk->arrowLoaded = TRUE;
                wk->arrowDone = FALSE;
            }
            break;
        }
        }

        switch (command) {
        case ZUKAN_DETAIL_CMD_NONE:
            break;
        case ZUKAN_DETAIL_CMD_CLOSE:
        case ZUKAN_DETAIL_CMD_RETURN:
            wk->exit = MAP_EXIT_SCREEN;
            break;
        case ZUKAN_DETAIL_CMD_INFO:
        case ZUKAN_DETAIL_CMD_VOICE:
        case ZUKAN_DETAIL_CMD_FORM:
            wk->exit = MAP_EXIT_PAGE;
            break;
        case ZUKAN_DETAIL_CMD_CUR_D:
            if (wk->arrowPushed) {
                wk->arrowDone = TRUE;
                if (wk->arrowLoaded) {
                    wk->arrowPushed = FALSE;
                    ZukanDetailTouchbar_Unlock(touchbar);
                }
            }
            break;
        case ZUKAN_DETAIL_CMD_CUR_U:
            if (wk->arrowPushed) {
                wk->arrowDone = TRUE;
                if (wk->arrowLoaded) {
                    wk->arrowPushed = FALSE;
                    ZukanDetailTouchbar_Unlock(touchbar);
                }
            }
            break;
        case ZUKAN_DETAIL_CMD_CHECK:
            GameData_SetKeyItemRegistration(ZukanDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_MAP,
                                            ZukanDetailTouchbar_GetCheck(touchbar));
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        case ZUKAN_DETAIL_CMD_MAP_RETURN:
            ZukanDetailMap_SetState(param, wk, common, MAP_STATE_CLOSE_START);
            break;
        case ZUKAN_DETAIL_CMD_MAP_PLACE: {
            void *habitatList = func_02010cb8(GameData_GetSaveControl(ZukanDetailCommon_GetGameData(common)));

            func_02010d70(habitatList, TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_ZONE));
            if (wk->bySeason == TRUE) {
                func_02010d80(habitatList, wk->season);
            }
            ZukanDetailTouchbar_SetMapPlaceVisible(touchbar, FALSE);
            ZukanDetailMap_ClearButton(wk);
            wk->exit = MAP_EXIT_SCREEN;
            break;
        }
        default:
            if (unlock) {
                ZukanDetailTouchbar_Unlock(touchbar);
            }
            break;
        }
    }
}

static void ZukanDetailMap_VBlank(TCB *tcb, void *data) {
    ZukanDetailMapWork *wk = data;

    switch (wk->vblankRequest) {
    case VBLANK_DIM:
        ZukanDetailMap_ClearHighlight(wk);
        ZukanDetailMap_SetDimBlend(wk);
        break;
    case VBLANK_HIGHLIGHT:
        ZukanDetailMap_ResetBlend(wk);
        ZukanDetailMap_SetHighlight(wk);
        break;
    case VBLANK_RESET:
        ZukanDetailMap_ClearHighlight(wk);
        ZukanDetailMap_ResetBlend(wk);
        ZukanDetailBlend_SetIn(0, wk->blendMain);
        break;
    }
    wk->vblankRequest = VBLANK_NONE;
    ZukanDetailPalFade_VBlank(wk->palFade);
}

static void ZukanDetailMap_InitPlaceOrder(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                          ZukanDetailCommon *common) {
    u8 i;
    u8 j;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        u16 zone = TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_ZONE);

        for (j = 0; j < TOWNMAP_PLACE_COUNT; j++) {
            if (zone == sZukanDetailMapZones[j]) {
                wk->placeOrder[i] = j;
                break;
            }
        }
    }
}

static void ZukanDetailMap_LoadMapBG(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_TOWNMAP_GRA, param->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG_EX, 0x6000, 0x2000, param->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG_EX, 0x4000, 0x2000, param->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 5, 2, 0, 0x4000, FALSE, param->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 3, 0, 0xc000, FALSE, param->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 17, 2, 0, 0x800, FALSE, param->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 16, 3, 0, 0x800, FALSE, param->heapId);
    GFL_ArcToolFree(arc);
}

static void ZukanDetailMap_FreeMapBG(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
}

static void ZukanDetailMap_CreateActors(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    ClActSurfaceSetup surface = { 0, 0, 256, 192, 0, 0 };
    ArcTool *arc;
    u8 i;

    wk->renderer = func_0204be9c(&surface, 1, param->heapId);
    wk->unit2 = func_0204bf1c(16, 0, param->heapId);
    func_0204c018(wk->unit2, wk->renderer);

    arc = GFL_ArcSysCreateFileHandle(ARCID_TOWNMAP_GRA, param->heapId);
    wk->mapPalette = func_0204bbb8(arc, 2, CLACT_VRAM_MAIN, 0, 0, 5, param->heapId);
    wk->mapChars = func_0204b81c(arc, 8, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->mapCellAnims = func_0204bde0(arc, 12, 19, param->heapId);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, param->heapId);
    wk->seasonPalette = func_0204bbb8(arc, 6, CLACT_VRAM_MAIN, 0xa0, 0, 2, param->heapId);
    wk->seasonChars = func_0204b81c(arc, 16, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->seasonCellAnims = func_0204bde0(arc, 30, 47, param->heapId);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, param->heapId);
    wk->areaPalette = func_0204bbb8(arc, 5, CLACT_VRAM_MAIN, 0xe0, 0, 1, param->heapId);
    wk->areaChars = func_0204b81c(arc, 15, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->areaCellAnims = func_0204bde0(arc, 29, 46, param->heapId);
    GFL_ArcToolFree(arc);

    {
        u8 anims[ACTOR_COUNT] = { 1, 4, 2, 0, 6, 4, 5, 6 };
        u8 bgPriorities[ACTOR_COUNT] = { 2, 2, 2, 2, 0, 0, 0, 0 };
        u8 priorities[ACTOR_COUNT] = { 2, 1, 0, 3, 0, 0, 1, 2 };
        ClActorSetup setup;
        ClActorPos pos;
        ClActorPos buttonPos;

        sys_memset(&setup, 0, sizeof(ClActorSetup));
        for (i = ACTOR_CURSOR; i < ACTOR_COUNT; i++) {
            wk->actors[i] = func_0204c040(wk->unit2, wk->mapChars, wk->mapPalette, wk->mapCellAnims, &setup,
                                          CLACT_SURFACE_MAIN, param->heapId);
        }
        for (i = 0; i < ACTOR_CURSOR; i++) {
            wk->actors[i] = func_0204c040(wk->unit2, wk->seasonChars, wk->seasonPalette, wk->seasonCellAnims, &setup,
                                          CLACT_SURFACE_MAIN, param->heapId);
        }
        for (i = 0; i < ACTOR_COUNT; i++) {
            func_0204c488(wk->actors[i], anims[i]);
            func_0204c520(wk->actors[i], TRUE);
            func_0204c468(wk->actors[i], bgPriorities[i]);
            func_0204c438(wk->actors[i], priorities[i]);
            func_0204c124(wk->actors[i], FALSE);
            func_0204c318(wk->actors[i], 1);
        }
        pos.x = 128;
        pos.y = 96;
        for (i = 0; i < ACTOR_CURSOR; i++) {
            func_0204c140(wk->actors[i], &pos, CLACT_SURFACE_MAIN);
        }
        buttonPos.x = 132;
        buttonPos.y = 176;
        func_0204c140(wk->actors[ACTOR_BUTTON], &buttonPos, CLACT_SURFACE_MAIN);
    }
    wk->arrow = ACTOR_NONE;
}

static void ZukanDetailMap_FreeActors(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c108(wk->actors[i]);
    }
    func_0204be64(wk->areaCellAnims);
    func_0204b98c(wk->areaChars);
    func_0204bcd0(wk->areaPalette);
    func_0204be64(wk->seasonCellAnims);
    func_0204b98c(wk->seasonChars);
    func_0204bcd0(wk->seasonPalette);
    func_0204be64(wk->mapCellAnims);
    func_0204b98c(wk->mapChars);
    func_0204bcd0(wk->mapPalette);
    func_0204bf98(wk->unit2);
    func_0204becc(wk->renderer);
}

static void ZukanDetailMap_CreateAreas(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    ClActorSetup setup;
    u8 i;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        setup.x = TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_AREA_X);
        setup.y = TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_AREA_Y);
        wk->areas[i] = func_0204c040(wk->unit, wk->areaChars, wk->areaPalette, wk->areaCellAnims, &setup,
                                     CLACT_SURFACE_MAIN, param->heapId);
        func_0204c488(wk->areas[i], TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_AREA_ANIM));
        func_0204c520(wk->areas[i], TRUE);
        func_0204c468(wk->areas[i], 2);
        func_0204c438(wk->areas[i], 4);
        func_0204c124(wk->areas[i], FALSE);
        func_0204c318(wk->areas[i], 1);
    }
}

static void ZukanDetailMap_FreeAreas(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        func_0204c108(wk->areas[i]);
    }
}

static inline BOOL ZukanDetailMap_IsPlaceHidden(GameData *gameData, u16 flag) {
    if (func_ov012_02160f74(gameData, flag) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static void ZukanDetailMap_CreatePlaces(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    GameData *gameData = ZukanDetailCommon_GetGameData(common);
    ClActorSetup setup;
    u8 i;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        u16 flag = TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_FLAG);

        if (flag != TOWNMAP_NO_FLAG) {
            if (ZukanDetailMap_IsPlaceHidden(gameData, flag)) {
                wk->places[i].state = PLACE_HIDDEN;
                wk->places[i].actor = NULL;
            } else {
                u16 type = TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_TYPE);

                setup.x = TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_X);
                setup.y = TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_Y);
                setup.sequence = type == TOWNMAP_PLACE_TYPE_TOWN ? 14 : 16;
                setup.priority = 5;
                setup.bgPriority = 2;
                wk->places[i].state = PLACE_SHOWN;
                wk->places[i].actor = func_0204c040(wk->unit2, wk->mapChars, wk->mapPalette, wk->mapCellAnims, &setup,
                                                    CLACT_SURFACE_MAIN, param->heapId);
                func_0204c520(wk->places[i].actor, TRUE);
                func_0204c124(wk->places[i].actor, TRUE);
                func_0204c318(wk->places[i].actor, 1);
            }
        } else {
            wk->places[i].state = PLACE_NONE;
            wk->places[i].actor = NULL;
        }
    }
}

static void ZukanDetailMap_FreePlaces(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (wk->places[i].state == PLACE_SHOWN) {
            func_0204c108(wk->places[i].actor);
        }
    }
}

static void ZukanDetailMap_CreatePrintQueues(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                             ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->printQueues[i] = func_02021998(param->heapId);
        wk->transferPending[i] = FALSE;
    }
}

static void ZukanDetailMap_FreePrintQueues(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                           ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->transferPending[i] = FALSE;
        func_02021c44(wk->printQueues[i]);
        func_02021a18(wk->printQueues[i]);
    }
}

static void ZukanDetailMap_UpdatePrint(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        func_02021a3c(wk->printQueues[i]);
        ZukanDetailMap_TransferWindow(param, wk, common, i);
    }
}

// Loads a window once its text is printed
static void ZukanDetailMap_TransferWindow(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                          u8 window) {
    if (wk->transferPending[window] && !func_02021c1c(wk->printQueues[window], BmpWin_GetBitmap(wk->windows[window]))) {
        BmpWin *bmpWin = wk->windows[window];

        BmpWin_FlushChar(bmpWin);
        BmpWin_FlushMap(bmpWin);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(bmpWin));
        wk->transferPending[window] = FALSE;
    }
}

static void ZukanDetailMap_CreateWindows(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                         ZukanDetailCommon *common) {
    ZukanDetailWindowData bgWindows[2] = {
        { 6, 0, 0, 1, 1, 8, 0 },
        { 0, 0, 0, 1, 1, 0, 0 },
    };
    ZukanDetailWindowData windows[WINDOW_COUNT] = {
        { 6, 12, 21, 16, 3, 8, 0 }, { 6, 9, 25, 16, 2, 8, 0 },  { 0, 2, 0, 15, 2, 0, 0 },
        { 0, 8, 10, 16, 2, 0, 0 },  { 0, 10, 21, 16, 3, 0, 0 },
    };
    u8 i;

    GFL_G2DIOLoadNCLR(ARCID_FONT, 5, PALTYPE_SUB_BG, 0, 8 * 0x20, 0x20, param->heapId);
    GFL_G2DIOLoadNCLR(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0, 0, 0x20, param->heapId);
    for (i = 0; i < 2; i++) {
        wk->bgWindows[i] = BmpWin_CreateDynamic(bgWindows[i].bg, bgWindows[i].x, bgWindows[i].y, bgWindows[i].width,
                                                bgWindows[i].height, bgWindows[i].palette, bgWindows[i].fromEnd);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->bgWindows[i]), 0);
        BmpWin_FlushChar(wk->bgWindows[i]);
    }
    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->windows[i] = BmpWin_CreateDynamic(windows[i].bg, windows[i].x, windows[i].y, windows[i].width,
                                              windows[i].height, windows[i].palette, windows[i].fromEnd);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[i]), 0);
        BmpWin_FlushChar(wk->windows[i]);
    }
    wk->msgData[0] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_SERVER_FLOW_TITLE, param->heapId);
    wk->msgData[1] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PLACE_NAMES, param->heapId);
}

static void ZukanDetailMap_FreeWindows(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        GFL_MsgDataFree(wk->msgData[i]);
    }
    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->transferPending[i] = FALSE;
        func_02021c44(wk->printQueues[i]);
        BmpWin_Free(wk->windows[i]);
    }
    for (i = 0; i < 2; i++) {
        BmpWin_Free(wk->bgWindows[i]);
    }
}

static void ZukanDetailMap_LoadPanelBG(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, param->heapId);

    GFL_G2DIOLoadArcNCLR(arc, 2, PALTYPE_SUB_BG, 0, 0, 8 * 0x20, param->heapId);
    wk->panelChars = GFL_BGSysLoadArcNCGRDynamic(arc, 12, PANEL_BG, 0, FALSE, param->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 39, PANEL_BG, 0, CHAR_POS(wk->panelChars), 0, FALSE, param->heapId);
    GFL_ArcToolFree(arc);
}

static void ZukanDetailMap_FreePanelBG(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    GFL_BGSysFreeCharMemory(PANEL_BG, CHAR_POS(wk->panelChars), CHAR_SIZE(wk->panelChars));
}

static void ZukanDetailMap_UpdateSlide(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    ZukanDetailMap_UpdateMainSlide(param, wk, common);
    ZukanDetailMap_UpdateSubSlide(param, wk, common);
}

// Slides the panel over the map and the Pokémon's icon up, and back
static void ZukanDetailMap_UpdateMainSlide(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                           ZukanDetailCommon *common) {
    switch (wk->panelSlide) {
    case SLIDE_NONE:
        break;
    case SLIDE_IN:
        if (GFL_BGSysGetBGOffsetY(PANEL_BG) >= 64) {
            GFL_BGSysMoveBGReq(PANEL_BG, BG_MOVE_SET_Y, 64);
            wk->panelSlide = SLIDE_DONE;
        } else {
            GFL_BGSysMoveBGReq(PANEL_BG, BG_MOVE_DOWN, 8);
            GFL_BGSysMoveBGReq(6, BG_MOVE_DOWN, 8);
        }
        break;
    case SLIDE_DONE:
        break;
    case SLIDE_OUT:
        if (GFL_BGSysGetBGOffsetY(PANEL_BG) <= 0) {
            GFL_BGSysMoveBGReq(PANEL_BG, BG_MOVE_SET_Y, 0);
            wk->panelSlide = SLIDE_NONE;
        } else {
            GFL_BGSysMoveBGReq(PANEL_BG, BG_MOVE_UP, 8);
            GFL_BGSysMoveBGReq(6, BG_MOVE_UP, 8);
        }
        break;
    }

    if (wk->icons[wk->icon] != NULL) {
        ClActorPos pos;

        switch (wk->iconSlide) {
        case SLIDE_NONE:
            break;
        case SLIDE_IN:
            func_0204c178(wk->icons[wk->icon], &pos, CLACT_SURFACE_SUB);
            if (pos.y <= 112) {
                pos.y = 112;
                wk->iconSlide = SLIDE_DONE;
            } else {
                pos.y -= 8;
            }
            func_0204c140(wk->icons[wk->icon], &pos, CLACT_SURFACE_SUB);
            break;
        case SLIDE_DONE:
            break;
        case SLIDE_OUT:
            func_0204c178(wk->icons[wk->icon], &pos, CLACT_SURFACE_SUB);
            if (pos.y >= 176) {
                pos.y = 176;
                wk->iconSlide = SLIDE_NONE;
            } else {
                pos.y += 8;
            }
            func_0204c140(wk->icons[wk->icon], &pos, CLACT_SURFACE_SUB);
            break;
        }
    }
}

static void ZukanDetailMap_UpdateSubSlide(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                          ZukanDetailCommon *common) {
}

// Once the season arrow's pushed animation has played, shows both arrows again
static void ZukanDetailMap_UpdateArrow(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    u16 anim;
    u8 other;
    u16 otherAnim;

    if (wk->arrow == ACTOR_SEASON_NEXT) {
        anim = 2;
        other = ACTOR_SEASON_PREV;
        otherAnim = 4;
    } else if (wk->arrow == ACTOR_SEASON_PREV) {
        anim = 4;
        other = ACTOR_SEASON_NEXT;
        otherAnim = 2;
    }
    if (wk->arrow != ACTOR_NONE && !func_0204c560(wk->actors[wk->arrow])) {
        func_0204c488(wk->actors[wk->arrow], anim);
        func_0204c520(wk->actors[other], TRUE);
        func_0204c488(wk->actors[other], otherAnim);
        wk->arrow = ACTOR_NONE;
        if (wk->state == MAP_STATE_MAP || wk->state == MAP_STATE_PLACE) {
            ZukanDetailTouchbar_SetActive(touchbar, TRUE);
        }
    }
}

static void ZukanDetailMap_LoadIconResources(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                             ZukanDetailCommon *common) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, param->heapId);

    wk->iconPalette = func_0204bc48(arc, func_02021114(), CLACT_VRAM_SUB, 0, param->heapId);
    wk->iconCellAnims = func_0204bde0(arc, func_02021154(), getOBJTileMapping_SubEng(), param->heapId);
    GFL_ArcToolFree(arc);
}

static void ZukanDetailMap_FreeIconResources(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                             ZukanDetailCommon *common) {
    func_0204be64(wk->iconCellAnims);
    func_0204bcd0(wk->iconPalette);
}

static ClActor *ZukanDetailMap_CreateIcon(u32 *chars, u32 palette, u32 cellAnims, ClActUnit *unit, HeapID heapId,
                                          u32 species, u32 form, u32 sex, BOOL egg, u8 x, u8 y) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, heapId);
    ClActorSetup setup;
    ClActor *actor;

    setup.x = x;
    setup.y = y;
    setup.sequence = 1;
    setup.priority = 1;
    setup.bgPriority = 0;
    *chars = func_0204b81c(arc, PokeParty_GetIconIndex(species, form, sex, egg), FALSE, CLACT_VRAM_SUB, heapId);
    actor = func_0204c040(unit, *chars, palette, cellAnims, &setup, CLACT_SURFACE_SUB, heapId);
    func_0204c520(actor, FALSE);
    func_0204c378(actor, func_02021034(species, form, sex, egg), 0);
    func_0204c318(actor, 0);
    GFL_ArcToolFree(arc);
    return actor;
}

static void ZukanDetailMap_FreeIcon(u32 chars, ClActor *actor) {
    func_0204c108(actor);
    func_0204b98c(chars);
}

static u8 ZukanDetailMap_GetPlayerPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    GameData *gameData = ZukanDetailCommon_GetGameData(common);
    u16 zone = PlayerState_GetZoneID(GameData_GetPlayerState(gameData));
    u8 place = NO_PLACE;
    u16 found = TownMapData_GetPlaceByZone(wk->townmap, func_ov012_02160eb4(gameData, zone));

    if (found != TOWNMAP_PLACE_NONE) {
        place = found;
    }
    return place;
}

static void ZukanDetailMap_InitPlayerMarker(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                            ZukanDetailCommon *common) {
    u8 place = ZukanDetailMap_GetPlayerPlace(param, wk, common);

    if (place != NO_PLACE) {
        ClActorPos pos;

        pos.x = TownMapData_GetParam(wk->townmap, place, TOWNMAP_PARAM_X);
        pos.y = TownMapData_GetParam(wk->townmap, place, TOWNMAP_PARAM_Y);
        func_0204c140(wk->actors[ACTOR_PLAYER], &pos, CLACT_SURFACE_MAIN);
    }
}

static void ZukanDetailMap_SetPlayerMarkerVisible(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                                  ZukanDetailCommon *common, BOOL visible) {
    if (visible) {
        if (ZukanDetailMap_GetPlayerPlace(param, wk, common) != NO_PLACE) {
            func_0204c488(wk->actors[ACTOR_PLAYER], 6);
            func_0204c124(wk->actors[ACTOR_PLAYER], TRUE);
        }
    } else {
        func_0204c124(wk->actors[ACTOR_PLAYER], FALSE);
    }
}

static BOOL ZukanDetailMap_IsPlayerMarkerVisible(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                                 ZukanDetailCommon *common) {
    return func_0204c138(wk->actors[ACTOR_PLAYER]);
}

// Shows the cursor at the picked place, or at the player's, or else at the middle of the map
static void ZukanDetailMap_ResetCursor(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    ClActorPos pos = { 128, 96 };

    if (wk->place == NO_PLACE) {
        wk->place = ZukanDetailMap_GetPlayerPlace(param, wk, common);
    }
    if (wk->place != NO_PLACE) {
        pos.x = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_X);
        pos.y = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_Y);
    }
    func_0204c140(wk->actors[ACTOR_CURSOR], &pos, CLACT_SURFACE_MAIN);
    func_0204c124(wk->actors[ACTOR_CURSOR], TRUE);
}

static void ZukanDetailMap_SelectPlayerPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                             ZukanDetailCommon *common) {
    wk->place = ZukanDetailMap_GetPlayerPlace(param, wk, common);
    ZukanDetailMap_ResetCursor(param, wk, common);
}

// The shown place whose capsule a touch at x, y hits, the nearest if several do, or NO_PLACE
static u8 ZukanDetailMap_FindTouchedPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                          u8 x, u8 y) {
    CalcSphere sphere;
    VecFx32 center;
    CalcCapsule capsule;
    VecFx32 start;
    VecFx32 end;
    CalcHitResult result;
    fx32 minDist;
    u8 place = NO_PLACE;
    u8 i;

    center.x = FX32_CONST(x);
    center.y = FX32_CONST(y);
    center.z = 0;
    CalcSphere_Set(&sphere, &center, 0);
    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (wk->places[i].state != PLACE_HIDDEN) {
            start.x = FX32_CONST(TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_HIT_START_X));
            start.y = FX32_CONST(TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_HIT_START_Y));
            start.z = 0;
            end.x = FX32_CONST(TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_HIT_END_X));
            end.y = FX32_CONST(TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_HIT_END_Y));
            end.z = 0;
            CalcCapsule_Set(&capsule, &start, &end,
                            FX32_CONST(TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_HIT_RADIUS)));
            if (CalcCapsule_HitSphere(&capsule, &sphere, &result)) {
                if (place == NO_PLACE) {
                    minDist = result.dist;
                    place = i;
                } else if (minDist > result.dist) {
                    minDist = result.dist;
                    place = i;
                }
            }
        }
    }
    return place;
}

// Whether a shown place's cursor point is within 12 pixels of x, y, and the square of the distance
static BOOL ZukanDetailMap_IsNearPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                       u8 place, u8 x, u8 y, u32 *distSq) {
    u16 placeX;
    u16 placeY;
    u32 dist;

    if (wk->places[place].state == PLACE_HIDDEN) {
        return FALSE;
    }
    placeX = TownMapData_GetParam(wk->townmap, place, TOWNMAP_PARAM_CURSOR_X);
    placeY = TownMapData_GetParam(wk->townmap, place, TOWNMAP_PARAM_CURSOR_Y);
    dist = (placeX - x) * (placeX - x) + (placeY - y) * (placeY - y);
    if (dist < 12 * 12) {
        if (distSq != NULL) {
            *distSq = dist;
        }
        return TRUE;
    }
    return FALSE;
}

static u8 ZukanDetailMap_FindNearestPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                          u8 x, u8 y) {
    u8 place = NO_PLACE;
    u32 minDist;
    u32 dist;
    u8 i;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (ZukanDetailMap_IsNearPlace(param, wk, common, i, x, y, &dist)) {
            if (place == NO_PLACE) {
                place = i;
                minDist = dist;
            } else if (minDist > dist) {
                place = i;
                minDist = dist;
            }
        }
    }
    return place;
}

// On the map, A or a touch on it opens the panel to pick a place, and L and R or the arrows change the season. On the
// panel, B closes it, and the rest moves the cursor
static void ZukanDetailMap_HandleInput(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    PokeDexSave *pokedex = GameData_GetPokedex(ZukanDetailCommon_GetGameData(common));

    if (!wk->inputEnabled || wk->arrow != ACTOR_NONE) {
        return;
    }
    if (ZukanDetailTouchbar_IsMapPlaceTriggered(touchbar) == TRUE) {
        // The touch bar opens the habitat list itself
        PokeDex_IsHabitatListEnabled(pokedex);
        return;
    }

    switch (wk->state) {
    case MAP_STATE_MAP: {
        BOOL handled = FALSE;
        BOOL open = FALSE;

        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            open = TRUE;
            handled = TRUE;
        } else if (wk->bySeason) {
            if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
                wk->season++;
                if (wk->season >= SEASON_COUNT) {
                    wk->season = 0;
                }
                ZukanDetailMap_ChangeSeason(param, wk, common, TRUE, FALSE);
                GFL_SndSEPlay(SEQ_SE_SELECT3);
                handled = TRUE;
            } else if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
                if (wk->season == 0) {
                    wk->season = SEASON_COUNT - 1;
                } else {
                    wk->season--;
                }
                ZukanDetailMap_ChangeSeason(param, wk, common, FALSE, FALSE);
                GFL_SndSEPlay(SEQ_SE_SELECT3);
                handled = TRUE;
            }
        }
        if (handled) {
            func_0203d564(FALSE);
            if (open) {
                wk->touched = FALSE;
            }
        }
        if (!handled) {
            u32 x;
            u32 y;

            if (func_0203dac8(&x, &y)) {
                if (!wk->bySeason) {
                    if (y < 168) {
                        open = TRUE;
                        handled = TRUE;
                    }
                } else if (x >= 136 && x < 160 && y < 16) {
                    wk->season++;
                    if (wk->season >= SEASON_COUNT) {
                        wk->season = 0;
                    }
                    handled = TRUE;
                    ZukanDetailMap_ChangeSeason(param, wk, common, TRUE, TRUE);
                    GFL_SndSEPlay(SEQ_SE_SELECT3);
                } else if (x < 24 && y < 16) {
                    if (wk->season == 0) {
                        wk->season = SEASON_COUNT - 1;
                    } else {
                        wk->season--;
                    }
                    handled = TRUE;
                    ZukanDetailMap_ChangeSeason(param, wk, common, FALSE, TRUE);
                    GFL_SndSEPlay(SEQ_SE_SELECT3);
                } else if ((x >= 160 || y >= 24) && y < 168) {
                    open = TRUE;
                    handled = TRUE;
                }
            }
            if (handled) {
                func_0203d564(TRUE);
                if (open) {
                    wk->touched = TRUE;
                    wk->touchX = x;
                    wk->touchY = y;
                }
            }
        }
        if (open && (wk->bySeason || wk->found)) {
            ZukanDetailMap_SetState(param, wk, common, MAP_STATE_OPEN_START);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ZukanDetailTouchbar_SetActive(touchbar, FALSE);
        }
        break;
    }
    case MAP_STATE_PLACE: {
        BOOL handled = FALSE;
        BOOL move = TRUE;

        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            wk->inputEnabled = FALSE;
            handled = TRUE;
            move = FALSE;
        } else if (wk->bySeason) {
            if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
                wk->season++;
                if (wk->season >= SEASON_COUNT) {
                    wk->season = 0;
                }
                move = FALSE;
                ZukanDetailMap_ChangeSeason(param, wk, common, TRUE, FALSE);
                GFL_SndSEPlay(SEQ_SE_SELECT3);
                handled = TRUE;
            } else if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
                if (wk->season == 0) {
                    wk->season = SEASON_COUNT - 1;
                } else {
                    wk->season--;
                }
                move = FALSE;
                ZukanDetailMap_ChangeSeason(param, wk, common, FALSE, FALSE);
                GFL_SndSEPlay(SEQ_SE_SELECT3);
                handled = TRUE;
            }
        }
        if (handled) {
            func_0203d564(FALSE);
        }
        if (!handled) {
            u32 x;
            u32 y;

            if (func_0203dac8(&x, &y)) {
                if (wk->bySeason == FALSE) {
                    if (y >= 168) {
                        move = FALSE;
                    }
                } else if (wk->bySeason == TRUE) {
                    if (x >= 136 && x < 160 && y < 16) {
                        wk->season++;
                        if (wk->season >= SEASON_COUNT) {
                            wk->season = 0;
                        }
                        handled = TRUE;
                        ZukanDetailMap_ChangeSeason(param, wk, common, TRUE, TRUE);
                        GFL_SndSEPlay(SEQ_SE_SELECT3);
                        move = FALSE;
                    } else if (x < 24 && y < 16) {
                        if (wk->season == 0) {
                            wk->season = SEASON_COUNT - 1;
                        } else {
                            wk->season--;
                        }
                        handled = TRUE;
                        ZukanDetailMap_ChangeSeason(param, wk, common, FALSE, TRUE);
                        move = FALSE;
                        GFL_SndSEPlay(SEQ_SE_SELECT3);
                    } else if (x < 160 && y < 24) {
                        move = FALSE;
                    } else if (y >= 168) {
                        move = FALSE;
                    }
                }
            }
            if (handled) {
                func_0203d564(TRUE);
            }
        }
        if (move) {
            ZukanDetailMap_MoveCursor(param, wk, common);
        }
        break;
    }
    }
}

// The D-pad moves the cursor, drawn to the nearest place and snapping onto it, and a touch picks the place touched,
// which a second touch confirms
static void ZukanDetailMap_MoveCursor(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    u32 x;
    u32 y;

    if (!wk->found) {
        return;
    }
    if (!func_0203d554()) {
        if (func_0203dac8(&x, &y)) {
            func_0203d564(TRUE);
            func_0204c124(wk->actors[ACTOR_CURSOR], FALSE);
        }
    } else if (GCTX_HIDGetPressedKeys()) {
        func_0203d564(FALSE);
        ZukanDetailMap_ResetCursor(param, wk, common);
        ZukanDetailMap_PrintPlace(param, wk, common);
        ZukanDetailMap_UpdateKinds(param, wk, common);
        return;
    }

    if (!func_0203d554()) {
        u8 prevPlace = wk->place;
        BOOL moved = FALSE;
        BOOL nearFound = FALSE;
        BOOL snap = FALSE;
        VecFx32 dir = { 0, 0, 0 };
        VecFx32 toPlace = { 0, 0, 0 };
        VecFx32 placeVec;
        VecFx32 cursorVec;
        ClActorPos cursorPos;
        ClActorPos placePos;
        u8 nearPlace;
        u32 held;

        func_0204c178(wk->actors[ACTOR_CURSOR], &cursorPos, CLACT_SURFACE_MAIN);
        held = GCTX_HIDGetHeldKeys();
        if ((held & PAD_KEY_RIGHT) || (held & PAD_KEY_LEFT) || (held & PAD_KEY_UP) || (held & PAD_KEY_DOWN)) {
            moved = TRUE;
            if (held & PAD_KEY_RIGHT) {
                dir.x += FX32_ONE;
            }
            if (held & PAD_KEY_LEFT) {
                dir.x -= FX32_ONE;
            }
            if (held & PAD_KEY_UP) {
                dir.y -= FX32_ONE;
            }
            if (held & PAD_KEY_DOWN) {
                dir.y += FX32_ONE;
            }
        }
        nearPlace = ZukanDetailMap_FindNearestPlace(param, wk, common, cursorPos.x, cursorPos.y);
        if (nearPlace != NO_PLACE) {
            nearFound = TRUE;
            placePos.x = TownMapData_GetParam(wk->townmap, nearPlace, TOWNMAP_PARAM_CURSOR_X);
            placePos.y = TownMapData_GetParam(wk->townmap, nearPlace, TOWNMAP_PARAM_CURSOR_Y);
            placeVec.x = FX32_CONST(placePos.x);
            placeVec.y = FX32_CONST(placePos.y);
            placeVec.z = 0;
            cursorVec.x = FX32_CONST(cursorPos.x);
            cursorVec.y = FX32_CONST(cursorPos.y);
            cursorVec.z = 0;
            VEC_Subtract(&placeVec, &cursorVec, &toPlace);
            if (VEC_Mag(&toPlace) < FX32_CONST(1.732f)) {
                snap = TRUE;
            }
            if (VEC_Mag(&toPlace) > FX32_ONE) {
                vecfx_normalize(&toPlace, &toPlace);
                vecfx_mul(&toPlace, FX32_ONE, &toPlace);
            }
        }
        if (!moved && !nearFound) {
            return;
        }
        if (!moved && nearFound && snap) {
            cursorPos = placePos;
        } else {
            fx32 dx;
            fx32 dy;

            vecfx_normalize(&dir, &dir);
            dx = toPlace.x + dir.x * 3;
            dy = toPlace.y + dir.y * 3;
            if (dx > 0 && dx < FX32_ONE) {
                dx = FX32_ONE;
            } else if (dx > -FX32_ONE && dx < 0) {
                dx = -FX32_ONE;
            }
            if (dy > 0 && dy < FX32_ONE) {
                dy = FX32_ONE;
            } else if (dy > -FX32_ONE && dy < 0) {
                dy = -FX32_ONE;
            }
            cursorPos.x += (s16)(dx >> FX32_SHIFT);
            cursorPos.y += (s16)(dy >> FX32_SHIFT);
        }
        if (cursorPos.x > 248) {
            cursorPos.x = 248;
        } else if (cursorPos.x < 0) {
            cursorPos.x = 0;
        }
        if (cursorPos.y > 168) {
            cursorPos.y = 168;
        } else if (cursorPos.y < 8) {
            cursorPos.y = 8;
        }
        func_0204c140(wk->actors[ACTOR_CURSOR], &cursorPos, CLACT_SURFACE_MAIN);
        wk->place = ZukanDetailMap_FindTouchedPlace(param, wk, common, cursorPos.x, cursorPos.y);
        if (wk->place != prevPlace) {
            ZukanDetailMap_PrintPlace(param, wk, common);
            ZukanDetailMap_UpdateKinds(param, wk, common);
            if (wk->place != NO_PLACE) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        }
    } else if (func_0203dac8(&x, &y)) {
        u8 prevPlace = wk->place;

        wk->place = ZukanDetailMap_FindTouchedPlace(param, wk, common, x, y);
        ZukanDetailMap_PrintPlace(param, wk, common);
        ZukanDetailMap_UpdateKinds(param, wk, common);
        if (prevPlace == wk->place) {
            ZukanDetailTouchbar_PushMapPlace(ZukanDetailCommon_GetTouchbar(common));
            return;
        }
        if (wk->place != NO_PLACE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
    }
}

// Picks the kind of habitat for the habitat list of the picked place, and returns whether it has one the player can
// see there
static BOOL ZukanDetailMap_SetHabitatKind(ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    // BUG: With no place picked, this reads past placeOrder. The value is not used then
#ifdef BUGFIX
    u8 order = wk->place != NO_PLACE ? wk->placeOrder[wk->place] : 0;
#else
    u8 order = wk->placeOrder[wk->place];
#endif

    if (wk->found == TRUE && wk->place != NO_PLACE) {
        int season = wk->bySeason == FALSE ? 0 : wk->season;
        u8 kinds = wk->habitat->seasons[season].places[order];

        if (kinds != 0 && ZukanDetailMap_IsPlaceListed(wk, common) == TRUE) {
            GameData *gameData = ZukanDetailCommon_GetGameData(common);
            void *habitatList = func_02010cb8(GameData_GetSaveControl(gameData));
            EventWork *eventWork = GameData_GetEventWork(gameData);

            if (kinds & HABITAT_LAND) {
                func_02010d90(habitatList, 0);
                return TRUE;
            }
            if ((kinds & HABITAT_WATER) && EventWork_FlagGet(eventWork, FLAG_HABITAT_WATER) == TRUE) {
                func_02010d90(habitatList, 1);
                return TRUE;
            }
            if ((kinds & HABITAT_FISHING) && EventWork_FlagGet(eventWork, FLAG_HABITAT_FISHING) == TRUE) {
                func_02010d90(habitatList, 2);
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void ZukanDetailMap_UpdateState(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    switch (wk->state) {
    case MAP_STATE_MAP:
        break;
    case MAP_STATE_OPEN_START:
        ZukanDetailMap_SetState(param, wk, common, MAP_STATE_OPEN);
        break;
    case MAP_STATE_OPEN:
        ZukanDetailMap_SetState(param, wk, common, MAP_STATE_PLACE);
        break;
    case MAP_STATE_CHANGE:
        switch (wk->step) {
        case 1:
            ZukanDetailMap_HideHabitat(param, wk, common);
            wk->step = 2;
            break;
        case 2:
            ZukanDetailMap_RequestHabitatBlend(param, wk, common);
            wk->step = 3;
            break;
        case 3:
            ZukanDetailMap_ShowAreas(param, wk, common);
            wk->step = 0;
            ZukanDetailMap_SetState(param, wk, common, MAP_STATE_MAP);
            break;
        }
        break;
    case MAP_STATE_EXIT:
        switch (wk->step) {
        case 1:
            ZukanDetailMap_HideAreas(param, wk, common);
            wk->step = 2;
            break;
        case 2:
            ZukanDetailMap_RequestReset(param, wk, common);
            wk->step = 3;
            break;
        case 3:
            ZukanDetailMap_FinishExit(param, wk, common);
            wk->step = 0;
            break;
        }
        break;
    case MAP_STATE_PLACE:
        break;
    case MAP_STATE_CLOSE_START:
        ZukanDetailMap_SetState(param, wk, common, MAP_STATE_CLOSE);
        break;
    case MAP_STATE_CLOSE:
        ZukanDetailMap_SetState(param, wk, common, MAP_STATE_MAP);
        break;
    case MAP_STATE_PLACE_CHANGE:
        switch (wk->step) {
        case 1:
            ZukanDetailMap_HideHabitat(param, wk, common);
            wk->step = 2;
            break;
        case 2:
            ZukanDetailMap_RequestHabitatBlend(param, wk, common);
            wk->step = 3;
            break;
        case 3:
            ZukanDetailMap_ShowAreas(param, wk, common);
            wk->step = 0;
            ZukanDetailMap_SetState(param, wk, common, MAP_STATE_PLACE);
            break;
        }
        break;
    }
}

static void ZukanDetailMap_SetState(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                    int state) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);

    switch (wk->state) {
    case MAP_STATE_MAP:
        if (state == MAP_STATE_OPEN_START) {
        } else if (state == MAP_STATE_CHANGE) {
            wk->step = 1;
        } else if (state == MAP_STATE_EXIT) {
            wk->step = 1;
        }
        break;
    case MAP_STATE_OPEN_START:
        if (state == MAP_STATE_OPEN) {
            ZukanDetailTouchbar_SetVisibleAll(touchbar, FALSE);
        }
        break;
    case MAP_STATE_OPEN:
        if (state == MAP_STATE_PLACE) {
            wk->panelSlide = SLIDE_IN;
            wk->iconSlide = SLIDE_IN;
            if (wk->found == TRUE) {
                if (!wk->touched) {
                    ZukanDetailMap_SelectPlayerPlace(param, wk, common);
                } else {
                    wk->place = ZukanDetailMap_FindTouchedPlace(param, wk, common, wk->touchX, wk->touchY);
                    if (wk->place != NO_PLACE) {
                        func_0204c124(wk->actors[ACTOR_PLACE], TRUE);
                    }
                }
                ZukanDetailMap_PrintPlace(param, wk, common);
                ZukanDetailMap_UpdateKinds(param, wk, common);
            }
            ZukanDetailTouchbar_SetType(touchbar, ZUKAN_DETAIL_TOUCHBAR_MAP, ZUKAN_DETAIL_PAGE_MAP - 1,
                                        ZukanDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
            wk->place = NO_PLACE;
            ZukanDetailMap_UpdateButton(param, wk, common, TRUE);
        }
        break;
    case MAP_STATE_CHANGE:
        if (state == MAP_STATE_MAP) {
            if (wk->arrowPushed) {
                wk->arrowLoaded = TRUE;
                if (wk->arrowDone) {
                    wk->arrowPushed = FALSE;
                    ZukanDetailTouchbar_Unlock(touchbar);
                }
            }
            if (wk->arrow == ACTOR_NONE && wk->active) {
                ZukanDetailTouchbar_SetActive(touchbar, TRUE);
            }
        }
        break;
    case MAP_STATE_EXIT:
        break;
    case MAP_STATE_PLACE:
        if (state == MAP_STATE_CLOSE_START) {
        } else if (state == MAP_STATE_PLACE_CHANGE) {
            wk->step = 1;
        }
        break;
    case MAP_STATE_CLOSE_START:
        if (state == MAP_STATE_CLOSE) {
            ZukanDetailTouchbar_SetVisibleAll(touchbar, FALSE);
            ZukanDetailMap_UpdateButton(param, wk, common, FALSE);
            ZukanDetailMap_ClearButton(wk);
        }
        break;
    case MAP_STATE_CLOSE:
        if (state == MAP_STATE_MAP) {
            BOOL showArrows;

            wk->iconSlide = SLIDE_OUT;
            wk->panelSlide = SLIDE_OUT;
            func_0204c124(wk->actors[ACTOR_CURSOR], FALSE);
            func_0204c124(wk->actors[ACTOR_PLACE], FALSE);
            showArrows = FALSE;
            if (ZukanDetailCommon_GetCount(common) > 1) {
                showArrows = TRUE;
            }
            ZukanDetailTouchbar_SetType(touchbar, ZUKAN_DETAIL_TOUCHBAR_GENERAL, ZUKAN_DETAIL_PAGE_MAP - 1, showArrows);
            ZukanDetailTouchbar_SetCheck(
                touchbar, GameData_IsShortcutRegistered(ZukanDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_MAP));
        }
        break;
    case MAP_STATE_PLACE_CHANGE:
        if (state == MAP_STATE_PLACE) {
            ZukanDetailTouchbar_Unlock(touchbar);
            if (wk->arrow == ACTOR_NONE && wk->active) {
                ZukanDetailTouchbar_SetActive(touchbar, TRUE);
            }
        }
        break;
    }
    wk->state = state;
}

// Shows the Pokémon the list is at: its icon slides in on the sub screen, with its name
static void ZukanDetailMap_LoadPokemon(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    BOOL wasBySeason = wk->bySeason;
    u16 species = ZukanDetailCommon_GetSpecies(common);
    WordSet *wordSet;
    StrBuf *format;
    StrBuf *name;
    u32 sex;
    u32 rare;
    u32 form;

    ZukanDetailMap_FreeHabitat(wk->habitat);
    wk->habitat = ZukanDetailMap_LoadHabitat(species, param->heapId, wk->habitatArc);
    wk->bySeason = wk->habitat->allYear == FALSE ? TRUE : FALSE;
    if (!wk->bySeason) {
        wk->found = wk->habitat->seasons[0].none == FALSE ? TRUE : FALSE;
    } else {
        wk->found = wk->habitat->seasons[wk->season].none == FALSE ? TRUE : FALSE;
    }

    if (wk->icons[wk->icon] != NULL) {
        func_0204c124(wk->icons[wk->icon], FALSE);
    }
    wk->icon = (wk->icon + 1) % 2;
    if (wk->icons[wk->icon] != NULL) {
        // The same release as the exit's, as if from a helper
        if (wk->icons[wk->icon] != NULL) {
            ZukanDetailMap_FreeIcon(wk->iconChars[wk->icon], wk->icons[wk->icon]);
        }
        wk->icons[wk->icon] = NULL;
    }
    func_0200d3c8(GameData_GetPokedex(ZukanDetailCommon_GetGameData(common)), species, &sex, &rare, &form,
                  param->heapId);
    wk->icons[wk->icon] = ZukanDetailMap_CreateIcon(&wk->iconChars[wk->icon], wk->iconPalette, wk->iconCellAnims,
                                                    wk->unit, param->heapId, species, form, sex, FALSE, 56, 176);

    wk->transferPending[WINDOW_NAME] = FALSE;
    func_02021c44(wk->printQueues[WINDOW_NAME]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_NAME]), 0);
    wordSet = GFL_WordSetSystemCreateDefault(param->heapId);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], BtlServerFlowTitle_Text_SArea);
    name = GFL_StrBufCreate(64, param->heapId);
    WordSet_LoadSpeciesName(wordSet, 0, species);
    GFL_WordSetFormatStrbuf(wordSet, name, format);
    GFL_StrBufFree(format);
    GFL_WordSetSystemFree(wordSet);
    func_02021c7c(wk->printQueues[WINDOW_NAME], BmpWin_GetBitmap(wk->windows[WINDOW_NAME]), 0, 5, name, wk->font,
                  PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(name);
    wk->transferPending[WINDOW_NAME] = TRUE;
    ZukanDetailMap_TransferWindow(param, wk, common, WINDOW_NAME);

    if (!wk->bySeason) {
        if (wasBySeason == TRUE) {
            wk->transferPending[WINDOW_SEASON] = FALSE;
            func_02021c44(wk->printQueues[WINDOW_SEASON]);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_SEASON]), 0);
            BmpWin_FlushChar(wk->windows[WINDOW_SEASON]);
            func_0204c124(wk->actors[ACTOR_SEASON], FALSE);
            func_0204c124(wk->actors[ACTOR_SEASON_NEXT], FALSE);
            func_0204c124(wk->actors[ACTOR_SEASON_PREV], FALSE);
        }
    } else if (wasBySeason == FALSE) {
        ZukanDetailMap_PrintSeason(param, wk, common);
        func_0204c124(wk->actors[ACTOR_SEASON], TRUE);
        func_0204c488(wk->actors[ACTOR_SEASON_NEXT], 2);
        func_0204c124(wk->actors[ACTOR_SEASON_NEXT], TRUE);
        func_0204c488(wk->actors[ACTOR_SEASON_PREV], 4);
        func_0204c124(wk->actors[ACTOR_SEASON_PREV], TRUE);
    }
}

static void ZukanDetailMap_ShowHabitat(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    ZukanDetailMap_StartChange(param, wk, common);
    ZukanDetailMap_UpdateKinds(param, wk, common);
}

static void ZukanDetailMap_StartExit(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    ZukanDetailMap_SetState(param, wk, common, MAP_STATE_EXIT);
}

// Prints the picked place's name, and marks it on the map
static void ZukanDetailMap_PrintPlace(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    wk->transferPending[WINDOW_PLACE] = FALSE;
    func_02021c44(wk->printQueues[WINDOW_PLACE]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_PLACE]), 0);
    if (wk->place != NO_PLACE) {
        StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(
            wk->msgData[1], ZoneData_GetPlaceNameID(TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_ZONE)));
        ClActorPos pos;

        func_02021c7c(wk->printQueues[WINDOW_PLACE], BmpWin_GetBitmap(wk->windows[WINDOW_PLACE]), 0, 1, strbuf,
                      wk->font, PRINT_COLOR(15, 2, 0));
        GFL_StrBufFree(strbuf);
        wk->transferPending[WINDOW_PLACE] = TRUE;
        ZukanDetailMap_TransferWindow(param, wk, common, WINDOW_PLACE);
        pos.x = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_X);
        pos.y = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_Y);
        func_0204c488(wk->actors[ACTOR_PLACE], 5);
        func_0204c140(wk->actors[ACTOR_PLACE], &pos, CLACT_SURFACE_MAIN);
        func_0204c124(wk->actors[ACTOR_PLACE], TRUE);
    } else {
        BmpWin_FlushChar(wk->windows[WINDOW_PLACE]);
        func_0204c124(wk->actors[ACTOR_PLACE], FALSE);
    }
}

static void ZukanDetailMap_ChangeSeason(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                        BOOL next, BOOL touch) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    u16 anim;
    u8 other;

    if (!wk->bySeason) {
        wk->found = wk->habitat->seasons[0].none == FALSE ? TRUE : FALSE;
    } else {
        wk->found = wk->habitat->seasons[wk->season].none == FALSE ? TRUE : FALSE;
    }
    wk->transferPending[WINDOW_SEASON] = FALSE;
    func_02021c44(wk->printQueues[WINDOW_SEASON]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_SEASON]), 0);
    ZukanDetailMap_PrintSeason(param, wk, common);
    if (next) {
        wk->arrow = ACTOR_SEASON_NEXT;
        anim = 3;
        other = ACTOR_SEASON_PREV;
    } else {
        wk->arrow = ACTOR_SEASON_PREV;
        anim = 5;
        other = ACTOR_SEASON_NEXT;
    }
    func_0204c488(wk->actors[wk->arrow], anim);
    func_0204c520(wk->actors[other], FALSE);
    func_0204c4d4(wk->actors[other], 0);
    ZukanDetailTouchbar_SetActive(touchbar, FALSE);
    if (touch == TRUE) {
        func_0204c124(wk->actors[ACTOR_CURSOR], FALSE);
    } else if (wk->state == MAP_STATE_PLACE && wk->found == TRUE) {
        ZukanDetailMap_ResetCursor(param, wk, common);
        ZukanDetailMap_PrintPlace(param, wk, common);
    }
    ZukanDetailMap_StartChange(param, wk, common);
    ZukanDetailMap_UpdateKinds(param, wk, common);
}

static ZukanHabitat *ZukanDetailMap_LoadHabitat(u16 species, HeapID heapId, ArcTool *arc) {
    u32 size;

    return GFL_ArcToolReadHeapNewLZGetLen(arc, species - 1, FALSE, heapId, &size);
}

static void ZukanDetailMap_FreeHabitat(ZukanHabitat *habitat) {
    if (habitat != NULL) {
        GFL_HeapFree(habitat);
    }
}

static void ZukanDetailMap_PrintSeason(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], BtlServerFlowTitle_Text_Spring + wk->season);
    u16 width = GFL_FontGetBlockWidth(strbuf, wk->font, 0);
    int windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[WINDOW_SEASON]));
    u16 x = (windowWidth - width) / 2;

    func_02021c7c(wk->printQueues[WINDOW_SEASON], BmpWin_GetBitmap(wk->windows[WINDOW_SEASON]), x, 1, strbuf, wk->font,
                  PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(strbuf);
    wk->transferPending[WINDOW_SEASON] = TRUE;
    ZukanDetailMap_TransferWindow(param, wk, common, WINDOW_SEASON);
}

static void ZukanDetailMap_StartChange(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    if (wk->state == MAP_STATE_MAP) {
        ZukanDetailMap_SetState(param, wk, common, MAP_STATE_CHANGE);
    } else if (wk->state == MAP_STATE_PLACE) {
        ZukanDetailMap_SetState(param, wk, common, MAP_STATE_PLACE_CHANGE);
    }
}

static void ZukanDetailMap_HideHabitat(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    u8 i;

    if (func_0204c138(wk->actors[ACTOR_UNKNOWN])) {
        if (wk->found == TRUE) {
            wk->transferPending[WINDOW_UNKNOWN] = FALSE;
            func_02021c44(wk->printQueues[WINDOW_UNKNOWN]);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_UNKNOWN]), 0);
            BmpWin_FlushChar(wk->windows[WINDOW_UNKNOWN]);
            func_0204c124(wk->actors[ACTOR_UNKNOWN], FALSE);
        }
    } else {
        for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
            func_0204c124(wk->areas[i], FALSE);
        }
        if (wk->found == FALSE) {
            ZukanDetailMap_SetPlayerMarkerVisible(param, wk, common, FALSE);
            wk->place = NO_PLACE;
            func_0204c124(wk->actors[ACTOR_CURSOR], FALSE);
            func_0204c124(wk->actors[ACTOR_PLACE], FALSE);
            ZukanDetailMap_PrintPlace(param, wk, common);
            ZukanDetailMap_UpdateKinds(param, wk, common);
        }
    }
}

static void ZukanDetailMap_RequestHabitatBlend(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                               ZukanDetailCommon *common) {
    if (!func_0204c138(wk->actors[ACTOR_UNKNOWN])) {
        if (wk->found == FALSE) {
            wk->vblankRequest = VBLANK_DIM;
        } else {
            wk->vblankRequest = VBLANK_HIGHLIGHT;
        }
    }
}

static void ZukanDetailMap_ShowAreas(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    BOOL unknownShown = func_0204c138(wk->actors[ACTOR_UNKNOWN]);

    if (wk->found == FALSE) {
        if (!unknownShown) {
            StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], BtlServerFlowTitle_Text_AreaUnknown);
            u16 width = GFL_FontGetBlockWidth(strbuf, wk->font, 0);
            int windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[WINDOW_UNKNOWN]));
            u16 x = (windowWidth - width) / 2;

            func_02021c7c(wk->printQueues[WINDOW_UNKNOWN], BmpWin_GetBitmap(wk->windows[WINDOW_UNKNOWN]), x, 1, strbuf,
                          wk->font, PRINT_COLOR(15, 2, 0));
            GFL_StrBufFree(strbuf);
            wk->transferPending[WINDOW_UNKNOWN] = TRUE;
            ZukanDetailMap_TransferWindow(param, wk, common, WINDOW_UNKNOWN);
            func_0204c124(wk->actors[ACTOR_UNKNOWN], TRUE);
        }
    } else {
        u8 season = wk->bySeason == FALSE ? 0 : wk->season;
        u8 i;

        for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
            if (wk->habitat->seasons[season].places[wk->placeOrder[i]] != 0 && wk->places[i].state != PLACE_HIDDEN) {
                func_0204c124(wk->areas[i], TRUE);
            }
        }
        if (!ZukanDetailMap_IsPlayerMarkerVisible(param, wk, common)) {
            ZukanDetailMap_SetPlayerMarkerVisible(param, wk, common, TRUE);
        }
    }
}

static void ZukanDetailMap_PrintButton(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                       BOOL active) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], BtlServerFlowTitle_Text_ViewDetails);

    if (active == TRUE) {
        func_02021c7c(wk->printQueues[WINDOW_BUTTON], BmpWin_GetBitmap(wk->windows[WINDOW_BUTTON]), 0, 5, strbuf,
                      wk->font, PRINT_COLOR(15, 2, 0));
    } else {
        func_02021c7c(wk->printQueues[WINDOW_BUTTON], BmpWin_GetBitmap(wk->windows[WINDOW_BUTTON]), 0, 5, strbuf,
                      wk->font, PRINT_COLOR(2, 1, 0));
    }
    GFL_StrBufFree(strbuf);
    wk->transferPending[WINDOW_BUTTON] = TRUE;
    ZukanDetailMap_TransferWindow(param, wk, common, WINDOW_BUTTON);
}

static void ZukanDetailMap_ClearButton(ZukanDetailMapWork *wk) {
    func_02021c44(wk->printQueues[WINDOW_BUTTON]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_BUTTON]), 0);
    BmpWin_FlushChar(wk->windows[WINDOW_BUTTON]);
    wk->transferPending[WINDOW_BUTTON] = TRUE;
}

static void ZukanDetailMap_HideAreas(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    u8 i;

    if (func_0204c138(wk->actors[ACTOR_UNKNOWN])) {
        wk->transferPending[WINDOW_UNKNOWN] = FALSE;
        func_02021c44(wk->printQueues[WINDOW_UNKNOWN]);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_UNKNOWN]), 0);
        BmpWin_FlushChar(wk->windows[WINDOW_UNKNOWN]);
        func_0204c124(wk->actors[ACTOR_UNKNOWN], FALSE);
    } else {
        for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
            func_0204c124(wk->areas[i], FALSE);
        }
    }
}

static void ZukanDetailMap_RequestReset(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    wk->vblankRequest = VBLANK_RESET;
}

static void ZukanDetailMap_FinishExit(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
}

// Lights the panel's icons of the kinds of habitat at the picked place
static void ZukanDetailMap_UpdateKinds(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    u8 kinds[HABITAT_KIND_COUNT] = { FALSE, FALSE, FALSE };
    u8 i;

    if (wk->found == TRUE && wk->place != NO_PLACE) {
        u8 season = wk->bySeason ? wk->season : 0;
        u8 order = wk->placeOrder[wk->place];

        if (wk->habitat->seasons[season].places[order] & HABITAT_LAND) {
            kinds[0] = TRUE;
        }
        if (wk->habitat->seasons[season].places[order] & HABITAT_WATER) {
            kinds[1] = TRUE;
        }
        if (wk->habitat->seasons[season].places[order] & HABITAT_FISHING) {
            kinds[2] = TRUE;
        }
    }
    for (i = 0; i < HABITAT_KIND_COUNT; i++) {
        GFL_BGSysSetScrPaletteNo(PANEL_BG, sZukanDetailMapKindRects[i][0], sZukanDetailMapKindRects[i][1],
                                 sZukanDetailMapKindRects[i][2], sZukanDetailMapKindRects[i][3],
                                 sZukanDetailMapKindPalettes[i][kinds[i]]);
    }
    if (wk->state == MAP_STATE_PLACE || wk->state == MAP_STATE_PLACE_CHANGE) {
        ZukanDetailMap_UpdateButton(param, wk, common, TRUE);
    }
    GFL_BGSysQueueScrLoad(PANEL_BG);
}

// Darkens the map when the Pokémon lives nowhere
static void ZukanDetailMap_SetDimBlend(ZukanDetailMapWork *wk) {
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 0, ALL_PLANES, 12, 4);
}

static void ZukanDetailMap_ResetBlend(ZukanDetailMapWork *wk) {
    ZukanDetailBlend_InitPlanes(wk->blendMain);
}

// Two windows over the map, out of which the blend's glow applies
static void ZukanDetailMap_SetHighlight(ZukanDetailMapWork *wk) {
    GX_SetVisibleWnd(GX_WNDMASK_W0 | GX_WNDMASK_W1);
    G2_SetWnd0Position(0, 0, 160, 168);
    G2_SetWnd1Position(160, 0, 0, 168);
    G2_SetWnd0InsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                          TRUE);
    G2_SetWnd1InsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                          TRUE);
    G2_SetWndOutsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                          FALSE);
    ZukanDetailMap_StartGlow(wk);
}

static void ZukanDetailMap_ClearHighlight(ZukanDetailMapWork *wk) {
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    ZukanDetailBlend_InitPlanes(wk->blendMain);
}

static void ZukanDetailMap_UpdateGlow(ZukanDetailMapWork *wk) {
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 0, ALL_PLANES, wk->glowEv, 16 - wk->glowEv);
    func_ov302_021ade74(&wk->glowPhase, &wk->glowEv);
}

static void ZukanDetailMap_StartGlow(ZukanDetailMapWork *wk) {
    func_ov302_021adec0(&wk->glowPhase, &wk->glowEv);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 0, ALL_PLANES, wk->glowEv, 16 - wk->glowEv);
}

static void ZukanDetailMap_SetActorsOpaque(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                           ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        if (i != ACTOR_UNKNOWN) {
            func_0204c318(wk->actors[i], 0);
        }
    }
    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (wk->places[i].state == PLACE_SHOWN) {
            func_0204c318(wk->places[i].actor, 0);
        }
    }
}

static void ZukanDetailMap_SetActorsBlended(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                            ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        if (i != ACTOR_UNKNOWN) {
            func_0204c318(wk->actors[i], 1);
        }
    }
    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (wk->places[i].state == PLACE_SHOWN) {
            func_0204c318(wk->places[i].actor, 1);
        }
    }
}

static void ZukanDetailMap_SetIconsOpaque(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                          ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (wk->icons[i] != NULL) {
            func_0204c318(wk->icons[i], 0);
        }
    }
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG3, ALL_PLANES, 12, 4);
}

static void ZukanDetailMap_SetIconsBlended(ZukanDetailMapParam *param, ZukanDetailMapWork *wk,
                                           ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (wk->icons[i] != NULL) {
            func_0204c318(wk->icons[i], 1);
        }
    }
    ZukanDetailBlend_InitPlanes(wk->blendSub);
}

// Whether the player has the event flag that the habitat list needs for the picked place
static BOOL ZukanDetailMap_IsPlaceListed(ZukanDetailMapWork *wk, ZukanDetailCommon *common) {
    GameData *gameData = ZukanDetailCommon_GetGameData(common);
    EventWork *eventWork;
    u16 zone;
    BOOL listed;
    int flag;

    func_02010cb8(GameData_GetSaveControl(gameData));
    eventWork = GameData_GetEventWork(gameData);
    zone = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_ZONE);
    listed = FALSE;
    flag = ZukanDetailMap_GetPlaceFlag(zone);
    if (flag >= 0) {
        listed = EventWork_FlagGet(eventWork, flag);
    }
    return listed;
}

static int ZukanDetailMap_GetPlaceFlag(u16 zone) {
    int i;

    for (i = 0; i < PLACE_FLAG_COUNT; i++) {
        if (zone == sZukanDetailMapPlaceFlags[i].zone) {
            return sZukanDetailMapPlaceFlags[i].flag;
        }
    }
    return -1;
}

// The touch bar's button that opens the habitat list of the picked place
static void ZukanDetailMap_UpdateButton(ZukanDetailMapParam *param, ZukanDetailMapWork *wk, ZukanDetailCommon *common,
                                        BOOL show) {
    PokeDexSave *pokedex = GameData_GetPokedex(ZukanDetailCommon_GetGameData(common));
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);

    param->place = ZUKAN_DETAIL_MAP_NO_PLACE;
    if (show == TRUE) {
        if (PokeDex_IsHabitatListEnabled(pokedex) == TRUE) {
            func_0204c124(wk->actors[ACTOR_BUTTON], FALSE);
            ZukanDetailTouchbar_SetMapPlaceVisible(touchbar, TRUE);
            if (ZukanDetailMap_SetHabitatKind(wk, common) == TRUE) {
                param->place = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_ZONE);
                ZukanDetailTouchbar_SetMapPlaceActive(touchbar, TRUE);
                ZukanDetailMap_PrintButton(param, wk, common, TRUE);
            } else {
                ZukanDetailTouchbar_SetMapPlaceActive(touchbar, FALSE);
                ZukanDetailMap_PrintButton(param, wk, common, FALSE);
            }
        } else {
            ZukanDetailTouchbar_SetMapPlaceVisible(touchbar, FALSE);
        }
    } else {
        ZukanDetailTouchbar_SetMapPlaceVisible(touchbar, FALSE);
    }
}
