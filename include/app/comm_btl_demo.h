#ifndef POKEBW2_APP_COMM_BTL_DEMO_H
#define POKEBW2_APP_COMM_BTL_DEMO_H

#include "types.h"
#include "battle/battle_proc.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The screen before and after a link battle, overlay 305 (comm_btl_demo.c): the trainers' names and their parties as
// rows of Poké Balls around a "VS", and after the battle the Pokémon each side has left and who won
#define OVERLAY_COMM_BTL_DEMO OVERLAY_ID(305)

// The screen's heap and its archive of graphics. Our names, not swan's
#define HEAPID_COMM_BTL_DEMO 0x74
#define ARCID_COMM_BTL_DEMO 178

// What the screen shows, BattlePlayers' unk44: the introduction or the result, of a battle of two or of four players
enum {
    COMM_BTL_DEMO_TYPE_NONE,
    COMM_BTL_DEMO_TYPE_SINGLE_START,
    COMM_BTL_DEMO_TYPE_SINGLE_END,
    COMM_BTL_DEMO_TYPE_MULTI_START,
    COMM_BTL_DEMO_TYPE_MULTI_END,
};

// BattlePlayers' result, for the first player's side
enum {
    COMM_BTL_DEMO_RESULT_WIN,
    COMM_BTL_DEMO_RESULT_LOSE,
    COMM_BTL_DEMO_RESULT_DRAW,
};

// What the result screen shows a Pokémon as, a BattlePlayer's order[]
enum {
    COMM_BTL_DEMO_POKE_HEALTHY,
    COMM_BTL_DEMO_POKE_SICK,
    COMM_BTL_DEMO_POKE_FAINTED,
    COMM_BTL_DEMO_POKE_NONE,
};

// What the screen is called with
typedef struct {
    BattlePlayers battle;
    // Set for the Wi-Fi competition, which has its own background and sounds
    BOOL competition;
    // Where the result screen counts the battle and its result, or NULL
    GameRecords *records;
} CommBtlDemoParam;

extern const GameProcFunctions CommBtlDemoProc;

#endif // POKEBW2_APP_COMM_BTL_DEMO_H
