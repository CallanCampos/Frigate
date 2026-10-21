#ifndef POKEBW2_SYSTEM_NET_SAVE_H
#define POKEBW2_SYSTEM_NET_SAVE_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Saving the game on two machines at once (net_save.c)

// Starts the save
NetSave *NetSave_Create(HeapID heapId, GameData *gameData);
// Starts the save, calling func with work each frame while the save is written
NetSave *NetSave_CreateWithCallback(HeapID heapId, GameData *gameData, void (*func)(void *work), void *work);
// Whether the save has ended
BOOL NetSave_Update(NetSave *save);
void NetSave_Free(NetSave *save);

#endif // POKEBW2_SYSTEM_NET_SAVE_H
