#include "types.h"
#include "field/field.h"
#include "field/field_map.h"
#include "field/gym_striaton.h"
#include "system/game_data.h"
#include "system/game_system.h"

void GymStriaton_Init(Field *field) {
    GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), 4);
    Field_AllocGimmickWorkBlock(field, 1, Field_GetHeapID(field), 4);
}

void GymStriaton_End(Field *field) {
    Field_DeleteGimmickWorkBlock(field, 1);
}

void GymStriaton_Update(Field *field) {
}
