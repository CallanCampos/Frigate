// The script commands of sound: the BGM and its volume, the sound effects, the fanfares (ME), the Pokémon cries and
// the ISS switches. The ROM has no name for the file; scrcmd_sound.c is descriptive. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/field_sound.h"
#include "field/scrcmd_sound.h"
#include "gfl/sound.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/iss_switch_sys.h"
#include "system/iss_sys.h"
#include "system/vm.h"

// The players of the sound effects that ScriptWork_GetSEBitMask tracks
#define SE_PLAYER_COUNT 5

// The work of the event that plays a cry
typedef struct {
    u16 species;
    u16 form;
    // Plays the save's Chatot recording in place of Chatot's cry
    BOOL chatter;
    FieldScriptEnv *env;
} PokeVoiceEvent;

static GameEventReturnCode EventPokeVoicePlay_Callback(GameEvent *event, u32 *state, void *data);

// Plays a cry with the save's Chatot recording, which Chatot's cry plays in place of its own. The callback reads the
// species and form before it makes the recording's info, as the arguments of a call
static inline u32 playChatterVoice(u32 species, u32 form) {
    PokeVoiceChatterInfo chatterInfo;

    PokeVoice_CreateChatterInfo(&chatterInfo);
    return PokeVoice_Play(species, form, 64, 0, 0, 0, 0, &chatterInfo);
}

BOOL Scrcmd_Sound_BGMPlay(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, EventBGMPlay_Create(gsys, VM_Read16(vm)));
    FieldSnd_StopAmbienceAll(GameData_GetFieldSoundSystem(GSYS_GetGameData(gsys)), FIELD_SND_AMBIENCE_STOP_SCRIPT_BGM);
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_BGM_PLAY);
    return TRUE;
}

BOOL Scrcmd_Sound_BGMPlayEx(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 bgm = VM_Read16(vm);
    u16 fadeOutFrames = VM_Read16(vm);

    ScriptWork_CallEvent(work, EventBGMPlayEx_Create(gsys, bgm, fadeOutFrames));
    FieldSnd_StopAmbienceAll(GameData_GetFieldSoundSystem(GSYS_GetGameData(gsys)), FIELD_SND_AMBIENCE_STOP_SCRIPT_BGM);
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_BGM_PLAY);
    return TRUE;
}

BOOL Scrcmd_Sound_BGMFadeOutAll(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, CreateBGMFadeOutEvent(gsys, VM_Read16(vm)));
    FieldSnd_StopAmbienceAll(GameData_GetFieldSoundSystem(GSYS_GetGameData(gsys)), FIELD_SND_AMBIENCE_STOP_SCRIPT_BGM);
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_BGM_PLAY);
    return TRUE;
}

BOOL Scrcmd_Sound_BGMFadeOut(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, CreateBGMFadeOutEvent(gsys, VM_Read16(vm)));
    return TRUE;
}

// The BGM it reads is unused: the result is whether any BGM is playing
BOOL Scrcmd_Sound_BGMIsPlaying(VM *vm, FieldScriptEnv *env) {
    VM_Read16(vm);
    u16 *result = ScriptReadVar(vm, env);

    *result = GFL_SndBGMIsPlaying();
    return FALSE;
}

static BOOL isSequenceNotPlaying(VM *vm, void *env) {
    FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetScriptWork(env);

    if (GFL_SndBGMIsPlaying() == FALSE) {
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_Sound_BGMWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, isSequenceNotPlaying);
    return TRUE;
}

// Fades the BGM players' volume
BOOL Scrcmd_Sound_FadeBgmVolume(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    GSYS_GetField(gsys);
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);
    u16 volume = ScriptReadAny(vm, env);
    u16 frames = ScriptReadAny(vm, env);

    // The original narrows the frames to u8, though the fade takes a u16
    FieldSnd_SetPlayerVolumeFade(fieldSound, volume, (u8)frames);
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_BGM_VOLUME);
    return FALSE;
}

// Fades the BGM players' volume back to full
BOOL Scrcmd_Sound_RestoreBgmVolume(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    GSYS_GetField(gsys);
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);
    u16 frames = ScriptReadAny(vm, env);

    FieldSnd_SetPlayerVolumeFade(fieldSound, SND_VOLUME_MAX, (u8)frames);
    FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_BGM_VOLUME);
    return FALSE;
}

// The steps of the sound's sub event finishers: bring the sound back, then end
enum {
    SOUND_FINISH_START,
    SOUND_FINISH_END,
};

// Finishes Scrcmd_Sound_FadeBgmVolume's sub event: the volume goes back to full at once
BOOL ScriptWork_RestoreBgmVolumeEvent(FinishScriptSubEventsWork *work, u32 *state) {
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(GSYS_GetGameData(work->gsys));

    switch (*state) {
    case SOUND_FINISH_START:
        FieldSnd_SetPlayerVolumeFade(fieldSound, SND_VOLUME_MAX, 0);
        (*state)++;
        break;
    case SOUND_FINISH_END:
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_Sound_BGMChangeMap(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u8 season = GameData_GetSeason(gameData);
    u32 bgm = GetMapBGMIDByPlayerState2(gameData, Field_GetPlayerStateZoneID(field), season);

    ScriptWork_CallEvent(work, EventBGMChange_Create(gsys, bgm, 90, 60));
    FieldSnd_ResumeAmbience(fieldSound, FIELD_SND_AMBIENCE_STOP_SCRIPT_BGM);
    FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_BGM_PLAY);
    return TRUE;
}

BOOL Scrcmd_Sound_BGMChangeMapEx(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 fadeOutFrames = VM_Read16(vm);
    u8 season = GameData_GetSeason(gameData);
    u32 bgm = GetMapBGMIDByPlayerState2(gameData, Field_GetPlayerStateZoneID(field), season);

    ScriptWork_CallEvent(work, EventBGMChange_Create(gsys, bgm, fadeOutFrames, 60));
    FieldSnd_ResumeAmbience(fieldSound, FIELD_SND_AMBIENCE_STOP_SCRIPT_BGM);
    FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_BGM_PLAY);
    return TRUE;
}

BOOL Scrcmd_Sound_BGMPlayPush(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetGameData(gsys);
    GSYS_GetField(gsys);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 bgm = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(work, EventBGMPlayPush_Create(gsys, bgm));
    return TRUE;
}

BOOL Scrcmd_Sound_BGMPush(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, EventBGMPushWait_Create(gsys, ScriptReadAny(vm, env)));
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_BGM_PUSH);
    return TRUE;
}

BOOL Scrcmd_Sound_BGMPop(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 fadeOutFrames = ScriptReadAny(vm, env);
    u16 fadeInFrames = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(work, EventPushBGMFinish_Create(gsys, fadeOutFrames, fadeInFrames));
    FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_BGM_PUSH);
    return TRUE;
}

// Finishes Scrcmd_Sound_BGMPush's sub event: the pushed BGM comes back
BOOL FieldScriptSubEventFinish_MEPlayback(FinishScriptSubEventsWork *work, u32 *state) {
    ScriptWork *scriptWork = work->scriptWork;
    ScriptWork_GetFieldWork(scriptWork);

    switch (*state) {
    case SOUND_FINISH_START:
        ScriptWork_CallEvent(scriptWork, EventPushBGMFinish_Create(work->gsys, 0, 0));
        (*state)++;
        break;
    case SOUND_FINISH_END:
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_Sound_BGMAmbienceResume(VM *vm, FieldScriptEnv *env) {
    FieldSnd_ResumeAmbience(GameData_GetFieldSoundSystem(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))),
                            FIELD_SND_AMBIENCE_STOP_SCRIPT_BGM);
    FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_BGM_PLAY);
    return FALSE;
}

// Finishes the sub event of the commands that play the BGM: the ambient sounds come back
BOOL FieldScriptSubEventFinish_BGMPlayback(FinishScriptSubEventsWork *work, u32 *state) {
    FieldSnd_ResumeAmbience(GameData_GetFieldSoundSystem(GSYS_GetGameData(work->gsys)),
                            FIELD_SND_AMBIENCE_STOP_SCRIPT_BGM);
    return TRUE;
}

// Forgets the players whose sound effect has ended
static void ScriptWork_SESyncStoppedPlayers(ScriptWork *work) {
    int i;
    u8 *mask = ScriptWork_GetSEBitMask(work);

    for (i = 0; i < SE_PLAYER_COUNT; i++) {
        int bit = 1 << i;

        if ((*mask & bit) && GFL_SndPlayerIsActive(i) == FALSE) {
            *mask &= (u8)~bit;
        }
    }
}

static void ScriptWork_SESetSndBit(ScriptWork *work, u16 se) {
    u8 *mask = ScriptWork_GetSEBitMask(work);

    *mask |= (u8)(1 << GFL_SndSeqGetPlayerIndex(se));
}

// Returns TRUE while a sound effect of the script is playing
static BOOL ScriptWork_SEWait(ScriptWork *work) {
    int i;
    u8 *mask = ScriptWork_GetSEBitMask(work);

    for (i = 0; i < SE_PLAYER_COUNT; i++) {
        if ((*mask & (1 << i)) && GFL_SndPlayerIsActive(i) == TRUE) {
            return TRUE;
        }
    }
    *mask = 0;
    return FALSE;
}

static void ScriptWork_SEStop(ScriptWork *work) {
    int i;
    u8 *mask = ScriptWork_GetSEBitMask(work);

    for (i = 0; i < SE_PLAYER_COUNT; i++) {
        if (*mask & (1 << i)) {
            GFL_SndPlayerStop(i);
        }
    }
    *mask = 0;
}

BOOL Scrcmd_Sound_SEPlay(VM *vm, FieldScriptEnv *env) {
    u16 se = ScriptReadAny(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_SESyncStoppedPlayers(work);
    GFL_SndSEPlay(se);
    ScriptWork_SESetSndBit(work, se);
    return FALSE;
}

BOOL Scrcmd_Sound_SEStop(VM *vm, FieldScriptEnv *env) {
    ScriptWork_SEStop(FieldScriptEnv_GetScriptWork(env));
    return FALSE;
}

static BOOL ScriptNative_SEWait(VM *vm, void *env) {
    if (ScriptWork_SEWait(FieldScriptEnv_GetScriptWork(env)) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_Sound_SEWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, ScriptNative_SEWait);
    return TRUE;
}

BOOL Scrcmd_Sound_MEPlay(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, EventMEPlay_Create(gsys, ScriptReadAny(vm, env)));
    return TRUE;
}

// Waits for the fanfare to end, then brings back the BGM
static BOOL ScriptNative_MEWait(VM *vm, void *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(GSYS_GetGameData(gsys));

    if (GFL_SndBGMIsPlaying() == FALSE && FieldSnd_IsBusy(fieldSound) == FALSE) {
        ScriptWork_CallEvent(work, EventPushBGMFinish_Create(gsys, 0, 6));
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_Sound_MEWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, ScriptNative_MEWait);
    return TRUE;
}

BOOL Scrcmd_Sound_PVPlay(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 species = ScriptReadAny(vm, env);
    u16 form = ScriptReadAny(vm, env);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventPokeVoicePlay_Callback, sizeof(PokeVoiceEvent));
    PokeVoiceEvent *voice = GameEvent_GetData(event);

    voice->species = species;
    voice->form = form;
    voice->chatter = FALSE;
    voice->env = env;
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Sound_PokeVoicePlay(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 species = ScriptReadAny(vm, env);
    u16 form = ScriptReadAny(vm, env);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventPokeVoicePlay_Callback, sizeof(PokeVoiceEvent));
    PokeVoiceEvent *voice = GameEvent_GetData(event);

    voice->species = species;
    voice->form = form;
    voice->chatter = TRUE;
    voice->env = env;
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static BOOL ScriptNative_VoiceWait(VM *vm, void *env) {
    if (PokeVoice_IsPlaying(GetScrEnvNowPkmVoice(env)) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_Sound_PVWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, ScriptNative_VoiceWait);
    return TRUE;
}

BOOL Scrcmd_Sound_ISSSwitchEnable(VM *vm, FieldScriptEnv *env) {
    ISSSwitchSys *switchSys = ISS_GetSwitchSys(GameSystem_GetISS(FieldScriptEnv_GetGameSystem(env)));

    ISSSwitchSys_ReqSwitchFadeIn(switchSys, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL Scrcmd_Sound_ISSSwitchDisable(VM *vm, FieldScriptEnv *env) {
    ISSSwitchSys *switchSys = ISS_GetSwitchSys(GameSystem_GetISS(FieldScriptEnv_GetGameSystem(env)));

    ISSSwitchSys_ReqSwitchFadeOut(switchSys, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL Scrcmd_Sound_ISSSwitchQuery(VM *vm, FieldScriptEnv *env) {
    ISSSwitchSys *switchSys = ISS_GetSwitchSys(GameSystem_GetISS(FieldScriptEnv_GetGameSystem(env)));
    u16 *result = ScriptReadVar(vm, env);

    *result = ISSSwitchSys_IsSwitchOn(switchSys, ScriptReadAny(vm, env));
    return FALSE;
}

// Plays the cry once no other sound blocks it, and keeps its handle for Scrcmd_Sound_PVWait
static GameEventReturnCode EventPokeVoicePlay_Callback(GameEvent *event, u32 *state, void *data) {
    PokeVoiceEvent *voice = data;
    u32 handle;

    if (GFL_SndIsLoadThreadRunning()) {
        return GAMEEVENT_CONTINUE;
    }
    if (voice->chatter) {
        handle = playChatterVoice(voice->species, voice->form);
    } else {
        handle = PokeVoice_Play(voice->species, voice->form, 64, 0, 0, 0, 0, NULL);
    }
    SetScrEnvNowPkmVoice(voice->env, handle);
    return GAMEEVENT_DONE;
}
