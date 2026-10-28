#ifndef POKEBW2_FIELD_SCRCMD_FADE_H
#define POKEBW2_FIELD_SCRCMD_FADE_H

// Overlay 36's scrcmd_fade.c: the script commands that fade the screens. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except our descriptive ones of commands 0x2E8 and 0x2E9

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Fade_FadeEx(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_CallSeasonBanner(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_FadeInBlackQ_01A8(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_FadeInBlackQ(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_FadeOutBlackQ(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_FadeInWhiteQ(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_FadeOutWhiteQ(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_FadeInBlack(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_FadeOutBlack(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_FadeInWhite(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_FadeOutWhite(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_FadeExWait(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_FadeWait(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_CallFieldFadeOut(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Fade_CallFieldFadeIn(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_FADE_H
