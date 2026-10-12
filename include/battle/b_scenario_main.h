#ifndef POKEBW2_BATTLE_B_SCENARIO_MAIN_H
#define POKEBW2_BATTLE_B_SCENARIO_MAIN_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/printsys.h"

// The scenario screen of a Pokéstar Studios filming battle in overlay 288 (b_scenario_main.c, the ROM's embedded
// string, and b_scenario_graphic.c, a guessed name): the film's summary, its script and its cast on the lower screen,
// three pages that the touch screen or the keys turn. It sets done when it ends. Every name here is ours; swan has
// none for this overlay.

// What the battle passes to the scenario screen, 0x1c bytes, inside BtlvCore
typedef struct {
    Font *font;       // 0x00
    HeapID heapId;    // 0x04
    u8 done;          // 0x06
    u8 unk07;         // 0x07
    PokeParty *party; // 0x08  the cast
    u32 scenario;     // 0x0c  the film, 0 to 40
    int scene;        // 0x10  the turn being filmed, from 1, which the script page marks
    BOOL close;       // 0x14  1 leaves with the fade
    BOOL abort;       // 0x18  1 leaves at once
} BScenarioParam;

#define BSCENARIO_WINDOW_MAX 14
#define BSCENARIO_SCENARIO_COUNT 41

// The groups of actors, as BScenarioGraphic_SetActorVisible takes them
enum {
    BSCENARIO_ACTORS_TYPE,   // 0 and 1 the Pokémon's types, 2 to 5 its moves' types
    BSCENARIO_ACTORS_ICON,   // the cast's icons
    BSCENARIO_ACTORS_CURSOR, // the cursor's four corners
    BSCENARIO_ACTORS_BUTTON, // BSCENARIO_BUTTON_*
};

// The actors of the buttons group
enum {
    BSCENARIO_BUTTON_PREV,
    BSCENARIO_BUTTON_NEXT,
    BSCENARIO_BUTTON_BACK,
    BSCENARIO_BUTTON_PLATE,  // shown on the script and cast pages
    BSCENARIO_BUTTON_MARKER, // marks the scene being filmed on the script page
    BSCENARIO_BUTTON_COUNT,
};

// The indices of the kinds of actors in BScenarioWork's palettes and cells
enum {
    BSCENARIO_RES_TYPE,
    BSCENARIO_RES_ICON,
    BSCENARIO_RES_CURSOR,
    BSCENARIO_RES_BUTTON,
    BSCENARIO_RES_COUNT,
};

// The first of each kind of actor's characters in BScenarioWork's chars
#define BSCENARIO_CHARS_TYPE 0
#define BSCENARIO_CHARS_ICON 6
#define BSCENARIO_CHARS_CURSOR 12
#define BSCENARIO_CHARS_BUTTON 13
#define BSCENARIO_CHARS_COUNT 14

typedef struct {
    BmpWin *window;
    u8 dirty;
} BScenarioWindow;

// What the cast page shows of a Pokémon, 0x3c bytes
typedef struct {
    int species;
    int sex;
    int level;
    int item;
    int ability;
    int type1;
    int type2;
    int moves[4];
    int moveTypes[4];
} BScenarioMon;

// The scenario screen's work, 0x2fc bytes, shared by the overlay's files
typedef struct BScenarioWork {
    BScenarioParam *param;                         // 0x000
    TCBExManager *tcbManager;                      // 0x004
    PaletteFade *paletteFade;                      // 0x008
    Font *smallFont;                               // 0x00c
    PrintQueue *printQueue;                        // 0x010
    BScenarioWindow windows[BSCENARIO_WINDOW_MAX]; // 0x014
    MsgData *msgData[2];                           // 0x084  the screen's texts, and the film's script
    WordSet *wordSet;                              // 0x08c
    ClActUnit *unit;                               // 0x090
    ClActor *typeActors[6];                        // 0x094
    ClActor *iconActors[6];                        // 0x0ac
    ClActor *cursorActors[4];                      // 0x0c4
    ClActor *buttonActors[BSCENARIO_BUTTON_COUNT]; // 0x0d4
    u32 palettes[BSCENARIO_RES_COUNT];             // 0x0e8
    u32 chars[BSCENARIO_CHARS_COUNT];              // 0x0f8
    u32 cells[BSCENARIO_RES_COUNT];                // 0x130
    int step;                                      // 0x140
    u32 pageSeq;                                   // 0x144  the page function that runs
    int nextPageSeq;                               // 0x148  taken when a page ends, -1 ends the screen
    u32 flags;                                     // 0x14c
    int pageStep;                                  // 0x150
    int page;                                      // 0x154  0 the summary, 1 the script, 2 the cast
    u32 flashFlags;                                // 0x158
    int flashStep;                                 // 0x15c
    int flashButton;                               // 0x160
    u32 shownWindows;                              // 0x164  a bit per window whose screen was copied
    u32 unk168;                                    // 0x168  never accessed
    int scenario;                                  // 0x16c
    int summaryCursor;                             // 0x170
    int scriptPage;                                // 0x174
    int scriptLines;                               // 0x178
    int scriptCursor;                              // 0x17c
    int castIndex;                                 // 0x180  the Pokémon shown
    int castCount;                                 // 0x184
    int castCursor;                                // 0x188
    s16 flashFrame[2];                             // 0x18c
    s16 *positions;                                // 0x190  the actors' positions, in pairs of x and y
    BScenarioMon mons[6];                          // 0x194
} BScenarioWork;                                   // 0x2fc

// BScenarioWork's flags
#define BSCENARIO_FLAG_REENTER 0x1
// The page changed, so its BG and actors are set at the task's end
#define BSCENARIO_FLAG_PAGE 0x4
#define BSCENARIO_FLAG_REFRESH 0x8
// The keys are used, so the cursor shows
#define BSCENARIO_FLAG_KEYS 0x400
// The screen was left with the keys
#define BSCENARIO_FLAG_KEY_CLOSE 0x800
// The previous and next arrows met the end, and their animations are to be set
#define BSCENARIO_FLAG_PREV_BLOCKED 0x10000
#define BSCENARIO_FLAG_NEXT_BLOCKED 0x20000
#define BSCENARIO_FLAG_PREV_ANIM 0x40000
#define BSCENARIO_FLAG_NEXT_ANIM 0x80000
// The previous and next arrows are greyed out
#define BSCENARIO_FLAG_PREV_END 0x100000
#define BSCENARIO_FLAG_NEXT_END 0x200000

// Starts the scenario screen, which runs as a task
void BScenarioMain_Start(BScenarioParam *param);

#endif // POKEBW2_BATTLE_B_SCENARIO_MAIN_H
