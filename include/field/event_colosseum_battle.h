#ifndef POKEBW2_FIELD_EVENT_COLOSSEUM_BATTLE_H
#define POKEBW2_FIELD_EVENT_COLOSSEUM_BATTLE_H

// Overlay 12's event_colosseum_battle.c (a descriptive name): the battle that overlay 28's Union Room colosseum
// starts, from the play category the players chose

#include "types.h"
#include "battle/battle_proc.h"
#include "struct_decls.h"
#include "system/game_event.h"

// The players of the battle that overlay 28's Colosseum_BuildBattlePlayers builds: battle_proc.h's BattlePlayers, then the
// records
typedef struct {
    BattlePlayers battle;
    u32 unk50;
    GameRecords *records;
} ColosseumPlayers;

typedef struct {
    PokeParty *party;
    u32 unk4;
    u32 unk8;
    u32 unkC;
    u16 bgm;
    u8 unk12;
    Regulation *regulation;
    ColosseumPlayers players;
} ColosseumBattleParam;

GameEvent *func_ov012_02152704(GameSystem *gsys, Field *field, u32 category, ColosseumBattleParam *param);

#endif // POKEBW2_FIELD_EVENT_COLOSSEUM_BATTLE_H
