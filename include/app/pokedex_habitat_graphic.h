#ifndef POKEBW2_APP_POKEDEX_HABITAT_GRAPHIC_H
#define POKEBW2_APP_POKEDEX_HABITAT_GRAPHIC_H

#include "types.h"
#include "app/townmap/townmap_data.h"
#include "app/ui/ui_scene.h"
#include "gfl/arc.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "struct_decls.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/blink_palanm.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"

// pokedex_habitat_graphic.c (a guess, the ROM has no name for it): what the Habitat List draws: its BGs, its actors and
// the icons of an area's Pokémon on the main screen, the map of the region on the sub screen, its windows and its
// fades. The names are ours

// How many Pokémon an area shows, in rows of POKEDEX_HABITAT_COLUMNS
#define POKEDEX_HABITAT_ICON_COUNT 30
#define POKEDEX_HABITAT_COLUMNS 5

// The actors
enum {
    // The arrows that change the season
    POKEDEX_HABITAT_OBJ_SEASON_LEFT,
    POKEDEX_HABITAT_OBJ_SEASON_RIGHT,
    // The stamp of an area's kind of habitat
    POKEDEX_HABITAT_OBJ_STAMP,
    // The player's place on the map, and the area's
    POKEDEX_HABITAT_OBJ_PLAYER,
    POKEDEX_HABITAT_OBJ_AREA,
    POKEDEX_HABITAT_OBJ_INFO,
    // The marks of the kinds of habitat that have a stamp to give, one for each tab
    POKEDEX_HABITAT_OBJ_NEW,
    // The tabs of the kinds of habitat
    POKEDEX_HABITAT_OBJ_TAB = POKEDEX_HABITAT_OBJ_NEW + 3,
    // The touch bar's buttons
    POKEDEX_HABITAT_OBJ_AREA_UP = POKEDEX_HABITAT_OBJ_TAB + 3,
    POKEDEX_HABITAT_OBJ_AREA_DOWN,
    POKEDEX_HABITAT_OBJ_SHORTCUT,
    POKEDEX_HABITAT_OBJ_EXIT,
    POKEDEX_HABITAT_OBJ_RETURN,
    POKEDEX_HABITAT_OBJ_FRAME,
    // The marks of the Pokémon: seen or caught
    POKEDEX_HABITAT_OBJ_MARK,
    POKEDEX_HABITAT_OBJ_COUNT = POKEDEX_HABITAT_OBJ_MARK + POKEDEX_HABITAT_ICON_COUNT,
};

// The windows: the main screen's, the sub screen's and the message's
enum {
    POKEDEX_HABITAT_WIN_MAIN,
    POKEDEX_HABITAT_WIN_SUB,
    POKEDEX_HABITAT_WIN_MESSAGE,
    POKEDEX_HABITAT_WIN_COUNT,
};

// How PokedexHabitatGraphic_SetTab shows a tab
enum {
    POKEDEX_HABITAT_TAB_CHOSEN,
    POKEDEX_HABITAT_TAB_SHOWN,
    POKEDEX_HABITAT_TAB_HIDDEN,
};

// The palette of a Pokémon's cell
enum {
    POKEDEX_HABITAT_CELL_CURSOR,
    POKEDEX_HABITAT_CELL_FILLED,
    POKEDEX_HABITAT_CELL_EMPTY,
};

// The mark of a Pokémon
enum {
    POKEDEX_HABITAT_MARK_CAUGHT,
    POKEDEX_HABITAT_MARK_SEEN,
    POKEDEX_HABITAT_MARK_NONE,
};

// An icon to load in the next vertical blank
typedef struct {
    BOOL requested;
    u16 species;
    u32 form;
    u32 sex;
} PokedexHabitatIconRequest;

// A place of the map
typedef struct {
    u32 state;
    ClActor *actor;
} PokedexHabitatPlace;

typedef struct {
    GameData *gameData;
    PaletteFade *paletteFade;
    BOOL blinkStopped;
    BlinkPalAnm *blink[2];
    ClActUnit *unit;
    UIObjRes iconRes[POKEDEX_HABITAT_ICON_COUNT];
    ClActor *icons[POKEDEX_HABITAT_ICON_COUNT];
    ArcTool *iconArc;
    PokedexHabitatIconRequest iconRequests[POKEDEX_HABITAT_ICON_COUNT];
    UIObjRes objRes;
    ClActor *actors[POKEDEX_HABITAT_OBJ_COUNT];
    UIObjRes mapRes;
    UIObjRes barRes;
    UIObjRes areaRes;
    // Whether all the text is in VRAM
    BOOL printed;
    Font *font;
    PrintWindow prints[POKEDEX_HABITAT_WIN_COUNT];
    PrintQueue *printQueue;
    BmpWin *windows[POKEDEX_HABITAT_WIN_COUNT];
    AppPrintsysCommon printCommon;
    TCBExManager *tcbManager;
    PrintStream *stream;
    u32 scrollTimer;
    void *townmap;
    // The glow of the area on the map
    int glowEv;
    u16 glowPhase;
    KeyCursor *keyCursor;
    // Whether the message waits for the player, with its cursor shown
    BOOL messageWaiting;
    // The palette of the tabs to load in the next vertical blank, for the kind of habitat
    BOOL paletteRequested;
    u32 paletteType;
    PokedexHabitatPlace places[TOWNMAP_PLACE_COUNT];
} PokedexHabitatGraphic; // size: 0x85c

void PokedexHabitatGraphic_Init(PokedexHabitatGraphic *graphic, GameData *gameData);
void PokedexHabitatGraphic_Exit(PokedexHabitatGraphic *graphic);
void PokedexHabitatGraphic_VBlank(PokedexHabitatGraphic *graphic);
// Whether no icon is waiting to be loaded
BOOL PokedexHabitatGraphic_AreIconsLoaded(PokedexHabitatGraphic *graphic);
void PokedexHabitatGraphic_Main(PokedexHabitatGraphic *graphic);
// Fades the palettes between two steps of 16 toward black
void PokedexHabitatGraphic_StartFade(PokedexHabitatGraphic *graphic, u8 start, u8 end);
u16 PokedexHabitatGraphic_IsFading(PokedexHabitatGraphic *graphic);
BOOL PokedexHabitatGraphic_IsPrinted(PokedexHabitatGraphic *graphic);
// Prints a string in a window, over a cleared rectangle
void PokedexHabitatGraphic_Print(u8 window, StrBuf *strbuf, PokedexHabitatGraphic *graphic, s16 x, s16 y, u16 width,
                                 u16 height, u16 color);
void PokedexHabitatGraphic_ClearArea(u8 window, PokedexHabitatGraphic *graphic, s16 x, s16 y, u16 width, u16 height);
void PokedexHabitatGraphic_FillWindow(u8 window, u8 color, PokedexHabitatGraphic *graphic);
ClActor *PokedexHabitatGraphic_GetActor(PokedexHabitatGraphic *graphic, u32 actor);
// Shows a tab as a POKEDEX_HABITAT_TAB_*
void PokedexHabitatGraphic_SetTab(PokedexHabitatGraphic *graphic, u32 tab, u32 state);
// Asks for a Pokémon's icon in a cell
void PokedexHabitatGraphic_RequestIcon(PokedexHabitatGraphic *graphic, u16 species, u32 form, u32 sex, u8 cell);
// Restarts an icon's animation, and plays or stops it
void PokedexHabitatGraphic_AnimateIcon(PokedexHabitatGraphic *graphic, u8 cell, BOOL play);
void PokedexHabitatGraphic_SetIconVisible(PokedexHabitatGraphic *graphic, u8 cell, BOOL visible);
// Colors a cell as a POKEDEX_HABITAT_CELL_*
void PokedexHabitatGraphic_SetCellPalette(PokedexHabitatGraphic *graphic, u8 cell, u32 state);
// Shows an area's zone on the map
void PokedexHabitatGraphic_SetArea(PokedexHabitatGraphic *graphic, u16 zone);
// Shows a cell's mark as a POKEDEX_HABITAT_MARK_*
void PokedexHabitatGraphic_SetMark(PokedexHabitatGraphic *graphic, u16 cell, u32 state);
// The message: its window at a row of the main screen, its text, its state (PRINT_STREAM_*), the cursor that shows
// once it is done, and its end
void PokedexHabitatGraphic_CreateMessageWindow(PokedexHabitatGraphic *graphic, u8 y, u8 height);
void PokedexHabitatGraphic_StartMessage(StrBuf *strbuf, PokedexHabitatGraphic *graphic);
void PokedexHabitatGraphic_CloseMessage(PokedexHabitatGraphic *graphic);
u32 PokedexHabitatGraphic_GetMessageState(PokedexHabitatGraphic *graphic);
void PokedexHabitatGraphic_WaitMessage(PokedexHabitatGraphic *graphic);
// Shows an actor and plays one of its animations from the start
void PokedexHabitatGraphic_StartActorAnim(PokedexHabitatGraphic *graphic, u32 actor, u32 anim);
void PokedexHabitatGraphic_UpdatePlayerMarker(PokedexHabitatGraphic *graphic);
// Moves the main screen's buttons down, or else the sub screen's BGs up
void PokedexHabitatGraphic_Scroll(PokedexHabitatGraphic *graphic, int value, BOOL objects);
void PokedexHabitatGraphic_RequestPalette(PokedexHabitatGraphic *graphic, u32 type);
BOOL PokedexHabitatGraphic_IsPaletteRequested(PokedexHabitatGraphic *graphic);
void PokedexHabitatGraphic_SetBlinkStopped(PokedexHabitatGraphic *graphic, BOOL stopped);
void PokedexHabitatGraphic_SetNewMarksVisible(PokedexHabitatGraphic *graphic, BOOL visible);

#endif // POKEBW2_APP_POKEDEX_HABITAT_GRAPHIC_H
