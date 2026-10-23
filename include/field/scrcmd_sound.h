#ifndef POKEBW2_FIELD_SCRCMD_SOUND_H
#define POKEBW2_FIELD_SCRCMD_SOUND_H

// Overlay 36's scrcmd_sound.c: the script commands of sound, the BGM, sound effects, fanfares, cries and the ISS
// switches. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field_script.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s0098_BGMPlay(VM *vm, FieldScriptEnv *env);
BOOL s0235_BGMPlayEx(VM *vm, FieldScriptEnv *env);
BOOL s0246_BGMFadeOutAll(VM *vm, FieldScriptEnv *env);
BOOL s0245_BGMFadeOut(VM *vm, FieldScriptEnv *env);
BOOL s009B_BGMIsPlaying(VM *vm, FieldScriptEnv *env);
BOOL s00A0_BGMWait(VM *vm, FieldScriptEnv *env);
BOOL ScriptWork_FadeBgmVolume(VM *vm, FieldScriptEnv *env);
BOOL ScriptWork_RestoreBgmVolume(VM *vm, FieldScriptEnv *env);
BOOL ScriptWork_RestoreBgmVolumeEvent(FinishScriptSubEventsWork *work, u32 *state);
BOOL s009E_BGMChangeMap(VM *vm, FieldScriptEnv *env);
BOOL s0238_BGMChangeMapEx(VM *vm, FieldScriptEnv *env);
BOOL s009F_BGMPlayPush(VM *vm, FieldScriptEnv *env);
BOOL s00A1_BGMPush(VM *vm, FieldScriptEnv *env);
BOOL s00A2_BGMPop(VM *vm, FieldScriptEnv *env);
BOOL FieldScriptSubEventFinish_MEPlayback(FinishScriptSubEventsWork *work, u32 *state);
BOOL s024C_BGMAmbienceResume(VM *vm, FieldScriptEnv *env);
BOOL FieldScriptSubEventFinish_BGMPlayback(FinishScriptSubEventsWork *work, u32 *state);
BOOL s00A6_SEPlay(VM *vm, FieldScriptEnv *env);
BOOL s00A7_SEStop(VM *vm, FieldScriptEnv *env);
BOOL s00A8_SEWait(VM *vm, FieldScriptEnv *env);
BOOL s00A9_MEPlay(VM *vm, FieldScriptEnv *env);
BOOL s00AA_MEWait(VM *vm, FieldScriptEnv *env);
BOOL s00AB_PVPlay(VM *vm, FieldScriptEnv *env);
// The same as s00AB_PVPlay, with the save's Chatot recording
BOOL ScriptWork_PlayPerapVoice(VM *vm, FieldScriptEnv *env);
BOOL s00AC_PVWait(VM *vm, FieldScriptEnv *env);
BOOL s00A3_ISSSwitchEnable(VM *vm, FieldScriptEnv *env);
BOOL s00A4_ISSSwitchDisable(VM *vm, FieldScriptEnv *env);
BOOL s00A5_ISSSwitchQuery(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_SOUND_H
