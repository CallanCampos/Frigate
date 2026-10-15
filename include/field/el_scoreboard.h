#ifndef POKEBW2_FIELD_EL_SCOREBOARD_H
#define POKEBW2_FIELD_EL_SCOREBOARD_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

ElScoreboard *ElScoreboard_Create(void *texture, const char *texName, const char *plName, const StrBuf *text, u16 a4,
                                  u16 a5, HeapID heapId);
void ElScoreboard_Free(ElScoreboard *board);
void ElScoreboard_Update(ElScoreboard *board);

#endif // POKEBW2_FIELD_EL_SCOREBOARD_H
