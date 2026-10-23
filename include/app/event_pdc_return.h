#ifndef POKEBW2_APP_EVENT_PDC_RETURN_H
#define POKEBW2_APP_EVENT_PDC_RETURN_H

// Overlay 330's event_pdc_return.c, after its embedded name: the return from the Entree Forest's capture (overlay
// 172's pdc.c). For a caught Pokémon it registers it in the Pokédex, shows the Pokédex entry (overlay 297), offers a
// nickname (overlay 280) and puts it in the party, or in a box when the party is full

#include "types.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "struct_decls.h"

// How the capture ended
enum {
    PDC_RESULT_CAUGHT = 1,
    // The Pokémon was seen but not caught
    PDC_RESULT_SEEN,
};

typedef struct {
    GameData *gameData;
    // PDC_RESULT_*
    u32 result;
    PartyPkm *pkm;
} EventPdcReturnParam;

EventPdcReturnParam *EventPdcReturn_CreateParam(GameData *gameData, u32 result, PartyPkm *pkm, HeapID heapId);
void EventPdcReturn_FreeParam(EventPdcReturnParam *param);

extern const GameProcFunctions EVENT_PDC_RETURN_PROC_FUNCTIONS;

#endif // POKEBW2_APP_EVENT_PDC_RETURN_H
