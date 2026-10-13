#include "types.h"
#include "field/unity_tower.h"
#include "save/player_info.h"

// Records the partners of trades in the Unity Tower's survey, a list of the latest 20 with at most 5 from one country.
// The file's name and the functions' are ours

// The visitors from one country that the list keeps
#define VISITORS_PER_COUNTRY_MAX 5

#define REGISTER_EXISTING 1
#define REGISTER_NEW 2

static u32 UnityTowerSurvey_RegisterVisitor(UnityTowerSurveySave *save, UnityTowerVisitor *visitor);
static u32 UnityTowerVisitors_Count(UnityTowerVisitor *visitors);
static u32 UnityTowerVisitors_CountCountry(UnityTowerVisitor *visitors, u32 country);
static u32 UnityTowerVisitors_FindCountry(UnityTowerVisitor *visitors, u32 country);
static u32 UnityTowerVisitors_Find(UnityTowerVisitor *visitors, UnityTowerVisitor *visitor);
static void UnityTowerVisitors_Remove(UnityTowerVisitor *visitors, unsigned long index);
static void UnityTowerSurvey_AddVisitor(UnityTowerSurveySave *save, UnityTowerVisitor *visitor, BOOL scriptFlag);
static void UnityTowerVisitors_Add(UnityTowerVisitor *visitors, UnityTowerVisitor *visitor, BOOL scriptFlag);

u32 UnityTowerSurvey_RegisterTrade(UnityTowerSurveySave *save, UnityTowerVisitor *visitor) {
    if (UnityTowerVisitor_GetCountry(&visitor->info) == 0) {
        return 0;
    }
    return UnityTowerSurvey_RegisterVisitor(save, visitor);
}

static u32 UnityTowerSurvey_RegisterVisitor(UnityTowerSurveySave *save, UnityTowerVisitor *visitor) {
    UnityTowerVisitor *visitors = func_02009e68(save);
    u32 index = UnityTowerVisitors_Find(visitors, visitor);
    u32 country;

    if (index != UNITY_TOWER_VISITOR_MAX) {
        BOOL scriptFlag = visitors[index].scriptFlag ? TRUE : FALSE;

        UnityTowerVisitors_Remove(visitors, index);
        UnityTowerSurvey_AddVisitor(save, visitor, scriptFlag);
        return REGISTER_EXISTING;
    }
    country = UnityTowerVisitor_GetCountry(&visitor->info);
    if (UnityTowerVisitors_CountCountry(visitors, country) < VISITORS_PER_COUNTRY_MAX) {
        if (UnityTowerVisitors_Count(visitors) >= UNITY_TOWER_VISITOR_MAX) {
            UnityTowerVisitors_Remove(visitors, 0);
        }
        UnityTowerSurvey_AddVisitor(save, visitor, FALSE);
    } else {
        UnityTowerVisitors_Remove(visitors, UnityTowerVisitors_FindCountry(visitors, country));
        UnityTowerSurvey_AddVisitor(save, visitor, FALSE);
    }
    return REGISTER_NEW;
}

static u32 UnityTowerVisitors_Count(UnityTowerVisitor *visitors) {
    u32 i;

    for (i = 0; i < UNITY_TOWER_VISITOR_MAX; i++) {
        if (!visitors[i].valid) {
            break;
        }
    }
    return i;
}

static u32 UnityTowerVisitors_CountCountry(UnityTowerVisitor *visitors, u32 country) {
    u32 count = 0;
    u32 i;
    u32 visitorCountry;

    for (i = 0; i < UNITY_TOWER_VISITOR_MAX; i++) {
        if (!visitors[i].valid) {
            break;
        }
        visitorCountry = UnityTowerVisitor_GetCountry(&visitors[i].info);
        if (visitorCountry == country) {
            count++;
        }
    }
    if (count > VISITORS_PER_COUNTRY_MAX) {
        count = VISITORS_PER_COUNTRY_MAX;
    }
    return count;
}

static u32 UnityTowerVisitors_FindCountry(UnityTowerVisitor *visitors, u32 country) {
    u32 i;
    u32 visitorCountry;

    for (i = 0; i < UNITY_TOWER_VISITOR_MAX; i++) {
        if (!visitors[i].valid) {
            break;
        }
        visitorCountry = UnityTowerVisitor_GetCountry(&visitors[i].info);
        if (visitorCountry == country) {
            return i;
        }
    }
    return UNITY_TOWER_VISITOR_MAX;
}

static u32 UnityTowerVisitors_Find(UnityTowerVisitor *visitors, UnityTowerVisitor *visitor) {
    u32 id = getIDAsUInt(&visitor->info);
    u32 country = UnityTowerVisitor_GetCountry(&visitor->info);
    u32 visitorId;
    u32 visitorCountry;
    u32 i;

    for (i = 0; i < UNITY_TOWER_VISITOR_MAX; i++) {
        if (!visitors[i].valid) {
            break;
        }
        visitorId = getIDAsUInt(&visitors[i].info);
        if (visitorId == id) {
            visitorCountry = UnityTowerVisitor_GetCountry(&visitors[i].info);
            if (visitorCountry == country) {
                return i;
            }
        }
    }
    return UNITY_TOWER_VISITOR_MAX;
}

// Closes the gap the visitor leaves, and clears the entries left at the end
// The SDK's u32 is unsigned long, which MWCC indexes with differently than unsigned int
static void UnityTowerVisitors_Remove(UnityTowerVisitor *visitors, unsigned long index) {
    if (index >= UNITY_TOWER_VISITOR_MAX) {
        return;
    }
    for (; index < UNITY_TOWER_VISITOR_MAX - 1; index++) {
        visitors[index] = visitors[index + 1];
        if (!visitors[index + 1].valid) {
            index++;
            break;
        }
    }
    for (; index < UNITY_TOWER_VISITOR_MAX; index++) {
        visitors[index].valid = FALSE;
    }
}

static void UnityTowerSurvey_AddVisitor(UnityTowerSurveySave *save, UnityTowerVisitor *visitor, BOOL scriptFlag) {
    UnityTowerVisitors_Add(func_02009e68(save), visitor, scriptFlag);
    if (func_02009e6c(save, UnityTowerVisitor_GetCountry(&visitor->info))) {
        func_02009d04(save);
    }
}

static void UnityTowerVisitors_Add(UnityTowerVisitor *visitors, UnityTowerVisitor *visitor, BOOL scriptFlag) {
    u32 index = UnityTowerVisitors_Count(visitors);

    if (index < UNITY_TOWER_VISITOR_MAX) {
        if (scriptFlag) {
            visitor->scriptFlag = TRUE;
        } else {
            visitor->scriptFlag = FALSE;
        }
        visitor->valid = TRUE;
        visitors[index] = *visitor;
    }
}
