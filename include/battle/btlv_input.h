#ifndef POKEBW2_BATTLE_BTLV_INPUT_H
#define POKEBW2_BATTLE_BTLV_INPUT_H

// Overlay 168's btlv_input.c (named by its string), the battle's lower screen: the command, move, target, yes/no,
// rotation and recorder screens, their transitions and their touch and key input. The names are ours; swan has none
// for this file. Another session's notes, ~/Projects/White2Decomp/docs/battle-ui-spec.md (White 2 addresses, +0x40),
// were the source of our understanding of the screens and their transitions, not of names or code

#include "types.h"
#include "gfl/heap.h"
#include "gfl/touchpanel.h"
#include "struct_decls.h"

// A screen's buttons: their touch rectangles, a flag word per button (bit 0: a cancel, bit 1: a cancel for a later
// Pokémon on the command screen) and the palette rows a press flashes (-1: none, bit 16: the input stays unlocked)
typedef struct {
    const TouchRect *rects;
    const s32 *flags;
    const s32 *flashRows;
} BtlvInputButtonSet;

// A stop of the key cursor, 12 bytes
struct BtlvInputKeyStop {
    s8 corners[6]; // the rectangle each cursor actor sits on a corner of, -1 to hide it
    s8 up;         // the stop each key moves to, KEY_CURSOR_NONE for none; -n is the stop the cursor came from, or n
    s8 down;
    s8 left;
    s8 right;
    s8 a; // the result of A
    s8 b; // the result of B; -n presses stop n, only for a later Pokémon
};

#define KEY_CURSOR_NONE (-128)
// Set on a stop: the stop the cursor came from, if it is in the list the caller passes
#define KEY_CURSOR_LAST 0x40

// A party slot of the command screen; only the icon animation byte is read
typedef struct {
    u8 unk0;
    u8 iconAnim; // added to the actor's base sequence
    u16 unk2;
    u32 unk4;
    u32 unk8;
} BtlvInputPokeEntry;

// Screen 1, the command screen
typedef struct {
    BtlvInputPokeEntry entries[2][6]; // 0x00
    BOOL laterPoke;                   // 0x90  not the first mon to choose this turn
    BOOL secondRow;                   // 0x94
    u32 species[3];                   // 0x98  the party icons of the mons on the field
    u32 forms[3];                     // 0xa4
    u32 sexes[3];                     // 0xb0
    s32 pokePos;                      // 0xbc  the battle position, halved in place from 2 on into a slot
    u32 scdState;      // 0xc0  the creator's scdState, which nothing reads
    u8 launcherPoints; // 0xc4  the Wonder Launcher's points
    u8 ballSlide;      // 0xc5  the ball count's slide, 0 for none
} BtlvInputCommandParam;

// Screen 2, the move screen
typedef struct {
    u16 moves[4];    // 0x00
    u8 pp[4];        // 0x08
    u8 maxPP[4];     // 0x0c
    u32 pos;         // 0x10  the battle position
    BOOL noMoveInfo; // 0x14  a move's information can't be shown with L held
} BtlvInputMoveParam;

// A panel of the target screen, 12 bytes
typedef struct {
    u8 msgForm : 2; // the name's message, by sex mark
    u8 selectable : 1;
    u8 unk1;
    u16 unk2;
    s16 hp;    // 0: no mon in that position, or it has fainted
    s16 maxHp;
    PartyPkm *pkm;
} BtlvInputTargetEntry;

// Screen 3, the target screen
typedef struct {
    BtlvInputTargetEntry mons[6];
    u8 attackerPos; // 0x48
    u8 range;       // 0x49  the move's range
} BtlvInputTargetParam;

// Screen 4, yes or no
typedef struct {
    StrBuf *strs[2];
    BOOL canCancel;
} BtlvInputYesNoParam;

// Screen 5, the rotation screen
typedef struct {
    BattleMon *mons[3];
    BOOL moveUsable[3][4];
} BtlvInputRotationParam;

// Screen 6, the recorder screen
typedef struct {
    s32 unk0;
    s32 unk4; // printed left of the slash, in red when it differs from unk0
    s32 unk8; // printed right of it
    u32 mode; // 0 none, 1-3 a message (message 9-11) and BG 4 scrolled
} BtlvInputRecorderParam;

BtlvInput *BtlvInput_CreateSimple(GameData *gameData, u32 rule, PaletteFade *paletteFade, Font *font, u8 *keyCursorFlag,
                                  HeapID heapId);
BtlvInput *BtlvInput_CreateForBattle(GameData *gameData, u32 rule, u32 unk54, PaletteFade *paletteFade, Font *font,
                                     u8 *keyCursorFlag, u32 scdState, u8 unk328, u32 fileSet, u32 typeCount,
                                     u32 typeMask, BOOL noCancel, HeapID heapId);
void BtlvInput_Delete(BtlvInput *work);
void BtlvInput_Update(BtlvInput *work);
void BtlvInput_StartPaletteFade(BtlvInput *work, u32 mode);
void BtlvInput_StartFadeInWithGraphics(BtlvInput *work);
BOOL BtlvInput_IsFading(BtlvInput *work);
// screen: 0 standby, 1 command, 2 moves, 3 target, 4 yes/no, 5 rotation, 6 recorder, 7 the recorder's buttons; param
// is that screen's parameter struct
void BtlvInput_SetScreen(BtlvInput *work, u32 screen, void *param);
s32 BtlvInput_CheckInput(BtlvInput *work, const BtlvInputButtonSet *set, const BtlvInputKeyStop *stops);
BOOL BtlvInput_FingerDemoMain(BtlvInput *work);
BOOL BtlvInput_MoveScreenMain(BtlvInput *work, u8 *outSlot, s32 *outButton);
BOOL BtlvInput_IsBusy(BtlvInput *work);
u32 BtlvInput_GetScreen(BtlvInput *work);
void BtlvInput_SetTypeIconVisible(BtlvInput *work, BOOL visible, int index, int sequence);
void BtlvInput_StartTypeIconAnim(BtlvInput *work, int index);
u32 BtlvInput_GetTypeMask(BtlvInput *work);

#endif // POKEBW2_BATTLE_BTLV_INPUT_H
