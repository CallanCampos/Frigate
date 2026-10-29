// The Pep quiz's script command, which picks a question and its answer. The ROM has no name for the file;
// scrcmd_pep_quiz.c is descriptive. It is a file of its own because overlay 36's .rodata has its 62-byte answer list
// after the 176-byte ambient sound table of scrcmd_field.c, which one file's size sort can't give. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_script.h"
#include "field/scrcmd_pep_quiz.h"
#include "gfl/random.h"
#include "save/adventure.h"
#include "save/save_control.h"
#include "system/game_data.h"

BOOL Scrcmd_PepQuiz_PepQuizGenerate(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 hard = ScriptReadAny(vm, env);
    u16 *question = ScriptReadVar(vm, env);
    u16 *answerMsg = ScriptReadVar(vm, env);
    u16 *answer = ScriptReadVar(vm, env);
    u16 answers[31] = {
        0x6a1, 0x5d6, 0x699, 0x5fe, 0x687, 0x5a3, 0x624, 0x6a3, 0x61a, 0x6b5, 0x60b, 0x602, 0x6a0, 0x57d, 0x249, 0x20a,
        0x257, 0x24b, 0x253, 0x208, 0x1ff, 0x213, 0x233, 0x210, 0x22b, 0x252, 0x206, 0x245, 0x26a, 0x2c5, 0x62b,
    };
    AdventureSave *adventure = getSaveAdventureDataBlk(GameData_GetSaveControl(gameData));
    u32 month = GameData_GetMonth(gameData);
    u32 day = GameData_GetDay(gameData);
    u32 birthMonth = getOSBirthdayMonth(adventure);

    if (day == getOSBirthdayDay(adventure) && month == birthMonth) {
        *question = 0x34;
        *answerMsg = 0x53;
        *answer = answers[30];
    } else {
        u32 index = GFL_RandomLC(hard ? 0x1e : 0x16);

        *question = index + 0x16;
        *answerMsg = index + 0x35;
        *answer = answers[index];
    }
    return FALSE;
}
