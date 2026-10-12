#ifndef POKEBW2_APP_UN_SELECT_H
#define POKEBW2_APP_UN_SELECT_H

#include "types.h"
#include "field/unity_tower.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// The Unity Tower's floor select, overlay 311: un_select.c, and un_select_graphic.c (app/un_select_graphic.h), which
// the ROM names. Each floor of the tower is a country; the floors of the countries whose visitors the player has met
// can be chosen, and those of the latest visitors are marked. The names are ours
#define OVERLAY_UN_SELECT OVERLAY_ID(311)

// The countries of the survey (field/unity_tower.h has the number of visitors it lists)
#define UNITY_TOWER_COUNTRY_MAX 232

typedef struct {
    // The floor the player is on
    u8 floor;
    // What the player chose: the floor and its country, or 0 for none
    u8 selectedFloor;
    u16 selectedCountry;
    BOOL selected;
    // The countries of the latest visitors, 0 past the last
    u16 visitorCountries[UNITY_TOWER_VISITOR_MAX];
    // For each country from 1, whether a visitor from it has come
    u8 countryVisited[UNITY_TOWER_COUNTRY_MAX];
} UnSelectParam;

extern const GameProcFunctions UN_SELECT_PROC_FUNCTIONS;

#endif // POKEBW2_APP_UN_SELECT_H
