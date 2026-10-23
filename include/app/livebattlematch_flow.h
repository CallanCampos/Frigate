#ifndef POKEBW2_APP_LIVEBATTLEMATCH_FLOW_H
#define POKEBW2_APP_LIVEBATTLEMATCH_FLOW_H

#include "types.h"
#include "app/wifibattlematch.h"
#include "app/wifibattlematch_graphic.h"
#include "app/wifibattlematch_util.h"
#include "app/wifibattlematch_view.h"
#include "gfl/heap.h"
#include "save/battle_rec.h"
#include "save/tournament_block.h"
#include "struct_decls.h"

// Overlay 261's livebattlematch_flow.c (named by its embedded string): the flow of the live competitions. Only what
// battle_championship_flow.c calls is declared so far, and none has a name yet

typedef struct LiveBattleMatchFlow LiveBattleMatchFlow;

// What battle_championship_flow.c gives the flow
typedef struct {
    // 0 to 3: the flow of a player who joins (0), of one who comes back to the menu (1) or after a match (2), or who
    // quits (3)
    u32 mode;
    WifiBattleMatchMsgBox *msgBox;
    Font *font;
    PrintQueue *printQueue;
    WifiBattleMatchRes *seq;
    WifiBattleMatchGraphic *net;
    GameData *gameData;
    // The player's data to send, where the other player's goes, and the result of the match (the last three of
    // overlay 290's parameters)
    WifiBattleMatchPlayer *playerData;
    WifiBattleMatchPlayer *partnerData;
    WifiBattleMatchBattleResult *matchResult;
} LiveBattleMatchFlowParam;

LiveBattleMatchFlow *LiveBattleMatchFlow_Create(const LiveBattleMatchFlowParam *param, HeapID heapId);
void LiveBattleMatchFlow_Delete(LiveBattleMatchFlow *flow);
// Called every frame
void LiveBattleMatchFlow_Main(LiveBattleMatchFlow *flow);
// 0 while the flow runs, then why it ended
u32 LiveBattleMatchFlow_GetResult(LiveBattleMatchFlow *flow);

// Functions of the save and battle video code that have no header yet (the tournament block's are
// save/tournament_block.h's)
// The live competition's record: clears it and gives it this console's address, adds an amount to a count (a
// LIVEMATCH_COUNT_*), sets the address of an opponent by its place and a byte kept of it (a
// LIVEMATCH_OPPONENT_PARAM_*), and whether an address is among the opponents
void LiveMatchData_Init(LiveMatchData *record);
void LiveMatchData_AddCount(LiveMatchData *record, u32 index, int amount);
void LiveMatchData_SetOpponentMacAddress(LiveMatchData *record, u32 place, const u8 *mac);
void LiveMatchData_SetOpponentParam(LiveMatchData *record, u32 place, u32 param, u8 value);
BOOL LiveMatchData_HasOpponent(LiveMatchData *record, const u8 *mac);

#endif // POKEBW2_APP_LIVEBATTLEMATCH_FLOW_H
