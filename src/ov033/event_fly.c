#include "types.h"
#include "field/event_fly.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/encounter_effect.h"
#include "field/field_event.h"
#include "field/field_nogrid_mapper.h"
#include "field/zone.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/std.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

struct FlyEventWork {
    GameSystem *gsys;
    void *bind;
    u32 zoneId;
    ZoneSpawnInfo spawn;
    // The field effects of flying off and of landing, by the player's sex
    u32 effectOut;
    u32 effectIn;
};

GameEvent *func_ov033_02178908(GameSystem *gsys, Field *field, u32 zoneId) {
    GameEvent *event;
    FlyEventWork *work;

    event = GameEvent_Create(gsys, NULL, EventFly_Callback, sizeof(FlyEventWork));
    work = GameEvent_GetData(event);
    sys_memset(work, 0, sizeof(FlyEventWork));
    work->gsys = gsys;
    work->zoneId = zoneId;
    if (getTrainerGender(GetGameDataPlayerInfo(GSYS_GetGameData(gsys))) == 0) {
        work->effectOut = 11;
        work->effectIn = 12;
    } else {
        work->effectOut = 31;
        work->effectIn = 32;
    }
    LoadZoneSpawnInfoCheckRail(&work->spawn, zoneId);
    return event;
}

GameEventReturnCode EventFly_Callback(GameEvent *event, u32 *state, void *data) {
    FlyEventWork *work = data;
    Field *field = GSYS_GetField(work->gsys);
    FieldCamera *camera = Field_GetCameraSystem(field);
    void *g3dCi = Field_Get3DCi(field);
    NoGridMapper *mapper = Field_GetNoGridMapper(field);
    GameEvent *next;
    FieldActor *actor;
    u32 enabled;

    switch (*state) {
    case 0:
        FieldCamera_SetUseBoundaryEnable(camera, FALSE);
        work->bind = FieldCamera_GetBind(camera);
        FieldCamera_FinishDelay(camera);
        (*state)++;
        break;
    case 1: {
        VecFx32 takeoffPosition;
        FieldEvCameraAnimationSetup takeoffSetup;

        if (work->bind != NULL && FieldCamera_IsDelayActive(camera)) {
            break;
        }
        FieldNoGridMapper_SetCameraAreaEnabled(mapper, FALSE);
        FieldCamera_ClearBind(camera);
        FieldCamera_EVCameraInit(camera);
        FieldPlayer_GetWPos(Field_GetPlayer(field), &takeoffPosition);
        takeoffSetup.targetCoords.pitch = 0x25d8;
        takeoffSetup.targetCoords.yaw = 0;
        takeoffSetup.targetCoords.distance = 0xed000;
        takeoffSetup.targetCoords.extraTranslation.x = 0;
        takeoffSetup.targetCoords.extraTranslation.y = 4 * FX32_ONE;
        takeoffSetup.targetCoords.extraTranslation.z = 0;
        takeoffSetup.targetCoords.targetPos = takeoffPosition;
        takeoffSetup.targetCoords.fov = 0xe38;
        takeoffSetup.flags.animateExtraTranslation = TRUE;
        takeoffSetup.flags.animatePitch = TRUE;
        takeoffSetup.flags.animateYaw = FALSE;
        takeoffSetup.flags.animateTargetDistance = TRUE;
        takeoffSetup.flags.animateFOV = TRUE;
        takeoffSetup.flags.animateTargetPos = TRUE;
        FieldCameraAnm_SetAnimation(camera, &takeoffSetup, 10);
        (*state)++;
    }
    case 2:
        if (FieldCamera_IsAnimating(camera)) {
            break;
        }
        DisableAllActorsMovement(Field_GetActorSystem(field));
        next = EventFieldEffect_Create(work->gsys, g3dCi, work->effectOut);
        if (next != NULL) {
            GameEvent_ChainNext(event, next);
        }
        (*state)++;
        break;
    case 3:
        EnableAllActorsMovement(Field_GetActorSystem(field));
        SetActorHidden(FieldPlayer_GetActor(Field_GetPlayer(field)), TRUE);
        FieldCameraAnm_EVCameraEnd(camera);
        GameEvent_ChainNext(event, func_ov036_021b8890(work->gsys, field, 0, 0));
        (*state)++;
        break;
    case 4:
        GameEvent_ChainNext(event, EventMapChange_CreateForFly(work->gsys, field, (u16)work->zoneId, &work->spawn, 1));
        (*state)++;
        break;
    case 5:
        func_ov036_021b5168(Field_GetPlaceName(field));
        work->bind = FieldCamera_GetBind(camera);
        FieldCamera_FinishDelay(camera);
        (*state)++;
        break;
    case 6: {
        VecFx32 landingPosition;
        FieldEvCameraAnimationSetup landingSetup;

        if (work->bind != NULL && FieldCamera_IsDelayActive(camera)) {
            break;
        }
        FieldNoGridMapper_SetCameraAreaEnabled(mapper, FALSE);
        FieldCamera_ClearBind(camera);
        FieldCameraAnm_EnsureInitDone(camera);
        FieldCamera_SetUseBoundaryEnable(camera, FALSE);
        FieldPlayer_GetWPos(Field_GetPlayer(field), &landingPosition);
        landingSetup.targetCoords.pitch = 0x25d8;
        landingSetup.targetCoords.yaw = 0;
        landingSetup.targetCoords.distance = 0xed000;
        landingSetup.targetCoords.extraTranslation.x = 0;
        landingSetup.targetCoords.extraTranslation.y = 4 * FX32_ONE;
        landingSetup.targetCoords.extraTranslation.z = 0;
        landingSetup.targetCoords.targetPos = landingPosition;
        landingSetup.targetCoords.fov = 0xe38;
        landingSetup.flags.animateExtraTranslation = TRUE;
        landingSetup.flags.animatePitch = TRUE;
        landingSetup.flags.animateYaw = FALSE;
        landingSetup.flags.animateTargetDistance = TRUE;
        landingSetup.flags.animateFOV = TRUE;
        landingSetup.flags.animateTargetPos = TRUE;
        FieldCameraAnm_SetAnimation(camera, &landingSetup, 1);
        (*state)++;
    }
    case 7:
        if (FieldCamera_IsAnimating(camera)) {
            break;
        }
        SetActorHidden(FieldPlayer_GetActor(Field_GetPlayer(field)), TRUE);
        enabled = GFL_BGSysGetEnabledBGsA();
        FieldG2D_SetLCDConfig();
        GFL_BGSysSetEnabledBGsA(enabled);
        FieldG2D_Prepare3DSurface(field);
        (*state)++;
        break;
    case 8:
        DisableAllActorsMovement(Field_GetActorSystem(field));
        next = EventFieldEffect_Create(work->gsys, g3dCi, work->effectIn);
        if (next != NULL) {
            GameEvent_ChainNext(event, next);
        } else {
            actor = FieldPlayer_GetActor(Field_GetPlayer(field));
            SetActorHidden(actor, FALSE);
            CheckSetActorFaceDir(actor, 1);
            next = GameEvent_Create(work->gsys, event, func_ov033_02178c6c, 0);
            GameEvent_ChainNext(event, next);
        }
        (*state)++;
        break;
    case 9: {
        FieldEvCameraAnimationFlags returnFlags;

        EnableAllActorsMovement(Field_GetActorSystem(field));
        func_ov036_021b50f4(Field_GetPlaceName(field), work->zoneId);
        returnFlags.animateExtraTranslation = TRUE;
        returnFlags.animatePitch = TRUE;
        returnFlags.animateYaw = FALSE;
        returnFlags.animateTargetDistance = TRUE;
        returnFlags.animateFOV = TRUE;
        returnFlags.animateTargetPos = TRUE;
        FieldCameraAnm_SetReturnAnimation(camera, &returnFlags, 10);
        (*state)++;
    }
    case 10:
        if (FieldCamera_IsAnimating(camera)) {
            break;
        }
        if (work->bind != NULL) {
            FieldCamera_SetBind(camera, work->bind);
        }
        FieldCamera_EnableDelay(camera);
        FieldNoGridMapper_SetCameraAreaEnabled(mapper, TRUE);
        FieldCameraAnm_EVCameraEnd(camera);
        FieldCamera_SetUseBoundaryEnable(camera, TRUE);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode func_ov033_02178c6c(GameEvent *event, u32 *state, void *data) {
    GameSystem *gsys;

    gsys = GameEvent_GetGameSystem(event);
    GSYS_GetField(gsys);
    switch (*state) {
    case 0:
        GFL_FadeSet(3, 16, 0, 0);
        ++*state;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}
