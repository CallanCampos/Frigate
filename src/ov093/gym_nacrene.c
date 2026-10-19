#include "types.h"
#include "field/field.h"
#include "field/field_map.h"
#include "field/gym_nacrene.h"
#include "system/game_data.h"
#include "system/game_system.h"

void GymNacrene_Init(Field *field) {
    Field_AllocGimmickWorkBlock(field, 1, Field_GetHeapID(field), 4);
    Field_GetGimmickWorkBlock(field, 1);
    GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))), 3);
}

void GymNacrene_End(Field *field) {
    Field_DeleteGimmickWorkBlock(field, 1);
}

void GymNacrene_Update(Field *field) {
}
