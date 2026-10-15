#ifndef POKEBW2_APP_WIFIBATTLEMATCH_BATTLE_H
#define POKEBW2_APP_WIFIBATTLEMATCH_BATTLE_H

#include "types.h"
#include "battle/battle_proc.h"
#include "battle/btl_setup.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// wifibattlematch_battle.c, in overlay 290 (a guessed name; the file has no string): the proc that
// wifibattlematch_sys.c's battle step runs

// The players of a battle, with what the ranking needs
typedef struct {
    BattlePlayers base;
    u32 unk50;
    GameRecords *records;
} WifiBattleMatchPlayers;

// What the battle step gets
typedef struct {
    GameData *gameData;
    BtlSetup *setup;
    WifiBattleMatchPlayers *players;
    u32 result;
} WifiBattleMatchBattleParam;

extern const GameProcFunctions WIFIBATTLEMATCH_BATTLE_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WIFIBATTLEMATCH_BATTLE_H
