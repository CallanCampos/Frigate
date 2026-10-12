#include "types.h"
#include "constants/sound.h"
#include "field/encounter.h"
#include "field/encounter_effect.h"
#include "field/event_data.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_3dci.h"
#include "field/field_actor.h"
#include "field/field_async_proc.h"
#include "field/field_camera.h"
#include "field/field_controller.h"
#include "field/field_daycare.h"
#include "field/field_disp_control.h"
#include "field/field_effect.h"
#include "field/field_environment.h"
#include "field/field_event.h"
#include "field/field_exp_obj.h"
#include "field/field_fog.h"
#include "field/field_g3d_mapper.h"
#include "field/field_g3dobj.h"
#include "field/field_internal.h"
#include "field/field_lens_flare.h"
#include "field/field_map.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_palace.h"
#include "field/field_player.h"
#include "field/field_pokemon_form.h"
#include "field/field_prop.h"
#include "field/field_rail.h"
#include "field/field_scene_area.h"
#include "field/field_script.h"
#include "field/field_script_plugin.h"
#include "field/field_skill_map_eff.h"
#include "field/field_sound.h"
#include "field/field_status.h"
#include "field/field_task.h"
#include "field/field_weather.h"
#include "field/fieldmap_ctrl_hybrid.h"
#include "field/intrude_work.h"
#include "field/medal.h"
#include "field/player_state.h"
#include "field/pleasure_boat.h"
#include "field/resort_mapcreate.h"
#include "field/scrcmd_ochiba.h"
#include "field/subscreen.h"
#include "field/union_eff.h"
#include "field/union_main.h"
#include "field/wbt.h"
#include "field/zone.h"
#include "field/zone_change.h"
#include "gfl/arc.h"
#include "gfl/blact.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/rtc_cache.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "gfl/wm_icon.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "save/box.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/area_data.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/iss_sys.h"
#include "system/main.h"
#include "system/resort_work.h"
#include "system/rtc.h"
#include "system/season.h"
#include "system/zone_weather.h"

static const u8 sResolvedControllerTypes[4] = { 0, 1, 2, 0 };

// A kind of map: the setup of its mapper and its controller, whether it takes its chunks from the zone's map matrix,
// and the size of its heap
typedef struct {
    FieldG3DMapperConfig mapperConfig;
    const FieldmapCtrlVTable *ctrlVTable;
    BOOL useMapMatrix;
    u32 heapSize;
} FieldMapConfig;

extern const FieldMapConfig MAP_CONFIGS[];

// A step of the field: 0 to run again next frame, 1 to go on to the next routine, 2 when the field ends
typedef u32 (*FieldRoutine)(GameSystem *gsys, Field *field);
typedef void (*FieldRenderFunc)(Field *field);

extern const FieldRoutine FIELDMAP_ROUTINES[][2];
extern const FieldRenderFunc FIELD_RENDER_FUNCS_PHASE1[];
extern const FieldRenderFunc FIELD_RENDER_FUNCS_PHASE2[];
extern const BGSysVRAMConfig FIELDMAP_VRAM_CONFIG;
extern const ClActSysSetup data_ov036_021c9f20;
extern const BGSysLCDConfig FIELD_LCD_CONFIG;
extern u8 FIELD_STD_PALETTE[];
extern const u32 FIELD_DEFAULT_FOG_TABLE[];
extern const GXRgb FIELD_DEFAULT_EDGE_COLOR_TABLE[];
extern const LightSetupList FIELD_LIGHT_SETUP;
extern const VecFx32 DEFAULT_FIELDCAMERA_POS;
extern const VecFx32 DEFAULT_FIELDCAMERA_TGT;
extern const VecFx32 DEFAULT_FIELDCAMERA_UP;
extern const VecFx32 data_ov036_021ca010;
// The edge colors of the area, loaded from archive 15
static GXRgb g_FieldEdgeColorTable[8];
// The memory of the field's heap 0x89
extern u8 data_ov036_021d57a0[];

u32 FieldRoutine_Open(GameSystem *gsys, Field *field);
u32 FieldRoutine_MapLoad(GameSystem *gsys, Field *field);
u32 FieldRoutine_MapSpawnFinish(GameSystem *gsys, Field *field);
u32 FieldRoutine_Run(GameSystem *gsys, Field *field);
u32 FieldRoutine_RunAltFrame(GameSystem *gsys, Field *field);
u32 FieldRoutine_MapUnload(GameSystem *gsys, Field *field);
u32 FieldRoutine_Close(GameSystem *gsys, Field *field);

BOOL FieldmapProc_Init(GameProc *proc, int *seq, void *param, void *work) {
    GameSystem *gsys = param;

    switch (*seq) {
    case 0: {
        Field **procWork;
        u32 heapSize = GetFieldmapZoneHeapSize(PlayerState_GetZoneID(GSYS_GetPlayerState(gsys))) - 0xf000;

        GFL_HeapCreateChild(HEAPID_USER, HEAPID_FIELDMAP, heapSize);
        GFL_HeapCreateChild(HEAPID_USER, 0x70, 0xf000);
        GFL_HeapCreateChild(HEAPID_FIELDMAP, 0x50, 0xc000);
        GFL_HeapCreateRoot(data_ov036_021d57a0, 0x10000, 0x89);
        GFL_HeapCreateChild(0x89, 0x92, 0x6400);
        GFL_HeapCreateChild(0x89, 0x93, 0x7400);
        GFL_HeapCreateChild(0x89, 0x96, 0x500);
        procWork = GFL_ProcInitSubsystem(proc, sizeof(Field *), HEAPID_FIELDMAP);
        *procWork = Field_Create(gsys, HEAPID_FIELDMAP);
        GSYS_SetField(gsys, *procWork);
        (*seq)++;
        break;
    }
    case 1:
        if (!func_02016b34(gsys)) {
            GameCommSys *comm = GSYS_GetGameCommSystem(gsys);

            switch (GameCommSys_BootCheck(comm)) {
            case 1:
            case 2:
                GameCommSys_ExitReq(comm);
                break;
            }
        }
        CheckResetKeldeoOrdinaryForme(gsys);
        return TRUE;
    }
    return FALSE;
}

BOOL FieldmapProc_Update(GameProc *proc, int *seq, void *param, void *work) {
    if (Field_CallRoutines(param, *(Field **)work) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldmapProc_End(GameProc *proc, int *seq, void *param, void *work) {
    GameSystem *gsys = param;

    Field_Free(*(Field **)work);
    GSYS_SetField(gsys, NULL);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(0x96);
    GFL_HeapDelete(0x92);
    GFL_HeapDelete(0x93);
    GFL_HeapDelete(0x89);
    GFL_HeapDelete(0x50);
    GFL_HeapDelete(0x70);
    GFL_HeapDelete(HEAPID_FIELDMAP);
    CheckResetKeldeoOrdinaryForme(gsys);
    return TRUE;
}

Field *Field_Create(GameSystem *gsys, HeapID heapId) {
    Field *field = GFL_HeapAllocate(heapId, sizeof(Field), TRUE, "fieldmap.c", 0x22b);
    u32 season = 0;
    u16 areaId;

    field->heapId = heapId;
    field->routineState = 0;
    field->gameSystem = gsys;
    field->gameData = GSYS_GetGameData(gsys);
    field->zoneId = PlayerState_GetZoneID(GSYS_GetPlayerState(gsys));
    areaId = ZoneData_GetAreaID(field->zoneId);
    if (AreaData_HasSeasons(areaId)) {
        season = GameSystem_GetSeason(gsys);
    } else if (func_02018f60(areaId)) {
        if (!func_ov036_0218141c(PlayerState_GetZoneID(GSYS_GetPlayerState(gsys)))) {
            season = func_02010378(getKeyDataBlkAddress(GameData_GetSaveControl(field->gameData)));
        }
    } else if (func_02018f78(areaId)) {
        season = func_02017b70(field->gameData);
    } else if (func_02018f90(areaId)) {
        WbtSystem **wbt = func_020179f0(field->gameData);

        if (wbt != NULL && *wbt != NULL) {
            season = func_ov036_021c98f4(*wbt);
        }
    }
    field->areaData = AreaData_Create(heapId, areaId, season);
    ResortWork_UpdateRecords(func_02017b84(field->gameData), GameData_GetSaveControl(field->gameData));
    field->ctrlVTable = GetZoneFieldmapCtrlVTable(field->zoneId);
    return field;
}

void Field_Free(Field *field) {
    GameSystem *gsys = field->gameSystem;

    AreaData_Free(field->areaData);
    GFL_HeapFree(field);
    GSYS_SetField(gsys, NULL);
    func_0203e874(0, 0);
}

u32 FieldRoutine_Open(GameSystem *gsys, Field *field) {
    HeapID heapId = field->heapId;

    GFL_BGSysSetVRAMBanks(&FIELDMAP_VRAM_CONFIG);
    FieldG2D_Init(field);
    ClActSys_Create(&data_ov036_021c9f20, &FIELDMAP_VRAM_CONFIG, 0x89);
    GFL_BGSysSetBGEnabledA(0x10, TRUE);
    GFL_BGSysSetBGEnabledB(0x10, TRUE);
    BmpWin_InitAllocator(heapId);
    func_020232d0();
    GFL_G3DSysCreate(0, 3, 0, 1, FX32_ONE, field->heapId, FieldG3D_InitCallback);
    FieldG3D_Init(field);
    field->g3DMapper = FieldG3DMapper_Create(field->heapId, GameData_GetSeason(field->gameData));
    field->asyncActorMatLoadTCB = GFL_VBlankTCBAdd(FldActSys_AsyncMatLoadTCBFunc, field, 0);
    field->tcbManagerHeap = GFL_HeapAllocate(heapId, GFL_TCBMgrCalcAllocSize(0x20), FALSE, "fieldmap.c", 0x2bf);
    field->tcbManager = GFL_TCBMgrCreate(0x20, field->tcbManagerHeap);
    field->taskManager = FieldTaskManager_Create(10, field->heapId);
    field->dispControl = FieldDispControl_Create(field->heapId);
    if (GetZoneIsUnionRoom(field->zoneId) == TRUE || IsZone150Or151(field->zoneId) == TRUE) {
        GFL_OvlLoad(OVERLAY_ID(34));
    } else {
        GFL_OvlLoad(OVERLAY_ID(33));
    }
    GCTX_HIDChangeFPS(30);
    return 1;
}

u32 FieldRoutine_MapLoad(GameSystem *gsys, Field *field) {
    GameData *gameData = GSYS_GetGameData(gsys);
    BOOL loading = TRUE;

    switch (field->subroutinePhase) {
    case 0:
        func_ov012_0215ef28(gameData, field->zoneId);
        field->msgBGSys = CreateFieldMsgBGSystem(field->heapId, field->g3dCamera);
        field->moneyWin = NULL;
        field->placeName = FieldPlaceName_Create(gsys, 0x93, field->msgBGSys);
        field->unk34 = func_ov036_021b5b7c(gsys, field->heapId);
        field->skillMapEff = FieldSkillMapEff_Create(FieldStatus_GetFlashPerms(GameData_GetFieldStatus(field->gameData)),
                                                     field->heapId);
        field->cameraSystem = FieldCamera_Create(GetZoneDefaultCameraIndex(field->zoneId), 0, field->g3dCamera,
                                                 &field->playerPos, field->heapId);
        field->sceneArea = FieldSceneArea_Create(field->heapId, field->cameraSystem, field);
        field->sceneAreaLoader = FieldSceneAreaLoader_Create(field->heapId);
        break;
    case 1: {
        FieldPropSystem *propSystem;

        field->noGridMapper = FieldNoGridMapper_Create(field->heapId, field->cameraSystem, field->sceneArea,
                                                       field->sceneAreaLoader);
        field->palaceSys = FieldPalaceSys_Create(field->heapId, gsys, field, field->zoneId);
        FieldColorPostFX_Set(field->colorPostFX, FieldPalaceSys_GetLuminanceTable(field->palaceSys),
                             IsZoneFlashbackMemoryPostFX(field->zoneId));
        propSystem = FieldG3DMapper_GetBMSystem(field->g3DMapper);
        SetupLoadZoneMapTypeData(field->zoneId, field->areaData, &field->mapperConfig,
                                 GetMapMatrixSystem(field->gameData));
        FieldPropSystem_LoadArea(propSystem, field->zoneId, field->areaData, field->colorPostFX);
        FieldG3DObjSystem_SetColorPostFX(field->g3DObjSystem, field->colorPostFX);
        Field_LoadWFBC(gameData, field, field->zoneId);
        Field_LoadJoinAvenue(gameData, field, field->zoneId);
        break;
    }
    case 2: {
        u32 railId;

        SetupZoneWarpArrivalGrid(&field->playerZoneState, field->zoneId, 0, 0, 0, field->playerPos.x, field->playerPos.y,
                                 field->playerPos.z);
        FieldG3DMapper_CreateMap(field->g3DMapper, &field->mapperConfig, field->colorPostFX);
        railId = GetRailIDForZone(field->zoneId);
        if (railId != 0xffffffff) {
            FieldNoGridMapper_LoadByHeader(field->noGridMapper, railId, field->heapId);
        }
        field->objectProjectionMatrixOffset = GetObjectProjectionMatrixOffset(field->zoneId);
        FieldCameraBoundary_ChangeZone(field, field->zoneId, field->heapId);
        field->fesGimmick = FesGimmick_Create(gsys, field, field->heapId);
        break;
    }
    case 3:
        Field_InitActorSystem(field);
        field->fieldEffects = FieldEffects_Create(field, 0x19, field->heapId);
        FieldEffects_TCBManagerInit(field->fieldEffects, 0x5e);
        FieldEffects_SetLuminanceTable(field->fieldEffects, FieldPalaceSys_GetLuminanceTable(field->palaceSys));
        FieldEffects_Load(field->fieldEffects, STATIC_LOADED_FIELD_EFFECT_IDS, data_ov036_021d0388);
        if (GetZoneIsUnionRoom(field->zoneId) == TRUE || IsZone150Or151(field->zoneId) == TRUE) {
            field->unkA0 = UnionEff_Create(field->heapId);
        }
        break;
    case 4: {
        PlayerState *playerState = GSYS_GetPlayerState(gsys);
        u16 dir = PlayerState_CalcDirection(playerState);
        VecFx32 *pos = &playerState->position;
        u8 season;

        if (FieldPlayerState_GetExState(playerState) == 1 && !GetZoneFlagsEnableCycling(field->zoneId)) {
            SetPlayerSpecialState(playerState, 0);
        }
        field->player = FieldPlayer_Create(playerState, field, pos, getTrainerGender(GetGameDataPlayerInfo(gameData)),
                                           field->heapId);
        field->playerPos = *pos;
        field->ctrlVTable->create(field, &field->playerPos, dir);
        FieldG3DMapper_SetPlayerPos(field->g3DMapper, &field->playerPos);
        func_ov036_02180fe4(&field->gimmickWork);
        field->expObjSystem = FieldExpObj_Create(0x3c, 0x60, field->heapId);
        Field_LoadEdgeColorTable(field->areaData, Field_GetPlayerStateZoneID(field));
        field->fog = FieldFog_Create(field->heapId);
        field->fogCtrl = FieldFogCtrl_Create(field->heapId);
        FieldFogCtrl_RequestLoad(field->fogCtrl, GetZoneFogIndexAll(field, field->zoneId),
                                 GetZoneStaticLightDataIndex(field->zoneId), TRUE);
        {
            u32 lightsId = AreaData_GetLightsID(field->areaData);
            u32 daySeconds = RTC_ConvertDaySecondsCached();

            field->lightSystem = FieldLight_Create(lightsId, daySeconds, GameSystem_GetSeason(gsys), field->fog,
                                                   field->g3dLights, field->heapId);
        }
        field->weatherSystem =
            func_ov036_02199004(field->cameraSystem, field->lightSystem, field->fog, field->fogCtrl,
                                GameData_GetFieldSoundSystem(field->gameData), GameSystem_GetSeason(gsys), 0x92);
        SetWeatherInit(field->weatherSystem, GetNowWeather(field->gameData), 0x92);
        Field_CheckGiveDiamondDustMedal(field);
        season = GameData_GetSeason(field->gameData);
        field->lensFlare = FieldLensFlare_Create(gsys, field->gameData, field->expObjSystem, season,
                                                 Field_GetDayPeriod(field), field->heapId);
        Field_InitGimmick(field);
        SetScrPluginByZone(field->gameData, Field_GetPlayerStateZoneID(field));
        LoadScrPluginOverlays(field->gameData);
        break;
    }
    case 5:
        field->subscreen = FieldSubscreen_Create(field->heapId, field, GameData_GetLastSubscreen(gameData));
        break;
    case 6:
        field->encountSystem = EncSys_Create(field);
        GameCommSys_FieldCreate(GSYS_GetGameCommSystem(gsys), field);
        field->asyncProcManager = FieldAsyncProcManager_Create(field, field->heapId, 0x20);
        field->particleSystem = func_ov036_021bb590(0x50);
        field->g3dCi = Fld3DCi_Create(0x50, field->particleSystem);
        field->encEff = EncEff_Create(field->heapId, field);
        FesGimmick_BindPlayer(field->fesGimmick, field->fieldEffects, field->player);
        field->dayCare = DayCare_Create(field->heapId, field, getDaycareBlockAddress(GameData_GetSaveControl(field->gameData)));
        Field_LoadSceneArea(field, field->zoneId);
        loading = FALSE;
        break;
    }
    field->subroutinePhase++;
    field->renderMode = 0;
    if (!loading) {
        return 1;
    }
    return 0;
}

u32 FieldRoutine_MapSpawnFinish(GameSystem *gsys, Field *field) {
    switch (field->subroutinePhase) {
    case 0:
        FieldG3DMapper_UpdateSync(field->g3DMapper);
        if (field->actorSystem != NULL) {
            do {
                FldActSys_Update(field->actorSystem);
                FldActSys_FinishAsyncMatLoad(field->actorSystem);
            } while (FldActSys_IsAsyncLoadPending(field->actorSystem) == TRUE);
        }
        while (func_ov036_0219917c(field->weatherSystem)) {
            FieldWeather_Update(field->weatherSystem, 0x92);
        }
        FieldG3D_Update(field);
        FieldG3D_RenderPhase1(field);
        FieldG3D_RenderPhase2(field);
        func_0204b794();
        if (field->msgBGSys != NULL) {
            func_ov036_021878d0(field->msgBGSys);
        }
        FieldEffects_Update(field->fieldEffects);
        FieldAsyncProcManager_Update(field->asyncProcManager);
        field->subroutinePhase++;
    case 1:
        if (FieldStatus_CheckContinueFlag(GameData_GetFieldStatus(field->gameData)) == TRUE
            && IsZoneTwoPassLoad(field->zoneId) == TRUE && func_02005cbc()) {
            return 0;
        }
        if (FieldStatus_GetNewLoadFlag(GameData_GetFieldStatus(field->gameData))) {
            FieldScript_CallOnZoneReload(field->gameSystem, field->heapId);
            FieldStatus_SetNewLoadFlag(GameData_GetFieldStatus(field->gameData), FALSE);
        } else {
            FieldScript_CallOnZoneNewLoad(field->gameSystem, field->heapId);
        }
        FieldStatus_SetContinueFlag(GameData_GetFieldStatus(field->gameData), FALSE);
        FieldG3DMapper_UpdateSync(field->g3DMapper);
        func_ov036_021b6660(field->fesGimmick);
        if (GetZoneIsUnionRoom(field->zoneId) == TRUE || IsZone150Or151(field->zoneId) == TRUE) {
            GSYS_SetEventProvider(gsys, FieldEventProvider_UnionRoom, field);
        } else if (Field_GetControllerTypeID(field) == 1) {
            GSYS_SetEventProvider(gsys, FieldEventProvider_NoGrid, field);
        } else if (Field_GetControllerTypeID(field) == 2) {
            GSYS_SetEventProvider(gsys, FieldEventProvider_Hybrid, field);
        } else {
            GSYS_SetEventProvider(gsys, FieldEventProvider_Grid, field);
        }
        field->routineState = 1;
        GameData_Set30FPSMode(GSYS_GetGameData(gsys), TRUE);
        break;
    }
    return 1;
}

u32 FieldRoutine_Run(GameSystem *gsys, Field *field) {
    if (GSYS_GetNowEvent(field->gameSystem) == NULL) {
        FieldStatus_SetBusyFlag(GameData_GetFieldStatus(field->gameData), FALSE);
    }
    if (GameData_CheckEventsPaused(field->gameData)) {
        return 0;
    }
    if (field->effectRunningFlag) {
        Field_RenderStart(field);
        func_0204b794();
        return 0;
    }
    GCTX_HIDResetFrameCount();
    Field_CheckDoRealTimeLoad(field);
    if (!GSYS_GetEventRunningFlag(gsys)) {
        field->ctrlVTable->update(field, &field->playerPos);
        func_ov012_02162f44(field->gameData);
    }
    if (field->placeName != NULL) {
        func_ov036_021b4ff8(field->placeName);
    }
    if (field->unk34 != NULL) {
        func_ov036_021b5c28(field->unk34);
    }
    FieldSkillMapEff_Update(field->skillMapEff);
    FieldPlayer_SyncState(field->player);
    if (GetZoneIsUnionRoom(field->zoneId) == TRUE || IsZone150Or151(field->zoneId) == TRUE) {
        UnionMain_Update(GSYS_GetGameCommSystem(gsys), field);
    }
    FieldSubscreen_Update(field->subscreen);
    if (field->actorSystem != NULL) {
        MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
        FldActSys_Update(field->actorSystem);
        MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    }
    Field_UpdateGimmick(field);
    FieldEffects_Update(field->fieldEffects);
    if (field->unkA0 != NULL) {
        UnionEff_Update(field->unkA0);
    }
    FieldAsyncProcManager_Update(field->asyncProcManager);
    if (field->msgBGSys != NULL) {
        func_ov036_021878d0(field->msgBGSys);
    }
    GFL_TCBMgrUpdate(field->tcbManager);
    FieldTaskManager_Update(field->taskManager);
    if (field->playerPosPtr != NULL) {
        field->playerPos = *field->playerPosPtr;
    }
    FieldG3DMapper_SetPlayerPos(field->g3DMapper, &field->playerPos);
    FieldG3D_Update(field);
    func_ov036_021c20e0(*GameData_GetPleasureBoatPtr(field->gameData));
    func_0204b794();
    Field_RenderStart(field);
    return 0;
}

void Field_RenderStart(Field *field) {
    GFL_G3DSysResetGeometryCounter();
    FieldCamera_CalcTransform(field->cameraSystem, GCTX_HIDGetHeldKeys());
    field->actorYOffset = func_ov036_0218132c(field);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    FieldG3D_RenderPhase1(field);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
}

u32 FieldRoutine_RunAltFrame(GameSystem *gsys, Field *field) {
    if (GameData_CheckEventsPaused(field->gameData)) {
        GameData_ResetSkipFrame(GSYS_GetGameData(gsys));
        return 0;
    }
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    FieldG3DMapper_CallAllLoaderUpdate(field->g3DMapper);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    FieldSubscreen_UpdateAltFrame(field->subscreen);
    if (field->msgBGSys != NULL) {
        func_ov036_021878d0(field->msgBGSys);
    }
    if (field->placeName != NULL) {
        func_ov036_021b5064(field->placeName);
    }
    if (field->unk34 != NULL) {
        func_ov036_021b5c78(field->unk34);
    }
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    FieldG3D_RenderPhase2(field);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    GameData_ResetSkipFrame(GSYS_GetGameData(gsys));
    return 0;
}

u32 FieldRoutine_MapUnload(GameSystem *gsys, Field *field) {
    GameData *gameData;

    func_ov036_021b65d4(field->fesGimmick);
    EncEff_Free(field->encEff);
    Fld3DCi_Free(field->g3dCi);
    func_ov036_021bb658(field->particleSystem);
    GSYS_SetEventProvider(gsys, NULL, NULL);
    FieldPlayer_SyncState(field->player);
    FieldAsyncProcManager_Free(field->asyncProcManager);
    GameCommSys_FieldDelete(GSYS_GetGameCommSystem(gsys), field);
    if (field->unk34 != NULL) {
        func_ov036_021b5bfc(field->unk34);
    }
    if (field->placeName != NULL) {
        FieldPlaceName_Free(field->placeName);
    }
    UnloadScrPluginOverlays(field->gameData);
    Field_TerminateGimmick(field);
    FieldLensFlare_Free(field->lensFlare);
    FieldExpObj_Free(field->expObjSystem);
    EncSys_Free(field->encountSystem);
    gameData = GSYS_GetGameData(gsys);
    GameData_SetLastSubscreen(gameData, FieldSubscreen_Free(field->subscreen));
    FieldEffects_Free(field->fieldEffects);
    if (field->unkA0 != NULL) {
        UnionEff_Free(field->unkA0);
        field->unkA0 = NULL;
    }
    func_ov036_021990d8(field->weatherSystem);
    field->weatherSystem = NULL;
    FieldLight_Free(field->lightSystem);
    FieldFogCtrl_Free(field->fogCtrl);
    FieldFog_Free(field->fog);
    FieldPalaceSys_Free(field->palaceSys);
    FieldNoGridMapper_Free(field->noGridMapper);
    FreeFieldSceneAreaLoader(field->sceneAreaLoader);
    FreeFieldSceneArea(field->sceneArea);
    FieldCamera_Free(field->cameraSystem);
    field->ctrlVTable->free(field);
    Field_ResetController(field);
    Field_SuspendActorSystem(field);
    FesGimmick_Free(field->fesGimmick);
    FieldPlayer_Free(field->player);
    FieldG3DMapper_FreeMap(field->g3DMapper);
    if (field->moneyWin != NULL) {
        func_ov036_02187c1c(field->moneyWin);
    }
    if (field->msgBGSys != NULL) {
        func_ov036_021877ac(field->msgBGSys);
    }
    GameData_Set30FPSMode(GSYS_GetGameData(gsys), FALSE);
    GCTX_HIDChangeFPS(60);
    DayCare_Free(field->dayCare);
    FieldSkillMapEff_Free(field->skillMapEff);
    return 1;
}

u32 FieldRoutine_Close(GameSystem *gsys, Field *field) {
    func_0203a610(field->tcbManager);
    GFL_HeapFree(field->tcbManagerHeap);
    FieldDispControl_Free(field->dispControl);
    field->dispControl = NULL;
    FieldTaskManager_Free(field->taskManager);
    GFL_TCBRemove(field->asyncActorMatLoadTCB);
    FieldG3DMapper_Free(field->g3DMapper);
    FieldG3D_Free(field);
    func_0204b758();
    BmpWin_FreeAllocator();
    Field_FreeGraphicsSystems(field);
    if (GetZoneIsUnionRoom(field->zoneId) == TRUE || IsZone150Or151(field->zoneId) == TRUE) {
        GFL_OvlUnload(OVERLAY_ID(34));
    } else {
        GFL_OvlUnload(OVERLAY_ID(33));
    }
    return 2;
}

BOOL Field_CallRoutines(GameSystem *gsys, Field *field) {
    u32 result = 0;
    FieldRoutine routine;

    field->framesSinceStart++;
    routine = FIELDMAP_ROUTINES[field->routineID][field->routineAlternator];
    if (routine != NULL) {
        result = routine(gsys, field);
    }
    switch (result) {
    case 0:
        field->routineAlternator = field->routineAlternator == 0 ? 1 : 0;
        break;
    case 1:
        field->routineID++;
        field->subroutinePhase = 0;
        field->routineAlternator = 0;
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

void Field_RequestClose(Field *field) {
    field->routineState = 2;
    field->routineID = 4;
}

BOOL Field_CheckMapLoadFinished(Field *field) {
    return field->routineState == 1;
}

BOOL Field_ToggleCycling(Field *field) {
    BOOL changed = FALSE;
    u32 exState = FieldPlayer_GetExState(field->player);

    if (exState == 1) {
        FieldPlayer_SetSpecialSeq(field->player, 1);
        changed = TRUE;
    } else if (exState == 0) {
        changed = TRUE;
        GFL_SndSEPlay(SEQ_SE_BICYCLE);
        FieldPlayer_SetSpecialSeq(field->player, 2);
        RecordAddOne(GameData_GetRecords(field->gameData), 3);
    }

    if (changed == TRUE) {
        func_ov036_0219a580(field->player);
    }
    return changed;
}

void *Field_GetMsgBGSys(Field *field) {
    return field->msgBGSys;
}

FieldCamera *Field_GetCameraSystem(Field *field) {
    return field->cameraSystem;
}

NoGridMapper *Field_GetNoGridMapper(Field *field) {
    return field->noGridMapper;
}

void *Field_GetLightSystem(Field *field) {
    return field->lightSystem;
}

FieldFog *Field_GetFog(Field *field) {
    return field->fog;
}

void *Field_GetFogCtrl(Field *field) {
    return field->fogCtrl;
}

void *Field_GetWeatherSystem(Field *field) {
    return field->weatherSystem;
}

u32 Field_GetWeatherForZone(Field *field, u16 zoneId) {
    return GetWeatherAll(field->gameSystem, zoneId);
}

MMSys *Field_GetActorSystem(Field *field) {
    return field->actorSystem;
}

GameSystem *Field_GetGameSystem(Field *field) {
    return field->gameSystem;
}

u16 Field_GetHeapID(Field *field) {
    return field->heapId;
}

void *Field_GetEffectBlAct(Field *field) {
    return field->effectBlAct;
}

void *Field_GetWildEffectBlAct(Field *field) {
    return field->wildEffectBlAct;
}

FieldG3DMapper *Field_GetG3DMapper(Field *field) {
    return field->g3DMapper;
}

u16 Field_GetPlayerStateZoneID(Field *field) {
    return field->playerZoneState.zoneId;
}

void *Field_GetController(Field *field) {
    return field->controller;
}

void Field_SetController(Field *field, void *controller) {
    field->controller = controller;
}

FieldPlayer *Field_GetPlayer(Field *field) {
    return field->player;
}

BOOL Field_HasPlayer(Field *field) {
    return field->player != NULL;
}

FieldSubscreen *Field_GetSubscreen(Field *field) {
    return field->subscreen;
}

void *Field_GetFieldEffects(Field *field) {
    return field->fieldEffects;
}

void *Field_GetG3DObjSys(Field *field) {
    return field->g3DObjSystem;
}

void *func_ov036_0218051c(Field *field) {
    return field->unkA0;
}

EncountSystem *Field_GetEncountSystem(Field *field) {
    return field->encountSystem;
}

u32 Field_GetControllerTypeID(Field *field) {
    return field->ctrlVTable->typeId;
}

u32 Field_GetResolvedControllerTypeID(Field *field) {
    u32 type = sResolvedControllerTypes[field->ctrlVTable->typeId];
    if (type == 2) {
        type = FieldmapCtrlHybrid_GetActiveTypeID(field->controller);
    }
    return type;
}

PlaceName *Field_GetPlaceName(Field *field) {
    return field->placeName;
}

void *Field_GetFesGimmick(Field *field) {
    return field->fesGimmick;
}

FieldAsyncProcManager *Field_GetAsyncProcMgr(Field *field) {
    return field->asyncProcManager;
}

FieldExpObjSystem *Field_GetExpObjSystem(Field *field) {
    return field->expObjSystem;
}

TCBManager *Field_GetTCBMgr(Field *field) {
    return field->tcbManager;
}

FieldTaskManager *Field_GetTaskManager(Field *field) {
    return field->taskManager;
}

void Field_SetPlayerPosPtr(Field *field, VecFx32 *position) {
    field->playerPosPtr = position;
}

void *Field_GetMoneyWin(Field *field) {
    return field->moneyWin;
}

void Field_SetMoneyWin(Field *field, void *moneyWin) {
    field->moneyWin = moneyWin;
}

DayCareSave *Field_GetDayCare(Field *field) {
    return field->dayCare;
}

AreaData *Field_GetAreaData(Field *field) {
    return field->areaData;
}

void FieldG2D_Init(Field *field) {
    GFL_BGSysCreate(field->heapId);
    GFL_BGSysUploadStdPalette(0, FIELD_STD_PALETTE, 0x20, 0);
    GFL_BGSysUploadStdPalette(4, FIELD_STD_PALETTE, 0x20, 0);
    GFL_BGSysSetDisplayLayout(1);
    GFL_BGSysEnableEngines();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
}

void FieldG2D_SetLCDConfig(void) {
    GFL_BGSysSetLCDConfig(&FIELD_LCD_CONFIG);
}

void FieldG2D_Prepare3DSurface(Field *field) {
    MtxFx22 mtx;

    MAT2_SetScaleRot(&mtx, 0, FX32_ONE, FX32_ONE, 0);
    G2_SetBG2Affine(&mtx, 0, 0, 0, 0);
    GFL_BGSysSet3DBGPriority(3);
    func_ov036_02187760(field->msgBGSys);
}

void func_ov036_02180630(Field *field) {
    MtxFx22 mtx;

    MAT2_SetScaleRot(&mtx, 0, FX32_ONE, FX32_ONE, 0);
    G2_SetBG2Affine(&mtx, 0, 0, 0, 0);
    GFL_BGSysSet3DBGPriority(3);
    func_ov036_0218776c(field->msgBGSys);
}

void *FieldG2D_GetDispControl(Field *field) {
    return field->dispControl;
}

void Field_FreeGraphicsSystems(Field *field) {
    GFL_BGSysSetDisplayLayout(1);
    NNS_G3DWaitFIFO();
    GFL_G3DSysFree();
    GFL_BGSysFree();
}

void FieldG3D_InitCallback(void) {
    G3X_SetShading(GX_SHADING_TOON);
    G3X_AntiAlias(TRUE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
    gfxSetFog(FALSE, 0, 5, 0xe00);
    G3X_SetFogColor(GX_RGB(31, 31, 31), 0);
    gfxSetFogTable(FIELD_DEFAULT_FOG_TABLE);
    gfxClearColor(GX_RGB(0, 0, 0), 31, 0x7fff, 0, FALSE);
    gfxSetEdgeColorTable(FIELD_DEFAULT_EDGE_COLOR_TABLE);
    G3X_EdgeMarking(FALSE);
    G3_ViewPort(0, 0, 255, 191);
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_MANUAL, GX_BUFFERMODE_Z);
}

void FieldG3D_Init(Field *field) {
    BlActScene *scene;
    VecFx32 scale;
    VecFx32 up;
    u8 polyId;

    field->actorBlAct = BlActSys_Create(0x40, 0x80, FldActSys_VRAMUploadFunc, field->heapId);
    scene = BlActSys_GetScene(field->actorBlAct);
    BlActScene_SetGeomOrigin(scene, 2);
    BlActScene_SetTexcoordOffset(scene, 0, -0xa00);
    field->effectBlAct = BlActSys_Create(0x3a, 0x50, FldActSys_VRAMUploadFunc, field->heapId);
    field->wildEffectBlAct = BlActSys_Create(0x1e, 0x28, FldActSys_VRAMUploadFunc, field->heapId);
    scene = BlActSys_GetScene(field->wildEffectBlAct);
    scale = data_ov036_021ca010;
    BlActScene_SetScale(scene, &scale);
    polyId = 0;
    BlActScene_SetBasePolyID(scene, &polyId);
    Field_LoadActorMatColorPreset(field);
    field->g3DObjSystem = FieldG3DObjSystem_Create(field->heapId, 0x40, 0x40);
    up = DEFAULT_FIELDCAMERA_UP;
    field->g3dCamera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                           FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0), 0, FX32_ONE,
                                           FX32_CONST(1024), 0, &DEFAULT_FIELDCAMERA_POS, &up,
                                           &DEFAULT_FIELDCAMERA_TGT, field->heapId);
    field->g3dLights = GFL_G3DLightCreate(&FIELD_LIGHT_SETUP, field->heapId);
    GFL_G3DCameraFlush(field->g3dCamera);
    GFL_G3DLightFlush(field->g3dLights);
}

void FieldG3D_Update(Field *field) {
    BOOL loading;

    BlActSys_UpdateActors(field->actorBlAct);
    BlActSys_UpdateActors(field->effectBlAct);
    BlActSys_UpdateActors(field->wildEffectBlAct);
    FieldNoGridMapper_UpdateCamera(field->noGridMapper);
    FieldCameraArea_Update(field->sceneArea, &field->playerPos);
    FieldFogCtrl_Update(field->fogCtrl, field->heapId);
    FieldWeather_Update(field->weatherSystem, 0x92);
    FieldLensFlare_Update(field->lensFlare, field->cameraSystem);
    FieldFog_Update(field->fog);
    FieldLight_Update(field->lightSystem, RTC_ConvertDaySecondsCached());
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    loading = FieldG3DMapper_Update(field->g3DMapper);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    if (loading) {
        func_02042a1c(2);
    }
}

void FieldG3D_RenderPhase1(Field *field) {
    FieldRenderFunc render;

    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
    render = FIELD_RENDER_FUNCS_PHASE1[field->renderMode];
    if (render != NULL) {
        render(field);
    }
}

void FieldG3D_RenderPhase2(Field *field) {
    FIELD_RENDER_FUNCS_PHASE2[field->renderMode](field);
    GFL_G3DSysReqSwapBuffers();
}

void FieldG3D_Free(Field *field) {
    GFL_G3DLightFree(field->g3dLights);
    GFL_G3DCameraFree(field->g3dCamera);
    BlActSys_Free(field->actorBlAct);
    BlActSys_Free(field->effectBlAct);
    BlActSys_Free(field->wildEffectBlAct);
    FieldG3DObjSystem_Free(field->g3DObjSystem);
}

void FldActSys_AsyncMatLoadTCBFunc(TCB *tcb, void *data) {
    Field *field = data;

    if (!GFL_IRQCartDataTransferIsEnabled()) {
        if (field->actorSystem != NULL) {
            FldActSys_FinishAsyncMatLoadSafe(field->actorSystem);
        }
        if (field->g3DObjSystem != NULL) {
            FieldG3DObjSystem_SetupResources(field->g3DObjSystem);
        }
    }
    func_0204b7c8();
    if (field->dispControl != NULL) {
        FieldDispControl_Update(field->dispControl);
    }
}

void FldActSys_VRAMUploadFunc(BOOL type, u32 dest, void *src, u32 size) {
    NNS_GfdRegisterNewVramTransferTask(!type ? NNS_GFD_DST_3D_TEX_VRAM : NNS_GFD_DST_3D_TEX_PLTT, dest, src, size);
}

void Field_LoadEdgeColorTable(AreaData *area, u16 zoneId) {
    u32 tableId = AreaData_GetEdgeColorTableID(area);

    if (tableId == 0xff) {
        G3X_EdgeMarking(FALSE);
        return;
    }
    if (IsZoneEntralinkEdgeColorTable(zoneId)) {
        G3X_EdgeMarking(FALSE);
    } else {
        G3X_EdgeMarking(TRUE);
    }
    GFL_ArcSysRead(g_FieldEdgeColorTable, 0xf, tableId);
    g_FieldEdgeColorTable[7] = 0x4210;
    gfxSetEdgeColorTable(g_FieldEdgeColorTable);
}

void Field_LoadActorMatColorPreset(Field *field) {
    HeapID heapId = HEAPID_TAIL(field->heapId);
    u32 presetId = AreaData_GetActorMatColorID(field->areaData);
    void *preset = FldActMatColorPreset_Create(heapId);

    FldActMatColorPreset_Load(preset, presetId);
    FldActMatColorPreset_Apply(preset, BlActSys_GetScene(field->actorBlAct));
    FldActMatColorPreset_Free(preset);
}

void Field_InitActorSystem(Field *field) {
    GameData *gameData = GSYS_GetGameData(field->gameSystem);
    MMSys *actorSystem = GameData_GetMMSys(gameData);

    field->actorSystem = actorSystem;
    FldActSys_AttachField(actorSystem, field->heapId, gameData, field, field->g3DMapper, field->noGridMapper,
                          field->colorPostFX);
    FldActSys_InitBlAct(field->actorSystem, field->actorBlAct, 0x20);
    FldActSys_LoadCachedBlact(field->actorSystem);
    FldActSys_LoadStaticBlact(actorSystem,
                              FieldPlayer_GetObjCodeByExState(getTrainerGender(GetGameDataPlayerInfo(gameData)), 0));
    FesGimmick_BindActorSystem(field->fesGimmick, actorSystem);
    FieldActorG3DSystem_Create(field->actorSystem, field->g3DObjSystem);
    func_ov012_021667cc(field->actorSystem, func_ov036_021863d8(field->cameraSystem));
    func_ov012_02166d48(field->actorSystem);
    func_ov012_021673e0(actorSystem, func_ov036_021813b8(field->zoneId));
}

void Field_SuspendActorSystem(Field *field) {
    FldActSys_SuspendAllActors(field->actorSystem);
    func_ov012_02166764(field->actorSystem);
    field->actorSystem = NULL;
}

BOOL Field_CheckDoRealTimeLoad(Field *field) {
    EventData *eventData;

    if (Field_UpdateZoneStatePos(field) == TRUE && Field_ShouldSwapZone(field) == TRUE) {
        Field_SwapZoneByMatrix(field);
        return TRUE;
    }
    eventData = GameData_GetEventData(field->gameData);
    if (!IsEncountDataLoaded(eventData)) {
        MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
        EventData_LoadEncData(eventData, field->playerZoneState.zoneId, GameData_GetSeason(field->gameData));
        MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    }
    return FALSE;
}

BOOL Field_UpdateZoneStatePos(Field *field) {
    ZoneSpawnInfo *zoneState = &field->playerZoneState;
    VecFx32 *playerPos = PlayerState_GetWPos(GameData_GetPlayerState(GSYS_GetGameData(field->gameSystem)));
    VecFx32 statePos;

    GetPlayerZoneStateWPos(zoneState, &statePos);
    if (playerPos->x != statePos.x || playerPos->z != statePos.z) {
        SetPlayerZoneStateWPos(zoneState, playerPos);
        return TRUE;
    }
    return FALSE;
}

BOOL Field_ShouldSwapZone(Field *field) {
    ZoneSpawnInfo *zoneState = &field->playerZoneState;
    MapMatrix *matrix = GetMapMatrixSystem(field->gameData);
    VecFx32 pos;

    GetPlayerZoneStateWPos(zoneState, &pos);
    if (RangeCheckChunkCoordinateWorld(matrix, pos.x, pos.z) == TRUE) {
        u16 zoneId = GetZoneIDAtMatrixXZWorld(matrix, pos.x, pos.z);

        if (zoneId != 0xffff && zoneId != zoneState->zoneId) {
            return TRUE;
        }
    }
    return FALSE;
}

void Field_SwapZoneByMatrix(Field *field) {
    ZoneSpawnInfo *zoneState = &field->playerZoneState;
    GameData *gameData = GSYS_GetGameData(field->gameSystem);
    EventData *eventData = GameData_GetEventData(gameData);
    MMSys *actorSystem = field->actorSystem;
    MapMatrix *matrix = GetMapMatrixSystem(field->gameData);
    VecFx32 pos;
    u32 zoneId;

    func_02042a1c(2);
    EncountSystem_CancelPhenomenon(field->encountSystem);
    GetPlayerZoneStateWPos(zoneState, &pos);
    zoneId = GetZoneIDAtMatrixXZWorld(matrix, pos.x, pos.z);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    DeleteAllActors(actorSystem);
    EventData_Reset(eventData);
    EventData_LoadEntities(eventData, zoneId, GameData_GetSeason(gameData));
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    func_ov012_0215ee10(gameData, field);
    Field_SwapZoneBGM(field, zoneId);
    Field_SwapFog(field, zoneId);
    Field_SwapWeather(field, zoneId);
    Field_CheckGiveDiamondDustMedal(field);
    Field_SwapCameraBoundaries(field, zoneId);
    Field_UpdatePlayerStateZoneID(gameData, zoneId);
    Field_LoadSceneArea(field, zoneId);
    if (field->placeName != NULL && !GSYS_GetEventRunningFlag(field->gameSystem)) {
        func_ov036_021b50c8(field->placeName, zoneId);
    }
    zoneState->zoneId = zoneId;
    GameBeacon_SetZone(zoneId, gameData);
    SetTeleportZoneDiscover(gameData, zoneId);
    FieldScript_CallOnZoneInit(field->gameSystem, 4);
    resetRebattleTrainers(GameData_GetEventWork(gameData));
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    Field_HotswapSpawnActors(field, gameData, actorSystem, eventData, zoneId);
    func_ov012_021683f4(field->gameSystem, zoneId);
    func_ov036_021b6660(field->fesGimmick);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    func_ov012_02153668(GSYS_GetGameCommSystem(field->gameSystem));
}

void Field_HotswapSpawnActors(Field *field, GameData *gameData, MMSys *actorSystem, EventData *eventData, u32 zoneId) {
    u32 count = GetZoneNPCsCount(eventData);

    if (count != 0) {
        EventWork *eventWork = GameData_GetEventWork(gameData);

        SpawnAllZoneNPCs(actorSystem, GetZoneNPCs(eventData), zoneId, count, eventWork);
    }
}

void Field_SwapZoneBGM(Field *field, u32 zoneId) {
    GameData *gameData = GSYS_GetGameData(field->gameSystem);
    ISS *iss = GameSystem_GetISS(field->gameSystem);
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);
    u32 exState = FieldPlayerState_GetExState(GameData_GetPlayerState(gameData));

    ISS_ChangeZone(iss, zoneId);
    if (exState != 2) {
        FieldSnd_ChangeZoneBGM(fieldSound, gameData, zoneId);
    }
}

void Field_SwapWeather(Field *field, u32 zoneId) {
    void *weather = Field_GetWeatherSystem(field);
    u16 weatherId;

    ResetWeather(field->gameSystem, zoneId);
    weatherId = GetNowWeather(field->gameData);
    if (weatherId != 15) {
        func_ov036_02199208(weather, weatherId);
    }
}

void Field_CheckGiveDiamondDustMedal(Field *field) {
    if (GetNowWeather(field->gameData) == 8) {
        MedalBox_GiveMedal(SaveControl_GetMedalBox(GameData_GetSaveControl(field->gameData)), 0x65);
    }
}

void Field_SwapFog(Field *field, u32 zoneId) {
    u32 fogIndex;

    FieldFogCtrl_Reset(Field_GetFogCtrl(field));
    fogIndex = GetZoneFogIndexAll(field, zoneId);
    FieldFogCtrl_RequestLoad(field->fogCtrl, fogIndex, GetZoneStaticLightDataIndex(zoneId), TRUE);
}

void Field_UpdatePlayerStateZoneID(GameData *gameData, u32 zoneId) {
    PlayerState_SetZoneID(GameData_GetPlayerState(gameData), zoneId);
}

void Field_SwapCameraBoundaries(Field *field, u32 zoneId) {
    FieldCameraBoundary_ChangeZone(field, zoneId, field->heapId);
}

void Field_ResetController(Field *field) {
    field->controller = NULL;
}

void FieldCameraBoundary_ChangeZone(Field *field, u32 zoneId, HeapID heapId) {
    u16 boundaryId = GetZoneMatrixCamBoundIdx(zoneId);

    if (boundaryId != 0xffff) {
        FieldCameraBoundary_ChangeID(field->cameraSystem, boundaryId, heapId);
    } else {
        FieldCameraBoundary_LoadDummy(field->cameraSystem);
    }
}

void Field_LoadWFBC(GameData *gameData, Field *field, u32 zoneId) {
    FieldStatus *status = GameData_GetFieldStatus(gameData);

    if (IsZoneBlackCityOrWhiteForestLobby(field->zoneId)) {
        BOOL isOther = FieldStatus_GetInLinkedWorld(status);
        CityState *city;

        if (isOther == FALSE) {
            city = GameData_GetMyCityState(field->gameData);
        } else {
            city = func_0201722c(field->gameData, 0);
        }
        func_ov036_021852f4(field->g3DMapper, city, isOther);
    }
}

void Field_LoadJoinAvenue(GameData *gameData, Field *field, u32 zoneId) {
    ResortMapCreateWork *resortMap = func_ov036_02185304(field->g3DMapper);
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
    JoinAvenueInfo *info = JoinAvenue_GetInfo(joinAvenue);
    JoinAvenueOccupants *occupants = getAddressOfBeginningOfOccupants(joinAvenue);

    if (IsZoneJoinAvenue(zoneId)) {
        func_ov036_021c8984(resortMap, zoneId, info, occupants);
        func_ov036_021c8b40(resortMap, GameData_GetEventData(gameData));
    }
}

void Field_LoadSceneArea(Field *field, u32 zoneId) {
    u16 cameraId = GetCameraIDForZone(zoneId);

    if (cameraId != 0xffff) {
        CameraArea *cameraData;
        u32 count;

        ResetSceneArea(field->sceneArea);
        ResetSceneAreaLoader(field->sceneAreaLoader);
        LoadCameraDataToSceneAreaLoader(field->sceneAreaLoader, 1, cameraId, 0x96);
        cameraData = GetFldSceneAreaLoaderCameraData(field->sceneAreaLoader);
        count = GetFldSceneAreaLoaderCamCount(field->sceneAreaLoader);
        BuildSceneArea(field->sceneArea, cameraData, count, GetFldSceneAreaLoaderCamFuncsStaticOffs(field->sceneAreaLoader));
        return;
    }
    if (!GetZoneHasRailSystem(zoneId)) {
        ResetSceneArea(field->sceneArea);
        ResetSceneAreaLoader(field->sceneAreaLoader);
    }
}

u32 func_ov036_02180f80(GameCommSys *comm) {
    u32 state = GameCommSys_BootCheck(comm);

    switch (state) {
    case 0:
        if (GFL_NetErrCheck()) {
            state = GameCommSys_GetLastCommNo(comm);
        }
        return state;
    case 1:
    case 2:
    case 5:
        return state;
    }
    return 2;
}

BOOL func_ov036_02180fc0(GameCommSys *comm) {
    if (GameCommSys_BootCheck(comm) == 0) {
        return TRUE;
    }
    if (!GameCommSys_IsTransitioning(comm)) {
        GameCommSys_ExitReq(comm);
    }
    return FALSE;
}

void func_ov036_02180fe4(FieldGimmickWorkBlock *block) {
    block->password = 0xffffffff;
    block->work = NULL;
}

void *Field_AllocGimmickWorkBlock(Field *field, u32 password, HeapID heapId, u32 size) {
    FieldGimmickWorkBlock *block = &field->gimmickWork;

    if (password == 0xffffffff) {
        return NULL;
    }
    if (block->password != 0xffffffff) {
        return NULL;
    }
    block->password = password;
    block->work = GFL_HeapAllocate(heapId, size, TRUE, "fieldmap.c", 0xe76);
    return block->work;
}

void Field_DeleteGimmickWorkBlock(Field *field, u32 password) {
    FieldGimmickWorkBlock *block = &field->gimmickWork;

    if (password != 0xffffffff) {
        GFL_HeapFree(block->work);
        block->password = 0xffffffff;
    }
}

BOOL Field_CheckGimmickWorkPassword(Field *field, u32 password) {
    if (field->gimmickWork.password == password) {
        return TRUE;
    }
    return FALSE;
}

void *Field_GetGimmickWorkBlock(Field *field, u32 password) {
    FieldGimmickWorkBlock *block = &field->gimmickWork;

    if (password == 0xffffffff) {
        return NULL;
    }
    if (password != block->password) {
        return NULL;
    }
    return block->work;
}

void FieldRenderPhase1_Fieldmap(Field *field) {
    FieldFog_Flush(field->fog);
    FieldLight_Flush(field->lightSystem, FALSE);
    GFL_G3DCameraFlush(field->g3dCamera);
    GFL_G3DLightFlush(field->g3dLights);
    FieldG3D_RenderFieldmap(field->g3DMapper, field->g3dCamera, 0);
}

void FieldRenderPhase2_Fieldmap(Field *field) {
    MtxFx44 saved;
    MtxFx44 mtx;

    if (field->msgBGSys != NULL) {
        func_ov036_0218796c(field->msgBGSys);
    }
    FieldG3D_RenderFieldmap(field->g3DMapper, field->g3dCamera, 1);
    FieldSkillMapEff_Draw(field->skillMapEff);
    saved = NNS_G3dGlb.projMtx;
    mtx = saved;
    mtx.m[3][2] += FX_Mul(mtx.m[2][2], field->objectProjectionMatrixOffset);
    NNS_G3dGlbSetProjectionMtx(&mtx);
    NNS_G3DFlushRenderState();
    NNS_G3DWaitFIFO();
    FieldEffects_Draw(field->fieldEffects);
    FieldG3DObjSystem_Draw(field->g3DObjSystem);
    BlActSys_Draw(field->actorBlAct, field->g3dCamera, field->g3dLights);
    BlActSys_Draw(field->wildEffectBlAct, field->g3dCamera, field->g3dLights);
    if (field->unkA0 != NULL) {
        UnionEff_Draw(field->unkA0);
    }
    mtx = saved;
    mtx.m[3][2] += FX_Mul(mtx.m[2][2], field->objectProjectionMatrixOffset / 2);
    NNS_G3dGlbSetProjectionMtx(&mtx);
    NNS_G3DFlushRenderState();
    NNS_G3DWaitFIFO();
    BlActSys_Draw(field->effectBlAct, field->g3dCamera, field->g3dLights);
    NNS_G3dGlbSetProjectionMtx(&saved);
    NNS_G3DFlushRenderState();
    FieldAsyncProcManager_Draw(field->asyncProcManager);
    FieldWeather_Draw(field->weatherSystem);
    FieldExpObj_Draw(field->expObjSystem);
    func_ov036_021bb674();
    Fld3DCi_Draw(field->g3dCi);
}

void FieldRenderPhase2_FieldEffect(Field *field) {
    gfxClearColor(0, 0, 0x7fff, 0, FALSE);
    func_ov036_021bb674();
    Fld3DCi_Draw(field->g3dCi);
}

void FieldRenderPhase2_EncountEffect(Field *field) {
    gfxClearColor(0x4210, 31, 0x7fff, 0, FALSE);
    EncEff_CallRenderFunc(field->encEff);
}

u32 Field_GetRenderMode(Field *field) {
    return field->renderMode;
}

void Field_SetRenderMode(Field *field, u32 mode) {
    field->renderMode = mode;
}

void *Field_Get3DCi(Field *field) {
    return field->g3dCi;
}

EncEff *Field_GetEncEff(Field *field) {
    return field->encEff;
}

void *Field_GetSkillMapEff(Field *field) {
    return field->skillMapEff;
}

void *Field_GetSceneArea(Field *field) {
    return field->sceneArea;
}

void Field_SetFadeFlag(Field *field, BOOL flag) {
    field->fadeFlag = flag;
}

BOOL Field_GetFadeFlag(Field *field) {
    return field->fadeFlag;
}

BOOL Field_IsEventRunning(Field *field) {
    return GSYS_GetEventRunningFlag(field->gameSystem);
}

u16 Field_GetDayPeriod(Field *field) {
    return GetRealTimeDayPeriod(GameData_GetSeason(field->gameData));
}

BOOL Field_GetSeasonBannerOverdrawFlag(Field *field) {
    return field->seasonBannerOverdrawFlag;
}

void Field_SetSeasonBannerOverdrawFlag(Field *field, BOOL flag) {
    field->seasonBannerOverdrawFlag = flag;
}

void Field_SetEffectRunningFlag(Field *field, BOOL flag) {
    field->effectRunningFlag = flag;
}

void *Field_GetNDemoDataHandle(Field *field) {
    return &field->nDemoData;
}

void Field_SetCasteliaRush(Field *field, CasteliaRush *rush) {
    field->casteliaRush = rush;
}

CasteliaRush *Field_GetCasteliaRush(Field *field) {
    return field->casteliaRush;
}

FieldLensFlare *Field_GetLensFlare(Field *field) {
    return field->lensFlare;
}

void *Field_GetColorPostFX(Field *field) {
    return field->colorPostFX;
}

fx32 func_ov036_02181324(Field *field) {
    return field->actorYOffset;
}

// How far below the camera the actors are drawn, from the camera's pitch
fx32 func_ov036_0218132c(Field *field) {
    u16 *pitch = func_ov036_021863c4(field->cameraSystem);
    fx32 cos = FX_CosIdx(*pitch);
    fx32 offset;

    if (cos < 0) {
        cos = -cos;
    }
    offset = FX_Mul(cos, FX32_CONST(4)) - FX32_CONST(4);
    if (offset < FX32_CONST(-2)) {
        offset = FX32_CONST(-2);
    }
    return offset;
}

u32 GetZoneFogIndexAll(Field *field, u16 zoneId) {
    if (zoneId == 0x78 && !func_ov011_02154e70(field->gameData, 0)) {
        return 0xfffffff;
    }
    return GetZoneFogIndex(zoneId);
}

u32 GetObjectProjectionMatrixOffset(u16 zoneId) {
    if (ZoneData_GetObjectProjectionMatrixType(zoneId) == 1) {
        return 0x1ee;
    }
    return 0x136;
}

BOOL func_ov036_021813b8(u16 zoneId) {
    if (zoneId == 0x249) {
        return FALSE;
    }
    if (IsZone150Or151(zoneId) == TRUE) {
        return FALSE;
    }
    if (zoneId == 0xf1) {
        return FALSE;
    }
    if (zoneId == 0xf2) {
        return FALSE;
    }
    if (zoneId == 0xf3) {
        return FALSE;
    }
    if (zoneId == 0xf4) {
        return FALSE;
    }
    return TRUE;
}

BOOL IsZoneTwoPassLoad(u16 zoneId) {
    if (zoneId == 0x6c)
        return TRUE;
    if (zoneId == 0x249)
        return TRUE;
    if (zoneId == 0x8f)
        return TRUE;
    return FALSE;
}

BOOL func_ov036_0218141c(u16 zoneId) {
    switch (zoneId) {
    case 0x1de:
    case 0x1df:
        return TRUE;
    }
    return FALSE;
}

u32 GetZoneMapType2(u16 zoneId) {
    return GetZoneMapType(zoneId);
}

void SetupLoadZoneMapTypeData(u16 zoneId, AreaData *area, FieldG3DMapperConfig *config, MapMatrix *matrix) {
    u32 type = GetZoneMapType2(zoneId);

    *config = MAP_CONFIGS[type].mapperConfig;
    if (MAP_CONFIGS[type].useMapMatrix) {
        config->matrixWidth = GetMapMatrixWidth(matrix);
        config->matrixHeight = GetMapMatrixHeight(matrix);
        config->chunkIdCount = GetMapMatrixChunkIDCount(matrix);
        config->chunkDatIds = GetMapMatrixChunkIDs(matrix);
    }
    config->isSharedTexResource = TRUE;
    config->textures.arcId = 14;
    config->textures.fileId = AreaData_GetTexSetID(area);
    config->terrainAnmInfo.srtAnimeId = AreaData_GetSRTAnmID(area);
    config->terrainAnmInfo.patAnimeId = AreaData_GetPatAnmID(area);
}

const FieldmapCtrlVTable *GetZoneFieldmapCtrlVTable(u16 zoneId) {
    return MAP_CONFIGS[GetZoneMapType2(zoneId)].ctrlVTable;
}

u32 GetFieldmapZoneHeapSize(u16 zoneId) {
    return MAP_CONFIGS[GetZoneMapType2(zoneId)].heapSize;
}
