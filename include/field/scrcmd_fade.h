#ifndef POKEBW2_FIELD_SCRCMD_FADE_H
#define POKEBW2_FIELD_SCRCMD_FADE_H

// Overlay 36's scrcmd_fade.c: the script commands that fade the screens. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except our descriptive ones of commands 0x2E8 and 0x2E9

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s00B3_FadeEx(VM *vm, FieldScriptEnv *env);
BOOL s01AA_CallSeasonBanner(VM *vm, FieldScriptEnv *env);
BOOL s01A8_FadeInBlackQ_(VM *vm, FieldScriptEnv *env);
BOOL s01A3_FadeInBlackQ(VM *vm, FieldScriptEnv *env);
BOOL s01A4_FadeOutBlackQ(VM *vm, FieldScriptEnv *env);
BOOL s01A5_FadeInWhiteQ(VM *vm, FieldScriptEnv *env);
BOOL s01A6_FadeOutWhiteQ(VM *vm, FieldScriptEnv *env);
BOOL s01AB_FadeInBlack(VM *vm, FieldScriptEnv *env);
BOOL s01AC_FadeOutBlack(VM *vm, FieldScriptEnv *env);
BOOL s01AD_FadeInWhite(VM *vm, FieldScriptEnv *env);
BOOL s01AE_FadeOutWhite(VM *vm, FieldScriptEnv *env);
BOOL s00B4_FadeExWait(VM *vm, FieldScriptEnv *env);
BOOL s01A7_FadeWait(VM *vm, FieldScriptEnv *env);
BOOL s02E8_CallFieldFadeOut(VM *vm, FieldScriptEnv *env);
BOOL s02E9_CallFieldFadeIn(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_FADE_H
