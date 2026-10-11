// The battle's lower screen. The name is the ROM's string, from GFL_HeapAllocate's asserts. The names are ours; swan
// has none for this file. Another session's notes, ~/Projects/White2Decomp/docs/battle-ui-spec.md (White 2 addresses,
// +0x40), were the source of our understanding of the screens and their transitions, not of names or code

#include "battle/btlv_input.h"
#include "types.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btlv.h"
#include "battle/btlv_effect.h"
#include "battle/btlv_finger_cursor.h"
#include "battle/btlv_gauge.h"
#include "battle/btlv_mcss.h"
#include "constants/arc.h"
#include "constants/moves.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/poke_party.h"
#include "pml/waza.h"
#include "system/app_menu_common.h"
#include "system/gf_font.h"
#include "system/infowin.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/0021.h"

// An actor slot; the second word is never touched
typedef struct {
    ClActor *actor;
    u32 unk4;
} BtlvInputActor;

// The lower screen's work, 0x36c bytes
struct BtlvInput {
    TCBManager *tcbMgr;                // 0x000
    void *tcbMgrBuf;                   // 0x004
    TCB *tasks[8];                     // 0x008
    void (*taskEndFuncs[8])(TCB *tcb); // 0x028
    ArcTool *arc;                      // 0x048  archive 11
    GameData *gameData;                // 0x04c
    u32 rule;                          // 0x050  2 = triple
    u32 unk54;                         // 0x054
    u32 screen;                        // 0x058  0..7
    MsgData *msgData;                  // 0x05c
    BtlvFingerCursor *fingerCursor;    // 0x060
    PaletteFade *paletteFade;          // 0x064
    // 0x068
    u32 busy : 1;          // bit 0: a screen change runs
    u32 subTaskCount : 3;  // bits 1-3: running sub tasks
    u32 laterPoke : 1;     // bit 4: not the first mon to choose this turn
    u32 cursor : 4;        // bits 5-8: current button
    u32 prevCursor : 4;    // bits 9-12: initialised to 0xf
    u32 cursorRefresh : 1; // bit 13
    u32 secondRow : 1;     // bit 14
    u32 fadeMode : 2;      // bits 15-16
    u32 unk68_17 : 2;      // bits 17-18
    u32 pokeSlot : 2;      // bits 19-20: index into mons[] and moveUsable[]
    u32 unk68_21 : 2;      // bits 21-22: never touched
    u32 fingerSeq : 4;     // bits 23-26: the finger cursor demonstration's step
    u32 fingerCount : 4;   // bits 27-30
    u32 inputLocked : 1;   // bit 31
    // 0x06c
    u32 canCancel : 1;       // bit 0
    u32 selectionLocked : 1; // bit 1
    u32 pos : 8;             // bits 2-9: the move's range, the column of ov169's target tables
    u32 unk6c_10 : 8;        // never touched
    u32 noCancel : 1;        // bit 18: no cancel sound
    u32 unk6c_19 : 13;
    s32 pressedButton;                    // 0x070  -1 none; | 0x8000 when L is held
    BtlvInputPokeEntry pokeEntries[2][6]; // 0x074
    u32 chars;                            // 0x104
    u32 palette;                          // 0x108
    u32 cellAnims;                        // 0x10c
    u32 chars2;                           // 0x110
    u32 cellAnims2;                       // 0x114
    ClActUnit *pokeUnit;                  // 0x118
    BtlvInputActor pokeActors[2][6];      // 0x11c
    ClActUnit *cursorUnit;                // 0x17c
    BtlvInputActor cursorActors[6];       // 0x180
    u32 iconChars[3];                     // 0x1b0
    u32 iconPalette;                      // 0x1bc
    u32 iconCellAnims;                    // 0x1c0
    ClActUnit *iconUnit;                  // 0x1c4
    BtlvInputActor iconActors[3];         // 0x1c8
    u32 moveChars[4];                     // 0x1e0
    u32 movePalette;                      // 0x1f0
    u32 moveCellAnims;                    // 0x1f4
    ClActUnit *moveUnit;                  // 0x1f8
    ClActor *moveActors[4];               // 0x1fc
    ClActUnit *unit20c;                   // 0x20c
    ClActor *actors210[2];                // 0x210
    ClActUnit *unit218;                   // 0x218
    ClActor *actor21c;                    // 0x21c
    BOOL noUsableMove;                    // 0x220
    u32 typePalette;                      // 0x224
    u32 typeCellAnims;                    // 0x228
    ClActUnit *typeUnit;                  // 0x22c
    ClActor *typeActors[22];              // 0x230
    BOOL typeLoaded;                      // 0x288
    u8 *typePalBuf;                       // 0x28c  0x180 bytes
    u8 *typePalBuf2;                      // 0x290  0x1e0 bytes
    s32 typeAnimCounter;                  // 0x294  the position in the animation's palette list
    s32 typeAnimFrame;                    // 0x298  frames since the last palette, 0 to 4
    s32 typeAnimIndex;                    // 0x29c
    s32 typeAnimRepeat;                   // 0x2a0
    BOOL typePalLoaded;                   // 0x2a4
    Font *font;                           // 0x2a8
    BmpWin *msgWin;                       // 0x2ac
    GFLBitmap *msgBitmap;                 // 0x2b0
    BmpWin *subWin;                       // 0x2b4
    GFLBitmap *subBitmap;                 // 0x2b8
    TCB *coreTask;                        // 0x2bc
    BOOL inBattle;                        // 0x2c0
    HeapID heapId;                        // 0x2c4
    u8 lastCursor[3][8];                  // 0x2c6  [pokeIndex][screen]
    u8 buttonEnabled[8];                  // 0x2de
    u8 moveExists[4];                     // 0x2e6
    u16 lastMove[3];                      // 0x2ea  [pokeIndex]
    u16 moves[4];                         // 0x2f0
    u8 *keyCursorFlag;                    // 0x2f8
    u32 pokePos;                          // 0x2fc
    s32 pokeIndex;                        // 0x300
    BOOL noMoveInfo;                      // 0x304  the move screen's parameter
    u16 savedPalette[16];                 // 0x308
    u8 unk328;                            // 0x328
    u8 fileSet;                           // 0x329  2 selects the second set of archive 11's files
    u8 typeCount;                         // 0x32a
    u32 typeMask;                         // 0x32c
    BattleMon *mons[3];                   // 0x330
    BOOL moveUsable[3][4];                // 0x33c
}; // 0x36c

#define BTLV_INPUT_TYPE_COUNT 22

// Archive 11's palette of the lower screen's BGs
#ifdef BLACK2
#define BTLV_INPUT_BG_PALETTE_FILE 0x168
#else
#define BTLV_INPUT_BG_PALETTE_FILE 0x167
#endif

// The work of a screen task, 0x24 bytes; the fields past seq are set by the task's starter
typedef struct {
    BtlvInput *work;
    s32 seq;
    u32 pos;     // the battle position
    u32 mapFile; // the BG 4 map of the screen, BtlvInput_ChooseButtonMap's or BtlvInput_DrawTargetPanels's
    s32 x;       // the recorder's BG 4 scroll
    s32 y;
    s32 wait;
    StrBuf *strbuf; // the recorder's message
    u32 prevScreen; // other to standby
} BtlvInputScreenTask;

// The work of a task that needs at most one value
typedef struct {
    BtlvInput *work;
    int value;
} BtlvInputSimpleTask;

// The work of a task that needs only the lower screen's work
typedef struct {
    BtlvInput *work;
} BtlvInputWorkTask;

typedef struct {
    BtlvInput *work;
    fx32 scale;
    fx32 target;
    fx32 step;
    int y;
} BtlvInputBallScale;

typedef struct {
    BtlvInput *work;
    int state;
    int x;
    int y;
    int step;
    int count;
} BtlvInputButtonRise;

typedef struct {
    BtlvInput *work;
    int step;
    int dir;
    int wait;
} BtlvInputFieldFrames;

typedef struct {
    BtlvInput *work;
    ClActUnit *unit;
    ClActor *actors[6];
} BtlvInputTileAnim;

typedef struct {
    BtlvInput *work;
    int dir;
} BtlvInputActorSlide;

typedef struct {
    BtlvInput *work;
    int bg;
    int x;
    int y;
    int bg4;
    int bg5;
    int bg6;
    int bg7;
} BtlvInputSetLayers;

// The press flash's task
typedef struct {
    BtlvInput *work;
    s32 seq;
    u32 rows;
    BOOL objOnly; // flashes the OBJ palette, for Struggle
} BtlvInputFlashWork;

// A BG to create, with its mode
typedef struct {
    BGSetup setup;
    u32 mode;
} BtlvInputBGSetup;

typedef struct {
    u8 x;
    u8 y;
    u8 palette;
    u8 pad;
} BtlvInputTypeIconPos;

// An animation of the type icons' palettes: how many palettes it shows and how often it repeats at most
typedef struct {
    s16 frames;
    s16 repeatMax;
} BtlvInputTypeAnim;

typedef struct {
    int x;
    int y;
} BtlvInputScroll;

typedef struct {
    ClActorPos pos[6];
    u32 sequences[2];
} BtlvInputTileSet;

// The game lays out the shared .rodata section by size, with the objects of one size in an order that depends on where
// each object is declared; no order of these declarations near the address order was found that gives the game's
// layout, so the objects of equal size come out in another order and the functions that address them from the
// section's base differ (BtlvInput_MoveScreenMain, BtlvInput_LoadTypeIcons)

// The move screen's slot of each party slot
static const u8 data_ov168_021f35cc[3] = { 1, 2, 3 };
// The stops the move screen's key cursor may come back to
static const u8 data_ov168_021f35cf[3] = { 5, 6, 0xff };
// A palette list of the type icons' animations
static const s16 data_ov168_021f35d2[2] = { 1, 0 };
// The pokeSlot that rotate buttons 5 and 6 switch to, by slot and button
static const u8 data_ov168_021f35d6[3][2] = { { 2, 1 }, { 0, 3 }, { 3, 0 } };
// Which of the two extra buttons of the move screen each party slot gets
static const u8 data_ov168_021f35dc[3][2] = { { 1, 1 }, { 1, 0 }, { 0, 1 } };
// The other palette lists of the type icons' animations
static const s16 data_ov168_021f35e2[4] = { 0, 0, 0, 0 };
static const s16 data_ov168_021f35ea[4] = { 1, 2, 1, 0 };
// The template of most actors, all zero: each actor is placed and animated after it is created
static const ClActorSetup data_ov168_021f35f2 = { 0, 0, 0, 0, 0 };
// The type icons' template, which BtlvInput_LoadTypeIcons places and gives a priority per icon
static const ClActorSetup data_ov168_021f35fa = { 0, 0, 0, 2, 1 };

static const s32 data_ov168_021f3670[7] = { 0, 0, 0, 0, 1, 0, 0 };
static const s32 data_ov168_021f368c[7] = { 0x200, 0x400, 0x800, 0x1000, 0x10, 0x4, 0x8 };
static const s32 data_ov168_021f36a8[7] = { 0, 0, 0, 0, 1, 0, 0 };
static const s32 data_ov168_021f36c4[7] = { 0, 0, 0, 0, 0x10, 0x4, 0x8 };

static const TouchRect data_ov168_021f3700[8] = {
    { 0x20, 0x50, 0x00, 0x80 }, { 0x20, 0x50, 0x80, 0xff }, { 0x50, 0x80, 0x00, 0x80 }, { 0x50, 0x80, 0x80, 0xff },
    { 0x90, 0xc0, 0xb0, 0xff }, { 0x98, 0xc0, 0x00, 0x50 }, { 0x98, 0xc0, 0x50, 0xa0 }, { TOUCH_RECT_END, 0, 0, 0 },
};
static const TouchRect data_ov168_021f3760[8] = {
    { 0x38, 0x68, 0x40, 0xc0 }, { 0x38, 0x68, 0x40, 0xc0 }, { 0x38, 0x68, 0x40, 0xc0 }, { 0x38, 0x68, 0x40, 0xc0 },
    { 0x90, 0xc0, 0xb0, 0xff }, { 0x98, 0xc0, 0x00, 0x50 }, { 0x98, 0xc0, 0x50, 0xa0 }, { TOUCH_RECT_END, 0, 0, 0 },
};

// The move screen's buttons, without a usable move and with one
static const BtlvInputButtonSet data_ov168_021f3604 = { data_ov168_021f3760, data_ov168_021f36a8, data_ov168_021f36c4 };
static const BtlvInputButtonSet data_ov168_021f3610 = { data_ov168_021f3700, data_ov168_021f3670, data_ov168_021f368c };

static const BtlvInputTypeAnim data_ov168_021f361c[3] = { { 4, 4 }, { 4, 4 }, { 2, 2 } };
// The party icons' heights, by row and direction
static const int data_ov168_021f3628[2][2] = { { 136, 136 }, { 24, 48 } };
// Where each move tile's type icon goes
static const ClActorPos data_ov168_021f3638[4] = { { 34, 65 }, { 162, 65 }, { 34, 113 }, { 162, 113 } };
static const BGSysLCDConfig data_ov168_021f3648 = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_3, GX_BG0_AS_2D };
static const u32 data_ov168_021f3658[3][2] = { { 0x247, 0x246 }, { 0x245, 0 }, { 0, 0x245 } };
static const BtlvInputScroll data_ov168_021f36e0[2][2] = { { { 256, 0 }, { 0, 192 } }, { { 256, 0 }, { 0, 0 } } };
// Where each move tile's name is centered
static const s32 data_ov168_021f3720[4][2] = { { 64, 10 }, { 192, 10 }, { 64, 58 }, { 192, 58 } };
// The scroll of BG 4 for the button rise of each slot
static const BtlvInputScroll data_ov168_021f3740[4] = { { 256, 448 }, { 256, 192 }, { 0, 192 }, { 0, 448 } };
// Where each party icon goes, by rule
static const ClActorPos data_ov168_021f3780[4][3] = {
    { { 128, 112 }, { -1, -1 }, { -1, -1 } },
    { { 104, 112 }, { 152, 112 }, { -1, -1 } },
    { { 88, 112 }, { 128, 112 }, { 168, 112 } },
    { { 104, 112 }, { 152, 112 }, { -1, -1 } },
};

// The tile palette of each move type
static const u32 data_ov168_021f3864[17] = {
    464, 470, 473, 471, 472, 476, 475, 477, 479, 465, 466, 468, 467, 469, 478, 474, 480,
};

// The key cursor's stops of the move screen, without a usable move and with one, per slot
static const BtlvInputKeyStop data_ov168_021f38a8[6] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, 5, KEY_CURSOR_NONE, 4, 4 },
    { { 5, 5, 5, 5, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 5, 4 },
};
static const BtlvInputKeyStop data_ov168_021f38f0[6] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, 5, KEY_CURSOR_NONE, 4, 4 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 6, 4 },
};
static const BtlvInputKeyStop data_ov168_021f3938[6] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 2, 2, 2, 2, -1, -1 }, 0, 5, KEY_CURSOR_NONE, 3, 2, 4 },
    { { 3, 3, 3, 3, -1, -1 }, 1, 4, 2, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 3, KEY_CURSOR_NONE, 5, KEY_CURSOR_NONE, 4, 4 },
    { { 5, 5, 5, 5, -1, -1 }, 2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 5, 4 },
};
static const BtlvInputKeyStop data_ov168_021f3980[6] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 2, 2, 2, 2, -1, -1 }, 0, 5, KEY_CURSOR_NONE, 3, 2, 4 },
    { { 3, 3, 3, 3, -1, -1 }, 1, 4, 2, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 3, KEY_CURSOR_NONE, 5, KEY_CURSOR_NONE, 4, 4 },
    { { 6, 6, 6, 6, -1, -1 }, 2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 4, 6, 4 },
};
static const BtlvInputKeyStop data_ov168_021f39c8[7] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, -5, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 0, KEY_CURSOR_NONE, 6, KEY_CURSOR_NONE, 4, 4 },
    { { 5, 5, 5, 5, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 5, 4 },
    { { 6, 6, 6, 6, -1, -1 }, 0, KEY_CURSOR_NONE, 5, 4, 6, 4 },
};
static const BtlvInputKeyStop data_ov168_021f3a1c[7] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 2, KEY_CURSOR_NONE, 1, 0, 4 },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, 3, 0, KEY_CURSOR_NONE, 1, 4 },
    { { 2, 2, 2, 2, -1, -1 }, 0, KEY_CURSOR_LAST | 5, KEY_CURSOR_NONE, 3, 2, 4 },
    { { 3, 3, 3, 3, -1, -1 }, 1, 4, 2, KEY_CURSOR_NONE, 3, 4 },
    { { 4, 4, 4, 4, -1, -1 }, 3, KEY_CURSOR_NONE, 6, KEY_CURSOR_NONE, 4, 4 },
    { { 5, 5, 5, 5, -1, -1 }, 2, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 6, 5, 4 },
    { { 6, 6, 6, 6, -1, -1 }, 2, KEY_CURSOR_NONE, 5, 4, 6, 4 },
};

// Where each type icon goes, with its palette
static const BtlvInputTypeIconPos data_ov168_021f3a70[BTLV_INPUT_TYPE_COUNT] = {
    { 0x20, 0x68, 3 }, { 0x50, 0x68, 3 }, { 0x80, 0x68, 3 }, { 0xb0, 0x68, 3 }, { 0xe0, 0x68, 3 }, { 0x08, 0x88, 2 },
    { 0x38, 0x88, 2 }, { 0x68, 0x88, 2 }, { 0x98, 0x88, 2 }, { 0xc8, 0x88, 2 }, { 0xf8, 0x88, 2 }, { 0x20, 0xa8, 1 },
    { 0x50, 0xa8, 1 }, { 0x80, 0xa8, 1 }, { 0xb0, 0xa8, 1 }, { 0xe0, 0xa8, 1 }, { 0x08, 0xc8, 0 }, { 0x38, 0xc8, 0 },
    { 0x68, 0xc8, 0 }, { 0x98, 0xc8, 0 }, { 0xc8, 0xc8, 0 }, { 0xf8, 0xc8, 0 },
};

// Where each target panel's name is centered, by rule
static const s32 data_ov168_021f3ac8[2][6][2] = {
    { { 64, 68 }, { 192, 20 }, { 192, 68 }, { 64, 20 }, { 0, 0 }, { 0, 0 } },
    { { 48, 68 }, { 208, 20 }, { 128, 68 }, { 128, 20 }, { 208, 68 }, { 48, 20 } },
};

// The tiles that grow or shrink, by set
static const BtlvInputTileSet data_ov168_021f3b28[3] = {
    { { { 64, 56 }, { 192, 56 }, { 64, 104 }, { 192, 104 }, { -1, -1 }, { -1, -1 } }, { 8, 9 } },
    { { { 64, 56 }, { 192, 56 }, { 64, 104 }, { 192, 104 }, { 64, 104 }, { 192, 104 } }, { 8, 9 } },
    { { { 64, 56 }, { 192, 56 }, { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } }, { 8, 9 } },
};

static const BtlvInputBGSetup data_ov168_021f3b88[4] = {
    { { 0, 0, 0x2000, 0, BGRES_512x512, GX_BG_COLORMODE_16, 0, 4, 0x8000, 0, 1, 0, 0 }, BGMODE_TEXT },
    { { 0, 0, 0x2000, 0, BGRES_512x512, GX_BG_COLORMODE_16, 4, 4, 0x8000, 0, 1, 0, 0 }, BGMODE_TEXT },
    { { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, 8, 6, 0x8000, 0, 0, 0, 0 }, BGMODE_TEXT },
    { { 0, 0, 0x2000, 0, BGRES_128x128, GX_BG_COLORMODE_16, 0, 0, 0x8000, 0, 1, 0, 0 }, BGMODE_EXTENDED },
};

// The target screen's BG 4 map, by rule, attacker and move range
static const u32 data_ov168_021f3c18[2][3][15] = {
    {
        { 418, 415, 414, 411, 416, 410, 412, 413, 409, 413, 409, 410, 412, 413, 418 },
        { 419, 415, 413, 411, 417, 410, 412, 414, 409, 414, 409, 410, 412, 414, 419 },
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    },
    {
        { 404, 398, 407, 389, 401, 386, 393, 395, 384, 395, 384, 385, 393, 395, 391 },
        { 403, 397, 406, 388, 400, 385, 393, 394, 384, 394, 384, 385, 393, 394, 403 },
        { 405, 399, 408, 390, 402, 387, 393, 396, 384, 396, 384, 385, 393, 396, 392 },
    },
};

// The key cursor's stops of the move screen of each slot, without a usable move and with one
static const BtlvInputKeyStop *data_ov168_021f41a4[3] = { data_ov168_021f39c8, data_ov168_021f38a8,
                                                          data_ov168_021f38f0 };
// The palette lists of the type icons' animations
static const s16 *data_ov168_021f41b0[3] = { data_ov168_021f35ea, data_ov168_021f35e2, data_ov168_021f35d2 };
static const BtlvInputKeyStop *data_ov168_021f41bc[3] = { data_ov168_021f3a1c, data_ov168_021f3938,
                                                          data_ov168_021f3980 };

static BtlvInput *BtlvInput_Create(GameData *gameData, u32 rule, u32 unk54, PaletteFade *paletteFade, Font *font,
                                   u8 *keyCursorFlag, u32 scdState, BOOL inBattle, u8 unk328, u32 fileSet,
                                   u32 typeCount, u32 typeMask, BOOL noCancel, HeapID heapId);
static void BtlvInput_CoreTask(TCB *tcb, void *data);
static void BtlvInput_InitGraphics(BtlvInput *work);
static void BtlvInput_FreeGraphics(BtlvInput *work);
static void BtlvInput_CreateBGs(BtlvInput *work);
static void BtlvInput_ReleaseBGs(void);
static s32 BtlvInput_CheckRecorderInput(BtlvInput *work, const BtlvInputButtonSet *set, s32 arg2);
static s32 BtlvInput_CheckRecorderCancel(BtlvInput *work, const BtlvInputButtonSet *set, s32 arg2);
static u32 BtlvInput_FileId(u32 fileId, BtlvInput *work);
static void BtlvInput_LoadGraphics(BtlvInput *work);
static void BtlvInput_StandbyToCommandTask(TCB *tcb, void *data);
static void BtlvInput_StandbyToMovesTask(TCB *tcb, void *data);
static void BtlvInput_CommandToMovesTask(TCB *tcb, void *data);
static void BtlvInput_MovesToCommandTask(TCB *tcb, void *data);
static void BtlvInput_ToTargetTask(TCB *tcb, void *data);
static void BtlvInput_TargetToMovesTask(TCB *tcb, void *data);
static void BtlvInput_CommandToStandbyTask(TCB *tcb, void *data);
static void BtlvInput_OtherToStandbyTask(TCB *tcb, void *data);
static void BtlvInput_YesNoTask(TCB *tcb, void *data);
static void BtlvInput_StandbyToMovesSlotTask(TCB *tcb, void *data);
static void BtlvInput_StandbyToMessageTask(TCB *tcb, void *data);
static void BtlvInput_LoadTypeIcons(BtlvInput *work);
static void BtlvInput_FreeTypeIcons(BtlvInput *work);
static BOOL BtlvInput_TestTypeBit(u32 mask, int bit);
static void BtlvInput_SetTypeBit(u32 *mask, int bit);
static void BtlvInput_ApplyTypeMask(BtlvInput *work);
static void BtlvInput_ShowTypeScreen(BtlvInput *work);
static void BtlvInput_LoadTypePalettes(BtlvInput *work);
static void BtlvInput_LoadTypePalette(const BtlvInput *work, int palette);
static void BtlvInput_UpdateTypePaletteAnim(BtlvInput *work);
static void BtlvInput_SetTypeAnimIndex(BtlvInput *work, int index);
static int BtlvInput_PickTypeAnimIndex(BtlvInput *work, int current);
static u32 BtlvInput_Random(BtlvInput *work, u32 max);
static void BtlvInput_RecorderOpenTask(TCB *tcb, void *data);
static void BtlvInput_RotationScreenTask(TCB *tcb, void *data);
static void BtlvInput_RotationSwitchTask(TCB *tcb, void *data);
static void BtlvInput_ScreenTaskEnd(TCB *tcb);
static void BtlvInput_StartBallScale(BtlvInput *work, fx32 from, fx32 to, fx32 step, int y);
static void BtlvInput_BallScaleTask(TCB *tcb, void *data);
static void BtlvInput_BallScaleEnd(TCB *tcb);
static void BtlvInput_StartButtonRise(BtlvInput *work, int x, int y, int step, int count);
static void BtlvInput_ButtonRiseTask(TCB *tcb, void *data);
static void BtlvInput_ButtonRiseEnd(TCB *tcb);
static void BtlvInput_StartFieldFrames(BtlvInput *work, int unused, int dir);
static void BtlvInput_FieldFramesTask(TCB *tcb, void *data);
static void BtlvInput_FieldFramesEnd(TCB *tcb);
static void BtlvInput_StartTileAnim(BtlvInput *work, int set, int which);
static void BtlvInput_TileAnimTask(TCB *tcb, void *data);
static void BtlvInput_TileAnimEnd(TCB *tcb);
static void BtlvInput_StartActorSlide(BtlvInput *work, int dir);
static void BtlvInput_ActorSlideTask(TCB *tcb, void *data);
static void BtlvInput_ActorSlideEnd(TCB *tcb);
static void BtlvInput_StartSetLayers(BtlvInput *work, int bg, int x, int y, int bg4, int bg5, int bg6, int bg7);
static void BtlvInput_SetLayersTask(TCB *tcb, void *data);
static void BtlvInput_FadeWaitTask(TCB *tcb, void *data);
static void BtlvInput_FadeWaitEnd(TCB *tcb);
static void BtlvInput_SlideActorsLeftTask(TCB *tcb, void *data);
static void BtlvInput_SlideActorsLeftEnd(TCB *tcb);
static void BtlvInput_StartRotationSwitch(BtlvInput *work, int screen);
static void BtlvInput_SetScdStateTask(TCB *tcb, void *data);
static void BtlvInput_WindowDarkenTask(TCB *tcb, void *data);
static void BtlvInput_WindowClearTask(TCB *tcb, void *data);
static void BtlvInput_StartClearBlend(BtlvInput *work);
static void BtlvInput_ClearBlendTask(TCB *tcb, void *data);
static void BtlvInput_GetStringWidth(const StrBuf *strbuf, Font *font, int *width, int *blocks);
static void BtlvInput_DrawMoveTiles(BtlvInput *work, const BtlvInputMoveParam *param);
static void BtlvInput_DrawTargetPanels(BtlvInput *work, BtlvInputScreenTask *task, const BtlvInputTargetParam *param);
static void BtlvInput_DrawYesNo(BtlvInput *work, const BtlvInputYesNoParam *param);
static void BtlvInput_SetupMoveScreen(BtlvInput *work);
static void BtlvInput_DrawRecorderCount(BtlvInput *work, const BtlvInputRecorderParam *param);
static void BtlvInput_ClearScreen(BtlvInput *work);
static s32 BtlvInput_GetPPColor(s32 pp, s32 maxPP);
static void BtlvInput_CreatePartyIcons(BtlvInput *work, const BtlvInputCommandParam *param);
static void BtlvInput_DeletePartyIcons(BtlvInput *work);
static void BtlvInput_SetPartyIconsActive(BtlvInput *work, BOOL active);
static void BtlvInput_CreateBallSlideActors(BtlvInput *work, int index);
static void BtlvInput_DeleteBallSlideActors(BtlvInput *work);
static void BtlvInput_CreatePokeListIcons(BtlvInput *work, int side);
static void BtlvInput_DeletePokeListIcons(BtlvInput *work);
static void BtlvInput_CreateCursorActors(BtlvInput *work);
static void BtlvInput_DeleteCursorActors(BtlvInput *work);
static void BtlvInput_DeleteMoveTypeIcons(BtlvInput *work);
static void BtlvInput_CreateStruggleActor(BtlvInput *work);
static void BtlvInput_DeleteStruggleActor(BtlvInput *work);
static s32 BtlvInput_KeyInput(BtlvInput *work, const BtlvInputButtonSet *set, const BtlvInputKeyStop *stops,
                              const u8 *lastStops, s32 result, BOOL noMoveInfo);
static s32 BtlvInput_YesNoKeyInput(BtlvInput *work);
static void BtlvInput_UpdateKeyCursorActors(BtlvInput *work, const TouchRect *rects, const BtlvInputKeyStop *stops);
static s32 BtlvInput_StartPressFlash(BtlvInput *work, s32 index, s32 rows);
static void BtlvInput_PressFlashTask(TCB *tcb, void *data);
static void BtlvInput_PressFlashEnd(TCB *tcb);
static void BtlvInput_PrintLauncherPoints(BtlvInput *work, BtlvInputCommandParam *param);
static void BtlvInput_StartFingerCursor(BtlvInput *work);
static void BtlvInput_ForgetCursorOnPokeChange(BtlvInput *work);
static void BtlvInput_AddTask(BtlvInput *work, TCB *task, void (*endFunc)(TCB *tcb));
static int BtlvInput_FindTask(BtlvInput *work, TCB *task);
static void BtlvInput_EndTask(BtlvInput *work, TCB *task);
static int BtlvInput_FreeTaskSlot(BtlvInput *work);
static void BtlvInput_EndAllTasks(BtlvInput *work);
static BOOL BtlvInput_IsCancelButton(BtlvInput *work, const BtlvInputButtonSet *set, u32 index);
static void BtlvInput_RestoreKeyCursor(BtlvInput *work);
static void BtlvInput_LoadDarkPalette(BtlvInput *work);
static void BtlvInput_RestoreSavedPalette(BtlvInput *work);
static u32 BtlvInput_ChooseButtonMap(BtlvInput *work);
static BOOL BtlvInput_SoundsEnabled(BtlvInput *work);
static void BtlvInput_PlayOpenSE(BtlvInput *work);
static void BtlvInput_PlayCursorSE(BtlvInput *work);
static void BtlvInput_PlayDecideSE(BtlvInput *work);
static void BtlvInput_PlayCancelSE(BtlvInput *work);
static void BtlvInput_PlayRotationSE(BtlvInput *work);
static void BtlvInput_PlayDecideSE2(BtlvInput *work);
static void BtlvInput_PlayBeepSE(BtlvInput *work);

BtlvInput *BtlvInput_CreateSimple(GameData *gameData, u32 rule, PaletteFade *paletteFade, Font *font, u8 *keyCursorFlag,
                                  HeapID heapId) {
    return BtlvInput_Create(gameData, rule, 0, paletteFade, font, keyCursorFlag, 0, FALSE, 0, 0, 0, 0, FALSE, heapId);
}

BtlvInput *BtlvInput_CreateForBattle(GameData *gameData, u32 rule, u32 unk54, PaletteFade *paletteFade, Font *font,
                                     u8 *keyCursorFlag, u32 scdState, u8 unk328, u32 fileSet, u32 typeCount,
                                     u32 typeMask, BOOL noCancel, HeapID heapId) {
    return BtlvInput_Create(gameData, rule, unk54, paletteFade, font, keyCursorFlag, scdState, TRUE, unk328, fileSet,
                            typeCount, typeMask, noCancel, heapId);
}

static BtlvInput *BtlvInput_Create(GameData *gameData, u32 rule, u32 unk54, PaletteFade *paletteFade, Font *font,
                                   u8 *keyCursorFlag, u32 scdState, BOOL inBattle, u8 unk328, u32 fileSet,
                                   u32 typeCount, u32 typeMask, BOOL noCancel, HeapID heapId) {
    BtlvInput *work;
    BtlvInputSimpleTask *coreTask;
    int i;

    work = GFL_HeapAllocate(heapId, sizeof(BtlvInput), TRUE, "btlv_input.c", 0x409);
    work->heapId = heapId;
    work->tcbMgrBuf = GFL_HeapAllocate(work->heapId, GFL_TCBMgrCalcAllocSize(8), TRUE, "btlv_input.c", 0x40d);
    work->tcbMgr = GFL_TCBMgrCreate(8, work->tcbMgrBuf);
    work->gameData = gameData;
    work->font = font;
    work->rule = rule;
    work->unk54 = unk54;
    work->unk328 = unk328;
    work->keyCursorFlag = keyCursorFlag;
    work->prevCursor = 0xf;
    work->inBattle = inBattle;
    work->fileSet = fileSet;
    work->typeCount = typeCount;
    work->typeMask = typeMask;
    work->noCancel = noCancel;
    work->paletteFade = paletteFade;
    work->chars = -1;
    work->palette = -1;
    work->cellAnims = -1;
    work->chars2 = -1;
    work->cellAnims2 = -1;
    work->iconPalette = -1;
    work->iconCellAnims = -1;
    work->movePalette = -1;
    work->moveCellAnims = -1;
    for (i = 0; i < 3; i++) {
        work->iconChars[i] = -1;
    }
    for (i = 0; i < 4; i++) {
        work->moveChars[i] = -1;
    }
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0021, work->heapId);
    work->pressedButton = -1;
    work->typeLoaded = FALSE;
    work->typePalBuf = NULL;
    work->typePalBuf2 = NULL;
    work->typeAnimCounter = 0;
    work->typeAnimFrame = 0;
    work->typeAnimIndex = 0;
    work->typeAnimRepeat = 0;
    work->typePalLoaded = FALSE;
    BtlvInput_InitGraphics(work);

    coreTask = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputSimpleTask), FALSE, "btlv_input.c", 0x449);
    coreTask->work = work;
    coreTask->value = scdState;
    BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_SetScdStateTask, coreTask, 0), NULL);
    return work;
}

void BtlvInput_Delete(BtlvInput *work) {
    if (work != NULL) {
        BtlvInput_EndAllTasks(work);
        BtlvInput_FreeGraphics(work);
        GFL_MsgDataFree(work->msgData);
        if (work->fingerCursor != NULL) {
            BtlvFingerCursor_Delete(work->fingerCursor);
        }
        func_0203a610(work->tcbMgr);
        GFL_HeapFree(work->tcbMgrBuf);
        GFL_HeapFree(work);
    }
}

void BtlvInput_Update(BtlvInput *work) {
    GFL_TCBMgrUpdate(work->tcbMgr);
    InfoWin_Update();
}

static void BtlvInput_CoreTask(TCB *tcb, void *data) {
    BtlvInput *work = data;

    GFL_TCBMgrUpdate(work->tcbMgr);
    InfoWin_Update();
    BtlvInput_UpdateTypePaletteAnim(work);
}

static void BtlvInput_InitGraphics(BtlvInput *work) {
    GFL_BGSysSetLCDConfigForEngine(&data_ov168_021f3648, BGSYS_ENGINE_SUB);
    work->arc = GFL_ArcSysCreateFileHandle(11, work->heapId);
    work->moveUnit = func_0204bf1c(4, 2, work->heapId);
    work->pokeUnit = func_0204bf1c(12, 2, work->heapId);
    work->cursorUnit = func_0204bf1c(6, 0, work->heapId);
    work->iconUnit = func_0204bf1c(3, 2, work->heapId);
    work->unit20c = func_0204bf1c(2, 2, work->heapId);
    work->unit218 = func_0204bf1c(1, 1, work->heapId);
    work->typeUnit = func_0204bf1c(22, 1, work->heapId);
    BtlvInput_CreateBGs(work);
    BtlvInput_LoadGraphics(work);
    BtlvInput_CreateCursorActors(work);

    work->msgWin = BmpWin_CreateDynamic(6, 0, 4, 32, 12, 13, TRUE);
    work->msgBitmap = BmpWin_GetBitmap(work->msgWin);
    work->subWin = BmpWin_CreateDynamic(6, 3, 22, 3, 2, 13, TRUE);
    work->subBitmap = BmpWin_GetBitmap(work->subWin);
    GFL_BitmapFill(work->msgBitmap, 0);
    GFL_BitmapFill(work->subBitmap, 0);
    BmpWin_FlushChar(work->msgWin);
    BmpWin_FlushChar(work->subWin);
    BmpWin_FlushMap(work->msgWin);
    BmpWin_FlushMap(work->subWin);
    GFL_BGSysLoadScr(6);
    InfoWin_Init(6, 15, work->gameData, work->heapId);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    func_02042ba8(FALSE, work->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, 3, 0xe0, 0x20);
    work->cursorRefresh = TRUE;
    if (work->inBattle == TRUE) {
        work->coreTask = GFL_TCBMgrAddTask(BtlvEffect_GetTCBManager(), BtlvInput_CoreTask, work, 0);
    }
    BtlvInput_SetScreen(work, 0, NULL);
}

static void BtlvInput_FreeGraphics(BtlvInput *work) {
    int i;

    BtlvInput_DeleteMoveTypeIcons(work);
    BtlvInput_DeletePokeListIcons(work);
    BtlvInput_DeleteCursorActors(work);
    BtlvInput_DeletePartyIcons(work);
    BtlvInput_DeleteBallSlideActors(work);
    if (work->chars != -1) {
        func_0204b98c(work->chars);
        work->chars = -1;
    }
    if (work->palette != -1) {
        func_0204bcd0(work->palette);
        work->palette = -1;
    }
    if (work->cellAnims != -1) {
        func_0204be64(work->cellAnims);
        work->cellAnims = -1;
    }
    if (work->chars2 != -1) {
        func_0204b98c(work->chars2);
        work->chars2 = -1;
    }
    if (work->cellAnims2 != -1) {
        func_0204be64(work->cellAnims2);
        work->cellAnims2 = -1;
    }
    for (i = 0; i < 4; i++) {
        if (work->moveChars[i] != -1) {
            func_0204b98c(work->moveChars[i]);
            work->moveChars[i] = -1;
        }
    }
    if (work->moveCellAnims != -1) {
        func_0204be64(work->moveCellAnims);
        work->moveCellAnims = -1;
    }
    if (work->movePalette != -1) {
        func_0204bcd0(work->movePalette);
        work->movePalette = -1;
    }
    if (work->typeLoaded) {
        BtlvInput_FreeTypeIcons(work);
        work->typeLoaded = FALSE;
    }
    if (work->moveUnit != NULL) {
        func_0204bf98(work->moveUnit);
        work->moveUnit = NULL;
    }
    if (work->pokeUnit != NULL) {
        func_0204bf98(work->pokeUnit);
        work->pokeUnit = NULL;
    }
    if (work->cursorUnit != NULL) {
        func_0204bf98(work->cursorUnit);
        work->cursorUnit = NULL;
    }
    if (work->iconUnit != NULL) {
        func_0204bf98(work->iconUnit);
        work->iconUnit = NULL;
    }
    if (work->unit20c != NULL) {
        func_0204bf98(work->unit20c);
        work->unit20c = NULL;
    }
    if (work->unit218 != NULL) {
        func_0204bf98(work->unit218);
        work->unit218 = NULL;
    }
    if (work->typeUnit != NULL) {
        func_0204bf98(work->typeUnit);
    }
    if (work->typePalBuf != NULL) {
        GFL_HeapFree(work->typePalBuf);
        work->typePalBuf = NULL;
    }
    if (work->typePalBuf2 != NULL) {
        GFL_HeapFree(work->typePalBuf2);
        work->typePalBuf2 = NULL;
    }
    if (work->msgWin != NULL) {
        BmpWin_Free(work->msgWin);
        work->msgWin = NULL;
    }
    if (work->subWin != NULL) {
        BmpWin_Free(work->subWin);
        work->subWin = NULL;
    }
    InfoWin_Exit();
    BtlvInput_ReleaseBGs();
    if (work->coreTask != NULL) {
        GFL_TCBRemove(work->coreTask);
        work->coreTask = NULL;
    }
    if (work->arc != NULL) {
        GFL_ArcToolFree(work->arc);
        work->arc = NULL;
    }
    work->screen = 0;
}

static void BtlvInput_CreateBGs(BtlvInput *work) {
    u32 i;

    for (i = 0; i < 4; i++) {
        GFL_BGSysCreateBG(i + 4, &data_ov168_021f3b88[i].setup, data_ov168_021f3b88[i].mode);
        GFL_BGSysFillScr(i + 4, 0x3ff);
        BtlvInput_StartSetLayers(work, i + 4, 0, 0, 2, 2, 2, 2);
    }
}

static void BtlvInput_ReleaseBGs(void) {
    u32 i;

    for (i = 0; i < 4; i++) {
        GFL_BGSysSetBGEnabled(i + 4, FALSE);
        GFL_BGSysReleaseBG(i + 4);
    }
}

void BtlvInput_StartPaletteFade(BtlvInput *work, u32 mode) {
    BtlvInputWorkTask *task;

    task = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputWorkTask), FALSE, "btlv_input.c", 0x58d);
    if (mode == 1) {
        PaletteFade_StartFade(work->paletteFade, 0xa, 0xffff, 1, 0, 16, 0, work->tcbMgr);
    } else {
        PaletteFade_StartFade(work->paletteFade, 0xa, 0xffff, 1, 12, 16, 0, work->tcbMgr);
    }
    work->fadeMode = 1;
    task->work = work;
    BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_FadeWaitTask, task, 0), BtlvInput_FadeWaitEnd);
}

void BtlvInput_StartFadeInWithGraphics(BtlvInput *work) {
    BtlvInputWorkTask *task;

    task = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputWorkTask), FALSE, "btlv_input.c", 0x5a2);
    if (gfxRegGetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR) <= 0) {
        GFL_FadeSet(FADE_ENGINE_B_BLACK, 16, 0, 0);
    } else {
        GFL_FadeSet(FADE_ENGINE_B_WHITE, 16, 0, 0);
    }
    BtlvInput_InitGraphics(work);
    work->fadeMode = 2;
    task->work = work;
    BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_FadeWaitTask, task, 0), BtlvInput_FadeWaitEnd);
}

BOOL BtlvInput_IsFading(BtlvInput *work) {
    if (work->fadeMode != 0) {
        return TRUE;
    }
    return FALSE;
}

void BtlvInput_SetScreen(BtlvInput *work, u32 screen, void *param) {
    BtlvInput_ClearScreen(work);
    work->fingerSeq = 0;
    work->canCancel = TRUE;

    switch (screen) {
    case 0: {
        BtlvInputScreenTask *task;

        work->pressedButton = -1;
        if (work->screen == 0) {
            BtlvInput_StartBallScale(work, 0x3000, 0x3000, 0, 0x98);
            BtlvInput_StartSetLayers(work, -1, 0, 0, 0, 0, 2, 1);
            if (work->fileSet == 2) {
                BtlvInput_ShowTypeScreen(work);
            } else {
                PaletteFade_StartFade(work->paletteFade, 2, 3, 1, 12, 12, 0x842, work->tcbMgr);
            }
        } else {
            task =
                GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputScreenTask), TRUE, "btlv_input.c", 0x5e7);
            work->busy = TRUE;
            task->work = work;
            if (work->inBattle == TRUE) {
                BtlvEffect_StopIdleEffect();
            }
            BtlvInput_DeletePokeListIcons(work);
            BtlvInput_DeletePartyIcons(work);
            if (work->screen == 1 || work->screen == 4) {
                BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_CommandToStandbyTask, task, 1),
                                  BtlvInput_ScreenTaskEnd);
            } else {
                task->prevScreen = work->screen;
                BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_OtherToStandbyTask, task, 1),
                                  BtlvInput_ScreenTaskEnd);
            }
            PaletteFade_RestartFade(work->paletteFade, 2, 3, 1, 0, 12, 0x842, work->tcbMgr);
            if (func_0204bfe8(work->cursorUnit)) {
                work->cursor = 0;
                work->prevCursor = 0xf;
                work->cursorRefresh = TRUE;
                func_0204bfd4(work->cursorUnit, FALSE);
            }
        }
        break;
    }
    case 1: {
        BtlvInputScreenTask *task;
        BtlvInputCommandParam *cmd = param;
        int i;

        task = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputScreenTask), TRUE, "btlv_input.c", 0x60e);
        work->pokePos = cmd->pokePos;
        BtlvInput_StartFingerCursor(work);
        BtlvEffect_SetIdleEffectMode(2);
        if (cmd->pokePos >= 2) {
            cmd->pokePos = (cmd->pokePos - 2) / 2;
        }
        work->busy = TRUE;
        work->laterPoke = cmd->laterPoke;
        task->work = work;
        task->pos = cmd->pokePos;
        work->pokeIndex = cmd->pokePos;
        if (work->screen == 0) {
            BtlvInput_ForgetCursorOnPokeChange(work);
        }
        for (i = 0; i < 6; i++) {
            work->pokeEntries[0][i] = cmd->entries[0][i];
            work->pokeEntries[1][i] = cmd->entries[1][i];
        }
        for (i = 0; i < 4; i++) {
            work->buttonEnabled[i] = TRUE;
        }
        work->buttonEnabled[1] = BtlvEffect_GetUnk304();
        BtlvInput_DeletePartyIcons(work);
        BtlvInput_CreatePartyIcons(work, cmd);
        BtlvInput_CreateBallSlideActors(work, cmd->ballSlide);
        task->mapFile = BtlvInput_ChooseButtonMap(work);
        BtlvInput_PrintLauncherPoints(work, cmd);
        if (work->screen == 2 || work->screen == 5) {
            BtlvInput_SetPartyIconsActive(work, 0);
            if (task->work->iconActors[0].actor != NULL) {
                BtlvInput_StartClearBlend(work);
            }
            BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_MovesToCommandTask, task, 1),
                              BtlvInput_ScreenTaskEnd);
        } else {
            BtlvInput_CreatePokeListIcons(work, FALSE);
            if (cmd->secondRow) {
                BtlvInput_CreatePokeListIcons(work, TRUE);
            }
            work->secondRow = cmd->secondRow;
            BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_StandbyToCommandTask, task, 1),
                              BtlvInput_ScreenTaskEnd);
        }
        break;
    }
    case 2: {
        BtlvInputScreenTask *task;
        const BtlvInputMoveParam *mv = param;
        int i;

        task = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputScreenTask), TRUE, "btlv_input.c", 0x660);
        BtlvInput_DrawMoveTiles(work, mv);
        work->busy = TRUE;
        task->work = work;
        task->pos = mv->pos;
        work->noMoveInfo = mv->noMoveInfo;
        task->mapFile = BtlvInput_ChooseButtonMap(work);
        for (i = 0; i < 4; i++) {
            work->moves[i] = mv->moves[i];
        }
        if (work->inBattle == TRUE && (work->rule == 1 || work->rule == 2)) {
            BtlvEffect_RestartIdleEffect(2);
            BtlvEffect_Start(0x244);
        }
        if (work->screen == 3) {
            BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_TargetToMovesTask, task, 1),
                              BtlvInput_ScreenTaskEnd);
        } else if (work->screen == 0) {
            BtlvInput_CreatePokeListIcons(work, FALSE);
            if (work->secondRow) {
                BtlvInput_CreatePokeListIcons(work, TRUE);
            }
            BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_StandbyToMovesTask, task, 1),
                              BtlvInput_ScreenTaskEnd);
        } else {
            BtlvInput_DeletePartyIcons(work);
            BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_CommandToMovesTask, task, 1),
                              BtlvInput_ScreenTaskEnd);
        }
        break;
    }
    case 3: {
        BtlvInputScreenTask *task;
        const BtlvInputTargetParam *target = param;

        task = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputScreenTask), TRUE, "btlv_input.c", 0x68f);
        BtlvInput_DrawTargetPanels(work, task, target);
        work->busy = TRUE;
        task->work = work;
        task->pos = target->attackerPos;
        BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_ToTargetTask, task, 1),
                          BtlvInput_ScreenTaskEnd);
        break;
    }
    case 4: {
        BtlvInputScreenTask *task;
        const BtlvInputYesNoParam *yesNo = param;

        task = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputScreenTask), TRUE, "btlv_input.c", 0x69b);
        BtlvInput_DrawYesNo(work, yesNo);
        work->busy = TRUE;
        task->work = work;
        work->canCancel = yesNo->canCancel;
        BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_YesNoTask, task, 1), BtlvInput_ScreenTaskEnd);
        break;
    }
    case 5: {
        BtlvInputScreenTask *task;
        const BtlvInputRotationParam *rotation = param;
        int i;
        int j;

        task = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputScreenTask), TRUE, "btlv_input.c", 0x6a8);
        if (work->screen != 0) {
            work->unk68_17 = 0;
            work->pokeSlot = 0;
        }
        for (i = 0; i < 3; i++) {
            work->mons[i] = rotation->mons[i];
            for (j = 0; j < 4; j++) {
                work->moveUsable[i][j] = rotation->moveUsable[i][j];
            }
        }
        BtlvInput_SetupMoveScreen(work);
        work->busy = TRUE;
        task->work = work;
        if (work->screen == 0) {
            BtlvInput_CreatePokeListIcons(work, FALSE);
            if (work->secondRow) {
                BtlvInput_CreatePokeListIcons(work, TRUE);
            }
            BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_StandbyToMovesSlotTask, task, 1),
                              BtlvInput_ScreenTaskEnd);
        } else {
            BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_RotationScreenTask, task, 1),
                              BtlvInput_ScreenTaskEnd);
        }
        break;
    }
    case 6: {
        BtlvInputScreenTask *task;
        const BtlvInputRecorderParam *recorder = param;
        StrBuf *strbuf = NULL;
        // func_ov167_0219c988 is the battle's Pokestar Studios mode, 0 for none; mode 2 shows this screen without its
        // count and animation
        BOOL studioMode2 = FALSE;
        int i;

        if (func_ov167_0219c988(BtlvEffect_GetMainModule()) == 2) {
            studioMode2 = TRUE;
        }
        for (i = 0; i < 3; i++) {
            work->buttonEnabled[i] = TRUE;
        }
        if (!studioMode2) {
            BtlvInput_DrawRecorderCount(work, recorder);
        }
        switch (recorder->mode) {
        case 0:
            break;
        case 1:
        case 2:
        case 3:
            GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
            strbuf = GFL_MsgDataLoadStrbufNew(work->msgData, recorder->mode + 8);
            break;
        }
        if (work->screen == 0) {
            task =
                GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputScreenTask), TRUE, "btlv_input.c", 0x70a);
            work->busy = TRUE;
            task->work = work;
            if (recorder->mode == 0) {
                task->x = 0;
                task->y = 0x100;
            } else {
                task->x = 0x100;
                task->y = 0;
            }
            task->strbuf = strbuf;
            BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_StandbyToMessageTask, task, 1),
                              BtlvInput_ScreenTaskEnd);
        } else {
            if (!studioMode2) {
                if (recorder->mode == 0) {
                    BtlvInput_StartSetLayers(work, 4, 0, 0x100, 2, 2, 2, 2);
                } else {
                    BtlvInput_StartSetLayers(work, 4, 0x100, 0, 2, 2, 2, 2);
                }
            }
            if (strbuf != NULL) {
                GFL_TextRendererDrawToBitmap(work->msgBitmap, 24, 56, strbuf, work->font);
                GFL_StrBufFree(strbuf);
            }
            BmpWin_FlushMap(work->msgWin);
            GFL_BGSysLoadScr(6);
            BmpWin_FlushChar(work->msgWin);
        }
        break;
    }
    case 7: {
        BtlvInputScreenTask *task;

        task = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputScreenTask), TRUE, "btlv_input.c", 0x73a);
        work->busy = TRUE;
        task->work = work;
        BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_RecorderOpenTask, task, 1),
                          BtlvInput_ScreenTaskEnd);
        work->buttonEnabled[0] = TRUE;
        work->buttonEnabled[1] = TRUE;
        break;
    }
    }
    work->screen = screen;
}

static s32 BtlvInput_CheckRecorderInput(BtlvInput *work, const BtlvInputButtonSet *set, s32 arg2) {
    s32 button = func_0203da0c(set->rects);

    if (button != -1) {
        if (!work->buttonEnabled[button]) {
            button = -1;
        } else if (button == 2) {
            BtlvInput_PlayCancelSE(work);
        } else {
            BtlvInput_PlayDecideSE(work);
        }
    } else {
        button = BtlvInput_YesNoKeyInput(work);
    }
    if (button != -1) {
        button = BtlvInput_StartPressFlash(work, button, set->flashRows[button]);
    }
    return button;
}

static s32 BtlvInput_CheckRecorderCancel(BtlvInput *work, const BtlvInputButtonSet *set, s32 arg2) {
    u32 keys = GCTX_HIDGetTypedKeys();
    s32 button = -1;

    if ((keys & PAD_BUTTON_B) && !work->noCancel) {
        BtlvInput_PlayCancelSE(work);
        button = 2;
    }
    return button;
}

s32 BtlvInput_CheckInput(BtlvInput *work, const BtlvInputButtonSet *set, const BtlvInputKeyStop *stops) {
    s32 button;
    s32 touched;
    u32 keys;

    if (work->busy || PaletteFade_GetActiveMask(work->paletteFade) != 0 || work->inputLocked) {
        return -1;
    }
    if (work->pressedButton != -1) {
        s32 pressed = work->pressedButton;

        work->selectionLocked = FALSE;
        work->pressedButton = -1;
        return pressed;
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) &&
        (work->screen == 1 || work->screen == 2 || work->screen == 3)) {
        BtlvEffect_SetGaugeFlag();
    }
    if (work->screen == 6) {
        if (work->fileSet != 2) {
            return BtlvInput_CheckRecorderInput(work, set, -1);
        }
        return BtlvInput_CheckRecorderCancel(work, set, -1);
    }

    touched = func_0203da0c(set->rects);
    button = BtlvInput_KeyInput(work, set, stops, NULL, touched, work->noMoveInfo);
    if (button != -1) {
        keys = GCTX_HIDGetHeldKeys();
        if (!work->buttonEnabled[button] && (!(keys & PAD_BUTTON_L) || work->screen != 2)) {
            if (work->moveExists[button] && work->screen == 2) {
                BtlvInput_PlayBeepSE(work);
            }
            button = -1;
        } else if (button < 4 && (keys & PAD_BUTTON_L) && work->screen == 2 && work->noMoveInfo == 1) {
            button = -1;
        } else if (button < 4 && (keys & PAD_BUTTON_L) && work->screen == 2 && !work->moveExists[button]) {
            button = -1;
        } else if (touched != -1) {
            if (!BtlvInput_IsCancelButton(work, set, button) && !work->selectionLocked) {
                BtlvInput_PlayDecideSE(work);
                switch (work->screen) {
                case 1:
                case 2:
                case 5:
                    work->lastCursor[work->pokeIndex][work->screen] = button;
                    break;
                case 3: {
                    u32 index = 0;
                    u8 count;
                    const BtlvInputKeyStop *targetStops;
                    int i;

                    if (work->rule != 2) {
                        count = data_ov169_0689e218[work->pokeIndex][work->pos];
                    } else {
                        count = data_ov169_0689e238[work->pokeIndex][work->pos];
                    }
                    if (work->rule != 2) {
                        targetStops = data_ov169_0689e6e0[work->pokeIndex][work->pos];
                    } else {
                        targetStops = data_ov169_0689e884[work->pokeIndex][work->pos];
                    }
                    for (i = 0; i < count; i++) {
                        if (button == targetStops[i].a) {
                            index = i;
                            break;
                        }
                    }
                    work->lastCursor[work->pokeIndex][work->screen] =
                        (work->lastCursor[work->pokeIndex][2] << 4) | index;
                    break;
                }
                }
            } else {
                BtlvInput_PlayCancelSE(work);
            }
        }
    }
    if (button != -1) {
        button = BtlvInput_StartPressFlash(work, button, set->flashRows[button]);
    }
    return button;
}

// Runs the finger cursor's demonstration of the move buttons; returns TRUE when it has ended
BOOL BtlvInput_FingerDemoMain(BtlvInput *work) {
    BOOL done = FALSE;

    switch (work->fingerSeq) {
    case 0: {
        const s32 pos[3][2] = { { 0x80, 0x48 }, { 0x40, 0x30 }, { 0x28, 0x98 } };

        if (work->fingerCursor == NULL) {
            work->fingerCursor = BtlvFingerCursor_Create(work->paletteFade, 11, work->heapId);
        }
        if (BtlvFingerCursor_Start(work->fingerCursor, pos[work->fingerCount][0], pos[work->fingerCount][1], 2, 7,
                                   16)) {
            work->fingerSeq++;
            work->fingerCount++;
        }
        break;
    }
    case 1:
        done = BtlvFingerCursor_IsTouched(work->fingerCursor);
        if (done == TRUE) {
            BtlvInput_PlayDecideSE(work);
        }
        break;
    }
    return done;
}

// Reads the move screen's input; returns TRUE when a button was chosen, with the slot and the button
BOOL BtlvInput_MoveScreenMain(BtlvInput *work, u8 *outSlot, s32 *outButton) {
    s32 button;
    s32 touch;
    BOOL transformed;
    u32 held;
    s32 result;

    if (work->busy || PaletteFade_GetActiveMask(work->paletteFade) != 0 || work->inputLocked) {
        return FALSE;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        BtlvEffect_SetGaugeFlag();
    }
    if (work->pressedButton != -1) {
        button = work->pressedButton & ~0x8000;
        if (button < 5) {
            if (button < 4 && work->noUsableMove == TRUE) {
                work->pressedButton = 7;
            }
            *outButton = work->pressedButton;
            *outSlot = data_ov168_021f35cc[work->pokeSlot];
            work->pressedButton = -1;
            work->selectionLocked = 0;
            PaletteFade_RestartFade(work->paletteFade, 2, 0x3e00, 0, 0, 0, 0, work->tcbMgr);
            PaletteFade_RestartFade(work->paletteFade, 8, 0x700, 0, 0, 0, 0, work->tcbMgr);
            return TRUE;
        }
        work->cursor = 0;
        work->lastCursor[work->pokeIndex][work->screen] = 0;
        work->selectionLocked = 0;
        BtlvInput_PlayRotationSE(work);
        BtlvInput_StartRotationSwitch(work, button);
        work->pressedButton = -1;
        return FALSE;
    }

    if (work->noUsableMove == TRUE) {
        touch = func_0203da0c(data_ov168_021f3604.rects);
        transformed = TransformCheck(work->mons[work->pokeSlot]);
        button = BtlvInput_KeyInput(work, &data_ov168_021f3604, data_ov168_021f41a4[work->pokeSlot], NULL, touch,
                                    transformed);
    } else {
        touch = func_0203da0c(data_ov168_021f3610.rects);
        transformed = TransformCheck(work->mons[work->pokeSlot]);
        button = BtlvInput_KeyInput(work, &data_ov168_021f3610, data_ov168_021f41bc[work->pokeSlot],
                                    data_ov168_021f35cf, touch, transformed);
    }
    if (button != -1) {
        u8 enabled;

        held = GCTX_HIDGetHeldKeys();
        enabled = work->buttonEnabled[button];
        if (enabled == 0 && !(held & PAD_BUTTON_L)) {
            if (work->moveExists[button] != 0) {
                BtlvInput_PlayBeepSE(work);
            }
            button = -1;
        } else if (button > 4 && enabled == 0) {
            button = -1;
        } else if (button < 4 && (held & PAD_BUTTON_L) &&
                   (TransformCheck(work->mons[work->pokeSlot]) == TRUE || work->noUsableMove == TRUE)) {
            button = -1;
        } else if (button < 4 && (held & PAD_BUTTON_L) && work->moveExists[button] == 0) {
            button = -1;
        }
    }
    if (button != -1) {
        if (work->noUsableMove == TRUE) {
            result = BtlvInput_IsCancelButton(work, &data_ov168_021f3604, button);
            if (result == 0 && !work->selectionLocked) {
                work->lastCursor[work->pokeIndex][work->screen] = button;
                BtlvInput_PlayDecideSE2(work);
            } else if (result == 1 && !work->selectionLocked) {
                BtlvInput_PlayCancelSE(work);
            }
            button = BtlvInput_StartPressFlash(work, button, data_ov168_021f3604.flashRows[button]);
        } else {
            result = BtlvInput_IsCancelButton(work, &data_ov168_021f3610, button);
            if (result == 0 && !work->selectionLocked) {
                work->lastCursor[work->pokeIndex][work->screen] = button;
                BtlvInput_PlayDecideSE2(work);
            } else if (result == 1 && !work->selectionLocked) {
                BtlvInput_PlayCancelSE(work);
            }
            button = BtlvInput_StartPressFlash(work, button, data_ov168_021f3610.flashRows[button]);
        }
    }
    if (button != -1) {
        return TRUE;
    }
    return FALSE;
}

// Whether a screen change, a palette fade or a lock blocks the input
BOOL BtlvInput_IsBusy(BtlvInput *work) {
    if (work->busy || PaletteFade_GetActiveMask(work->paletteFade) != 0 || work->inputLocked) {
        return TRUE;
    }
    return FALSE;
}

u32 BtlvInput_GetScreen(BtlvInput *work) {
    return work->screen;
}

// Archive 11's second set of the lower screen's files follows the first
static u32 BtlvInput_FileId(u32 fileId, BtlvInput *work) {
    if (work->fileSet != 0) {
        fileId += 0x86;
    }
    return fileId;
}

// Loads the lower screen's characters, maps and palettes, and the button and move type actors' resources
static void BtlvInput_LoadGraphics(BtlvInput *work) {
    ArcTool *arc;
    u32 palFile;
    s32 i;

    if (work->fileSet != 2) {
        palFile = BtlvInput_FileId(BTLV_INPUT_BG_PALETTE_FILE, work);
        GFL_BGSysLoadArcNCGRStatic(work->arc, BtlvInput_FileId(0x166, work), 4, 0, 0, FALSE, work->heapId);
        GFL_BGSysLoadArcNCGRStatic(work->arc, BtlvInput_FileId(0x17d, work), 7, 0, 0x8000, FALSE, work->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(work->arc, BtlvInput_FileId(0x17e, work), 7, 0, 0, FALSE, work->heapId);
        PaletteFade_LoadArcNCLR(work->paletteFade, work->arc, palFile, work->heapId, 1, 0x1e0, 0);
        if (work->inBattle == TRUE) {
            sys_memcpy16(PaletteFade_GetUnfadedBuffer(BtlvEffect_GetPaletteFade(), 1) + 0x20, work->savedPalette, 0x20);
        }
    }
    work->chars = func_0204b81c(work->arc, 0x1a4, FALSE, 1, work->heapId);
    work->cellAnims = func_0204bde0(work->arc, 0x1a6, 0x1a7, work->heapId);
    work->palette = func_0204bba0(work->arc, 0x1a5, 1, 0, work->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, 3, func_0204bdc0(work->palette, TRUE) / 2, 0x100);
    work->chars2 = func_0204b81c(work->arc, 0x1a8, FALSE, 1, work->heapId);
    work->cellAnims2 = func_0204bde0(work->arc, 0x1a9, 0x1aa, work->heapId);

    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_TAIL(work->heapId));
    work->moveCellAnims = func_0204bde0(arc, func_0202d7f8(0), func_0202d7fc(0), work->heapId);
    work->movePalette = func_0204bba0(arc, func_0202d7e4(), 1, 0x100, work->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, 3, func_0204bdc0(work->movePalette, TRUE) / 2, 0x60);
    for (i = 0; i < 4; i++) {
        work->moveChars[i] = func_0204b81c(arc, func_0202d7f4(0), FALSE, 1, work->heapId);
    }
    GFL_ArcToolFree(arc);
}

// Standby to command: the ball opens while the buttons rise
static void BtlvInput_StandbyToCommandTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *param = data;

    switch (param->seq) {
    case 0:
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, param->mapFile, 4, 0, 0, FALSE, param->work->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x174, param->work), 5, 0, 0, FALSE,
                                               param->work->heapId);
        BtlvInput_PlayOpenSE(param->work);
        BtlvInput_StartSetLayers(param->work, 5, 0, 0, 1, 0, 2, 1);
        BtlvInput_StartBallScale(param->work, 0x3000, 0x1000, -0x400, 0x98);
        BtlvInput_StartButtonRise(param->work, 0, 0x1c0, 8, 8);
        BtlvInput_LoadDarkPalette(param->work);
        BmpWin_FlushMap(param->work->subWin);
        GFL_BGSysLoadScr(6);
        PaletteFade_RestartFade(param->work->paletteFade, 2, 3, 1, 12, 0, 0x842, param->work->tcbMgr);
        param->seq++;
        break;
    case 1:
    default:
        if (param->work->subTaskCount == 0) {
            BmpWin_FlushChar(param->work->subWin);
            GFL_BGSysSetBGEnabled(4, 1);
            GFL_BGSysSetBGEnabled(5, 1);
            GFL_BGSysSetBGEnabled(7, 0);
            BtlvInput_EndTask(param->work, tcb);
        }
        break;
    }
}

// Standby to moves: the ball opens, then the command to moves sequence runs
static void BtlvInput_StandbyToMovesTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *param = data;

    switch (param->seq) {
    case 0:
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x169, param->work), 4, 0, 0, FALSE,
                                               param->work->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x17b, param->work), 5, 0, 0, FALSE,
                                               param->work->heapId);
        BtlvInput_PlayOpenSE(param->work);
        BtlvInput_StartSetLayers(param->work, 5, 0, 0, 0, 0, 2, 1);
        BtlvInput_StartBallScale(param->work, 0x3000, 0x1000, -0x400, 0x98);
        GFL_BGSysLoadScr(6);
        PaletteFade_RestartFade(param->work->paletteFade, 2, 3, 1, 12, 0, 0x842, param->work->tcbMgr);
        param->seq++;
        break;
    case 1:
        if (param->work->subTaskCount != 0) {
            break;
        }
        if (param->work->rule == 2 && param->pos != 4) {
            loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x16a, param->work), 4, 0, 0,
                                                   FALSE, param->work->heapId);
        } else {
            loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x169, param->work), 4, 0, 0,
                                                   FALSE, param->work->heapId);
        }
        BmpWin_FlushMap(param->work->msgWin);
        GFL_BGSysLoadScr(6);
        BtlvInput_StartButtonRise(param->work, 0x100, 0x1c0, 8, 8);
        BtlvInput_StartFieldFrames(param->work, 0, 0);
        BtlvInput_StartActorSlide(param->work, 0);
        BtlvInput_StartSetLayers(param->work, -1, 0, 0, 1, 1, 2, 0);
        param->wait = 4;
        param->seq++;
        break;
    case 2:
        param->wait--;
        if (param->wait == 0) {
            BtlvInput_StartTileAnim(param->work, 0, 0);
            param->seq++;
        }
        break;
    case 3:
    default:
        if (param->work->subTaskCount == 0) {
            BmpWin_FlushChar(param->work->msgWin);
            func_0204bfd4(param->work->moveUnit, TRUE);
            BtlvInput_StartSetLayers(param->work, 5, 0x100, 0xc0, 2, 2, 2, 2);
            BtlvInput_EndTask(param->work, tcb);
        }
        break;
    }
}

// Command to moves: the cancel button rises, the field frames and the actors slide in, then the tiles grow
static void BtlvInput_CommandToMovesTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *param = data;

    switch (param->seq) {
    case 0:
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, param->mapFile, 4, 0, 0, FALSE, param->work->heapId);
        BmpWin_FlushMap(param->work->msgWin);
        GFL_BGSysLoadScr(6);
        BtlvInput_StartButtonRise(param->work, 0x100, 0x1c0, 8, 8);
        BtlvInput_StartFieldFrames(param->work, 0, 0);
        BtlvInput_StartActorSlide(param->work, 0);
        BtlvInput_RestoreSavedPalette(param->work);
        BtlvInput_StartSetLayers(param->work, -1, 0, 0, 1, 1, 2, 0);
        param->wait = 4;
        param->seq++;
        break;
    case 1:
        param->wait--;
        if (param->wait == 0) {
            BtlvInput_StartTileAnim(param->work, 0, 0);
            param->seq++;
        }
        break;
    case 2:
    default:
        if (param->work->subTaskCount == 0) {
            BmpWin_FlushChar(param->work->msgWin);
            func_0204bfd4(param->work->moveUnit, TRUE);
            BtlvInput_StartSetLayers(param->work, 5, 0x100, 0xc0, 2, 2, 2, 2);
            BtlvInput_EndTask(param->work, tcb);
        }
        break;
    }
}

// Moves to command: the tiles shrink, then the buttons rise while the field frames and the actors slide back
static void BtlvInput_MovesToCommandTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *param = data;

    switch (param->seq) {
    case 0:
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, param->mapFile, 4, 0, 0, FALSE, param->work->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x174, param->work), 5, 0, 0, FALSE,
                                               param->work->heapId);
        BmpWin_FlushMap(param->work->subWin);
        BtlvInput_LoadDarkPalette(param->work);
        GFL_BGSysLoadScr(6);
        BtlvInput_StartSetLayers(param->work, 5, 0, 0xc0, 1, 1, 2, 0);
        param->seq++;
        break;
    case 1:
        BtlvInput_StartTileAnim(param->work, 0, 1);
        param->wait = 4;
        param->seq++;
        break;
    case 2:
        param->wait--;
        if (param->wait == 0) {
            BtlvInput_StartButtonRise(param->work, 0, 0x1c0, 8, 8);
            BtlvInput_StartFieldFrames(param->work, 0, 1);
            BtlvInput_StartActorSlide(param->work, 1);
            param->seq++;
        }
        break;
    case 3:
    default:
        if (param->work->subTaskCount == 0) {
            BtlvInput_SetPartyIconsActive(param->work, TRUE);
            if (param->work->iconActors[0].actor != NULL) {
                gfxRegSetBrightnessBlend(0x4001050, 0x10, -8);
            }
            BmpWin_FlushChar(param->work->subWin);
            BtlvInput_EndTask(param->work, tcb);
        }
        break;
    }
}

// To target: the target panel is copied over the right-hand screen of the move map
static void BtlvInput_ToTargetTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *param = data;

    loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x169, param->work), 4, 0, 0, FALSE,
                                           param->work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, param->mapFile, 4, 0x440, 0x380, FALSE,
                                           param->work->heapId);
    BmpWin_FlushMap(param->work->msgWin);
    GFL_BGSysLoadScr(6);
    if (param->work->rule == 2) {
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x176, param->work), 5, 0, 0, FALSE,
                                               param->work->heapId);
    } else {
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x175, param->work), 5, 0, 0, FALSE,
                                               param->work->heapId);
    }
    BmpWin_FlushChar(param->work->msgWin);
    BtlvInput_EndTask(param->work, tcb);
}

// Target to moves
static void BtlvInput_TargetToMovesTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *param = data;

    switch (param->seq) {
    case 0:
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, param->mapFile, 4, 0, 0, FALSE, param->work->heapId);
        BtlvInput_StartSetLayers(param->work, 5, 0, 0xc0, 2, 2, 2, 2);
        param->seq++;
        break;
    case 1:
        BmpWin_FlushMap(param->work->msgWin);
        GFL_BGSysLoadScr(6);
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x174, param->work), 5, 0, 0, FALSE,
                                               param->work->heapId);
        BmpWin_FlushChar(param->work->msgWin);
        func_0204bfd4(param->work->moveUnit, TRUE);
        BtlvInput_StartSetLayers(param->work, 5, 0x100, 0xc0, 2, 2, 2, 2);
        BtlvInput_EndTask(param->work, tcb);
        break;
    }
}

// Command to standby: the ball closes over the buttons
static void BtlvInput_CommandToStandbyTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *param = data;

    switch (param->seq) {
    case 0:
        BtlvInput_StartBallScale(param->work, 0x1000, 0x3000, 0x400, 0xa8);
        param->seq++;
        break;
    case 1:
        GFL_BGSysSetBGEnabled(4, 0);
        GFL_BGSysSetBGEnabled(5, 0);
        GFL_BGSysSetBGEnabled(7, 1);
        param->seq++;
        break;
    case 2:
    default:
        if (param->work->subTaskCount == 0) {
            BtlvInput_EndTask(param->work, tcb);
        }
        break;
    }
}

// Another screen to standby: the tiles shrink and the field frames go back, then the ball closes
static void BtlvInput_OtherToStandbyTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *param = data;

    switch (param->seq) {
    case 0:
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x169, param->work), 4, 0, 0, FALSE,
                                               param->work->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x17a, param->work), 5, 0, 0, FALSE,
                                               param->work->heapId);
        BtlvInput_StartSetLayers(param->work, 5, 0, 0xc0, 1, 1, 2, 0);
        param->seq++;
        break;
    case 1:
        if (param->prevScreen == 2 || param->prevScreen == 5) {
            BtlvInput_StartTileAnim(param->work, 0, 1);
        }
        param->seq++;
        param->wait = 4;
        break;
    case 2:
        param->wait--;
        if (param->wait == 0) {
            BtlvInput_StartFieldFrames(param->work, 0, 1);
            param->seq++;
        }
        break;
    case 3:
        if (param->work->subTaskCount == 0) {
            BtlvInput_StartBallScale(param->work, 0x1000, 0x3000, 0x400, 0xa8);
            param->seq++;
        }
        break;
    case 4:
        GFL_BGSysSetBGEnabled(4, 0);
        GFL_BGSysSetBGEnabled(5, 0);
        GFL_BGSysSetBGEnabled(7, 1);
        param->seq++;
        break;
    case 5:
    default:
        if (param->work->subTaskCount == 0) {
            BtlvInput_EndTask(param->work, tcb);
        }
        break;
    }
}

// Yes or no: the ball opens on the question
static void BtlvInput_YesNoTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *param = data;

    switch (param->seq) {
    case 0:
        BtlvInput_PlayOpenSE(param->work);
        BtlvInput_StartSetLayers(param->work, 5, 0, 0, 0, 0, 2, 1);
        BtlvInput_StartBallScale(param->work, 0x3000, 0x1000, -0x400, 0x98);
        BtlvInput_RestoreSavedPalette(param->work);
        PaletteFade_StartFade(param->work->paletteFade, 2, 3, 1, 12, 0, 0x842, param->work->tcbMgr);
        param->seq++;
        break;
    case 1:
        if (param->work->subTaskCount == 0) {
            BtlvInput_StartFieldFrames(param->work, 0, 0);
            GFL_BGSysSetBGEnabled(5, 1);
            GFL_BGSysSetBGEnabled(7, 0);
            param->seq++;
        }
        break;
    case 2:
    default:
        if (param->work->subTaskCount == 0) {
            BmpWin_FlushMap(param->work->msgWin);
            GFL_BGSysLoadScr(6);
            BmpWin_FlushChar(param->work->msgWin);
            BtlvInput_StartSetLayers(param->work, 5, 0x100, 0xc0, 2, 2, 2, 2);
            BtlvInput_EndTask(param->work, tcb);
        }
        break;
    }
}

// Standby to the second move layout: the ball opens, then the buttons rise from the slot's position
static void BtlvInput_StandbyToMovesSlotTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *param = data;

    switch (param->seq) {
    case 0:
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x169, param->work), 4, 0, 0, FALSE,
                                               param->work->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x17b, param->work), 5, 0, 0, FALSE,
                                               param->work->heapId);
        BtlvInput_PlayOpenSE(param->work);
        BtlvInput_StartSetLayers(param->work, 5, 0, 0, 0, 0, 2, 1);
        BtlvInput_StartBallScale(param->work, 0x3000, 0x1000, -0x400, 0x98);
        GFL_BGSysLoadScr(6);
        PaletteFade_RestartFade(param->work->paletteFade, 2, 3, 1, 12, 0, 0x842, param->work->tcbMgr);
        param->seq++;
        break;
    case 1:
        if (param->work->subTaskCount != 0) {
            break;
        }
        loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x170, param->work), 4, 0, 0, FALSE,
                                               param->work->heapId);
        BmpWin_FlushMap(param->work->msgWin);
        GFL_BGSysLoadScr(6);
        BtlvInput_StartButtonRise(param->work, data_ov168_021f3740[param->work->pokeSlot].x,
                                  data_ov168_021f3740[param->work->pokeSlot].y, 8, 8);
        BtlvInput_StartFieldFrames(param->work, 0, 0);
        BtlvInput_StartActorSlide(param->work, 0);
        BtlvInput_StartSetLayers(param->work, -1, 0, 0, 1, 1, 2, 0);
        param->wait = 4;
        param->seq++;
        break;
    case 2:
        param->wait--;
        if (param->wait == 0) {
            BtlvInput_StartTileAnim(param->work, 0, 0);
            param->seq++;
        }
        break;
    case 3:
    default:
        if (param->work->subTaskCount == 0) {
            BmpWin_FlushChar(param->work->msgWin);
            func_0204bfd4(param->work->moveUnit, TRUE);
            BtlvInput_StartSetLayers(param->work, 5, 0x100, 0xc0, 2, 2, 2, 2);
            BtlvInput_EndTask(param->work, tcb);
        }
        break;
    }
}

// Standby to a message screen: the ball opens while the buttons rise, then the text is drawn
static void BtlvInput_StandbyToMessageTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *param = data;
    BOOL studioMode2 = FALSE;

    if (func_ov167_0219c988(BtlvEffect_GetMainModule()) == 2) {
        studioMode2 = TRUE;
    }
    switch (param->seq) {
    case 0:
        if (!studioMode2) {
            loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x16e, param->work), 4, 0, 0,
                                                   FALSE, param->work->heapId);
            loadBGScrToVramByFileNoReserveNegAlign(param->work->arc, BtlvInput_FileId(0x179, param->work), 5, 0, 0,
                                                   FALSE, param->work->heapId);
            BtlvInput_PlayOpenSE(param->work);
            BtlvInput_StartSetLayers(param->work, 5, 0, 0, 1, 0, 2, 1);
            BtlvInput_StartBallScale(param->work, 0x3000, 0x1000, -0x400, 0x98);
            BtlvInput_StartButtonRise(param->work, 0, 0x1c0, 8, 8);
            PaletteFade_StartFade(param->work->paletteFade, 2, 3, 1, 12, 0, 0x842, param->work->tcbMgr);
        }
        param->seq++;
        break;
    case 1:
    default:
        if (param->work->subTaskCount == 0) {
            if (param->strbuf != NULL) {
                GFL_TextRendererDrawToBitmap(param->work->msgBitmap, 0x18, 0x38, param->strbuf, param->work->font);
                GFL_StrBufFree(param->strbuf);
            }
            BmpWin_FlushMap(param->work->msgWin);
            GFL_BGSysLoadScr(6);
            BmpWin_FlushChar(param->work->msgWin);
            if (!studioMode2) {
                BtlvInput_StartSetLayers(param->work, 4, param->x, param->y, 1, 1, 2, 0);
            }
            BtlvInput_EndTask(param->work, tcb);
            func_ov167_0219c988(BtlvEffect_GetMainModule());
        }
        break;
    }
}

// Loads the type icons' actors
static void BtlvInput_LoadTypeIcons(BtlvInput *work) {
    NNSG2dCharacterData *character;
    void *buf;
    ClActorSetup s;
    ClActorSetup setup;
    int i;

    work->typeCellAnims = func_0204bde0(work->arc, 0x238, 0x239, work->heapId);
    work->typePalette = func_0204bbb8(work->arc, 0x23a, 1, 0, 0, 4, work->heapId);
    buf = GFL_G2DIOReadOBJNCGRArc(work->arc, 0x237, FALSE, &character, work->heapId);
    func_0204ba40(work->chars, character);
    GFL_HeapFree(buf);
    PaletteFade_LoadFromVRAM(work->paletteFade, 3, 0, 0x1e0);

    setup = data_ov168_021f35fa;
    for (i = 0; i < BTLV_INPUT_TYPE_COUNT; i++) {
        s = setup;
        s.x = data_ov168_021f3a70[i].x;
        s.y = data_ov168_021f3a70[i].y;
        s.priority = 0x80 - i;
        work->typeActors[i] =
            func_0204c040(work->typeUnit, work->chars, work->typePalette, work->typeCellAnims, &s, 1, work->heapId);
        func_0204c520(work->typeActors[i], FALSE);
        func_0204c124(work->typeActors[i], TRUE);
        func_0204c378(work->typeActors[i], data_ov168_021f3a70[i].palette, 1);
    }
    work->typeLoaded = TRUE;
    BtlvInput_LoadTypePalettes(work);
}

// Frees them
static void BtlvInput_FreeTypeIcons(BtlvInput *work) {
    int i;

    for (i = 0; i < BTLV_INPUT_TYPE_COUNT; i++) {
        if (work->typeActors[i] != NULL) {
            func_0204c108(work->typeActors[i]);
            work->typeActors[i] = NULL;
        }
    }
    func_0204be64(work->typeCellAnims);
    func_0204bcd0(work->typePalette);
}

static BOOL BtlvInput_TestTypeBit(u32 mask, int bit) {
    return (mask >> bit) & 1;
}

static void BtlvInput_SetTypeBit(u32 *mask, int bit) {
    *mask |= 1 << bit;
}

// Hides the type icons the mask leaves out, after setting every type's bit when it is empty
static void BtlvInput_ApplyTypeMask(BtlvInput *work) {
    int i;

    if (work->typeMask == 0) {
        for (i = 0; i < work->typeCount; i++) {
            BtlvInput_SetTypeBit(&work->typeMask, i);
        }
    }
    for (i = 0; i < BTLV_INPUT_TYPE_COUNT; i++) {
        if (!BtlvInput_TestTypeBit(work->typeMask, i)) {
            BtlvInput_SetTypeIconVisible(work, FALSE, i, 0);
        }
    }
}

// Shows the type icon screen
static void BtlvInput_ShowTypeScreen(BtlvInput *work) {
    if (work->fileSet != 2 || work->typeLoaded == TRUE) {
        return;
    }
    BtlvInput_LoadTypeIcons(work);
    BtlvInput_ApplyTypeMask(work);
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysSetBGEnabled(7, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysLoadArcNCGRStatic(work->arc, 0x231, 5, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(work->arc, 0x235, 5, 0, 0, FALSE, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(work->arc, 0x230, 4, 0, 0xa0, work->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 7, 4, 0x1a0, 0x20, work->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, 1, 0, 0x1e0);
    BtlvInput_StartSetLayers(work, -1, 0, 0, FALSE, TRUE, TRUE, FALSE);
}

// Shows or hides one type icon, with its animation
void BtlvInput_SetTypeIconVisible(BtlvInput *work, BOOL visible, int index, int sequence) {
    func_0204c124(work->typeActors[index], visible);
    if (visible) {
        func_0204c488(work->typeActors[index], sequence);
        func_0204c520(work->typeActors[index], TRUE);
        func_0204c378(work->typeActors[index], data_ov168_021f3a70[index].palette, 1);
    }
}

void BtlvInput_StartTypeIconAnim(BtlvInput *work, int index) {
    func_0204c520(work->typeActors[index], TRUE);
}

u32 BtlvInput_GetTypeMask(BtlvInput *work) {
    return work->typeMask;
}

// Keeps copies of the type icons' palettes for their animation
static void BtlvInput_LoadTypePalettes(BtlvInput *work) {
    NNSG2dPaletteData *palette;
    NNSG2dPaletteData *palette2;
    void *buf;

    work->typePalBuf = GFL_HeapAllocate(work->heapId, 0x180, FALSE, "btlv_input.c", 3411);
    sys_memset(work->typePalBuf, 0, 0x180);
    work->typePalBuf2 = GFL_HeapAllocate(work->heapId, 0x1e0, FALSE, "btlv_input.c", 3414);
    sys_memset(work->typePalBuf2, 0, 0x1e0);

    buf = GFL_G2DIOReadNCLRArc(work->arc, 0x23a, &palette, work->heapId);
    sys_memcpy(palette->rawData, work->typePalBuf, 0x180);
    GFL_HeapFree(buf);
    buf = GFL_G2DIOReadNCLRArc(work->arc, 0x230, &palette2, work->heapId);
    sys_memcpy(palette2->rawData, work->typePalBuf2, 0x1e0);
    GFL_HeapFree(buf);
    work->typePalLoaded = TRUE;
}

// Loads one of the copied palettes into the fade buffers
static void BtlvInput_LoadTypePalette(const BtlvInput *work, int palette) {
    PaletteFade_LoadData(work->paletteFade, work->typePalBuf + palette * 0x80, 3, 0, 0x80);
    PaletteFade_LoadData(work->paletteFade, work->typePalBuf2 + palette * 0xa0, 1, 0, 0xa0);
}

// Animates the type icons' palettes: every 5 frames the next palette of the current step, a new random step when
// it has repeated enough
static void BtlvInput_UpdateTypePaletteAnim(BtlvInput *work) {
    int index;

    if (work->fileSet != 2 || !work->typePalLoaded) {
        return;
    }
    work->typeAnimFrame++;
    if (work->typeAnimFrame <= 4) {
        return;
    }
    work->typeAnimFrame = 0;
    work->typeAnimCounter++;
    if (work->typeAnimCounter >= data_ov168_021f361c[work->typeAnimIndex].frames) {
        work->typeAnimCounter = 0;
        work->typeAnimRepeat--;
        if (work->typeAnimRepeat <= 0) {
            index = BtlvInput_PickTypeAnimIndex(work, work->typeAnimIndex);
            BtlvInput_SetTypeAnimIndex(work, index);
            work->typeAnimRepeat = BtlvInput_Random(work, data_ov168_021f361c[work->typeAnimIndex].repeatMax) + 1;
        }
    }
    BtlvInput_LoadTypePalette(work, data_ov168_021f41b0[work->typeAnimIndex][work->typeAnimCounter]);
}

static void BtlvInput_SetTypeAnimIndex(BtlvInput *work, int index) {
    work->typeAnimIndex = index;
    work->typeAnimFrame = 0;
    work->typeAnimCounter = 0;
}

// A random step other than the current one, the next one when ten tries fail
static int BtlvInput_PickTypeAnimIndex(BtlvInput *work, int current) {
    BOOL found = FALSE;
    int i;
    int index;

    for (i = 0; i < 10; i++) {
        index = BtlvInput_Random(work, 3);
        if (index != current) {
            found = TRUE;
            break;
        }
    }
    if (!found) {
        index = current + 1;
        if (index > 2) {
            index = 0;
        }
    }
    return index;
}

static u32 BtlvInput_Random(BtlvInput *work, u32 max) {
    return GFL_RandomLC(max);
}

// The screen task that opens the ball onto the recorder's buttons
static void BtlvInput_RecorderOpenTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *task = data;

    switch (task->seq) {
    case 0:
        loadBGScrToVramByFileNoReserveNegAlign(task->work->arc, BtlvInput_FileId(0x16f, task->work), 4, 0, 0, FALSE,
                                               task->work->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(task->work->arc, BtlvInput_FileId(0x177, task->work), 5, 0, 0, FALSE,
                                               task->work->heapId);
        BtlvInput_PlayOpenSE(task->work);
        BtlvInput_StartSetLayers(task->work, 5, 0, 0, TRUE, FALSE, 2, TRUE);
        BtlvInput_StartBallScale(task->work, FX32_CONST(3), FX32_ONE, -FX32_CONST(0.25), 152);
        BtlvInput_StartButtonRise(task->work, 0, 0x1c0, 8, 8);
        PaletteFade_StartFade(task->work->paletteFade, 2, 3, 1, 12, 0, 0x842, task->work->tcbMgr);
        task->seq++;
        break;
    case 1:
    default:
        if (task->work->subTaskCount == 0) {
            GFL_BGSysSetBGEnabled(4, TRUE);
            GFL_BGSysSetBGEnabled(5, TRUE);
            GFL_BGSysSetBGEnabled(7, FALSE);
            BtlvInput_EndTask(task->work, tcb);
        }
        break;
    }
}

// The rotation screen's task: the buttons rise with the field frames and the icons slide, then the tiles grow
static void BtlvInput_RotationScreenTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *task = data;

    switch (task->seq) {
    case 0:
        if (BtlvEffect_GetState() == 1) {
            loadBGScrToVramByFileNoReserveNegAlign(task->work->arc, BtlvInput_FileId(0x23b, task->work), 4, 0, 0, FALSE,
                                                   task->work->heapId);
        } else {
            loadBGScrToVramByFileNoReserveNegAlign(task->work->arc, BtlvInput_FileId(0x170, task->work), 4, 0, 0, FALSE,
                                                   task->work->heapId);
        }
        BmpWin_FlushMap(task->work->msgWin);
        BtlvInput_RestoreSavedPalette(task->work);
        GFL_BGSysLoadScr(6);
        BtlvInput_StartButtonRise(task->work, data_ov168_021f3740[task->work->pokeSlot].x,
                                  data_ov168_021f3740[task->work->pokeSlot].y, 8, 8);
        BtlvInput_StartFieldFrames(task->work, 0, 0);
        BtlvInput_StartActorSlide(task->work, 0);
        BtlvInput_StartSetLayers(task->work, -1, 0, 0, TRUE, TRUE, 2, FALSE);
        task->wait = 4;
        task->seq++;
        break;
    case 1:
        task->wait--;
        if (task->wait == 0) {
            BtlvInput_StartTileAnim(task->work, 0, 0);
            task->seq++;
        }
        break;
    case 2:
    default:
        if (task->work->subTaskCount == 0) {
            BmpWin_FlushChar(task->work->msgWin);
            func_0204bfd4(task->work->moveUnit, TRUE);
            BtlvInput_StartSetLayers(task->work, 5, 256, 192, 2, 2, 2, 2);
            BtlvInput_EndTask(task->work, tcb);
        }
        break;
    }
}

// The rotation switch's task: the move screen is redrawn for the new slot, its buttons rising from the slot's position
static void BtlvInput_RotationSwitchTask(TCB *tcb, void *data) {
    BtlvInputScreenTask *task = data;

    switch (task->seq) {
    case 0:
        BtlvInput_ClearScreen(task->work);
        task->seq++;
        break;
    case 1:
        BtlvInput_SetupMoveScreen(task->work);
        BmpWin_FlushMap(task->work->msgWin);
        GFL_BGSysLoadScr(6);
        BtlvInput_StartButtonRise(task->work, data_ov168_021f3740[task->work->pokeSlot].x,
                                  data_ov168_021f3740[task->work->pokeSlot].y, 8, 8);
        BtlvInput_StartSetLayers(task->work, -1, 0, 0, TRUE, TRUE, 2, FALSE);
        task->seq++;
        break;
    case 2:
    default:
        if (task->work->subTaskCount == 0) {
            BmpWin_FlushChar(task->work->msgWin);
            func_0204bfd4(task->work->moveUnit, TRUE);
            BtlvInput_StartSetLayers(task->work, 5, 256, 192, 2, 2, 2, 2);
            BtlvInput_EndTask(task->work, tcb);
        }
        break;
    }
}

// The end of a screen task
static void BtlvInput_ScreenTaskEnd(TCB *tcb) {
    BtlvInputScreenTask *task = GFL_TCBGetData(tcb);
    task->work->busy = FALSE;
}

// Scales the ball picture of BG 7 from one scale to another, moving it by two pixels a frame
static void BtlvInput_StartBallScale(BtlvInput *work, fx32 from, fx32 to, fx32 step, int y) {
    BtlvInputBallScale *param =
        GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputBallScale), FALSE, "btlv_input.c", 3819);
    param->work = work;
    param->scale = from;
    param->target = to;
    param->step = step;
    param->y = y;
    BtlvInput_AddTask(work, GFL_VBlankTCBAdd(BtlvInput_BallScaleTask, param, 0), BtlvInput_BallScaleEnd);
    work->subTaskCount++;
}

static void BtlvInput_BallScaleTask(TCB *tcb, void *data) {
    BtlvInputBallScale *param = data;
    MtxFx22 mtx;

    param->scale += param->step;
    if (param->step > 0) {
        param->y -= 2;
    } else if (param->step < 0) {
        param->y += 2;
    }
    MAT2_Scaling(&mtx, param->scale, param->scale);
    GFL_BGSysSetBGTransformEx(7, BG_MOVE_SET_X, 128, &mtx, 256, 256);
    GFL_BGSysSetBGTransformEx(7, BG_MOVE_SET_Y, param->y, &mtx, 256, 256);
    if (param->scale == param->target) {
        BtlvInput_EndTask(param->work, tcb);
    }
}

static void BtlvInput_BallScaleEnd(TCB *tcb) {
    BtlvInputBallScale *param = GFL_TCBGetData(tcb);
    param->work->subTaskCount--;
}

// Scrolls BG 4 to (x, y), then by step for count frames
static void BtlvInput_StartButtonRise(BtlvInput *work, int x, int y, int step, int count) {
    BtlvInputButtonRise *param =
        GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputButtonRise), FALSE, "btlv_input.c", 3881);
    param->work = work;
    param->state = 0;
    param->x = x;
    param->y = y;
    param->step = step;
    param->count = count;
    BtlvInput_AddTask(work, GFL_VBlankTCBAdd(BtlvInput_ButtonRiseTask, param, 0), BtlvInput_ButtonRiseEnd);
    work->subTaskCount++;
}

static void BtlvInput_ButtonRiseTask(TCB *tcb, void *data) {
    BtlvInputButtonRise *param = data;

    if (param->state == 0) {
        GFL_BGSysMoveBG(4, BG_MOVE_SET_X, param->x);
        GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, param->y);
        param->state++;
        return;
    }
    param->y += param->step;
    GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, param->y);
    param->count--;
    if (param->count == 0) {
        BtlvInput_EndTask(param->work, tcb);
    }
}

static void BtlvInput_ButtonRiseEnd(TCB *tcb) {
    BtlvInputButtonRise *param = GFL_TCBGetData(tcb);
    param->work->subTaskCount--;
}

// Scrolls BG 5 through the two positions of a table, one every two frames
static void BtlvInput_StartFieldFrames(BtlvInput *work, int unused, int dir) {
    BtlvInputFieldFrames *param =
        GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputFieldFrames), FALSE, "btlv_input.c", 3940);
    param->work = work;
    param->step = 0;
    param->dir = dir;
    param->wait = 2;
    BtlvInput_AddTask(work, GFL_VBlankTCBAdd(BtlvInput_FieldFramesTask, param, 0), BtlvInput_FieldFramesEnd);
    work->subTaskCount++;
}

static void BtlvInput_FieldFramesTask(TCB *tcb, void *data) {
    BtlvInputFieldFrames *param = data;

    param->wait--;
    if (param->wait != 0) {
        return;
    }
    param->wait = 2;
    GFL_BGSysMoveBG(5, BG_MOVE_SET_X, data_ov168_021f36e0[param->dir][param->step].x);
    GFL_BGSysMoveBG(5, BG_MOVE_SET_Y, data_ov168_021f36e0[param->dir][param->step++].y);
    if (param->step >= 2) {
        BtlvInput_EndTask(param->work, tcb);
    }
}

static void BtlvInput_FieldFramesEnd(TCB *tcb) {
    BtlvInputFieldFrames *param = GFL_TCBGetData(tcb);
    param->work->subTaskCount--;
}

// Plays the grow (which 0) or shrink (1) animation on an actor at each tile of the set
static void BtlvInput_StartTileAnim(BtlvInput *work, int set, int which) {
    BtlvInputTileAnim *param =
        GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputTileAnim), FALSE, "btlv_input.c", 4007);
    ClActorSetup setup = { 0, 0, 0, 0, 2 };
    int i;

    param->work = work;
    param->unit = func_0204bf1c(6, 0, work->heapId);
    for (i = 0; i < 6; i++) {
        if (data_ov168_021f3b28[set].pos[i].x == -1) {
            param->actors[i] = NULL;
            continue;
        }
        param->actors[i] =
            func_0204c040(param->unit, work->chars, work->palette, work->cellAnims, &setup, 1, work->heapId);
        func_0204c140(param->actors[i], &data_ov168_021f3b28[set].pos[i], 1);
        func_0204c244(param->actors[i], 2);
        func_0204c520(param->actors[i], TRUE);
        func_0204c488(param->actors[i], data_ov168_021f3b28[set].sequences[which]);
    }
    BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_TileAnimTask, param, 0), BtlvInput_TileAnimEnd);
    work->subTaskCount++;
}

static void BtlvInput_TileAnimTask(TCB *tcb, void *data) {
    BtlvInputTileAnim *param = data;
    int i;

    for (i = 0; i < 6; i++) {
        if (param->actors[i] != NULL && func_0204c560(param->actors[i])) {
            return;
        }
    }
    BtlvInput_EndTask(param->work, tcb);
}

static void BtlvInput_TileAnimEnd(TCB *tcb) {
    BtlvInputTileAnim *param = GFL_TCBGetData(tcb);
    int i;

    for (i = 0; i < 6; i++) {
        if (param->actors[i] != NULL) {
            func_0204c108(param->actors[i]);
        }
    }
    func_0204bf98(param->unit);
    param->work->subTaskCount--;
}

// Slides the party icons to the row's height for the direction, four pixels a frame
static void BtlvInput_StartActorSlide(BtlvInput *work, int dir) {
    BtlvInputActorSlide *param =
        GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputActorSlide), FALSE, "btlv_input.c", 4125);
    param->work = work;
    param->dir = dir;
    BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_ActorSlideTask, param, 0),
                      BtlvInput_ActorSlideEnd);
    work->subTaskCount++;
}

static void BtlvInput_ActorSlideTask(TCB *tcb, void *data) {
    BtlvInputActorSlide *param = data;
    int row;
    BOOL moving = FALSE;
    BtlvInputActor *actors;
    int i;
    ClActorPos pos;

    for (row = 0; row < 2; row++) {
        int target = data_ov168_021f3628[row][param->dir];

        if (row == 0) {
            actors = param->work->pokeActors[0];
        } else if (param->work->secondRow) {
            actors = param->work->pokeActors[1];
        } else {
            break;
        }
        for (i = 0; i < 6; i++) {
            func_0204c178(actors[i].actor, &pos, 1);
            if (pos.y > target) {
                pos.y -= 4;
                moving = TRUE;
            } else if (pos.y < target) {
                pos.y += 4;
                moving = TRUE;
            }
            func_0204c140(actors[i].actor, &pos, 1);
        }
    }
    if (!moving) {
        BtlvInput_EndTask(param->work, tcb);
    }
}

static void BtlvInput_ActorSlideEnd(TCB *tcb) {
    BtlvInputActorSlide *param = GFL_TCBGetData(tcb);
    param->work->subTaskCount--;
}

// Next frame, scrolls a BG (none with -1) and switches BGs 4 to 7 on (TRUE), off (FALSE) or leaves them (2)
static void BtlvInput_StartSetLayers(BtlvInput *work, int bg, int x, int y, int bg4, int bg5, int bg6, int bg7) {
    BtlvInputSetLayers *param =
        GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputSetLayers), FALSE, "btlv_input.c", 4209);
    param->work = work;
    param->bg = bg;
    param->x = x;
    param->y = y;
    param->bg4 = bg4;
    param->bg5 = bg5;
    param->bg6 = bg6;
    param->bg7 = bg7;
    BtlvInput_AddTask(work, GFL_VBlankTCBAdd(BtlvInput_SetLayersTask, param, 0), NULL);
}

static void BtlvInput_SetLayersTask(TCB *tcb, void *data) {
    BtlvInputSetLayers *param = data;

    if (param->bg != -1) {
        GFL_BGSysMoveBG(param->bg, BG_MOVE_SET_X, param->x);
        GFL_BGSysMoveBG(param->bg, BG_MOVE_SET_Y, param->y);
    }
    if (param->bg4 != 2) {
        GFL_BGSysSetBGEnabled(4, param->bg4);
    }
    if (param->bg5 != 2) {
        GFL_BGSysSetBGEnabled(5, param->bg5);
    }
    if (param->bg6 != 2) {
        GFL_BGSysSetBGEnabled(6, param->bg6);
    }
    if (param->bg7 != 2) {
        GFL_BGSysSetBGEnabled(7, param->bg7);
    }
    BtlvInput_EndTask(param->work, tcb);
}

// Waits for the fades to end, freeing the graphics when the fade was one out of the screen
static void BtlvInput_FadeWaitTask(TCB *tcb, void *data) {
    BtlvInputWorkTask *param = data;

    if (PaletteFade_GetActiveMask(param->work->paletteFade) == 0 && !GFL_FadeIsRunning()) {
        if (param->work->fadeMode == 1) {
            BtlvInput_FreeGraphics(param->work);
        }
        BtlvInput_EndTask(param->work, tcb);
    }
}

static void BtlvInput_FadeWaitEnd(TCB *tcb) {
    BtlvInputWorkTask *param = GFL_TCBGetData(tcb);
    param->work->fadeMode = 0;
}

// Slides the two actors left eight pixels a frame until they reach x 224
static void BtlvInput_SlideActorsLeftTask(TCB *tcb, void *data) {
    BtlvInputWorkTask *param = data;
    ClActorPos pos;

    func_0204c178(param->work->actors210[0], &pos, 1);
    pos.x -= 8;
    func_0204c140(param->work->actors210[0], &pos, 1);
    func_0204c178(param->work->actors210[1], &pos, 1);
    pos.x -= 8;
    func_0204c140(param->work->actors210[1], &pos, 1);
    if (pos.x == 224) {
        BtlvInput_EndTask(param->work, tcb);
    }
}

static void BtlvInput_SlideActorsLeftEnd(TCB *tcb) {
    BtlvInputWorkTask *param = GFL_TCBGetData(tcb);
    param->work->subTaskCount--;
}

// Starts the rotation switch for rotate button 5 or 6, which rotates another mon to the front
static void BtlvInput_StartRotationSwitch(BtlvInput *work, int screen) {
    BtlvInputScreenTask *task =
        GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputScreenTask), TRUE, "btlv_input.c", 4320);
    int slot = work->pokeSlot;
    int index = screen - 5;

    work->pokeSlot = data_ov168_021f35d6[slot][index];
    work->busy = TRUE;
    task->work = work;
    BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_RotationSwitchTask, task, 1),
                      BtlvInput_ScreenTaskEnd);
    BtlvEffect_RestartIdleEffect(2);
    BtlvEffect_Start(data_ov168_021f3658[slot][index]);
}

// Waits for the touch screen's work and sets its state
static void BtlvInput_SetScdStateTask(TCB *tcb, void *data) {
    BtlvInputSimpleTask *param = data;

    if (BtlvEffect_GetWork()) {
        BtlvEffect_SetState(param->value);
        BtlvInput_EndTask(param->work, tcb);
    }
}

// Darkens the sub screen's OBJ outside the windows, window 1 only with three or more of them
static void BtlvInput_WindowDarkenTask(TCB *tcb, void *data) {
    BtlvInputSimpleTask *param = data;

    GXS_SetVisibleWnd(GX_WNDMASK_W0 | GX_WNDMASK_W1);
    G2S_SetWnd0InsidePlane(0x1f, param->value > 1);
    G2S_SetWnd1InsidePlane(0x1f, param->value == 3);
    G2S_SetWndOutsidePlane(0x1f, FALSE);
    gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_OBJ, -8);
    BtlvInput_EndTask(param->work, tcb);
}

// Undoes it
static void BtlvInput_WindowClearTask(TCB *tcb, void *data) {
    BtlvInputSimpleTask *param = data;

    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    G2S_SetWnd0InsidePlane(0, FALSE);
    G2S_SetWnd1InsidePlane(0, FALSE);
    G2S_SetWndOutsidePlane(0, FALSE);
    gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 0, 0);
    BtlvInput_EndTask(param->work, tcb);
}

// Clears the sub screen's blend next frame
static void BtlvInput_StartClearBlend(BtlvInput *work) {
    BtlvInputSimpleTask *param =
        GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputSimpleTask), FALSE, "btlv_input.c", 4504);
    param->work = work;
    BtlvInput_AddTask(work, GFL_VBlankTCBAdd(BtlvInput_ClearBlendTask, param, 0), NULL);
}

static void BtlvInput_ClearBlendTask(TCB *tcb, void *data) {
    BtlvInputSimpleTask *param = data;

    gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 0, 0);
    BtlvInput_EndTask(param->work, tcb);
}

// A string's width in pixels and in 8-pixel blocks
static void BtlvInput_GetStringWidth(const StrBuf *strbuf, Font *font, int *width, int *blocks) {
    int w = GFL_FontGetBlockWidth(strbuf, font, 0);
    int b = w / 8;

    if (FX_ModS32(w, 8) != 0) {
        b++;
    }
    *width = w;
    *blocks = b;
}

// Draws the four move tiles' text and type icons
static void BtlvInput_DrawMoveTiles(BtlvInput *work, const BtlvInputMoveParam *param) {
    const s32 ppLabelPos[4][2] = { { 53, 26 }, { 181, 26 }, { 53, 74 }, { 181, 74 } };
    const s32 ppPos[4][2] = { { 90, 26 }, { 218, 26 }, { 90, 74 }, { 218, 74 } };
    NNSG2dImageProxy proxy;
    s32 width;
    s32 blocks;
    NNSG2dCharacterData *character;
    NNSG2dPaletteData *paletteData;
    u8 letter;
    u8 shadow;
    u8 background;
    WordSet *wordSet;
    StrBuf *nameBuf;
    StrBuf *nameFmt;
    StrBuf *ppLabel;
    StrBuf *ppBuf;
    StrBuf *ppFmt;
    void *charBuf;
    void *palBuf;
    u32 palette;
    s32 type;
    s32 color;
    int i;

    nameBuf = GFL_StrBufCreate(16, work->heapId);
    nameFmt = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0021_Text_Empty);
    ppLabel = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0021_Text_Pp);
    wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    ppBuf = GFL_StrBufCreate(16, work->heapId);
    ppFmt = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0021_Text_Empty_2);
    func_0204bfd4(work->moveUnit, FALSE);

    for (i = 0; i < 4; i++) {
        if (param->moves[i] != MOVE_NONE) {
            HeapID heapId = HEAPID_TAIL(work->heapId);
            type = PML_MoveGetParam(param->moves[i], MOVE_PARAM_TYPE);
            palette = data_ov168_021f3864[type];
            work->moveActors[i] = func_0204c040(work->moveUnit, work->moveChars[i], work->movePalette,
                                                work->moveCellAnims, &data_ov168_021f35f2, 1, work->heapId);
            func_0204c140(work->moveActors[i], &data_ov168_021f3638[i], 1);
            charBuf = GFL_G2DIOReadOBJNCGR(getUINarcIdx(), func_0202d7f4(type), FALSE, &character, heapId);
            func_0204c378(work->moveActors[i], func_0202d7e8(type), 1);
            func_0204c40c(work->moveActors[i], &proxy);
            MI_CpuCopy16(character->rawData,
                         (void *)(HW_DB_OBJ_VRAM + proxy.vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_2DSUB]), 0x100);
            GFL_HeapFree(charBuf);

            GFL_TextRndGetGlobalColors(&letter, &shadow, &background);
            GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
            loadMoveNameToStrbuf(wordSet, 0, param->moves[i]);
            GFL_WordSetFormatStrbuf(wordSet, nameBuf, nameFmt);
            BtlvInput_GetStringWidth(nameBuf, work->font, &width, &blocks);
            GFL_TextRendererDrawToBitmap(work->msgBitmap, data_ov168_021f3720[i][0] - width / 2,
                                         data_ov168_021f3720[i][1], nameBuf, work->font);

            WordSetNumber(wordSet, 0, param->pp[i], 2, 1, TRUE);
            WordSetNumber(wordSet, 1, param->maxPP[i], 2, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, ppBuf, ppFmt);
            color = BtlvInput_GetPPColor(param->pp[i], param->maxPP[i]);
            GFL_TextRndUpdateColorIndexLUT((color >> 10) & 0x1f, (color >> 5) & 0x1f, color & 0x1f);
            GFL_TextRendererDrawToBitmap(work->msgBitmap, ppLabelPos[i][0], ppLabelPos[i][1], ppLabel, work->font);
            BtlvInput_GetStringWidth(ppBuf, work->font, &width, &blocks);
            GFL_TextRendererDrawToBitmap(work->msgBitmap, ppPos[i][0] - width / 2, ppPos[i][1], ppBuf, work->font);
            GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);

            work->buttonEnabled[i] = param->pp[i] != 0;
            work->moveExists[i] = param->moves[i] != MOVE_NONE;
        } else {
            palette = 463;
        }
        palBuf = GFL_G2DIOReadNCLR(11, palette, &paletteData, HEAPID_TAIL(work->heapId));
        PaletteFade_LoadData(work->paletteFade, paletteData->rawData, 1, i * 16 + 0x90, 0x20);
        GFL_HeapFree(palBuf);
    }
    work->buttonEnabled[i] = TRUE;
    if (work->rule == 2) {
        work->buttonEnabled[i + 1] = TRUE;
    }

    GFL_WordSetSystemFree(wordSet);
    GFL_StrBufFree(nameBuf);
    GFL_StrBufFree(ppLabel);
    GFL_StrBufFree(nameFmt);
    GFL_StrBufFree(ppFmt);
    GFL_StrBufFree(ppBuf);
}

// Draws the target panels' names, picks the screen's layout and loads the panel palettes
static void BtlvInput_DrawTargetPanels(BtlvInput *work, BtlvInputScreenTask *task, const BtlvInputTargetParam *param) {
    s32 width;
    s32 blocks;
    NNSG2dPaletteData *paletteData;
    u8 letter;
    u8 shadow;
    u8 background;
    StrBuf *buf;
    StrBuf *fmt;
    WordSet *wordSet;
    void *palBuf;
    int count;
    BOOL triple;
    int attacker;
    int i;

    count = (work->rule == 2) ? 6 : 4;
    triple = (work->rule == 2) ? TRUE : FALSE;
    GFL_TextRndGetGlobalColors(&letter, &shadow, &background);
    buf = GFL_StrBufCreate(12, HEAPID_TAIL(work->heapId));
    wordSet = GFL_WordSetSystemCreateDefault(HEAPID_TAIL(work->heapId));
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
    for (i = 0; i < count; i++) {
        if (param->mons[i].species != 0) {
            fmt = GFL_MsgDataLoadStrbufNew(work->msgData, param->mons[i].msgForm + 3);
            loadPokemonNicknameToStrbuf(wordSet, 0, param->mons[i].pkm);
            GFL_WordSetFormatStrbuf(wordSet, buf, fmt);
            BtlvInput_GetStringWidth(buf, work->font, &width, &blocks);
            GFL_TextRendererDrawToBitmap(work->msgBitmap, data_ov168_021f3ac8[triple][i][0] - width / 2,
                                         data_ov168_021f3ac8[triple][i][1], buf, work->font);
            GFL_StrBufFree(fmt);
        }
        work->buttonEnabled[i] = param->mons[i].selectable;
    }
    work->buttonEnabled[i] = TRUE;
    GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);
    GFL_WordSetSystemFree(wordSet);
    GFL_StrBufFree(buf);

    attacker = (param->attackerPos - 2) / 2;
    work->pos = param->range;
    task->mapFile = BtlvInput_FileId(data_ov168_021f3c18[triple][attacker][param->range], work);

    count = (work->rule == 2) ? 3 : 2;
    for (i = 0; i < count; i++) {
        palBuf = GFL_G2DIOReadNCLR(11, 481, &paletteData, HEAPID_TAIL(work->heapId));
        PaletteFade_LoadData(work->paletteFade, paletteData->rawData, 1, i * 16 + 0x70, 0x20);
        GFL_HeapFree(palBuf);
    }
    for (; i < count * 2; i++) {
        palBuf = GFL_G2DIOReadNCLR(11, 482, &paletteData, HEAPID_TAIL(work->heapId));
        PaletteFade_LoadData(work->paletteFade, paletteData->rawData, 1, i * 16 + 0x70, 0x20);
        GFL_HeapFree(palBuf);
    }
}

// Draws the two strings of the Yes / No screen and loads its BG screen
static void BtlvInput_DrawYesNo(BtlvInput *work, const BtlvInputYesNoParam *param) {
    s32 width;
    s32 blocks;

    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
    BtlvInput_GetStringWidth(param->strs[0], work->font, &width, &blocks);
    GFL_TextRendererDrawToBitmap(work->msgBitmap, 128 - width / 2, 16, param->strs[0], work->font);
    BtlvInput_GetStringWidth(param->strs[1], work->font, &width, &blocks);
    GFL_TextRendererDrawToBitmap(work->msgBitmap, 128 - width / 2, 64, param->strs[1], work->font);
    work->buttonEnabled[0] = TRUE;
    work->buttonEnabled[1] = TRUE;
    loadBGScrToVramByFileNoReserveNegAlign(work->arc, BtlvInput_FileId(375, work), 5, 0, 0, FALSE, work->heapId);
}

// Draws the move screen for the current mon and starts its fade
static void BtlvInput_SetupMoveScreen(BtlvInput *work) {
    BtlvInputMoveParam param;
    BattleMon *mon;
    int i;

    for (i = 0; i < 4; i++) {
        param.moves[i] = 0;
        param.pp[i] = 0;
        param.maxPP[i] = 0;
    }
    for (i = 0; i < GetBattleMonMoveCount(work->mons[work->pokeSlot]); i++) {
        param.moves[i] = func_ov167_021bad28(work->mons[work->pokeSlot], i, &param.pp[i], &param.maxPP[i]);
    }
    BtlvInput_DrawMoveTiles(work, &param);
    work->buttonEnabled[5] = data_ov168_021f35dc[work->pokeSlot][0];
    work->buttonEnabled[6] = data_ov168_021f35dc[work->pokeSlot][1];
    work->noUsableMove = TRUE;
    for (i = 0; i < 4; i++) {
        if (work->moveUsable[work->pokeSlot][i] == TRUE) {
            work->noUsableMove = FALSE;
        }
    }
    if (IsFainted(work->mons[work->pokeSlot])) {
        for (i = 0; i < 4; i++) {
            work->buttonEnabled[i] = FALSE;
        }
        work->noUsableMove = FALSE;
        PaletteFade_RestartFade(work->paletteFade, 2, 0x3e00, 0, 8, 8, 0, work->tcbMgr);
        PaletteFade_RestartFade(work->paletteFade, 8, 0x700, 0, 8, 8, 0, work->tcbMgr);
    } else if (work->noUsableMove == TRUE) {
        work->buttonEnabled[0] = TRUE;
        BtlvInput_CreateStruggleActor(work);
        PaletteFade_RestartFade(work->paletteFade, 2, 0x3e00, 0, 8, 8, 0, work->tcbMgr);
        PaletteFade_RestartFade(work->paletteFade, 8, 0x700, 0, 8, 8, 0, work->tcbMgr);
    } else {
        PaletteFade_RestartFade(work->paletteFade, 2, 0x3e00, 0, 0, 0, 0, work->tcbMgr);
        PaletteFade_RestartFade(work->paletteFade, 8, 0x700, 0, 0, 0, 0, work->tcbMgr);
    }
}

// Draws the recorder screen's count line, unk4 / unk8, unk4 in red when it differs from unk0
static void BtlvInput_DrawRecorderCount(BtlvInput *work, const BtlvInputRecorderParam *param) {
    u8 letter;
    u8 shadow;
    u8 background;
    StrBuf *buf;
    WordSet *wordSet;
    StrBuf *fmt;
    StrBuf *label;

    wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    buf = GFL_StrBufCreate(8, work->heapId);
    fmt = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0021_Text_Empty_6);
    GFL_TextRndGetGlobalColors(&letter, &shadow, &background);
    GFL_TextRndUpdateColorIndexLUT(9, 10, 0);
    WordSetNumber(wordSet, 0, param->unk8, 3, 2, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, buf, fmt);
    GFL_TextRendererDrawToBitmap(work->msgBitmap, 140, 8, buf, work->font);
    if (param->unk0 != param->unk4) {
        GFL_TextRndUpdateColorIndexLUT(7, 8, 0);
    }
    WordSetNumber(wordSet, 0, param->unk4, 3, 2, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, buf, fmt);
    GFL_TextRendererDrawToBitmap(work->msgBitmap, 96, 8, buf, work->font);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
    label = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0021_Text_Empty_7);
    GFL_TextRendererDrawToBitmap(work->msgBitmap, 124, 8, label, work->font);
    GFL_StrBufFree(label);
    GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);
    GFL_WordSetSystemFree(wordSet);
    GFL_StrBufFree(fmt);
    GFL_StrBufFree(buf);
    PaletteFade_LoadData(work->paletteFade, PaletteFade_GetUnfadedBuffer(BtlvEffect_GetPaletteFade(), 1) + 0x20, 1, 0x70,
                         0x20);
}

// Clears the screen's text, buttons and actors and fades its palettes out
static void BtlvInput_ClearScreen(BtlvInput *work) {
    int i;

    work->selectionLocked = FALSE;
    for (i = 0; i < 8; i++) {
        work->buttonEnabled[i] = FALSE;
    }
    for (i = 0; i < 4; i++) {
        work->moveExists[i] = FALSE;
    }
    GFL_BitmapFill(work->msgBitmap, 0);
    BmpWin_FlushChar(work->msgWin);
    GFL_BitmapFill(work->subBitmap, 0);
    BmpWin_FlushChar(work->subWin);
    BtlvInput_DeleteMoveTypeIcons(work);
    BtlvInput_DeleteStruggleActor(work);
    BtlvInput_DeleteBallSlideActors(work);
    GFL_BGSysFillScrArea(4, 0, 32, 2, 32, 32, 0);
    PaletteFade_RestartFade(work->paletteFade, 2, 0x3e00, 0, 0, 0, 0, work->tcbMgr);
    PaletteFade_RestartFade(work->paletteFade, 8, 0x700, 0, 0, 0, 0, work->tcbMgr);
}

// The text color of a move's PP count
static s32 BtlvInput_GetPPColor(s32 pp, s32 maxPP) {
    if (pp == 0) {
        return PRINT_COLOR(7, 8, 0);
    }
    if (maxPP == pp) {
        return PRINT_COLOR(1, 2, 0);
    }
    if (maxPP <= 2) {
        if (pp == 1) {
            return PRINT_COLOR(5, 6, 0);
        }
    } else if (maxPP <= 7) {
        switch (pp) {
        case 1:
            return PRINT_COLOR(5, 6, 0);
        case 2:
            return PRINT_COLOR(3, 4, 0);
        }
    } else {
        if (pp <= maxPP / 4) {
            return PRINT_COLOR(5, 6, 0);
        }
        if (pp <= maxPP / 2) {
            return PRINT_COLOR(3, 4, 0);
        }
    }
    return PRINT_COLOR(1, 2, 0);
}

// Creates the party icons of the mons on the field, with windows around the ones not selected
static void BtlvInput_CreatePartyIcons(BtlvInput *work, const BtlvInputCommandParam *param) {
    int count;
    ArcTool *arc;
    int i;
    int windows;
    BtlvInputSimpleTask *data;
    s16 x;
    s16 y;

    count = (work->rule == 3) ? 2 : work->rule + 1;
    windows = 0;
    if (work->rule == 0 || work->rule == 3) {
        return;
    }
    arc = GFL_ArcSysCreateFileHandle(7, HEAPID_TAIL(work->heapId));
    work->iconCellAnims = func_0204bde0(arc, func_02021154(), getOBJTileMapping_SubEng(), work->heapId);
    work->iconPalette = func_0204bc48(arc, func_02021114(), 1, 0x160, work->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, 3, (u16)(func_0204bdc0(work->iconPalette, TRUE) >> 1), 0x60);
    G2S_SetWnd0Position(1, 0, 0, 0);
    G2S_SetWnd1Position(1, 0, 0, 0);
    if (count > 1) {
        data = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputSimpleTask), FALSE, "btlv_input.c", 5294);
        data->work = work;
        data->value = count;
        BtlvInput_AddTask(work, GFL_VBlankTCBAdd(BtlvInput_WindowDarkenTask, data, 0), NULL);
    }
    for (i = 0; i < count; i++) {
        if (param->species[i] == 0) {
            continue;
        }
        work->iconChars[i] =
            func_0204b81c(arc, PokeParty_GetIconIndex(param->species[i], param->forms[i], param->sexes[i], FALSE),
                          FALSE, 1, work->heapId);
        work->iconActors[i].actor = func_0204c040(work->iconUnit, work->iconChars[i], work->iconPalette,
                                                  work->iconCellAnims, &data_ov168_021f35f2, 1, work->heapId);
        func_0204c140(work->iconActors[i].actor, &data_ov168_021f3780[work->rule][i], 1);
        func_0204c520(work->iconActors[i].actor, TRUE);
        if (param->pokePos == i) {
            func_0204c488(work->iconActors[i].actor, 1);
        } else if (windows == 0) {
            x = data_ov168_021f3780[work->rule][i].x;
            y = data_ov168_021f3780[work->rule][i].y;
            G2S_SetWnd0Position(x - 16, y - 16, x + 16, y + 16);
            windows++;
        } else {
            x = data_ov168_021f3780[work->rule][i].x;
            y = data_ov168_021f3780[work->rule][i].y;
            G2S_SetWnd1Position(x - 16, y - 16, x + 16, y + 16);
        }
        func_0204c378(work->iconActors[i].actor,
                      func_02021034(param->species[i], param->forms[i], param->sexes[i], FALSE), 0);
    }
    GFL_ArcToolFree(arc);
}

// Frees the party icons and their resources; a task clears their windows when any was freed
static void BtlvInput_DeletePartyIcons(BtlvInput *work) {
    BtlvInputSimpleTask *data;
    BOOL none;
    int i;

    none = TRUE;
    for (i = 0; i < 3; i++) {
        if (work->iconActors[i].actor != NULL) {
            func_0204c108(work->iconActors[i].actor);
            func_0204b98c(work->iconChars[i]);
            work->iconActors[i].actor = NULL;
            work->iconChars[i] = -1;
            none = FALSE;
        }
    }
    if (work->iconCellAnims != -1) {
        func_0204be64(work->iconCellAnims);
        work->iconCellAnims = -1;
    }
    if (work->iconPalette != -1) {
        func_0204bcd0(work->iconPalette);
        work->iconPalette = -1;
    }
    if (!none) {
        data = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputSimpleTask), FALSE, "btlv_input.c", 5403);
        data->work = work;
        BtlvInput_AddTask(work, GFL_VBlankTCBAdd(BtlvInput_WindowClearTask, data, 0), NULL);
    }
}

// Shows or hides the party icons
static void BtlvInput_SetPartyIconsActive(BtlvInput *work, BOOL active) {
    func_0204bfd4(work->iconUnit, active);
}

// Creates the two actors of the ball count's slide for the ball index and starts its task
static void BtlvInput_CreateBallSlideActors(BtlvInput *work, int index) {
    const u32 palettes[5] = { 0, 1, 3, 2, 1 };
    const u32 xs[5] = { 5, 5, 1, 1, 1 };
    const u32 ys[5] = { -12, -12, -8, -8, -12 };
    ClActorPos pos;
    BtlvInputWorkTask *data;

    if (index == 0) {
        return;
    }
    work->actors210[0] = func_0204c040(work->unit20c, work->chars, work->palette, work->cellAnims, &data_ov168_021f35f2,
                                       1, work->heapId);
    pos.x = xs[index - 1] + 304;
    pos.y = ys[index - 1] + 32;
    func_0204c140(work->actors210[0], &pos, 1);
    func_0204c520(work->actors210[0], TRUE);
    func_0204c488(work->actors210[0], index + 14);
    work->actors210[1] = func_0204c040(work->unit20c, work->chars, work->palette, work->cellAnims, &data_ov168_021f35f2,
                                       1, work->heapId);
    pos.x = 304;
    pos.y = 32;
    func_0204c140(work->actors210[1], &pos, 1);
    func_0204c520(work->actors210[1], TRUE);
    func_0204c488(work->actors210[1], 14);
    func_0204c378(work->actors210[1], (u8)palettes[index - 1], 0);
    data = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputWorkTask), FALSE, "btlv_input.c", 5461);
    data->work = work;
    BtlvInput_AddTask(work, GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_SlideActorsLeftTask, data, 0),
                      BtlvInput_SlideActorsLeftEnd);
    work->subTaskCount++;
}

// Frees the slide's actors
static void BtlvInput_DeleteBallSlideActors(BtlvInput *work) {
    int i;

    for (i = 0; i < 2; i++) {
        if (work->actors210[i] != NULL) {
            func_0204c108(work->actors210[i]);
            work->actors210[i] = NULL;
        }
    }
}

// Creates the six party icons of one side of the party screen, in a row
static void BtlvInput_CreatePokeListIcons(BtlvInput *work, int side) {
    BtlvInputActor *actors;
    ClActorPos pos;
    int step;
    int base;
    int i;

    if (side == 1) {
        actors = work->pokeActors[1];
        pos.x = 148;
        pos.y = 48;
        step = -8;
        base = 4;
    } else {
        actors = work->pokeActors[0];
        pos.x = 88;
        pos.y = 136;
        step = 16;
        base = 0;
    }
    for (i = 0; i < 6; i++) {
        actors[i].actor = func_0204c040(work->pokeUnit, work->chars, work->palette, work->cellAnims,
                                        &data_ov168_021f35f2, 1, work->heapId);
        func_0204c140(actors[i].actor, &pos, 1);
        func_0204c520(actors[i].actor, TRUE);
        func_0204c488(actors[i].actor, base + work->pokeEntries[side][i].iconAnim);
        pos.x += step;
    }
}

// Frees both sides' party icons
static void BtlvInput_DeletePokeListIcons(BtlvInput *work) {
    BtlvInputActor *actors;
    int side;
    int i;

    for (side = 0; side < 2; side++) {
        actors = (side != 0) ? work->pokeActors[1] : work->pokeActors[0];
        for (i = 0; i < 6; i++) {
            if (actors[i].actor != NULL) {
                func_0204c108(actors[i].actor);
                actors[i].actor = NULL;
            }
        }
    }
}

// Creates the six hidden cursor actors
static void BtlvInput_CreateCursorActors(BtlvInput *work) {
    const u32 sequences[6] = { 0, 2, 1, 3, 1, 3 };
    int i;

    for (i = 0; i < 6; i++) {
        work->cursorActors[i].actor = func_0204c040(work->cursorUnit, work->chars2, work->palette, work->cellAnims2,
                                                    &data_ov168_021f35f2, 1, work->heapId);
        func_0204c520(work->cursorActors[i].actor, TRUE);
        func_0204c488(work->cursorActors[i].actor, sequences[i]);
        func_0204c124(work->cursorActors[i].actor, FALSE);
    }
}

// Frees the cursor actors
static void BtlvInput_DeleteCursorActors(BtlvInput *work) {
    int i;

    for (i = 0; i < 6; i++) {
        if (work->cursorActors[i].actor != NULL) {
            func_0204c108(work->cursorActors[i].actor);
            work->cursorActors[i].actor = NULL;
        }
    }
}

// Frees the move tiles' type icons
static void BtlvInput_DeleteMoveTypeIcons(BtlvInput *work) {
    int i;

    for (i = 0; i < 4; i++) {
        if (work->moveActors[i] != NULL) {
            func_0204c108(work->moveActors[i]);
            work->moveActors[i] = NULL;
        }
    }
}

static void BtlvInput_CreateStruggleActor(BtlvInput *work) {
    ClActorPos pos;
    NNSG2dImageProxy proxy;
    u8 letter, shadow, background;
    s32 width, tiles;
    StrBuf *strbuf;
    StrBuf *src;
    WordSet *wordSet;
    GFLBitmap *bitmap;
    u8 *pixels;
    int i;

    work->actor21c = func_0204c040(work->unit218, work->chars, work->palette, work->cellAnims, &data_ov168_021f35f2, 1,
                                   work->heapId);
    pos.x = 128;
    pos.y = 80;
    func_0204c140(work->actor21c, &pos, 1);
    func_0204c520(work->actor21c, TRUE);
    func_0204c488(work->actor21c, 20);

    strbuf = GFL_StrBufCreate(16, work->heapId);
    src = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0021_Text_Empty);
    wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    bitmap = GFL_BitmapCreate(14, 4, 0x20, work->heapId);
    GFL_BitmapFill(bitmap, 0);
    GFL_TextRndGetGlobalColors(&letter, &shadow, &background);
    GFL_TextRndUpdateColorIndexLUT(14, 10, 0);
    loadMoveNameToStrbuf(wordSet, 0, MOVE_STRUGGLE);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, src);
    BtlvInput_GetStringWidth(strbuf, work->font, &width, &tiles);
    GFL_TextRendererDrawToBitmap(bitmap, 56 - width / 2, 8, strbuf, work->font);
    GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);

    pixels = GFL_BitmapGetPixelData(bitmap);
    func_0204c40c(work->actor21c, &proxy);
    for (i = 0; i < 4; i++) {
        MI_CpuCopy16(&pixels[i * 0x1c0],
                     (void *)(HW_DB_OBJ_VRAM + (0xde + i * 8) * 0x20 +
                              proxy.vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_2DSUB]),
                     0x100);
        MI_CpuCopy16(&pixels[i * 0x1c0 + 0x100],
                     (void *)(HW_DB_OBJ_VRAM + (0xfe + i * 4) * 0x20 +
                              proxy.vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_2DSUB]),
                     0x80);
        MI_CpuCopy16(&pixels[i * 0x1c0 + 0x180],
                     (void *)(HW_DB_OBJ_VRAM + (0x10e + i * 2) * 0x20 +
                              proxy.vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_2DSUB]),
                     0x40);
    }

    GFL_StrBufFree(strbuf);
    GFL_StrBufFree(src);
    GFL_WordSetSystemFree(wordSet);
    GFL_BitmapFree(bitmap);
}

static void BtlvInput_DeleteStruggleActor(BtlvInput *work) {
    if (work->actor21c != NULL) {
        func_0204c108(work->actor21c);
        work->actor21c = NULL;
    }
}

// Reads the keys for a screen: moves the cursor along its stops and returns the stop's result when A or B is
// pressed, or result as it came in. lastStops lists the stops the cursor may come back to. noMoveInfo says that a
// move's information can't be shown with L held
static s32 BtlvInput_KeyInput(BtlvInput *work, const BtlvInputButtonSet *set, const BtlvInputKeyStop *stops,
                              const u8 *lastStops, s32 result, BOOL noMoveInfo) {
    int pressed = GCTX_HIDGetPressedKeys();
    u32 held = GCTX_HIDGetHeldKeys();
    BOOL decided = FALSE;
    s8 next;

    if (!work->canCancel) {
        pressed &= ~PAD_BUTTON_B;
    }
    if (work->cursorRefresh) {
        work->cursorRefresh = 0;
        BtlvInput_RestoreKeyCursor(work);
        BtlvInput_UpdateKeyCursorActors(work, set->rects, stops);
        func_0204bfd4(work->cursorUnit, TRUE);
    }
    if (result != -1) {
        *work->keyCursorFlag = 0;
        work->cursor = 0;
        work->prevCursor = 0xf;
        BtlvInput_UpdateKeyCursorActors(work, set->rects, stops);
        return result;
    }

    if (pressed != 0) {
        if (*work->keyCursorFlag != 0) {
            const BtlvInputKeyStop *stop;

            next = KEY_CURSOR_NONE;
            stop = &stops[work->cursor];

            switch (pressed) {
            case PAD_KEY_UP:
                next = stop->up;
                break;
            case PAD_KEY_DOWN:
                next = stop->down;
                break;
            case PAD_KEY_LEFT:
                next = stop->left;
                break;
            case PAD_KEY_RIGHT:
                next = stop->right;
                break;
            case PAD_BUTTON_B:
                if (stop->b < 0) {
                    if (work->laterPoke == 1) {
                        next = -stop->b;
                    }
                } else {
                    next = stop->b;
                }
                break;
            }

            if (pressed == PAD_BUTTON_A) {
                result = stop->a;
                if (!(result < 4 && (held & PAD_BUTTON_L) && work->screen == 5 &&
                      (noMoveInfo == 1 || work->noUsableMove == 1)) &&
                    !(result < 4 && (held & PAD_BUTTON_L) && work->screen == 2 && noMoveInfo == 1)) {
                    if (work->buttonEnabled[result] == 1) {
                        if (!BtlvInput_IsCancelButton(work, set, result)) {
                            BtlvInput_PlayDecideSE(work);
                        } else {
                            BtlvInput_PlayCancelSE(work);
                        }
                        decided = TRUE;
                    } else if (result < 4 && (held & PAD_BUTTON_L) && work->screen == 2 &&
                               work->moveExists[result] == 1) {
                        BtlvInput_PlayDecideSE(work);
                    }
                }
            } else if (next != KEY_CURSOR_NONE) {
                BtlvInput_PlayCursorSE(work);
                if (next < 0) {
                    if (-next != work->prevCursor && work->prevCursor != 0xf) {
                        next = work->prevCursor;
                    } else {
                        next *= -1;
                    }
                } else if (next & KEY_CURSOR_LAST) {
                    int i = 0;

                    next &= ~KEY_CURSOR_LAST;
                    for (; lastStops[i] != 0xff; i++) {
                        if (work->prevCursor == lastStops[i]) {
                            next = work->prevCursor;
                            break;
                        }
                    }
                }
                work->prevCursor = work->cursor;
                work->cursor = next;
                if (pressed == PAD_BUTTON_B) {
                    if (next != 0xf) {
                        BtlvInput_PlayCancelSE(work);
                    }
                    result = next;
                    decided = TRUE;
                }
            }
        } else if (pressed & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_X | PAD_BUTTON_Y | PAD_PLUS_KEY_MASK)) {
            *work->keyCursorFlag = 1;
            BtlvInput_RestoreKeyCursor(work);
            BtlvInput_PlayCursorSE(work);
        }
        BtlvInput_UpdateKeyCursorActors(work, set->rects, stops);
    }

    if (decided == TRUE) {
        switch (work->screen) {
        case 2:
            if (!BtlvInput_IsCancelButton(work, set, work->cursor)) {
                work->lastMove[work->pokeIndex] = work->moves[work->cursor];
            }
            // fallthrough
        case 1:
        case 5:
            if (!BtlvInput_IsCancelButton(work, set, work->cursor)) {
                work->lastCursor[work->pokeIndex][work->screen] = work->cursor;
            }
            work->selectionLocked = 1;
            break;
        case 3:
            if (!BtlvInput_IsCancelButton(work, set, result)) {
                work->lastCursor[work->pokeIndex][work->screen] =
                    work->cursor | (work->lastCursor[work->pokeIndex][2] << 4);
            }
            work->selectionLocked = 1;
            break;
        }
        work->cursor = 0;
        work->prevCursor = 0xf;
        work->cursorRefresh = 1;
        func_0204bfd4(work->cursorUnit, FALSE);
    }
    return result;
}

// Keys of the yes / no screen: 0 for left, 1 for right, 2 for B
static s32 BtlvInput_YesNoKeyInput(BtlvInput *work) {
    u32 keys = GCTX_HIDGetTypedKeys();

    if (keys & PAD_BUTTON_B) {
        BtlvInput_PlayCancelSE(work);
        return 2;
    }
    if (keys & PAD_KEY_LEFT) {
        BtlvInput_PlayDecideSE(work);
        return 0;
    }
    if (keys & PAD_KEY_RIGHT) {
        BtlvInput_PlayDecideSE(work);
        return 1;
    }
    return -1;
}

// Puts the cursor actors on the corners of the current stop's rectangles
static void BtlvInput_UpdateKeyCursorActors(BtlvInput *work, const TouchRect *rects, const BtlvInputKeyStop *stops) {
    ClActorPos pos;
    int i;

    for (i = 0; i < 6; i++) {
        s8 corner = stops[work->cursor].corners[i];

        if (corner >= 0) {
            const TouchRect *rect = &rects[corner];

            switch (i) {
            case 0:
                pos.x = rect->left;
                pos.y = rect->top;
                break;
            case 1:
                pos.x = rect->right;
                pos.y = rect->top;
                break;
            case 2:
                pos.x = rect->left;
                pos.y = rect->bottom;
                break;
            case 3:
                pos.x = rect->right;
                pos.y = rect->bottom;
                break;
            case 4:
                pos.x = rect->left;
                pos.y = rect->bottom;
                break;
            case 5:
                pos.x = rect->right;
                pos.y = rect->bottom;
                break;
            }
            func_0204c124(work->cursorActors[i].actor, *work->keyCursorFlag);
            func_0204c140(work->cursorActors[i].actor, &pos, 1);
        } else {
            func_0204c124(work->cursorActors[i].actor, FALSE);
        }
    }
}

// Starts the flash of a pressed button and locks the input until it ends. Returns -1, or index when there are no
// rows to flash
static s32 BtlvInput_StartPressFlash(BtlvInput *work, s32 index, s32 rows) {
    BtlvInputFlashWork *flash;
    TCB *task;
    BOOL lock;

    if (rows < 0) {
        return index;
    }
    if ((GCTX_HIDGetHeldKeys() & PAD_BUTTON_L) && index < 4 && (work->screen == 2 || work->screen == 5)) {
        work->pressedButton = index | 0x8000;
    } else {
        work->pressedButton = index;
    }

    flash = GFL_HeapAllocate(HEAPID_TAIL(work->heapId), sizeof(BtlvInputFlashWork), FALSE, "btlv_input.c", 6107);
    lock = FALSE;
    if (!(rows & 0x10000)) {
        lock = TRUE;
    }
    work->inputLocked = lock;
    flash->work = work;
    flash->seq = 0;
    flash->rows = rows & 0xffff;
    if (work->noUsableMove == 1 && flash->rows == 0) {
        flash->objOnly = TRUE;
    } else {
        flash->objOnly = FALSE;
    }
    task = GFL_TCBMgrAddTask(work->tcbMgr, BtlvInput_PressFlashTask, flash, 0);
    BtlvInput_AddTask(work, task, BtlvInput_PressFlashEnd);
    return -1;
}

static void BtlvInput_PressFlashTask(TCB *tcb, void *data) {
    BtlvInputFlashWork *flash = data;

    switch (flash->seq) {
    case 0:
        if (flash->objOnly == 1) {
            PaletteFade_StartFade(flash->work->paletteFade, 8, 2, 0, 0, 8, 0x7fff, flash->work->tcbMgr);
        } else {
            PaletteFade_StartFade(flash->work->paletteFade, 2, flash->rows, 0, 0, 8, 0x7fff, flash->work->tcbMgr);
        }
        flash->seq++;
        break;
    case 1:
        if (PaletteFade_GetActiveMask(flash->work->paletteFade) == 0) {
            if (flash->objOnly == 1) {
                PaletteFade_StartFade(flash->work->paletteFade, 8, 2, 0, 8, 0, 0x7fff, flash->work->tcbMgr);
            } else {
                PaletteFade_StartFade(flash->work->paletteFade, 2, flash->rows, 0, 8, 0, 0x7fff, flash->work->tcbMgr);
            }
            flash->seq++;
        }
        break;
    case 2:
        if (PaletteFade_GetActiveMask(flash->work->paletteFade) == 0) {
            BtlvInput_EndTask(flash->work, tcb);
        }
        break;
    }
}

static void BtlvInput_PressFlashEnd(TCB *tcb) {
    BtlvInputFlashWork *flash = GFL_TCBGetData(tcb);

    flash->work->inputLocked = 0;
}

// Prints the Wonder Launcher's points, from the command's parameter, into the small window
static void BtlvInput_PrintLauncherPoints(BtlvInput *work, BtlvInputCommandParam *param) {
    u8 letter, shadow, background;
    s32 width, tiles;
    WordSet *wordSet;
    StrBuf *strbuf;
    StrBuf *src;

    if (!BtlvEffect_GetUnk2E4()) {
        return;
    }
    wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    strbuf = GFL_StrBufCreate(6, work->heapId);
    src = GFL_MsgDataLoadStrbufNew(work->msgData, Bank0021_Text_Empty_8);
    GFL_TextRndGetGlobalColors(&letter, &shadow, &background);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
    WordSetNumber(wordSet, 0, param->launcherPoints, 2, 0, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, src);
    BtlvInput_GetStringWidth(strbuf, work->font, &width, &tiles);
    GFL_TextRendererDrawToBitmap(work->subBitmap, 12 - width / 2, 0, strbuf, work->font);
    GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);
    GFL_WordSetSystemFree(wordSet);
    GFL_StrBufFree(src);
    GFL_StrBufFree(strbuf);
}

static void BtlvInput_StartFingerCursor(BtlvInput *work) {
    BtlvEffect_RestartIdleEffect(2);
    BtlvEffect_ZoomCamera(work->pokePos, 1, 10, 0, 8);
}

// Forgets the Pokémon's cursor stops when it changed
static void BtlvInput_ForgetCursorOnPokeChange(BtlvInput *work) {
    BOOL changed = BtlvGauge_CheckChanged(BtlvEffect_GetGauge(), work->pokePos);

    if ((changed | BtlvMcss_CheckChanged(BtlvEffect_GetMcss(), work->pokePos)) == TRUE) {
        int i;

        for (i = 0; i < 8; i++) {
            work->lastCursor[work->pokeIndex][i] = 0;
        }
    }
}

static void BtlvInput_AddTask(BtlvInput *work, TCB *task, void (*endFunc)(TCB *tcb)) {
    int slot = BtlvInput_FreeTaskSlot(work);

    work->tasks[slot] = task;
    work->taskEndFuncs[slot] = endFunc;
}

static int BtlvInput_FindTask(BtlvInput *work, TCB *task) {
    int i;

    for (i = 0; i < 8; i++) {
        if (work->tasks[i] == task) {
            break;
        }
    }
    return i;
}

static void BtlvInput_EndTask(BtlvInput *work, TCB *task) {
    int slot = BtlvInput_FindTask(work, task);
    void *data;

    if (task == NULL) {
        return;
    }
    data = GFL_TCBGetData(task);
    if (work->taskEndFuncs[slot] != NULL) {
        work->taskEndFuncs[slot](task);
    }
    if (data != NULL) {
        GFL_HeapFree(data);
    }
    GFL_TCBRemove(task);
    work->tasks[slot] = NULL;
    work->taskEndFuncs[slot] = NULL;
}

// A free slot, ending the first task when there is none
static int BtlvInput_FreeTaskSlot(BtlvInput *work) {
    int i;

    for (i = 0; i < 8; i++) {
        if (work->tasks[i] == NULL) {
            break;
        }
    }
    if (i == 8) {
        BtlvInput_EndTask(work, work->tasks[0]);
        i = 0;
    }
    return i;
}

static void BtlvInput_EndAllTasks(BtlvInput *work) {
    int i;

    for (i = 0; i < 8; i++) {
        if (work->tasks[i] != NULL) {
            BtlvInput_EndTask(work, work->tasks[i]);
        }
    }
}

// Whether a rectangle is a cancel, which forgets the cursor stop of a later Pokémon's command screen
static BOOL BtlvInput_IsCancelButton(BtlvInput *work, const BtlvInputButtonSet *set, u32 index) {
    BOOL cancel;

    if (!work->laterPoke || work->screen != 1) {
        return set->flags[index] & 1;
    }
    cancel = (set->flags[index] & 2) >> 1;
    if (cancel == 1) {
        work->lastCursor[work->pokeIndex][work->screen] = 0;
    }
    return cancel;
}

// Puts the cursor on the stop remembered for the screen
static void BtlvInput_RestoreKeyCursor(BtlvInput *work) {
    switch (work->screen) {
    case 2:
        if (work->lastMove[work->pokeIndex] != work->moves[work->lastCursor[work->pokeIndex][work->screen]]) {
            work->lastCursor[work->pokeIndex][3] = 0;
        }
        // fallthrough
    case 1:
    case 5:
        work->cursor = work->lastCursor[work->pokeIndex][work->screen];
        break;
    case 3:
        if ((work->lastCursor[work->pokeIndex][work->screen] & 0xf0) >> 4 == work->lastCursor[work->pokeIndex][2]) {
            work->cursor = work->lastCursor[work->pokeIndex][work->screen];
        } else {
            work->cursor = 0;
        }
        break;
    default:
        work->cursor = 0;
        break;
    }
}

// Loads the saved palette row darkened, outside the Battle Subway and when the Launcher is off
static void BtlvInput_LoadDarkPalette(BtlvInput *work) {
    u16 colors[16];

    if (BtlSetup_GetBattleType(BtlvEffect_GetMainModule()) != 4 && !BtlvEffect_GetUnk304()) {
        BlendColors(work->savedPalette, colors, 16, 8, 0);
        PaletteFade_LoadData(BtlvEffect_GetPaletteFade(), colors, 1, 0x20, 0x20);
    }
}

static void BtlvInput_RestoreSavedPalette(BtlvInput *work) {
    if (work->inBattle == 1) {
        PaletteFade_LoadData(BtlvEffect_GetPaletteFade(), work->savedPalette, 1, 0x20, 0x20);
    }
}

// The command screen's button map: with a back button for a later Pokémon, with SHIFT on a triple battle's side
// position, with LAUNCHER when the Wonder Launcher is on
static u32 BtlvInput_ChooseButtonMap(BtlvInput *work) {
    if (!work->laterPoke) {
        if (work->rule == 2) {
            if (BtlvEffect_GetState() == 1) {
                if (work->pokePos == 4) {
                    return BtlvInput_FileId(369, work);
                }
                return BtlvInput_FileId(363, work);
            }
            if (work->pokePos == 4) {
                return BtlvInput_FileId(361, work);
            }
            return BtlvInput_FileId(362, work);
        }
        if (BtlvEffect_GetState() == 1) {
            return BtlvInput_FileId(369, work);
        }
        return BtlvInput_FileId(361, work);
    }
    if (work->rule == 2 && work->pokePos != 4) {
        if (BtlvEffect_GetState() == 1) {
            return BtlvInput_FileId(371, work);
        }
        return BtlvInput_FileId(365, work);
    }
    if (BtlvEffect_GetState() == 1) {
        return BtlvInput_FileId(370, work);
    }
    return BtlvInput_FileId(364, work);
}

// Whether sounds play
static BOOL BtlvInput_SoundsEnabled(BtlvInput *work) {
    if (work->screen != 6 && work->unk54 == 3) {
        if (work->unk328 == 2) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

static void BtlvInput_PlayOpenSE(BtlvInput *work) {
    if (BtlvInput_SoundsEnabled(work)) {
        GFL_SndSEPlay(SEQ_SE_OPEN2);
    }
}

static void BtlvInput_PlayCursorSE(BtlvInput *work) {
    if (BtlvInput_SoundsEnabled(work)) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
    }
}

static void BtlvInput_PlayDecideSE(BtlvInput *work) {
    if (BtlvInput_SoundsEnabled(work)) {
        GFL_SndSEPlay(SEQ_SE_DECIDE2);
    }
}

static void BtlvInput_PlayCancelSE(BtlvInput *work) {
    if (BtlvInput_SoundsEnabled(work)) {
        GFL_SndSEPlay(SEQ_SE_CANCEL2);
    }
}

static void BtlvInput_PlayRotationSE(BtlvInput *work) {
    if (BtlvInput_SoundsEnabled(work)) {
        GFL_SndSEPlay(SEQ_SE_ROTATION_S);
    }
}

static void BtlvInput_PlayDecideSE2(BtlvInput *work) {
    if (BtlvInput_SoundsEnabled(work)) {
        GFL_SndSEPlay(SEQ_SE_DECIDE2);
    }
}

static void BtlvInput_PlayBeepSE(BtlvInput *work) {
    if (BtlvInput_SoundsEnabled(work)) {
        GFL_SndSEPlay(SEQ_SE_BEEP);
    }
}
