#include "types.h"
#include "field/field_map.h"
#include "field/gimmick_roller_coaster.h"
#include "field/gimmick_state.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The saved state of gimmick 10, zone 121 in Opelucid City
typedef struct {
    u8 mode;
    u8 step;
    u32 values[5];
} GimmickState10;

void GimmickState_SetNimbasaGym(GameSystem *gsys, u16 value) {
    u16 *state = GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(gsys)), 2);
    *state = value;
}

void GimmickState_ResetOpelucid(GameSystem *gsys) {
    GimmickState10 *state = GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(gsys)), 10);
    int i;

    state->step = 0;
    state->mode = 2;
    for (i = 0; i < 5; i++) {
        state->values[i] = 0;
    }
}

void GimmickState_SetNimbasaFlags(GameSystem *gsys, BOOL car1IntroSeen, BOOL car2IntroSeen) {
    RollerCoasterSave *state = GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(gsys)), 0x30);

    if (car1IntroSeen) {
        state->introSeen[0] = 1;
    }
    if (car2IntroSeen) {
        state->introSeen[1] = 1;
    }
}
