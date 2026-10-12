#ifndef POKEBW2_APP_ZUKAN_HABITAT_GRAPHIC_H
#define POKEBW2_APP_ZUKAN_HABITAT_GRAPHIC_H

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

// zukan_habitat_graphic.c (a guess, the ROM has no name for it): what the Habitat List draws: its BGs, its actors and
// the icons of an area's Pokémon on the main screen, the map of the region on the sub screen, its windows and its
// fades. The names are ours

// How many Pokémon an area shows, in rows of ZUKAN_HABITAT_COLUMNS
#define ZUKAN_HABITAT_ICON_COUNT 30
#define ZUKAN_HABITAT_COLUMNS 5

// The actors
enum {
    // The arrows that change the season
    ZUKAN_HABITAT_OBJ_SEASON_LEFT,
    ZUKAN_HABITAT_OBJ_SEASON_RIGHT,
    // The stamp of an area's kind of habitat
    ZUKAN_HABITAT_OBJ_STAMP,
    // The player's place on the map, and the area's
    ZUKAN_HABITAT_OBJ_PLAYER,
    ZUKAN_HABITAT_OBJ_AREA,
    ZUKAN_HABITAT_OBJ_INFO,
    // The marks of the kinds of habitat that have a stamp to give, one for each tab
    ZUKAN_HABITAT_OBJ_NEW,
    // The tabs of the kinds of habitat
    ZUKAN_HABITAT_OBJ_TAB = ZUKAN_HABITAT_OBJ_NEW + 3,
    // The touch bar's buttons
    ZUKAN_HABITAT_OBJ_AREA_UP = ZUKAN_HABITAT_OBJ_TAB + 3,
    ZUKAN_HABITAT_OBJ_AREA_DOWN,
    ZUKAN_HABITAT_OBJ_SHORTCUT,
    ZUKAN_HABITAT_OBJ_EXIT,
    ZUKAN_HABITAT_OBJ_RETURN,
    ZUKAN_HABITAT_OBJ_FRAME,
    // The marks of the Pokémon: seen or caught
    ZUKAN_HABITAT_OBJ_MARK,
    ZUKAN_HABITAT_OBJ_COUNT = ZUKAN_HABITAT_OBJ_MARK + ZUKAN_HABITAT_ICON_COUNT,
};

// The windows: the main screen's, the sub screen's and the message's
enum {
    ZUKAN_HABITAT_WIN_MAIN,
    ZUKAN_HABITAT_WIN_SUB,
    ZUKAN_HABITAT_WIN_MESSAGE,
    ZUKAN_HABITAT_WIN_COUNT,
};

// How ZukanHabitatGraphic_SetTab shows a tab
enum {
    ZUKAN_HABITAT_TAB_CHOSEN,
    ZUKAN_HABITAT_TAB_SHOWN,
    ZUKAN_HABITAT_TAB_HIDDEN,
};

// The palette of a Pokémon's cell
enum {
    ZUKAN_HABITAT_CELL_CURSOR,
    ZUKAN_HABITAT_CELL_FILLED,
    ZUKAN_HABITAT_CELL_EMPTY,
};

// The mark of a Pokémon
enum {
    ZUKAN_HABITAT_MARK_CAUGHT,
    ZUKAN_HABITAT_MARK_SEEN,
    ZUKAN_HABITAT_MARK_NONE,
};

// An icon to load in the next vertical blank
typedef struct {
    BOOL requested;
    u16 species;
    u32 form;
    u32 sex;
} ZukanHabitatIconRequest;

// A place of the map
typedef struct {
    u32 state;
    ClActor *actor;
} ZukanHabitatPlace;

typedef struct {
    GameData *gameData;
    PaletteFade *paletteFade;
    BOOL blinkStopped;
    BlinkPalAnm *blink[2];
    ClActUnit *unit;
    UIObjRes iconRes[ZUKAN_HABITAT_ICON_COUNT];
    ClActor *icons[ZUKAN_HABITAT_ICON_COUNT];
    ArcTool *iconArc;
    ZukanHabitatIconRequest iconRequests[ZUKAN_HABITAT_ICON_COUNT];
    UIObjRes objRes;
    ClActor *actors[ZUKAN_HABITAT_OBJ_COUNT];
    UIObjRes mapRes;
    UIObjRes barRes;
    UIObjRes areaRes;
    // Whether all the text is in VRAM
    BOOL printed;
    Font *font;
    PrintWindow prints[ZUKAN_HABITAT_WIN_COUNT];
    PrintQueue *printQueue;
    BmpWin *windows[ZUKAN_HABITAT_WIN_COUNT];
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
    ZukanHabitatPlace places[TOWNMAP_PLACE_COUNT];
} ZukanHabitatGraphic; // size: 0x85c

void ZukanHabitatGraphic_Init(ZukanHabitatGraphic *graphic, GameData *gameData);
void ZukanHabitatGraphic_Exit(ZukanHabitatGraphic *graphic);
void ZukanHabitatGraphic_VBlank(ZukanHabitatGraphic *graphic);
// Whether no icon is waiting to be loaded
BOOL ZukanHabitatGraphic_AreIconsLoaded(ZukanHabitatGraphic *graphic);
void ZukanHabitatGraphic_Main(ZukanHabitatGraphic *graphic);
// Fades the palettes between two steps of 16 toward black
void ZukanHabitatGraphic_StartFade(ZukanHabitatGraphic *graphic, u8 start, u8 end);
u16 ZukanHabitatGraphic_IsFading(ZukanHabitatGraphic *graphic);
BOOL ZukanHabitatGraphic_IsPrinted(ZukanHabitatGraphic *graphic);
// Prints a string in a window, over a cleared rectangle
void ZukanHabitatGraphic_Print(u8 window, StrBuf *strbuf, ZukanHabitatGraphic *graphic, s16 x, s16 y, u16 width,
                               u16 height, u16 color);
void ZukanHabitatGraphic_ClearArea(u8 window, ZukanHabitatGraphic *graphic, s16 x, s16 y, u16 width, u16 height);
void ZukanHabitatGraphic_FillWindow(u8 window, u8 color, ZukanHabitatGraphic *graphic);
ClActor *ZukanHabitatGraphic_GetActor(ZukanHabitatGraphic *graphic, u32 actor);
// Shows a tab as a ZUKAN_HABITAT_TAB_*
void ZukanHabitatGraphic_SetTab(ZukanHabitatGraphic *graphic, u32 tab, u32 state);
// Asks for a Pokémon's icon in a cell
void ZukanHabitatGraphic_RequestIcon(ZukanHabitatGraphic *graphic, u16 species, u32 form, u32 sex, u8 cell);
// Restarts an icon's animation, and plays or stops it
void ZukanHabitatGraphic_AnimateIcon(ZukanHabitatGraphic *graphic, u8 cell, BOOL play);
void ZukanHabitatGraphic_SetIconVisible(ZukanHabitatGraphic *graphic, u8 cell, BOOL visible);
// Colors a cell as a ZUKAN_HABITAT_CELL_*
void ZukanHabitatGraphic_SetCellPalette(ZukanHabitatGraphic *graphic, u8 cell, u32 state);
// Shows an area's zone on the map
void ZukanHabitatGraphic_SetArea(ZukanHabitatGraphic *graphic, u16 zone);
// Shows a cell's mark as a ZUKAN_HABITAT_MARK_*
void ZukanHabitatGraphic_SetMark(ZukanHabitatGraphic *graphic, u16 cell, u32 state);
// The message: its window at a row of the main screen, its text, its state (PRINT_STREAM_*), the cursor that shows
// once it is done, and its end
void ZukanHabitatGraphic_CreateMessageWindow(ZukanHabitatGraphic *graphic, u8 y, u8 height);
void ZukanHabitatGraphic_StartMessage(StrBuf *strbuf, ZukanHabitatGraphic *graphic);
void ZukanHabitatGraphic_CloseMessage(ZukanHabitatGraphic *graphic);
u32 ZukanHabitatGraphic_GetMessageState(ZukanHabitatGraphic *graphic);
void ZukanHabitatGraphic_WaitMessage(ZukanHabitatGraphic *graphic);
// Shows an actor and plays one of its animations from the start
void ZukanHabitatGraphic_StartActorAnim(ZukanHabitatGraphic *graphic, u32 actor, u32 anim);
void ZukanHabitatGraphic_UpdatePlayerMarker(ZukanHabitatGraphic *graphic);
// Moves the main screen's buttons down, or else the sub screen's BGs up
void ZukanHabitatGraphic_Scroll(ZukanHabitatGraphic *graphic, int value, BOOL objects);
void ZukanHabitatGraphic_RequestPalette(ZukanHabitatGraphic *graphic, u32 type);
BOOL ZukanHabitatGraphic_IsPaletteRequested(ZukanHabitatGraphic *graphic);
void ZukanHabitatGraphic_SetBlinkStopped(ZukanHabitatGraphic *graphic, BOOL stopped);
void ZukanHabitatGraphic_SetNewMarksVisible(ZukanHabitatGraphic *graphic, BOOL visible);

#endif // POKEBW2_APP_ZUKAN_HABITAT_GRAPHIC_H
