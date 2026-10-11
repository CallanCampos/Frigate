#ifndef POKEBW2_APP_BSUBWAY_RECORDS_H
#define POKEBW2_APP_BSUBWAY_RECORDS_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 141, the Battle Subway's records screen: the list of players' records, ten to a page, with each one's
// name, country and message. overlay 12's event_bsubway.c runs it

typedef struct {
    GameData *gameData;
    // The rank and the train number that the messages show, from the Battle Subway's save
    u32 rank;
    u32 trainNumber;
} BSubwayRecordsParam;

extern const GameProcFunctions BSUBWAY_RECORDS_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BSUBWAY_RECORDS_H
