#include "types.h"
#include "constants/sound.h"
#include "constants/vars.h"
#include "constants/zones.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_exp_obj.h"
#include "field/gym_virbank.h"
#include "gfl/g3d.h"
#include "nitro/fx.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/iss_switch_sys.h"
#include "system/iss_sys.h"

#define GIMMICK_WORK_ID 1
// The archive of the gym's models
#define GIMMICK_ARCID 0x108

#define MODEL_COUNT 3
// How many frames the music takes to fade to a volume
#define VOLUME_FADE_FRAMES 30
// The music has no volume where the player stands
#define NO_VOLUME_INDEX 0xffff

// The music: its switches follow the event variables and its volume where the player stands
typedef struct {
    Field *field;
    ISSSwitchSys *switchSys;
    // Which of the volumes the music has now, or 0xff before the first one
    u8 volumeIndex;
} VirbankMusic;

typedef struct {
    FieldExpObjSystem *expObj;
    VirbankMusic music;
} VirbankWork;

static void func_ov100_021eed80(VirbankWork *work, BOOL hidden);
static void func_ov100_021eee34(VirbankWork *work);
static void func_ov100_021eee40(VirbankWork *work);
static void func_ov100_021eee4c(VirbankMusic *music, Field *field);
static BOOL func_ov100_021eefac(VirbankMusic *music);
static u32 func_ov100_021ef004(VirbankMusic *music);

static const G3DSceneAnimationSetup sAnimations[] = { { 1, 0 }, { 2, 0 } };

// The sizes of the two rooms' grids, by z then x
static const int sGridSizes[2][2] = { { 18, 12 }, { 16, 17 } };

static const G3DSceneResourceSetup sResources[3] = {
    { GIMMICK_ARCID, 0, 0 },
    { GIMMICK_ARCID, 1, 0 },
    { GIMMICK_ARCID, 2, 0 },
};

static const G3DSceneActorSetup sActors[3] = {
    { 0, 0, 0, 0, sAnimations, NELEMS(sAnimations) },
    { 0, 0, 0, 0, sAnimations, NELEMS(sAnimations) },
    { 0, 0, 0, 0, sAnimations, NELEMS(sAnimations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

// The index of the music's volume by where the player stands, by z then x, in each of the two rooms
static const u32 sVolumeIndicesRoom1[18][12] = {
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
    { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
    { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
};

static const u32 sVolumeIndicesRoom2[16][17] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1 },
    { 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1 },
    { 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 },
    { 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
};

void func_ov100_021eec80(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    VirbankWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(VirbankWork));
    BOOL hidden = TRUE;

    work->expObj = Field_GetExpObjSystem(field);
    if (Field_GetPlayerStateZoneID(field) == ZONE_VIRBANK_CITY_GYM_2) {
        hidden = FALSE;
    }
    func_ov100_021eed80(work, hidden);
    func_ov100_021eee4c(&work->music, field);
}

void func_ov100_021eecd0(Field *field) {
    VirbankWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    ISSSwitchSys_ResetMuteStateChangeRequests(work->music.switchSys);
    ISSSwitchSys_ResetMasterVolumeChangeRequest(work->music.switchSys);
    func_ov100_021eee34(work);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void func_ov100_021eecf8(Field *field) {
    VirbankWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    func_ov100_021eee40(work);
    func_ov100_021eefac(&work->music);
}

void func_ov100_021eed10(GameSystem *gsys, int model) {
    VirbankWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    u16 modelId = model;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, modelId, 0);

    FieldExpObj_SetAnm(work->expObj, 0, modelId, 0, FALSE);
    FieldExpObj_SetAnm(work->expObj, 0, modelId, 1, FALSE);
    FieldExpObjAnm_SetPaused(anm, TRUE);
    FieldExpObjAnm_SetLooped(anm, FALSE);
}

void func_ov100_021eed64(GameSystem *gsys, int actorId) {
    FieldActor *actor = FindFieldActor(Field_GetActorSystem(GSYS_GetField(gsys)), actorId);

    SetActorFlag(actor, 0x10);
}

// Places the models and plays their animations, looping and paused
static void func_ov100_021eed80(VirbankWork *work, BOOL hidden) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    u8 i = 0;

    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    const VecFx32 positions[MODEL_COUNT] = {
        { FX32_CONST(29), FX32_CONST(0), FX32_CONST(78) },
        { FX32_CONST(243), FX32_CONST(0), FX32_CONST(78) },
        { FX32_CONST(227), FX32_CONST(18), FX32_CONST(25) },
    };
    for (; i < MODEL_COUNT; i++) {
        matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, i);
        matrix->translation.x = positions[i].x;
        matrix->translation.y = positions[i].y;
        matrix->translation.z = positions[i].z;
        func_ov036_021b8248(work->expObj, 0, i, 1);
        FieldExpObj_SetActorHidden(work->expObj, 0, i, hidden);
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, i, 0);
        FieldExpObj_SetAnm(work->expObj, 0, i, 0, TRUE);
        FieldExpObj_SetAnm(work->expObj, 0, i, 1, TRUE);
        FieldExpObjAnm_SetLooped(anm, TRUE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}

static void func_ov100_021eee34(VirbankWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void func_ov100_021eee40(VirbankWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Unmutes the music's tracks as the event variables say, and sets its volume
static void func_ov100_021eee4c(VirbankMusic *music, Field *field) {
    BOOL unmute0 = TRUE;
    VirbankWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    EventWork *eventWork = GameData_GetEventWork(GSYS_GetGameData(Field_GetGameSystem(field)));
    u16 *var0 = EventWork_GetWkPtr(eventWork, EVENT_WORK_0x4000);
    u16 *var1 = EventWork_GetWkPtr(eventWork, EVENT_WORK_0x4001);
    u16 *var2 = EventWork_GetWkPtr(eventWork, EVENT_WORK_0x4002);
    u16 *var3 = EventWork_GetWkPtr(eventWork, EVENT_WORK_0x4003);
    BOOL unmute1;
    u8 i;
    FieldExpObjAnm *anm;
    u32 volumeIndex;

    music->field = field;
    music->switchSys = ISS_GetSwitchSys(GameSystem_GetISS(Field_GetGameSystem(field)));
    music->volumeIndex = 0xff;
    if (*var0 != 0) {
        unmute0 = FALSE;
    }
    unmute1 = (*var1 == 0) ? TRUE : FALSE;
    BOOL unmute2 = (*var2 == 0) ? TRUE : FALSE;
    BOOL unmute3 = (*var3 == 0) ? TRUE : FALSE;
    ISSSwitch_ReqSwitchMuteStateChange(music->switchSys, 3, unmute2, SEQ_BGM_POISONE_GYM_01);
    ISSSwitch_ReqSwitchMuteStateChange(music->switchSys, 2, unmute1, SEQ_BGM_POISONE_GYM_01);
    ISSSwitch_ReqSwitchMuteStateChange(music->switchSys, 1, unmute0, SEQ_BGM_POISONE_GYM_01);
    ISSSwitch_ReqSwitchMuteStateChange(music->switchSys, 4, unmute3, SEQ_BGM_POISONE_GYM_01);
    if (*var2 == 1) {
        for (i = 0; i < MODEL_COUNT; i++) {
            anm = FieldExpObj_GetAnmInfo(work->expObj, 0, i, 0);
            FieldExpObj_SetAnm(work->expObj, 0, i, 0, FALSE);
            FieldExpObj_SetAnm(work->expObj, 0, i, 1, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
            FieldExpObjAnm_SetLooped(anm, FALSE);
        }
    }
    const int volumes[4] = { 127, 78, 38, 12 };
    volumeIndex = func_ov100_021ef004(music);
    if (volumeIndex != NO_VOLUME_INDEX) {
        ISSSwitchSys_ReqMasterVolumeChange(music->switchSys, volumes[volumeIndex], VOLUME_FADE_FRAMES,
                                           SEQ_BGM_POISONE_GYM_01);
    }
}

// Fades the music to the volume of where the player stands, if it has changed
static BOOL func_ov100_021eefac(VirbankMusic *music) {
    FieldPlayer *player = Field_GetPlayer(music->field);
    u32 volumeIndex = func_ov100_021ef004(music);

    if (volumeIndex == NO_VOLUME_INDEX) {
        return FALSE;
    }
    if (volumeIndex != music->volumeIndex) {
        const int volumes[4] = { 127, 78, 38, 12 };

        music->volumeIndex = volumeIndex;
        ISSSwitchSys_SeqMasterVolumeFade(music->switchSys, volumes[music->volumeIndex], VOLUME_FADE_FRAMES);
        return TRUE;
    }
    return FALSE;
}

// Which volume the player's place has, or NO_VOLUME_INDEX outside the rooms' grids
static u32 func_ov100_021ef004(VirbankMusic *music) {
    s16 x;
    s16 y;
    s16 z;
    FieldPlayer *player = Field_GetPlayer(music->field);
    u32 volumeIndex = 0;

    FieldPlayer_GetGPos(player, &x, &y, &z);
    switch (Field_GetPlayerStateZoneID(music->field)) {
    case ZONE_VIRBANK_CITY_GYM:
        if (z >= sGridSizes[0][0] || x >= sGridSizes[0][1]) {
            return NO_VOLUME_INDEX;
        }
        volumeIndex = sVolumeIndicesRoom1[z][x];
        break;
    case ZONE_VIRBANK_CITY_GYM_2:
        if (z >= sGridSizes[1][0] || x >= sGridSizes[1][1]) {
            return NO_VOLUME_INDEX;
        }
        volumeIndex = sVolumeIndicesRoom2[z][x];
        break;
    }
    return volumeIndex;
}
