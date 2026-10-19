#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_COMMON_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_COMMON_H

#include "types.h"

// Shared constants of the character-call Xtransceiver. Names are descriptive.

#define HEAPID_LOCAL_TVT 85

// At most this many faces are on the upper screen
#define LOCAL_TVT_FACES 4

// The persons of a call. The player is LOCAL_TVT_PERSON_PLAYER_FEMALE when female
#define LOCAL_TVT_PERSON_PLAYER 0
#define LOCAL_TVT_PERSON_RIVAL 1
#define LOCAL_TVT_PERSON_PLAYER_FEMALE 13
#define LOCAL_TVT_PERSON_COUNT 14
#define LOCAL_TVT_PERSON_NONE 0xff

// A face has no object beside it
#define LOCAL_TVT_OBJECT_NONE 3
// The name of a face is the person's own
#define LOCAL_TVT_NAME_DEFAULT 0xff

// The part of the day, which picks the backgrounds of the faces
enum {
    LOCAL_TVT_TIME_DAY,
    LOCAL_TVT_TIME_EVENING,
    LOCAL_TVT_TIME_NIGHT,
    LOCAL_TVT_TIME_COUNT,
};

// The persons the player can call, which the save data of the calls counts
enum {
    LOCAL_TVT_CALL_DOCTOR,
    LOCAL_TVT_CALL_RIVAL,
    LOCAL_TVT_CALL_CHELEN,
    LOCAL_TVT_CALL_SUPPORT,
    LOCAL_TVT_CALL_MOTHER,
    LOCAL_TVT_CALL_SECRET,
    LOCAL_TVT_CALL_COUNT,
};

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_COMMON_H
