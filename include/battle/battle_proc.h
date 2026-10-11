#ifndef POKEBW2_BATTLE_BATTLE_PROC_H
#define POKEBW2_BATTLE_BATTLE_PROC_H

#include "types.h"
#include "gfl/overlay.h"
#include "app/ov306.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 10 starts a battle, overlay 167 is the battle system and overlay 213 selects the party for a Wi-Fi battle
#define OVERLAY_BATTLE OVERLAY_ID(10)
#define OVERLAY_BATTLE_MAIN OVERLAY_ID(167)
#define OVERLAY_BATTLE_SELECT OVERLAY_ID(213)

typedef struct {
    PokeParty *party;
    PlayerInfo *info;
    u8 unk08[2];
    // The party slots of the Pokémon the player sends out
    u8 order[6];
} BattlePlayer;

// The players of a battle, and the result that the battle proc leaves
typedef struct {
    BattlePlayer players[4];
    u32 result;
    u32 unk44; // 3 when every player has a party, else 1 (Colosseum_BuildBattlePlayers)
    u32 rule;
    u32 unk4C;
} BattlePlayers;

// What overlay 10's battle proc runs: the party select, the battle and overlay 306's screen after it
typedef struct {
    GameData *gameData;
    BtlSetup *setup;
    BattlePlayers *players;
    u32 rule;
    u32 unk10;
    u32 unk14;
    Ov306Param ov306Param;
} BattleParam;

// The battle proc's work
typedef struct {
    GameProcManager *manager;
    // What overlay 171 gets
    struct {
        PokeParty *party0;
        PokeParty *party1;
        u32 unk08;
    } ov171Param;
    BOOL ov167Loaded;
    BOOL commandsSet;
    BOOL vsPlayerLoaded;
} BattleProcWork;

extern const GameProcFunctions data_ov010_0215039c;
// The battle system's proc
extern const GameProcFunctions data_ov167_021d6ce0;
extern const GameProcFunctions data_ov171_021deaec;
extern const GameProcFunctions data_ov305_0219e990;

BOOL func_ov010_0214ff00(GameProc *proc, u32 *state, void *param, void *work);
BOOL func_ov010_0214ff28(GameProc *proc, u32 *state, void *param, void *work);
BOOL func_ov010_0214ff58(GameProc *proc, u32 *state, void *param, void *work);

#endif // POKEBW2_BATTLE_BATTLE_PROC_H
