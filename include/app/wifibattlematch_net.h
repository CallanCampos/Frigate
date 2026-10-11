#ifndef POKEBW2_APP_WIFIBATTLEMATCH_NET_H
#define POKEBW2_APP_WIFIBATTLEMATCH_NET_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 260's wifibattlematch_net.c (named by its embedded string): the net part of the Wi-Fi battle competitions.
// Only the functions that overlay 263's party exchange calls are declared so far, and none has a name yet

typedef struct WifiBattleMatchNet WifiBattleMatchNet;

// The proc of the Wi-Fi link, which overlay 290 chains
extern const GameProcFunctions data_ov260_021bfcd8;

WifiBattleMatchNet *func_ov260_021b8cc8(void *a0, GameData *gameData, u32 a2, HeapID heapId);
void func_ov260_021b8da4(WifiBattleMatchNet *net);
// Sends the team picked, and then is TRUE once it is sent
BOOL func_ov260_021bb21c(WifiBattleMatchNet *net, PokeParty *party);
// Copies the other player's team into party, and then is TRUE, once it has arrived
BOOL func_ov260_021bb25c(WifiBattleMatchNet *net, PokeParty *party);

// Which a Wi-Fi library error of the right range is passed to
void func_ov260_021bec44(void);

#endif // POKEBW2_APP_WIFIBATTLEMATCH_NET_H
