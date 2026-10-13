#ifndef POKEBW2_FIELD_EVENT_FIELD_TRADE_H
#define POKEBW2_FIELD_EVENT_FIELD_TRADE_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/fld_trade.h"
#include "struct_decls.h"
#include "system/game_event.h"

typedef struct {
    GameSystem *gameSystem;
    GameData *gameData;
    PokeParty *party;
    u8 offerIndex;
    u8 partyIndex;
    u8 paddingE[2];
    FieldTradeInput *input;
    u8 subprocessData[0x30];
    GameData *transitionGameData;
    PartyPkm *partyPkm;
    PartyPkm *tradePkm;
    PlayerInfo *playerInfo;
    PlayerInfo *tradeTrainer;
    void *evolutionParam;
} EventFieldTradeWork;

GameEvent *EventFieldTrade_Create(GameSystem *gsys, u8 offerIndex, u8 partyIndex);
GameEventReturnCode EventFieldTrade_Callback(GameEvent *event, u32 *state, void *data);
void EventFieldTrade_CreatePkm(GameData *gameData, HeapID heapId, PartyPkm *pkm, FieldTradeOfferData *offer,
                               u32 offerIndex);
void EventFieldTrade_DebugLogPkm(PartyPkm *pkm);
void func_ov033_0217a864(void *param);

extern const GameProcFunctions data_ov194_021c63ac;

#endif // POKEBW2_FIELD_EVENT_FIELD_TRADE_H
