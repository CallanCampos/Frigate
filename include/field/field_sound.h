#ifndef POKEBW2_FIELD_FIELD_SOUND_H
#define POKEBW2_FIELD_FIELD_SOUND_H

// The field's sound system (src/system/field_sound_system.c) and the zone BGM choice (src/system/field_sound.c).
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except FieldSnd_IsBGMChanging,
// FieldSnd_IsPopDone, FieldSnd_RingRingtone, FieldSnd_StopRingtone, FieldSnd_DuckVolume and FieldSnd_RestoreVolume

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A request to the field's sound system
typedef enum {
    FIELD_SND_CMD_NULL = 0x0,
    FIELD_SND_FADE_IN = 0x1,
    FIELD_SND_FADE_OUT = 0x2,
    FIELD_SND_BGM_PUSH = 0x3,
    FIELD_SND_BGM_POP = 0x4,
    FIELD_SND_BGM_CHANGE = 0x5,
    FIELD_SND_BGM_PREPARE = 0x6,
    FIELD_SND_BGM_PLAY = 0x7,
} FieldSoundCommand;

// src/system/field_sound_system.c
FieldSound *FieldSnd_Create(GameData *gameData, HeapID heapId);
void FieldSnd_Free(FieldSound *fieldSound);
u32 FieldSnd_BGMGetStackIndex(FieldSound *fieldSound);
u32 FieldSnd_GetNowBGM(const FieldSound *fieldSound);
// Whether the BGM is changing, fading, being pushed or popped
BOOL FieldSnd_IsBGMChanging(FieldSound *fieldSound);
BOOL FieldSnd_IsBusy(FieldSound *fieldSound);
// Whether no pop is still running or queued
BOOL FieldSnd_IsPopDone(FieldSound *fieldSound);
void FieldSnd_SendRequest(FieldSound *fieldSound, FieldSoundCommand command, u32 bgm, u16 fadeOutFrames,
                          u16 fadeInFrames);
// Called every frame
void FieldSnd_Update(FieldSound *fieldSound);
void FieldSnd_Release(FieldSound *fieldSound, GameData *gameData);
void FieldSnd_SetPlayerVolumeFade(FieldSound *fieldSound, u8 volume, u8 duration);
// Ring and stop the ringtone
void FieldSnd_RingRingtone(FieldSound *fieldSound);
void FieldSnd_StopRingtone(FieldSound *fieldSound);
// Ambient sound effects: a looped one is played again after a stop by any of the flags is lifted. volume is 0 to 127,
// or FIELD_SND_AMBIENCE_VOLUME_DEFAULT for the sequence's own
#define FIELD_SND_AMBIENCE_VOLUME_DEFAULT 0xffff
// The stop flag that the BGM push sets
#define FIELD_SND_AMBIENCE_STOP_BGM_PUSH 0x1
// The stop flag of the BGM that a script plays, which the script's map BGM and the end of its sub event lift
#define FIELD_SND_AMBIENCE_STOP_SCRIPT_BGM 0x2
void FieldSnd_PlayAmbience(FieldSound *fieldSound, u32 se);
void FieldSnd_PlayAmbienceEx(FieldSound *fieldSound, u32 se, u32 volume);
void FieldSnd_SetAmbienceVolume(FieldSound *fieldSound, u32 se, u16 volume);
void FieldSnd_StopAmbience(FieldSound *fieldSound, u32 se);
void FieldSnd_StopAmbienceAll(FieldSound *fieldSound, u32 flags);
void FieldSnd_ResumeAmbience(FieldSound *fieldSound, u32 flags);

// src/system/field_sound.c
// Changes to the zone's BGM, fading in only when the player is cycling
void FieldSnd_ChangeZoneBGM(FieldSound *fieldSound, GameData *gameData, u16 zoneId);
// Prepares the zone's BGM, for FieldSnd_FadeInImmediate to start
void FieldSnd_SetZoneBGM(FieldSound *fieldSound, GameData *gameData, u16 zoneId, u8 season);
void FieldSnd_FadeInImmediate(FieldSound *fieldSound, GameData *gameData);
// Changes to the BGM for the player's new state (walking, cycling or surfing)
void FieldSnd_SetBGMOnPlayerExStateChange(FieldSound *fieldSound, GameData *gameData, u16 zoneId);
// Silences the 3D sound and lowers the BGM, such as for a menu over the field
void FieldSnd_DuckVolume(FieldSound *fieldSound, ISS *iss);
void FieldSnd_RestoreVolume(FieldSound *fieldSound, ISS *iss);
u32 GetMapBGMIDByPlayerState2(GameData *gameData, s32 zoneId, u8 season);
u32 GameData_GetNowBGM(GameData *gameData);
// Whether an event flag or the Funfest puts another BGM on the zone
BOOL GameData_IsZoneBGMOverriden(GameData *gameData, s32 zoneId);

#endif // POKEBW2_FIELD_FIELD_SOUND_H
