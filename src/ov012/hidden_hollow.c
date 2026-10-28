// The Hidden Grottoes: where their entrances are, and the time of day a grotto's gimmick shows. The name is
// descriptive; GetHiddenHollowEntranceParam is swan's name (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
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

// The gimmick's state
typedef struct {
    u8 unk00[0x50];
    // 0 by day, 2 in the evening and 1 at night
    u32 period;
} HiddenHollowGimmick;

// clang-format off
static const HiddenHollowEntrance HIDDEN_HOLLOW_ENTRANCES[20] = {
    {0x1bd, 59, 2, 12},
    {0x149, 373, 0, 425},
    {0x181, 22, 0, 10},
    {0x14b, 133, 0, 392},
    {0x14b, 153, 0, 360},
    {0x151, 106, 0, 258},
    {0x172, 677, 0, 202},
    {0x172, 699, 0, 174},
    {0x15c, 344, 0, 168},
    {0xe8, 28, 0, 59},
    {0x1da, 747, 0, 132},
    {0x1db, 645, 5, 136},
    {0x178, 47, 3, 30},
    {0x178, 35, 5, 10},
    {0x9b, 58, 0, 52},
    {0x9a, 628, 5, 619},
    {0x141, 728, 0, 582},
    {0x141, 703, 0, 576},
    {0x13f, 751, 0, 634},
    {0x183, 620, 0, 741},
};

// clang-format on

u32 GetHiddenHollowEntranceParam(u8 hollow, u32 param) {
    switch (param) {
    case 0:
        return HIDDEN_HOLLOW_ENTRANCES[hollow].zoneId;
    case 1:
        return HIDDEN_HOLLOW_ENTRANCES[hollow].x;
    case 2:
        return HIDDEN_HOLLOW_ENTRANCES[hollow].y;
    case 3:
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
        period = 2;
        MedalBox_GiveMedal(SaveControl_GetMedalBox(GameData_GetSaveControl(gameData)), MEDAL_HIDDEN_HOLLOW_EVENING);
    } else if (hour >= 6 && hour <= 14) {
        period = 0;
    } else {
        period = 1;
    }
    gimmick->period = period;
    return FALSE;
}
