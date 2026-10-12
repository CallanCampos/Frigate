#include "types.h"
#include "system/fest_mission.h"
#include "gfl/random.h"
#include "gfl/rtc_cache.h"
#include "gfl/std.h"
#include "system/game_beacon.h"

// A Funfest mission's config: its start, its time, its score and whether something a player did counts for it. The
// ROM's string names the file. The lists the festival keeps of a mission are fest_mission_beacon.c's.

typedef BOOL (*FestMissionTargetFunc)(const FestMissionConfig *config, u32 value);

static BOOL func_02014e50(FestMissionConfig *config);
static u8 func_02014f88(FestMissionConfig *config);
static BOOL func_02014f98(const FestMissionConfig *config, u32 value);
static BOOL func_02014fac(const FestMissionConfig *config, u32 value);

// The seconds a gain of time gives, by the mission's unk10_9
static const u8 FestMission_ExtraSeconds[4] = { 0, 30, 60, 180 };

// Whether a value is the mission's target, by the mission's kind
static const FestMissionTargetFunc FestMission_TargetFuncs[7] = {
    NULL,
    func_02014f98,
    func_02014f98,
    func_02014fac,
    func_02014fac,
    func_02014fac,
    func_02014fac,
};

FestMissionConfig *allocateForFestMission(HeapID heapId) {
    FestMissionConfig *config = GFL_HeapAllocate(heapId, sizeof(FestMissionConfig), TRUE, "fest_mission.c", 60);

    ClearFesMissionCfg(config);
    return config;
}

void ClearFesMissionCfg(FestMissionConfig *config) {
    sys_memset(config, 0, sizeof(FestMissionConfig));
    config->info.count = 0x3fff;
    config->info.index = FEST_MISSION_NONE;
    config->unk24 = -1;
}

void FestMissionConfig_Start(FestMissionConfig *config, const FestMissionConfig *src, u8 joined) {
    ClearFesMissionCfg(config);
    *config = *src;
    if (!joined) {
        config->unk1C = GFL_RandomLC(0);
        config->unk24 = -1;
    } else {
        config->missionId = 0;
    }
}

void func_02014dac(FestMissionConfig *config) {
    UpdateFesMissionConfigSecondsTime(config);
}

void func_02014db4(FestMissionConfig *config) {
    ClearFesMissionCfg(config);
}

void UpdateFesMissionConfigSecondsTime(FestMissionConfig *config) {
    config->unk24 = RTC_ConvertSecondsCached();
}

u32 FestMissionConfig_GetSecondsLeft(FestMissionConfig *config) {
    s64 now = RTC_ConvertSecondsCached();
    s64 elapsed;
    s64 left;

    if (now == -1) {
        return 0;
    }
    if (config->unk24 == -1) {
        return config->info.seconds;
    }
    elapsed = now - config->unk24;
    if (elapsed < 0) {
        return 0;
    }
    left = config->info.seconds - elapsed;
    if (left < 0) {
        left = 0;
    }
    return left;
}

BOOL FestMissionConfig_HasTimeLeft(FestMissionConfig *config) {
    if (FestMissionConfig_GetSecondsLeft(config) >= 30) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_02014e50(FestMissionConfig *config) {
    if (config->info.index != FEST_MISSION_NONE) {
        return TRUE;
    }
    return FALSE;
}

BOOL isFesMissionAvailable(FestMissionConfig *config) {
    if (config->info.index != FEST_MISSION_NONE) {
        return TRUE;
    }
    return FALSE;
}

BOOL FestMissionConfig_IsTarget(FestMissionConfig *config, u32 kind, u32 value) {
    const FestMissionTargetFunc *func = &FestMission_TargetFuncs[config->header.kind];

    switch (config->header.kind) {
        case 1:
            if (kind != 0x13 && kind != 0x14) {
                return FALSE;
            }
            break;
        case 2:
            if (kind != 0x15 && kind != 0x16) {
                return FALSE;
            }
            break;
        case 3:
            if (kind - 0x18 != config->info.target) {
                return FALSE;
            }
            break;
        case 4:
            if (kind != 0x17) {
                return FALSE;
            }
            break;
        case 5:
            if (kind - 0x1e != config->info.target) {
                return FALSE;
            }
            break;
    }
    return (*func)(config, value);
}

BOOL FestMissionConfig_AddScore(FestMissionConfig *config, u32 kind, u32 value, int count) {
    int score;

    if (!func_02014e50(config)) {
        return FALSE;
    }
    if (!FestMissionConfig_IsTarget(config, kind, value)) {
        return FALSE;
    }
    score = config->missionId + count;
    if (score > FEST_MISSION_SCORE_MAX) {
        config->missionId = FEST_MISSION_SCORE_MAX;
    } else {
        config->missionId = score;
    }
    return TRUE;
}

u32 FestMissionConfig_AddSeconds(FestMissionConfig *config, u16 gain) {
    u32 seconds = func_02014f88(config) * gain;

    if (seconds == 0) {
        return 0;
    }
    if (config->info.seconds + seconds > FEST_MISSION_SECONDS_MAX) {
        seconds = FEST_MISSION_SECONDS_MAX - config->info.seconds;
        config->info.seconds = FEST_MISSION_SECONDS_MAX;
    } else {
        config->info.seconds += seconds;
    }
    return seconds;
}

BOOL FestMissionConfig_IsSame(const FestMissionConfig *config, const FestMissionConfig *other) {
    if (config->unk1C != other->unk1C || config->unk00[0] != other->unk00[0]) {
        return FALSE;
    }
    return TRUE;
}

static u8 func_02014f88(FestMissionConfig *config) {
    return FestMission_ExtraSeconds[config->info.unk10_9];
}

static BOOL func_02014f98(const FestMissionConfig *config, u32 value) {
    BOOL result = TRUE;

    if (config->info.target != 0 && config->info.target != value) {
        result = FALSE;
    }
    return result;
}

static BOOL func_02014fac(const FestMissionConfig *config, u32 value) {
    return TRUE;
}

BOOL FestMissionConfig_FromBeacon(FestMissionConfig *config, const GameBeacon *beacon) {
    ClearFesMissionCfg(config);
    if (GameBeacon_IsMissionType(beacon)) {
        config->info = *(const FestMissionInfo *)&beacon->payload;
        config->missionId = beacon->arg.mission.missionId;
        config->unk1A = beacon->arg.mission.unk;
        config->unk1C = beacon->missionWord;
        return TRUE;
    }
    return FALSE;
}
