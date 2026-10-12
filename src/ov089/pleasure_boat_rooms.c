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

// How many Trainers are aboard by weekday, Monday first
static const u8 sTrainerCounts[7] = { 4, 3, 4, 5, 4, 6, 7 };

static const u8 sFirstRooms[6] = { 9, 10, 11, 12, 13, 14 };

static const u8 sStarboardRooms[7] = { 4, 5, 6, 7, 8, 13, 14 };

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
    RTCDate date;
    RoomPicker picker;
    u8 trainerScratch[TRAINER_TABLE_SIZE];
    u8 peopleScratch[PERSON_TABLE_SIZE];
    u8 pairScratch[PAIR_TABLE_SIZE];
    int trainerCount;
    int slot;
    u8 i;
    u8 j;

    RTC_GetCachedDate(&date);
    switch (date.week) {
    case RTC_WEEK_MONDAY:
        trainerCount = sTrainerCounts[0];
        break;
    case RTC_WEEK_TUESDAY:
        trainerCount = sTrainerCounts[1];
        break;
    case RTC_WEEK_WEDNESDAY:
        trainerCount = sTrainerCounts[2];
        break;
    case RTC_WEEK_THURSDAY:
        trainerCount = sTrainerCounts[3];
        break;
    case RTC_WEEK_FRIDAY:
        trainerCount = sTrainerCounts[4];
        break;
    case RTC_WEEK_SATURDAY:
        trainerCount = sTrainerCounts[5];
        break;
    case RTC_WEEK_SUNDAY:
        trainerCount = sTrainerCounts[6];
        break;
    default:
        trainerCount = 0;
        break;
    }
    boat->trainerCount = trainerCount;

    RoomPicker_Reset(&picker);
    if (!RoomPicker_PickOrder(&picker)) {
        RoomPicker_Reset(&picker);
    }
    PickRandom(NELEMS(sTrainers), trainerCount, trainerScratch, picker.trainers);
    PickRandom(NELEMS(sPairs), 2, pairScratch, picker.pairs);
    PickRandom(NELEMS(sPeople), NELEMS(sPeople) - (trainerCount + 2), peopleScratch, picker.people);

    for (i = 0; i < trainerCount; i++) {
        const RoomSide *side = &sRoomSides[picker.order[i]];

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
        u32 room = picker.order[slot];
        u32 trainer = picker.trainers[i];

        boat->rooms[room].index = trainer;
        boat->rooms[room].kind = ROOM_TRAINER;
        slot++;
        if (flag == 1) {
            boat->rooms[room].unk4 = sTrainers[trainer].unk4;
            boat->rooms[room].objCode = sTrainers[trainer].objCode;
            boat->rooms[room].values[0] = sTrainers[trainer].values[0];
            boat->rooms[room].values[1] = sTrainers[trainer].values[1];
        } else {
            boat->rooms[room].unk4 = sTrainersAlt[trainer].unk4;
            boat->rooms[room].objCode = sTrainersAlt[trainer].objCode;
            boat->rooms[room].values[0] = sTrainersAlt[trainer].values[0];
            boat->rooms[room].values[1] = sTrainersAlt[trainer].values[1];
        }
    }
    for (i = 0; i < 2; i++) {
        u32 room = picker.order[slot];
        u32 person = picker.pairs[i];

        boat->rooms[room].index = i;
        boat->rooms[room].unk4 = 0;
        boat->rooms[room].objCode = sPairs[person].objCode;
        boat->rooms[room].kind = ROOM_PAIR;
        slot++;
        boat->rooms[room].values[0] = sPairs[person].values[0];
        boat->rooms[room].values[1] = sPairs[person].values[1];
    }
    for (j = 0; j < PLEASURE_BOAT_ROOM_COUNT - (trainerCount + 2); j++) {
        u32 room = picker.order[slot];
        u32 person = picker.people[j];

        boat->rooms[room].index = j;
        boat->rooms[room].unk4 = 0;
        boat->rooms[room].objCode = sPeople[person].objCode;
        boat->rooms[room].kind = ROOM_PERSON;
        slot++;
        boat->rooms[room].values[0] = sPeople[person].values[0];
        boat->rooms[room].values[1] = sPeople[person].values[1];
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
