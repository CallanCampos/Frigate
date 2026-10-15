#include "types.h"
#include "field/event_abyssal_ruins.h"
#include "field/field.h"
#include "field/field_script_event.h"
#include "save/save_control.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The script that plays at each count of steps
static const struct {
    u16 stepCount;
    u16 scriptId;
} ABYSSAL_RUINS_DULL_SOUND_SCRIPTS[5] = {
    { 0, 0x28e7 }, { 100, 0x28e8 }, { 300, 0x28e9 }, { 450, 0x28ea }, { 500, 0x28eb },
};

GameEvent *CheckAbyssalRuinsStepEvent(GameSystem *gsys, Field *field) {
    GameData *gameData;
    SaveControl *save;
    PlayerSave *playerSave;
    u16 stepCount;
    u32 i;

    gameData = GSYS_GetGameData(gsys);
    save = GameData_GetSaveControl(gameData);
    playerSave = SaveControl_GetPlayerSave(save);
    stepCount = PlayerSave_GetAbyssalRuinsStepCounter(playerSave);
    for (i = 0; i < 5; i++) {
        if (stepCount == ABYSSAL_RUINS_DULL_SOUND_SCRIPTS[i].stepCount) {
            return EventScriptCall_Create(gsys, ABYSSAL_RUINS_DULL_SOUND_SCRIPTS[i].scriptId, NULL,
                                          Field_GetHeapID(field));
        }
    }
    if (stepCount > 500) {
        return EventScriptCall_Create(gsys, 0x28eb, NULL, Field_GetHeapID(field));
    }
    return NULL;
}
