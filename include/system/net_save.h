#ifndef POKEBW2_SYSTEM_NET_SAVE_H
#define POKEBW2_SYSTEM_NET_SAVE_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Saving the game on two machines at once (net_save.c)

// Starts the save
NetSave *func_02012ee4(HeapID heapId, GameData *gameData);
// Starts the save, calling func with work each frame while the save is written
NetSave *func_02012f1c(HeapID heapId, GameData *gameData, void (*func)(void *work), void *work);
// Whether the save has ended
BOOL func_02012f5c(NetSave *save);
void func_02012f8c(NetSave *save);

#endif // POKEBW2_SYSTEM_NET_SAVE_H
