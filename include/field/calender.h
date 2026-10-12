#ifndef POKEBW2_FIELD_CALENDER_H
#define POKEBW2_FIELD_CALENDER_H

// Overlay 12's calender.c: the weather of the zones whose weather follows the date. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A zone with weather by date, and where its year of weather starts in the archive's first file
typedef struct {
    u16 zoneId;
    u16 offset;
} CalendarZone;

struct Calendar {
    HeapID heapId;
    GameData *gameData;
    ArcTool *arc;
    CalendarZone zones[24];
};

Calendar *Calendar_Create(GameData *gameData, HeapID heapId);
void Calendar_Free(Calendar *calendar);
u16 Calendar_GetWeather(Calendar *calendar, u16 zoneId);

#endif // POKEBW2_FIELD_CALENDER_H
