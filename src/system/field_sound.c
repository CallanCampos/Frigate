// The field's choice of BGM for a zone: the zone's own, the cycling or surfing BGM, or one that an event flag or the
// Funfest puts on it. The file's name is descriptive, a guess: the ROM has no string for it. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except ZoneBGMOverride, sFunfestBGMExcludedParentZones,
// sFunfestBGMExcludedZones, FieldSnd_DuckVolume and FieldSnd_RestoreVolume

#include "types.h"
#include "constants/sound.h"
#include "field/field_sound.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/iss_3ds_sys.h"
#include "system/iss_sys.h"

// What GetZoneBGMOverrideID returns when nothing overrides the zone's BGM
#define NO_BGM_OVERRIDE 0xFFFFFFFF

// A BGM that plays in a zone while an event flag is set
typedef struct {
    u16 flag;
    u16 zoneId;
    u32 bgm;
} ZoneBGMOverride;

static u32 GetPlayerLandSurfCycleState(GameData *gameData);
static u32 GetRegularZoneBGMID(GameData *gameData, s32 zoneId, u8 season);
static u32 GetZoneBGMOverrideID(GameData *gameData, s32 zoneId);
static u32 GetMapBGMIDByPlayerState(GameData *gameData, s32 zoneId, u8 season);

// The zones, by their parent zones, where the Funfest's BGM doesn't play
static const u16 sFunfestBGMExcludedParentZones[] = { 0x108, 0x228 };

// The zones where the Funfest's BGM doesn't play
static const u16 sFunfestBGMExcludedZones[] = { 0x1c7, 0x1f6, 0xea, 0x25c, 0xd5, 0x3f };

static const ZoneBGMOverride ZONE_WK_BGM_MAP[] = {
    { 0x97d, 0x3f, SEQ_BGM_ERECTRIC_GYM_02 },
    { 0x97e, 0x1b3, SEQ_BGM_E_TSURETEKE2 },
    { 0x9f1, 0x1ea, SEQ_BGM_JAPARADE },
    { 0x9f2, 0x78, SEQ_BGM_E_C08_ICE },
    { 0x9f9, 0x78, SEQ_BGM_SILENCE_FIELD },
    { 0x9f9, 0x7b, SEQ_BGM_SILENCE_FIELD },
    { 0x9fa, 0x22d, SEQ_BGM_E_PLASMA },
    { 0x9fb, 0x25c, SEQ_BGM_EV_GIANTHOLE_02 },
    { 0x9fc, 0x25c, SEQ_BGM_EV_GIANTHOLE_01 },
    { 0x9fe, 0x1be, SEQ_BGM_E_PLASMA },
    { 0x9ff, 0x76, SEQ_BGM_R_F },
    { 0xa00, 0x228, SEQ_BGM_SILENCE_FIELD },
    { 0x9f2, 0x7b, SEQ_BGM_E_C08_ICE },
    { 0x9f2, 0x7c, SEQ_BGM_E_C08_ICE },
};

void FieldSnd_ChangeZoneBGM(FieldSound *fieldSound, GameData *gameData, u16 zoneId) {
    u8 season = GameData_GetSeason(gameData);
    u32 bgm = GetZoneBGMOverrideID(gameData, zoneId);
    u16 fadeOutFrames;
    u16 fadeInFrames;

    if (bgm == NO_BGM_OVERRIDE) {
        bgm = GetRegularZoneBGMID(gameData, zoneId, season);
    }
    if (GetPlayerLandSurfCycleState(gameData) == FLD_PLAYER_EXSTATE_CYCLING) {
        fadeOutFrames = 60;
        fadeInFrames = 60;
    } else {
        fadeOutFrames = 60;
        fadeInFrames = 0;
    }
    FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_CHANGE, bgm, fadeOutFrames, fadeInFrames);
}

void FieldSnd_SetZoneBGM(FieldSound *fieldSound, GameData *gameData, u16 zoneId, u8 season) {
    u32 bgm = GetZoneBGMOverrideID(gameData, zoneId);

    if (bgm == NO_BGM_OVERRIDE) {
        bgm = GetRegularZoneBGMID(gameData, zoneId, season);
    }
    FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_PREPARE, bgm, 60, 0);
}

void FieldSnd_FadeInImmediate(FieldSound *fieldSound, GameData *gameData) {
    FieldSnd_SendRequest(fieldSound, FIELD_SND_FADE_IN, 0, 0, 0);
}

void FieldSnd_SetBGMOnPlayerExStateChange(FieldSound *fieldSound, GameData *gameData, u16 zoneId) {
    u8 season = GameData_GetSeason(gameData);
    u32 bgm = GetMapBGMIDByPlayerState(gameData, zoneId, season);

    FieldSnd_SendRequest(fieldSound, FIELD_SND_BGM_CHANGE, bgm, 30, 0);
}

void FieldSnd_DuckVolume(FieldSound *fieldSound, ISS *iss) {
    ISS3DSoundSys_ReqChangeMasterVolume(ISS_Get3DSoundSys(iss), 0);
    FieldSnd_SetPlayerVolumeFade(fieldSound, 64, 6);
}

void FieldSnd_RestoreVolume(FieldSound *fieldSound, ISS *iss) {
    ISS3DSoundSys_ReqChangeMasterVolume(ISS_Get3DSoundSys(iss), 127);
    FieldSnd_SetPlayerVolumeFade(fieldSound, 127, 6);
}

u32 GetMapBGMIDByPlayerState2(GameData *gameData, s32 zoneId, u8 season) {
    return GetMapBGMIDByPlayerState(gameData, zoneId, season);
}

u32 GameData_GetNowBGM(GameData *gameData) {
    return FieldSnd_GetNowBGM(GameData_GetFieldSoundSystem(gameData));
}

BOOL GameData_IsZoneBGMOverriden(GameData *gameData, s32 zoneId) {
    if (GetZoneBGMOverrideID(gameData, zoneId) != NO_BGM_OVERRIDE) {
        return TRUE;
    }
    return FALSE;
}

static u32 GetPlayerLandSurfCycleState(GameData *gameData) {
    return FieldPlayerState_GetExState(GameData_GetPlayerState(gameData));
}

static u32 GetRegularZoneBGMID(GameData *gameData, s32 zoneId, u8 season) {
    return DecideZoneHeaderBGMID(zoneId, season);
}

static u32 GetZoneBGMOverrideID(GameData *gameData, s32 zoneId) {
    EventWork *eventWork = GameData_GetEventWork(gameData);
    u32 i;
    u16 parentZone;

    for (i = 0; i < NELEMS(ZONE_WK_BGM_MAP); i++) {
        if (zoneId == ZONE_WK_BGM_MAP[i].zoneId && EventWork_FlagGet(eventWork, ZONE_WK_BGM_MAP[i].flag) == TRUE) {
            return ZONE_WK_BGM_MAP[i].bgm;
        }
    }

    parentZone = GetZoneParentZone(zoneId);
    for (i = 0; i < NELEMS(sFunfestBGMExcludedParentZones); i++) {
        if (parentZone == sFunfestBGMExcludedParentZones[i]) {
            return NO_BGM_OVERRIDE;
        }
    }
    for (i = 0; i < NELEMS(sFunfestBGMExcludedZones); i++) {
        if (zoneId == sFunfestBGMExcludedZones[i]) {
            return NO_BGM_OVERRIDE;
        }
    }
    if (IsZoneFlashbackMemoryPostFX(zoneId)) {
        return NO_BGM_OVERRIDE;
    }

    if (GameData_IsFunfestMission(gameData)) {
        return SEQ_BGM_FES;
    }
    return NO_BGM_OVERRIDE;
}

static u32 GetMapBGMIDByPlayerState(GameData *gameData, s32 zoneId, u8 season) {
    u32 exState = GetPlayerLandSurfCycleState(gameData);
    // Unused: the original fetches the sound system here and drops it
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);
    u32 bgm = GetZoneBGMOverrideID(gameData, zoneId);

    if (bgm != NO_BGM_OVERRIDE) {
        return bgm;
    }
    if (exState == FLD_PLAYER_EXSTATE_SURF && GetZoneFlagsEnableCycleSurfBGM(zoneId)) {
        return SEQ_BGM_NAMINORI;
    }
    if (exState == FLD_PLAYER_EXSTATE_CYCLING && GetZoneFlagsEnableCycleSurfBGM(zoneId)) {
        return SEQ_BGM_BICYCLE;
    }
    return GetRegularZoneBGMID(gameData, zoneId, season);
}
