#ifndef POKEBW2_DEMO_THE_END_H
#define POKEBW2_DEMO_THE_END_H

// The end screen after the credits. The ROM names the_end_graphic.c; the_end.c, the proc file before it, is a guess

#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The "THE END" picture that the game shows once the credits are over, overlay 267 (the_end.c and
// the_end_graphic.c). A black screen in Black 2 and a white one in White 2, which A, B or START ends
#define OVERLAY_THE_END OVERLAY_ID(267)

typedef struct TheEndGraphic TheEndGraphic;

extern const GameProcFunctions THE_END_PROC_FUNCTIONS;

// the_end_graphic.c
TheEndGraphic *TheEndGraphic_Create(u32 layout, HeapID heapId);
void TheEndGraphic_Free(TheEndGraphic *graphic);
void TheEndGraphic_Update(TheEndGraphic *graphic);

#endif // POKEBW2_DEMO_THE_END_H
