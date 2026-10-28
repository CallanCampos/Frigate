#include "types.h"
#include "app/pokedex_common.h"
#include "app/pokedex_detail.h"
#include "app/townmap.h"
#include "app/townmap/townmap_data.h"
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
#include "save/pokedex_savedata.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/season.h"
#include "system/wordset.h"
#include "text/system/btl_server_flow_title.h"

// The detail screen's habitat map page, which the ROM does not name: the town map on the main screen with the areas
// where the Pokémon lives in each season, and a cursor to pick a place for the Pokédex's habitat list

// The Pokémon icons, and the habitats, a file per species
#define ARCID_POKEICON 7
#define ARCID_POKEDEX_AREA 176

// The event flags that show the water and fishing habitats
#define FLAG_HABITAT_WATER 0x987
#define FLAG_HABITAT_FISHING 0x985

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
        // The HABITAT_* of each place, in the order of sPokedexDetailMapZones
        u8 places[TOWNMAP_PLACE_COUNT];
    } seasons[SEASON_COUNT];
} PokedexHabitat;

typedef struct {
    int state;
    ClActor *actor;
} MapPlace;

typedef struct {
    ClActUnit *unit;
    Font *font;
    PokedexDetailBackground *background;
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
    PokedexHabitat *habitat;
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
    // Each place's index in sPokedexDetailMapZones
    u8 placeOrder[TOWNMAP_PLACE_COUNT];
    ArcTool *habitatArc;
    TCB *vblankTcb;
    u8 vblankRequest;
    PokedexDetailBlend *blendMain;
    PokedexDetailBlend *blendSub;
    PokedexDetailPalFade *palFade;
    int exit;
    BOOL inputEnabled;
    // An arrow of the touch bar was touched, the Pokémon it moved to is shown, and the arrow's animation has played
    BOOL arrowPushed;
    BOOL arrowLoaded;
    BOOL arrowDone;
} PokedexDetailMapWork;

// The event flag that the habitat list needs for a zone
#define PLACE_FLAG_COUNT 57

typedef struct {
    u16 flag;
    u16 zone;
    u16 id;
} MapPlaceFlag;

static BOOL PokedexDetailMap_Init(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                  PokedexDetailCommon *common);
static BOOL PokedexDetailMap_Exit(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                  PokedexDetailCommon *common);
static BOOL PokedexDetailMap_Main(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                  PokedexDetailCommon *common);
static void PokedexDetailMap_Command(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                     PokedexDetailCommon *common, int command);
static void PokedexDetailMap_VBlank(TCB *tcb, void *data);
static void PokedexDetailMap_InitPlaceOrder(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common);
static void PokedexDetailMap_LoadMapBG(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common);
static void PokedexDetailMap_FreeMapBG(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common);
static void PokedexDetailMap_CreateActors(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common);
static void PokedexDetailMap_FreeActors(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                        PokedexDetailCommon *common);
static void PokedexDetailMap_CreateAreas(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_FreeAreas(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common);
static void PokedexDetailMap_CreatePlaces(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common);
static void PokedexDetailMap_FreePlaces(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                        PokedexDetailCommon *common);
static void PokedexDetailMap_CreatePrintQueues(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                               PokedexDetailCommon *common);
static void PokedexDetailMap_FreePrintQueues(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                             PokedexDetailCommon *common);
static void PokedexDetailMap_UpdatePrint(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_TransferWindow(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common, u8 window);
static void PokedexDetailMap_CreateWindows(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                           PokedexDetailCommon *common);
static void PokedexDetailMap_FreeWindows(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_LoadPanelBG(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_FreePanelBG(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_UpdateSlide(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_UpdateMainSlide(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                             PokedexDetailCommon *common);
static void PokedexDetailMap_UpdateSubSlide(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common);
static void PokedexDetailMap_UpdateArrow(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_LoadIconResources(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                               PokedexDetailCommon *common);
static void PokedexDetailMap_FreeIconResources(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                               PokedexDetailCommon *common);
static ClActor *PokedexDetailMap_CreateIcon(u32 *chars, u32 palette, u32 cellAnims, ClActUnit *unit, HeapID heapId,
                                            u32 species, u32 form, u32 sex, BOOL egg, u8 x, u8 y);
static void PokedexDetailMap_FreeIcon(u32 chars, ClActor *actor);
static u8 PokedexDetailMap_GetPlayerPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common);
static void PokedexDetailMap_InitPlayerMarker(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                              PokedexDetailCommon *common);
static void PokedexDetailMap_SetPlayerMarkerVisible(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                                    PokedexDetailCommon *common, BOOL visible);
static BOOL PokedexDetailMap_IsPlayerMarkerVisible(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                                   PokedexDetailCommon *common);
static void PokedexDetailMap_ResetCursor(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_SelectPlayerPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                               PokedexDetailCommon *common);
static u8 PokedexDetailMap_FindTouchedPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common, u8 x, u8 y);
static BOOL PokedexDetailMap_IsNearPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common, u8 place, u8 x, u8 y, u32 *distSq);
static u8 PokedexDetailMap_FindNearestPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common, u8 x, u8 y);
static void PokedexDetailMap_HandleInput(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_MoveCursor(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                        PokedexDetailCommon *common);
static BOOL PokedexDetailMap_SetHabitatKind(PokedexDetailMapWork *wk, PokedexDetailCommon *common);
static void PokedexDetailMap_UpdateState(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_SetState(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                      PokedexDetailCommon *common, int state);
static void PokedexDetailMap_LoadPokemon(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_ShowHabitat(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_StartExit(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common);
static void PokedexDetailMap_PrintPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                        PokedexDetailCommon *common);
static void PokedexDetailMap_ChangeSeason(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common, BOOL next, BOOL touch);
static PokedexHabitat *PokedexDetailMap_LoadHabitat(u16 species, HeapID heapId, ArcTool *arc);
static void PokedexDetailMap_FreeHabitat(PokedexHabitat *habitat);
static void PokedexDetailMap_PrintSeason(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_StartChange(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_HideHabitat(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_RequestHabitatBlend(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                                 PokedexDetailCommon *common);
static void PokedexDetailMap_ShowAreas(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common);
static void PokedexDetailMap_PrintButton(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common, BOOL active);
static void PokedexDetailMap_ClearButton(PokedexDetailMapWork *wk);
static void PokedexDetailMap_HideAreas(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common);
static void PokedexDetailMap_RequestReset(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common);
static void PokedexDetailMap_FinishExit(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                        PokedexDetailCommon *common);
static void PokedexDetailMap_UpdateKinds(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailMap_SetDimBlend(PokedexDetailMapWork *wk);
static void PokedexDetailMap_ResetBlend(PokedexDetailMapWork *wk);
static void PokedexDetailMap_SetHighlight(PokedexDetailMapWork *wk);
static void PokedexDetailMap_ClearHighlight(PokedexDetailMapWork *wk);
static void PokedexDetailMap_UpdateGlow(PokedexDetailMapWork *wk);
static void PokedexDetailMap_StartGlow(PokedexDetailMapWork *wk);
static void PokedexDetailMap_SetActorsOpaque(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                             PokedexDetailCommon *common);
static void PokedexDetailMap_SetActorsBlended(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                              PokedexDetailCommon *common);
static void PokedexDetailMap_SetIconsOpaque(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common);
static void PokedexDetailMap_SetIconsBlended(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                             PokedexDetailCommon *common);
static BOOL PokedexDetailMap_IsPlaceListed(PokedexDetailMapWork *wk, PokedexDetailCommon *common);
static int PokedexDetailMap_GetPlaceFlag(u16 zone);
static void PokedexDetailMap_UpdateButton(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common, BOOL show);

const PokedexDetailProcFuncs POKEDEX_DETAIL_MAP_PROC_FUNCS = {
    PokedexDetailMap_Init, PokedexDetailMap_Main, PokedexDetailMap_Exit, PokedexDetailMap_Command, NULL,
};

// The zones of the places, in the order of the habitat files, and an unused 0
static const u16 sPokedexDetailMapZones[TOWNMAP_PLACE_COUNT + 1] = {
    0x185, 0x18d, 0x006, 0x010, 0x01c, 0x03e, 0x060, 0x06b, 0x071, 0x078, 0x088, 0x196, 0x19c, 0x1a2, 0x000,
    0x1a8, 0x093, 0x117, 0x17d, 0x098, 0x09a, 0x09d, 0x0a0, 0x0bf, 0x0c2, 0x0c6, 0x0cd, 0x0e6, 0x0eb, 0x0ee,
    0x144, 0x14d, 0x152, 0x15a, 0x178, 0x0f0, 0x181, 0x13d, 0x13f, 0x141, 0x146, 0x149, 0x14b, 0x151, 0x159,
    0x15c, 0x16d, 0x170, 0x172, 0x176, 0x17a, 0x17f, 0x1a7, 0x183, 0x0f9, 0x0fd, 0x0fe, 0x0ff, 0x107, 0x1ab,
    0x1b7, 0x1c0, 0x1ca, 0x1d1, 0x236, 0x1ea, 0x1b5, 0x1be, 0x1cf, 0x1da, 0x1ef, 0x1bc, 0x1c8, 0x1cd, 0x1ce,
    0x1db, 0x23d, 0x1f7, 0x203, 0x1fa, 0x205, 0x1d0, 0x108, 0x228, 0x0f1, 0x000,
};

// The map's BGs 2 and 3, 256-color bitmaps, and the BGs they replace
static const PokedexDetailBGSetup sPokedexDetailMapBGSetups[2] = {
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

static const PokedexDetailBGSetup sPokedexDetailMapDefaultBGSetups[2] = {
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

static const MapPlaceFlag sPokedexDetailMapPlaceFlags[PLACE_FLAG_COUNT] = {
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
static u8 sPokedexDetailMapKindPalettes[HABITAT_KIND_COUNT][2] = {
    { 5, 2 },
    { 6, 0 },
    { 7, 4 },
};

static u8 sPokedexDetailMapKindRects[HABITAT_KIND_COUNT][4] = {
    { 7, 28, 4, 3 },
    { 14, 28, 4, 3 },
    { 21, 28, 4, 3 },
};

void PokedexDetailMap_InitParam(PokedexDetailMapParam *param, HeapID heapId) {
    param->heapId = heapId;
    param->place = ZONE_MAX;
}

static BOOL PokedexDetailMap_Init(PokedexDetailProcSys *sys, int *seq, void *param_, void *work,
                                  PokedexDetailCommon *common) {
    PokedexDetailMapParam *param = param_;
    GameData *gameData = PokedexDetailCommon_GetGameData(common);
    PokedexDetailMapWork *wk;
    u8 i;

    GFL_OvlLoad(OVERLAY_TOWNMAP);
    wk = PokedexDetailProcSys_AllocWork(sys, sizeof(PokedexDetailMapWork), param->heapId);
    MI_CpuFill8(wk, 0, sizeof(PokedexDetailMapWork));
    wk->unit = PokedexDetailGraphic_GetClActUnit(PokedexDetailCommon_GetGraphic(common));
    wk->font = PokedexDetailCommon_GetFont(common);
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
    PokedexDetailMap_InitPlaceOrder(param, wk, common);
    wk->habitatArc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_AREA, param->heapId);
    wk->vblankTcb = GFL_VBlankTCBAdd(PokedexDetailMap_VBlank, wk, 1);
    wk->vblankRequest = VBLANK_NONE;
    PokedexDetailMap_CreatePrintQueues(param, wk, common);
    wk->blendMain = PokedexDetailBlend_Create(param->heapId);
    wk->blendSub = PokedexDetailBlend_Create(param->heapId);
    PokedexDetailBlend_InitPlanes(wk->blendMain);
    PokedexDetailBlend_InitPlanes(wk->blendSub);
    PokedexDetailBlend_SetOut(0, wk->blendMain);
    PokedexDetailBlend_SetOut(1, wk->blendSub);
    wk->palFade =
        PokedexDetailPalFade_CreateEx(param->heapId, POKEDEX_DETAIL_PALFADE_SUB_BG | POKEDEX_DETAIL_PALFADE_SUB_OBJ);
    wk->exit = MAP_EXIT_NONE;
    wk->inputEnabled = TRUE;
    wk->arrowPushed = FALSE;
    wk->arrowLoaded = TRUE;
    wk->arrowDone = TRUE;
    return TRUE;
}

static BOOL PokedexDetailMap_Exit(PokedexDetailProcSys *sys, int *seq, void *param_, void *work,
                                  PokedexDetailCommon *common) {
    PokedexDetailMapParam *param = param_;
    PokedexDetailMapWork *wk = work;
    u8 i;

    PokedexDetailMap_FreeHabitat(wk->habitat);
    for (i = 0; i < 2; i++) {
        if (wk->icons[i] != NULL) {
            PokedexDetailMap_FreeIcon(wk->iconChars[i], wk->icons[i]);
        }
        wk->icons[i] = NULL;
    }
    PokedexDetailMap_FreeIconResources(param, wk, common);
    PokedexDetailMap_FreePanelBG(param, wk, common);
    PokedexDetailMap_FreeWindows(param, wk, common);
    PokedexDetailBackground_Free(wk->background);
    PokedexDetailMap_FreePlaces(param, wk, common);
    PokedexDetailMap_FreeAreas(param, wk, common);
    PokedexDetailMap_FreeActors(param, wk, common);
    PokedexDetailPalFade_Free(wk->palFade);
    PokedexDetailBlend_Free(wk->blendSub);
    PokedexDetailBlend_Free(wk->blendMain);
    PokedexDetailMap_FreePrintQueues(param, wk, common);
    GFL_TCBRemove(wk->vblankTcb);
    GFL_ArcToolFree(wk->habitatArc);
    TownMapData_Free(wk->townmap);
    PokedexDetailProcSys_FreeWork(sys);
    GFL_OvlUnload(OVERLAY_TOWNMAP);
    return TRUE;
}

static BOOL PokedexDetailMap_Main(PokedexDetailProcSys *sys, int *seq, void *param_, void *work,
                                  PokedexDetailCommon *common) {
    PokedexDetailMapParam *param = param_;
    PokedexDetailMapWork *wk = work;
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);
    PokedexDetailHeadbar *headbar = PokedexDetailCommon_GetHeadbar(common);

    switch (*seq) {
    case MAP_SEQ_INIT: {
        u32 i;
        u8 bg;

        *seq = MAP_SEQ_FADE_IN_SUB;
        GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BG0_AS_2D);
        for (i = 0; i < NELEMS(sPokedexDetailMapBGSetups); i++) {
            GFL_BGSysReleaseBG(sPokedexDetailMapBGSetups[i].bg);
            GFL_BGSysCreateBG(sPokedexDetailMapBGSetups[i].bg, &sPokedexDetailMapBGSetups[i].setup,
                              sPokedexDetailMapBGSetups[i].mode);
            GFL_BGSysClearBG(sPokedexDetailMapBGSetups[i].bg);
            GFL_BGSysSetBGEnabled(sPokedexDetailMapBGSetups[i].bg, sPokedexDetailMapBGSetups[i].enabled);
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
        PokedexDetailTouchbar_SetBGPriority(touchbar, 1);
        PokedexDetailMap_LoadMapBG(param, wk, common);
        PokedexDetailMap_CreateActors(param, wk, common);
        PokedexDetailMap_CreateAreas(param, wk, common);
        PokedexDetailMap_CreatePlaces(param, wk, common);
        wk->background = PokedexDetailBackground_Create(param->heapId, 1, 4, 9, 10);
        PokedexDetailMap_CreateWindows(param, wk, common);
        PokedexDetailMap_LoadPanelBG(param, wk, common);
        PokedexDetailMap_LoadIconResources(param, wk, common);
        PokedexDetailMap_LoadPokemon(param, wk, common);
        ClActor_SetObjMode(wk->icons[wk->icon], 1);
        PokedexDetailMap_InitPlayerMarker(param, wk, common);
        if (wk->found == TRUE) {
            PokedexDetailMap_SetPlayerMarkerVisible(param, wk, common, TRUE);
        }
        PokedexDetailPalFade_ReadPalettes(wk->palFade);
        PokedexDetailPalFade_SetHidden(wk->palFade);
        break;
    }
    case MAP_SEQ_FADE_IN_SUB:
        *seq = MAP_SEQ_FADE_IN;
        PokedexDetailBlend_SetIn(1, wk->blendSub);
        PokedexDetailMap_SetIconsOpaque(param, wk, common);
        break;
    case MAP_SEQ_FADE_IN:
        *seq = MAP_SEQ_WAIT_FADE_IN;
        PokedexDetailBlend_StartIn(wk->blendMain);
        PokedexDetailPalFade_StartIn(wk->palFade);
        if (PokedexDetailTouchbar_GetState(touchbar) != POKEDEX_DETAIL_TOUCHBAR_SHOWN) {
            PokedexDetailTouchbar_SetType(touchbar, POKEDEX_DETAIL_TOUCHBAR_GENERAL, POKEDEX_DETAIL_PAGE_MAP - 1,
                                          PokedexDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
            PokedexDetailTouchbar_Appear(touchbar, 0);
        } else {
            PokedexDetailTouchbar_SetPage(touchbar, POKEDEX_DETAIL_PAGE_MAP - 1);
        }
        PokedexDetailTouchbar_SetActive(touchbar, FALSE);
        wk->active = FALSE;
        PokedexDetailTouchbar_SetCheck(
            touchbar, GameData_IsShortcutRegistered(PokedexDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_MAP));
        if (PokedexDetailHeadbar_GetState(headbar) != POKEDEX_DETAIL_TOUCHBAR_SHOWN) {
            PokedexDetailHeadbar_SetTitle(headbar, 1);
            PokedexDetailHeadbar_Appear(headbar);
        }
        break;
    case MAP_SEQ_WAIT_FADE_IN:
        if (!PokedexDetailBlend_IsActive(wk->blendMain) && !PokedexDetailPalFade_IsFading(wk->palFade) &&
            PokedexDetailTouchbar_GetState(touchbar) == POKEDEX_DETAIL_TOUCHBAR_SHOWN &&
            PokedexDetailHeadbar_GetState(headbar) == POKEDEX_DETAIL_TOUCHBAR_SHOWN) {
            PokedexDetailTouchbar_Unlock(touchbar);
            PokedexDetailMap_SetActorsOpaque(param, wk, common);
            PokedexDetailMap_ShowHabitat(param, wk, common);
            *seq = MAP_SEQ_WAIT_STEP;
        }
        break;
    case MAP_SEQ_WAIT_STEP:
        if (wk->step == 0) {
            PokedexDetailTouchbar_SetActive(touchbar, TRUE);
            wk->active = TRUE;
            *seq = MAP_SEQ_MAIN;
        }
        break;
    case MAP_SEQ_MAIN:
        if (wk->exit != MAP_EXIT_NONE) {
            PokedexDetailMap_StartExit(param, wk, common);
            *seq = MAP_SEQ_WAIT_EXIT;
        } else {
            if (wk->found == TRUE) {
                PokedexDetailMap_UpdateGlow(wk);
            }
            PokedexDetailMap_HandleInput(param, wk, common);
        }
        break;
    case MAP_SEQ_WAIT_EXIT:
        if (wk->step == 0) {
            PokedexDetailMap_SetActorsBlended(param, wk, common);
            *seq = MAP_SEQ_WAIT_FADE_OUT;
            PokedexDetailBlend_StartOut(wk->blendMain);
            PokedexDetailPalFade_StartOut(wk->palFade);
            PokedexDetailHeadbar_Disappear(headbar);
            if (wk->exit == MAP_EXIT_SCREEN) {
                PokedexDetailTouchbar_Disappear(touchbar, 0);
            }
        }
        break;
    case MAP_SEQ_WAIT_FADE_OUT: {
        BOOL done = FALSE;

        if (!PokedexDetailBlend_IsActive(wk->blendMain) && !PokedexDetailPalFade_IsFading(wk->palFade) &&
            PokedexDetailHeadbar_GetState(headbar) == POKEDEX_DETAIL_TOUCHBAR_HIDDEN) {
            if (wk->exit == MAP_EXIT_SCREEN) {
                if (PokedexDetailTouchbar_GetState(touchbar) == POKEDEX_DETAIL_TOUCHBAR_HIDDEN) {
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
        PokedexDetailMap_SetIconsBlended(param, wk, common);
        PokedexDetailBlend_SetOut(1, wk->blendSub);
        break;
    case MAP_SEQ_EXIT: {
        u32 i;

        PokedexDetailMap_FreeMapBG(param, wk, common);
        GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_2D);
        for (i = 0; i < NELEMS(sPokedexDetailMapDefaultBGSetups); i++) {
            GFL_BGSysReleaseBG(sPokedexDetailMapDefaultBGSetups[i].bg);
            GFL_BGSysCreateBG(sPokedexDetailMapDefaultBGSetups[i].bg, &sPokedexDetailMapDefaultBGSetups[i].setup,
                              sPokedexDetailMapDefaultBGSetups[i].mode);
            GFL_BGSysClearBG(sPokedexDetailMapDefaultBGSetups[i].bg);
            GFL_BGSysSetBGEnabled(sPokedexDetailMapDefaultBGSetups[i].bg, sPokedexDetailMapDefaultBGSetups[i].enabled);
        }
        return TRUE;
    }
    }

    if (*seq >= MAP_SEQ_FADE_IN) {
        PokedexDetailBackground_Update(wk->background);
        PokedexDetailMap_UpdateSlide(param, wk, common);
        PokedexDetailMap_UpdateArrow(param, wk, common);
    }
    PokedexDetailMap_UpdateState(param, wk, common);
    PokedexDetailBlend_Update(wk->blendMain, wk->blendSub);
    PokedexDetailPalFade_Update(wk->palFade);
    PokedexDetailMap_UpdatePrint(param, wk, common);
    return FALSE;
}

static void PokedexDetailMap_Command(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                     PokedexDetailCommon *common, int command) {
    PokedexDetailMapWork *wk = work;

    if (wk != NULL) {
        PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);
        BOOL unlock = FALSE;

        // Input waits while the bar's icons play their animations
        switch (command) {
        case POKEDEX_DETAIL_CMD_CLOSE_TOUCH:
        case POKEDEX_DETAIL_CMD_RETURN_TOUCH:
        case POKEDEX_DETAIL_CMD_CUR_D_TOUCH:
        case POKEDEX_DETAIL_CMD_CUR_U_TOUCH:
        case POKEDEX_DETAIL_CMD_CHECK_TOUCH:
        case POKEDEX_DETAIL_CMD_INFO_TOUCH:
        case POKEDEX_DETAIL_CMD_VOICE_TOUCH:
        case POKEDEX_DETAIL_CMD_FORM_TOUCH:
        case POKEDEX_DETAIL_CMD_MAP_RETURN_TOUCH:
        case POKEDEX_DETAIL_CMD_MAP_PLACE_TOUCH:
            wk->inputEnabled = FALSE;
            break;
        case POKEDEX_DETAIL_CMD_MAP_TOUCH:
            break;
        }
        switch (command) {
        case POKEDEX_DETAIL_CMD_CLOSE:
        case POKEDEX_DETAIL_CMD_RETURN:
        case POKEDEX_DETAIL_CMD_CUR_D:
        case POKEDEX_DETAIL_CMD_CUR_U:
        case POKEDEX_DETAIL_CMD_CHECK:
        case POKEDEX_DETAIL_CMD_INFO:
        case POKEDEX_DETAIL_CMD_VOICE:
        case POKEDEX_DETAIL_CMD_FORM:
        case POKEDEX_DETAIL_CMD_MAP_RETURN:
            unlock = TRUE;
            wk->inputEnabled = TRUE;
            break;
        }

        switch (command) {
        case POKEDEX_DETAIL_CMD_CUR_D_TOUCH: {
            u16 species = PokedexDetailCommon_GetSpecies(common);

            PokedexDetailCommon_GoNext(common);
            if (species != PokedexDetailCommon_GetSpecies(common)) {
                PokedexDetailMap_LoadPokemon(param, wk, common);
                PokedexDetailMap_ShowHabitat(param, wk, common);
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
        case POKEDEX_DETAIL_CMD_CUR_U_TOUCH: {
            u16 species = PokedexDetailCommon_GetSpecies(common);

            PokedexDetailCommon_GoPrev(common);
            if (species != PokedexDetailCommon_GetSpecies(common)) {
                PokedexDetailMap_LoadPokemon(param, wk, common);
                PokedexDetailMap_ShowHabitat(param, wk, common);
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
        case POKEDEX_DETAIL_CMD_NONE:
            break;
        case POKEDEX_DETAIL_CMD_CLOSE:
        case POKEDEX_DETAIL_CMD_RETURN:
            wk->exit = MAP_EXIT_SCREEN;
            break;
        case POKEDEX_DETAIL_CMD_INFO:
        case POKEDEX_DETAIL_CMD_VOICE:
        case POKEDEX_DETAIL_CMD_FORM:
            wk->exit = MAP_EXIT_PAGE;
            break;
        case POKEDEX_DETAIL_CMD_CUR_D:
            if (wk->arrowPushed) {
                wk->arrowDone = TRUE;
                if (wk->arrowLoaded) {
                    wk->arrowPushed = FALSE;
                    PokedexDetailTouchbar_Unlock(touchbar);
                }
            }
            break;
        case POKEDEX_DETAIL_CMD_CUR_U:
            if (wk->arrowPushed) {
                wk->arrowDone = TRUE;
                if (wk->arrowLoaded) {
                    wk->arrowPushed = FALSE;
                    PokedexDetailTouchbar_Unlock(touchbar);
                }
            }
            break;
        case POKEDEX_DETAIL_CMD_CHECK:
            GameData_SetKeyItemRegistration(PokedexDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_MAP,
                                            PokedexDetailTouchbar_GetCheck(touchbar));
            PokedexDetailTouchbar_Unlock(touchbar);
            break;
        case POKEDEX_DETAIL_CMD_MAP_RETURN:
            PokedexDetailMap_SetState(param, wk, common, MAP_STATE_CLOSE_START);
            break;
        case POKEDEX_DETAIL_CMD_MAP_PLACE: {
            PokedexHabitatSave *habitatList =
                SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(PokedexDetailCommon_GetGameData(common)));

            PokedexHabitatSave_SetZone(habitatList, TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_ZONE));
            if (wk->bySeason == TRUE) {
                PokedexHabitatSave_SetSeason(habitatList, wk->season);
            }
            PokedexDetailTouchbar_SetMapPlaceVisible(touchbar, FALSE);
            PokedexDetailMap_ClearButton(wk);
            wk->exit = MAP_EXIT_SCREEN;
            break;
        }
        default:
            if (unlock) {
                PokedexDetailTouchbar_Unlock(touchbar);
            }
            break;
        }
    }
}

static void PokedexDetailMap_VBlank(TCB *tcb, void *data) {
    PokedexDetailMapWork *wk = data;

    switch (wk->vblankRequest) {
    case VBLANK_DIM:
        PokedexDetailMap_ClearHighlight(wk);
        PokedexDetailMap_SetDimBlend(wk);
        break;
    case VBLANK_HIGHLIGHT:
        PokedexDetailMap_ResetBlend(wk);
        PokedexDetailMap_SetHighlight(wk);
        break;
    case VBLANK_RESET:
        PokedexDetailMap_ClearHighlight(wk);
        PokedexDetailMap_ResetBlend(wk);
        PokedexDetailBlend_SetIn(0, wk->blendMain);
        break;
    }
    wk->vblankRequest = VBLANK_NONE;
    PokedexDetailPalFade_VBlank(wk->palFade);
}

static void PokedexDetailMap_InitPlaceOrder(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common) {
    u8 i;
    u8 j;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        u16 zone = TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_ZONE);

        for (j = 0; j < TOWNMAP_PLACE_COUNT; j++) {
            if (zone == sPokedexDetailMapZones[j]) {
                wk->placeOrder[i] = j;
                break;
            }
        }
    }
}

static void PokedexDetailMap_LoadMapBG(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_TOWNMAP_GRA, param->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG_EX, 0x6000, 0x2000, param->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG_EX, 0x4000, 0x2000, param->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 5, 2, 0, 0x4000, FALSE, param->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 3, 0, 0xc000, FALSE, param->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 17, 2, 0, 0x800, FALSE, param->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 16, 3, 0, 0x800, FALSE, param->heapId);
    GFL_ArcToolFree(arc);
}

static void PokedexDetailMap_FreeMapBG(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common) {
}

static void PokedexDetailMap_CreateActors(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common) {
    ClActSurfaceSetup surface = { 0, 0, 256, 192, 0, 0 };
    ArcTool *arc;
    u8 i;

    wk->renderer = ClActRenderer_Create(&surface, 1, param->heapId);
    wk->unit2 = ClActUnit_Create(16, 0, param->heapId);
    ClActUnit_SetRenderer(wk->unit2, wk->renderer);

    arc = GFL_ArcSysCreateFileHandle(ARCID_TOWNMAP_GRA, param->heapId);
    wk->mapPalette = ClActSys_LoadPlttRange(arc, 2, CLACT_VRAM_MAIN, 0, 0, 5, param->heapId);
    wk->mapChars = ClActSys_LoadChars(arc, 8, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->mapCellAnims = ClActSys_LoadCellAnims(arc, 12, 19, param->heapId);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, param->heapId);
    wk->seasonPalette = ClActSys_LoadPlttRange(arc, 6, CLACT_VRAM_MAIN, 0xa0, 0, 2, param->heapId);
    wk->seasonChars = ClActSys_LoadChars(arc, 16, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->seasonCellAnims = ClActSys_LoadCellAnims(arc, 30, 47, param->heapId);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, param->heapId);
    wk->areaPalette = ClActSys_LoadPlttRange(arc, 5, CLACT_VRAM_MAIN, 0xe0, 0, 1, param->heapId);
    wk->areaChars = ClActSys_LoadChars(arc, 15, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->areaCellAnims = ClActSys_LoadCellAnims(arc, 29, 46, param->heapId);
    GFL_ArcToolFree(arc);

    {
        u8 anims[ACTOR_COUNT] = { 1, 4, 2, 0, 6, 4, 5, 6 };
        u8 bgPriorities[ACTOR_COUNT] = { 2, 2, 2, 2, 0, 0, 0, 0 };
        u8 priorities[ACTOR_COUNT] = { 2, 1, 0, 3, 0, 0, 1, 2 };
        ClActorSetup setup;
        ClActorPos pos;
        ClActorPos buttonPos;

        MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
        for (i = ACTOR_CURSOR; i < ACTOR_COUNT; i++) {
            wk->actors[i] = ClActor_Create(wk->unit2, wk->mapChars, wk->mapPalette, wk->mapCellAnims, &setup,
                                           CLACT_SURFACE_MAIN, param->heapId);
        }
        for (i = 0; i < ACTOR_CURSOR; i++) {
            wk->actors[i] = ClActor_Create(wk->unit2, wk->seasonChars, wk->seasonPalette, wk->seasonCellAnims, &setup,
                                           CLACT_SURFACE_MAIN, param->heapId);
        }
        for (i = 0; i < ACTOR_COUNT; i++) {
            ClActor_SetSequence(wk->actors[i], anims[i]);
            ClActor_SetAutoAnim(wk->actors[i], TRUE);
            ClActor_SetBgPriority(wk->actors[i], bgPriorities[i]);
            ClActor_SetPriority(wk->actors[i], priorities[i]);
            ClActor_SetVisible(wk->actors[i], FALSE);
            ClActor_SetObjMode(wk->actors[i], 1);
        }
        pos.x = 128;
        pos.y = 96;
        for (i = 0; i < ACTOR_CURSOR; i++) {
            ClActor_SetSurfacePos(wk->actors[i], &pos, CLACT_SURFACE_MAIN);
        }
        buttonPos.x = 132;
        buttonPos.y = 176;
        ClActor_SetSurfacePos(wk->actors[ACTOR_BUTTON], &buttonPos, CLACT_SURFACE_MAIN);
    }
    wk->arrow = ACTOR_NONE;
}

static void PokedexDetailMap_FreeActors(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                        PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        ClActor_Delete(wk->actors[i]);
    }
    ClActSys_FreeCellAnims(wk->areaCellAnims);
    ClActSys_FreeChars(wk->areaChars);
    ClActSys_FreePltt(wk->areaPalette);
    ClActSys_FreeCellAnims(wk->seasonCellAnims);
    ClActSys_FreeChars(wk->seasonChars);
    ClActSys_FreePltt(wk->seasonPalette);
    ClActSys_FreeCellAnims(wk->mapCellAnims);
    ClActSys_FreeChars(wk->mapChars);
    ClActSys_FreePltt(wk->mapPalette);
    ClActUnit_Delete(wk->unit2);
    ClActRenderer_Delete(wk->renderer);
}

static void PokedexDetailMap_CreateAreas(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    ClActorSetup setup;
    u8 i;

    MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        setup.x = TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_AREA_X);
        setup.y = TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_AREA_Y);
        wk->areas[i] = ClActor_Create(wk->unit, wk->areaChars, wk->areaPalette, wk->areaCellAnims, &setup,
                                      CLACT_SURFACE_MAIN, param->heapId);
        ClActor_SetSequence(wk->areas[i], TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_AREA_ANIM));
        ClActor_SetAutoAnim(wk->areas[i], TRUE);
        ClActor_SetBgPriority(wk->areas[i], 2);
        ClActor_SetPriority(wk->areas[i], 4);
        ClActor_SetVisible(wk->areas[i], FALSE);
        ClActor_SetObjMode(wk->areas[i], 1);
    }
}

static void PokedexDetailMap_FreeAreas(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        ClActor_Delete(wk->areas[i]);
    }
}

static inline BOOL PokedexDetailMap_IsPlaceHidden(GameData *gameData, u16 flag) {
    if (TownMap_CheckFlag(gameData, flag) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static void PokedexDetailMap_CreatePlaces(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common) {
    GameData *gameData = PokedexDetailCommon_GetGameData(common);
    ClActorSetup setup;
    u8 i;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        u16 flag = TownMapData_GetParam(wk->townmap, i, TOWNMAP_PARAM_FLAG);

        if (flag != TOWNMAP_NO_FLAG) {
            if (PokedexDetailMap_IsPlaceHidden(gameData, flag)) {
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
                wk->places[i].actor = ClActor_Create(wk->unit2, wk->mapChars, wk->mapPalette, wk->mapCellAnims, &setup,
                                                     CLACT_SURFACE_MAIN, param->heapId);
                ClActor_SetAutoAnim(wk->places[i].actor, TRUE);
                ClActor_SetVisible(wk->places[i].actor, TRUE);
                ClActor_SetObjMode(wk->places[i].actor, 1);
            }
        } else {
            wk->places[i].state = PLACE_NONE;
            wk->places[i].actor = NULL;
        }
    }
}

static void PokedexDetailMap_FreePlaces(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                        PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (wk->places[i].state == PLACE_SHOWN) {
            ClActor_Delete(wk->places[i].actor);
        }
    }
}

static void PokedexDetailMap_CreatePrintQueues(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                               PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->printQueues[i] = PrintQueue_CreateDefault(param->heapId);
        wk->transferPending[i] = FALSE;
    }
}

static void PokedexDetailMap_FreePrintQueues(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                             PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->transferPending[i] = FALSE;
        PrintQueue_Clear(wk->printQueues[i]);
        PrintQueue_Free(wk->printQueues[i]);
    }
}

static void PokedexDetailMap_UpdatePrint(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        PrintQueue_Main(wk->printQueues[i]);
        PokedexDetailMap_TransferWindow(param, wk, common, i);
    }
}

// Loads a window once its text is printed
static void PokedexDetailMap_TransferWindow(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common, u8 window) {
    if (wk->transferPending[window] &&
        !PrintQueue_IsBitmapPending(wk->printQueues[window], BmpWin_GetBitmap(wk->windows[window]))) {
        BmpWin *bmpWin = wk->windows[window];

        BmpWin_FlushChar(bmpWin);
        BmpWin_FlushMap(bmpWin);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(bmpWin));
        wk->transferPending[window] = FALSE;
    }
}

static void PokedexDetailMap_CreateWindows(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                           PokedexDetailCommon *common) {
    PokedexDetailWindowData bgWindows[2] = {
        { 6, 0, 0, 1, 1, 8, 0 },
        { 0, 0, 0, 1, 1, 0, 0 },
    };
    PokedexDetailWindowData windows[WINDOW_COUNT] = {
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

static void PokedexDetailMap_FreeWindows(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        GFL_MsgDataFree(wk->msgData[i]);
    }
    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->transferPending[i] = FALSE;
        PrintQueue_Clear(wk->printQueues[i]);
        BmpWin_Free(wk->windows[i]);
    }
    for (i = 0; i < 2; i++) {
        BmpWin_Free(wk->bgWindows[i]);
    }
}

static void PokedexDetailMap_LoadPanelBG(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, param->heapId);

    GFL_G2DIOLoadArcNCLR(arc, 2, PALTYPE_SUB_BG, 0, 0, 8 * 0x20, param->heapId);
    wk->panelChars = GFL_BGSysLoadArcNCGRDynamic(arc, 12, PANEL_BG, 0, FALSE, param->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 39, PANEL_BG, 0, CHAR_POS(wk->panelChars), 0, FALSE, param->heapId);
    GFL_ArcToolFree(arc);
}

static void PokedexDetailMap_FreePanelBG(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    GFL_BGSysFreeCharMemory(PANEL_BG, CHAR_POS(wk->panelChars), CHAR_SIZE(wk->panelChars));
}

static void PokedexDetailMap_UpdateSlide(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    PokedexDetailMap_UpdateMainSlide(param, wk, common);
    PokedexDetailMap_UpdateSubSlide(param, wk, common);
}

// Slides the panel over the map and the Pokémon's icon up, and back
static void PokedexDetailMap_UpdateMainSlide(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                             PokedexDetailCommon *common) {
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
            ClActor_GetSurfacePos(wk->icons[wk->icon], &pos, CLACT_SURFACE_SUB);
            if (pos.y <= 112) {
                pos.y = 112;
                wk->iconSlide = SLIDE_DONE;
            } else {
                pos.y -= 8;
            }
            ClActor_SetSurfacePos(wk->icons[wk->icon], &pos, CLACT_SURFACE_SUB);
            break;
        case SLIDE_DONE:
            break;
        case SLIDE_OUT:
            ClActor_GetSurfacePos(wk->icons[wk->icon], &pos, CLACT_SURFACE_SUB);
            if (pos.y >= 176) {
                pos.y = 176;
                wk->iconSlide = SLIDE_NONE;
            } else {
                pos.y += 8;
            }
            ClActor_SetSurfacePos(wk->icons[wk->icon], &pos, CLACT_SURFACE_SUB);
            break;
        }
    }
}

static void PokedexDetailMap_UpdateSubSlide(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common) {
}

// Once the season arrow's pushed animation has played, shows both arrows again
static void PokedexDetailMap_UpdateArrow(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);
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
    if (wk->arrow != ACTOR_NONE && !ClActor_IsAnimActive(wk->actors[wk->arrow])) {
        ClActor_SetSequence(wk->actors[wk->arrow], anim);
        ClActor_SetAutoAnim(wk->actors[other], TRUE);
        ClActor_SetSequence(wk->actors[other], otherAnim);
        wk->arrow = ACTOR_NONE;
        if (wk->state == MAP_STATE_MAP || wk->state == MAP_STATE_PLACE) {
            PokedexDetailTouchbar_SetActive(touchbar, TRUE);
        }
    }
}

static void PokedexDetailMap_LoadIconResources(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                               PokedexDetailCommon *common) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, param->heapId);

    wk->iconPalette = ClActSys_LoadPlttEx(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_SUB, 0, param->heapId);
    wk->iconCellAnims =
        ClActSys_LoadCellAnims(arc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_SubEng(), param->heapId);
    GFL_ArcToolFree(arc);
}

static void PokedexDetailMap_FreeIconResources(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                               PokedexDetailCommon *common) {
    ClActSys_FreeCellAnims(wk->iconCellAnims);
    ClActSys_FreePltt(wk->iconPalette);
}

static ClActor *PokedexDetailMap_CreateIcon(u32 *chars, u32 palette, u32 cellAnims, ClActUnit *unit, HeapID heapId,
                                            u32 species, u32 form, u32 sex, BOOL egg, u8 x, u8 y) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, heapId);
    ClActorSetup setup;
    ClActor *actor;

    setup.x = x;
    setup.y = y;
    setup.sequence = 1;
    setup.priority = 1;
    setup.bgPriority = 0;
    *chars = ClActSys_LoadChars(arc, PokeParty_GetIconIndex(species, form, sex, egg), FALSE, CLACT_VRAM_SUB, heapId);
    actor = ClActor_Create(unit, *chars, palette, cellAnims, &setup, CLACT_SURFACE_SUB, heapId);
    ClActor_SetAutoAnim(actor, FALSE);
    ClActor_SetPalette(actor, PokeIcon_GetPalette(species, form, sex, egg), 0);
    ClActor_SetObjMode(actor, 0);
    GFL_ArcToolFree(arc);
    return actor;
}

static void PokedexDetailMap_FreeIcon(u32 chars, ClActor *actor) {
    ClActor_Delete(actor);
    ClActSys_FreeChars(chars);
}

static u8 PokedexDetailMap_GetPlayerPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common) {
    GameData *gameData = PokedexDetailCommon_GetGameData(common);
    u16 zone = PlayerState_GetZoneID(GameData_GetPlayerState(gameData));
    u8 place = NO_PLACE;
    u16 found = TownMapData_GetPlaceByZone(wk->townmap, TownMap_GetMapZone(gameData, zone));

    if (found != TOWNMAP_PLACE_NONE) {
        place = found;
    }
    return place;
}

static void PokedexDetailMap_InitPlayerMarker(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                              PokedexDetailCommon *common) {
    u8 place = PokedexDetailMap_GetPlayerPlace(param, wk, common);

    if (place != NO_PLACE) {
        ClActorPos pos;

        pos.x = TownMapData_GetParam(wk->townmap, place, TOWNMAP_PARAM_X);
        pos.y = TownMapData_GetParam(wk->townmap, place, TOWNMAP_PARAM_Y);
        ClActor_SetSurfacePos(wk->actors[ACTOR_PLAYER], &pos, CLACT_SURFACE_MAIN);
    }
}

static void PokedexDetailMap_SetPlayerMarkerVisible(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                                    PokedexDetailCommon *common, BOOL visible) {
    if (visible) {
        if (PokedexDetailMap_GetPlayerPlace(param, wk, common) != NO_PLACE) {
            ClActor_SetSequence(wk->actors[ACTOR_PLAYER], 6);
            ClActor_SetVisible(wk->actors[ACTOR_PLAYER], TRUE);
        }
    } else {
        ClActor_SetVisible(wk->actors[ACTOR_PLAYER], FALSE);
    }
}

static BOOL PokedexDetailMap_IsPlayerMarkerVisible(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                                   PokedexDetailCommon *common) {
    return ClActor_IsVisible(wk->actors[ACTOR_PLAYER]);
}

// Shows the cursor at the picked place, or at the player's, or else at the middle of the map
static void PokedexDetailMap_ResetCursor(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    ClActorPos pos = { 128, 96 };

    if (wk->place == NO_PLACE) {
        wk->place = PokedexDetailMap_GetPlayerPlace(param, wk, common);
    }
    if (wk->place != NO_PLACE) {
        pos.x = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_X);
        pos.y = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_Y);
    }
    ClActor_SetSurfacePos(wk->actors[ACTOR_CURSOR], &pos, CLACT_SURFACE_MAIN);
    ClActor_SetVisible(wk->actors[ACTOR_CURSOR], TRUE);
}

static void PokedexDetailMap_SelectPlayerPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                               PokedexDetailCommon *common) {
    wk->place = PokedexDetailMap_GetPlayerPlace(param, wk, common);
    PokedexDetailMap_ResetCursor(param, wk, common);
}

// The shown place whose capsule a touch at x, y hits, the nearest if several do, or NO_PLACE
static u8 PokedexDetailMap_FindTouchedPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common, u8 x, u8 y) {
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
static BOOL PokedexDetailMap_IsNearPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common, u8 place, u8 x, u8 y, u32 *distSq) {
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

static u8 PokedexDetailMap_FindNearestPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common, u8 x, u8 y) {
    u8 place = NO_PLACE;
    u32 minDist;
    u32 dist;
    u8 i;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (PokedexDetailMap_IsNearPlace(param, wk, common, i, x, y, &dist)) {
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
static void PokedexDetailMap_HandleInput(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);
    PokeDexSave *pokedex = GameData_GetPokedex(PokedexDetailCommon_GetGameData(common));

    if (!wk->inputEnabled || wk->arrow != ACTOR_NONE) {
        return;
    }
    if (PokedexDetailTouchbar_IsMapPlaceTriggered(touchbar) == TRUE) {
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
                PokedexDetailMap_ChangeSeason(param, wk, common, TRUE, FALSE);
                GFL_SndSEPlay(SEQ_SE_SELECT3);
                handled = TRUE;
            } else if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
                if (wk->season == 0) {
                    wk->season = SEASON_COUNT - 1;
                } else {
                    wk->season--;
                }
                PokedexDetailMap_ChangeSeason(param, wk, common, FALSE, FALSE);
                GFL_SndSEPlay(SEQ_SE_SELECT3);
                handled = TRUE;
            }
        }
        if (handled) {
            GCTX_HIDSetUsingTouch(FALSE);
            if (open) {
                wk->touched = FALSE;
            }
        }
        if (!handled) {
            u32 x;
            u32 y;

            if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
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
                    PokedexDetailMap_ChangeSeason(param, wk, common, TRUE, TRUE);
                    GFL_SndSEPlay(SEQ_SE_SELECT3);
                } else if (x < 24 && y < 16) {
                    if (wk->season == 0) {
                        wk->season = SEASON_COUNT - 1;
                    } else {
                        wk->season--;
                    }
                    handled = TRUE;
                    PokedexDetailMap_ChangeSeason(param, wk, common, FALSE, TRUE);
                    GFL_SndSEPlay(SEQ_SE_SELECT3);
                } else if ((x >= 160 || y >= 24) && y < 168) {
                    open = TRUE;
                    handled = TRUE;
                }
            }
            if (handled) {
                GCTX_HIDSetUsingTouch(TRUE);
                if (open) {
                    wk->touched = TRUE;
                    wk->touchX = x;
                    wk->touchY = y;
                }
            }
        }
        if (open && (wk->bySeason || wk->found)) {
            PokedexDetailMap_SetState(param, wk, common, MAP_STATE_OPEN_START);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            PokedexDetailTouchbar_SetActive(touchbar, FALSE);
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
                PokedexDetailMap_ChangeSeason(param, wk, common, TRUE, FALSE);
                GFL_SndSEPlay(SEQ_SE_SELECT3);
                handled = TRUE;
            } else if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
                if (wk->season == 0) {
                    wk->season = SEASON_COUNT - 1;
                } else {
                    wk->season--;
                }
                move = FALSE;
                PokedexDetailMap_ChangeSeason(param, wk, common, FALSE, FALSE);
                GFL_SndSEPlay(SEQ_SE_SELECT3);
                handled = TRUE;
            }
        }
        if (handled) {
            GCTX_HIDSetUsingTouch(FALSE);
        }
        if (!handled) {
            u32 x;
            u32 y;

            if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
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
                        PokedexDetailMap_ChangeSeason(param, wk, common, TRUE, TRUE);
                        GFL_SndSEPlay(SEQ_SE_SELECT3);
                        move = FALSE;
                    } else if (x < 24 && y < 16) {
                        if (wk->season == 0) {
                            wk->season = SEASON_COUNT - 1;
                        } else {
                            wk->season--;
                        }
                        handled = TRUE;
                        PokedexDetailMap_ChangeSeason(param, wk, common, FALSE, TRUE);
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
                GCTX_HIDSetUsingTouch(TRUE);
            }
        }
        if (move) {
            PokedexDetailMap_MoveCursor(param, wk, common);
        }
        break;
    }
    }
}

// The D-pad moves the cursor, drawn to the nearest place and snapping onto it, and a touch picks the place touched,
// which a second touch confirms
static void PokedexDetailMap_MoveCursor(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                        PokedexDetailCommon *common) {
    u32 x;
    u32 y;

    if (!wk->found) {
        return;
    }
    if (!GCTX_HIDIsUsingTouch()) {
        if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
            GCTX_HIDSetUsingTouch(TRUE);
            ClActor_SetVisible(wk->actors[ACTOR_CURSOR], FALSE);
        }
    } else if (GCTX_HIDGetPressedKeys()) {
        GCTX_HIDSetUsingTouch(FALSE);
        PokedexDetailMap_ResetCursor(param, wk, common);
        PokedexDetailMap_PrintPlace(param, wk, common);
        PokedexDetailMap_UpdateKinds(param, wk, common);
        return;
    }

    if (!GCTX_HIDIsUsingTouch()) {
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

        ClActor_GetSurfacePos(wk->actors[ACTOR_CURSOR], &cursorPos, CLACT_SURFACE_MAIN);
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
        nearPlace = PokedexDetailMap_FindNearestPlace(param, wk, common, cursorPos.x, cursorPos.y);
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
                VEC_Normalize(&toPlace, &toPlace);
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

            VEC_Normalize(&dir, &dir);
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
        cursorPos.x = MATH_CLAMP(cursorPos.x, 0, 248);
        cursorPos.y = MATH_CLAMP(cursorPos.y, 8, 168);
        ClActor_SetSurfacePos(wk->actors[ACTOR_CURSOR], &cursorPos, CLACT_SURFACE_MAIN);
        wk->place = PokedexDetailMap_FindTouchedPlace(param, wk, common, cursorPos.x, cursorPos.y);
        if (wk->place != prevPlace) {
            PokedexDetailMap_PrintPlace(param, wk, common);
            PokedexDetailMap_UpdateKinds(param, wk, common);
            if (wk->place != NO_PLACE) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        }
    } else if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
        u8 prevPlace = wk->place;

        wk->place = PokedexDetailMap_FindTouchedPlace(param, wk, common, x, y);
        PokedexDetailMap_PrintPlace(param, wk, common);
        PokedexDetailMap_UpdateKinds(param, wk, common);
        if (prevPlace == wk->place) {
            PokedexDetailTouchbar_PushMapPlace(PokedexDetailCommon_GetTouchbar(common));
            return;
        }
        if (wk->place != NO_PLACE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
    }
}

// Picks the kind of habitat for the habitat list of the picked place, and returns whether it has one the player can
// see there
static BOOL PokedexDetailMap_SetHabitatKind(PokedexDetailMapWork *wk, PokedexDetailCommon *common) {
    // BUG: With no place picked, this reads past placeOrder. The value is not used then
#ifdef BUGFIX
    u8 order = wk->place != NO_PLACE ? wk->placeOrder[wk->place] : 0;
#else
    u8 order = wk->placeOrder[wk->place];
#endif

    if (wk->found == TRUE && wk->place != NO_PLACE) {
        int season = wk->bySeason == FALSE ? 0 : wk->season;
        u8 kinds = wk->habitat->seasons[season].places[order];

        if (kinds != 0 && PokedexDetailMap_IsPlaceListed(wk, common) == TRUE) {
            GameData *gameData = PokedexDetailCommon_GetGameData(common);
            PokedexHabitatSave *habitatList = SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(gameData));
            EventWork *eventWork = GameData_GetEventWork(gameData);

            if (kinds & HABITAT_LAND) {
                PokedexHabitatSave_SetType(habitatList, 0);
                return TRUE;
            }
            if ((kinds & HABITAT_WATER) && EventWork_FlagGet(eventWork, FLAG_HABITAT_WATER) == TRUE) {
                PokedexHabitatSave_SetType(habitatList, 1);
                return TRUE;
            }
            if ((kinds & HABITAT_FISHING) && EventWork_FlagGet(eventWork, FLAG_HABITAT_FISHING) == TRUE) {
                PokedexHabitatSave_SetType(habitatList, 2);
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void PokedexDetailMap_UpdateState(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    switch (wk->state) {
    case MAP_STATE_MAP:
        break;
    case MAP_STATE_OPEN_START:
        PokedexDetailMap_SetState(param, wk, common, MAP_STATE_OPEN);
        break;
    case MAP_STATE_OPEN:
        PokedexDetailMap_SetState(param, wk, common, MAP_STATE_PLACE);
        break;
    case MAP_STATE_CHANGE:
        switch (wk->step) {
        case 1:
            PokedexDetailMap_HideHabitat(param, wk, common);
            wk->step = 2;
            break;
        case 2:
            PokedexDetailMap_RequestHabitatBlend(param, wk, common);
            wk->step = 3;
            break;
        case 3:
            PokedexDetailMap_ShowAreas(param, wk, common);
            wk->step = 0;
            PokedexDetailMap_SetState(param, wk, common, MAP_STATE_MAP);
            break;
        }
        break;
    case MAP_STATE_EXIT:
        switch (wk->step) {
        case 1:
            PokedexDetailMap_HideAreas(param, wk, common);
            wk->step = 2;
            break;
        case 2:
            PokedexDetailMap_RequestReset(param, wk, common);
            wk->step = 3;
            break;
        case 3:
            PokedexDetailMap_FinishExit(param, wk, common);
            wk->step = 0;
            break;
        }
        break;
    case MAP_STATE_PLACE:
        break;
    case MAP_STATE_CLOSE_START:
        PokedexDetailMap_SetState(param, wk, common, MAP_STATE_CLOSE);
        break;
    case MAP_STATE_CLOSE:
        PokedexDetailMap_SetState(param, wk, common, MAP_STATE_MAP);
        break;
    case MAP_STATE_PLACE_CHANGE:
        switch (wk->step) {
        case 1:
            PokedexDetailMap_HideHabitat(param, wk, common);
            wk->step = 2;
            break;
        case 2:
            PokedexDetailMap_RequestHabitatBlend(param, wk, common);
            wk->step = 3;
            break;
        case 3:
            PokedexDetailMap_ShowAreas(param, wk, common);
            wk->step = 0;
            PokedexDetailMap_SetState(param, wk, common, MAP_STATE_PLACE);
            break;
        }
        break;
    }
}

static void PokedexDetailMap_SetState(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                      PokedexDetailCommon *common, int state) {
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);

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
            PokedexDetailTouchbar_SetVisibleAll(touchbar, FALSE);
        }
        break;
    case MAP_STATE_OPEN:
        if (state == MAP_STATE_PLACE) {
            wk->panelSlide = SLIDE_IN;
            wk->iconSlide = SLIDE_IN;
            if (wk->found == TRUE) {
                if (!wk->touched) {
                    PokedexDetailMap_SelectPlayerPlace(param, wk, common);
                } else {
                    wk->place = PokedexDetailMap_FindTouchedPlace(param, wk, common, wk->touchX, wk->touchY);
                    if (wk->place != NO_PLACE) {
                        ClActor_SetVisible(wk->actors[ACTOR_PLACE], TRUE);
                    }
                }
                PokedexDetailMap_PrintPlace(param, wk, common);
                PokedexDetailMap_UpdateKinds(param, wk, common);
            }
            PokedexDetailTouchbar_SetType(touchbar, POKEDEX_DETAIL_TOUCHBAR_MAP, POKEDEX_DETAIL_PAGE_MAP - 1,
                                          PokedexDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
            wk->place = NO_PLACE;
            PokedexDetailMap_UpdateButton(param, wk, common, TRUE);
        }
        break;
    case MAP_STATE_CHANGE:
        if (state == MAP_STATE_MAP) {
            if (wk->arrowPushed) {
                wk->arrowLoaded = TRUE;
                if (wk->arrowDone) {
                    wk->arrowPushed = FALSE;
                    PokedexDetailTouchbar_Unlock(touchbar);
                }
            }
            if (wk->arrow == ACTOR_NONE && wk->active) {
                PokedexDetailTouchbar_SetActive(touchbar, TRUE);
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
            PokedexDetailTouchbar_SetVisibleAll(touchbar, FALSE);
            PokedexDetailMap_UpdateButton(param, wk, common, FALSE);
            PokedexDetailMap_ClearButton(wk);
        }
        break;
    case MAP_STATE_CLOSE:
        if (state == MAP_STATE_MAP) {
            BOOL showArrows;

            wk->iconSlide = SLIDE_OUT;
            wk->panelSlide = SLIDE_OUT;
            ClActor_SetVisible(wk->actors[ACTOR_CURSOR], FALSE);
            ClActor_SetVisible(wk->actors[ACTOR_PLACE], FALSE);
            showArrows = FALSE;
            if (PokedexDetailCommon_GetCount(common) > 1) {
                showArrows = TRUE;
            }
            PokedexDetailTouchbar_SetType(touchbar, POKEDEX_DETAIL_TOUCHBAR_GENERAL, POKEDEX_DETAIL_PAGE_MAP - 1,
                                          showArrows);
            PokedexDetailTouchbar_SetCheck(
                touchbar, GameData_IsShortcutRegistered(PokedexDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_MAP));
        }
        break;
    case MAP_STATE_PLACE_CHANGE:
        if (state == MAP_STATE_PLACE) {
            PokedexDetailTouchbar_Unlock(touchbar);
            if (wk->arrow == ACTOR_NONE && wk->active) {
                PokedexDetailTouchbar_SetActive(touchbar, TRUE);
            }
        }
        break;
    }
    wk->state = state;
}

// Shows the Pokémon the list is at: its icon slides in on the sub screen, with its name
static void PokedexDetailMap_LoadPokemon(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    BOOL wasBySeason = wk->bySeason;
    u16 species = PokedexDetailCommon_GetSpecies(common);
    WordSet *wordSet;
    StrBuf *format;
    StrBuf *name;
    u32 sex;
    u32 rare;
    u32 form;

    PokedexDetailMap_FreeHabitat(wk->habitat);
    wk->habitat = PokedexDetailMap_LoadHabitat(species, param->heapId, wk->habitatArc);
    wk->bySeason = wk->habitat->allYear == FALSE ? TRUE : FALSE;
    if (!wk->bySeason) {
        wk->found = wk->habitat->seasons[0].none == FALSE ? TRUE : FALSE;
    } else {
        wk->found = wk->habitat->seasons[wk->season].none == FALSE ? TRUE : FALSE;
    }

    if (wk->icons[wk->icon] != NULL) {
        ClActor_SetVisible(wk->icons[wk->icon], FALSE);
    }
    wk->icon = (wk->icon + 1) % 2;
    if (wk->icons[wk->icon] != NULL) {
        // The same release as the exit's, as if from a helper
        if (wk->icons[wk->icon] != NULL) {
            PokedexDetailMap_FreeIcon(wk->iconChars[wk->icon], wk->icons[wk->icon]);
        }
        wk->icons[wk->icon] = NULL;
    }
    PokeDex_GetShownSexRareForm(GameData_GetPokedex(PokedexDetailCommon_GetGameData(common)), species, &sex, &rare,
                                &form, param->heapId);
    wk->icons[wk->icon] = PokedexDetailMap_CreateIcon(&wk->iconChars[wk->icon], wk->iconPalette, wk->iconCellAnims,
                                                      wk->unit, param->heapId, species, form, sex, FALSE, 56, 176);

    wk->transferPending[WINDOW_NAME] = FALSE;
    PrintQueue_Clear(wk->printQueues[WINDOW_NAME]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_NAME]), 0);
    wordSet = GFL_WordSetSystemCreateDefault(param->heapId);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData[0], BtlServerFlowTitle_Text_SArea);
    name = GFL_StrBufCreate(64, param->heapId);
    WordSet_LoadSpeciesName(wordSet, 0, species);
    GFL_WordSetFormatStrbuf(wordSet, name, format);
    GFL_StrBufFree(format);
    GFL_WordSetSystemFree(wordSet);
    PrintQueue_PrintColor(wk->printQueues[WINDOW_NAME], BmpWin_GetBitmap(wk->windows[WINDOW_NAME]), 0, 5, name,
                          wk->font, PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(name);
    wk->transferPending[WINDOW_NAME] = TRUE;
    PokedexDetailMap_TransferWindow(param, wk, common, WINDOW_NAME);

    if (!wk->bySeason) {
        if (wasBySeason == TRUE) {
            wk->transferPending[WINDOW_SEASON] = FALSE;
            PrintQueue_Clear(wk->printQueues[WINDOW_SEASON]);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_SEASON]), 0);
            BmpWin_FlushChar(wk->windows[WINDOW_SEASON]);
            ClActor_SetVisible(wk->actors[ACTOR_SEASON], FALSE);
            ClActor_SetVisible(wk->actors[ACTOR_SEASON_NEXT], FALSE);
            ClActor_SetVisible(wk->actors[ACTOR_SEASON_PREV], FALSE);
        }
    } else if (wasBySeason == FALSE) {
        PokedexDetailMap_PrintSeason(param, wk, common);
        ClActor_SetVisible(wk->actors[ACTOR_SEASON], TRUE);
        ClActor_SetSequence(wk->actors[ACTOR_SEASON_NEXT], 2);
        ClActor_SetVisible(wk->actors[ACTOR_SEASON_NEXT], TRUE);
        ClActor_SetSequence(wk->actors[ACTOR_SEASON_PREV], 4);
        ClActor_SetVisible(wk->actors[ACTOR_SEASON_PREV], TRUE);
    }
}

static void PokedexDetailMap_ShowHabitat(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    PokedexDetailMap_StartChange(param, wk, common);
    PokedexDetailMap_UpdateKinds(param, wk, common);
}

static void PokedexDetailMap_StartExit(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common) {
    PokedexDetailMap_SetState(param, wk, common, MAP_STATE_EXIT);
}

// Prints the picked place's name, and marks it on the map
static void PokedexDetailMap_PrintPlace(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                        PokedexDetailCommon *common) {
    wk->transferPending[WINDOW_PLACE] = FALSE;
    PrintQueue_Clear(wk->printQueues[WINDOW_PLACE]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_PLACE]), 0);
    if (wk->place != NO_PLACE) {
        StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(
            wk->msgData[1], ZoneData_GetPlaceNameID(TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_ZONE)));
        ClActorPos pos;

        PrintQueue_PrintColor(wk->printQueues[WINDOW_PLACE], BmpWin_GetBitmap(wk->windows[WINDOW_PLACE]), 0, 1, strbuf,
                              wk->font, PRINT_COLOR(15, 2, 0));
        GFL_StrBufFree(strbuf);
        wk->transferPending[WINDOW_PLACE] = TRUE;
        PokedexDetailMap_TransferWindow(param, wk, common, WINDOW_PLACE);
        pos.x = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_X);
        pos.y = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_Y);
        ClActor_SetSequence(wk->actors[ACTOR_PLACE], 5);
        ClActor_SetSurfacePos(wk->actors[ACTOR_PLACE], &pos, CLACT_SURFACE_MAIN);
        ClActor_SetVisible(wk->actors[ACTOR_PLACE], TRUE);
    } else {
        BmpWin_FlushChar(wk->windows[WINDOW_PLACE]);
        ClActor_SetVisible(wk->actors[ACTOR_PLACE], FALSE);
    }
}

static void PokedexDetailMap_ChangeSeason(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common, BOOL next, BOOL touch) {
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);
    u16 anim;
    u8 other;

    if (!wk->bySeason) {
        wk->found = wk->habitat->seasons[0].none == FALSE ? TRUE : FALSE;
    } else {
        wk->found = wk->habitat->seasons[wk->season].none == FALSE ? TRUE : FALSE;
    }
    wk->transferPending[WINDOW_SEASON] = FALSE;
    PrintQueue_Clear(wk->printQueues[WINDOW_SEASON]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_SEASON]), 0);
    PokedexDetailMap_PrintSeason(param, wk, common);
    if (next) {
        wk->arrow = ACTOR_SEASON_NEXT;
        anim = 3;
        other = ACTOR_SEASON_PREV;
    } else {
        wk->arrow = ACTOR_SEASON_PREV;
        anim = 5;
        other = ACTOR_SEASON_NEXT;
    }
    ClActor_SetSequence(wk->actors[wk->arrow], anim);
    ClActor_SetAutoAnim(wk->actors[other], FALSE);
    ClActor_SetAnimTime(wk->actors[other], 0);
    PokedexDetailTouchbar_SetActive(touchbar, FALSE);
    if (touch == TRUE) {
        ClActor_SetVisible(wk->actors[ACTOR_CURSOR], FALSE);
    } else if (wk->state == MAP_STATE_PLACE && wk->found == TRUE) {
        PokedexDetailMap_ResetCursor(param, wk, common);
        PokedexDetailMap_PrintPlace(param, wk, common);
    }
    PokedexDetailMap_StartChange(param, wk, common);
    PokedexDetailMap_UpdateKinds(param, wk, common);
}

static PokedexHabitat *PokedexDetailMap_LoadHabitat(u16 species, HeapID heapId, ArcTool *arc) {
    u32 size;

    return GFL_ArcToolReadHeapNewLZGetLen(arc, species - 1, FALSE, heapId, &size);
}

static void PokedexDetailMap_FreeHabitat(PokedexHabitat *habitat) {
    if (habitat != NULL) {
        GFL_HeapFree(habitat);
    }
}

static void PokedexDetailMap_PrintSeason(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], BtlServerFlowTitle_Text_Spring + wk->season);
    u16 width = GFL_FontGetBlockWidth(strbuf, wk->font, 0);
    int windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[WINDOW_SEASON]));
    u16 x = (windowWidth - width) / 2;

    PrintQueue_PrintColor(wk->printQueues[WINDOW_SEASON], BmpWin_GetBitmap(wk->windows[WINDOW_SEASON]), x, 1, strbuf,
                          wk->font, PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(strbuf);
    wk->transferPending[WINDOW_SEASON] = TRUE;
    PokedexDetailMap_TransferWindow(param, wk, common, WINDOW_SEASON);
}

static void PokedexDetailMap_StartChange(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    if (wk->state == MAP_STATE_MAP) {
        PokedexDetailMap_SetState(param, wk, common, MAP_STATE_CHANGE);
    } else if (wk->state == MAP_STATE_PLACE) {
        PokedexDetailMap_SetState(param, wk, common, MAP_STATE_PLACE_CHANGE);
    }
}

static void PokedexDetailMap_HideHabitat(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
    u8 i;

    if (ClActor_IsVisible(wk->actors[ACTOR_UNKNOWN])) {
        if (wk->found == TRUE) {
            wk->transferPending[WINDOW_UNKNOWN] = FALSE;
            PrintQueue_Clear(wk->printQueues[WINDOW_UNKNOWN]);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_UNKNOWN]), 0);
            BmpWin_FlushChar(wk->windows[WINDOW_UNKNOWN]);
            ClActor_SetVisible(wk->actors[ACTOR_UNKNOWN], FALSE);
        }
    } else {
        for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
            ClActor_SetVisible(wk->areas[i], FALSE);
        }
        if (wk->found == FALSE) {
            PokedexDetailMap_SetPlayerMarkerVisible(param, wk, common, FALSE);
            wk->place = NO_PLACE;
            ClActor_SetVisible(wk->actors[ACTOR_CURSOR], FALSE);
            ClActor_SetVisible(wk->actors[ACTOR_PLACE], FALSE);
            PokedexDetailMap_PrintPlace(param, wk, common);
            PokedexDetailMap_UpdateKinds(param, wk, common);
        }
    }
}

static void PokedexDetailMap_RequestHabitatBlend(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                                 PokedexDetailCommon *common) {
    if (!ClActor_IsVisible(wk->actors[ACTOR_UNKNOWN])) {
        if (wk->found == FALSE) {
            wk->vblankRequest = VBLANK_DIM;
        } else {
            wk->vblankRequest = VBLANK_HIGHLIGHT;
        }
    }
}

static void PokedexDetailMap_ShowAreas(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common) {
    BOOL unknownShown = ClActor_IsVisible(wk->actors[ACTOR_UNKNOWN]);

    if (wk->found == FALSE) {
        if (!unknownShown) {
            StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], BtlServerFlowTitle_Text_AreaUnknown);
            u16 width = GFL_FontGetBlockWidth(strbuf, wk->font, 0);
            int windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[WINDOW_UNKNOWN]));
            u16 x = (windowWidth - width) / 2;

            PrintQueue_PrintColor(wk->printQueues[WINDOW_UNKNOWN], BmpWin_GetBitmap(wk->windows[WINDOW_UNKNOWN]), x, 1,
                                  strbuf, wk->font, PRINT_COLOR(15, 2, 0));
            GFL_StrBufFree(strbuf);
            wk->transferPending[WINDOW_UNKNOWN] = TRUE;
            PokedexDetailMap_TransferWindow(param, wk, common, WINDOW_UNKNOWN);
            ClActor_SetVisible(wk->actors[ACTOR_UNKNOWN], TRUE);
        }
    } else {
        u8 season = wk->bySeason == FALSE ? 0 : wk->season;
        u8 i;

        for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
            if (wk->habitat->seasons[season].places[wk->placeOrder[i]] != 0 && wk->places[i].state != PLACE_HIDDEN) {
                ClActor_SetVisible(wk->areas[i], TRUE);
            }
        }
        if (!PokedexDetailMap_IsPlayerMarkerVisible(param, wk, common)) {
            PokedexDetailMap_SetPlayerMarkerVisible(param, wk, common, TRUE);
        }
    }
}

static void PokedexDetailMap_PrintButton(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common, BOOL active) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], BtlServerFlowTitle_Text_ViewDetails);

    if (active == TRUE) {
        PrintQueue_PrintColor(wk->printQueues[WINDOW_BUTTON], BmpWin_GetBitmap(wk->windows[WINDOW_BUTTON]), 0, 5,
                              strbuf, wk->font, PRINT_COLOR(15, 2, 0));
    } else {
        PrintQueue_PrintColor(wk->printQueues[WINDOW_BUTTON], BmpWin_GetBitmap(wk->windows[WINDOW_BUTTON]), 0, 5,
                              strbuf, wk->font, PRINT_COLOR(2, 1, 0));
    }
    GFL_StrBufFree(strbuf);
    wk->transferPending[WINDOW_BUTTON] = TRUE;
    PokedexDetailMap_TransferWindow(param, wk, common, WINDOW_BUTTON);
}

static void PokedexDetailMap_ClearButton(PokedexDetailMapWork *wk) {
    PrintQueue_Clear(wk->printQueues[WINDOW_BUTTON]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_BUTTON]), 0);
    BmpWin_FlushChar(wk->windows[WINDOW_BUTTON]);
    wk->transferPending[WINDOW_BUTTON] = TRUE;
}

static void PokedexDetailMap_HideAreas(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                       PokedexDetailCommon *common) {
    u8 i;

    if (ClActor_IsVisible(wk->actors[ACTOR_UNKNOWN])) {
        wk->transferPending[WINDOW_UNKNOWN] = FALSE;
        PrintQueue_Clear(wk->printQueues[WINDOW_UNKNOWN]);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_UNKNOWN]), 0);
        BmpWin_FlushChar(wk->windows[WINDOW_UNKNOWN]);
        ClActor_SetVisible(wk->actors[ACTOR_UNKNOWN], FALSE);
    } else {
        for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
            ClActor_SetVisible(wk->areas[i], FALSE);
        }
    }
}

static void PokedexDetailMap_RequestReset(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common) {
    wk->vblankRequest = VBLANK_RESET;
}

static void PokedexDetailMap_FinishExit(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                        PokedexDetailCommon *common) {
}

// Lights the panel's icons of the kinds of habitat at the picked place
static void PokedexDetailMap_UpdateKinds(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                         PokedexDetailCommon *common) {
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
        GFL_BGSysSetScrPaletteNo(PANEL_BG, sPokedexDetailMapKindRects[i][0], sPokedexDetailMapKindRects[i][1],
                                 sPokedexDetailMapKindRects[i][2], sPokedexDetailMapKindRects[i][3],
                                 sPokedexDetailMapKindPalettes[i][kinds[i]]);
    }
    if (wk->state == MAP_STATE_PLACE || wk->state == MAP_STATE_PLACE_CHANGE) {
        PokedexDetailMap_UpdateButton(param, wk, common, TRUE);
    }
    GFL_BGSysQueueScrLoad(PANEL_BG);
}

// Darkens the map when the Pokémon lives nowhere
static void PokedexDetailMap_SetDimBlend(PokedexDetailMapWork *wk) {
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 0, ALL_PLANES, 12, 4);
}

static void PokedexDetailMap_ResetBlend(PokedexDetailMapWork *wk) {
    PokedexDetailBlend_InitPlanes(wk->blendMain);
}

// Two windows over the map, out of which the blend's glow applies
static void PokedexDetailMap_SetHighlight(PokedexDetailMapWork *wk) {
    GX_SetVisibleWnd(GX_WNDMASK_W0 | GX_WNDMASK_W1);
    G2_SetWnd0Position(0, 0, 160, 168);
    G2_SetWnd1Position(160, 0, 0, 168);
    G2_SetWnd0InsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                          TRUE);
    G2_SetWnd1InsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                          TRUE);
    G2_SetWndOutsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                          FALSE);
    PokedexDetailMap_StartGlow(wk);
}

static void PokedexDetailMap_ClearHighlight(PokedexDetailMapWork *wk) {
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    PokedexDetailBlend_InitPlanes(wk->blendMain);
}

static void PokedexDetailMap_UpdateGlow(PokedexDetailMapWork *wk) {
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 0, ALL_PLANES, wk->glowEv, 16 - wk->glowEv);
    PokedexCommon_StepGlow(&wk->glowPhase, &wk->glowEv);
}

static void PokedexDetailMap_StartGlow(PokedexDetailMapWork *wk) {
    PokedexCommon_StartGlow(&wk->glowPhase, &wk->glowEv);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 0, ALL_PLANES, wk->glowEv, 16 - wk->glowEv);
}

static void PokedexDetailMap_SetActorsOpaque(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                             PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        if (i != ACTOR_UNKNOWN) {
            ClActor_SetObjMode(wk->actors[i], 0);
        }
    }
    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (wk->places[i].state == PLACE_SHOWN) {
            ClActor_SetObjMode(wk->places[i].actor, 0);
        }
    }
}

static void PokedexDetailMap_SetActorsBlended(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                              PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        if (i != ACTOR_UNKNOWN) {
            ClActor_SetObjMode(wk->actors[i], 1);
        }
    }
    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (wk->places[i].state == PLACE_SHOWN) {
            ClActor_SetObjMode(wk->places[i].actor, 1);
        }
    }
}

static void PokedexDetailMap_SetIconsOpaque(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                            PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (wk->icons[i] != NULL) {
            ClActor_SetObjMode(wk->icons[i], 0);
        }
    }
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG3, ALL_PLANES, 12, 4);
}

static void PokedexDetailMap_SetIconsBlended(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                             PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (wk->icons[i] != NULL) {
            ClActor_SetObjMode(wk->icons[i], 1);
        }
    }
    PokedexDetailBlend_InitPlanes(wk->blendSub);
}

// Whether the player has the event flag that the habitat list needs for the picked place
static BOOL PokedexDetailMap_IsPlaceListed(PokedexDetailMapWork *wk, PokedexDetailCommon *common) {
    GameData *gameData = PokedexDetailCommon_GetGameData(common);
    EventWork *eventWork;
    u16 zone;
    BOOL listed;
    int flag;

    SaveControl_GetPokedexHabitatSave(GameData_GetSaveControl(gameData));
    eventWork = GameData_GetEventWork(gameData);
    zone = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_ZONE);
    listed = FALSE;
    flag = PokedexDetailMap_GetPlaceFlag(zone);
    if (flag >= 0) {
        listed = EventWork_FlagGet(eventWork, flag);
    }
    return listed;
}

static int PokedexDetailMap_GetPlaceFlag(u16 zone) {
    int i;

    for (i = 0; i < PLACE_FLAG_COUNT; i++) {
        if (zone == sPokedexDetailMapPlaceFlags[i].zone) {
            return sPokedexDetailMapPlaceFlags[i].flag;
        }
    }
    return -1;
}

// The touch bar's button that opens the habitat list of the picked place
static void PokedexDetailMap_UpdateButton(PokedexDetailMapParam *param, PokedexDetailMapWork *wk,
                                          PokedexDetailCommon *common, BOOL show) {
    PokeDexSave *pokedex = GameData_GetPokedex(PokedexDetailCommon_GetGameData(common));
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);

    param->place = ZONE_MAX;
    if (show == TRUE) {
        if (PokeDex_IsHabitatListEnabled(pokedex) == TRUE) {
            ClActor_SetVisible(wk->actors[ACTOR_BUTTON], FALSE);
            PokedexDetailTouchbar_SetMapPlaceVisible(touchbar, TRUE);
            if (PokedexDetailMap_SetHabitatKind(wk, common) == TRUE) {
                param->place = TownMapData_GetParam(wk->townmap, wk->place, TOWNMAP_PARAM_ZONE);
                PokedexDetailTouchbar_SetMapPlaceActive(touchbar, TRUE);
                PokedexDetailMap_PrintButton(param, wk, common, TRUE);
            } else {
                PokedexDetailTouchbar_SetMapPlaceActive(touchbar, FALSE);
                PokedexDetailMap_PrintButton(param, wk, common, FALSE);
            }
        } else {
            PokedexDetailTouchbar_SetMapPlaceVisible(touchbar, FALSE);
        }
    } else {
        PokedexDetailTouchbar_SetMapPlaceVisible(touchbar, FALSE);
    }
}
