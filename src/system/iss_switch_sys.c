#include "types.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "system/iss_switch_set.h"
#include "system/iss_switch_sys.h"

// The interactive sound system's BGM switches. Each BGM with switches has a switch set, loaded from ARCID_ISS_SWITCH;
// the system updates the set of the BGM that is playing, and keeps requests to mute switches or fade the master volume
// until their BGM plays. Names and layouts from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except
// ISSSwitchSys_ResetSwitches, ISSSwitchSys_ResetMuteStateChangeRequests and
// ISSSwitchSys_ResetMasterVolumeChangeRequest

#define ISS_SWITCH_MUTE_REQUEST_MAX 4

typedef struct {
    BOOL enabled;
    BOOL isUnmute;
    u32 bgmId;
    ISSSwitchIndex switchIndex;
} ISSSwitchMuteStateChangeRequest;

typedef struct {
    BOOL enabled;
    u32 bgmId;
    int targetVolume;
    // The fade's length, in frames
    u16 interval;
} ISSSwitchMasterVolumeChangeRequest;

struct ISSSwitchSys {
    BOOL isEnabled;
    u8 setCount;
    ISSSwitchSet **switchSets;
    // The switch set of the BGM that is playing
    ISSSwitchSet *nowSwitchSet;
    ISSSwitchMuteStateChangeRequest muteStateChangeRequests[ISS_SWITCH_MUTE_REQUEST_MAX];
    ISSSwitchMasterVolumeChangeRequest volumeChangeRequest;
};

static void ISSSwitchSys_Reset(ISSSwitchSys *switchSys);
static ISSSwitchSys *ISSSwitchSys_Alloc(HeapID heapId);
static void ISSSwitchSys_FreeCore(ISSSwitchSys *switchSys);
static void ISSSwitchSys_LoadArcData(ISSSwitchSys *switchSys, HeapID heapId);
static void ISSSwitchSys_FreeSwitchSets(ISSSwitchSys *switchSys);
static void ISSSwitchSys_UpdateSwitchSet(ISSSwitchSys *switchSys);
static void ISSSwitchSys_EnableCore(ISSSwitchSys *switchSys);
static void ISSSwitchSys_DisableCore(ISSSwitchSys *switchSys);
static void ISSSwitchSys_ChangeZoneCore(ISSSwitchSys *switchSys, u16 zoneId);
static u8 ISSSwitchSys_GetSwitchSetCount(ISSSwitchSys *switchSys);
static ISSSwitchSet *ISSSwitchSys_GetSwitchSet(ISSSwitchSys *switchSys, u8 index);
static ISSSwitchSet *ISSSwitchSys_GetNowSwitchSet(ISSSwitchSys *switchSys);
static BOOL ISSSwitchSys_IsZoneIDNoNeedSetup(ISSSwitchSys *switchSys, u16 zoneId);
static ISSSwitchSet *ISSSwitchSys_GetSwitchSetForBGM(ISSSwitchSys *switchSys, u32 bgm);
static BOOL ISSSwitchSys_ChangeSwitchSetForBGM(ISSSwitchSys *switchSys, u32 bgm);
static void ISSSwitchSys_CommitVolume(ISSSwitchSys *switchSys);
static u32 ISSSwitchSys_GetBGMID(void);
static BOOL ISSSwitchSys_IsEnabled(ISSSwitchSys *switchSys);
static void ISSSwitchSys_ResetSwitchSetState(ISSSwitchSys *switchSys);
static void ISSSwitch_ReqSwitchMuteStateChangeCore(ISSSwitchSys *switchSys, ISSSwitchIndex switchIndex, BOOL unmute,
                                                   u32 bgm);
static void ISSSwitchMuteStateChangeRequest_Set(ISSSwitchMuteStateChangeRequest *request, ISSSwitchIndex switchIndex,
                                                BOOL unmute, u32 bgm);
static void ISSSwitchMuteStateChangeRequest_Reset(ISSSwitchMuteStateChangeRequest *request);
static void ISSSwitchSys_ProcessMuteStateChangeRequest(ISSSwitchMuteStateChangeRequest *request,
                                                       ISSSwitchSys *switchSys);
static BOOL ISSSwitchMuteStateChangeRequest_IsEnabled(ISSSwitchMuteStateChangeRequest *request);
static void ISSSwitchSys_ReqMasterVolumeChangeCore(ISSSwitchSys *switchSys, int volume, u16 frames, u32 bgm);
static void ISSSwitchMasterVolumeChangeRequest_Set(ISSSwitchMasterVolumeChangeRequest *request, int volume, u16 frames,
                                                   u32 bgm);
static void ISSSwitchMasterVolumeChangeRequest_Reset(ISSSwitchMasterVolumeChangeRequest *request);
static void ISSSwitchSys_ProcessMasterVolumeChangeRequest(ISSSwitchMasterVolumeChangeRequest *request,
                                                          ISSSwitchSys *switchSys);
static BOOL ISSSwitchMasterVolumeChangeRequest_IsEnabled(ISSSwitchMasterVolumeChangeRequest *request);

ISSSwitchSys *ISSSwitchSys_Create(HeapID heapId) {
    ISSSwitchSys *switchSys = ISSSwitchSys_Alloc(heapId);

    ISSSwitchSys_Reset(switchSys);
    ISSSwitchSys_LoadArcData(switchSys, heapId);
    return switchSys;
}

void ISSSwitchSys_Free(ISSSwitchSys *switchSys) {
    ISSSwitchSys_FreeSwitchSets(switchSys);
    ISSSwitchSys_FreeCore(switchSys);
}

void ISSSwitchSys_Update(ISSSwitchSys *switchSys) {
    ISSSwitchMasterVolumeChangeRequest *volumeRequest;
    int i;

    if (ISSSwitchSys_IsEnabled(switchSys)) {
        for (i = 0; i < ISS_SWITCH_MUTE_REQUEST_MAX; i++) {
            if (ISSSwitchMuteStateChangeRequest_IsEnabled(&switchSys->muteStateChangeRequests[i])) {
                ISSSwitchSys_ProcessMuteStateChangeRequest(&switchSys->muteStateChangeRequests[i], switchSys);
            }
        }
        volumeRequest = &switchSys->volumeChangeRequest;
        if (ISSSwitchMasterVolumeChangeRequest_IsEnabled(volumeRequest)) {
            ISSSwitchSys_ProcessMasterVolumeChangeRequest(volumeRequest, switchSys);
        }
        ISSSwitchSys_UpdateSwitchSet(switchSys);
        ISSSwitchSys_CommitVolume(switchSys);
    }
}

void ISSSwitchSys_Enable(ISSSwitchSys *switchSys) {
    ISSSwitchSys_EnableCore(switchSys);
}

void ISSSwitchSys_Disable(ISSSwitchSys *switchSys) {
    ISSSwitchSys_DisableCore(switchSys);
}

void ISSSwitchSys_ChangeZone(ISSSwitchSys *switchSys, u16 zoneId) {
    ISSSwitchSys_ChangeZoneCore(switchSys, zoneId);
}

void ISSSwitchSys_SeqMasterVolumeFade(ISSSwitchSys *switchSys, int volume, u16 frames) {
    ISSSwitchSet *set = ISSSwitchSys_GetNowSwitchSet(switchSys);

    if (set != NULL) {
        ISSSwitchSet_SetMasterVolumeFade(set, volume, frames);
    }
}

void ISSSwitchSys_ReqSwitchFadeIn(ISSSwitchSys *switchSys, ISSSwitchIndex switchIndex) {
    ISSSwitchSet *set = ISSSwitchSys_GetNowSwitchSet(switchSys);

    if (set != NULL) {
        ISSSwitchSet_ReqSwitchFadeIn(set, switchIndex);
    }
}

void ISSSwitchSys_ReqSwitchFadeOut(ISSSwitchSys *switchSys, ISSSwitchIndex switchIndex) {
    ISSSwitchSet *set = ISSSwitchSys_GetNowSwitchSet(switchSys);

    if (set != NULL) {
        ISSSwitchSet_ReqSwitchFadeOut(set, switchIndex);
    }
}

BOOL ISSSwitchSys_IsSwitchOn(ISSSwitchSys *switchSys, ISSSwitchIndex switchIndex) {
    ISSSwitchSet *set = ISSSwitchSys_GetNowSwitchSet(switchSys);

    if (set != NULL) {
        return ISSSwitchSet_IsSwitchOn(set, switchIndex);
    }
    return FALSE;
}

void ISSSwitchSys_ResetSwitches(ISSSwitchSys *switchSys) {
    ISSSwitchSys_ResetSwitchSetState(switchSys);
}

void ISSSwitch_ReqSwitchMuteStateChange(ISSSwitchSys *switchSys, ISSSwitchIndex switchIndex, BOOL unmute, u32 bgm) {
    ISSSwitch_ReqSwitchMuteStateChangeCore(switchSys, switchIndex, unmute, bgm);
}

void ISSSwitchSys_ResetMuteStateChangeRequests(ISSSwitchSys *switchSys) {
    int i;

    for (i = 0; i < ISS_SWITCH_MUTE_REQUEST_MAX; i++) {
        ISSSwitchMuteStateChangeRequest_Reset(&switchSys->muteStateChangeRequests[i]);
    }
}

void ISSSwitchSys_ReqMasterVolumeChange(ISSSwitchSys *switchSys, int volume, u16 frames, u32 bgm) {
    ISSSwitchSys_ReqMasterVolumeChangeCore(switchSys, volume, frames, bgm);
}

void ISSSwitchSys_ResetMasterVolumeChangeRequest(ISSSwitchSys *switchSys) {
    ISSSwitchMasterVolumeChangeRequest_Reset(&switchSys->volumeChangeRequest);
}

static void ISSSwitchSys_Reset(ISSSwitchSys *switchSys) {
    int i;

    switchSys->isEnabled = FALSE;
    switchSys->setCount = 0;
    switchSys->switchSets = NULL;
    switchSys->nowSwitchSet = NULL;
    for (i = 0; i < ISS_SWITCH_MUTE_REQUEST_MAX; i++) {
        ISSSwitchMuteStateChangeRequest_Reset(&switchSys->muteStateChangeRequests[i]);
    }
    ISSSwitchMasterVolumeChangeRequest_Reset(&switchSys->volumeChangeRequest);
}

static ISSSwitchSys *ISSSwitchSys_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(ISSSwitchSys), FALSE, "iss_switch_sys.c", 409);
}

static void ISSSwitchSys_FreeCore(ISSSwitchSys *switchSys) {
    GFL_HeapFree(switchSys);
}

static void ISSSwitchSys_LoadArcData(ISSSwitchSys *switchSys, HeapID heapId) {
    int count = GFL_ArcSysGetDataMax(ARCID_ISS_SWITCH);
    int i;

    switchSys->setCount = count;
    switchSys->switchSets = GFL_HeapAllocate(heapId, count * sizeof(ISSSwitchSet *), FALSE, "iss_switch_sys.c", 457);
    for (i = 0; i < count; i++) {
        ISSSwitchSet *set = ISSSwitchSet_Create(heapId);

        ISSSwitchSet_LoadArcData(set, i, heapId);
        switchSys->switchSets[i] = set;
    }
}

static void ISSSwitchSys_FreeSwitchSets(ISSSwitchSys *switchSys) {
    int count = switchSys->setCount;
    int i;

    for (i = 0; i < count; i++) {
        ISSSwitchSet_Free(switchSys->switchSets[i]);
    }
    GFL_HeapFree(switchSys->switchSets);
    switchSys->setCount = 0;
    switchSys->switchSets = NULL;
    switchSys->nowSwitchSet = NULL;
}

static void ISSSwitchSys_UpdateSwitchSet(ISSSwitchSys *switchSys) {
    ISSSwitchSet *set = ISSSwitchSys_GetNowSwitchSet(switchSys);

    if (set != NULL) {
        ISSSwitchSet_Update(set);
    }
}

static void ISSSwitchSys_EnableCore(ISSSwitchSys *switchSys) {
    if (ISSSwitchSys_IsEnabled(switchSys) == TRUE) {
        return;
    }
    switchSys->isEnabled = TRUE;
    if (ISSSwitchSys_ChangeSwitchSetForBGM(switchSys, ISSSwitchSys_GetBGMID()) == TRUE) {
        ISSSwitchSys_ResetSwitchSetState(switchSys);
        ISSSwitchSys_CommitVolume(switchSys);
    }
}

static void ISSSwitchSys_DisableCore(ISSSwitchSys *switchSys) {
    if (ISSSwitchSys_IsEnabled(switchSys)) {
        switchSys->isEnabled = FALSE;
    }
}

static void ISSSwitchSys_ChangeZoneCore(ISSSwitchSys *switchSys, u16 zoneId) {
    if (ISSSwitchSys_IsEnabled(switchSys) && ISSSwitchSys_IsZoneIDNoNeedSetup(switchSys, zoneId) == FALSE) {
        ISSSwitchSys_ResetSwitchSetState(switchSys);
        ISSSwitchSys_DisableCore(switchSys);
    }
}

static u8 ISSSwitchSys_GetSwitchSetCount(ISSSwitchSys *switchSys) {
    return switchSys->setCount;
}

static ISSSwitchSet *ISSSwitchSys_GetSwitchSet(ISSSwitchSys *switchSys, u8 index) {
    return switchSys->switchSets[index];
}

static ISSSwitchSet *ISSSwitchSys_GetNowSwitchSet(ISSSwitchSys *switchSys) {
    return switchSys->nowSwitchSet;
}

// Whether the BGM's switch set also plays in the zone
static BOOL ISSSwitchSys_IsZoneIDNoNeedSetup(ISSSwitchSys *switchSys, u16 zoneId) {
    ISSSwitchSet *set = ISSSwitchSys_GetNowSwitchSet(switchSys);

    if (set == NULL) {
        return FALSE;
    }
    return ISSSwitchSet_CheckHasZone(set, zoneId);
}

static ISSSwitchSet *ISSSwitchSys_GetSwitchSetForBGM(ISSSwitchSys *switchSys, u32 bgm) {
    int count = ISSSwitchSys_GetSwitchSetCount(switchSys);
    int i;

    for (i = 0; i < count; i++) {
        ISSSwitchSet *set = ISSSwitchSys_GetSwitchSet(switchSys, i);

        if (bgm == ISSSwitchSet_GetBGMId(set)) {
            return set;
        }
    }
    return NULL;
}

// Returns whether the switch set changed
static BOOL ISSSwitchSys_ChangeSwitchSetForBGM(ISSSwitchSys *switchSys, u32 bgm) {
    ISSSwitchSet *nowSet = ISSSwitchSys_GetNowSwitchSet(switchSys);
    ISSSwitchSet *set = ISSSwitchSys_GetSwitchSetForBGM(switchSys, bgm);

    if (nowSet == set) {
        return FALSE;
    }
    switchSys->nowSwitchSet = set;
    return TRUE;
}

static void ISSSwitchSys_CommitVolume(ISSSwitchSys *switchSys) {
    ISSSwitchSet *set = ISSSwitchSys_GetNowSwitchSet(switchSys);

    if (set != NULL) {
        ISSSwitchSet_ApplyVolume(set);
    }
}

static u32 ISSSwitchSys_GetBGMID(void) {
    return GFL_SndBGMGetID();
}

static BOOL ISSSwitchSys_IsEnabled(ISSSwitchSys *switchSys) {
    return switchSys->isEnabled;
}

static void ISSSwitchSys_ResetSwitchSetState(ISSSwitchSys *switchSys) {
    ISSSwitchSet *set = ISSSwitchSys_GetNowSwitchSet(switchSys);

    if (set != NULL) {
        ISSSwitchSet_ResetState(set);
    }
}

static void ISSSwitch_ReqSwitchMuteStateChangeCore(ISSSwitchSys *switchSys, ISSSwitchIndex switchIndex, BOOL unmute,
                                                   u32 bgm) {
    int i;

    for (i = 0; i < ISS_SWITCH_MUTE_REQUEST_MAX; i++) {
        if (ISSSwitchMuteStateChangeRequest_IsEnabled(&switchSys->muteStateChangeRequests[i]) == FALSE) {
            ISSSwitchMuteStateChangeRequest_Set(&switchSys->muteStateChangeRequests[i], switchIndex, unmute, bgm);
            return;
        }
    }
}

static void ISSSwitchMuteStateChangeRequest_Set(ISSSwitchMuteStateChangeRequest *request, ISSSwitchIndex switchIndex,
                                                BOOL unmute, u32 bgm) {
    ISSSwitchMuteStateChangeRequest_Reset(request);
    request->switchIndex = switchIndex;
    request->isUnmute = unmute;
    request->bgmId = bgm;
    request->enabled = TRUE;
}

static void ISSSwitchMuteStateChangeRequest_Reset(ISSSwitchMuteStateChangeRequest *request) {
    MI_CpuFill8(request, 0, sizeof(ISSSwitchMuteStateChangeRequest));
}

static void ISSSwitchSys_ProcessMuteStateChangeRequest(ISSSwitchMuteStateChangeRequest *request,
                                                       ISSSwitchSys *switchSys) {
    ISSSwitchSet *set;

    if (request->enabled == FALSE || ISSSwitchSys_GetNowSwitchSet(switchSys) == NULL ||
        request->bgmId != ISSSwitchSys_GetBGMID()) {
        return;
    }
    set = ISSSwitchSys_GetNowSwitchSet(switchSys);
    if (request->isUnmute) {
        if (set != NULL) {
            ISSSwitchSet_UnmuteSwitch(set, request->switchIndex);
        }
    } else {
        if (set != NULL) {
            ISSSwitchSet_MuteSwitch(set, request->switchIndex);
        }
    }
    ISSSwitchMuteStateChangeRequest_Reset(request);
}

static BOOL ISSSwitchMuteStateChangeRequest_IsEnabled(ISSSwitchMuteStateChangeRequest *request) {
    return request->enabled;
}

static void ISSSwitchSys_ReqMasterVolumeChangeCore(ISSSwitchSys *switchSys, int volume, u16 frames, u32 bgm) {
    ISSSwitchMasterVolumeChangeRequest *request = &switchSys->volumeChangeRequest;

    if (ISSSwitchMasterVolumeChangeRequest_IsEnabled(request) == FALSE) {
        ISSSwitchMasterVolumeChangeRequest_Set(request, volume, frames, bgm);
    }
}

static void ISSSwitchMasterVolumeChangeRequest_Set(ISSSwitchMasterVolumeChangeRequest *request, int volume, u16 frames,
                                                   u32 bgm) {
    ISSSwitchMasterVolumeChangeRequest_Reset(request);
    request->targetVolume = volume;
    request->interval = frames;
    request->bgmId = bgm;
    request->enabled = TRUE;
}

static void ISSSwitchMasterVolumeChangeRequest_Reset(ISSSwitchMasterVolumeChangeRequest *request) {
    MI_CpuFill8(request, 0, sizeof(ISSSwitchMasterVolumeChangeRequest));
}

static void ISSSwitchSys_ProcessMasterVolumeChangeRequest(ISSSwitchMasterVolumeChangeRequest *request,
                                                          ISSSwitchSys *switchSys) {
    if (request->enabled == FALSE || ISSSwitchSys_GetNowSwitchSet(switchSys) == NULL ||
        request->bgmId != ISSSwitchSys_GetBGMID()) {
        return;
    }
    ISSSwitchSet_SetMasterVolumeFade(ISSSwitchSys_GetNowSwitchSet(switchSys), request->targetVolume, request->interval);
    ISSSwitchMasterVolumeChangeRequest_Reset(request);
}

static BOOL ISSSwitchMasterVolumeChangeRequest_IsEnabled(ISSSwitchMasterVolumeChangeRequest *request) {
    return request->enabled;
}
