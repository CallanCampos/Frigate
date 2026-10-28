#ifndef POKEBW2_FIELD_SCRCMD_SOUND_H
#define POKEBW2_FIELD_SCRCMD_SOUND_H

// Overlay 36's scrcmd_sound.c: the script commands of sound, the BGM, sound effects, fanfares, cries and the ISS
// switches. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field_script.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Sound_BGMPlay(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_BGMPlayEx(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_BGMFadeOutAll(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_BGMFadeOut(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_BGMIsPlaying(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_BGMWait(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_FadeBgmVolume(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_RestoreBgmVolume(VM *vm, FieldScriptEnv *env);
BOOL ScriptWork_RestoreBgmVolumeEvent(FinishScriptSubEventsWork *work, u32 *state);
BOOL Scrcmd_Sound_BGMChangeMap(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_BGMChangeMapEx(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_BGMPlayPush(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_BGMPush(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_BGMPop(VM *vm, FieldScriptEnv *env);
BOOL FieldScriptSubEventFinish_MEPlayback(FinishScriptSubEventsWork *work, u32 *state);
BOOL Scrcmd_Sound_BGMAmbienceResume(VM *vm, FieldScriptEnv *env);
BOOL FieldScriptSubEventFinish_BGMPlayback(FinishScriptSubEventsWork *work, u32 *state);
BOOL Scrcmd_Sound_SEPlay(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_SEStop(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_SEWait(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_MEPlay(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_MEWait(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_PVPlay(VM *vm, FieldScriptEnv *env);
// The same as Scrcmd_Sound_PVPlay, with the save's Chatot recording
BOOL Scrcmd_Sound_PokeVoicePlay(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_PVWait(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_ISSSwitchEnable(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_ISSSwitchDisable(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Sound_ISSSwitchQuery(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_SOUND_H
