#ifndef POKEBW2_FIELD_SCRCMD_PEP_QUIZ_H
#define POKEBW2_FIELD_SCRCMD_PEP_QUIZ_H

// Overlay 36's scrcmd_pep_quiz.c (a descriptive name): the Pep quiz's script command. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field_script.h"
#include "struct_decls.h"

BOOL Scrcmd_PepQuiz_PepQuizGenerate(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_PEP_QUIZ_H
