#include "battle/btlv_scd.h"
#include "types.h"
#include "battle/btl_action.h"
#include "battle/btl_client.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
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
#include "pml/poke_party.h"
#include "pml/waza.h"
#include "system/game_data.h"
#include "system/printsys.h"

#define NUM_PROCS 4

// What BtlMain gets the positions of a group of targets by: a group, and the position of the user
#define TARGET_FLAGS(group, pos) (((group) << 8) | (pos))

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

// The screens' buttons (data_ov169_0689da5c: the command screen's, 0689daa4 and 0689da08: the move screen's,
// 0689da14: yes or no, 0689dabc: the recorder's, and the ones of data_ov169_0689e758 and 0689e7d0: the target
// screen's, by position and move range), the cursor stops that go with them, and the tables of both by position and
// range. They are declared in the order that makes MWCC's size sort lay .rodata out as the game's
static const BtlvInputKeyStop data_ov169_0689dcb0[2] = {
    { { 5, 1, 0, 4, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 5, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689dfb4[3] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 4 },
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 2, 4 },
    { { 4, 4, 4, 4, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689dc38[2] = {
    { { 0, 2, 0, 2, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689dfd8[3] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 6 },
    { { 4, 4, 4, 4, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689dc50[2] = {
    { { 5, 1, 0, 4, 4, 0 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 5, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689dc68[2] = {
    { { 5, 1, 5, 1, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 5, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689dffc[3] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 3, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689dc80[2] = {
    { { 3, 1, 2, 2, -1, 1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689e0bc[4] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 3, 0, 2, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 1, KEY_CURSOR_NONE, 1, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689e0ec[4] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -1, 1, 2, 0, -3 },
    { { 1, 1, 1, 1, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 1, -3 },
    { { 2, 2, 2, 2, -1, -1 }, 0, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 2, -3 },
    { { 3, 3, 3, 3, -1, -1 }, 0, KEY_CURSOR_NONE, 1, 2, 3, -3 },
};
static const BtlvInputKeyStop data_ov169_0689dcc8[2] = {
    { { 0, 4, 0, 4, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689dd10[2] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689dce0[2] = {
    { { 3, 1, 3, 1, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689dde8[2] = {
    { { 5, 3, 5, 3, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 5, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const TouchRect data_ov169_0689dcf8[6] = {
    { 0x20, 0x50, 0x00, 0x80 }, { 0x20, 0x50, 0x80, 0xff }, { 0x50, 0x80, 0x00, 0x80 },
    { 0x50, 0x80, 0x80, 0xff }, { 0x90, 0xc0, 0xb0, 0xff }, { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const TouchRect data_ov169_0689dbb0[5] = {
    { 0x18, 0x90, 0x00, 0xff }, { 0x90, 0xc0, 0x00, 0x50 },           { 0x90, 0xc0, 0xb0, 0xff },
    { 0x98, 0xc0, 0x58, 0xa8 }, { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const TouchRect data_ov169_0689dd28[6] = {
    { 0x58, 0x80, 0x00, 0x80 }, { 0x20, 0x58, 0x80, 0xff }, { 0x58, 0x80, 0x80, 0xff },
    { 0x20, 0x58, 0x00, 0x80 }, { 0x90, 0xc0, 0xb0, 0xff }, { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const BtlvInputKeyStop data_ov169_0689e14c[4] = {
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 3, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 2, 2, 2, 2, -1, -1 }, 1, 3, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 4 },
    { { 4, 4, 4, 4, -1, -1 }, -2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689dd58[2] = {
    { { 5, 3, 5, 2, 2, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 5, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const s32 data_ov169_0689de34[7] = { 0x400, 0x200, 0x800, 0x100, 0x1000, 0x80, 0x10 };
static const s32 data_ov169_0689de50[7] = { 0x980, 0x980, 0x980, 0x980, 0x980, 0x980, 0x10 };
static const s32 data_ov169_0689de6c[7] = { 0x1780, 0x1780, 0x1780, 0x1780, 0x1780, 0x1780, 0x10 };
static const BtlvInputKeyStop data_ov169_0689dd88[2] = {
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689dbf0[2] = {
    { { 3, 1, 3, 1, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689dda0[2] = {
    { { 4, 4, 4, 4, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689e17c[4] = {
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 3, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 0, 0, 0, 0, -1, -1 }, 0, 3, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 4, 4, 4, 4, -1, -1 }, -2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689e068[3] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 6 },
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689e1ac[4] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 3, 6 },
    { { 2, 2, 2, 2, -1, -1 }, 1, 3, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689ddd0[2] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 1 },
    { { 1, 1, 1, 1, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 1, 1 },
};
const u8 data_ov169_0689e218[2][15] = {
    { 4, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4 },
    { 4, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4 },
};
static const u8 data_ov169_0689d9b0[4] = { 1, 2, 3, 4 };
static const s32 data_ov169_0689de00[6] = { 0, 0, 0, 0, 1, 0 };
static const s32 data_ov169_0689dec0[7] = { 0x380, 0x380, 0x380, 0x380, 0x380, 0x380, 0x10 };
static const s32 data_ov169_0689db9c[5] = { 0x780, 0x780, 0x780, 0x780, 0x10 };
static const s32 data_ov169_0689def8[7] = { 0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x10 };
static const s32 data_ov169_0689dbc4[5] = { 0x200, 0x400, 0x800, 0x1000, 0x10 };
static const s32 data_ov169_0689df14[7] = { 0x1c00, 0x1c00, 0x1c00, 0x1c00, 0x1c00, 0x1c00, 0x10 };
static const s32 data_ov169_0689daf0[4] = { 0x10003, 4, 8, 0x10 };
static const s32 data_ov169_0689df30[7] = { 0x1f80, 0x1f80, 0x1f80, 0x1f80, 0x1f80, 0x1f80, 0x10 };
static const s32 data_ov169_0689db10[5] = { 0x200, 0x100, 0x400, 0x80, 0x10 };
static const s32 data_ov169_0689da38[3] = { 4, 8, 0x10 };
static const s32 data_ov169_0689db24[5] = { 0x380, 0x380, 0x380, 0x380, 0x10 };
static const s32 data_ov169_0689d9bc[2] = { 4, 8 };
static const TouchRect data_ov169_0689df4c[8] = {
    { 0x58, 0x80, 0x08, 0x58 }, { 0x20, 0x58, 0xa8, 0xf8 },           { 0x58, 0x80, 0x58, 0xa8 },
    { 0x20, 0x58, 0x58, 0xa8 }, { 0x58, 0x80, 0xa8, 0xf8 },           { 0x20, 0x58, 0x08, 0x58 },
    { 0x90, 0xc0, 0xb0, 0xff }, { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const BtlvInputKeyStop data_ov169_0689e1dc[5] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 2, 2, 2, 2, -1, -1 }, 0, 4, KEY_CURSOR_NONE, 3, 2, 4 },
    { { 3, 3, 3, 3, -1, -1 }, 1, 4, 2, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, -2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689dbd8[2] = {
    { { 3, 1, 0, 1, -1, 0 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689df6c[3] = {
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 3, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 4, 4, 4, 4, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689dc08[2] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689dc20[2] = {
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689e08c[4] = {
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 0, KEY_CURSOR_NONE, 1, 6 },
    { { 2, 2, 2, 2, -1, -1 }, 0, 3, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689df90[3] = {
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 2, 6 },
    { { 4, 4, 4, 4, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689e268[6] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 5, 0, 2, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 4, 1, KEY_CURSOR_NONE, 1, 6 },
    { { 0, 0, 0, 0, -1, -1 }, 0, 5, KEY_CURSOR_NONE, 4, 0, 6 },
    { { 4, 4, 4, 4, -1, -1 }, 2, 5, 3, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689ddb8[2] = {
    { { 3, 1, 3, 2, 2, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689dd70[2] = {
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689e2b0[6] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 2, 2, 2, 2, -1, -1 }, 0, 5, KEY_CURSOR_NONE, 3, 2, 4 },
    { { 3, 3, 3, 3, -1, -1 }, 1, 4, 2, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 3, KEY_CURSOR_NONE, 5, KEY_CURSOR_NONE, 4, 4 },
    { { 5, 5, 5, 5, -1, -1 }, 2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 5, 4 },
};
static const BtlvInputKeyStop data_ov169_0689e2f8[6] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 5, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 3, 0, 2, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 4, 1, KEY_CURSOR_NONE, 1, 6 },
    { { 2, 2, 2, 2, -1, -1 }, 1, 5, KEY_CURSOR_NONE, 4, 2, 6 },
    { { 4, 4, 4, 4, -1, -1 }, 2, 5, 3, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689e11c[4] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 0, 6 },
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, 3, 0, 2, 2, 6 },
    { { 4, 4, 4, 4, -1, -1 }, KEY_CURSOR_NONE, 3, 1, KEY_CURSOR_NONE, 4, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689e340[6] = {
    { { 5, 5, 5, 5, -1, -1 }, KEY_CURSOR_NONE, 3, KEY_CURSOR_NONE, 1, 5, 6 },
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 4, 0, 2, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 5, 1, KEY_CURSOR_NONE, 1, 6 },
    { { 0, 0, 0, 0, -1, -1 }, 0, 5, KEY_CURSOR_NONE, 4, 0, 6 },
    { { 2, 2, 2, 2, -1, -1 }, 2, 5, 3, KEY_CURSOR_NONE, 2, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689e020[3] = {
    { { 3, 3, 3, 3, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 3, 6 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE, 1, 6 },
    { { 6, 6, 6, 6, -1, -1 }, -1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 6 },
};
static const BtlvInputKeyStop data_ov169_0689dd40[2] = {
    { { 3, 1, 0, 2, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 4 },
};
static const BtlvInputKeyStop data_ov169_0689e044[3] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, 0, KEY_CURSOR_NONE },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE },
    { { 2, 2, 2, 2, -1, -1 }, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 1, 2, KEY_CURSOR_NONE },
};
static const s32 data_ov169_0689db74[5] = { 0x580, 0x580, 0x580, 0x580, 0x10 };
static const s32 data_ov169_0689dc98[6] = { 0x200, 0x400, 0x800, 0x1000, 0x10, 4 };
static const s32 data_ov169_0689db4c[5] = { 0x600, 0x600, 0x600, 0x600, 0x10 };
static const s32 data_ov169_0689db88[5] = { 0x180, 0x180, 0x180, 0x180, 0x10 };
static const s32 data_ov169_0689de88[7] = { 0xb00, 0xb00, 0xb00, 0xb00, 0xb00, 0xb00, 0x10 };
static const s32 data_ov169_0689dea4[7] = { 0x180, 0x180, 0x180, 0x180, 0x180, 0x180, 0x10 };
static const TouchRect data_ov169_0689dedc[7] = {
    { 0x20, 0x50, 0x00, 0x80 },           { 0x20, 0x50, 0x80, 0xff }, { 0x50, 0x80, 0x00, 0x80 },
    { 0x50, 0x80, 0x80, 0xff },           { 0x90, 0xc0, 0xb0, 0xff }, { 0x90, 0xc0, 0x00, 0x50 },
    { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const TouchRect data_ov169_0689da80[3] = {
    { 0x20, 0x50, 0x00, 0xff },
    { 0x50, 0x80, 0x00, 0xff },
    { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const TouchRect data_ov169_0689dae0[4] = {
    { 0x90, 0xc0, 0x00, 0x50 },
    { 0x90, 0xc0, 0xb0, 0xff },
    { 0x98, 0xc0, 0x58, 0xa8 },
    { TOUCH_RECT_END, 0x00, 0x00, 0x00 },
};
static const s32 data_ov169_0689db00[4] = { 0, 0, 0, 2 };
static const s32 data_ov169_0689da68[3] = { 0, 0, 0 };
static const s32 data_ov169_0689d9c4[2] = { 0, 0 };
static const s32 data_ov169_0689de18[7] = { 0, 0, 0, 0, 0, 0, 3 };
static const s32 data_ov169_0689db60[5] = { 0, 0, 0, 0, 1 };
static const BtlvInputButtonSet data_ov169_0689da8c = { data_ov169_0689dd28, data_ov169_0689db60, data_ov169_0689db9c };
static const BtlvInputButtonSet data_ov169_0689d9fc = { data_ov169_0689df4c, data_ov169_0689de18, data_ov169_0689de50 };
static const BtlvInputButtonSet data_ov169_0689da74 = { data_ov169_0689dd28, data_ov169_0689db60, data_ov169_0689db24 };
static const BtlvInputButtonSet data_ov169_0689da2c = { data_ov169_0689df4c, data_ov169_0689de18, data_ov169_0689de88 };
static const BtlvInputButtonSet data_ov169_0689da50 = { data_ov169_0689dd28, data_ov169_0689db60, data_ov169_0689db88 };
static const BtlvInputButtonSet data_ov169_0689d9cc = { data_ov169_0689df4c, data_ov169_0689de18, data_ov169_0689de34 };
static const BtlvInputButtonSet data_ov169_0689da98 = { data_ov169_0689df4c, data_ov169_0689de18, data_ov169_0689dea4 };
static const BtlvInputButtonSet data_ov169_0689dad4 = { data_ov169_0689df4c, data_ov169_0689de18, data_ov169_0689df14 };
static const BtlvInputButtonSet data_ov169_0689d9d8 = { data_ov169_0689df4c, data_ov169_0689de18, data_ov169_0689dec0 };
const u8 data_ov169_0689e238[3][15] = {
    { 4, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6 },
    { 6, 4, 3, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6 },
    { 4, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6 },
};
static const u8 data_ov169_0689d9b4[6] = { 2, 3, 4, 5, 6, 7 };
const BtlvInputKeyStop *data_ov169_0689e6e0[2][15] = {
    { data_ov169_0689e14c, data_ov169_0689dfb4, data_ov169_0689dd70, data_ov169_0689df6c, data_ov169_0689ddb8,
      data_ov169_0689dbf0, data_ov169_0689dc38, data_ov169_0689dc08, data_ov169_0689dd40, data_ov169_0689dc08,
      data_ov169_0689dd40, data_ov169_0689dbf0, data_ov169_0689dc38, data_ov169_0689dc08, data_ov169_0689e14c },
    { data_ov169_0689e17c, data_ov169_0689dfb4, data_ov169_0689dc08, data_ov169_0689df6c, data_ov169_0689dbd8,
      data_ov169_0689dbf0, data_ov169_0689dc38, data_ov169_0689dd70, data_ov169_0689dd40, data_ov169_0689dd70,
      data_ov169_0689dd40, data_ov169_0689dbf0, data_ov169_0689dc38, data_ov169_0689dd70, data_ov169_0689e17c },
};
static const s32 data_ov169_0689db38[5] = { 0, 0, 0, 0, 1 };
static const BtlvInputButtonSet data_ov169_0689d9f0 = { data_ov169_0689dd28, data_ov169_0689db60, data_ov169_0689db74 };
static const BtlvInputButtonSet data_ov169_0689da5c = { data_ov169_0689dbb0, data_ov169_0689db00, data_ov169_0689daf0 };
static const BtlvInputButtonSet data_ov169_0689dabc = { data_ov169_0689dae0, data_ov169_0689da68, data_ov169_0689da38 };
static const BtlvInputButtonSet data_ov169_0689da14 = { data_ov169_0689da80, data_ov169_0689d9c4, data_ov169_0689d9bc };
static const BtlvInputButtonSet data_ov169_0689dab0 = { data_ov169_0689df4c, data_ov169_0689de18, data_ov169_0689de6c };
static const BtlvInputButtonSet data_ov169_0689daa4 = { data_ov169_0689dcf8, data_ov169_0689db38, data_ov169_0689dbc4 };
static const BtlvInputButtonSet data_ov169_0689da08 = { data_ov169_0689dedc, data_ov169_0689de00, data_ov169_0689dc98 };
static const BtlvInputButtonSet data_ov169_0689d9e4 = { data_ov169_0689dd28, data_ov169_0689db60, data_ov169_0689db4c };
static const BtlvInputButtonSet data_ov169_0689da20 = { data_ov169_0689dd28, data_ov169_0689db60, data_ov169_0689db10 };
static const BtlvInputButtonSet data_ov169_0689dac8 = { data_ov169_0689df4c, data_ov169_0689de18, data_ov169_0689df30 };
static const BtlvInputButtonSet data_ov169_0689da44 = { data_ov169_0689df4c, data_ov169_0689de18, data_ov169_0689def8 };
static const BtlvInputButtonSet *data_ov169_0689e7d0[3][15] = {
    { &data_ov169_0689d9cc, &data_ov169_0689d9cc, &data_ov169_0689d9cc, &data_ov169_0689d9cc, &data_ov169_0689d9fc,
      &data_ov169_0689da98, &data_ov169_0689dad4, &data_ov169_0689d9cc, &data_ov169_0689dac8, &data_ov169_0689d9cc,
      &data_ov169_0689dac8, &data_ov169_0689d9d8, &data_ov169_0689dad4, &data_ov169_0689d9cc, &data_ov169_0689d9cc },
    { &data_ov169_0689d9cc, &data_ov169_0689d9cc, &data_ov169_0689d9cc, &data_ov169_0689d9cc, &data_ov169_0689dab0,
      &data_ov169_0689d9d8, &data_ov169_0689dad4, &data_ov169_0689d9cc, &data_ov169_0689dac8, &data_ov169_0689d9cc,
      &data_ov169_0689dac8, &data_ov169_0689d9d8, &data_ov169_0689dad4, &data_ov169_0689d9cc, &data_ov169_0689d9cc },
    { &data_ov169_0689d9cc, &data_ov169_0689d9cc, &data_ov169_0689d9cc, &data_ov169_0689d9cc, &data_ov169_0689da2c,
      &data_ov169_0689da44, &data_ov169_0689dad4, &data_ov169_0689d9cc, &data_ov169_0689dac8, &data_ov169_0689d9cc,
      &data_ov169_0689dac8, &data_ov169_0689d9d8, &data_ov169_0689dad4, &data_ov169_0689d9cc, &data_ov169_0689d9cc },
};
const BtlvInputKeyStop *data_ov169_0689e884[3][15] = {
    { data_ov169_0689e1ac, data_ov169_0689e068, data_ov169_0689dc20, data_ov169_0689dffc, data_ov169_0689dd58,
      data_ov169_0689dde8, data_ov169_0689dcc8, data_ov169_0689dd10, data_ov169_0689dcb0, data_ov169_0689dd10,
      data_ov169_0689dcb0, data_ov169_0689dc68, data_ov169_0689dcc8, data_ov169_0689dd10, data_ov169_0689e2f8 },
    { data_ov169_0689e268, data_ov169_0689e11c, data_ov169_0689dfd8, data_ov169_0689e0bc, data_ov169_0689dc50,
      data_ov169_0689dc68, data_ov169_0689dcc8, data_ov169_0689dd88, data_ov169_0689dcb0, data_ov169_0689dd88,
      data_ov169_0689dcb0, data_ov169_0689dc68, data_ov169_0689dcc8, data_ov169_0689dd88, data_ov169_0689e268 },
    { data_ov169_0689e08c, data_ov169_0689df90, data_ov169_0689dc20, data_ov169_0689e020, data_ov169_0689dc80,
      data_ov169_0689dce0, data_ov169_0689dcc8, data_ov169_0689dda0, data_ov169_0689dcb0, data_ov169_0689dda0,
      data_ov169_0689dcb0, data_ov169_0689dc68, data_ov169_0689dcc8, data_ov169_0689dda0, data_ov169_0689e340 },
};
static const BtlvInputButtonSet *data_ov169_0689e758[2][15] = {
    { &data_ov169_0689da20, &data_ov169_0689da20, &data_ov169_0689da20, &data_ov169_0689da20, &data_ov169_0689d9f0,
      &data_ov169_0689da50, &data_ov169_0689d9e4, &data_ov169_0689da20, &data_ov169_0689da8c, &data_ov169_0689da20,
      &data_ov169_0689da8c, &data_ov169_0689da50, &data_ov169_0689d9e4, &data_ov169_0689da20, &data_ov169_0689da20 },
    { &data_ov169_0689da20, &data_ov169_0689da20, &data_ov169_0689da20, &data_ov169_0689da20, &data_ov169_0689da74,
      &data_ov169_0689da50, &data_ov169_0689d9e4, &data_ov169_0689da20, &data_ov169_0689da8c, &data_ov169_0689da20,
      &data_ov169_0689da8c, &data_ov169_0689da50, &data_ov169_0689d9e4, &data_ov169_0689da20, &data_ov169_0689da20 },
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
    scd->step = 0;
    scd->forceQuit = FALSE;
    scd->typeCount = func_ov167_021b9194(client);
    scd->typeMask = func_ov167_021b919c(client);
    scd->noCancel = func_ov167_0219c9c0(mainModule);
    scd->keyCursorFlag = func_020174c8(func_ov167_0219bf98(scd->mainModule));
    scd->printQueue = func_02021998(scd->heapId);
    BtlvScd_ClearProcs(scd);
    return scd;
}

void BtlvScd_CreateInput(BtlvScd *scd) {
    GameData *gameData = func_ov167_0219bf98(scd->mainModule);
    u32 rule = BtlSetup_GetBattleStyle(scd->mainModule);
    u32 battleType = BtlSetup_GetBattleType(scd->mainModule);
    PaletteFade *paletteFade = BtlvEffect_GetPaletteFade();
    u8 unk328 = func_ov167_0219bedc(scd->mainModule);
    u32 fileSet = func_ov167_0219c988(scd->mainModule);

    scd->input = BtlvInput_CreateForBattle(gameData, rule, battleType, paletteFade, scd->font, &scd->keyCursorFlag,
                                           scd->scdState, unk328, fileSet, scd->typeCount, scd->typeMask, scd->noCancel,
                                           scd->heapId);
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
    func_020174bc(func_ov167_0219bf98(scd->mainModule), scd->keyCursorFlag);
    func_02021a18(scd->printQueue);
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
    scd->result = 0;
    scd->slot = slot;
    scd->shooterEnergy = shooterEnergy;
    BtlvScd_PushProc(scd, BtlvScd_CommandInit, BtlvScd_CommandMain);
}

void BtlvScd_StartCommandDemo(BtlvScd *scd, BattleMon *mon, u8 slot, BattleAction *action) {
    scd->mon = mon;
    scd->action = action;
    scd->result = 0;
    scd->slot = slot;
    BtlvScd_PushProc(scd, BtlvScd_CommandInit, BtlvScd_FingerDemoMain);
}

u32 BtlvScd_GetCommandResult(BtlvScd *scd) {
    if (BtlvScd_RunProcs(scd)) {
        return scd->result;
    }
    return 0;
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
        func_ov167_0219c6dc(scd->mainModule, MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)));
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
            otherClient = func_ov167_0219c86c(scd->mainModule);
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
                mon = func_ov167_0219d188(scd->pokeCon, func_ov167_0219c744(scd->mainModule, 0));
                if (!IsFainted(mon)) {
                    param.species[0] = GetBattleMonSpecies(mon);
                    param.forms[0] = PokeParty_GetParam(func_ov167_021bb064(mon), PKM_PARAM_FORM, NULL);
                    param.sexes[0] = GetBattleMonStat(mon, BATTLEMON_GENDER);
                }
                break;
            case BTL_STYLE_TRIPLE:
                mon = func_ov167_0219d188(scd->pokeCon, func_ov167_0219c744(scd->mainModule, 6));
                if (!IsFainted(mon)) {
                    param.species[2] = GetBattleMonSpecies(mon);
                    param.forms[2] = PokeParty_GetParam(func_ov167_021bb064(mon), PKM_PARAM_FORM, NULL);
                    param.sexes[2] = GetBattleMonStat(mon, BATTLEMON_GENDER);
                }
                // fall through
            case BTL_STYLE_DOUBLE:
            case BTL_STYLE_ROTATION:
                mon = func_ov167_0219d188(scd->pokeCon, func_ov167_0219c744(scd->mainModule, 2));
                if (!IsFainted(mon)) {
                    param.species[0] = GetBattleMonSpecies(mon);
                    param.forms[0] = PokeParty_GetParam(func_ov167_021bb064(mon), PKM_PARAM_FORM, NULL);
                    param.sexes[0] = GetBattleMonStat(mon, BATTLEMON_GENDER);
                }
                mon = func_ov167_0219d188(scd->pokeCon, func_ov167_0219c744(scd->mainModule, 4));
                if (!IsFainted(mon)) {
                    param.species[1] = GetBattleMonSpecies(mon);
                    param.forms[1] = PokeParty_GetParam(func_ov167_021bb064(mon), PKM_PARAM_FORM, NULL);
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
            firstClient = func_ov167_0219c8b8(scd->mainModule, 0);
            secondClient = func_ov167_0219c8b8(scd->mainModule, 1);
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
    param.ballSlide = func_ov167_021b9188(scd->client);
    BtlvInput_SetScreen(scd->input, 1, &param);
    return TRUE;
}

static BOOL BtlvScd_CommandMain(s32 *seq, void *arg) {
    BtlvScd *scd = arg;
    s32 input;

    switch (*seq) {
    case 0:
        input = BtlvInput_CheckInput(scd->input, &data_ov169_0689da5c, data_ov169_0689e0ec);
        if (input != TOUCH_RECT_NONE) {
            scd->result = data_ov169_0689d9b0[input];
            if (scd->result == 4) {
                func_ov167_021bdc3c(scd->action);
                BtlvInput_SetScreen(scd->input, 0, NULL);
                BtlvEffect_GaugeAction(8);
                BtlvEffect_StopIdleEffect();
                (*seq)++;
            } else {
                return TRUE;
            }
        }
        break;
    case 1:
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
        param.moves[i] = func_ov167_021bad28(scd->mon, i, &pp, &maxPP);
        param.pp[i] = pp;
        param.maxPP[i] = maxPP;
    }
    for (; i < 4; i++) {
        param.moves[i] = 0;
        param.pp[i] = 0;
        param.maxPP[i] = 0;
    }
    param.pos =
        func_ov167_0219c6dc(scd->mainModule, MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)));
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

    pos = func_ov167_0219c6dc(scd->mainModule, MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)));
    if (BtlSetup_GetBattleStyle(scd->mainModule) == BTL_STYLE_TRIPLE && pos != 4) {
        set = &data_ov169_0689da08;
    } else {
        set = &data_ov169_0689daa4;
    }
    if (BtlSetup_GetBattleStyle(scd->mainModule) == BTL_STYLE_TRIPLE && pos != 4) {
        stops = data_ov169_0689e2b0;
    } else {
        stops = data_ov169_0689e1dc;
    }
    input = BtlvInput_CheckInput(scd->input, set, stops);
    if (input != TOUCH_RECT_NONE) {
        flagged = (input & 0x8000) != 0;
        input &= ~0x8000;
        if (input == 4) {
            BattleAction_SetNull(scd->action);
            return TRUE;
        }
        if (input < GetBattleMonMoveCount(scd->mon)) {
            BattleAction_SetFightParam(scd->action, MoveGetID(scd->mon, input), 6);
            if (flagged) {
                func_ov167_021bdb40(scd->action);
            }
            return TRUE;
        }
        if (BtlSetup_GetBattleStyle(scd->mainModule) == BTL_STYLE_TRIPLE && input == 5) {
            func_ov167_021bdc4c(scd->action);
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
        flagged = (button & 0x8000) != 0;
        button &= ~0x8000;
        if (button != 4) {
            if (slot == 0) {
                slot = 1;
            }
            if (button != 7) {
                mon = scd->targetParam->mons[func_ov167_0219d38c(slot)].mon;
                move = MoveGetID(mon, button);
            } else {
                move = MOVE_STRUGGLE;
            }
            func_ov167_021bdc24(&scd->targetParam->rotateAction, slot);
            BattleAction_SetFightParam(&scd->targetParam->action, move, 6);
            BattleAction_SetFightParam(scd->action, move, 6);
            if (flagged) {
                func_ov167_021bdb40(scd->action);
            }
        } else if (button == 4) { // tested again, as the original does
            BattleAction_SetNull(scd->action);
        }
        return TRUE;
    }
    return FALSE;
}

static void BtlvScd_ClearTargetList(BtlvScdTargetList *list) {
    sys_memset(list, 0, sizeof(*list));
}

static void BtlvScd_FillSelectTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 posFlags) {
    list->selectCount = func_ov167_0219bfe4(mainModule, posFlags, list->pos);
}

static void BtlvScd_FillGroupTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 posFlags) {
    list->count = func_ov167_0219bfe4(mainModule, posFlags, list->pos);
    list->kind = TARGETS_GROUP;
}

static void BtlvScd_FillSideTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 posFlags) {
    list->count = func_ov167_0219bfe4(mainModule, posFlags, list->pos);
    list->kind = TARGETS_SIDE;
}

// Adds the far positions that a move of a triple battle reaches
static void BtlvScd_AddFarTargets(BtlvScdTargetList *list, BtlMainModule *mainModule, u16 move, u8 pos) {
    u8 far;

    if (BtlSetup_GetBattleStyle(mainModule) == BTL_STYLE_TRIPLE && getMoveFlag(move, 0xb) &&
        func_ov167_0219d2dc(pos, &far)) {
        list->pos[list->selectCount++] = far;
        list->pos[list->selectCount++] = func_ov167_0219c4c8(BTL_STYLE_TRIPLE, far);
    }
}

// Whether the target screen's button is one of the targets, and its position
static BOOL BtlvScd_GetTargetPos(BtlvScdTargetList *list, u32 button, BtlMainModule *mainModule, u8 *outPos) {
    u8 i;

    if (list->selectCount != 0) {
        *outPos = func_ov167_0219c744(mainModule, data_ov169_0689d9b4[button]);
        for (i = 0; i < list->selectCount; i++) {
            if (*outPos == list->pos[i]) {
                return TRUE;
            }
        }
        return FALSE;
    }
    *outPos = 6;
    return TRUE;
}

// The callers pass the list's kind, which the code doesn't read
static void BtlvScd_SetTargetScreen(const u8 *positions, u8 count, u32 kind, BtlvScd *scd) {
    BtlvInputTargetParam param;
    u16 move = scd->action->bits.move;

    MI_CpuClear16(&param, sizeof(param));
    param.attackerPos =
        func_ov167_0219c6dc(scd->mainModule, MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)));
    param.range = PML_MoveGetParam(move, MOVE_PARAM_TARGET);
    if (move == MOVE_CURSE) {
        param.range = func_ov167_021bd8d0(scd->mon);
    }
    if (param.range == 0 && getMoveFlag(move, 0xb)) {
        param.range = 14;
    }
    while (count-- != 0) {
        BattleMon *mon = func_ov167_0219d188(scd->pokeCon, *positions);
        PartyPkm *pkm = func_ov167_021bb064(mon);
        u8 index = func_ov167_0219c6dc(scd->mainModule, *positions) - 2;
        BtlvInputTargetEntry *entry = &param.mons[index];

        entry->hp = GetBattleMonStat(mon, BATTLEMON_HP);
        entry->selectable = TRUE;
        if (entry->hp != 0) {
            GetBattleMonSpecies(mon); // the original reads the species and drops it
            entry->pkm = pkm;
            entry->maxHp = GetBattleMonStat(mon, BATTLEMON_HP + 1);
            if (PokeParty_GetParam(func_ov167_021bb064(mon), PKM_PARAM_SHOW_SEX, NULL) == 0) {
                entry->msgForm = 2;
            } else {
                entry->msgForm = PokeParty_GetSex(func_ov167_021bb064(mon));
            }
            if (GetBattleMonStatus(mon) != 0) {
                entry->unk1 = 3;
            } else {
                entry->unk1 = 1;
            }
        } else {
            entry->unk1 = 2;
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
        u8 target = 6;

        if (scd->targets.selectCount != 0) {
            target = func_ov167_0219d2bc(MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)))->list1[0];
        }
        BattleAction_SetFightParam(scd->action, scd->action->bits.move, target);
        scd->targetChosen = TRUE;
    }
    BtlvScd_ShowTargetScreen(&scd->targets, scd);
    scd->targetChosen = FALSE;
    scd->moveRange = PML_MoveGetParam(scd->action->bits.move, MOVE_PARAM_TARGET);
    if (scd->action->bits.move == MOVE_CURSE) {
        scd->moveRange = func_ov167_021bd8d0(scd->mon);
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
        pos = func_ov167_0219c6dc(scd->mainModule, MonIDToBattlePos(scd->mainModule, scd->pokeCon, GetMonID(scd->mon)));
        range = scd->moveRange;
        index = (pos - 2) / 2;
        if (BtlSetup_GetBattleStyle(scd->mainModule) != BTL_STYLE_TRIPLE) {
            set = data_ov169_0689e758[index][range];
            stops = data_ov169_0689e6e0[index][range];
            cancel = 4;
        } else {
            if (range == 0 && getMoveFlag(scd->action->bits.move, 0xb)) {
                range = 14;
            }
            set = data_ov169_0689e7d0[index][range];
            stops = data_ov169_0689e884[index][range];
            cancel = 6;
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
        range = func_ov167_021bd8d0(scd->mon);
    }
    switch (range) {
    case 0:
        BtlvScd_FillSelectTargets(list, scd->mainModule, TARGET_FLAGS(2, pos));
        BtlvScd_AddFarTargets(list, scd->mainModule, move, pos);
        break;
    case 3:
        BtlvScd_FillSelectTargets(list, scd->mainModule, TARGET_FLAGS(1, pos));
        BtlvScd_AddFarTargets(list, scd->mainModule, move, pos);
        break;
    case 1:
        BtlvScd_FillSelectTargets(list, scd->mainModule, TARGET_FLAGS(3, pos));
        break;
    case 2:
        BtlvScd_FillSelectTargets(list, scd->mainModule, TARGET_FLAGS(4, pos));
        break;
    case 5:
        BtlvScd_FillGroupTargets(list, scd->mainModule, TARGET_FLAGS(1, pos));
        break;
    case 4:
        BtlvScd_FillGroupTargets(list, scd->mainModule, TARGET_FLAGS(2, pos));
        break;
    case 7:
    case 9:
        BtlvScd_FillGroupTargets(list, scd->mainModule, TARGET_FLAGS(0, pos));
        break;
    case 6:
    case 12:
        BtlvScd_FillSideTargets(list, scd->mainModule, TARGET_FLAGS(7, pos));
        break;
    case 11:
        BtlvScd_FillSideTargets(list, scd->mainModule, TARGET_FLAGS(6, pos));
        break;
    case 8:
    case 10:
        BtlvScd_FillSideTargets(list, scd->mainModule, TARGET_FLAGS(8, pos));
        break;
    case 13:
        BtlvScd_FillGroupTargets(list, scd->mainModule, TARGET_FLAGS(0, pos));
        break;
    }
}

void BtlvScd_ShowRotationScreen(BtlvScd *scd, void *param) {
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
    scd->yesNoSeq = 0;
}

BOOL BtlvScd_PollYesNo(BtlvScd *scd, u32 *answer) {
    s32 input;

    switch (scd->yesNoSeq) {
    case 0:
        input = BtlvInput_CheckInput(scd->input, &data_ov169_0689da14, data_ov169_0689ddd0);
        if (input != -1) {
            scd->answer = input;
            BtlvInput_SetScreen(scd->input, 0, NULL);
            scd->yesNoSeq++;
        }
        break;
    case 1:
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
        scd->step = 0;
        scd->forceQuit = TRUE;
    }
}

BOOL BtlvScd_IsForceQuitInputDone(BtlvScd *scd) {
    switch (scd->step) {
    case 0:
        if (!BtlvInput_IsBusy(scd->input)) {
            scd->step++;
        }
        break;
    case 1:
        BtlvInput_SetScreen(scd->input, 0, NULL);
        BtlvEffect_GaugeAction(8);
        BtlvEffect_StopIdleEffect();
        scd->step++;
        break;
    case 2:
        if (!BtlvInput_IsBusy(scd->input)) {
            scd->step++;
        }
        break;
    case 3:
        scd->forceQuit = FALSE;
        return TRUE;
    }
    return FALSE;
}

void BtlvScd_ShowRecorderScreen(BtlvScd *scd, void *param) {
    BtlvInput_SetScreen(scd->input, 6, param);
}

u32 BtlvScd_CheckRecorderInput(BtlvScd *scd) {
    return BtlvInput_CheckInput(scd->input, &data_ov169_0689dabc, data_ov169_0689e044);
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
