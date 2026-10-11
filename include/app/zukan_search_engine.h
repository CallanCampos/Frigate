#ifndef POKEBW2_APP_ZUKAN_SEARCH_ENGINE_H
#define POKEBW2_APP_ZUKAN_SEARCH_ENGINE_H

#include "types.h"
#include "app/zukan_common.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Pokédex's search, overlay 302 (zukan_search_engine.c, by the ROM's string): from the species of the Pokédex's
// list it keeps those that match the criteria, a few at a time so that the search screen goes on drawing. The names
// are ours; swan has none

typedef struct ZukanSearchEngine ZukanSearchEngine;

ZukanSearchEngine *ZukanSearchEngine_Create(PokeDexSave *pokedex, const ZukanSearchCriteria *criteria, HeapID heapId);
void ZukanSearchEngine_Free(ZukanSearchEngine *engine);
// Runs a step of the search, returning TRUE once it is done, and then the species found and their count
BOOL ZukanSearchEngine_Step(ZukanSearchEngine *engine, HeapID heapId, u16 *count, u16 **list);
// Loads the species of the regional Pokédex (type 1) or the national one (type 0) in their order, and returns their
// number
u32 ZukanSearchEngine_LoadList(u32 type, HeapID heapId, u16 **list);

#endif // POKEBW2_APP_ZUKAN_SEARCH_ENGINE_H
