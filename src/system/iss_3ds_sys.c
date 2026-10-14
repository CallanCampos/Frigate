#include "types.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/iss_3ds_sys.h"

// The interactive sound system's 3D sound. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

// No config is used: the BGM has no entry in the 3D sound archive
#define ISS_3DS_CONFIG_NONE 0xff
// How far a unit's volume and pan move toward their targets each update
#define ISS_3DS_VOLUME_STEP 16
#define ISS_3DS_PAN_STEP 32

// A BGM that plays in 3D, from the 3D sound archive
typedef struct {
    u32 bgm;
    // The tracks that the units play, which start muted
    u16 trackMask;
} ISS3DSoundConfig;

// A source of sound, which plays its track at a volume that falls with its distance from the listener
typedef struct {
    BOOL enabled;
    VecFx32 pos;
    // How far it can be heard
    fx32 range;
    // Its volume where it is
    s32 volume;
    u16 trackMask;
} ISS3DSoundUnit;

struct ISS3DSoundSys {
    HeapID heapId;
    BOOL enabled;
    u8 targetMasterVolume;
    u8 masterVolume;
    // How far the master volume moves toward its target each update
    u8 masterVolumeStep;
    VecFx32 listenerPos;
    VecFx32 listenerTarget;
    ISS3DSoundUnit units[ISS_3DS_UNIT_COUNT];
    s32 unitVolume[ISS_3DS_UNIT_COUNT];
    s32 unitPan[ISS_3DS_UNIT_COUNT];
    ISS3DSoundConfig *configs;
    u8 configCount;
    u8 usedConfigId;
};

static void ISS3DSoundSys_Init(ISS3DSoundSys *sys);
static void ISS3DSoundSys_LoadArcData(ISS3DSoundSys *sys);
static void ISS3DSoundSys_FreeEntries(ISS3DSoundSys *sys);
static void ISS3DSoundSys_SetUsedConfigID(ISS3DSoundSys *sys, u8 configId);
static u8 ISS3DSoundSys_DecideUsedConfigID(ISS3DSoundSys *sys, u32 bgm);
static void ISS3DSoundSys_Reset(ISS3DSoundSys *sys);
static void ISS3DSoundSys_EnableUnitCore(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, fx32 range, s32 volume);
static void ISS3DSoundSys_SetUnitLocationCore(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, const VecFx32 *pos);
static void ISS3DSoundSys_SetListenerCore(ISS3DSoundSys *sys, const VecFx32 *pos, const VecFx32 *target);
static void ISS3DSoundSys_EnableCore(ISS3DSoundSys *sys);
static void ISS3DSoundSys_DisableCore(ISS3DSoundSys *sys);
static void ISS3DSoundSys_UpdateCore(ISS3DSoundSys *sys);
static void ISS3DSoundSys_ChangeZoneCore(ISS3DSoundSys *sys, u16 zoneId);
static void ISS3DSoundSys_MuteConfig(ISS3DSoundSys *sys);
static void ISS3DSoundSys_CalcUnitVolume(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit);
static void ISS3DSoundSys_CalcUnitPan(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit);
static s32 ISS3DSoundUnit_CalcVolume(const ISS3DSoundUnit *unit, const VecFx32 *listenerPos);
static void ISS3DSoundSys_RaiseUnitVolume(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, s32 volume);
static void ISS3DSoundSys_LowerUnitVolume(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, s32 volume);
static void ISS3DSoundSys_CommitVolume(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit);
static s32 ISS3DSoundUnit_CalcPan(const ISS3DSoundUnit *unit, const VecFx32 *listenerPos,
                                  const VecFx32 *listenerTarget);
static void ISS3DSoundSys_LowerUnitPan(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, s32 pan);
static void ISS3DSoundSys_RaiseUnitPan(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, s32 pan);
static void ISS3DSoundSys_CommitPan(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit);
static void ISS3DSoundSys_UpdateMasterVolume(ISS3DSoundSys *sys);
static u8 ISS3DSoundSys_AdjustVolume(ISS3DSoundSys *sys, u8 volume);

ISS3DSoundSys *ISS3DSoundSys_Create(HeapID heapId) {
    ISS3DSoundSys *sys = GFL_HeapAllocate(heapId, sizeof(ISS3DSoundSys), FALSE, "iss_3ds_sys.c", 164);

    sys->heapId = heapId;
    sys->enabled = FALSE;
    sys->targetMasterVolume = SND_VOLUME_MAX;
    sys->masterVolume = SND_VOLUME_MAX;
    sys->masterVolumeStep = 4;
    sys->listenerPos.x = 0;
    sys->listenerPos.y = 0;
    sys->listenerPos.z = 0;
    sys->listenerTarget.x = 0;
    sys->listenerTarget.y = 0;
    sys->listenerTarget.z = 0;
    ISS3DSoundSys_Reset(sys);
    ISS3DSoundSys_Init(sys);
    ISS3DSoundSys_LoadArcData(sys);
    return sys;
}

void ISS3DSoundSys_Free(ISS3DSoundSys *sys) {
    ISS3DSoundSys_FreeEntries(sys);
    GFL_HeapFree(sys);
}

void ISS3DSoundSys_Update(ISS3DSoundSys *sys) {
    ISS3DSoundSys_UpdateCore(sys);
}

void ISS3DSoundSys_Enable(ISS3DSoundSys *sys) {
    ISS3DSoundSys_EnableCore(sys);
}

void ISS3DSoundSys_Disable(ISS3DSoundSys *sys) {
    ISS3DSoundSys_DisableCore(sys);
}

void ISS3DSoundSys_ChangeZone(ISS3DSoundSys *sys, u16 zoneId) {
    ISS3DSoundSys_ChangeZoneCore(sys, zoneId);
}

void ISS3DSoundSys_ReqChangeMasterVolume(ISS3DSoundSys *sys, u8 volume) {
    if (volume > SND_VOLUME_MAX) {
        volume = SND_VOLUME_MAX;
    }
    sys->targetMasterVolume = volume;
}

void ISS3DSoundSys_EnableUnit(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, fx32 range, s32 volume) {
    ISS3DSoundSys_EnableUnitCore(sys, unit, range, volume);
}

BOOL ISS3DSoundSys_IsUnitEnabled(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit) {
    return sys->units[unit].enabled;
}

void ISS3DSoundSys_SetUnitLocation(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, const VecFx32 *pos) {
    ISS3DSoundSys_SetUnitLocationCore(sys, unit, pos);
}

void ISS3DSoundSys_SetListener(ISS3DSoundSys *sys, const VecFx32 *pos, const VecFx32 *target) {
    ISS3DSoundSys_SetListenerCore(sys, pos, target);
}

static void ISS3DSoundSys_Init(ISS3DSoundSys *sys) {
    sys->configs = NULL;
    sys->configCount = 0;
    sys->usedConfigId = ISS_3DS_CONFIG_NONE;
}

static void ISS3DSoundSys_LoadArcData(ISS3DSoundSys *sys) {
    u32 i;
    HeapID heapId = sys->heapId;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ISS_3D_SOUND, HEAPID_TAIL(heapId));
    u8 count = GFL_ArcToolGetDataMax(arc);

    sys->configCount = count;
    sys->configs = GFL_HeapAllocate(heapId, sizeof(ISS3DSoundConfig) * count, FALSE, "iss_3ds_sys.c", 377);
    for (i = 0; i < count; i++) {
        GFL_ArcToolRead(arc, i, &sys->configs[i]);
    }
    GFL_ArcToolFree(arc);
}

static void ISS3DSoundSys_FreeEntries(ISS3DSoundSys *sys) {
    GFL_HeapFree(sys->configs);
    ISS3DSoundSys_Init(sys);
}

static void ISS3DSoundSys_SetUsedConfigID(ISS3DSoundSys *sys, u8 configId) {
    if (sys->usedConfigId != configId) {
        sys->usedConfigId = configId;
    }
}

static u8 ISS3DSoundSys_DecideUsedConfigID(ISS3DSoundSys *sys, u32 bgm) {
    u8 i;
    u8 count;

    if (sys->configs == NULL) {
        return ISS_3DS_CONFIG_NONE;
    }
    count = sys->configCount;
    for (i = 0; i < count; i++) {
        if (bgm == sys->configs[i].bgm) {
            return i;
        }
    }
    return ISS_3DS_CONFIG_NONE;
}

static void ISS3DSoundSys_Reset(ISS3DSoundSys *sys) {
    ISS3DSoundUnitIndex i;

    for (i = 0; i < ISS_3DS_UNIT_COUNT; i++) {
        sys->units[i].enabled = FALSE;
        sys->units[i].volume = SND_VOLUME_MAX;
        sys->units[i].trackMask = 1 << i;
        sys->unitVolume[i] = 0;
        sys->unitPan[i] = 0;
    }
}

static void ISS3DSoundSys_EnableUnitCore(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, fx32 range, s32 volume) {
    ISS3DSoundUnit *u = &sys->units[unit];

    if (u->enabled != TRUE) {
        u->enabled = TRUE;
        u->range = range;
        u->volume = volume;
    }
}

static void ISS3DSoundSys_SetUnitLocationCore(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, const VecFx32 *pos) {
    ISS3DSoundUnit *u = &sys->units[unit];

    if (u->enabled) {
        VEC_Set(&u->pos, pos->x, pos->y, pos->z);
    }
}

static void ISS3DSoundSys_SetListenerCore(ISS3DSoundSys *sys, const VecFx32 *pos, const VecFx32 *target) {
    VEC_Set(&sys->listenerPos, pos->x, pos->y, pos->z);
    VEC_Set(&sys->listenerTarget, target->x, target->y, target->z);
}

static void ISS3DSoundSys_EnableCore(ISS3DSoundSys *sys) {
    if (!sys->enabled) {
        sys->enabled = TRUE;
        ISS3DSoundSys_SetUsedConfigID(sys, ISS3DSoundSys_DecideUsedConfigID(sys, GFL_SndBGMGetID()));
        ISS3DSoundSys_MuteConfig(sys);
    }
}

static void ISS3DSoundSys_DisableCore(ISS3DSoundSys *sys) {
    if (sys->enabled) {
        sys->enabled = FALSE;
    }
}

static void ISS3DSoundSys_UpdateCore(ISS3DSoundSys *sys) {
    ISS3DSoundUnitIndex i;

    if (!sys->enabled) {
        return;
    }
    ISS3DSoundSys_UpdateMasterVolume(sys);
    for (i = 0; i < ISS_3DS_UNIT_COUNT; i++) {
        if (sys->units[i].enabled) {
            ISS3DSoundSys_CalcUnitVolume(sys, i);
            ISS3DSoundSys_CalcUnitPan(sys, i);
            ISS3DSoundSys_CommitVolume(sys, i);
            ISS3DSoundSys_CommitPan(sys, i);
        }
    }
}

static void ISS3DSoundSys_ChangeZoneCore(ISS3DSoundSys *sys, u16 zoneId) {
    ISS3DSoundSys_Reset(sys);
}

static void ISS3DSoundSys_MuteConfig(ISS3DSoundSys *sys) {
    if (sys->usedConfigId != ISS_3DS_CONFIG_NONE) {
        GFL_SndBGMSetVolume(sys->configs[sys->usedConfigId].trackMask, 0);
    }
}

static void ISS3DSoundSys_CalcUnitVolume(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit) {
    s32 volume;
    ISS3DSoundUnit *u = &sys->units[unit];

    if (u->enabled) {
        volume = ISS3DSoundUnit_CalcVolume(u, &sys->listenerPos);
        if (sys->unitVolume[unit] < volume) {
            ISS3DSoundSys_RaiseUnitVolume(sys, unit, volume);
        } else if (volume < sys->unitVolume[unit]) {
            ISS3DSoundSys_LowerUnitVolume(sys, unit, volume);
        }
    }
}

static void ISS3DSoundSys_CalcUnitPan(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit) {
    s32 pan;
    ISS3DSoundUnit *u = &sys->units[unit];

    if (u->enabled) {
        pan = ISS3DSoundUnit_CalcPan(u, &sys->listenerPos, &sys->listenerTarget);
        if (pan < sys->unitPan[unit]) {
            ISS3DSoundSys_LowerUnitPan(sys, unit, pan);
        } else if (sys->unitPan[unit] < pan) {
            ISS3DSoundSys_RaiseUnitPan(sys, unit, pan);
        }
    }
}

static s32 ISS3DSoundUnit_CalcVolume(const ISS3DSoundUnit *unit, const VecFx32 *listenerPos) {
    VecFx32 diff;
    fx32 dist;
    f32 ratio;

    VEC_Subtract(&unit->pos, listenerPos, &diff);
    dist = VEC_Mag(&diff);
    if (unit->range < dist) {
        return 0;
    }
    ratio = FX_FX32_TO_F32(FX_Div(unit->range - dist, unit->range));
    return unit->volume * ratio;
}

static void ISS3DSoundSys_RaiseUnitVolume(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, s32 volume) {
    s32 newVolume = sys->unitVolume[unit] + ISS_3DS_VOLUME_STEP;

    if (volume < newVolume) {
        newVolume = volume;
    }
    sys->unitVolume[unit] = newVolume;
}

static void ISS3DSoundSys_LowerUnitVolume(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, s32 volume) {
    s32 newVolume = sys->unitVolume[unit] - ISS_3DS_VOLUME_STEP;

    if (newVolume < volume) {
        newVolume = volume;
    }
    sys->unitVolume[unit] = newVolume;
}

static void ISS3DSoundSys_CommitVolume(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit) {
    GFL_SndBGMSetVolume(sys->units[unit].trackMask, ISS3DSoundSys_AdjustVolume(sys, sys->unitVolume[unit]));
}

// The pan of a unit: how far it is to the listener's right, from -128 to 127, along the ground
static s32 ISS3DSoundUnit_CalcPan(const ISS3DSoundUnit *unit, const VecFx32 *listenerPos,
                                  const VecFx32 *listenerTarget) {
    VecFx32 toTarget;
    VecFx32 toUnit;
    VecFx32 toUnitFlat;
    VecFx32 up;
    VecFx32 toUnitVertical;
    VecFx32 right;
    VecFx32 vertical;
    VecFx32 forward;
    VecFx32 pos = *listenerPos;
    VecFx32 target = *listenerTarget;
    VecFx32 unitPos = unit->pos;
    fx32 height;
    s32 pan;

    VEC_Subtract(&target, &pos, &toTarget);
    VEC_Subtract(&unitPos, &pos, &toUnit);
    VEC_Set(&up, 0, FX32_ONE, 0);
    VEC_Normalize(&toTarget, &forward);
    VEC_CrossProduct(&forward, &up, &right);
    VEC_Normalize(&right, &right);
    VEC_CrossProduct(&right, &forward, &vertical);
    // The original normalizes it twice
    VEC_Normalize(&vertical, &vertical);
    VEC_Normalize(&vertical, &vertical);
    height = VEC_DotProduct(&toUnit, &vertical);
    toUnitVertical.x = FX_Mul(vertical.x, height);
    toUnitVertical.y = FX_Mul(vertical.y, height);
    toUnitVertical.z = FX_Mul(vertical.z, height);
    VEC_Subtract(&toUnit, &toUnitVertical, &toUnitFlat);
    VEC_Normalize(&toUnitFlat, &toUnitFlat);
    pan = 128.0f * FX_FX32_TO_F32(VEC_DotProduct(&right, &toUnitFlat));
    if (pan < -128) {
        pan = -128;
    }
    if (pan > 127) {
        pan = 127;
    }
    return pan;
}

static void ISS3DSoundSys_LowerUnitPan(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, s32 pan) {
    s32 newPan = sys->unitPan[unit] - ISS_3DS_PAN_STEP;

    if (newPan < pan) {
        newPan = pan;
    }
    sys->unitPan[unit] = newPan;
}

static void ISS3DSoundSys_RaiseUnitPan(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, s32 pan) {
    s32 newPan = sys->unitPan[unit] + ISS_3DS_PAN_STEP;

    if (pan < newPan) {
        newPan = pan;
    }
    sys->unitPan[unit] = newPan;
}

static void ISS3DSoundSys_CommitPan(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit) {
    GFL_SndBGMSetParams(sys->units[unit].trackMask, 256, 0, sys->unitPan[unit]);
}

static void ISS3DSoundSys_UpdateMasterVolume(ISS3DSoundSys *sys) {
    int diff;

    if (sys->masterVolume != sys->targetMasterVolume) {
        diff = sys->targetMasterVolume - sys->masterVolume;
        if ((diff > 0 ? diff : -diff) >= sys->masterVolumeStep) {
            if (diff < 0) {
                diff = -sys->masterVolumeStep;
            } else {
                diff = sys->masterVolumeStep;
            }
        }
        sys->masterVolume += diff;
    }
}

static u8 ISS3DSoundSys_AdjustVolume(ISS3DSoundSys *sys, u8 volume) {
    return (u32)(sys->masterVolume * volume) / SND_VOLUME_MAX;
}
