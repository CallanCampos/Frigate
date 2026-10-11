#include "types.h"
#include "field/pleasure_boat.h"
#include "field/pleasure_boat_rooms.h"
#include "gfl/random.h"
#include "nitro/rtc.h"

// Who is in the Royal Unova's rooms. The ROM has no string for the file; the name is a guess from what it does. The
// cruise has 15 rooms. Trainers fill as many as the day of the week allows, two hold a pair of people and the rest hold
// one person each. First the order of the rooms is drawn, with one of the six last rooms and one that suits its side
// first, then the people are drawn for the rooms in that order

#define TRAINER_TABLE_SIZE 10
#define PAIR_TABLE_SIZE 5
#define PERSON_TABLE_SIZE 15

// The kinds of people in a room, PleasureBoatRoom's kind
enum {
    ROOM_TRAINER,
    ROOM_PERSON,
    ROOM_PAIR,
};

// The rooms that are left to place and the order they were placed in, with the draws for each kind of room
typedef struct {
    // The rooms not placed yet. The last one at poolLast takes the place of a room that is placed
    u8 pool[PLEASURE_BOAT_ROOM_COUNT];
    u8 order[PLEASURE_BOAT_ROOM_COUNT];
    u8 orderCount;
    u8 poolLast;
    u8 trainers[7];
    u8 pairs[2];
    u8 people[10];
} RoomPicker;

// A Trainer who can be aboard
typedef struct {
    u32 objCode;
    u32 unk4;
    u32 values[2];
} TrainerEntry;

// A pair of people or a person who can be aboard
typedef struct {
    u32 objCode;
    u32 values[2];
} PersonEntry;

// Which side a room is on and something else about it (the counts of PleasureBoat_GetInfo)
typedef struct {
    u32 isPort;
    u32 unk4;
} RoomSide;

static void RoomPicker_Reset(RoomPicker *picker);
static void PickRandom(int count, int picks, u8 *scratch, u8 *picked);
static int RoomPicker_Find(const RoomPicker *picker, u32 room);
static int RoomPicker_Get(const RoomPicker *picker, int index);
static BOOL RoomPicker_Remove(RoomPicker *picker, int index);
static BOOL RoomPicker_PickOrder(RoomPicker *picker);

static const u8 sFirstRooms[6] = { 9, 10, 11, 12, 13, 14 };

static const u8 sStarboardRooms[7] = { 4, 5, 6, 7, 8, 13, 14 };

// Nothing reads it. Declared here, it gives the original's order of .rodata
static const u8 sUnused[7] = { 4, 3, 4, 5, 4, 6, 7 };

static const u8 sPortRooms[8] = { 0, 1, 2, 3, 9, 10, 11, 12 };

static const PersonEntry sPairs[PAIR_TABLE_SIZE] = {
    { 52, { 13, 14 } }, { 15, { 30, 31 } }, { 44, { 43, 44 } }, { 45, { 48, 49 } }, { 20, { 53, 54 } },
};

static const RoomSide sRoomSides[PLEASURE_BOAT_ROOM_COUNT] = {
    { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 1, 0 }, { 1, 0 }, { 1, 0 }, { 1, 0 },
    { 1, 0 }, { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 1 }, { 1, 1 }, { 1, 1 },
};

static const TrainerEntry sTrainers[TRAINER_TABLE_SIZE] = {
    { 52, 19, { 15, 16 } }, { 30, 32, { 19, 20 } }, { 31, 31, { 23, 24 } }, { 11, 16, { 27, 28 } },
    { 15, 33, { 32, 33 } }, { 13, 34, { 36, 37 } }, { 17, 35, { 40, 41 } }, { 44, 37, { 45, 46 } },
    { 45, 36, { 50, 51 } }, { 65, 141, { 55, 56 } },
};

// The same, once the flag of PleasureBoat_Create is set
static const TrainerEntry sTrainersAlt[TRAINER_TABLE_SIZE] = {
    { 52, 623, { 15, 16 } }, { 30, 625, { 19, 20 } }, { 31, 624, { 23, 24 } }, { 11, 622, { 27, 28 } },
    { 15, 626, { 32, 33 } }, { 13, 627, { 36, 37 } }, { 17, 628, { 40, 41 } }, { 44, 630, { 45, 46 } },
    { 45, 629, { 50, 51 } }, { 65, 631, { 55, 56 } },
};

static const PersonEntry sPeople[PERSON_TABLE_SIZE] = {
    { 52, { 12, 12 } }, { 30, { 17, 17 } }, { 30, { 18, 18 } }, { 31, { 21, 21 } }, { 31, { 22, 22 } },
    { 11, { 25, 25 } }, { 11, { 26, 26 } }, { 15, { 29, 29 } }, { 13, { 34, 34 } }, { 13, { 35, 35 } },
    { 17, { 38, 38 } }, { 17, { 39, 39 } }, { 44, { 42, 42 } }, { 45, { 47, 47 } }, { 20, { 52, 52 } },
};

void PleasureBoat_SetUpRooms(PleasureBoat *boat, BOOL flag) {
    struct {
        u8 pairScratch[5];
        u8 peopleScratch[15];
        u8 trainerScratch[10];
        RoomPicker picker;
    } draw;
    RTCDate date;
    int trainerCount;
    int slot;
    u8 i;
    u8 j;

    RTC_GetCachedDate(&date);
    switch (date.week) {
    case RTC_WEEK_SUNDAY:
        trainerCount = 7;
        break;
    case RTC_WEEK_MONDAY:
    case RTC_WEEK_WEDNESDAY:
    case RTC_WEEK_FRIDAY:
        trainerCount = 4;
        break;
    case RTC_WEEK_TUESDAY:
        trainerCount = 3;
        break;
    case RTC_WEEK_THURSDAY:
        trainerCount = 5;
        break;
    case RTC_WEEK_SATURDAY:
        trainerCount = 6;
        break;
    default:
        trainerCount = 0;
        break;
    }
    boat->trainerCount = trainerCount;

    RoomPicker_Reset(&draw.picker);
    if (!RoomPicker_PickOrder(&draw.picker)) {
        RoomPicker_Reset(&draw.picker);
    }
    PickRandom(10, trainerCount, draw.trainerScratch, draw.picker.trainers);
    PickRandom(5, 2, draw.pairScratch, draw.picker.pairs);
    PickRandom(15, 15 - (trainerCount + 2), draw.peopleScratch, draw.picker.people);

    for (i = 0; i < trainerCount; i++) {
        const RoomSide *side = &sRoomSides[draw.picker.order[i]];

        if (side->isPort == 0) {
            boat->infoCounts[0]++;
        } else {
            boat->infoCounts[1]++;
        }
        if (side->unk4 == 0) {
            boat->infoCounts[2]++;
        } else {
            boat->infoCounts[3]++;
        }
    }

    slot = 0;
    for (i = 0; i < trainerCount; i++) {
        PleasureBoatRoom *room = &boat->rooms[draw.picker.order[slot]];
        u32 trainer = draw.picker.trainers[i];
        const TrainerEntry *table;

        room->index = trainer;
        room->kind = ROOM_TRAINER;
        slot++;
        if (flag == 1) {
            table = sTrainers;
        } else {
            table = sTrainersAlt;
        }
        room->unk4 = table[trainer].unk4;
        room->objCode = table[trainer].objCode;
        room->values[0] = table[trainer].values[0];
        room->values[1] = table[trainer].values[1];
    }
    for (i = 0; i < 2; i++) {
        PleasureBoatRoom *room = &boat->rooms[draw.picker.order[slot]];
        const PersonEntry *entry = &sPairs[draw.picker.pairs[i]];

        room->index = i;
        room->unk4 = 0;
        room->objCode = entry->objCode;
        room->kind = ROOM_PAIR;
        slot++;
        room->values[0] = entry->values[0];
        room->values[1] = entry->values[1];
    }
    for (j = 0; j < 15 - (trainerCount + 2); j++) {
        PleasureBoatRoom *room = &boat->rooms[draw.picker.order[slot]];
        const PersonEntry *entry = &sPeople[draw.picker.people[j]];

        room->index = j;
        room->unk4 = 0;
        room->objCode = entry->objCode;
        room->kind = ROOM_PERSON;
        slot++;
        room->values[0] = entry->values[0];
        room->values[1] = entry->values[1];
    }
}

// Picks picks of the numbers below count, none twice
static void PickRandom(int count, int picks, u8 *scratch, u8 *picked) {
    u8 i;
    u8 n;

    for (i = 0; i < count; i++) {
        scratch[i] = i;
    }
    for (n = 0; n < picks; n++) {
        u8 index = GFL_RandomLC(count);

        picked[n] = scratch[index];
        scratch[index] = scratch[count - 1];
        count--;
    }
}

static void RoomPicker_Reset(RoomPicker *picker) {
    int i;

    picker->poolLast = 14;
    picker->orderCount = 0;
    for (i = 0; i < PLEASURE_BOAT_ROOM_COUNT; i++) {
        picker->pool[i] = i;
        picker->order[i] = 0;
    }
    for (i = 0; i < 7; i++) {
        picker->trainers[i] = i;
    }
    for (i = 0; i < 2; i++) {
        picker->pairs[i] = i;
    }
    for (i = 0; i < 10; i++) {
        picker->people[i] = i;
    }
}

static int RoomPicker_Find(const RoomPicker *picker, u32 room) {
    int i;

    for (i = 0; i < PLEASURE_BOAT_ROOM_COUNT; i++) {
        if (room == picker->pool[i]) {
            return i;
        }
    }
    return -1;
}

static int RoomPicker_Get(const RoomPicker *picker, int index) {
    if (index > picker->poolLast) {
        return -1;
    }
    return picker->pool[index];
}

static BOOL RoomPicker_Remove(RoomPicker *picker, int index) {
    if (index > picker->poolLast) {
        return FALSE;
    }
    if (index < 0) {
        return FALSE;
    }
    picker->pool[index] = picker->pool[picker->poolLast];
    if (picker->poolLast != 0) {
        picker->poolLast--;
    }
    return TRUE;
}

static BOOL RoomPicker_PickOrder(RoomPicker *picker) {
    BOOL starboard = FALSE;
    BOOL port = FALSE;
    u32 roll;
    u8 room;
    u8 second;
    int left = PLEASURE_BOAT_ROOM_COUNT;
    int i;

    roll = GFL_RandomLC(6);
    room = sFirstRooms[roll];
    if (!RoomPicker_Remove(picker, RoomPicker_Find(picker, room))) {
        return FALSE;
    }
    picker->order[picker->orderCount] = room;
    picker->orderCount++;
    left--;
    if (sRoomSides[roll].isPort == 0) {
        starboard = TRUE;
    } else {
        port = TRUE;
    }
    if (!port || !starboard) {
        if (port == 0) {
            second = sStarboardRooms[GFL_RandomLC(7)];
        } else {
            second = sPortRooms[GFL_RandomLC(8)];
        }
        if (!RoomPicker_Remove(picker, RoomPicker_Find(picker, second))) {
            return FALSE;
        }
        picker->order[picker->orderCount] = second;
        picker->orderCount++;
        left--;
    }
    for (i = 0; i < left; i++) {
        int index = GFL_RandomLC(left - i);
        int value = RoomPicker_Get(picker, index);

        if (value < 0) {
            return FALSE;
        }
        if (!RoomPicker_Remove(picker, index)) {
            return FALSE;
        }
        picker->order[picker->orderCount] = value;
        picker->orderCount++;
    }
    return TRUE;
}
