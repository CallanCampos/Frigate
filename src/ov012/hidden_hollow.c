// The Hidden Grottoes: where their entrances are, and the time of day a grotto's gimmick shows. The name is
// descriptive; GetHiddenHollowEntranceParam is swan's name (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/zones.h"
#include "field/field_map.h"
#include "field/field_script.h"
#include "field/hidden_hollow.h"
#include "save/medal_box.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

#define GIMMICK_HIDDEN_HOLLOW 0x32
#define MEDAL_HIDDEN_HOLLOW_EVENING 0x61

typedef struct {
    u16 zoneId;
    u32 x;
    u32 y;
    u32 z;
} HiddenHollowEntrance;

// The time of day a grotto's gimmick shows, by the hour
enum {
    HIDDEN_HOLLOW_PERIOD_DAY,
    HIDDEN_HOLLOW_PERIOD_NIGHT,
    HIDDEN_HOLLOW_PERIOD_EVENING,
};

// The gimmick's state
typedef struct {
    u8 unk00[0x50];
    // HIDDEN_HOLLOW_PERIOD_*
    u32 period;
} HiddenHollowGimmick;

// clang-format off
static const HiddenHollowEntrance HIDDEN_HOLLOW_ENTRANCES[20] = {
    {ZONE_FLOCCESY_RANCH_2, 59, 2, 12},
    {ZONE_ROUTE_5, 373, 0, 425},
    {ZONE_LOSTLORN_FOREST, 22, 0, 10},
    {ZONE_ROUTE_6, 133, 0, 392},
    {ZONE_ROUTE_6, 153, 0, 360},
    {ZONE_ROUTE_7, 106, 0, 258},
    {ZONE_ROUTE_13, 677, 0, 202},
    {ZONE_ROUTE_13, 699, 0, 174},
    {ZONE_ROUTE_9, 344, 0, 168},
    {ZONE_GIANT_CHASM_3, 28, 0, 59},
    {ZONE_ROUTE_22, 747, 0, 132},
    {ZONE_ROUTE_23, 645, 5, 136},
    {ZONE_ABUNDANT_SHRINE, 47, 3, 30},
    {ZONE_ABUNDANT_SHRINE, 35, 5, 10},
    {ZONE_PINWHEEL_FOREST_2, 58, 0, 52},
    {ZONE_PINWHEEL_FOREST, 628, 5, 619},
    {ZONE_ROUTE_3, 728, 0, 582},
    {ZONE_ROUTE_3, 703, 0, 576},
    {ZONE_ROUTE_2, 751, 0, 634},
    {ZONE_ROUTE_18, 620, 0, 741},
};

// clang-format on

u32 GetHiddenHollowEntranceParam(u8 hollow, u32 param) {
    switch (param) {
    case HIDDEN_HOLLOW_ENTRANCE_ZONE:
        return HIDDEN_HOLLOW_ENTRANCES[hollow].zoneId;
    case HIDDEN_HOLLOW_ENTRANCE_X:
        return HIDDEN_HOLLOW_ENTRANCES[hollow].x;
    case HIDDEN_HOLLOW_ENTRANCE_Y:
        return HIDDEN_HOLLOW_ENTRANCES[hollow].y;
    case HIDDEN_HOLLOW_ENTRANCE_Z:
        return HIDDEN_HOLLOW_ENTRANCES[hollow].z;
    }
    return 0;
}

BOOL Scrcmd_HiddenHollow_HiddenHollowSetPeriod(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    HiddenHollowGimmick *gimmick = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_HIDDEN_HOLLOW);
    int hour = getCurrentHour(gameData);
    u32 period;

    if (hour >= 15 && hour <= 16) {
        period = HIDDEN_HOLLOW_PERIOD_EVENING;
        MedalBox_GiveMedal(SaveControl_GetMedalBox(GameData_GetSaveControl(gameData)), MEDAL_HIDDEN_HOLLOW_EVENING);
    } else if (hour >= 6 && hour <= 14) {
        period = HIDDEN_HOLLOW_PERIOD_DAY;
    } else {
        period = HIDDEN_HOLLOW_PERIOD_NIGHT;
    }
    gimmick->period = period;
    return FALSE;
}
