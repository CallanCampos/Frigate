#include "battle/btlv_scd.h"
#include "types.h"
#include "battle/btl_action.h"
#include "battle/btl_client.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_pospoke.h"
#include "battle/btlv.h"
#include "battle/btlv_effect.h"
#include "battle/btlv_input.h"
#include "constants/battle.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/mi.h"
#include "pml/move.h"
#include "pml/poke_party.h"
#include "system/game_data.h"
#include "system/printsys.h"

#define NUM_PROCS 4

// What BtlMain gets the positions of a group of targets by: a group, and the position of the user
#define TARGET_FLAGS(group, pos) (((group) << 8) | (pos))

// The target screen's range for MOVE_TARGET_SELECTED when a move with MOVE_FLAG_DISTANT is used in a triple battle,
// after the move data's ranges
#define TARGET_RANGE_DISTANT 14

// The steps of the command screen: wait for a command, then, after Run, for the screen to close
enum {
    COMMAND_SEQ_SELECT,
    COMMAND_SEQ_WAIT_CLOSE,
};

// The steps of the yes/no screen: wait for the answer, then for the screen to close
enum {
    YES_NO_SEQ_SELECT,
    YES_NO_SEQ_WAIT_CLOSE,
};

// The steps of a forced quit of the input: wait for it to be idle, close the screen, wait for that, and end
enum {
    FORCE_QUIT_STEP_WAIT_INPUT,
    FORCE_QUIT_STEP_CLOSE,
    FORCE_QUIT_STEP_WAIT_CLOSE,
    FORCE_QUIT_STEP_DONE,
};

// The move screen's buttons after the four moves (the rotation screen's are numbered the same), and the bit the input
// sets on the button pressed with L held, which marks the action with BattleAction_SetMoveInfoFlag
enum {
    MOVE_BUTTON_BACK = 4,
    MOVE_BUTTON_SHIFT,
    MOVE_BUTTON_STRUGGLE = 7,
};
#define MOVE_BUTTON_L_HELD 0x8000

// The target screen's back button, after the panels of a double and of a triple battle
enum {
    TARGET_BUTTON_BACK_DOUBLE = 4,
    TARGET_BUTTON_BACK_TRIPLE = 6,
};

// How the target screen offers its targets: the mons to choose among, or all of a group
enum {
    TARGETS_SELECT,
    TARGETS_GROUP,
    TARGETS_SIDE,
};

// The positions a move can aim at, which the target screen shows. When a choice is to be made the count is in
// selectCount, and the positions are what the choice is among; otherwise count is the number the move hits
typedef struct BtlvScdTargetList {
    u8 selectCount;
    u8 count;
    u8 kind; // TARGETS_GROUP or TARGETS_SIDE, when selectCount is 0
    u8 pos[6];
} BtlvScdTargetList;

struct BtlvScd {
    u8 unk00[8];
    Font *font;                   // 0x08
    StrBuf *strbuf;               // 0x0C 0x80 characters, never used
    BtlClient *client;            // 0x10
    PrintQueue *printQueue;       // 0x14
    u8 unk18[8];                  // 0x18
    BtlvSubProc procs[NUM_PROCS]; // 0x20 a stack of the procedures that are running; the last one runs
    u32 numProcs;                 // 0x60
    u8 unk64[4];
    BtlvSubProcFn moveSelectMain; // 0x68 what the move screens run, to go back to them
    BattleMon *mon;               // 0x6C
    BattleAction *action;         // 0x70 the choice of the player
    u32 unk74;                    // 0x74 only cleared
    u8 unk78[4];
    BtlvSelectTargetParam *targetParam; // 0x7C the mons of a rotation battle
    BtlvScdTargetList targets;          // 0x80
    u8 targetChosen;                    // 0x89
    u8 moveRange;                       // 0x8A the move's target type
    u8 slot;                            // 0x8B the number of the mon choosing this turn
    s8 fadeFrom;                        // 0x8C the fade's brightness, which nothing reads
    s8 fadeTo;                          // 0x8D
    s8 fadeStep;                        // 0x8E
    u8 keyCursorFlag;                   // 0x8F set by the input when the player uses the keys
    u8 scdState;                        // 0x90
    u8 shooterEnergy;                   // 0x91 the Wonder Launcher's points
    u8 step : 7;                        // 0x92 the step of the forced quit
    u8 forceQuit : 1;                   //      set while it runs
    u8 yesNoSeq;                        // 0x93
    u8 typeCount;                       // 0x94
    u32 typeMask;                       // 0x98
    u32 answer;                         // 0x9C the answer of the yes/no screen
    BOOL noCancel;                      // 0xA0
    u32 result;                         // 0xA4 the command, or the demo's end
    BtlvCore *core;                     // 0xA8 only stored
    BtlMainModule *mainModule;          // 0xAC
    BtlPokeCon *pokeCon;                // 0xB0
    BattleParty *party;                 // 0xB4 the client's own
    HeapID heapId;                      // 0xB8
    BtlvInput *input;                   // 0xBC
}; // size 0xC0

// The screens' buttons (the command screen's, the move screen's, its Shift variant for the side mons of a triple
// battle, yes or no, the recorder's, and the target screen's, by the user's position and the move's range), the
// cursor stops that go with them, and the tables of both by position and range. The target screen's are named after
// the battle, the range (as MOVE_TARGET_*: Single flashes each panel alone, Distant is TARGET_RANGE_DISTANT) and the
// user's place on the player's side, Left, Center or Right, where they differ by it. They are declared in the order
// that makes MWCC's size sort lay .rodata out as the game's
static const BtlvInputKeyStop sTargetStopsTripleAll[2] = {
    { { 5, 1, 0, 4, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 5, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsDoubleUserOrAlly[3] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 4 },
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 2, 4 },
    { { 4, 4, 4, 4, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sTargetStopsDoubleUserSide[2] = {
    { { 0, 2, 0, 2, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sTargetStopsTripleAllyCenter[3] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 6 },
    { { 4, 4, 4, 4, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleAllAdjacentCenter[2] = {
    { { 5, 1, 0, 4, 4, 0 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 5, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleFoes[2] = {
    { { 5, 1, 5, 1, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 5, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleFoeLeft[3] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 3, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleAllAdjacentRight[2] = {
    { { 3, 1, 2, 2, -1, 1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleFoeCenter[4] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 3, 0, 2, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 1, KEY_CURSOR_NONE, 1, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sCommandStops[4] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -1, 1, 2, 0, -3 },
    { { 1, 1, 1, 1, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 1, -3 },
    { { 2, 2, 2, 2, -1, -1 }, 0, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 2, -3 },
    { { 3, 3, 3, 3, -1, -1 }, 0, KEY_CURSOR_NONE, 1, 2, 3, -3 },
};
static const BtlvInputKeyStop sTargetStopsTripleUserSide[2] = {
    { { 0, 4, 0, 4, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleLeft[2] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleAdjacentFoesRight[2] = {
    { { 3, 1, 3, 1, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleAdjacentFoesLeft[2] = {
    { { 5, 3, 5, 3, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 5, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const TouchRect sMoveRects[6] = {
    { 0x20, 0x50, 0x00, 0x80 }, { 0x20, 0x50, 0x80, 0xff }, { 0x50, 0x80, 0x00, 0x80 },
    { 0x50, 0x80, 0x80, 0xff }, { 0x90, 0xc0, 0xb0, 0xff }, { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const TouchRect sCommandRects[5] = {
    { 0x18, 0x90, 0x00, 0xff }, { 0x90, 0xc0, 0x00, 0x50 },           { 0x90, 0xc0, 0xb0, 0xff },
    { 0x98, 0xc0, 0x58, 0xa8 }, { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const TouchRect sTargetRectsDouble[6] = {
    { 0x58, 0x80, 0x00, 0x80 }, { 0x20, 0x58, 0x80, 0xff }, { 0x58, 0x80, 0x80, 0xff },
    { 0x20, 0x58, 0x00, 0x80 }, { 0x90, 0xc0, 0xb0, 0xff }, { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const BtlvInputKeyStop sTargetStopsDoubleSelectedLeft[4] = {
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 3, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 2, 2, 2, 2, -1, -1 }, 1, 3, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 4 },
    { { 4, 4, 4, 4, -1, -1 }, -2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sTargetStopsTripleAllAdjacentLeft[2] = {
    { { 5, 3, 5, 2, 2, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 5, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const s32 sTargetFlashRowsTripleSingle[7] = { 0x400, 0x200, 0x800, 0x100, 0x1000, 0x80, 0x10 };
static const s32 sTargetFlashRowsTripleAllAdjacentLeft[7] = { 0x980, 0x980, 0x980, 0x980, 0x980, 0x980, 0x10 };
static const s32 sTargetFlashRowsTripleAllAdjacentCenter[7] = { 0x1780, 0x1780, 0x1780, 0x1780, 0x1780, 0x1780, 0x10 };
static const BtlvInputKeyStop sTargetStopsTripleCenter[2] = {
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsDoubleFoes[2] = {
    { { 3, 1, 3, 1, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sTargetStopsTripleRight[2] = {
    { { 4, 4, 4, 4, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsDoubleSelectedRight[4] = {
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 3, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 0, 0, 0, 0, -1, -1 }, 0, 3, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 4, 4, 4, 4, -1, -1 }, -2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sTargetStopsTripleUserOrAllyLeft[3] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 6 },
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleSelectedLeft[4] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 3, 6 },
    { { 2, 2, 2, 2, -1, -1 }, 1, 3, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sYesNoStops[2] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 1 },
    { { 1, 1, 1, 1, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 1, 1 },
};
const u8 BTLV_SCD_TARGET_STOP_COUNTS_DOUBLE[2][15] = {
    { 4, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4 },
    { 4, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4 },
};
static const u8 sCommandResults[4] = { BTLV_COMMAND_FIGHT, BTLV_COMMAND_BAG, BTLV_COMMAND_POKEMON, BTLV_COMMAND_RUN };
static const s32 sMoveFlagsShift[6] = { 0, 0, 0, 0, 1, 0 };
static const s32 sTargetFlashRowsTripleFoes[7] = { 0x380, 0x380, 0x380, 0x380, 0x380, 0x380, 0x10 };
static const s32 sTargetFlashRowsDoubleAll[5] = { 0x780, 0x780, 0x780, 0x780, 0x10 };
static const s32 sTargetFlashRowsTripleAdjacentFoesRight[7] = { 0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x10 };
static const s32 sMoveFlashRows[5] = { 0x200, 0x400, 0x800, 0x1000, 0x10 };
static const s32 sTargetFlashRowsTripleUserSide[7] = { 0x1c00, 0x1c00, 0x1c00, 0x1c00, 0x1c00, 0x1c00, 0x10 };
static const s32 sCommandFlashRows[4] = { 0x10003, 4, 8, 0x10 };
static const s32 sTargetFlashRowsTripleAll[7] = { 0x1f80, 0x1f80, 0x1f80, 0x1f80, 0x1f80, 0x1f80, 0x10 };
static const s32 sTargetFlashRowsDoubleSingle[5] = { 0x200, 0x100, 0x400, 0x80, 0x10 };
static const s32 sRecorderFlashRows[3] = { 4, 8, 0x10 };
static const s32 sTargetFlashRowsDoubleAllAdjacentRight[5] = { 0x380, 0x380, 0x380, 0x380, 0x10 };
static const s32 sYesNoFlashRows[2] = { 4, 8 };
static const TouchRect sTargetRectsTriple[8] = {
    { 0x58, 0x80, 0x08, 0x58 }, { 0x20, 0x58, 0xa8, 0xf8 },           { 0x58, 0x80, 0x58, 0xa8 },
    { 0x20, 0x58, 0x58, 0xa8 }, { 0x58, 0x80, 0xa8, 0xf8 },           { 0x20, 0x58, 0x08, 0x58 },
    { 0x90, 0xc0, 0xb0, 0xff }, { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const BtlvInputKeyStop sMoveStops[5] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 2, 2, 2, 2, -1, -1 }, 0, 4, KEY_CURSOR_NONE, 3, 2, 4 },
    { { 3, 3, 3, 3, -1, -1 }, 1, 4, 2, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, -2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sTargetStopsDoubleAllAdjacentRight[2] = {
    { { 3, 1, 0, 1, -1, 0 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sTargetStopsDoubleFoe[3] = {
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 3, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 4, 4, 4, 4, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sTargetStopsDoubleLeft[2] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sTargetStopsTripleAllyOuter[2] = {
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleSelectedRight[4] = {
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 0, KEY_CURSOR_NONE, 1, 6 },
    { { 2, 2, 2, 2, -1, -1 }, 0, 3, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleUserOrAllyRight[3] = {
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 2, 6 },
    { { 4, 4, 4, 4, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleSelectedCenter[6] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 5, 0, 2, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 4, 1, KEY_CURSOR_NONE, 1, 6 },
    { { 0, 0, 0, 0, -1, -1 }, 0, 5, KEY_CURSOR_NONE, 4, 0, 6 },
    { { 4, 4, 4, 4, -1, -1 }, 2, 5, 3, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsDoubleAllAdjacentLeft[2] = {
    { { 3, 1, 3, 2, 2, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sTargetStopsDoubleRight[2] = {
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sMoveStopsShift[6] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 2, 2, 2, 2, -1, -1 }, 0, 5, KEY_CURSOR_NONE, 3, 2, 4 },
    { { 3, 3, 3, 3, -1, -1 }, 1, 4, 2, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 3, KEY_CURSOR_NONE, 5, KEY_CURSOR_NONE, 4, 4 },
    { { 5, 5, 5, 5, -1, -1 }, 2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 5, 4 },
};
static const BtlvInputKeyStop sTargetStopsTripleDistantLeft[6] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 5, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 3, 0, 2, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 4, 1, KEY_CURSOR_NONE, 1, 6 },
    { { 2, 2, 2, 2, -1, -1 }, 1, 5, KEY_CURSOR_NONE, 4, 2, 6 },
    { { 4, 4, 4, 4, -1, -1 }, 2, 5, 3, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleUserOrAllyCenter[4] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 0, 6 },
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 3, 0, 2, 2, 6 },
    { { 4, 4, 4, 4, -1, -1 }, KEY_CURSOR_NONE, 3, 1, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleDistantRight[6] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 4, 0, 2, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 5, 1, KEY_CURSOR_NONE, 1, 6 },
    { { 0, 0, 0, 0, -1, -1 }, 0, 5, KEY_CURSOR_NONE, 4, 0, 6 },
    { { 2, 2, 2, 2, -1, -1 }, 2, 5, 3, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsTripleFoeRight[3] = {
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 1, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop sTargetStopsDoubleAll[2] = {
    { { 3, 1, 0, 2, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop sRecorderStops[3] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE },
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 1, 2, KEY_CURSOR_NONE },
};
static const s32 sTargetFlashRowsDoubleAllAdjacentLeft[5] = { 0x580, 0x580, 0x580, 0x580, 0x10 };
static const s32 sMoveFlashRowsShift[6] = { 0x200, 0x400, 0x800, 0x1000, 0x10, 4 };
static const s32 sTargetFlashRowsDoubleUserSide[5] = { 0x600, 0x600, 0x600, 0x600, 0x10 };
static const s32 sTargetFlashRowsDoubleFoes[5] = { 0x180, 0x180, 0x180, 0x180, 0x10 };
static const s32 sTargetFlashRowsTripleAllAdjacentRight[7] = { 0xb00, 0xb00, 0xb00, 0xb00, 0xb00, 0xb00, 0x10 };
static const s32 sTargetFlashRowsTripleAdjacentFoesLeft[7] = { 0x180, 0x180, 0x180, 0x180, 0x180, 0x180, 0x10 };
static const TouchRect sMoveRectsShift[7] = {
    { 0x20, 0x50, 0x00, 0x80 },           { 0x20, 0x50, 0x80, 0xff }, { 0x50, 0x80, 0x00, 0x80 },
    { 0x50, 0x80, 0x80, 0xff },           { 0x90, 0xc0, 0xb0, 0xff }, { 0x90, 0xc0, 0x00, 0x50 },
    { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const TouchRect sYesNoRects[3] = {
    { 0x20, 0x50, 0x00, 0xff },
    { 0x50, 0x80, 0x00, 0xff },
    { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const TouchRect sRecorderRects[4] = {
    { 0x90, 0xc0, 0x00, 0x50 },
    { 0x90, 0xc0, 0xb0, 0xff },
    { 0x98, 0xc0, 0x58, 0xa8 },
    { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const s32 sCommandFlags[4] = { 0, 0, 0, 2 };
static const s32 sRecorderFlags[3] = { 0, 0, 0 };
static const s32 sYesNoFlags[2] = { 0, 0 };
static const s32 sTargetFlagsTriple[7] = { 0, 0, 0, 0, 0, 0, 3 };
static const s32 sTargetFlagsDouble[5] = { 0, 0, 0, 0, 1 };
static const BtlvInputButtonSet sTargetButtonsDoubleAll = { sTargetRectsDouble, sTargetFlagsDouble,
                                                            sTargetFlashRowsDoubleAll };
static const BtlvInputButtonSet sTargetButtonsTripleAllAdjacentLeft = { sTargetRectsTriple, sTargetFlagsTriple,
                                                                        sTargetFlashRowsTripleAllAdjacentLeft };
static const BtlvInputButtonSet sTargetButtonsDoubleAllAdjacentRight = { sTargetRectsDouble, sTargetFlagsDouble,
                                                                         sTargetFlashRowsDoubleAllAdjacentRight };
static const BtlvInputButtonSet sTargetButtonsTripleAllAdjacentRight = { sTargetRectsTriple, sTargetFlagsTriple,
                                                                         sTargetFlashRowsTripleAllAdjacentRight };
static const BtlvInputButtonSet sTargetButtonsDoubleFoes = { sTargetRectsDouble, sTargetFlagsDouble,
                                                             sTargetFlashRowsDoubleFoes };
static const BtlvInputButtonSet sTargetButtonsTripleSingle = { sTargetRectsTriple, sTargetFlagsTriple,
                                                               sTargetFlashRowsTripleSingle };
static const BtlvInputButtonSet sTargetButtonsTripleAdjacentFoesLeft = { sTargetRectsTriple, sTargetFlagsTriple,
                                                                         sTargetFlashRowsTripleAdjacentFoesLeft };
static const BtlvInputButtonSet sTargetButtonsTripleUserSide = { sTargetRectsTriple, sTargetFlagsTriple,
                                                                 sTargetFlashRowsTripleUserSide };
static const BtlvInputButtonSet sTargetButtonsTripleFoes = { sTargetRectsTriple, sTargetFlagsTriple,
                                                             sTargetFlashRowsTripleFoes };
const u8 BTLV_SCD_TARGET_STOP_COUNTS_TRIPLE[3][15] = {
    { 4, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6 },
    { 6, 4, 3, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6 },
    { 4, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6 },
};
static const u8 sTargetButtonViewPos[6] = { 2, 3, 4, 5, 6, 7 };
const BtlvInputKeyStop *BTLV_SCD_TARGET_STOPS_DOUBLE[2][15] = {
    { sTargetStopsDoubleSelectedLeft, sTargetStopsDoubleUserOrAlly, sTargetStopsDoubleRight, sTargetStopsDoubleFoe,
      sTargetStopsDoubleAllAdjacentLeft, sTargetStopsDoubleFoes, sTargetStopsDoubleUserSide, sTargetStopsDoubleLeft,
      sTargetStopsDoubleAll, sTargetStopsDoubleLeft, sTargetStopsDoubleAll, sTargetStopsDoubleFoes,
      sTargetStopsDoubleUserSide, sTargetStopsDoubleLeft, sTargetStopsDoubleSelectedLeft },
    { sTargetStopsDoubleSelectedRight, sTargetStopsDoubleUserOrAlly, sTargetStopsDoubleLeft, sTargetStopsDoubleFoe,
      sTargetStopsDoubleAllAdjacentRight, sTargetStopsDoubleFoes, sTargetStopsDoubleUserSide, sTargetStopsDoubleRight,
      sTargetStopsDoubleAll, sTargetStopsDoubleRight, sTargetStopsDoubleAll, sTargetStopsDoubleFoes,
      sTargetStopsDoubleUserSide, sTargetStopsDoubleRight, sTargetStopsDoubleSelectedRight },
};
static const s32 sMoveFlags[5] = { 0, 0, 0, 0, 1 };
static const BtlvInputButtonSet sTargetButtonsDoubleAllAdjacentLeft = { sTargetRectsDouble, sTargetFlagsDouble,
                                                                        sTargetFlashRowsDoubleAllAdjacentLeft };
static const BtlvInputButtonSet sCommandButtons = { sCommandRects, sCommandFlags, sCommandFlashRows };
static const BtlvInputButtonSet sRecorderButtons = { sRecorderRects, sRecorderFlags, sRecorderFlashRows };
static const BtlvInputButtonSet sYesNoButtons = { sYesNoRects, sYesNoFlags, sYesNoFlashRows };
static const BtlvInputButtonSet sTargetButtonsTripleAllAdjacentCenter = { sTargetRectsTriple, sTargetFlagsTriple,
                                                                          sTargetFlashRowsTripleAllAdjacentCenter };
static const BtlvInputButtonSet sMoveButtons = { sMoveRects, sMoveFlags, sMoveFlashRows };
static const BtlvInputButtonSet sMoveButtonsShift = { sMoveRectsShift, sMoveFlagsShift, sMoveFlashRowsShift };
static const BtlvInputButtonSet sTargetButtonsDoubleUserSide = { sTargetRectsDouble, sTargetFlagsDouble,
                                                                 sTargetFlashRowsDoubleUserSide };
static const BtlvInputButtonSet sTargetButtonsDoubleSingle = { sTargetRectsDouble, sTargetFlagsDouble,
                                                               sTargetFlashRowsDoubleSingle };
static const BtlvInputButtonSet sTargetButtonsTripleAll = { sTargetRectsTriple, sTargetFlagsTriple,
                                                            sTargetFlashRowsTripleAll };
static const BtlvInputButtonSet sTargetButtonsTripleAdjacentFoesRight = { sTargetRectsTriple, sTargetFlagsTriple,
                                                                          sTargetFlashRowsTripleAdjacentFoesRight };
static const BtlvInputButtonSet *sTargetButtonSetsTriple[3][15] = {
    { &sTargetButtonsTripleSingle, &sTargetButtonsTripleSingle, &sTargetButtonsTripleSingle,
      &sTargetButtonsTripleSingle, &sTargetButtonsTripleAllAdjacentLeft, &sTargetButtonsTripleAdjacentFoesLeft,
      &sTargetButtonsTripleUserSide, &sTargetButtonsTripleSingle, &sTargetButtonsTripleAll, &sTargetButtonsTripleSingle,
      &sTargetButtonsTripleAll, &sTargetButtonsTripleFoes, &sTargetButtonsTripleUserSide, &sTargetButtonsTripleSingle,
      &sTargetButtonsTripleSingle },
    { &sTargetButtonsTripleSingle, &sTargetButtonsTripleSingle, &sTargetButtonsTripleSingle,
      &sTargetButtonsTripleSingle, &sTargetButtonsTripleAllAdjacentCenter, &sTargetButtonsTripleFoes,
      &sTargetButtonsTripleUserSide, &sTargetButtonsTripleSingle, &sTargetButtonsTripleAll, &sTargetButtonsTripleSingle,
      &sTargetButtonsTripleAll, &sTargetButtonsTripleFoes, &sTargetButtonsTripleUserSide, &sTargetButtonsTripleSingle,
      &sTargetButtonsTripleSingle },
    { &sTargetButtonsTripleSingle, &sTargetButtonsTripleSingle, &sTargetButtonsTripleSingle,
      &sTargetButtonsTripleSingle, &sTargetButtonsTripleAllAdjacentRight, &sTargetButtonsTripleAdjacentFoesRight,
      &sTargetButtonsTripleUserSide, &sTargetButtonsTripleSingle, &sTargetButtonsTripleAll, &sTargetButtonsTripleSingle,
      &sTargetButtonsTripleAll, &sTargetButtonsTripleFoes, &sTargetButtonsTripleUserSide, &sTargetButtonsTripleSingle,
      &sTargetButtonsTripleSingle },
};
const BtlvInputKeyStop *BTLV_SCD_TARGET_STOPS_TRIPLE[3][15] = {
    { sTargetStopsTripleSelectedLeft, sTargetStopsTripleUserOrAllyLeft, sTargetStopsTripleAllyOuter,
      sTargetStopsTripleFoeLeft, sTargetStopsTripleAllAdjacentLeft, sTargetStopsTripleAdjacentFoesLeft,
      sTargetStopsTripleUserSide, sTargetStopsTripleLeft, sTargetStopsTripleAll, sTargetStopsTripleLeft,
      sTargetStopsTripleAll, sTargetStopsTripleFoes, sTargetStopsTripleUserSide, sTargetStopsTripleLeft,
      sTargetStopsTripleDistantLeft },
    { sTargetStopsTripleSelectedCenter, sTargetStopsTripleUserOrAllyCenter, sTargetStopsTripleAllyCenter,
      sTargetStopsTripleFoeCenter, sTargetStopsTripleAllAdjacentCenter, sTargetStopsTripleFoes,
      sTargetStopsTripleUserSide, sTargetStopsTripleCenter, sTargetStopsTripleAll, sTargetStopsTripleCenter,
      sTargetStopsTripleAll, sTargetStopsTripleFoes, sTargetStopsTripleUserSide, sTargetStopsTripleCenter,
      sTargetStopsTripleSelectedCenter },
    { sTargetStopsTripleSelectedRight, sTargetStopsTripleUserOrAllyRight, sTargetStopsTripleAllyOuter,
      sTargetStopsTripleFoeRight, sTargetStopsTripleAllAdjacentRight, sTargetStopsTripleAdjacentFoesRight,
      sTargetStopsTripleUserSide, sTargetStopsTripleRight, sTargetStopsTripleAll, sTargetStopsTripleRight,
      sTargetStopsTripleAll, sTargetStopsTripleFoes, sTargetStopsTripleUserSide, sTargetStopsTripleRight,
      sTargetStopsTripleDistantRight },
};
static const BtlvInputButtonSet *sTargetButtonSetsDouble[2][15] = {
    { &sTargetButtonsDoubleSingle, &sTargetButtonsDoubleSingle, &sTargetButtonsDoubleSingle,
      &sTargetButtonsDoubleSingle, &sTargetButtonsDoubleAllAdjacentLeft, &sTargetButtonsDoubleFoes,
      &sTargetButtonsDoubleUserSide, &sTargetButtonsDoubleSingle, &sTargetButtonsDoubleAll, &sTargetButtonsDoubleSingle,
      &sTargetButtonsDoubleAll, &sTargetButtonsDoubleFoes, &sTargetButtonsDoubleUserSide, &sTargetButtonsDoubleSingle,
      &sTargetButtonsDoubleSingle },
    { &sTargetButtonsDoubleSingle, &sTargetButtonsDoubleSingle, &sTargetButtonsDoubleSingle,
      &sTargetButtonsDoubleSingle, &sTargetButtonsDoubleAllAdjacentRight, &sTargetButtonsDoubleFoes,
      &sTargetButtonsDoubleUserSide, &sTargetButtonsDoubleSingle, &sTargetButtonsDoubleAll, &sTargetButtonsDoubleSingle,
      &sTargetButtonsDoubleAll, &sTargetButtonsDoubleFoes, &sTargetButtonsDoubleUserSide, &sTargetButtonsDoubleSingle,
      &sTargetButtonsDoubleSingle },
};

static void BtlvScd_Nop(BtlvScd *scd);
static void BtlvScd_ClearProcs(BtlvScd *scd);
static void BtlvScd_PushProc(BtlvScd *scd, BtlvSubProcFn init, BtlvSubProcFn main);
static BOOL BtlvScd_RunProcs(BtlvScd *scd);
static BOOL BtlvScd_CommandInit(s32 *seq, void *arg);
static BOOL BtlvScd_CommandMain(s32 *seq, void *arg);
static BOOL BtlvScd_FingerDemoMain(s32 *seq, void *arg);
static BOOL BtlvScd_MoveInit(s32 *seq, void *arg);
static BOOL BtlvScd_MoveMain(s32 *seq, void *arg);
static BOOL BtlvScd_RotationInit(s32 *seq, void *arg);
static BOOL BtlvScd_RotationMain(s32 *seq, void *arg);
static void BtlvScd_ClearTargetList(BtlvScdTargetList *list);
static void BtlvScd_FillSelectTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 posFlags);
static void BtlvScd_FillGroupTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 posFlags);
static void BtlvScd_FillSideTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 posFlags);
static void BtlvScd_AddFarTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 move, u8 pos);
static BOOL BtlvScd_GetTargetPos(BtlvScdTargetList *list, u32 button, BtlMainModule *mainModule, u8 *outPos);
static void BtlvScd_SetTargetScreen(const u8 *positions, u8 count, u32 kind, BtlvScd *scd);
static void BtlvScd_ShowTargetScreen(BtlvScdTargetList *list, BtlvScd *scd);
static BOOL BtlvScd_TargetInit(s32 *seq, void *arg);
static BOOL BtlvScd_TargetMain(s32 *seq, void *arg);
static void BtlvScd_BuildTargetList(BtlvScdTargetList *list, BtlvScd *scd);

BtlvScd *BtlvScd_Create(BtlvCore *core, BtlMainModule *mainModule, BtlPokeCon *pokeCon, TCBExManager *tcbManager,
                        Font *font, BtlClient *client, u32 scdState, HeapID heapId) {
    BtlvScd *scd = GFL_HeapAllocate(heapId, sizeof(BtlvScd), TRUE, "btlv_scd.c", 204);

    scd->core = core;
    scd->mainModule = mainModule;
    scd->pokeCon = pokeCon;
    scd->heapId = heapId;
    scd->font = font;
    scd->client = client;
    scd->party = GetClientParty(pokeCon, BattleClient_GetClientId(client));
    scd->strbuf = GFL_StrBufCreate(0x80, heapId);
    scd->unk74 = 0;
    scd->scdState = scdState;
    scd->shooterEnergy = 0;
    scd->step = FORCE_QUIT_STEP_WAIT_INPUT;
    scd->forceQuit = FALSE;
    scd->typeCount = BattleClient_GetAudienceSize(client);
    scd->typeMask = BattleClient_GetAudienceMask(client);
    scd->noCancel = IsStudioSkipDisabled(mainModule);
    scd->keyCursorFlag = GameData_GetBattleKeyCursorFlag(GetGameData(scd->mainModule));
    scd->printQueue = PrintQueue_CreateDefault(scd->heapId);
    BtlvScd_ClearProcs(scd);
    return scd;
}

void BtlvScd_CreateInput(BtlvScd *scd) {
    GameData *gameData = GetGameData(scd->mainModule);
    u32 rule = BtlSetup_GetBattleStyle(scd->mainModule);
    u32 battleType = BtlSetup_GetBattleType(scd->mainModule);
    PaletteFade *paletteFade = BtlvEffect_GetPaletteFade();
    u8 multiMode = GetMultiMode(scd->mainModule);
    u32 studioMode = GetStudioMode(scd->mainModule);

    scd->input = BtlvInput_CreateForBattle(gameData, rule, battleType, paletteFade, scd->font, &scd->keyCursorFlag,
                                           scd->scdState, multiMode, studioMode, scd->typeCount, scd->typeMask,
                                           scd->noCancel, scd->heapId);
    GFL_BGSysSetBGEnabledB(0x10, TRUE);
    BtlvScd_Nop(scd);
}

void BtlvScd_StartFadeOut(BtlvScd *scd, u32 mode) {
    scd->fadeFrom = 0;
    scd->fadeTo = -16;
    scd->fadeStep = -1;
    BtlvInput_StartPaletteFade(scd->input, mode);
}

void BtlvScd_StartFadeIn(BtlvScd *scd) {
    scd->fadeFrom = -16;
    scd->fadeTo = 0;
    scd->fadeStep = 1;
    BtlvInput_StartFadeInWithGraphics(scd->input);
}

BOOL BtlvScd_IsFadeDone(BtlvScd *scd) {
    if (!BtlvInput_IsFading(scd->input)) {
        return TRUE;
    }
    return FALSE;
}

void BtlvScd_OnFadeOutDone(BtlvScd *scd) {
}

void BtlvScd_OnFadeInStart(BtlvScd *scd) {
    BtlvScd_Nop(scd);
}

static void BtlvScd_Nop(BtlvScd *scd) {
}

void BtlvScd_ResetScreenIfShown(BtlvScd *scd) {
    if (BtlvInput_GetScreen(scd->input) != 0) {
        BtlvScd_ResetScreen(scd);
    }
}

void BtlvScd_ResetScreen(BtlvScd *scd) {
    BtlvInput_SetScreen(scd->input, 0, NULL);
    BtlvEffect_GaugeAction(8);
    BtlvEffect_StopIdleEffect();
}

BOOL BtlvScd_IsIdle(BtlvScd *scd) {
    if (!BtlvInput_IsBusy(scd->input)) {
        return TRUE;
    }
    return FALSE;
}

void BtlvScd_Delete(BtlvScd *scd) {
    GameData_SetBattleKeyCursorFlag(GetGameData(scd->mainModule), scd->keyCursorFlag);
    PrintQueue_Free(scd->printQueue);
    GFL_StrBufFree(scd->strbuf);
    BtlvInput_Delete(scd->input);
    GFL_HeapFree(scd);
}

static void BtlvScd_ClearProcs(BtlvScd *scd) {
    int i;

    for (i = 0; i < NUM_PROCS; i++) {
        scd->procs[i].init = NULL;
        scd->procs[i].main = NULL;
        scd->procs[i].arg = NULL;
        scd->procs[i].seq = 0;
    }
    scd->numProcs = 0;
}

static void BtlvScd_PushProc(BtlvScd *scd, BtlvSubProcFn init, BtlvSubProcFn main) {
    BtlvSubProc *proc = &scd->procs[scd->numProcs++];

    proc->init = init;
    proc->main = main;
    proc->arg = scd;
    proc->seq = 0;
}

// Runs the procedure on top of the stack, and returns TRUE only when there is none
static BOOL BtlvScd_RunProcs(BtlvScd *scd) {
    BtlvSubProc *proc;
    BOOL done;

    if (scd->numProcs != 0) {
        proc = &scd->procs[scd->numProcs - 1];
        if (proc->init != NULL) {
            if (proc->init(&proc->seq, proc->arg)) {
                proc->init = NULL;
                proc->seq = 0;
            }
            done = FALSE;
        } else if (proc->main != NULL) {
            if (proc->main(&proc->seq, proc->arg)) {
                proc->main = NULL;
                done = TRUE;
            } else {
                done = FALSE;
            }
        } else {
            done = TRUE;
        }
        if (done) {
            proc->init = NULL;
            proc->main = NULL;
            proc->arg = NULL;
            proc->seq = 0;
            scd->numProcs--;
        }
        return FALSE;
    }
    return TRUE;
}

void BtlvScd_StartCommandSelect(BtlvScd *scd, BattleMon *mon, u8 slot, u8 shooterEnergy, BattleAction *action) {
    scd->mon = mon;
    scd->action = action;
    scd->result = BTLV_COMMAND_NONE;
    scd->slot = slot;
    scd->shooterEnergy = shooterEnergy;
    BtlvScd_PushProc(scd, BtlvScd_CommandInit, BtlvScd_CommandMain);
}

void BtlvScd_StartCommandDemo(BtlvScd *scd, BattleMon *mon, u8 slot, BattleAction *action) {
    scd->mon = mon;
    scd->action = action;
    scd->result = BTLV_COMMAND_NONE;
    scd->slot = slot;
    BtlvScd_PushProc(scd, BtlvScd_CommandInit, BtlvScd_FingerDemoMain);
}

u32 BtlvScd_GetCommandResult(BtlvScd *scd) {
    if (BtlvScd_RunProcs(scd)) {
        return scd->result;
    }
    return BTLV_COMMAND_NONE;
}

void BtlvScd_StartMoveSelect(BtlvScd *scd, BattleMon *mon, BattleAction *action) {
    scd->mon = mon;
    scd->action = action;
    scd->result = 0;
    scd->moveSelectMain = BtlvScd_MoveMain;
    BtlvScd_PushProc(scd, BtlvScd_MoveInit, BtlvScd_MoveMain);
}

void BtlvScd_StartRotationSelect(BtlvScd *scd, BtlvSelectTargetParam *param, BattleAction *action) {
    scd->result = 0;
    scd->targetParam = param;
    scd->action = action;
    scd->moveSelectMain = BtlvScd_RotationMain;
    BattleAction_SetNull(action);
    BattleAction_SetNull(&param->rotateAction);
    BattleAction_SetNull(&param->action);
    BtlvScd_PushProc(scd, BtlvScd_RotationInit, BtlvScd_RotationMain);
}

BOOL BtlvScd_IsDone(BtlvScd *scd) {
    return BtlvScd_RunProcs(scd);
}

void BtlvScd_ResumeMoveSelect(BtlvScd *scd) {
    BtlvScd_PushProc(scd, NULL, scd->moveSelectMain);
}

void BtlvScd_StartMoveDemo(BtlvScd *scd, BattleMon *mon, BattleAction *action) {
    scd->mon = mon;
    scd->action = action;
    scd->result = 0;
    BtlvScd_PushProc(scd, BtlvScd_MoveInit, BtlvScd_FingerDemoMain);
}

void BtlvScd_StartTargetSelect(BtlvScd *scd, BattleMon *mon, BattleAction *action) {
    scd->mon = mon;
    scd->action = action;
    BtlvScd_PushProc(scd, BtlvScd_TargetInit, BtlvScd_TargetMain);
}

BOOL BtlvScd_IsTargetSelectDone(BtlvScd *scd) {
    return BtlvScd_RunProcs(scd);
}

void BtlvScd_CancelProcs(BtlvScd *scd) {
    BtlvScd_ClearProcs(scd);
}

// Sets up the command screen: which of the mons on the field can battle, and the party icons
static BOOL BtlvScd_CommandInit(s32 *seq, void *arg) {
    BtlvScd *scd = arg;
    BtlvInputCommandParam param;
    BattleParty *parties[2] = { NULL, NULL };
    u16 counts[2] = { 0, 0 };
    BattleMon *mon;
    u16 total;
    u16 side = 0;
    u32 style;

    MI_CpuClear16(&param, sizeof(param));
    param.scdState = scd->scdState;
    param.launcherPoints = scd->shooterEnergy;
    param.pokePos =
        BattlePosToViewPos(scd->mainModule, MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)));
    BtlvEffect_GaugeAction(param.pokePos);
    style = BtlSetup_GetBattleStyle(scd->mainModule);
    for (; side < 2; side++) {
        u8 playerClient;
        u8 otherClient;
        u16 i;

        parties[0] = NULL;
        parties[1] = NULL;
        if (side == 0) {
            playerClient = GetPlayerClientID(scd->mainModule);
            otherClient = GetPlayerAllyClientID(scd->mainModule);
            if (style == BTL_STYLE_DOUBLE && otherClient != 4) {
                if (playerClient < otherClient) {
                    parties[0] = GetClientParty(scd->pokeCon, playerClient);
                    counts[0] = GetNumMonsInParty(parties[0]);
                    parties[1] = GetClientParty(scd->pokeCon, otherClient);
                    counts[1] = GetNumMonsInParty(parties[1]);
                } else {
                    parties[0] = GetClientParty(scd->pokeCon, otherClient);
                    counts[0] = GetNumMonsInParty(parties[0]);
                    parties[1] = GetClientParty(scd->pokeCon, playerClient);
                    counts[1] = GetNumMonsInParty(parties[1]);
                }
            } else {
                parties[0] = scd->party;
                counts[0] = GetNumMonsInParty(parties[0]);
            }
            switch (style) {
            case BTL_STYLE_SINGLE:
                mon = PokeCon_GetPosMonInner(scd->pokeCon, ViewPosToBattlePos(scd->mainModule, 0));
                if (!IsFainted(mon)) {
                    param.species[0] = GetBattleMonSpecies(mon);
                    param.forms[0] = PokeParty_GetParam(BattleMon_GetViewSrcData(mon), PKM_PARAM_FORM, NULL);
                    param.sexes[0] = GetBattleMonStat(mon, BATTLEMON_GENDER);
                }
                break;
            case BTL_STYLE_TRIPLE:
                mon = PokeCon_GetPosMonInner(scd->pokeCon, ViewPosToBattlePos(scd->mainModule, 6));
                if (!IsFainted(mon)) {
                    param.species[2] = GetBattleMonSpecies(mon);
                    param.forms[2] = PokeParty_GetParam(BattleMon_GetViewSrcData(mon), PKM_PARAM_FORM, NULL);
                    param.sexes[2] = GetBattleMonStat(mon, BATTLEMON_GENDER);
                }
                // fall through
            case BTL_STYLE_DOUBLE:
            case BTL_STYLE_ROTATION:
                mon = PokeCon_GetPosMonInner(scd->pokeCon, ViewPosToBattlePos(scd->mainModule, 2));
                if (!IsFainted(mon)) {
                    param.species[0] = GetBattleMonSpecies(mon);
                    param.forms[0] = PokeParty_GetParam(BattleMon_GetViewSrcData(mon), PKM_PARAM_FORM, NULL);
                    param.sexes[0] = GetBattleMonStat(mon, BATTLEMON_GENDER);
                }
                mon = PokeCon_GetPosMonInner(scd->pokeCon, ViewPosToBattlePos(scd->mainModule, 4));
                if (!IsFainted(mon)) {
                    param.species[1] = GetBattleMonSpecies(mon);
                    param.forms[1] = PokeParty_GetParam(BattleMon_GetViewSrcData(mon), PKM_PARAM_FORM, NULL);
                    param.sexes[1] = GetBattleMonStat(mon, BATTLEMON_GENDER);
                }
                break;
            }
        } else {
            u8 firstClient;
            u8 secondClient;

            if (BtlSetup_GetBattleType(scd->mainModule) == 0 || BtlSetup_GetBattleType(scd->mainModule) == 4) {
                break;
            }
            firstClient = GetPlayerOpponentClientID(scd->mainModule, 0);
            secondClient = GetPlayerOpponentClientID(scd->mainModule, 1);
            param.secondRow = TRUE;
            parties[0] = GetClientParty(scd->pokeCon, firstClient);
            counts[0] = GetNumMonsInParty(parties[0]);
            if (style == BTL_STYLE_DOUBLE && firstClient != secondClient) {
                parties[1] = GetClientParty(scd->pokeCon, secondClient);
                counts[1] = GetNumMonsInParty(parties[1]);
            }
        }
        if (parties[1] == NULL) {
            total = counts[0];
        } else {
            if (counts[0] > 3) {
                counts[0] = 3;
            }
            if (counts[1] > 3) {
                counts[1] = 3;
            }
            total = counts[0] + counts[1];
        }
        for (i = 0; i < total; i++) {
            u32 party;
            u32 slot;

            if (i < counts[0]) {
                party = 0;
                slot = i;
            } else {
                party = 1;
                slot = i - counts[0];
            }
            mon = GetBattleMonFromParty(parties[party], slot);
            if (!IsFainted(mon)) {
                if (GetBattleMonStatus(mon) != 0) {
                    param.entries[side][party * 3 + slot].iconAnim = 3;
                } else if (!CanPokemonBattle(mon)) {
                    param.entries[side][party * 3 + slot].iconAnim = 0;
                } else {
                    param.entries[side][party * 3 + slot].iconAnim = 1;
                }
            } else {
                param.entries[side][party * 3 + slot].iconAnim = 2;
            }
        }
    }
    param.laterPoke = scd->slot;
    param.ballSlide = BattleClient_GetWeather(scd->client);
    BtlvInput_SetScreen(scd->input, 1, &param);
    return TRUE;
}

static BOOL BtlvScd_CommandMain(s32 *seq, void *arg) {
    BtlvScd *scd = arg;
    s32 input;

    switch (*seq) {
    case COMMAND_SEQ_SELECT:
        input = BtlvInput_CheckInput(scd->input, &sCommandButtons, sCommandStops);
        if (input != TOUCH_RECT_NONE) {
            scd->result = sCommandResults[input];
            if (scd->result == BTLV_COMMAND_RUN) {
                BattleAction_SetEscape(scd->action);
                BtlvInput_SetScreen(scd->input, 0, NULL);
                BtlvEffect_GaugeAction(8);
                BtlvEffect_StopIdleEffect();
                (*seq)++;
            } else {
                return TRUE;
            }
        }
        break;
    case COMMAND_SEQ_WAIT_CLOSE:
        if (!BtlvInput_IsBusy(scd->input)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL BtlvScd_FingerDemoMain(s32 *seq, void *arg) {
    BtlvScd *scd = arg;
    BOOL done = BtlvInput_FingerDemoMain(scd->input);

    if (done == TRUE) {
        scd->result = 1;
    }
    return done;
}

static BOOL BtlvScd_MoveInit(s32 *seq, void *arg) {
    BtlvScd *scd = arg;
    BtlvInputMoveParam param;
    u16 count;
    u16 i;
    u8 pp;
    u8 maxPP;

    count = GetBattleMonMoveCount(scd->mon);
    param.noMoveInfo = TransformCheck(scd->mon);
    for (i = 0; i < count; i++) {
        param.moves[i] = BattleMon_GetMoveInfo(scd->mon, i, &pp, &maxPP);
        param.pp[i] = pp;
        param.maxPP[i] = maxPP;
    }
    for (; i < 4; i++) {
        param.moves[i] = 0;
        param.pp[i] = 0;
        param.maxPP[i] = 0;
    }
    param.pos =
        BattlePosToViewPos(scd->mainModule, MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)));
    BtlvInput_SetScreen(scd->input, 2, &param);
    return TRUE;
}

static BOOL BtlvScd_MoveMain(s32 *seq, void *arg) {
    BtlvScd *scd = arg;
    const BtlvInputButtonSet *set;
    const BtlvInputKeyStop *stops;
    s32 input;
    u8 pos;
    BOOL flagged;

    pos = BattlePosToViewPos(scd->mainModule, MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)));
    if (BtlSetup_GetBattleStyle(scd->mainModule) == BTL_STYLE_TRIPLE && pos != 4) {
        set = &sMoveButtonsShift;
    } else {
        set = &sMoveButtons;
    }
    if (BtlSetup_GetBattleStyle(scd->mainModule) == BTL_STYLE_TRIPLE && pos != 4) {
        stops = sMoveStopsShift;
    } else {
        stops = sMoveStops;
    }
    input = BtlvInput_CheckInput(scd->input, set, stops);
    if (input != TOUCH_RECT_NONE) {
        flagged = (input & MOVE_BUTTON_L_HELD) != 0;
        input &= ~MOVE_BUTTON_L_HELD;
        if (input == MOVE_BUTTON_BACK) {
            BattleAction_SetNull(scd->action);
            return TRUE;
        }
        if (input < GetBattleMonMoveCount(scd->mon)) {
            BattleAction_SetFightParam(scd->action, MoveGetID(scd->mon, input), BTL_POS_NONE);
            if (flagged) {
                BattleAction_SetMoveInfoFlag(scd->action);
            }
            return TRUE;
        }
        if (BtlSetup_GetBattleStyle(scd->mainModule) == BTL_STYLE_TRIPLE && input == MOVE_BUTTON_SHIFT) {
            BattleAction_SetShift(scd->action);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL BtlvScd_RotationInit(s32 *seq, void *arg) {
    BtlvScd *scd = arg;
    BtlvInputRotationParam param;
    int i;
    int j;

    for (i = 0; i < 3; i++) {
        param.mons[i] = scd->targetParam->mons[i].mon;
        for (j = 0; j < 4; j++) {
            param.moveUsable[i][j] = scd->targetParam->mons[i].selectable[j];
        }
    }
    BtlvInput_SetScreen(scd->input, 5, &param);
    return TRUE;
}

static BOOL BtlvScd_RotationMain(s32 *seq, void *arg) {
    BtlvScd *scd = arg;
    u8 slot;
    s32 button;
    u16 move;
    BOOL flagged;
    BattleMon *mon;

    if (BtlvInput_MoveScreenMain(scd->input, &slot, &button)) {
        flagged = (button & MOVE_BUTTON_L_HELD) != 0;
        button &= ~MOVE_BUTTON_L_HELD;
        if (button != MOVE_BUTTON_BACK) {
            if (slot == 0) {
                slot = 1;
            }
            if (button != MOVE_BUTTON_STRUGGLE) {
                mon = scd->targetParam->mons[GetRotationInSlot(slot)].mon;
                move = MoveGetID(mon, button);
            } else {
                move = MOVE_STRUGGLE;
            }
            BattleAction_SetRotationParam(&scd->targetParam->rotateAction, slot);
            BattleAction_SetFightParam(&scd->targetParam->action, move, BTL_POS_NONE);
            BattleAction_SetFightParam(scd->action, move, BTL_POS_NONE);
            if (flagged) {
                BattleAction_SetMoveInfoFlag(scd->action);
            }
        } else if (button == MOVE_BUTTON_BACK) { // tested again, as the original does
            BattleAction_SetNull(scd->action);
        }
        return TRUE;
    }
    return FALSE;
}

static void BtlvScd_ClearTargetList(BtlvScdTargetList *list) {
    MI_CpuFill8(list, 0, sizeof(*list));
}

static void BtlvScd_FillSelectTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 posFlags) {
    list->selectCount = ExpandPosFlags(mainModule, posFlags, list->pos);
}

static void BtlvScd_FillGroupTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 posFlags) {
    list->count = ExpandPosFlags(mainModule, posFlags, list->pos);
    list->kind = TARGETS_GROUP;
}

static void BtlvScd_FillSideTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 posFlags) {
    list->count = ExpandPosFlags(mainModule, posFlags, list->pos);
    list->kind = TARGETS_SIDE;
}

// Adds the far positions that a move of a triple battle reaches
static void BtlvScd_AddFarTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 move, u8 pos) {
    u8 far;

    if (BtlSetup_GetBattleStyle(mainModule) == BTL_STYLE_TRIPLE && getMoveFlag(move, 0xb) &&
        GetNonAdjacentOpponentPos(pos, &far)) {
        list->pos[list->selectCount++] = far;
        list->pos[list->selectCount++] = GetFacingPos(BTL_STYLE_TRIPLE, far);
    }
}

// Whether the target screen's button is one of the targets, and its position
static BOOL BtlvScd_GetTargetPos(BtlvScdTargetList *list, u32 button, BtlMainModule *mainModule, u8 *outPos) {
    u8 i;

    if (list->selectCount != 0) {
        *outPos = ViewPosToBattlePos(mainModule, sTargetButtonViewPos[button]);
        for (i = 0; i < list->selectCount; i++) {
            if (*outPos == list->pos[i]) {
                return TRUE;
            }
        }
        return FALSE;
    }
    *outPos = BTL_POS_NONE;
    return TRUE;
}

// The callers pass the list's kind, which the code doesn't read
static void BtlvScd_SetTargetScreen(const u8 *positions, u8 count, u32 kind, BtlvScd *scd) {
    BtlvInputTargetParam param;
    u16 move = scd->action->bits.move;

    MI_CpuClear16(&param, sizeof(param));
    param.attackerPos =
        BattlePosToViewPos(scd->mainModule, MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)));
    param.range = PML_MoveGetParam(move, MOVE_PARAM_TARGET);
    if (move == MOVE_CURSE) {
        param.range = BtlCalc_GetCurseTarget(scd->mon);
    }
    if (param.range == MOVE_TARGET_SELECTED && getMoveFlag(move, 0xb)) {
        param.range = TARGET_RANGE_DISTANT;
    }
    while (count-- != 0) {
        BattleMon *mon = PokeCon_GetPosMonInner(scd->pokeCon, *positions);
        PartyPkm *pkm = BattleMon_GetViewSrcData(mon);
        u8 index = BattlePosToViewPos(scd->mainModule, *positions) - 2;
        BtlvInputTargetEntry *entry = &param.mons[index];

        entry->hp = GetBattleMonStat(mon, BATTLEMON_HP);
        entry->selectable = TRUE;
        if (entry->hp != 0) {
            GetBattleMonSpecies(mon); // the original reads the species and drops it
            entry->pkm = pkm;
            entry->maxHp = GetBattleMonStat(mon, BATTLEMON_HP + 1);
            if (PokeParty_GetParam(BattleMon_GetViewSrcData(mon), PKM_PARAM_SHOW_SEX, NULL) == 0) {
                entry->msgForm = 2;
            } else {
                entry->msgForm = PokeParty_GetSex(BattleMon_GetViewSrcData(mon));
            }
            if (GetBattleMonStatus(mon) != 0) {
                entry->iconAnim = 3;
            } else {
                entry->iconAnim = 1;
            }
        } else {
            entry->iconAnim = 2;
        }
        positions++;
    }
    BtlvInput_SetScreen(scd->input, 3, &param);
}

static void BtlvScd_ShowTargetScreen(BtlvScdTargetList *list, BtlvScd *scd) {
    if (list->selectCount != 0) {
        BtlvScd_SetTargetScreen(list->pos, list->selectCount, TARGETS_SELECT, scd);
    } else if (list->kind == TARGETS_GROUP) {
        BtlvScd_SetTargetScreen(list->pos, list->count, TARGETS_GROUP, scd);
    } else {
        BtlvScd_SetTargetScreen(list->pos, list->count, TARGETS_SIDE, scd);
    }
}

BOOL BtlvScd_IsTargetChosen(BtlvScd *scd) {
    return scd->targetChosen;
}

static BOOL BtlvScd_TargetInit(s32 *seq, void *arg) {
    BtlvScd *scd = arg;

    BtlvScd_BuildTargetList(&scd->targets, scd);
    if (BtlSetup_GetBattleStyle(scd->mainModule) == BTL_STYLE_TRIPLE) {
        u8 target = BTL_POS_NONE;

        if (scd->targets.selectCount != 0) {
            target =
                GetAdjacentOpponentData(MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)))->list1[0];
        }
        BattleAction_SetFightParam(scd->action, scd->action->bits.move, target);
        scd->targetChosen = TRUE;
    }
    BtlvScd_ShowTargetScreen(&scd->targets, scd);
    scd->targetChosen = FALSE;
    scd->moveRange = PML_MoveGetParam(scd->action->bits.move, MOVE_PARAM_TARGET);
    if (scd->action->bits.move == MOVE_CURSE) {
        scd->moveRange = BtlCalc_GetCurseTarget(scd->mon);
    }
    return TRUE;
}

static BOOL BtlvScd_TargetMain(s32 *seq, void *arg) {
    BtlvScd *scd = arg;
    const BtlvInputButtonSet *set;
    const BtlvInputKeyStop *stops;
    u8 pos;
    u8 index;
    u8 range;
    u8 cancel;
    u8 target;
    s32 input;

    if (*seq == 0) {
        pos = BattlePosToViewPos(scd->mainModule, MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)));
        range = scd->moveRange;
        index = (pos - 2) / 2;
        if (BtlSetup_GetBattleStyle(scd->mainModule) != BTL_STYLE_TRIPLE) {
            set = sTargetButtonSetsDouble[index][range];
            stops = BTLV_SCD_TARGET_STOPS_DOUBLE[index][range];
            cancel = TARGET_BUTTON_BACK_DOUBLE;
        } else {
            if (range == MOVE_TARGET_SELECTED && getMoveFlag(scd->action->bits.move, 0xb)) {
                range = TARGET_RANGE_DISTANT;
            }
            set = sTargetButtonSetsTriple[index][range];
            stops = BTLV_SCD_TARGET_STOPS_TRIPLE[index][range];
            cancel = TARGET_BUTTON_BACK_TRIPLE;
        }
        input = BtlvInput_CheckInput(scd->input, set, stops);
        if (input != -1) {
            if (input != cancel) {
                if (BtlvScd_GetTargetPos(&scd->targets, input, scd->mainModule, &target)) {
                    BattleAction_SetFightParam(scd->action, scd->action->bits.move, target);
                    scd->targetChosen = TRUE;
                    return TRUE;
                }
            } else {
                scd->targetChosen = FALSE;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void BtlvScd_BuildTargetList(BtlvScdTargetList *list, BtlvScd *scd) {
    u16 move = scd->action->bits.move;
    u32 range = PML_MoveGetParam(move, MOVE_PARAM_TARGET);
    u8 pos = MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon));

    BtlvScd_ClearTargetList(list);
    if (move == MOVE_CURSE) {
        range = BtlCalc_GetCurseTarget(scd->mon);
    }
    switch (range) {
    case MOVE_TARGET_SELECTED:
        BtlvScd_FillSelectTargets(list, scd->mainModule, TARGET_FLAGS(2, pos));
        BtlvScd_AddFarTargets(list, scd->mainModule, move, pos);
        break;
    case MOVE_TARGET_FOE:
        BtlvScd_FillSelectTargets(list, scd->mainModule, TARGET_FLAGS(1, pos));
        BtlvScd_AddFarTargets(list, scd->mainModule, move, pos);
        break;
    case MOVE_TARGET_USER_OR_ALLY:
        BtlvScd_FillSelectTargets(list, scd->mainModule, TARGET_FLAGS(3, pos));
        break;
    case MOVE_TARGET_ALLY:
        BtlvScd_FillSelectTargets(list, scd->mainModule, TARGET_FLAGS(4, pos));
        break;
    case MOVE_TARGET_ADJACENT_FOES:
        BtlvScd_FillGroupTargets(list, scd->mainModule, TARGET_FLAGS(1, pos));
        break;
    case MOVE_TARGET_ALL_ADJACENT:
        BtlvScd_FillGroupTargets(list, scd->mainModule, TARGET_FLAGS(2, pos));
        break;
    case MOVE_TARGET_USER:
    case MOVE_TARGET_RANDOM_FOE:
        BtlvScd_FillGroupTargets(list, scd->mainModule, TARGET_FLAGS(0, pos));
        break;
    case MOVE_TARGET_USER_PARTY:
    case MOVE_TARGET_USER_SIDE:
        BtlvScd_FillSideTargets(list, scd->mainModule, TARGET_FLAGS(7, pos));
        break;
    case MOVE_TARGET_FOE_SIDE:
        BtlvScd_FillSideTargets(list, scd->mainModule, TARGET_FLAGS(6, pos));
        break;
    case MOVE_TARGET_ALL:
    case MOVE_TARGET_FIELD:
        BtlvScd_FillSideTargets(list, scd->mainModule, TARGET_FLAGS(8, pos));
        break;
    case MOVE_TARGET_DEPENDS:
        BtlvScd_FillGroupTargets(list, scd->mainModule, TARGET_FLAGS(0, pos));
        break;
    }
}

void BtlvScd_ShowRotationScreen(BtlvScd *scd, BtlvInputRotationParam *param) {
    BtlvInput_SetScreen(scd->input, 5, param);
}

BOOL BtlvScd_PollRotationInput(BtlvScd *scd, u8 *button) {
    s32 input = BtlvInput_CheckInput(scd->input, NULL, NULL);

    if (input != -1) {
        *button = input;
        return TRUE;
    }
    return FALSE;
}

void BtlvScd_HideRotationScreen(BtlvScd *scd) {
    BtlvInput_SetScreen(scd->input, 0, NULL);
    BtlvEffect_GaugeAction(8);
    BtlvEffect_StopIdleEffect();
}

void BtlvScd_ShowYesNo(BtlvScd *scd, void *param) {
    BtlvInput_SetScreen(scd->input, 4, param);
    scd->yesNoSeq = YES_NO_SEQ_SELECT;
}

BOOL BtlvScd_PollYesNo(BtlvScd *scd, u32 *answer) {
    s32 input;

    switch (scd->yesNoSeq) {
    case YES_NO_SEQ_SELECT:
        input = BtlvInput_CheckInput(scd->input, &sYesNoButtons, sYesNoStops);
        if (input != -1) {
            scd->answer = input;
            BtlvInput_SetScreen(scd->input, 0, NULL);
            scd->yesNoSeq++;
        }
        break;
    case YES_NO_SEQ_WAIT_CLOSE:
        if (!BtlvInput_IsBusy(scd->input)) {
            *answer = scd->answer;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

u8 *BtlvScd_GetKeyCursorFlag(BtlvScd *scd) {
    return &scd->keyCursorFlag;
}

void BtlvScd_ForceQuitInput(BtlvScd *scd) {
    if (!scd->forceQuit) {
        scd->step = FORCE_QUIT_STEP_WAIT_INPUT;
        scd->forceQuit = TRUE;
    }
}

BOOL BtlvScd_IsForceQuitInputDone(BtlvScd *scd) {
    switch (scd->step) {
    case FORCE_QUIT_STEP_WAIT_INPUT:
        if (!BtlvInput_IsBusy(scd->input)) {
            scd->step++;
        }
        break;
    case FORCE_QUIT_STEP_CLOSE:
        BtlvInput_SetScreen(scd->input, 0, NULL);
        BtlvEffect_GaugeAction(8);
        BtlvEffect_StopIdleEffect();
        scd->step++;
        break;
    case FORCE_QUIT_STEP_WAIT_CLOSE:
        if (!BtlvInput_IsBusy(scd->input)) {
            scd->step++;
        }
        break;
    case FORCE_QUIT_STEP_DONE:
        scd->forceQuit = FALSE;
        return TRUE;
    }
    return FALSE;
}

void BtlvScd_ShowRecorderScreen(BtlvScd *scd, void *param) {
    BtlvInput_SetScreen(scd->input, 6, param);
}

u32 BtlvScd_CheckRecorderInput(BtlvScd *scd) {
    return BtlvInput_CheckInput(scd->input, &sRecorderButtons, sRecorderStops);
}

void BtlvScd_SetTypeIconVisible(BtlvScd *scd, BOOL visible, int index, int sequence) {
    BtlvInput_SetTypeIconVisible(scd->input, visible, index, sequence);
}

u32 BtlvScd_GetTypeMask(BtlvScd *scd) {
    return BtlvInput_GetTypeMask(scd->input);
}

void BtlvScd_StartTypeIconAnim(BtlvScd *scd, int index) {
    BtlvInput_StartTypeIconAnim(scd->input, index);
}
