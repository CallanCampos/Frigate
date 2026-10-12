#ifndef POKEBW2_APP_MEDALBOX_DATA_SYS_H
#define POKEBW2_APP_MEDALBOX_DATA_SYS_H

#include "types.h"
#include "gfl/heap.h"
#include "save/medal_box.h"
#include "struct_decls.h"

// The medals that the Medal Box screen (overlay 145) shows, and their order. Named after the ROM's
// "medalbox_data_sys.c"; the function names are ours

// The medals of a column of the screen
#define MEDALBOX_COLUMN_SIZE 5
// The fewest columns the box has
#define MEDALBOX_COLUMN_MIN 10

// The orders the medals can be shown in
enum {
    // By the day they were obtained, the oldest first
    MEDALBOX_SORT_OLDEST,
    MEDALBOX_SORT_NEWEST,
    MEDALBOX_SORT_MEDALS_ONLY,
    MEDALBOX_SORT_HINTS_ONLY,
    MEDALBOX_SORT_NAME,
    // By number, the hint medals last
    MEDALBOX_SORT_TYPE,
};

// A medal of the box: one the player has (MEDAL_STATUS_OBTAINED) or a hint medal (MEDAL_STATUS_DISCOVERED)
typedef struct {
    u16 id;
    u8 status;
    u8 year;
    u8 month;
    u8 day;
    // The sequence of the medal's cell actor
    u8 sequence;
    // Where the medal's name comes in the alphabet
    u8 nameOrder;
    // Not looked at yet
    BOOL special;
} MedalBoxEntry;

typedef struct {
    MedalBoxEntry *medals[MEDALBOX_COLUMN_SIZE];
    u8 count;
    u8 column;
} MedalBoxColumn;

typedef struct {
    int sort;
    // Indexes into entries in the order shown, 0xff after the last
    u16 order[510];
    // The medal the player registered as the favorite, or 0xff
    u16 favoriteMedal;
    MedalBoxEntry *entries;
    u8 unk408;
    u8 count;
    u8 columnCount;
} MedalBoxDataSys;

void MedalBoxDataSys_Init(MedalBoxDataSys *data, GameData *gameData, HeapID heapId);
void MedalBoxDataSys_Free(MedalBoxDataSys *data);
// Writes the favorite medal to the save
void MedalBoxDataSys_SaveFavorite(MedalBoxDataSys *data, GameData *gameData, HeapID heapId);
// Fills in a column of the order
void MedalBoxDataSys_GetColumn(MedalBoxDataSys *data, MedalBoxColumn *column, int index);
void MedalBoxDataSys_Sort(MedalBoxDataSys *data, int sort);
// Where a medal is in the order, or 0xff
u8 MedalBoxDataSys_FindMedal(MedalBoxDataSys *data, int medal);

#endif // POKEBW2_APP_MEDALBOX_DATA_SYS_H
