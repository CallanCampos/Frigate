#ifndef POKEBW2_APP_MONOLITH_MONOLITH_MAIN_H
#define POKEBW2_APP_MONOLITH_MONOLITH_MAIN_H

#include "types.h"
#include "app/monolith.h"
#include "gfl/clact.h"
#include "save/high_link.h"
#include "struct_decls.h"
#include "system/app_printsys_common.h"
#include "system/printsys.h"

// The Entralink monolith's work (monolith_main.c, a guessed name), which its screens share: the graphics, text and
// actors, and the block that each screen's procs get as their param

// The screens, each a proc on the top screen and one on the bottom screen
enum {
    MONOLITH_SCREEN_MENU,
    MONOLITH_SCREEN_PASS_POWER,
    MONOLITH_SCREEN_RECORDS,
    MONOLITH_SCREEN_MISSION,
    MONOLITH_SCREEN_COUNT,
    // Asked for by a screen to close the monolith
    MONOLITH_SCREEN_EXIT = MONOLITH_SCREEN_COUNT,
};

// The menu's items, where MonolithState's menuCursor returns
enum {
    MONOLITH_MENU_MISSION,
    MONOLITH_MENU_PASS_POWER,
    MONOLITH_MENU_RECORDS,
};

// The flag set once the player's first visit to the Entralink is done
#define MONOLITH_FLAG_VISITED 0x986

// The event work that holds the Entralink's part of the story: the scene that waits for the first pass power, and the
// scene after it
#define MONOLITH_SCENE_WORK 0x404d
#define MONOLITH_SCENE_FIRST_POWER 4
#define MONOLITH_SCENE_GOT_POWER 5

// The BG of each screen
#define MONOLITH_BG_MAIN 3
#define MONOLITH_BG_SUB 7

// The palette effects of the screens' panels, one per PaletteFade buffer (monolith_tool.c)
#define PANEL_CONTROL_MAX 4

// A panel's palette effect: none, a pulse that marks the cursor's panel, or a flash that marks a picked one
enum {
    PANEL_MODE_NONE,
    PANEL_MODE_PULSE,
    PANEL_MODE_FLASH,
};

typedef struct {
    // The weight of the blend, in 1/256
    u16 fraction;
    // PANEL_MODE_*
    u8 mode;
    // Whether the pulse is fading back
    u8 falling;
    // The frames until the flash's next step
    u8 wait;
    u8 flashes;
    u8 pad[2];
} MonolithPanelControl;

// What the screens hand to each other
typedef struct {
    u8 unk0;
    // The pass power or mission that the cursor is on, or HIGH_LINK_POWER_NONE
    u8 focusPower;
    // 0 when it can be used, 1 when it is locked or already equipped
    u8 focusPowerState;
    // Set when the equipped pass powers change
    u8 equipChanged;
    // The mission chosen, 0xff for none, and its level
    u8 missionIndex;
    u8 missionLevel;
    // Where the menu's cursor returns
    s8 menuCursor;
    u8 unk7;
    // The equipped pass powers, as in the high link save
    u8 equipped[4];
} MonolithState;

// The param of every screen's procs
typedef struct {
    MonolithParam *param;
    MonolithWork *work;
    MonolithState *state;
    MonolithPanelControl panels[PANEL_CONTROL_MAX];
    // The screen to show next, MONOLITH_SCREEN_*
    u8 next;
    // Asks the top screen's proc to end
    u8 endTop;
    u8 exiting;
} MonolithScreenParam;

// The actors' resources on each screen
typedef struct {
    u32 palette;
    u32 chars;
    u32 cellAnims;
    u32 fontPalette;
} MonolithActorRes;

struct MonolithWork {
    PaletteFade *fade;
    ActorPalSlots *palSlots;
    PrintQueue *printQueue;
    Font *font;
    WordSet *wordSet;
    ClActUnit *clactUnit;
    TCBExManager *tcbMgr;
    // Message files 87 (lines of spaces), 263 (the pass powers' names), 264 (their descriptions) and 89 (the
    // monolith's own text)
    MsgData *msgBlank;
    MsgData *msgPowerNames;
    MsgData *msgPowerInfo;
    MsgData *msgMonolith;
    BmpOamSys *bmpOam;
    ArcTool *arc;
    PassPowerData *passPowerData;
    AppPrintsysCommon printsys;
    MonolithActorRes actorRes[2];
    u8 unk60[0x90];
    MonolithState state;
    MonolithScreenParam screen;
    TCB *vblankTask;
    GameProcManager *topProcMgr;
    GameProcManager *bottomProcMgr;
    // The screen shown, and whether its procs were started
    u8 current;
    u8 topStarted;
    u8 bottomStarted;
    // The characters of the bottom screen's bar
    u32 barChar;
};

#endif // POKEBW2_APP_MONOLITH_MONOLITH_MAIN_H
