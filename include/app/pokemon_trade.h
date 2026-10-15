#ifndef POKEBW2_APP_POKEMON_TRADE_H
#define POKEBW2_APP_POKEMON_TRADE_H

#include "types.h"
#include "app/gtsnego.h"
#include "demo/shinka_demo.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

#define OVERLAY_POKEMONTRADE OVERLAY_ID(194)

// What to do after the trade
#define TRADE_NEXT_EVOLVE 1
#define TRADE_NEXT_NEGOTIATE 2

typedef struct {
    u32 unk0;
    u32 unk4;
    u32 next;
    // After TRADE_NEXT_EVOLVE, the species the Pokémon evolves into and how, which the events pass to the evolution
    // demo
    u32 evolveSpecies;
    u32 evolveMethod;
    GameData *gameData;
    GtsNegoParam *nego;
    PokeParty *party;
    // Not used by the trade, the events keep the evolution demo's parameter here
    ShinkaDemoParam *evolution;
    void *buffer;
    // Where the Pokémon traded away is
    u16 box;
    u16 slot;
    u16 friendIndex;
    u16 unk2E;
} PokemonTradeParam;

// The parameter of the procs that only show the trade's animation
typedef struct {
    PokemonTradeParam trade;
    GameData *gameData;
    // The Pokémon of this player and of the other
    PartyPkm *pkm[2];
    PlayerInfo *myInfo;
    PlayerInfo *partnerInfo;
} PokemonTradeDemoParam;

// For GTS Negotiation
extern const GameProcFunctions POKEMONTRADE_PROC_FUNCTIONS;
extern const GameProcFunctions POKEMONTRADE_WIFICLUB_PROC_FUNCTIONS;
// For the infrared event
extern const GameProcFunctions data_ov194_021c63dc;
// The procs that only show the trade's animation
extern const GameProcFunctions data_ov194_021c63ac;
extern const GameProcFunctions data_ov194_021c63b8;
extern const GameProcFunctions data_ov194_021c63d0;
extern const GameProcFunctions data_ov194_021c6400;
// A trade that runs the GTS Negotiation's proc table with other values
extern const GameProcFunctions data_ov194_021c640c;

// The trade demo's parameter for the Global Trade Station
typedef struct {
    u8 unk0[0x30];
    GameData *gameData;
    PartyPkm *sendPkm;
    PartyPkm *recvPkm;
    PlayerInfo *myStatus;
    PlayerInfo *partnerStatus;
} PokemonTradeGtsParam;

// For the Global Trade Station: a deposit, a received trade, and a trade made or picked up
extern const GameProcFunctions data_ov194_021c6400;
extern const GameProcFunctions data_ov194_021c63b8;
extern const GameProcFunctions data_ov194_021c63d0;

#endif // POKEBW2_APP_POKEMON_TRADE_H
